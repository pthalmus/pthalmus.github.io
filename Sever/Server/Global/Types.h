#pragma once
#include<magic_enum/magic_enum.hpp>

namespace ServerType {
	enum en {
		MainServer,
		UserServer,
		ChatServer,
		LoginServer,
		MemCachedServer,
		eMAX
	};
}

namespace LogType {
	enum en {
		SystemLog,
		ErrorLog,
		eMAX
	};
}