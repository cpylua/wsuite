/*
// Download this: http://www.hidedragon.com/cn/HideDragon32_full.exe to perform a test.
// Note, you should download the full version, only that can hide processes.
*/

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <psapi.h>			//for EnumProcess(...)
#include <locale.h>			//for _tsetlocale(...)

//#define CHECK_OS_VERSION			//Check this if you want to check the OS version

//Enable SeDebugPrivilege
VOID EnableDebugPrivilege();

//Find the specific PID in a PID list
BOOL SeekPID(IN DWORD *pdwArray,IN DWORD dwArraySize,IN DWORD dwDesiredPID);

#ifdef CHECK_OS_VERSION
//Version check
BOOL IsXpOrLater();
#endif

//Copyright info
VOID Info();

//Convert the device form path to dos form path
BOOL RenamePath(IN LPCTSTR lpDeviceFormPath,OUT LPTSTR lpBuffer);

#pragma comment(lib,"Psapi.lib")

int _tmain(int argc,TCHAR **argv)
{
	//Parameters for QueryFullProcessImageName(...)
	TCHAR lpExeName[MAX_PATH];				//Buffer for QueryFullProcessImageName(...)
	DWORD dwSize = _countof(lpExeName);		//Buffer size

	TCHAR lpBuffer[MAX_PATH];				//Buffer for Dos form path

	//Parameters for EnumProcess(...)
	DWORD dwPIDs[1024];						//PID list,large enough?
	DWORD dwPidRetSize = 0;					//Returned PID list size,in bytes

	//Handles
	HANDLE hProc = NULL;					//Process handle
	HANDLE hToken = NULL;					//Current process' token handle
	
	//Count values
	DWORD dwHidden = 0;						//Track there's how many hidden processes
	DWORD dwSuccess = 0;					//Track how many times OpenProcess(...) successes
	
	//If the path contains Chinese Characters,
	//_tprintf(...) may not be able to deal with those characters with the default locale.
	//This may only happen on Chinese versions of Windows systems
	_tsetlocale(LC_ALL,TEXT("Chinese"));	

	//Show info
	_tprintf(TEXT("%s\tWhilter (c) 2007-2008\n")
			 TEXT("Detect hidden processes with user mode method\n")
			 TEXT("NOTE:\tSee readme.txt\n\n"),argv[0]);

#ifdef CHECK_OS_VERSION
	//Check the OS version
	//This program requires Windows XP or later
	if(!IsXpOrLater())
	{
		_tprintf(TEXT("%s requires Windows Vista or later\n"),argv[0]);
		return -2;
	}
#endif

	//Assign SeDebugPrivilege privilege to current process,
	//without this, some processes may be unaccessable, like csrss.exe
	EnableDebugPrivilege();

	//Use EnumProcess to Enumerate all Active Processes
	//And store their PIDs in dwPIDs
	EnumProcesses(dwPIDs,sizeof(dwPIDs),&dwPidRetSize);

	_tprintf(TEXT("PID\tImage Full Path\n"));
	//crazy segmentation line
	_tprintf(TEXT("---     ------------------------------------------------------------------------"));

	//Try OpenProcess(...) with each possible PID,from 0 to 0x41DC,
	//if succeed, try to find the PID in dwPIDs,
	//if the PID isn't in dwPIDs,
	//then it may be a hidden process.
	for(DWORD i = 0;i <= 0x41DC; i += 4)
	{
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,i);
		if(hProc)
		{
			dwSuccess++;
			if(!SeekPID(dwPIDs,_countof(dwPIDs),i))
			{
				dwHidden++;

				dwSize = _countof(lpExeName);
				if(GetProcessImageFileName(hProc,lpExeName,dwSize))
				{
					if(RenamePath(lpExeName,lpBuffer))
						_tprintf(TEXT("%u\t%s\n"),i,lpBuffer);
					else
						_tprintf(TEXT("%u\t%s\n"),i,lpExeName);
				}
				else
					_tprintf(TEXT("%u\tCan not Obtain the Image Full Path.GetLastError() = %u\n"),i,GetLastError());
					
			}
			//Close the process' handle
			CloseHandle(hProc);
			hProc = NULL;
		}
	}
	//If there's no hidden process
	if(!dwHidden)
		_tprintf(TEXT("n/a\tn/a\n"));
	
	//Show the statistics
	_tprintf(TEXT("\nHidden Processes: %u\nTotal Processes(Self Not Included): %u.\n"),dwHidden,dwSuccess -1);

	return 0;
}

//--------------------------------------------------------------------
//Name:				EnableDebugPrivilege()
//Function:			Enable the SedDebugPrivilege on current process
//Return:			n/a
//					If fails, exit.
//--------------------------------------------------------------------
VOID EnableDebugPrivilege()
{
	//Parameters for AdjustTokenPrivileges(...)
	TOKEN_PRIVILEGES tp;					
	LUID luid;
	TCHAR lpszPrivilege[] = TEXT("SeDebugPrivilege");

	HANDLE hToken = NULL;					//Current process' token handle

	if( !OpenProcessToken(GetCurrentProcess(),TOKEN_ALL_ACCESS,&hToken) )
	{
		_tprintf(TEXT("OpenProcessToken Failed."));
		exit(EXIT_FAILURE);
	}

	if ( !LookupPrivilegeValue( 
			NULL,            // lookup privilege on local system
			lpszPrivilege,   // privilege to lookup 
			&luid ) )        // receives LUID of privilege
	{
		_tprintf(TEXT("LookupPrivilegeValue error: %u\n"), GetLastError() ); 
		exit(EXIT_FAILURE);
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if ( !AdjustTokenPrivileges(
		   hToken, 
		   FALSE, 
		   &tp, 
		   sizeof(TOKEN_PRIVILEGES), 
		   (PTOKEN_PRIVILEGES) NULL, 
		   (PDWORD) NULL) )
	{ 
		  _tprintf(TEXT("AdjustTokenPrivileges error: %u\n"), GetLastError() ); 
		  exit(EXIT_FAILURE);
	} 

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		  _tprintf(TEXT("The token does not have the specified privilege.\n")
				   TEXT("Make sure you have administrator privileges.\n"));
		  exit(EXIT_FAILURE);
	} 

}

//---------------------------------------------------------
//Name:			SeekPID(...)
//Function:		Find the specified PID in a PID list
//Return:		If find the specified PID, return TRUE;
//				else return FALSE.
//---------------------------------------------------------
BOOL SeekPID(IN DWORD *pdwArray,IN DWORD dwArraySize,IN DWORD dwDesiredPID)
{
	BOOL bRet = FALSE;

	for(DWORD i = 0; i != dwArraySize;++i)
	{
		if(dwDesiredPID == pdwArray[i])
		{
			bRet = TRUE;
			break;
		}
	}

	return bRet;
}

#ifdef CHECK_OS_VERSION
//---------------------------------------------------------------------------
//Name:			IsVistaOrLater()
//Function:		Test whether the current system if Windows Visa or later
//Return:		If the current system is Windows Vista or later, return TRUE;
//				else return FALSE;
//---------------------------------------------------------------------------
BOOL IsXpOrLater() 
{
   OSVERSIONINFOEX osvi;
   DWORDLONG dwlConditionMask = 0;
   int iOperation=VER_GREATER_EQUAL;

   // Initialize the OSVERSIONINFOEX structure.
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   osvi.dwMajorVersion = 5;
   osvi.dwMinorVersion = 1;
   osvi.wServicePackMajor = 0;
   osvi.wServicePackMinor = 0;


   // Initialize the condition mask.
   VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, iOperation );
   VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, iOperation );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, iOperation );
   VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR, iOperation );


   // Perform the test.
   return VerifyVersionInfo(
      &osvi, 
	  VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID,      
      dwlConditionMask);
}
#endif

//---------------------------------------
//Name:				Info()
//Function:			Show copyright info
//Return:			n/a
//---------------------------------------
//VOID Info( szName)
//{
//	_tprintf(TEXT("%s\tWhilter (c) 2007-2008\n")
//			 TEXT("Detect hidden processes with user mode mathod\n")
//			 TEXT("NOTE:\tSee readme.txt\n\n"),argv[0]);
//}

//----------------------------------------------------------------
//Name:				RenamePath(...)
//Function;			Convert the device form path to dos form path.
//Return;			If convertion was successful, return TRUE;;
//					else return FALSE;
//----------------------------------------------------------------
BOOL RenamePath(IN LPCTSTR lpDeviceFormPath,OUT LPTSTR lpBuffer)
{
	BOOL bRet = FALSE;
	DWORD dwOffset = _countof(TEXT("\\Device\\"));
	TCHAR szTempPath[MAX_PATH];
	TCHAR cLetter = TEXT('A');
	TCHAR szDosDriver[3];
	TCHAR szDeviceName[128];
	
	LPCTSTR lpPath = _tcschr(lpDeviceFormPath + dwOffset,TEXT('\\'));
	if(lpPath)
	{
		LPTSTR lpDeviceName = (LPTSTR)LocalAlloc(LPTR,(lpPath - lpDeviceFormPath + 1) * sizeof(TCHAR));
		StringCchCopy(lpDeviceName,lpPath - lpDeviceFormPath + 1,lpDeviceFormPath);
		//_tprintf(TEXT("%s\n"),lpDeviceName);

		do
		{
			StringCchPrintf(szDosDriver,_countof(szDosDriver),TEXT("%c:"),cLetter);

			if(QueryDosDevice(szDosDriver,szDeviceName,_countof(szDeviceName)))
			{
				//_tprintf(TEXT("%s\n"),szDeviceName);
				if(_tcscmp(lpDeviceName,szDeviceName) == 0)
				{
					StringCchPrintf(szTempPath,_countof(szTempPath),TEXT("%s\\%s"),szDosDriver,lpPath + 1);
					StringCchCopy(lpBuffer,MAX_PATH,szTempPath);
					
					bRet = TRUE;
					break;
				}

			}
		}while(cLetter++ != TEXT('Z'));

		LocalFree((HLOCAL)lpDeviceName);		
	}

	return bRet;
}

		
		



