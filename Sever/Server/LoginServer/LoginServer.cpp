#include<iostream>
#include <thread>

bool WINAPI Release(DWORD dwType)
{
	return GetMainThread().Release(dwType);
}

int main()
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)Release, TRUE);
}
