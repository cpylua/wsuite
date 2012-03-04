#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <locale.h>
#include <shlwapi.h>
#include "ntfs_adstool.h"
#include "resource.h"

#pragma comment(lib,"shlwapi.lib")

const DWORD dwBufferSize = 64 * 1024;
#define ALLOWED_ATTRIBUTES (FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)

void ShowError(DWORD dwErrorCode);
void ShowUsage(LPCTSTR lpFileName);

BOOL CopyStream(LPTSTR *lpStreamPaths);
BOOL DelStream(LPCTSTR lpStreamPath);
BOOL ListStream(LPCTSTR lpStreamPath);


int _tmain(int argc,TCHAR *argv[])
{
	BOOL bValidCmd = FALSE;

	//To make _tprintf work with Chinese Character
	_tsetlocale(LC_ALL,TEXT("Chinese"));

	_tprintf(TEXT("NTFS Alternate Data Streams Tool\nCopyright(C) 2007-2008 Whislter Lee\n\n"));

	if( argc > 2)
	{
		if( !_tcsicmp(argv[1],TEXT("/c")) || !_tcsicmp(argv[1],TEXT("-c")) )
		{
			if( argc == 4 )
			{
				bValidCmd = TRUE;
				CopyStream(&argv[2]);					
			}			
		}
		else if( !_tcsicmp(argv[1],TEXT("/d")) || !_tcsicmp(argv[1],TEXT("-d")) )
		{
			if( argc == 3 )
			{
				bValidCmd = TRUE;
				DelStream(argv[2]);
					
			}
		}
		else if( !_tcsicmp(argv[1],TEXT("/l")) || !_tcsicmp(argv[1],TEXT("-l")) )
		{
			if( argc == 3 )
			{
				bValidCmd = TRUE;
				ListStream(argv[2]);
			}
		}
	}

	if( !bValidCmd )
	{
		ShowUsage(argv[0]);
		return -1;
	}

	return 0;
}

BOOL CopyStream(LPTSTR *lpStreamPaths)
{
	BY_HANDLE_FILE_INFORMATION bhfi;
	DWORD dwBytesWritten = 0;
	DWORD dwBytesRead = 0;
	BOOL bRet;
	
	if( PathIsDirectory(lpStreamPaths[0]) )
	{
		_tprintf(TEXT("Operation failed: %s is a directory.\n"),lpStreamPaths[0]);
		return FALSE;
	}
	if( PathIsDirectory(lpStreamPaths[1]) )
	{
		_tprintf(TEXT("Operation failed: %s is a directory.\n"),lpStreamPaths[1]);
		return FALSE;
	}

	HANDLE hInFile = CreateFile(lpStreamPaths[0],GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if( hInFile == INVALID_HANDLE_VALUE )
		goto ErrorExit;

	bRet = GetFileInformationByHandle(hInFile,&bhfi);
	if( !bRet )
		goto ErrorExit;

	HANDLE hOutFile = CreateFile(lpStreamPaths[1],GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL,NULL);
	if( hOutFile == INVALID_HANDLE_VALUE )
		goto ErrorExit;

	//Allocate buffer
	LPVOID lpBuffer = LocalAlloc(LPTR,dwBufferSize);
	do
	{
		bRet = ReadFile(hInFile,lpBuffer,dwBufferSize,&dwBytesRead,NULL);
		if( !bRet )
			goto ErrorExit;

		bRet = WriteFile(hOutFile,lpBuffer,dwBytesRead,&dwBytesWritten,NULL);
		if( !bRet || dwBytesWritten < dwBytesRead )
			goto ErrorExit;

	}while( dwBytesRead == dwBufferSize );

	CloseHandle(hInFile);

	bRet = SetFileTime(hOutFile,&bhfi.ftCreationTime,&bhfi.ftLastAccessTime,&bhfi.ftLastWriteTime);
	if( !bRet )
		goto ErrorExit;

	CloseHandle(hOutFile);

	bRet = SetFileAttributes(lpStreamPaths[1],bhfi.dwFileAttributes & ALLOWED_ATTRIBUTES);
	if( !bRet )
		goto ErrorExit;

	_tprintf(TEXT("Stream copyed.\n"));

	return TRUE;

ErrorExit:
		ShowError( GetLastError() );
		return FALSE;

}

BOOL DelStream(LPCTSTR lpStreamPath)
{
	BOOL bRet;

	if( PathIsDirectory(lpStreamPath) )
	{
		_tprintf(TEXT("Operation failed: %s is a directory.\n"),lpStreamPath);
		return FALSE;
	}

	bRet = DeleteFile( lpStreamPath );
	if( !bRet )
	{
		ShowError( GetLastError() );
		return FALSE;
	}

	_tprintf(TEXT("Stream deleted.\n"));

	return TRUE;
}

BOOL ListStream(LPCTSTR lpStreamPath)
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

	NTQUERYINFORMATIONFILE NtQueryInformationFile;
	NtQueryInformationFile = (NTQUERYINFORMATIONFILE)GetProcAddress( GetModuleHandle(TEXT("ntdll.dll")),"NtQueryInformationFile" );
	if( !NtQueryInformationFile )
		goto ErrorExit;

	hFile = CreateFile(lpStreamPath,0/*no need of read or write access*/,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
	if( hFile == INVALID_HANDLE_VALUE )
		goto ErrorExit;

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

	bRet = GetFullPathName(lpStreamPath,MAX_PATH,szPath,&pszName);
	if( !bRet )
		goto ErrorExit;

	_tprintf(TEXT("File: %s\n"),szPath);
	_tprintf(TEXT("Streams: ----------------------------------------------------------------------\n"));

	pStreamInfo = (PFILE_STREAM_INFORMATION)pInfoBlock;
    // Loop for all streams
    do
	{
		// Check if stream info block is empty (directory may have no stream)
		if( pStreamInfo->StreamNameLength == 0 )
			break;

		memcpy_s(szStreamName,MAX_PATH * sizeof(WCHAR),pStreamInfo->StreamName,pStreamInfo->StreamNameLength);
		szStreamName[pStreamInfo->StreamNameLength / sizeof(WCHAR)] = L'\0';

		//Remove attribute tag
		LPWSTR pTag = wcsstr(szStreamName,L":$DATA");
		if( pTag )
			*pTag = L'\0';
		else
			szStreamName[1] = L'\0';

		//Build full path including the stream name
		StringCchCopy(szPath,MAX_PATH,lpStreamPath);
		if( wcscmp(szStreamName,L":") )
			StringCchCat(szPath,MAX_PATH,szStreamName);

		// Get stream size
		hFile = CreateFile(szPath,0,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
		if( hFile == INVALID_HANDLE_VALUE )
			goto ErrorExit;

		bRet = GetFileSizeEx(hFile,&iFileSize);
		if( !bRet )
			goto ErrorExit;

		CloseHandle(hFile);

		// Append spaces up to position 53
		StringCchLength(szStreamName,MAX_PATH,(size_t*)&dwLen);
	    if (dwLen < 53) 
		{
			StringCchCat(szStreamName,MAX_PATH,L"                                                     ");
			szStreamName[53] = L'\0';
		}
	    else
			StringCchCat(szStreamName,MAX_PATH,L" ");

		_tprintf(TEXT("\t%s%I64u bytes\n"),szStreamName,iFileSize.QuadPart);
		uTotalSize += iFileSize.QuadPart;

		if( pStreamInfo->NextEntryOffset == 0 )
			break;
		else
			pStreamInfo = (PFILE_STREAM_INFORMATION)((LPBYTE)pStreamInfo + pStreamInfo->NextEntryOffset);

	} while(true);	

	if( uTotalSize )
		_tprintf(TEXT("Total stream size: %I64u bytes\n"),uTotalSize);
	else
		_tprintf(TEXT("No stream found"));

	return TRUE;

ErrorExit:
		ShowError( GetLastError() );
		return FALSE;
}

void ShowUsage(LPCTSTR lpFileName)
{
	_tprintf(TEXT("Usage: %s /c src_stream dest_stream\n"),lpFileName);
	_tprintf(TEXT("       %s /d stream_path\n"),lpFileName);
	_tprintf(TEXT("       %s /l stream_path\n"),lpFileName);
}

void ShowError(DWORD dwErrorCode)
{
	LPVOID lpMsgBuf = NULL;
	LPVOID lpDisplayBuf = NULL;
	DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM	   |
					FORMAT_MESSAGE_IGNORE_INSERTS;

	FormatMessage(dwFlags,NULL,dwErrorCode,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR)&lpMsgBuf,0,NULL);
	lpDisplayBuf = (LPVOID)LocalAlloc(LPTR,(_tcslen( (LPCTSTR)lpMsgBuf ) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,LocalSize((HLOCAL)lpDisplayBuf) / sizeof(TCHAR),TEXT("Failed with error %d: %s"),dwErrorCode,lpMsgBuf);
	_tprintf(TEXT("%s\n"),(LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}