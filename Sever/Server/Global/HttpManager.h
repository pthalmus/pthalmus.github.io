#pragma once

#include"Singleton.h"
#include<string>
#include<mutex>
#include<queue>
#include<functional>
#include<map>

#include <Types.h>
#include <json/json.h>
#include <curl/curl.h>

struct httpRequest
{
	HttpMethod::en						enMethod;
	std::string								strUrl;
	std::string								strBody;
	std::map<std::string, std::string>	mapHeaders;
	std::map<std::string, std::string>	mapParams;
	long										nTimeOut;
};

struct HttpResponse {
	long nRet = 0;
	std::string body;
};

class HttpManager : public Singleton<HttpManager>
{
public:
	HttpManager();
	bool init();
	void Release();

	HttpResponse Run(const httpRequest* psRequest);
	size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

	Json::Value serialize(const httpRequest& request);
	Json::Value serializeMap(const std::map<std::string, std::string>& mapData);
};

#define GetHttpManager() HttpManager::Instance()