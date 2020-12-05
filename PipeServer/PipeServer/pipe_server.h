#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <windows.h> 
#include "blockingconcurrentqueue.h"

class IIncomingMessageObserver {
public:
	virtual ~IIncomingMessageObserver() = default;

	virtual void onMessage(const std::string& message) = 0;
};

namespace vi {

	class PipeServer
	{
	public:
		static std::shared_ptr<PipeServer>& instance();

		~PipeServer();

		void addObserver(std::shared_ptr<IIncomingMessageObserver> observer);
		void removeObserver(std::shared_ptr<IIncomingMessageObserver> observer);

		void startup();
		void shutdown();

	protected:
		void listenThreadProc();
		void callbackThreadProc();
		static DWORD WINAPI instanceThread(LPVOID);
		static VOID processData(LPTSTR, LPTSTR, LPDWORD);

	private:
		PipeServer();
		PipeServer(const PipeServer&) = delete;
		PipeServer& operator=(const PipeServer&) = delete;

	private:
		std::mutex m_observerMutex;
		std::vector<std::weak_ptr<IIncomingMessageObserver>> m_observers;

		std::atomic_bool m_running;
		std::thread m_listenThread;

		// key: thread id, value: pipe handle
		std::unordered_map<DWORD, HANDLE> m_pipeMap;
		std::mutex m_pipeMapMutex;

		moodycamel::BlockingConcurrentQueue<std::string> m_messageQ;
		std::thread m_callbackThread;
	};

}

