//---------------------------------------------------------------------------
//  ������ �������� �������� ������� ������ ����������
//
//  ������ 1.0
//  ����: ���� 2012
//
//
//  �����!
//
//  ��� ������������� ��������� ������ ������� AZ ������������ ����
//  ������ ���� ������� � ��������� ���� ������� Modules.h, � ���������
//  ������ ����� �������������� �������� ��������� ����
//---------------------------------------------------------------------------

#ifndef AzConfigH
#define AzConfigH
//---------------------------------------------------------------------------


#include "Strings.h"

//----------------------------------------------------------
// ������������ ������ ������ �������� ������ ������� AZ
//----------------------------------------------------------
#define AZ_HOSTS_SIZE 512


//----------------------------------------------------
// GetAzHost - ������� ���������� ������ ������� ����
// �� ������� ������ ������� AZ
//----------------------------------------------------
string GetAzHost();

//----------------------------------------------------
//  GetAzURL - ������� ���������� ������ ����� ��
//   ������ ����� ������� � ����������� ����
//----------------------------------------------------
string GetAzURL(const char*  Path);

//---------------------------------------------------------------------------
#endif
