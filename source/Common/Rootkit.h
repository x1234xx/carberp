#include <windows.h>

#ifndef RootkitH
#define RootkitH
//----------------------------------------------------------------------------

void HookZwQueryDirectoryFile();
void HookZwResumeThread();
void HookZwOpenFile();
void HookZwCreateFile();

DWORD WINAPI RootkitThread( LPVOID lpData, LPVOID, LPVOID );


void ProtectPage( LPVOID lpAddr, DWORD dwParams );

//----------------------------------------------------------------------------
#endif
