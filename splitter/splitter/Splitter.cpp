#include <iostream>
#include "windows.h"

using namespace std;

const unsigned int PATHLIMIT = 0x7FFF;

int wmain(int argc, TCHAR *argv[])
{
	TCHAR szHelp[] = TEXT(" [-merge | -splitter] [-split size(bytes)] [source] [destination]\n\nExamples: \n\nsplitter -split -1024 c:\\test.dat c:\\split\\test\nThis will ")
					 TEXT("split the source file into some small files with the names \"testX.splt\"(x is a number beginning from 0) and store the splitted files in the destination fold.\n\n")
					 TEXT("splitter -merge c\\split\\test c:\\merge.dat\nThis will merge the splitted files to a single file named merge.dat in the directory \"c:\\\"\n");
	TCHAR *szArgument = argv[1] + 1;
	DWORD dSplitSize = 0;
	DWORD dFileSize = 0;
	TCHAR szSourceFile[PATHLIMIT] = TEXT("\\\\?\\");
	TCHAR szDesFile[PATHLIMIT] = TEXT("\\\\?\\");
	TCHAR szNameFix[12] = {TEXT('\0')};
	TCHAR szDesFileTemp[PATHLIMIT];
	HANDLE hFileRead = 0;
	HANDLE hFileWrite = 0;
	DWORD dCount = 0;
	TCHAR *EndPointer = 0;
	DWORD dNumRead = 0;
	DWORD dNumWritten = 0;
	LPVOID lpBuffer = 0;
	DWORD dwPos = 0;
	
	if (argc > 1)
	{
		if (!wcsncmp(szArgument,TEXT("split"),6))
		{
			if (argc!= 5)
			{
				wcout << argv[0] << szHelp << endl;
				//system("pause");
				exit(EXIT_FAILURE);
			}
			dSplitSize = wcstoul(argv[2] + 1,&EndPointer,10);
			
			wcsncat_s(szSourceFile,PATHLIMIT - 1,argv[3],PATHLIMIT-1);
			
			wcsncat_s(szDesFile,PATHLIMIT - 1,argv[4],PATHLIMIT-1);
			wcsncpy_s(szDesFileTemp,PATHLIMIT - 1,szDesFile,PATHLIMIT - 1);
			hFileRead = CreateFile((LPTSTR)szSourceFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,0);
			if (hFileRead != INVALID_HANDLE_VALUE)
			{
				dFileSize = GetFileSize(hFileRead,NULL);
				
				if ( dFileSize >= dSplitSize )
				{
					lpBuffer = (LPVOID)(new char[dSplitSize]);
					for (;dCount != dFileSize / dSplitSize;dCount++)
					{    
						ReadFile(hFileRead,lpBuffer,dSplitSize,&dNumRead,NULL);
						_ultow_s(dCount,szNameFix,sizeof(szNameFix) - 1,10);
						wcsncat_s(szDesFile,PATHLIMIT - 1,szNameFix,sizeof(szNameFix));
						wcsncat_s(szDesFile,PATHLIMIT - 1,TEXT(".splt"),6);
						hFileWrite = CreateFile((LPTSTR) szDesFile,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,NULL);
						WriteFile(hFileWrite,lpBuffer,dSplitSize,&dNumWritten,0);
						CloseHandle(hFileWrite);
						wcsncpy_s(szDesFile,PATHLIMIT - 1,szDesFileTemp,PATHLIMIT - 1);
					}
					delete [] lpBuffer;
					if (!(GetFileSize(hFileRead,NULL) % dSplitSize))
					{
						CloseHandle(hFileRead);
						wcout << TEXT("\nRequest Done!!") << endl;
						//system("pause");
						exit(EXIT_SUCCESS);
					}

				}
				lpBuffer = (LPVOID)(new char[dFileSize - dCount * dSplitSize]);
				ReadFile(hFileRead,lpBuffer,dFileSize - dCount * dSplitSize,&dNumRead,NULL);
				
				_ultow_s(dCount,szNameFix,sizeof(szNameFix) - 1,10);
				wcsncat_s(szDesFile,PATHLIMIT - 1,szNameFix,sizeof(szNameFix));
				wcsncat_s(szDesFile,PATHLIMIT - 1,TEXT(".splt"),6);
				
				hFileWrite = CreateFile((LPTSTR) szDesFile,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,NULL);
				WriteFile(hFileWrite,lpBuffer,dFileSize - dCount * dSplitSize,&dNumWritten,0);

				delete [] lpBuffer;
				CloseHandle(hFileWrite);				
				CloseHandle(hFileRead);

				wcout << TEXT("\nRequest Done!!") << endl;
			}
			else
			{
				wcout << TEXT("Open file failed!") << endl;
				//system("pause");
				exit(EXIT_FAILURE);
			}			
        }
		else if (!wcsncmp(szArgument,TEXT("merge"),6))
		{
			if (argc != 4)
			{
				wcout << argv[0] << szHelp << endl;
				//system("pause");
				exit(EXIT_FAILURE);
			}
			wcsncat_s(szDesFile,PATHLIMIT - 1,argv[3] ,PATHLIMIT);
			
			do
			{
				wcsncat_s(szSourceFile,PATHLIMIT - 1,argv[2] ,PATHLIMIT);				
				_ultow_s(dCount,szNameFix,sizeof(szNameFix) - 1,10);
				wcsncat_s(szSourceFile,PATHLIMIT - 1,szNameFix,sizeof(szNameFix));
				wcsncat_s(szSourceFile,PATHLIMIT - 1,TEXT(".splt"),6);
				
				hFileRead = CreateFile(szSourceFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,NULL);
				if (hFileRead != INVALID_HANDLE_VALUE)
				{
					hFileWrite = CreateFile((LPTSTR)szDesFile,GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);					
					if (hFileWrite != INVALID_HANDLE_VALUE)
					{
						lpBuffer = (LPVOID) (new char[GetFileSize(hFileRead,NULL)]);
						
						ReadFile(hFileRead,lpBuffer,GetFileSize(hFileRead,NULL),&dNumRead,NULL);
						
						//这里需要锁定文件，防止其他线程对文件的访问，否则写操作会有意想不到的结果	
						dwPos = SetFilePointer(hFileWrite, 0, NULL, FILE_END);
						LockFile(hFileWrite, dwPos, 0, GetFileSize(hFileRead,NULL), 0);
						WriteFile(hFileWrite,lpBuffer,GetFileSize(hFileRead,NULL),&dNumWritten,NULL);
						UnlockFile(hFileWrite, dwPos, 0, GetFileSize(hFileRead,NULL), 0);

						
						CloseHandle(hFileWrite);
						CloseHandle(hFileRead);
						delete [] lpBuffer;
						dCount++;
						wcsncpy_s(szSourceFile,PATHLIMIT - 1,TEXT("\\\\?\\"),8);
					
					}
					else
					{
						wcout << TEXT("Open destination file failed!") << endl;
						//system("pause");
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					if (dCount != 0)
					{
						wcout << TEXT("\nRequest Done!") << endl;
						//system("pause");
						exit(EXIT_SUCCESS);
					}
					else
					{
						wcout << TEXT("Merge failed!") << endl;
						//system("pause");
						exit(EXIT_FAILURE);
					}
				}
			} while(true);
			

		}
	
		else
		{
			wcout << argv[0] << szHelp << endl;
			//system("pause");
			exit(EXIT_FAILURE);
		}

	}
	else
	{
		wcout << argv[0] << szHelp << endl;
		//system("pause");
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
	
}