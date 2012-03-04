#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <locale.h>

const DWORD dwBufferSize = 64 * 1024;
#define ALLOWED_ATTRIBUTES (FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)

void ShowError(DWORD dwErrorCode);
void ShowUsage(LPCTSTR lpFileName);

int _tmain(int argc,TCHAR *argv[])
{
	//To make _tprintf work with Chinese Character
	_tsetlocale(LC_ALL,TEXT("Chinese"));

	BY_HANDLE_FILE_INFORMATION bhfi;
	DWORD dwBytesWritten = 0;
	DWORD dwBytesRead = 0;
	BOOL bRet;

	if( argc != 3 )
	{
		ShowUsage(argv[0]);
		return -1;
	}

	HANDLE hInFile = CreateFile(argv[1],GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if( hInFile == INVALID_HANDLE_VALUE )
		ShowError( GetLastError() );

	bRet = GetFileInformationByHandle(hInFile,&bhfi);
	if( !bRet )
		ShowError( GetLastError() );

	HANDLE hOutFile = CreateFile(argv[2],GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL,NULL);
	if( hOutFile == INVALID_HANDLE_VALUE )
		ShowError( GetLastError() );

	//Allocate buffer
	LPVOID lpBuffer = LocalAlloc(LPTR,dwBufferSize);
	do
	{
		bRet = ReadFile(hInFile,lpBuffer,dwBufferSize,&dwBytesRead,NULL);
		if( !bRet )
			ShowError( GetLastError() );

		bRet = WriteFile(hOutFile,lpBuffer,dwBytesRead,&dwBytesWritten,NULL);
		if( !bRet || dwBytesWritten < dwBytesRead )
			ShowError( GetLastError() );

	}while( dwBytesRead == dwBufferSize );

	CloseHandle(hInFile);

	bRet = SetFileTime(hOutFile,&bhfi.ftCreationTime,&bhfi.ftLastAccessTime,&bhfi.ftLastWriteTime);
	if( !bRet )
		ShowError( GetLastError() );

	CloseHandle(hOutFile);

	bRet = SetFileAttributes(argv[2],bhfi.dwFileAttributes & ALLOWED_ATTRIBUTES);
	if( !bRet )
		ShowError( GetLastError() );

	_tprintf(TEXT("Stream copyed.\n"));
	return 0;
}

void ShowUsage(LPCTSTR lpFileName)
{
	_tprintf(TEXT("Usage: %s src_stream dest_stream"),lpFileName);
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

	exit(-1);
}
