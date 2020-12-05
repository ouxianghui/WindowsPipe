// main.cpp : Defines the entry point for the console application.
//

#include "pipe_server.h"

int main(VOID)
{
	vi::PipeServer::instance()->startup();

	while (1) {
		Sleep(1000);
	}
	return 0;
}


