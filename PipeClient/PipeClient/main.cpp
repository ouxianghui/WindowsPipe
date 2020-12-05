// PipeClient.cpp : Defines the entry point for the console application.
//

#include "pipe_client.h"
#include <string>
#include <chrono>
int main()
{
	vi::PipeClient::instance()->startup();

	int counter = 0;
	while (1)
	{
		std::string msg = "message: " + std::to_string(++counter);
		vi::PipeClient::instance()->send(msg);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return 0;
}

