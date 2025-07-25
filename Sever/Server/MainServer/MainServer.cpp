#pragma once

#include<iostream>
#include <thread>

#include "Mainthread.h"

int main()
{
	std::thread tMainThread(&Mainthread::StartMainThread, &GetMainThread());

	tMainThread.join();
	return 0;
}