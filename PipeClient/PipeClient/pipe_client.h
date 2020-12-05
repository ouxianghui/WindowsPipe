#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <windows.h> 
#include "blockingconcurrentqueue.h"

namespace vi {

	class PipeClient
	{
	public:
		static std::shared_ptr<PipeClient>& instance();

		~PipeClient();

		void startup();
		void shutdown();

		bool send(const std::string& msg);

	protected:
		int loopThreadProc();

	private:
		PipeClient();
		PipeClient(const PipeClient&) = delete;
		PipeClient& operator=(const PipeClient&) = delete;

	private:
		std::atomic_bool m_running;
		std::thread m_loopThread;
		moodycamel::BlockingConcurrentQueue<std::string> m_messageQ;
	};

}



