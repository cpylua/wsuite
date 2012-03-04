#include "HijackReturnCode.h"

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

/*=========================================================================

*	Function			CreateHijackEntry
*	Purpose				Create an ihj entry in the system registry
*	Return				If the function succeeds, the return value is TRUE
						If the funciton fails, the return value is FALSE

*	Requirement			Need administrator privilege

*/
//=========================================================================
BOOL CreateHijackEntry(IN LPCTSTR lpHijackName)
{
	BOOL bRet = 0;
	TCHAR szHijackKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
	TCHAR szHijackExec[MAX_PATH] = {0};
	size_t dwSize = 0;
	HKEY hSubKey = NULL;

	if( !GetWindowsDirectory(szHijackExec,MAX_PATH) )
		return FALSE;
	StringCchCat(szHijackExec,MAX_PATH,TEXT("\\blank_ihj.exe"));

	StringCchLength(lpHijackName,MAX_VALUE_NAME,&dwSize);
	dwSize += _countof(szHijackKey);
	LPTSTR szSubKey = (LPTSTR)LocalAlloc(LPTR,dwSize * sizeof(TCHAR));
	StringCbCopy(szSubKey,LocalSize((HLOCAL)szSubKey),szHijackKey);
	StringCbCat(szSubKey,LocalSize((HLOCAL)szSubKey),lpHijackName);

	if( RegCreateKeyEx(HKEY_LOCAL_MACHINE,szSubKey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&hSubKey,NULL) == ERROR_SUCCESS)
	{
		if( RegSetValueEx(hSubKey,TEXT("Debugger"),0,REG_SZ,(BYTE*)szHijackExec,sizeof(szHijackExec) )== ERROR_SUCCESS)
			bRet = TRUE;
		
		RegCloseKey(hSubKey);
	}

	LocalFree((HLOCAL)szSubKey);
	return bRet;
}

/*=========================================================================

*	Function			DeleteHijackEntry
*	Purpose				Delete an ihj entry in the system registry
*	Return				If the function succeeds, the return value is TRUE
						If the funciton fails, the return value is FALSE

*	Requirement			Need administrator privilege

*/
//=========================================================================
BOOL DeleteHijackEntry(IN LPCTSTR lpHijackName)
{
	BOOL bRet = FALSE;
	TCHAR szHijackKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
	size_t dwSize = 0;

	StringCchLength(lpHijackName,MAX_VALUE_NAME,&dwSize);
	dwSize += _countof(szHijackKey);
	LPTSTR szSubKey = (LPTSTR)LocalAlloc(LPTR,dwSize * sizeof(TCHAR));
	StringCbCopy(szSubKey,LocalSize((HLOCAL)szSubKey),szHijackKey);
	StringCbCat(szSubKey,LocalSize((HLOCAL)szSubKey),lpHijackName);

	if( RegDeleteKey(HKEY_LOCAL_MACHINE,szSubKey) == ERROR_SUCCESS )
		bRet =TRUE;

	LocalFree((HLOCAL)szSubKey);

	return bRet;
}

void FormatError(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (_tcslen((LPCTSTR)lpMsgBuf)+_tcslen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Worker"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

BOOL ExtractRes(IN LPCTSTR lpResType,IN LPCTSTR lpResName, PCTSTR pszTargeTPath)
{
	HRSRC hRes = FindResource(NULL,lpResName,lpResType);
	if(!hRes)
	{
		FormatError(TEXT("FindResource"));
		return FALSE;
	}

	HGLOBAL hResGlobal = LoadResource(NULL,hRes);
	if(!hResGlobal)
	{
		FormatError(TEXT("LoadResource"));
		return FALSE;
	}

	DWORD dwResSize = SizeofResource(NULL,hRes);
	LPBYTE pResPtr = (LPBYTE)LockResource(hResGlobal);
	if(!pResPtr)
	{
		FormatError(TEXT("LockResource"));
		return FALSE;
	}
	
	HANDLE hFile = CreateFile(pszTargeTPath,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		FormatError(TEXT("CreateFile"));
		return FALSE;
	}

	//write to the temp file
	DWORD dwBytesWritten = 0;
	WriteFile(hFile,pResPtr,dwResSize,&dwBytesWritten,NULL);

	UnlockResource(hResGlobal);
	CloseHandle(hFile);
	
	return TRUE;
}

BOOL ExtractBlank(IN LPCTSTR lpResType,IN LPCTSTR lpResName)
{
	TCHAR szFilePath[MAX_PATH];

	DWORD uRetVal = GetWindowsDirectory(szFilePath,MAX_PATH); 
	if(!uRetVal)
	{
		FormatError(TEXT("GetWindowsDirectory"));
		return FALSE;
	}
	StringCchCat(szFilePath,MAX_PATH,TEXT("\\blank_ihj.exe"));

	return ExtractRes(lpResType,lpResName,szFilePath);

}

BOOL DelBlank()
{
	TCHAR szFilePath[MAX_PATH];

	DWORD uRetVal = GetWindowsDirectory(szFilePath,MAX_PATH); 
	if(!uRetVal)
	{
		FormatError(TEXT("GetWindowsDirectory"));
		return FALSE;
	}
	StringCchCat(szFilePath,MAX_PATH,TEXT("\\blank_ihj.exe"));

	BOOL bOk = DeleteFile(szFilePath);
	if( !bOk )
		FormatError(TEXT("DeleteFile"));

	return bOk;
}

int WINAPI _tWinMain(HINSTANCE hInstance,
					 HINSTANCE,
					 LPTSTR lpCmdLine,
					 int nCmdShow)
{
	if( _tcsncmp(lpCmdLine,TEXT("???"),3) == 0 )
	{
		DWORD dwOption = _tcstoul(lpCmdLine + 3,NULL,0);
		if( dwOption == HIJACK_ADD_BLANK )
		{
			if( ExtractBlank(TEXT("BINARY"),MAKEINTRESOURCE(IDR_BLANK)) )
				return HijackCreateOk;
			else
				return HijackFailed;
		}
		if( dwOption == HIJACK_DEL_BLANK )
		{
			if( DelBlank() )
				return HijackDeleteOk;
			else
				return HijackFailed;
		}
	}
	else
	{
		HANDLE hFile = CreateFile(lpCmdLine,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		if( hFile == INVALID_HANDLE_VALUE )
		{
			FormatError(TEXT("CreateFile, ReadOnly"));
			return HijackFailed;
		}

		// Read the header
		HijackInfo hi;
		DWORD dwBytesReturned;
		ReadFile(hFile,&hi,sizeof(hi),&dwBytesReturned,NULL);
		
		// Get the names
		DWORD dwFileSize = GetFileSize(hFile,NULL);
		DWORD cbNames = dwFileSize - sizeof(hi);
		PTSTR pszNameStrings = (PTSTR)LocalAlloc(LPTR,cbNames);
		ReadFile(hFile,pszNameStrings,cbNames,&dwBytesReturned,NULL);
		CloseHandle(hFile);

		PTSTR pszCurrent = pszNameStrings;
		DWORD dwDeleted = 0;
		DWORD dwAdded = 0;
		for(int i = 0; i != hi.cEntry; ++i)
		{
			switch(hi.dwOption)
			{
			case HIJACK_CREATE:
				if( !CreateHijackEntry(pszCurrent) )
					return HijackFailed;
				dwAdded++;
				break;

			case HIJACK_DELETE:
				if( !DeleteHijackEntry(pszCurrent) )
					return HijackFailed;
				dwDeleted++;
				break;
			}

			// Next entry please
			pszCurrent = _tcschr(pszCurrent,TEXT('\0'));
			pszCurrent++;
		}
		
		LocalFree(pszNameStrings);

		if( hi.dwOption == HIJACK_CREATE )
			return dwAdded + HijackCreateOk;
		else
			return dwDeleted + HijackDeleteOk;
	}

	return 0;
}
	





