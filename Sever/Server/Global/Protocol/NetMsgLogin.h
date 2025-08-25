#pragma once
#include <UserSocket.h>

#pragma pack(push, 1)

//LoginServer�� ����ϴ� �κ��� �����մϴ�.
//��� ���� User�� MainServer�Դϴ�.
//MainServer���� ���������� NetMsgMain.h�� ���ǵǾ� �ֽ��ϴ�.
//�ش� Header ������ User�� �α��� ��û, �α��� ���, Heartbeat ��� ���� �����մϴ�.
//User �������� Mainserver���� ��ŵ� ���ԵǾ� �ֽ��ϴ�.
//DEV�� User ������ Login�Դϴ�.
//Stage �� Live�� User ������ Cert�Դϴ�.

namespace NetLogin
{
	enum eNetResult
	{
		eNetSuccess = 0,
		eMAX = 255
	};
	////////////////////////////////////////////////////
	enum { eRequest_Login_FromUser = 1 };
	struct request_login_fromUser : public PACKET
	{
		char szUserID[32]; // User ID
		char szPassword[32]; // Password
		char szClientVersion[16]; // Client Version
		size_t GetSize() override { return sizeof(request_login_fromUser); }
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Login_FromLogin = 2 };
	struct result_login_fromLogin : public PACKET
	{
		eNetResult eResult; // Result of the login attempt
		size_t GetSize() override { return sizeof(result_login_fromLogin); }
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eInform_Heartbeat_FromUser = 3 };
	struct inform_heartbeat_fromUser : public PACKET
	{
		size_t GetSize() override { return sizeof(inform_heartbeat_fromUser); }
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eRequest_Login_FromLogin = 11 };
	struct request_login_fromLogin : public PACKET
	{
		char szUserID[32]; // User ID
		char szPassword[32]; // Password
		size_t GetSize() override { return sizeof(request_login_fromLogin); }
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Login_FromMain = 12 };
	struct result_login_fromMain : public PACKET
	{
		eNetResult eResult; // Result of the login attempt
		size_t GetSize() override { return sizeof(result_login_fromMain); }
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eRequest_Cert_FromLogin = 13 };
	struct request_cert_fromLogin : public PACKET
	{
		char szUserID[32]; // User ID
		char szCertKey[64]; // Certification Key
		size_t GetSize() override { return sizeof(request_cert_fromLogin); }
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Cert_FromMain = 12 };
	struct result_cert_fromMain : public PACKET
	{
		eNetResult eResult; // Result of the login attempt
		size_t GetSize() override { return sizeof(result_cert_fromMain); }
	};
	////////////////////////////////////////////////////
}

#pragma pack(pop)