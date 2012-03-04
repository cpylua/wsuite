#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <locale.h>
#include "ListStream.h"

void ShowError(DWORD dwErrorCode);
void ShowUsage(LPCTSTR lpFileName);

int _tmain(int argc,TCHAR *argv[])
{
	LPBYTE pInfoBlock = NULL;
    ULONG uInfoBlockSize = 0;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;
	BOOL bRet;
	HANDLE hFile;
	PFILE_STREAM_INFORMATION pStreamInfo = 0;
	ULONGLONG uTotalSize = 0;
	LARGE_INTEGER iFileSize;
	TCHAR szStreamName[MAX_PATH];
	TCHAR szPath[MAX_PATH];
	LPTSTR pszName = NULL;
	DWORD dwLen = 0;

	//To make _tprintf work with Chinese Character
	_tsetlocale(LC_ALL,TEXT("Chinese"));

	if( argc != 2)
	{
		ShowUsage(argv[0]);
		return -1;
	}

	//Load the function pointer
	NTQUERYINFORMATIONFILE NtQueryInformationFile;
	NtQueryInformationFile = (NTQUERYINFORMATIONFILE)GetProcAddress( GetModuleHandle(TEXT("ntdll.dll")),"NtQueryInformationFile" );
	if( !NtQueryInformationFile )
	{
		ShowError( GetLastError() );
		return -1;
	}

	hFile = CreateFile(argv[1],0/*no need of read or write access*/,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
	if( hFile == INVALID_HANDLE_VALUE )
	{
		ShowError( GetLastError() );
		return -1;
	}

	// Get stream information block.
	do 
	{
		uInfoBlockSize += 16 * 1024;

		if( pInfoBlock )
			delete[] pInfoBlock;

		pInfoBlock = new BYTE[uInfoBlockSize];

		((PFILE_STREAM_INFORMATION)pInfoBlock)->StreamNameLength = 0;
		status = NtQueryInformationFile(hFile, &ioStatus, (LPVOID)pInfoBlock, uInfoBlockSize, FileStreamInformation);
	}while( status == STATUS_BUFFER_OVERFLOW );
	CloseHandle(hFile);

	bRet = GetFullPathName(argv[1],MAX_PATH,szPath,&pszName);
	if( !bRet )
	{
		ShowError( GetLastError() );
		return -1;
	}
	_tprintf(TEXT("File: %s\n"),szPath);
	_tprintf(TEXT("Streams: ------------------------------------------------------------\n"));

	pStreamInfo = (PFILE_STREAM_INFORMATION)pInfoBlock;
    // Loop for all streams
    for (;;) 
	{
		// Check if stream info block is empty (directory may have no stream)
		if( pStreamInfo->StreamNameLength == 0 )
			break;

		//Get steam name
		memcpy_s(szStreamName,MAX_PATH * sizeof(WCHAR),pStreamInfo->StreamName,pStreamInfo->StreamNameLength);
		szStreamName[pStreamInfo->StreamNameLength / sizeof(WCHAR)] = L'\0';

		//Remove attribute tag
		LPWSTR pTag = wcsstr(szStreamName,L":$DATA");
		if( pTag )
			*pTag = L'\0';
		else
			szStreamName[1] = L'\0';

		// Full path including stream name
		StringCchCopy(szPath,MAX_PATH,argv[1]);
		if( wcscmp(szStreamName,L":") )
			StringCchCat(szPath,MAX_PATH,szStreamName);

		// Get stream size
		hFile = CreateFile(szPath,0,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
		if( hFile == INVALID_HANDLE_VALUE )
		{
			ShowError( GetLastError() );
			return -1;
		}
		bRet = GetFileSizeEx(hFile,&iFileSize);
		if( !bRet )
		{
			ShowError( GetLastError() );
			return -1;
		}
		CloseHandle(hFile);

		// Append spaces up to position 64
		StringCchLength(szStreamName,MAX_PATH,(size_t*)&dwLen);
	    if (dwLen < 64) 
		{
			StringCchCat(szStreamName,MAX_PATH,L"                                        ");
			szStreamName[40] = L'\0';
		}
	    else
			StringCchCat(szStreamName,MAX_PATH,L" ");

		_tprintf(TEXT("\t%s%I64u bytes\n"),szStreamName,iFileSize.QuadPart);
		uTotalSize += iFileSize.QuadPart;

		if( pStreamInfo->NextEntryOffset == 0 )
			break;
		else
			pStreamInfo = (PFILE_STREAM_INFORMATION)((LPBYTE)pStreamInfo + pStreamInfo->NextEntryOffset);
	}		//end of if(;;)

	if( uTotalSize )
		_tprintf(TEXT("Total stream size: %I64u bytes\n"),uTotalSize);
	else
		_tprintf(TEXT("No stream found"));


	return 0;
}

void ShowUsage(LPCTSTR lpFileName)
{
	_tprintf(TEXT("Usage: %s stream_name"),lpFileName);
}

void ShowError(DWORD dwErrorCode)
{
	LPVOID lpMsgBuf = NULL;
	LPVOID lpDisplayBuf = NULL;
	DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM	   |
					FORMAT_MESSAGE_IGNORE_INSERTS;

	FormatMessage(dwFlags,NULL,dwErrorCode,MAKELANGID(LANG_CHINESE_SIMPLIFIED,SUBLANG_CHINESE_SIMPLIFIED),(LPTSTR)&lpMsgBuf,0,NULL);
	lpDisplayBuf = (LPVOID)LocalAlloc(LPTR,(_tcslen( (LPCTSTR)lpMsgBuf ) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,LocalSize((HLOCAL)lpDisplayBuf) / sizeof(TCHAR),TEXT("Failed with error %d: %s"),dwErrorCode,lpMsgBuf);
	_tprintf(TEXT("%s\n"),(LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	//exit(-1);
}

