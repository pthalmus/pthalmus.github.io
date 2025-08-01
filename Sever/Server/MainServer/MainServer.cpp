#pragma once

#include "Mainthread.h"

bool WINAPI Release(DWORD dwType)
{
	return GetMainThread().Release(dwType);
}

int main()
{
	std::thread tMainThread(&Mainthread::StartMainThread, &GetMainThread());
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)Release, TRUE);

	tMainThread.join();
	return 0;
}