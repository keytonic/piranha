#include <windows.h>
#include <Shellapi.h>
#include <tlhelp32.h>
#include "patches.h"
#include "resources/resource.h"

#define EnumProcessModules(a,b,c,d) ((int (__stdcall *)(void *,void*,unsigned long,unsigned long*))GetProcAddress(LoadLibrary("psapi.dll"),"EnumProcessModules"))(a,b,c,d)
#define GetModuleBaseNameA(a,b,c,d) ((unsigned long (__stdcall *)(void *,void*,char*,unsigned long))GetProcAddress(LoadLibrary("psapi.dll"),"GetModuleBaseNameA"))(a,b,c,d)

#define WM_TRAYMSG	WM_APP
#define WM_ABOUT	WM_APP + 1
#define WM_EXIT		WM_APP + 2

NOTIFYICONDATA	niData;

bool bInjected = 0;

Patches UT4;

void CheckInstance()
{
	HANDLE handle = CreateMutex(0,true,__DATE__ __TIME__);
	if(GetLastError() != ERROR_SUCCESS)
		exit(0);
}

DWORDLONG FindModule (HANDLE hProcess, const char* sModule)
{
	HMODULE hMod[1024];
	DWORD cbNeeded;

	if( EnumProcessModules( hProcess, hMod, sizeof(hMod), &cbNeeded) )
    {
        for ( unsigned int i=0 ; i < (cbNeeded / sizeof(HMODULE)) ; i++ )
        {
            char sModName[MAX_PATH];

            GetModuleBaseNameA( hProcess, hMod[i], sModName, sizeof(sModName) );

			if ( !_stricmp(sModName, sModule) )
			{
				return (DWORDLONG)hMod[i];
			}
        }
    }

	return 0;
}

DWORD FindProcess (const char* sProcess)
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE ) return 0;

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

    if( Process32First( hProcessSnap, &pe32 ) )
    {
        do
        {
			if ( !_stricmp(pe32.szExeFile, sProcess) )
			{
				return pe32.th32ProcessID;
			}
        }
        while( Process32Next( hProcessSnap, &pe32 ) );
    }

    CloseHandle( hProcessSnap );

    return 0;
}


DWORD __stdcall LoaderThread( LPVOID lpParam )
{
	HANDLE hProcess;
	DWORD dwProcessID;
	DWORDLONG dwModule;
	

	for(;;)
	{
		if((dwProcessID = FindProcess("UE4-Win64-Shipping.exe")))
		{
			if(!bInjected)
			{
				if((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID)))
				{
					if((dwModule = FindModule(hProcess,"ue4-unrealtournament-win64-shipping.dll")))
					{
						//beacon radar
						//ue4-unrealtournament-win64-shipping.dll + 0x2D1643    6 nops // AUTHUD::DrawActorOverlays
						//memhax((void *)(hModule + 0x2D2243), nopstr, 6);//team check?
						UT4.AddPatch(hProcess,(void *)(dwModule + 0x2D2243),"\x90\x90\x90\x90\x90\x90",6);
						UT4.TogglePatches(1);

						niData.hIcon = (HICON)LoadIcon(GetModuleHandle(0),(char*)IDI_ICON2);
						Shell_NotifyIcon(NIM_MODIFY, &niData);
						bInjected = true;
					}
					//CloseHandle( hProcess );
				}
			}
		}
		else if(bInjected)
		{
			niData.hIcon = (HICON)LoadIcon(GetModuleHandle(0),(char*)IDI_ICON1);
			Shell_NotifyIcon(NIM_MODIFY, &niData);
			bInjected = false;
		}

		Sleep(1000);
	}

	return 0;
}

void ShowContextMenu(HWND hWnd)
{
	HMENU hMenu = CreatePopupMenu();

	if(hMenu)
	{
		POINT pt;

		GetCursorPos(&pt);

		
		//InsertMenu(hMenu, 1, MF_BYPOSITION, WM_ABOUT, "About");
		//InsertMenu(hMenu, 2, MF_SEPARATOR, 0, 0);
		InsertMenu(hMenu, 3, MF_BYPOSITION, WM_EXIT, "Exit");
	


		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, 0 );

		DestroyMenu(hMenu);
	}
}


bool __stdcall DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
		case WM_TRAYMSG:
		{
			switch(lParam)
			{
				case WM_RBUTTONDOWN:
				case WM_CONTEXTMENU:
				{
					ShowContextMenu(hWnd);
					
				}					
			}
			break;
		}
		case WM_COMMAND:
		{
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			switch (wmId)
			{
				case WM_EXIT:
					niData.uFlags = 0;
					Shell_NotifyIcon(NIM_DELETE,&niData);
					DestroyWindow(hWnd);
					break;
				case WM_ABOUT:
					MessageBoxA(0,"Who:\tkeytonic\nWhat:\tpiranha\nWhere:\tyour mom's house\nWhen:\t" __DATE__,"About.",0);
					break;
			}
			return 1;
		}
		case WM_INITDIALOG:
		{
			return true;
		}
		case WM_CLOSE:
		{
			niData.uFlags = 0;
			Shell_NotifyIcon(NIM_DELETE,&niData);
			DestroyWindow(hWnd);
			break;
		}
		case WM_DESTROY:
		{
			niData.uFlags = 0;
			Shell_NotifyIcon(NIM_DELETE,&niData);
			PostQuitMessage(0);
			break;
		}
		default:
		{                /* for messages that we don't deal with */
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return 0;
}


bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd = (HWND)CreateDialog(hInstance,(LPSTR)IDD_MAIN,0,(DLGPROC)DlgProc);

	if (!hWnd) return false;

	ZeroMemory(&niData,sizeof(NOTIFYICONDATA));
	
	niData.hWnd = hWnd;
	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uID = 1;
	niData.uCallbackMessage = WM_TRAYMSG;
	niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	niData.hIcon = (HICON)LoadImage(hInstance,(LPSTR)IDI_ICON1,IMAGE_ICON,16,16,LR_DEFAULTCOLOR);

	strcpy(niData.szTip, "piranha");

	Shell_NotifyIcon(NIM_ADD, &niData);

	if(niData.hIcon && DestroyIcon(niData.hIcon))
	{
		niData.hIcon = 0;
	}

	CreateThread(0,0,LoaderThread,0,0,0);

	return true;
}

int __stdcall WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nCmdShow)
{
	CheckInstance();

	MSG msg;

	if (!InitInstance(hInstance, nCmdShow)) return false;

	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

