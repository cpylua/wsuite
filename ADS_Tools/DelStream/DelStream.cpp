#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <locale.h>

void ShowError(DWORD dwErrorCode);
void ShowUsage(LPCTSTR lpFileName);

int _tmain(int argc,TCHAR *argv[])
{
	//To make _tprintf work with Chinese Character
	_tsetlocale(LC_ALL,TEXT("Chinese"));

	if( argc != 2)
	{
		ShowUsage(argv[0]);
		return 0;
	}
	
	BOOL bRet;
	bRet = DeleteFile( argv[1] );
	if( !bRet )
		ShowError( GetLastError() );

	_tprintf(TEXT("Stream deleted.\n"));
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

	exit(-1);
}
