

//---------------------------------------------------------------------------
//
// ������ ��� ������ � ������������ �������� ������� ��� �����������
// �������������� ����� ���������� ����
//
// ������ 1.0
//
//---------------------------------------------------------------------------

#ifndef PipesH
#define PipesH

//---------------------------------------------------------------------------

#include <windows.h>
#include "BotClasses.h"


// �������� ���������
typedef struct TProcessPipe
{
	PCHAR  Name;                // ��� ������
	HANDLE Handle;              // ������������� ������
	PList  Handlers;            // ������ ������������������ ������������ ���������
	RTL_CRITICAL_SECTION Lock;  // ��������� ���������� ������
	HANDLE Event;               // ��� ����������� �������������
    bool Terminated;            // ������� ����, ��� �������� ������ ������
}*PProcessPipe;



typedef struct TPipeMessage
{
	DWORD PID;         // ������������� ��������, ����������� ���������
	PCHAR ProcessName; // ��� ��������, ����������� ���������
	DWORD MsgSize;     // ������ ���������
	PCHAR Message;     // ����� ���������
	DWORD MessageHash; // ��� ���������
	PCHAR Data;        // ������ ���������
	DWORD DataSize;    // ������ ������
} *PPipeMessage;


//------------------------------------------------------------
// TPipeMessageHandler - �������, ���������� ���������
//						 ������������ ������
//
// Data - ������ ������� ������������ ��� �����������
//		  �����������
//
// Message - ��������� �� ��������� �������������� ���������
//
// Cancel - �������� �������� ���������� ��������� ���������.
//------------------------------------------------------------
typedef void (WINAPI *TPipeMessageHandler)(LPVOID Data, PPipeMessage Message, bool &Cancel);


//*************************************************************
//  ������ ��� ������ � ������������ ��������
//*************************************************************
namespace PIPE
{
	//-----------------------------------------------------------
	//  Create - ������� ������ ������ ������������ ������
	//
	//  Name - �������� ��� ������. WIN API ��� ������ �
	//         ������������ �������� ������ � ������ �������
	//         ������ ����. ��� ������ ������� ����� �������
	//         ������ ��� ������, ��� ���� � ���������� �������.
	//
	//  ���������: � ������ ������ ������� ����������
	//             ������������� ������. � ������ �������
	//			   ������� ���������� NULL
	//-----------------------------------------------------------
	HANDLE Create(PCHAR Name);


	//-----------------------------------------------------------
	//  CreateProcessPipe - ������� ������ ���������� �����
	//					    �������� � ��������� �������
	//				        �������� ���������
	//
	//  PipeName - ��� ������
	//
	//  StartPipe - ������� ������������ ������ ������
	//
	//  ���������: � ������ ������ ������ ���������� ���������
	//			   �� ��������� ������
	//-----------------------------------------------------------
	PProcessPipe        CreateProcessPipe(const PCHAR PipeName, bool StartPipe);
	PProcessPipe inline CreateProcessPipe(const string &PipeName, bool StartPipe) { return CreateProcessPipe(PipeName.t_str(), StartPipe); }

	//-----------------------------------------------------------
	//  StartProcessPipe - ������� �������� ������ ������
	//-----------------------------------------------------------
	bool StartProcessPipe(PProcessPipe Pipe);

	//-----------------------------------------------------------
	//  FreeProcessPipe ������� ���������� ���� ��������
	//-----------------------------------------------------------
	void FreeProcessPipe(PProcessPipe Pipe);

	//-----------------------------------------------------------
	//  InitializeProcessPipe - ������� ��������������� �����
	//						    ��������.
	//-----------------------------------------------------------
   //	bool InitializeProcessPipe(PCHAR Name, PProcessPipe Pipe);

	//-----------------------------------------------------------
	//  StartProcessPipe - ������� �������� ����� ����� ������
	//
	//
	//  Pipe - ����� ��������� �������� InitializeProcessPipe
	//-----------------------------------------------------------
   //	void StartProcessPipe(PProcessPipe Pipe);

   	//-----------------------------------------------------------
	//  SendMessage - ������� ���������� ���������� ������
	//				 ���������
	//-----------------------------------------------------------
	bool        SendMessage(PCHAR PipeName, PCHAR Msg, PCHAR Data, DWORD DataSize, PPipeMessage Received);
	bool        SendMessage(PCHAR PipeName, PCHAR Msg);
	bool inline SendMessage(const string &PipeName, const string &Msg) { return SendMessage(PipeName.t_str(), Msg.t_str());}

	//-----------------------------------------------------------
	//  Ping - ������� ��������� ������������� ������ PipeName
	//
	//  ���������:
	//
	//  � ������ ������ ������� ������ PID �������� �������
	//  �������.
	//  � ������ ������� ������� ������ ����
	//-----------------------------------------------------------
	DWORD Ping(PCHAR PipeName);

	//-----------------------------------------------------------
	//  RegisterMessageHandler - ������� ������������ ����������
	//							 ���������
	//  PProcessPipe - ���������� ������� �����.
	//
	//  Handler - ���������� ���������
	//
	//  DATA - ������, ������� ����� �������� � ����������
	//
	//  Message - ��������� ��� �������� ��������������
	//			  ����������. �� ������������ ���������
	//
	//  MessageHash - ��� �������� ��� �������� ��������������
	//				  ����������. ������� 0 ���� �������� ��
	//				  �����.
	//
	//  ���� ������� Message ��� MessageHash �� ����������
	//  ����� ���������� ������ ��� ���������� ���������, �
	//  ��������� ������ ���������� ����� ���������� ���
	//  ���� ���������.
	//  ��� ��������� ��������� ����������� �������� Message,
	//  ���� ��� �� ������� (NULL) �� ����������� MessageHash.
	//
	//-----------------------------------------------------------
	bool RegisterMessageHandler(PProcessPipe Pipe, TPipeMessageHandler Handler,
								LPVOID Data, PCHAR Message, DWORD MessageHash);

}


//---------------------------------------------------------------------------
#endif
