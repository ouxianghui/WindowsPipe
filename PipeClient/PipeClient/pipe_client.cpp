#include "pipe_client.h"
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <string>
#include <vector>

#define BUFSIZE 2048

namespace {
	std::wstring toUTF8(const std::string& str)
	{
		if (str.empty()) {
			return std::wstring();
		}

		size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
		if (charsNeeded == 0) {
			throw std::runtime_error("Failed converting UTF-8 string to UTF-16");
		}

		std::vector<wchar_t> buffer(charsNeeded);
		int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &buffer[0], (int)buffer.size());
		if (charsConverted == 0) {
			throw std::runtime_error("Failed converting UTF-8 string to UTF-16");
		}

		return std::wstring(&buffer[0], charsConverted);
	}
}

namespace vi {

	std::shared_ptr<PipeClient>& PipeClient::instance()
	{
		static std::shared_ptr<PipeClient> _instance;
		static std::once_flag ocf;
		std::call_once(ocf, []() {
			_instance.reset(new PipeClient());
		});
		return _instance;
	}

	PipeClient::PipeClient()
		: m_running(false)
	{

	}

	PipeClient::~PipeClient()
	{
		if (m_loopThread.joinable()) {
			m_loopThread.join();
		}
	}

	void PipeClient::startup()
	{
		if (m_running) {
			return;
		}

		m_loopThread = std::thread([this]() {
			this->loopThreadProc();
		});
	}

	void PipeClient::shutdown()
	{
		if (m_loopThread.joinable()) {
			m_loopThread.join();
		}
		m_running = false;
	}

	bool PipeClient::send(const std::string& msg)
	{
		if (!m_running || m_messageQ.size_approx() >= 1000) {
			return false;
		}
		return m_messageQ.try_enqueue(msg);
	}

	int PipeClient::loopThreadProc()
	{
	_BEGIN:
		m_running = true;
		HANDLE hPipe;
		LPTSTR lpvMessage = (LPTSTR)TEXT("Default message from client.");
		TCHAR chBuf[BUFSIZE];
		BOOL fSuccess = FALSE;
		DWORD cbRead, cbToWrite, cbWritten, dwMode;
		LPTSTR lpszPipename = (LPTSTR)TEXT("\\\\.\\pipe\\vi-message-named-pipe");

		//if (argc > 1) {
		//	lpvMessage = argv[1];
		//}

		// Try to open a named pipe; wait for it, if necessary. 

		while (1) {
			hPipe = CreateFile(
				lpszPipename,   // pipe name 
				GENERIC_READ |  // read and write access 
				GENERIC_WRITE,
				0,              // no sharing 
				NULL,           // default security attributes
				OPEN_EXISTING,  // opens existing pipe 
				0,              // default attributes 
				NULL);          // no template file 


			// Break if the pipe handle is valid. 

			if (hPipe != INVALID_HANDLE_VALUE) {
				break;
			}

			// Exit if an error other than ERROR_PIPE_BUSY occurs. 

			if (GetLastError() != ERROR_PIPE_BUSY) {
				_tprintf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
				//return -1;
				Sleep(1000 * 3);
				goto _BEGIN;
			}

			// All pipe instances are busy, so wait for 20 seconds. 

			if (!WaitNamedPipe(lpszPipename, 20000)) {
				printf("Could not open pipe: 20 second wait timed out.");
				//return -1;
				Sleep(1000 * 3);
				goto _BEGIN;
			}
		}

		// The pipe connected; change to message-read mode. 

		dwMode = PIPE_READMODE_MESSAGE;
		fSuccess = SetNamedPipeHandleState(
			hPipe,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL);    // don't set maximum time 

		if (!fSuccess) {
			_tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
			//return -1;
			CloseHandle(hPipe);
			goto _BEGIN;
		}

		// Send a message to the pipe server. 

		while (1) {
			std::string msg;
			if (!m_messageQ.wait_dequeue_timed(msg, std::chrono::seconds(1))) {
				continue;
			}
#ifdef UNICODE
			std::wstring wmsg = toUTF8(msg);
			lpvMessage = (LPTSTR)wmsg.c_str();
#else
			lpvMessage = (LPTSTR)msg.c_str();
#endif
			cbToWrite = (lstrlen(lpvMessage) + 1) * sizeof(TCHAR);
			_tprintf(TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage);

			fSuccess = WriteFile(
				hPipe,                  // pipe handle 
				lpvMessage,             // message 
				cbToWrite,              // message length 
				&cbWritten,             // bytes written 
				NULL);                  // not overlapped 

			if (!fSuccess) {
				_tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
				break;
				//return -1;
			}

			printf("\nMessage sent to server, receiving reply as follows:\n");

			do {
				// Read from the pipe. 

				fSuccess = ReadFile(
					hPipe,    // pipe handle 
					chBuf,    // buffer to receive reply 
					BUFSIZE * sizeof(TCHAR),  // size of buffer 
					&cbRead,  // number of bytes read 
					NULL);    // not overlapped 

				if (!fSuccess && GetLastError() != ERROR_MORE_DATA) {
					break;
				}

				_tprintf(TEXT("\"%s\"\n"), chBuf);
			} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

			if (!fSuccess) {
				_tprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
				//return -1;
				break;
			}
		}

		printf("\n<End of message, press ENTER to terminate connection and exit>");

		CloseHandle(hPipe);

		goto _BEGIN;

		return 0;
	}

}