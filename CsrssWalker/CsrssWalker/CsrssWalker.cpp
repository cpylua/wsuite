// CsrssWalker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define ALLOC_SIZE	256

BOOL GetCsrssPIDs(PDWORD pCsrssPIDs, DWORD cb, PDWORD pBytesReturned);
BOOL EnableDebugPrivilege();
ULONG GetCsrssRootProcessPointerOffset();
ULONG GetModuleHandleByName(ULONG pid, PTSTR szModuleName);
VOID ShowProcessByWalkCsrssProcessLink(ULONG CsrssPid,PVOID pvRootPointer);
VOID ShowProcessNameByPID(DWORD PID);

int _tmain(int argc, _TCHAR* argv[])
{
	DWORD dwCsrssPIDs[4];
	DWORD dwReturned;

	EnableDebugPrivilege();

	GetCsrssPIDs(dwCsrssPIDs, sizeof(dwCsrssPIDs), &dwReturned);
	//_tprintf(TEXT("dwReturned = %d\n"), dwReturned);
	ULONG ulOffset = GetCsrssRootProcessPointerOffset();
	//_tprintf(TEXT("ulOffser = 0x%X\n"), ulOffset);

	_tprintf(TEXT("\n%-26s%8s\n"), TEXT("Image Name"), TEXT("PID"));
	_tprintf(TEXT("========================= ========\n"));
	for(int i = 0; i != dwReturned / sizeof(DWORD); i++)
	{
		ULONG ulRootPointer = GetModuleHandleByName(dwCsrssPIDs[i], TEXT("csrsrv.dll")) + ulOffset;
		ShowProcessByWalkCsrssProcessLink(dwCsrssPIDs[i], (PVOID)ulRootPointer);
	}

	return 0;
}

BOOL EnableDebugPrivilege() 
{ 
	TOKEN_PRIVILEGES tkp; 
	HANDLE hToken; 

	if ( !OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken) )
		return FALSE; 

	LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1; 
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	return TRUE; 
}

BOOL GetCsrssPIDs(PDWORD pCsrssPIDs, DWORD cb, PDWORD pBytesReturned)
{
	DWORD dwSize, dwReturnedBytes;
	PVOID pvPIDs;
	DWORD dwProcNum, i;
	HANDLE hProcess = NULL;
	PDWORD pdwPIDs = NULL;
	TCHAR szBaseName[] = TEXT("csrss.exe");
	PTSTR pImageFileName = NULL;
	DWORD dwCsrssPIDs = 0;
	int iCmpResult;
	BOOL bFlag;
	
	// Get all PIDs
	dwSize = ALLOC_SIZE;
	do 
	{
		pvPIDs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
		EnumProcesses((PDWORD)pvPIDs, dwSize, &dwReturnedBytes);
		if ( bFlag = (dwSize == dwReturnedBytes) )
		{
			HeapFree(GetProcessHeap(), 0, pvPIDs);
			dwSize *= 2;
		}
	} while ( bFlag );
	
	// Loop through PIDs,
	// fill in pCsrssPIDs with processes whose image name is csrss.exe
	dwProcNum = dwReturnedBytes / sizeof(DWORD);
	pdwPIDs = (PDWORD)pvPIDs;
	for(i = 0; i != dwProcNum; i++)
	{
		if(pdwPIDs[i] == 0)
			continue;

		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pdwPIDs[i]);
		if(hProcess)
		{
			dwSize = ALLOC_SIZE;
			do
			{
				pImageFileName = (PTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
				dwReturnedBytes = GetProcessImageFileName(hProcess, pImageFileName, dwSize);
				if( bFlag = (dwReturnedBytes == dwSize) )
				{
					HeapFree(GetProcessHeap(), 0, pImageFileName);
					dwSize *= 2;
				}
			} while( bFlag );

			iCmpResult = _tcsicmp(szBaseName, _tcsrchr(pImageFileName, TEXT('\\')) + 1);
			HeapFree(GetProcessHeap(), 0, pImageFileName);
			if( !iCmpResult )
			{				
				if (dwCsrssPIDs * sizeof(DWORD) <= cb)
					pCsrssPIDs[dwCsrssPIDs++] = pdwPIDs[i];
				else break;
			}		
		}
		CloseHandle(hProcess);
	}
	HeapFree(GetProcessHeap(), 0, pdwPIDs);

	if(pBytesReturned)
		*pBytesReturned = dwCsrssPIDs * sizeof(DWORD);
	
	return TRUE;
}

ULONG GetCsrssRootProcessPointerOffset()
{
	TCHAR szDllName[MAX_PATH];
	PBYTE pfnCsrLockProcessByClientId;
	HMODULE hCsrsrv = NULL;
	ULONG Offset = 0;;

	GetSystemDirectory(szDllName, MAX_PATH);
	StringCchCat(szDllName, MAX_PATH, TEXT("\\csrsrv.dll"));

	hCsrsrv = LoadLibraryEx(szDllName, NULL, DONT_RESOLVE_DLL_REFERENCES);
	if( hCsrsrv )
	{
		pfnCsrLockProcessByClientId = (PBYTE)GetProcAddress(hCsrsrv, "CsrLockProcessByClientId");

		/*
		75AA52ED    8B35 1C89AA75          mov esi,dword ptr ds:[75AA891C]
		75AA52F3    83C6 08                add esi,8
		*/
		for (int i = 0; i<0x50; i++)
		{
			PBYTE p=(PBYTE)pfnCsrLockProcessByClientId + i;
			if ( *p==0x8B && *(p+6)==0x83 && *(p+8)==0x08 )
			{
				Offset = *(ULONG*)(p+2);
				Offset = Offset-(ULONG)hCsrsrv;
				break;
			}
		}

		FreeLibrary(hCsrsrv);
	}

	return Offset;
}

ULONG GetModuleHandleByName(ULONG pid, PTSTR szModuleName)
{
	ULONG ulBase = 0; 

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

	if(hProcess)
	{
		DWORD dwSize = ALLOC_SIZE;
		HMODULE *pMods = NULL;
		DWORD dwNeeded;
		BOOL bFlag;

		do 
		{
			pMods = (HMODULE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
			EnumProcessModules(hProcess, pMods, dwSize, &dwNeeded);
			if( bFlag = (dwNeeded > dwSize) )
			{
				HeapFree(GetProcessHeap(), 0, pMods);
				dwSize *= 2;
			}
		} while ( bFlag );

		DWORD dwMods = dwNeeded / sizeof(HMODULE);
		TCHAR szBaseName[16];
		for(int i = 0; i != dwMods; i++)
		{
			GetModuleBaseName(hProcess, pMods[i], szBaseName, _countof(szBaseName));
			if( !_tcsicmp(szBaseName, szModuleName) )
			{
				ulBase = (ULONG)pMods[i];
				break;
			}				
		}

		HeapFree(GetProcessHeap(), 0, pMods);
		CloseHandle(hProcess);
	}

	return ulBase;
}

VOID ShowProcessByWalkCsrssProcessLink(ULONG CsrssPid, PVOID pvRootPointer )
{
	HANDLE hProcess;
	PCSR_PROCESS pCsrssRootProcess = NULL;
	PCSR_PROCESS pProcessInfo;
	PLIST_ENTRY pProcessList,pNext;
	ULONG ReadSize;

	// Open csrss process
	hProcess=OpenProcess(PROCESS_VM_READ, FALSE, CsrssPid);
	if (!hProcess)
	{
		_tprintf(TEXT("Open Csrss.exe Failed.\n"));
		return;
	}

	// Read the pointer value
	ReadProcessMemory(hProcess, pvRootPointer, &pCsrssRootProcess, sizeof(PCSR_PROCESS), &ReadSize);
	//_tprintf(TEXT("ProcessRoot =0x%08X\n"), pCsrssRootProcess);

	// Read process info
	pProcessInfo=(PCSR_PROCESS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSR_PROCESS));
	ReadProcessMemory(hProcess, pCsrssRootProcess, pProcessInfo, sizeof(CSR_PROCESS), &ReadSize);

	// Traverse CsrssProcessLink
	pNext = pProcessInfo->ListLink.Flink;
	pProcessList = (PLIST_ENTRY)( (PBYTE)pCsrssRootProcess + sizeof(CLIENT_ID) );

	while( pNext != pProcessList )
	{
		ReadProcessMemory(hProcess, (PBYTE)pNext - sizeof(CLIENT_ID), pProcessInfo, sizeof(CSR_PROCESS), &ReadSize);
		ShowProcessNameByPID((DWORD)pProcessInfo->ClientId.UniqueProcess);
		
		pNext = pProcessInfo->ListLink.Flink;
	}

	if (pProcessInfo)
		HeapFree(GetProcessHeap(), 0, pProcessInfo);

	return;
}

VOID ShowProcessNameByPID(DWORD PID)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
	if( hProcess )
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if( EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded) )
		{
			PTSTR pProcessName;
			DWORD dwSize = ALLOC_SIZE, dwCopied;
			BOOL bFlag;

			do 
			{
				pProcessName = (PTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
				dwCopied = GetModuleBaseName(hProcess, hMod, pProcessName, dwSize);
				if( bFlag = (dwCopied == dwSize) )
				{
					HeapFree(GetProcessHeap(), 0, pProcessName);
					dwSize *= 2;
				}
			} while (bFlag);

			_tprintf(TEXT("%-26s%8d\n"), pProcessName, PID);

			HeapFree(GetProcessHeap(), 0 ,pProcessName);
			CloseHandle(hProcess);			
		}
	}
}
