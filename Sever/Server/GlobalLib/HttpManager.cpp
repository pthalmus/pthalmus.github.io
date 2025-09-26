#include "pch.h"

HttpManager::HttpManager()
{
}

bool HttpManager::init()
{
	return true;
}

void HttpManager::Release()
{
}

size_t HttpManager::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

HttpResponse HttpManager::Run(const httpRequest* psRequest)
{
	HttpResponse response;
	CURL* curl = curl_easy_init();
	if (!curl) return response;

	Json::Value jsonRequest = serialize(*psRequest);
	std::string jsonString = Json::writeString(Json::StreamWriterBuilder(), jsonRequest);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    for (const auto& pair : psRequest->mapHeaders)
    {
        std::string headerString = pair.first + ": " + pair.second;
        headers = curl_slist_append(headers, headerString.c_str());
    }

    // URL 설정
    curl_easy_setopt(curl, CURLOPT_URL, jsonRequest["url"].asString().c_str());
    // 헤더 설정
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // POST 설정
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    // 핵심: Body에 직렬화된 JSON 문자열 전달
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());
    // 타임아웃 설정
    if (jsonRequest["timeout"].asInt64() > 0)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, jsonRequest["timeout"].asInt64());
    }
    // 응답 처리 설정
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &HttpManager::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

    // 요청 실행
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.nRet);
    }
    else {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    // 리소스 정리
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

Json::Value HttpManager::serialize(const httpRequest& request)
{
	Json::Value root;
	root["method"] = std::string(magic_enum::enum_name(request.enMethod));
	root["url"] = request.strUrl;
	root["body"] = request.strBody;
	root["timeout"] = static_cast<Json::Int64>(request.nTimeOut);

	root["header"] = serializeMap(request.mapHeaders);
	root["params"] = serializeMap(request.mapParams);

	return root;
}

Json::Value HttpManager::serializeMap(const std::map<std::string, std::string>& mapData)
{
	Json::Value root;
	for (auto& iter : mapData)
	{
		root[iter.first] = iter.second;
	}
	return root;
}

