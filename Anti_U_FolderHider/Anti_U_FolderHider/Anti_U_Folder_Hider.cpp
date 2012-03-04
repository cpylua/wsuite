#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <strsafe.h>
#include <Dbt.h>
#include "resource.h"

enum ScanStatus
{
	STATUS_SCANNING,		//scan in process
	STATUS_IDEL,			//idel status
	STATUS_FINISHED		//a scan has just finished
} g_Status;

#define WM_UPDATESTATUS (WM_USER+100)
/*
	wParam		handle to the static control
	lParam		current status
*/

#define TIMER_UPDATE 20

BOOL IsDotsDirectory(IN LPCTSTR lpFileName)
{
	return (_tcscmp(lpFileName, _T(".")) == 0 || _tcscmp(lpFileName, _T("..")) == 0);
}

BOOL IsFileMalicious(IN LPCTSTR lpFileName)
{
	TCHAR *szMaliciousTypes[] = {TEXT(".com"),TEXT(".pif"),TEXT(".bat"),TEXT(".cmd")};
	BOOL bRet = FALSE;

	size_t dwLen = 0;
	StringCchLength(lpFileName,MAX_PATH,&dwLen);

	int i = dwLen - 1;
	for(/*Empty*/; i >= 0; --i)
	{
		if(lpFileName[i] == '.')
		{
			break;
		}
	}

	if(i >= 0)
	{
		for(int j = 0; j != _countof(szMaliciousTypes);++j)
		{
			if( !_tcsicmp(szMaliciousTypes[j],lpFileName + i) )
			{
				bRet = TRUE;
				break;
			}
		}
	}

	return bRet ;
}

//delete suspicious file
BOOL DelSuspiciousFile(IN LPCTSTR lpFileName,IN const DWORD dwFileAttributes)
{
	BOOL bRet = FALSE;

	bRet = DeleteFile(lpFileName);
	if( !bRet && GetLastError() == ERROR_ACCESS_DENIED )
	{
		//it may be a read only file
		SetFileAttributes(lpFileName,dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
		bRet = DeleteFile(lpFileName);
	}

	return bRet;
}

BOOL ScanUDisk(IN LPCTSTR lpUDisk,IN BOOL bDelSuspicious)
{
	size_t dwLen = 0;
	size_t dwDirNameLen = 0;

	LPTSTR lpSearchDisk = NULL;		//The U Disk to search
	LPTSTR lpDirName = NULL;		//Full file path
	LPTSTR lpFileName = NULL;		//File with the same name as a directory, but has a ".exe" type extension

	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA FindFileData;

	//Build the search string
	StringCchLength(lpUDisk,20/*set this to 4 later*/,&dwLen);
	lpSearchDisk = (LPTSTR)LocalAlloc(LPTR,(dwLen+2) * sizeof(TCHAR));
	StringCchPrintf(lpSearchDisk,dwLen+3,TEXT("%s%s"),lpUDisk,TEXT("*"));

	hFind = FindFirstFile(lpSearchDisk, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ( IsDotsDirectory(FindFileData.cFileName) )
				continue;

			//construct the directory or file full path
			StringCchLength(FindFileData.cFileName,MAX_PATH,&dwDirNameLen);	//get the directory name length
			dwDirNameLen += dwLen + 2;
			lpDirName = (LPTSTR)LocalAlloc(LPTR,dwDirNameLen * sizeof(TCHAR));	
			StringCchPrintf(lpDirName,LocalSize((HLOCAL)lpDirName),TEXT("%s%s"),lpUDisk,FindFileData.cFileName);
			
			//construct the full suspicious file path
			lpFileName = (LPTSTR)LocalAlloc(LPTR,(dwDirNameLen + 4) * sizeof(TCHAR));
			StringCchPrintf(lpFileName,LocalSize((HLOCAL)lpFileName),TEXT("%s.exe"),lpDirName);

			if ( (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY )
			{
				//it's a directory
				SetFileAttributes(lpDirName,FindFileData.dwFileAttributes & ~FILE_ATTRIBUTE_HIDDEN & ~FILE_ATTRIBUTE_SYSTEM);
				
				if( bDelSuspicious )
				{
					//delete suspicious file
					DelSuspiciousFile(lpFileName,FindFileData.dwFileAttributes);
				}
			}
			else		//it's a file
			{	
				if( bDelSuspicious )
				{
					//delete "autorun.inf"
					if( !_tcsicmp(FindFileData.cFileName,TEXT("autorun.inf")) )
						DelSuspiciousFile(lpDirName,FILE_ATTRIBUTE_NORMAL);
						
					//delete other malicious files
					if( IsFileMalicious(FindFileData.cFileName) )
					{						
						DelSuspiciousFile(lpDirName,FindFileData.dwFileAttributes);
					}
				}
			}

		} while (FindNextFile(hFind, &FindFileData));
	}
	FindClose(hFind);
	hFind = NULL;

	LocalFree((HLOCAL)lpDirName);
	LocalFree((HLOCAL)lpSearchDisk);
	lpSearchDisk = NULL;
	lpDirName = NULL;	
	
	return TRUE;
}

void FindDriverName(IN OUT TCHAR &cDriverName,IN DWORD dwMask)
{
    TCHAR i = 0;
    for(;i <= 26; ++i)
    {
	if(dwMask & 0x1)
	    break;
	dwMask = dwMask >> 1;
    }
    cDriverName = TEXT('A') + i;
}



INT_PTR CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	TCHAR szDriver[4];
	TCHAR cDriverName;

	PDEV_BROADCAST_HDR pdbHdr = NULL;
	PDEV_BROADCAST_VOLUME pdbVol = NULL;

	HICON hIcon = NULL;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			CheckDlgButton(hwndDlg,CHK_DELSUSPICIOUS,TRUE);

			hIcon = LoadIcon( (HINSTANCE)GetWindowLong(hwndDlg,GWLP_HINSTANCE),MAKEINTRESOURCE(IDI_SCAN) );
			SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
			SendMessage(hwndDlg,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);

			SendMessage(hwndDlg,WM_UPDATESTATUS,(WPARAM)GetDlgItem(hwndDlg,STATIC_STATUS),(LPARAM)STATUS_IDEL);
		}
		break;

	case WM_CLOSE:
		{
			SendMessage(hwndDlg,WM_COMMAND,BTN_EXIT,TRUE);
		}
		break;

	case WM_COMMAND:
		{
			if( lParam && HIWORD(wParam) == 0)
			{
				switch( LOWORD(wParam) )
				{
				case BTN_SCAN:
					{
						//get removable disks
						StringCchCopy(szDriver,_countof(szDriver),TEXT("c:\\"));
						while( szDriver[0]++ != TEXT('z') )
						{
							if( GetDriveType(szDriver) == DRIVE_REMOVABLE )
							{
								SendMessage(hwndDlg,WM_UPDATESTATUS,(WPARAM)GetDlgItem(hwndDlg,STATIC_STATUS),(LPARAM)STATUS_SCANNING);
								ScanUDisk( szDriver,(BOOL)IsDlgButtonChecked(hwndDlg,CHK_DELSUSPICIOUS) );
								SendMessage(hwndDlg,WM_UPDATESTATUS,(WPARAM)GetDlgItem(hwndDlg,STATIC_STATUS),(LPARAM)STATUS_FINISHED);
							}
						}
					}
					break;

				case BTN_EXIT:
					{
						EndDialog(hwndDlg,0);
					}
					break;
				}
			}
		}
		break;

	case WM_DEVICECHANGE:
	    {
			//handle device change message to detect driver insertion
			if(wParam == DBT_DEVICEARRIVAL)
			{
				pdbHdr = (PDEV_BROADCAST_HDR)lParam;
				if(pdbHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					pdbVol = (PDEV_BROADCAST_VOLUME)pdbHdr;
					FindDriverName(cDriverName, pdbVol->dbcv_unitmask);

					StringCchPrintf(szDriver,_countof(szDriver),TEXT("%c:\\"),cDriverName);
					SendMessage(hwndDlg,WM_UPDATESTATUS,(WPARAM)GetDlgItem(hwndDlg,STATIC_STATUS),(LPARAM)STATUS_SCANNING);
					ScanUDisk( szDriver,IsDlgButtonChecked(hwndDlg,CHK_DELSUSPICIOUS) );
					SendMessage(hwndDlg,WM_UPDATESTATUS,(WPARAM)GetDlgItem(hwndDlg,STATIC_STATUS),(LPARAM)STATUS_FINISHED);
				}
			}
	    }
	    break;

	case WM_UPDATESTATUS:
		{
			g_Status = (ScanStatus)lParam;

			switch(g_Status)
			{
			case STATUS_SCANNING:
				{
					SetDlgItemText(hwndDlg,STATIC_STATUS,TEXT("…®√Ë÷–..."));
					break;
				}

			case STATUS_FINISHED:
				{
					MessageBeep(MB_ICONINFORMATION);
					SetDlgItemText(hwndDlg,STATIC_STATUS,TEXT("…®√ËÕÍ≥…°£"));

					SetTimer(hwndDlg,TIMER_UPDATE,5000,NULL);
					break;
				}
				
			case STATUS_IDEL:
				{
					SetDlgItemText(hwndDlg,STATIC_STATUS,TEXT(""));
					break;
				}
			}
		}
		break;	

	case WM_TIMER:
		{
			if(g_Status == STATUS_FINISHED)
			{
				SetDlgItemText(hwndDlg,STATIC_STATUS,TEXT(""));
				KillTimer(hwndDlg,TIMER_UPDATE);
				g_Status = STATUS_IDEL;
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

int WINAPI _tWinMain(HINSTANCE hInstance,HINSTANCE,LPTSTR /*lpCmdLine*/,int /*nCmdShow*/)
{
	//allow only one instance
	HWND hWnd = FindWindow(TEXT("#32770"),TEXT("Anti U Folder Hider"));
	if( IsWindow(hWnd) )
		SetForegroundWindow(hWnd);
	else
	{
		DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DLG_ANTI),NULL,(DLGPROC)DialogProc,NULL);
	}

	return 0;
}