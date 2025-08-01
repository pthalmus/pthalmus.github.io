#include "pch.h"

bool CreateNestedDirectoryA(const std::string& relativePath)
{
    char buffer[MAX_PATH];
    DWORD len = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (len == 0 || len > MAX_PATH)
    {
        std::cerr << "���� �۾� ���͸� ��ȸ ����\n";
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
                    std::cerr << "���� ���� ����: " << tempPath << " (���� �ڵ�: " << err << ")\n";
                    return false;
                }
            }
        }
    }

    // ������ ��ü ��ε� ����
    if (GetFileAttributesA(fullPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryA(fullPath.c_str(), NULL))
        {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
            {
                std::cerr << "���� ���� ���� ����: " << fullPath << " (���� �ڵ�: " << err << ")\n";
                return false;
            }
        }
    }

    return true;
}
