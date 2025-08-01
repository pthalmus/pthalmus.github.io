#include "pch.h"

bool CreateNestedDirectoryA(const std::string& relativePath)
{
    char buffer[MAX_PATH];
    DWORD len = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (len == 0 || len > MAX_PATH)
    {
        std::cerr << "현재 작업 디렉터리 조회 실패\n";
        return false;
    }

    std::string fullPath = std::string(buffer) + "\\" + relativePath;

    size_t pos = 0;
    std::string tempPath;
    while ((pos = fullPath.find('\\', pos)) != std::string::npos)
    {
        tempPath = fullPath.substr(0, pos++);
        if (tempPath.empty()) continue;

        DWORD attr = GetFileAttributesA(tempPath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES)
        {
            if (!CreateDirectoryA(tempPath.c_str(), NULL))
            {
                DWORD err = GetLastError();
                if (err != ERROR_ALREADY_EXISTS)
                {
                    std::cerr << "폴더 생성 실패: " << tempPath << " (오류 코드: " << err << ")\n";
                    return false;
                }
            }
        }
    }

    // 마지막 전체 경로도 생성
    if (GetFileAttributesA(fullPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryA(fullPath.c_str(), NULL))
        {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
            {
                std::cerr << "최종 폴더 생성 실패: " << fullPath << " (오류 코드: " << err << ")\n";
                return false;
            }
        }
    }

    return true;
}
