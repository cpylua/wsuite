#if !defined(UNICODE)
	#define UNICODE
#endif

#if !defined(_UNICODE)
	#define _UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

/*
 * Return TRUE if the process with processID has the name pszName
 *
 */
static BOOL IsProcessFound( DWORD processID, PTSTR pszName )
{
    TCHAR szProcessName[MAX_PATH] = {0};

    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
             &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName, 
                               sizeof(szProcessName)/sizeof(TCHAR) );
        }
    }
    CloseHandle( hProcess );
    
    return _tcsicmp(pszName, szProcessName) == 0;
}

/*
 * Find the process whose name is pszName,
 * and return the pid.
 * If there are two or more processes, return the first one
 *
 */
static DWORD GetPidByName(PTSTR pszName)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return -1;

    cProcesses = cbNeeded / sizeof(DWORD);
    for ( i = 0; i < cProcesses; i++ )
        if( aProcesses[i] != 0 && IsProcessFound(aProcesses[i], pszName) )
            return aProcesses[i];
    
    return -1;
}

/*
 * Kill the process with specified pid
 *
 */
static BOOL KillProcessByPid(DWORD pid)
{
	HANDLE hProcess = NULL;
	BOOL status = FALSE;
	
	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (hProcess != NULL) {
		/* Exit code must set to 1, otherwise explorer will restart */
		status = TerminateProcess(hProcess, 1);
		WaitForSingleObject(hProcess, INFINITE);
		CloseHandle(hProcess);
	}
	
	return status;
}


/*
 * Run pszName and wait until it exits
 *
 */
static BOOL StartAndWaitForProcess(PTSTR pszName)
{
	BOOL status = FALSE;
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	
	si.cb = sizeof(si);
	
	status = CreateProcess(pszName,
				NULL,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pi);

	if (status) {
		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
	}
	
	return status;
}

/*
 * Run pszName using pszCurrentDirectory as current directory
 *
 */
static BOOL StartProcess(PTSTR pszName, PTSTR pszCurrentDirectory)
{
	BOOL status = FALSE;
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(si);
	
	status = CreateProcess(NULL,
				pszName,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				pszCurrentDirectory,
				&si,
				&pi);
	
	if (status) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	
	return status;
}

/*
 * Check Windows version
 * The problem happens only in Windows 7
 *
 */
static BOOL IsWin7OrLater() 
{
   OSVERSIONINFOEX osvi;
   DWORDLONG dwlConditionMask = 0;
   int op = VER_GREATER_EQUAL;

   /* Initialize the OSVERSIONINFOEX structure */
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   osvi.dwMajorVersion = 6;
   osvi.dwMinorVersion = 1;
   osvi.wServicePackMajor = 0;
   osvi.wServicePackMinor = 0;

   /* Initialize the condition mask */
   VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, op );
   VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, op );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, op );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR, op );

   /* Perform the test */
   return VerifyVersionInfo(
      &osvi, 
      VER_MAJORVERSION | VER_MINORVERSION | 
      VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
      dwlConditionMask);
}

/*
 * Apply Starcraft fix in Windows 7
 *
 */
static BOOL DoScFix()
{
	DWORD pid = -1;
	BOOL status = TRUE;
	TCHAR szPath[MAX_PATH] = {0};
	TCHAR szCmdLine[MAX_PATH] = {0};
	
	if (IsWin7OrLater()) {
		while (TRUE) {
			pid = GetPidByName(TEXT("explorer.exe"));
			if (pid == -1) break;
			status = KillProcessByPid(pid);
		}
		
		if (status) {
			status = StartAndWaitForProcess(TEXT("StarCraft.exe"));
			
			GetWindowsDirectory(szPath, MAX_PATH);
			_tcscpy_s(szCmdLine, MAX_PATH, szPath);
			_tcscat_s(szCmdLine, MAX_PATH, TEXT("\\explorer.exe"));
			status = StartProcess(szCmdLine, szPath);
		}
	}
	else {
		status = StartProcess(TEXT("StarCraft.exe"), NULL);
	}
	
	return status;
}


int WINAPI _tWinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		PTSTR pCmdLine,
		int nCmdShow)
{
	BOOL status;
	
	status = DoScFix();
	return (int)status;
}
