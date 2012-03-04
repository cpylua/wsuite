#include <windows.h>
#include <cstdio>

using namespace std;

int wmain(int argc,TCHAR *argv[])
{
	if( argc == 3)
	{
		TCHAR *pEndPointer = NULL;
		LARGE_INTEGER LISize;	

		LISize.HighPart = 1;
		LISize.LowPart = wcstoul(argv[2] + 1,&pEndPointer,10);

		//is it larger than 4GB?
		if (LISize.LowPart > 0x400000)
		{
			wprintf(TEXT("You cannot create a file that is larger than 4GB.\n"));
			exit(EXIT_FAILURE);
		}
		else if(LISize.LowPart < 0x400000)			
			LISize.HighPart = 0;

		LISize.LowPart *= 1024;

		HANDLE hFile = CreateFile(argv[1] + 1,FILE_ALL_ACCESS,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (INVALID_HANDLE_VALUE != hFile)
		{
			LockFile(hFile,0,0,LISize.LowPart,LISize.HighPart);
			SetFilePointerEx(hFile,LISize,NULL,FILE_BEGIN);
			SetEndOfFile(hFile);
			UnlockFile(hFile,0,0,LISize.LowPart,LISize.HighPart);
			
			CloseHandle(hFile);
			wprintf(TEXT("\"%s\" successfully created."),argv[1] + 1);
		}
		else
		{
			wprintf(TEXT("Create file \"%s\" failed!"),argv[1] +1);
			exit(EXIT_FAILURE);
		}

	}
	else
	{
		wprintf(TEXT("Usage:%s -FileName -FileSize\n      -FileName -- name of the new file\n      -FileSize -- size of file in KBytes\n"),argv[0]);
		//system("pause");
		exit(EXIT_FAILURE);
	}

	return 0;
}

