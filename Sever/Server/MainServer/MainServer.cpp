#pragma once
#include <ThreadPool.h>
#include "Mainthread.h"

bool WINAPI Release(DWORD dwType)
{
	return GetMainThread().Release(dwType);
}

int main()
{
	//메인 스레드 시작
	GetThreadPool().init(MAX_THREAD_CNT);
	GetThreadPool().enqueue([]() { GetMainThread().StartMainThread(); });
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)Release, TRUE);
	
	//종료 대기
	while(GetMainThread().IsRunning())
	{
		Sleep(1000);
	}
	Release(CTRL_C_EVENT);
	GetThreadPool().Stop();
	return 0;
}