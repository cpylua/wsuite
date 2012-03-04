#include <windows.h>
#include <cwchar>

extern IMAGE_NT_HEADERS	ntHdr;
extern IMAGE_DOS_HEADER	dosHdr;
extern TCHAR	szFileName[1024];

void ShowError()
{
    TCHAR szBuf[128]; 
    LPVOID lpMsgBuf;
    DWORD dwErrorCode = GetLastError(); 
	if (dwErrorCode)
	{
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dwErrorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		wcscpy_s(szBuf,128,(const TCHAR*)lpMsgBuf); 	 
		MessageBox(NULL, szBuf, L"PEditor Error", MB_OK|MB_ICONSTOP); 
	    LocalFree(lpMsgBuf);
	}

}

BOOL SetEditText(HWND hWnd,int nDlgItemID,DWORD dwNumToSet)
{
	TCHAR szDlgText[9];
	if (_ultow_s(dwNumToSet,szDlgText,9,16))			//Zero if the function was successful or an error code
	{
		ShowError();
		return FALSE;
	}
	for (int i = 0;i<9;++i)
	{
		if(iswalpha(szDlgText[i]))
		{
			szDlgText[i] = toupper(szDlgText[i]);
		}
	}
	return SetDlgItemText(hWnd,nDlgItemID,szDlgText);
}

DWORD GetStrToNum(HWND hDlg,int nIDDlgItem,BOOL* bRet)
{
	TCHAR str[9];
	UINT dwNumCopyed;
	TCHAR* stopstr = NULL;
	DWORD dwNum = 0;

	dwNumCopyed = GetDlgItemText(hDlg,nIDDlgItem,str,9);
	if (dwNumCopyed)
	{
		*bRet = TRUE;
		dwNum = wcstoul(str,&stopstr,16);
		if ((dwNum ==ULLONG_MAX && !wcscmp(str,L"FFFFFFFF")) || (!dwNum && wcscmp(str,L"0")))
		{
			ShowError();
			*bRet = FALSE;
			return 0;
		}
		else
		{
			return dwNum;
		}
	
	}
	ShowError();
	*bRet = FALSE;
	return 0;
}

BOOL IsPEFile(TCHAR* lpFileName)
{

	BOOL bResult;

	HANDLE hFile = CreateFile(lpFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwNumOfBytesRead = 0;
		bResult = ReadFile(hFile,&dosHdr,sizeof(IMAGE_DOS_HEADER),&dwNumOfBytesRead,NULL);
		if (bResult)
		{
			DWORD dwPtr;
			dwPtr = SetFilePointer(hFile,dosHdr.e_lfanew,NULL,FILE_BEGIN);
			if (dwPtr != INVALID_SET_FILE_POINTER)
			{
				dwNumOfBytesRead = 0;
				bResult = ReadFile(hFile,&ntHdr,sizeof(IMAGE_NT_HEADERS),&dwNumOfBytesRead,NULL);
				if (bResult && dosHdr.e_magic == IMAGE_DOS_SIGNATURE && ntHdr.Signature == IMAGE_NT_SIGNATURE)
				{
					CloseHandle (hFile);
					return TRUE;
				}
			}
		}
	}
	ShowError();
	CloseHandle (hFile);
	return FALSE;
}