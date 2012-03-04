#include <windows.h>
#include <tchar.h>

const TCHAR g_lpSubKey[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\");
const TCHAR g_lpValue[] = TEXT("PendingFileRenameOperations");
#define CHARS_TO_SKIP	4

//--------------------
// Print the usage
//--------------------
static VOID __stdcall ShowUsage(LPCTSTR lpFileName)
{
	DWORD dwStrLen;

	dwStrLen = _tcslen(lpFileName);
	while( dwStrLen && (lpFileName[dwStrLen--] != L'\\') );
	if( dwStrLen )
		dwStrLen += 2;
	_tprintf(TEXT("Usage:  %s [source] [dest]\n\tSpecifying an empty destination(\"\") deletes the source at boot time.\n\n"),
			&lpFileName[dwStrLen]);
	_tprintf(TEXT("\t%s /l\n\tList the registered operations.\n\n"),&lpFileName[dwStrLen]);
	_tprintf(TEXT("\t%s /d[a | #]\n\tDelete all the registered entries or the entry specified by #.\n")
			 TEXT("\tTo determine #, use the /l switch.\n"),&lpFileName[dwStrLen]);
}

//------------------------------------
// Format an error code and print it
//------------------------------------
static VOID __stdcall PrintErrorByCode(DWORD dwErrorCode)
{
	LPVOID lpMsgBuf;

	FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwErrorCode,
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
				(LPTSTR) &lpMsgBuf,
				0, NULL );

    // Display the error message

	_tprintf(TEXT("Failed with error code: 0x%08X\n--> %s\n"),dwErrorCode,(LPTSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

//--------------------------------------------------------------------
// Retrieve the registry value of "PendingFileRenameOperations"
// Remember to call LocalFree when you no longer need the lpValueBuf
//--------------------------------------------------------------------
static BOOL __stdcall RetrievePendMove(OUT LPVOID *lpValueBuf,OUT PDWORD pdwValueSize)
{
	LONG lRet;

	// Get the needed size
	lRet = RegGetValue(HKEY_LOCAL_MACHINE,
					   g_lpSubKey,
					   g_lpValue,
					   RRF_RT_REG_MULTI_SZ,
					   NULL,
					   NULL,
					   pdwValueSize);
	if( ERROR_SUCCESS != lRet )
	{
		if( lRet == ERROR_FILE_NOT_FOUND )
			_tprintf(TEXT("No pending file rename operations registered.\n"));
		else
			PrintErrorByCode(lRet);
		return FALSE;
	}
	
	// Allocate buffer
	*lpValueBuf = (LPVOID)LocalAlloc(LPTR,*pdwValueSize);
	if( !*lpValueBuf )
	{
		PrintErrorByCode(GetLastError());
		return FALSE;
	}

	// Get the value
	lRet = RegGetValue(HKEY_LOCAL_MACHINE,
					   g_lpSubKey,
					   g_lpValue,
					   RRF_RT_REG_MULTI_SZ,
					   NULL,
					   *lpValueBuf,
					   pdwValueSize);

	if( ERROR_SUCCESS != lRet )
	{
		PrintErrorByCode(lRet);
		return FALSE;
	}

	return TRUE;
}

//-----------------------------
// Get the last update time
//-----------------------------
BOOL __stdcall GetLastUpdateTime()
{
	HKEY hPendMoveKey;
	FILETIME ftLastUpdate;
	FILETIME ftLocalLastUpdate;
	SYSTEMTIME stLastUpdate;
	LPTSTR lpDateStr;
	LPTSTR lpTimeStr;
	LONG lRet;

	lRet = RegOpenKey(HKEY_LOCAL_MACHINE,g_lpSubKey,&hPendMoveKey);
	if( lRet != ERROR_SUCCESS )
		return FALSE;

	lRet = RegQueryInfoKey(hPendMoveKey,
						   NULL,
						   NULL,
						   NULL,
						   NULL,
						   NULL,
						   NULL,
						   NULL,
						   NULL,
						   NULL,
						   NULL,
						   &ftLastUpdate);

	if( lRet != ERROR_SUCCESS )
		return FALSE;

	RegCloseKey(hPendMoveKey);

	if( !FileTimeToLocalFileTime(&ftLastUpdate,&ftLocalLastUpdate) )
		return FALSE;

	if( !FileTimeToSystemTime(&ftLocalLastUpdate,&stLastUpdate) )
		return FALSE;
	
	lRet = GetDateFormat(LOCALE_SYSTEM_DEFAULT,
						 DATE_SHORTDATE,
						 &stLastUpdate,
						 NULL,
						 NULL,
						 0);
	lpDateStr = (LPTSTR)LocalAlloc(LPTR,lRet * sizeof(TCHAR));
	if( !lpDateStr )
		return FALSE;

	lRet = GetDateFormat(LOCALE_SYSTEM_DEFAULT,
						 DATE_SHORTDATE,
						 &stLastUpdate,
						 NULL,
						 lpDateStr,
						 lRet);
	if( !lRet )
		return FALSE;

	lRet = GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
						 TIME_FORCE24HOURFORMAT,
						 &stLastUpdate,
						 NULL,
						 NULL,
						 0);
	lpTimeStr = (LPTSTR)LocalAlloc(LPTR,lRet * sizeof(TCHAR));
	if( !lpTimeStr )
		return FALSE;

	lRet = GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
						 TIME_FORCE24HOURFORMAT,
						 &stLastUpdate,
						 NULL,
						 lpTimeStr,
						 lRet);

	if( !lRet )
		return FALSE;

	_tprintf(TEXT("Time of last update to pending moves key: %s %s\n"),lpDateStr,lpTimeStr);

	LocalFree((HLOCAL)lpDateStr);
	LocalFree((HLOCAL)lpTimeStr);

	return TRUE;
}

//--------------------------------------------------------------
// Parse the "PendingFileRenameOperations" value and print it
//--------------------------------------------------------------
static BOOL __stdcall ParsePendMove()
{
	DWORD dwValueSize;
	LPVOID lpValueBuf;
	LPTSTR lpValueStr;
	DWORD dwSizeTotal;
	DWORD dwCount;

	if( !RetrievePendMove(&lpValueBuf,&dwValueSize) )
		return FALSE;

	// Print the strings
	lpValueStr = (LPTSTR)lpValueBuf;
	if( !*lpValueStr )
	{
		_tprintf(TEXT("No pending file rename operations registered.\n"));
		return TRUE;
	}
	
	dwSizeTotal = 0;
	dwCount = 1;
	while( dwSizeTotal < dwValueSize - 2 * sizeof(TCHAR) )
	{
		if( !_tcsncmp(lpValueStr,TEXT("\\??\\"),CHARS_TO_SKIP) )
			_tprintf(TEXT("#%d\nSource: %s\n"),dwCount++,lpValueStr + CHARS_TO_SKIP);
		else
			_tprintf(TEXT("#%d\nSource: %s\n"),dwCount++,lpValueStr);
		dwSizeTotal += (_tcslen(lpValueStr) + 1) * sizeof(TCHAR);

		// Skip source string
		while( *lpValueStr++ );

		if( *lpValueStr )
		{
			if( !_tcsncmp(lpValueStr,TEXT("\\??\\"),CHARS_TO_SKIP) )
				_tprintf(TEXT("Target: %s\n\n"),lpValueStr + CHARS_TO_SKIP);
			else
				_tprintf(TEXT("Target: %s\n"),dwCount++,lpValueStr);
		}
		else
			_tprintf(TEXT("Target: NULL. --> DELETE source at boot time.\n\n"));
		dwSizeTotal += (_tcslen(lpValueStr) + 1) * sizeof(TCHAR);

		// Skip source string
		while( *lpValueStr++ );
	}
	
	LocalFree((HLOCAL)lpValueBuf);

	if( !GetLastUpdateTime() )
		_tprintf(TEXT("Failed to retrieve the last update time of pending moves key.\n"));

	return TRUE;
}

//-------------------------------------------------------
// Delete all the registered entries or a specified one
//-------------------------------------------------------
static BOOL __stdcall DelPendMove(DWORD dwIndexToDel)
{
	LPVOID lpValueBuf;
	LPTSTR lpValueStr;
	LPVOID lpValueToSet;
	DWORD dwValueSize;
	DWORD dwTotalSize;
	DWORD dwValidSize;
	DWORD dwCurIndex;
	DWORD dwCurStrLen;
	LONG lRet;
	
	// Delete all the registered operations
	if( dwIndexToDel == 0 )
	{
		lRet = RegDeleteKeyValue(HKEY_LOCAL_MACHINE,g_lpSubKey,g_lpValue);
		if( lRet != ERROR_SUCCESS )
		{
			PrintErrorByCode(lRet);
			return FALSE;
		}

		return TRUE;
	}

	if( !RetrievePendMove(&lpValueBuf,&dwValueSize) )
		return FALSE;

	lpValueStr = (LPTSTR)lpValueBuf;
	lpValueToSet = (LPVOID)LocalAlloc(LPTR,dwValueSize);
	if( !lpValueToSet )
	{
		PrintErrorByCode(GetLastError());
		return FALSE;
	}

	dwTotalSize = 0;
	dwCurIndex = 0;
	dwValidSize = 0;

	while( dwTotalSize < dwValueSize - 2 * sizeof(TCHAR) )
	{
		dwCurStrLen = (_tcslen(lpValueStr) + 1) * sizeof(TCHAR);
		
		//
		// Each entry has two strings
		//
		if( dwCurIndex / 2 != dwIndexToDel - 1 )
		{
			memcpy_s(lpValueToSet,
					 dwValueSize - dwValidSize,
					 (LPVOID)lpValueStr,
					 dwCurStrLen);
			dwValidSize += dwCurStrLen;
		    lpValueToSet = (PBYTE)lpValueToSet + dwCurStrLen;
		}
		
		// Skip
		lpValueStr = (LPTSTR)((PBYTE)lpValueStr + dwCurStrLen);

		dwTotalSize += dwCurStrLen;
		dwCurIndex++;
	}
	
	//
	// Multi_Sz ends with two NULLs
	//
	((LPTSTR)lpValueToSet)[dwValidSize] = L'\0';

	// There're no more operations, delete the registry value
	if( dwValidSize == 0 )
	{
		lRet = RegDeleteKeyValue(HKEY_LOCAL_MACHINE,g_lpSubKey,g_lpValue);
		if( lRet != ERROR_SUCCESS )
		{
			PrintErrorByCode(lRet);
			return FALSE;
		}

		return TRUE;
	}

	// Save the remaining registered operations
	lRet = RegSetKeyValue(HKEY_LOCAL_MACHINE,
						  g_lpSubKey,
						  g_lpValue,
						  REG_MULTI_SZ,
						  (LPVOID)((PBYTE)lpValueToSet - dwValidSize),
						  dwValidSize + sizeof(TCHAR));
	if( lRet != ERROR_SUCCESS )
	{
		PrintErrorByCode(lRet);
		return FALSE;
	}

	LocalFree((HLOCAL)lpValueBuf);
	LocalFree((HLOCAL)lpValueToSet);

	return TRUE;
}

//----------------
// Entry
//----------------
int _tmain(int argc,TCHAR** argv)
{
	BOOL bRet;
	DWORD dwIndexToDel;
	TCHAR *pEndChar = NULL;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WORD wOldColorAttrib;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&csbi);
	wOldColorAttrib = csbi.wAttributes;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);

	_tprintf(TEXT("\nPendMove v1.1\nCopyright (C) 2008 Whislter Lee\n\n"));

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),wOldColorAttrib);

	if( argc == 2 && 
		( !_tcsicmp(argv[1],TEXT("/l")) || !_tcsicmp(argv[1],TEXT("-l")) ) )
	{
		if( !ParsePendMove() )
			return -1;

		return 0;
	}

	if( argc == 2 && 
		(argv[1][0] == L'/' || argv[1][0] == L'-')&& 
		(argv[1][1] == L'd' || argv[1][1] == L'D') )
	{
		// Delete all
		if( argv[1][3] == L'a' || argv[1][3] == L'A' )
		{
			if( !DelPendMove(0) )
				return -1;
			
			_tprintf(TEXT("All registered entries successfully deleted.\n"));
			return 0;
		}

		dwIndexToDel = _tcstoul(argv[1] + 2,&pEndChar,10);
		if( !DelPendMove(dwIndexToDel) )
			return -1;
		
		_tprintf(TEXT("Entry successfully deleted.\n"));
		return 0;
	}
		

	if( argc != 3 )
	{
		ShowUsage(argv[0]);
		return -1;
	}

	bRet = MoveFileEx(argv[1],
					  *(argv[2]) ? argv[2] : NULL,
					  MOVEFILE_DELAY_UNTIL_REBOOT);

	if( !bRet )
	{
		PrintErrorByCode(GetLastError());
		return -2;
	}

	_tprintf(TEXT("File move successfully scheduled.\n"));
	return 0;
}