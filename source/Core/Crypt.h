
#ifndef CryptH
#define CryptH

//-----------------------------------------------------------------------------
#include <windows.h>
#include <wincrypt.h>

#include "Strings.h"



//****************************************************************************
//  BASE64 - ������ �����������/������������� BASE64
//****************************************************************************

namespace BASE64
{
	// ����������� ������ � ������ ������� BASE64
	PCHAR Encode(LPBYTE Buf, DWORD BufSize);

	// ��c��������� ������ �� ������� BASE64
	// ������ ��������� ������ ����� ������ �������� STR::Length,
	// ���� �������� ��������� �� ����������, ���� ��� ���������
	// ��������� ���������� �������� STR::Free()
	PCHAR Decode(PCHAR Buf, DWORD *ResultSize = NULL);
}



// ����� �����������
typedef bool (*TCryptMethod)(LPVOID Key, LPBYTE Buffer, DWORD BufferSize, PDWORD ResultBufferSize);


//****************************************************************************
//  ������ ��� ���������� XOR ����������
//  ���������� � ����������� ����������� ������ � ����-�� ��������
//****************************************************************************
namespace XORCrypt
{
	// ���������� XOR ����������
	DWORD Crypt(PCHAR Password, LPBYTE Buffer, DWORD Size, BYTE Delta = 0);

	// ��� ������������ TCryptMethod
	bool Crypt(LPVOID Password, LPBYTE Buffer, DWORD Size, PDWORD OutSize);


	//----------------------------------------------------------------
	//   DecodeBuffer - ������� ���������� ����� ���������� � ����
	//                   ������
	//   ������ ������
	//   [���������](DWORD ������ ������)(������)(������)
	//
	//
	//  Signature - ��������� ������. NULL ���� �����������
	//
	//  Buffer - ����� ������������� ������
	//
	//   Size - ������ ������. ����� ������ ������� ����� ���������
	//			������ �������������� ������
	//
	//  ��������� - ������� ���������� ��������� �� ������ ������
	//              ��������������� ������.
	//  ������ ��� ��������� �� ����������!
	//----------------------------------------------------------------
	LPBYTE DecodeBuffer(PCHAR Signature, LPVOID Buffer, DWORD &Size);

	//---------------------------------------------------------
	//  EncodeString  ������� �������� ������. ������������
	//                ������ ������ 255 ����
	//  ���������� ����� ������ ������ ��������� ����� ��������
	//  ������. �.� ������ ������������ ���� �� ������� ������
	//  �� ���� ���� � ������ ������ ���������� ������
	//  �������� ������. �� ��� �������� ������ ��������������
	//  ������ ���� �� ���� ���� ������!!!!
	//---------------------------------------------------------
	string EncodeString(const char* Password, const char* Str);

	//---------------------------------------------------------
	//  DecodeString ������� ��������� ������ , ��� ����
	// ����������� �������� EncodeString
	//---------------------------------------------------------
	string DecodeString(const char* Password, const char* Str);
}


//****************************************************************************
//  ������ ��� ���������� �������� WIN Crypt
//  ����� ������ �� ���������� ��������� (� - ����):
//
//  (4� ������� �������� IV)(BASE64 ������)(4� ������ �������� IV)(��������� BASE64 ==)
//
//****************************************************************************


//  ������������� ������ ������ ��� ��������� RC2
#define RC2_DEFAULT_PASSWORD_SIZE 16


namespace RC2Crypt
{
	// ������� ��������� ������ ������������� ����� �� ������
	PCHAR ExtractIV(PCHAR Buf, DWORD StrLen);

	// ������������ ��������� ������
	PCHAR GenerateIV();

	// ���������� �����
    PCHAR Encode(LPBYTE Buf, DWORD BufSize,  PCHAR Password);

	// ������������ �����
	// �� ����� ������ BASE64 �������� � ������ � ��� �������� IV
	// BufSize - ������ ������������� ������.
	// ���� �������� ������� ��������, �� ������ ������
	// ����� ��������� �� �������� �������.
	// ����� ��������� ������ BufSize ����� ��������� �� ������
	// ��������������� ������
	bool Decode(PCHAR Password, PCHAR Buf, DWORD &BufSize);

	// ������� ���������� ������ � ���������� � ������
    bool DecodeStr(PCHAR Password, PCHAR Str, DWORD &Size);

	// ��������� ����� ��������� Win Crypto API
    // ������� �������� ����������� ����� ������ ������� MemAlloc()
	LPBYTE WinEncode(LPBYTE Buf, DWORD &BufSize, PCHAR Password, PCHAR IV);

	// ������������ ����� ��������� Win Crypto API
	PCHAR WinDecode(PCHAR Buf, DWORD &BufSize, PCHAR Password, PCHAR IV);

	// ������������ ���� ����������� �� ������ ������ � ������� ������������� IV
	bool GenerateKey(PCHAR Password, PCHAR IV, HCRYPTPROV &Provider, HCRYPTKEY &Key);


    PCHAR CryptByUID(LPVOID Buffer );
}


//****************************************************************************
//  ������ ��� ���������� ������ ������
//****************************************************************************
namespace CryptFile
{
	//----------------------------------------------------------
	//  WriteFromBuffer - ������� ������������� ����������
	//                    ������ � ���������� ��� � �����
	//----------------------------------------------------------
	DWORD WriteFromBuffer(PCHAR FileName, LPVOID Buffer, DWORD BufferSize, PCHAR Password);

	//----------------------------------------------------------
	//  ReadToBuffer - ������� ������ ���������� �����,
	//				   �������������� ��� � ���������� ���������
	//----------------------------------------------------------
    LPVOID ReadToBuffer(PCHAR FileName, LPDWORD BufferSize, PCHAR Password);
}


//****************************************************************************
//  ������ ��� ���������� ������ ��������������� ���� (UID) ������ ������
//  ���������� ������������ ��������  RC2Crypt
//
//  ��������� ������� ���������� � ���, ��� ������ ������ ��������� ��
//  ����� ����������, �� ���������� �� ������ �� ������ �����������
//****************************************************************************
namespace UIDCrypt
{
	//------------------------------------------------------------------
	//  GeneratePassword - ������� ���������� ������ �� ������ UID
	//------------------------------------------------------------------
	PCHAR GeneratePassword();

	//------------------------------------------------------------------
	//  ������� ������� ������ ������ ����
	//
	//  Data - ������ ����������� �����������
	//
	//  DataSize - ������ ������
	//
	//  Vector - �������������� ������ ����������. ���� �� ������� ��
	//			 ����� ����������� ����������� ������
	//------------------------------------------------------------------
	PCHAR Crypt(LPVOID Data, DWORD DataSize, PCHAR Vector);

	//------------------------------------------------------------------
	//  CryptFileName - ������� �������� ��� ����� ����� ��� ����������
	//				    ��� ������� ����������. ���� � ����� ��
	//					���������
	//
	//  FileName - �������� ��� �����
	//
	//  CryptExt - ������� ������������� ��������� ���������� �����
	//------------------------------------------------------------------
	PCHAR CryptFileName(const char* FileName, bool CryptExt);

	//------------------------------------------------------------------
	// ConvertFileNameChars - ������� ����������� ������������ �������
	//                        ����� ����� ����������� ���� ����������
	//------------------------------------------------------------------
    void ConvertFileNameChars(PCHAR Name);
}



//*********************************************************************
//  ������� ����� ��� ���������� ��������� CryptoApi
//*********************************************************************
class TWinCrypt : public TBotObject
{
protected:
	HCRYPTPROV FProvider;
	bool       FProviderAssigned;
	HCRYPTKEY  FKey;
	void InitializeProvider(HCRYPTPROV Provider, const char* Container, DWORD Flags);
	void DestroyKey();
protected:
	LPBYTE DoExportKey(HCRYPTKEY ExpKey, DWORD BlobType, DWORD *BufSize);
	bool   DoImportKey(HCRYPTKEY ExpKey, DWORD BlobType, LPBYTE Buf, DWORD BufSize);
public:

	TWinCrypt(const char *Container, DWORD Flags);
	TWinCrypt(HCRYPTPROV Provider);
	~TWinCrypt();
    bool GenerateKey(DWORD AlgId, DWORD Flags);
	bool CreateRC4Key(const char *Password);

    HCRYPTHASH HashData(DWORD Algoritm, const void* Data, DWORD DataLen);

	LPBYTE ExportPrivateKey(const char *ExpPassword, DWORD *BufSize);
	LPBYTE ExportPublicKey(DWORD *BufSize);
	bool   ImportPrivateKey(const char *ExpPassword, LPBYTE Buf, DWORD BufSize);
	bool   ImportPublicKey(LPBYTE Buf, DWORD BufSize);


	//--------------------------------------------------------
	// Encode - ������� ������� ������
	// Data - ������ ��� ����������
	// DataSize - ������ ������.
	// � ������ ������ ������� ������� ������ ��� �����������
	// ����� � ������ �� ���� ���������. ������ ������
	// ��������� � ���������� DataSize
	//--------------------------------------------------------
	LPBYTE Encode(LPBYTE Data, DWORD &DataSize);

	//--------------------------------------------------------
	//  Decode - ������� �������������� ������.
	//  Data - ��������� �� ���� ����������� ������
	//  DataSize - ������ ������.
	//  � ������ ������ ������� ������ ����� ������ ������ �
	//  ���������� DataSize
	//--------------------------------------------------------
	bool Decode(LPBYTE Data, DWORD &DataSize);

	HCRYPTPROV inline Provider()  { return FProvider; }
	HCRYPTKEY  inline Key()       { return FKey; }
};


//-----------------------------------------------------------------------------
#endif