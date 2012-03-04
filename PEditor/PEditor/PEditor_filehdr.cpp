#include "resource.h"
#include <windows.h>
#include <winnt.h>


extern IMAGE_NT_HEADERS	ntHdr;
extern IMAGE_DOS_HEADER	dosHdr;
extern TCHAR	szFileName[1024];
extern HINSTANCE hAppHwnd;

void ShowError();
BOOL SetEditText(HWND hWnd,int nDlgItemID,DWORD dwNumToSet);
DWORD GetStrToNum(HWND hDlg,int nIDDlgItem,BOOL* bRet);
BOOL IsPEFile(TCHAR* lpFileName);

LRESULT CALLBACK DosHdrDlg(HWND hDosHdrDlg, UINT message, WPARAM wParam, LPARAM lParam);




LRESULT CALLBACK FileHdrDlg(HWND hFileHdrDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static BOOL bIsPE;
	static BOOL bWarned = FALSE;
	HANDLE hFile;
	DWORD dwPtr = 0;
	DWORD dwNumOfBytesWritten = 0;
	BOOL bRet;	
	HICON hIcon;


	switch (message)
	{
	case WM_INITDIALOG:
		//load icon
		hIcon = LoadIcon(hAppHwnd,(LPCTSTR)IDI_ICON_PE);
		SendMessage(hFileHdrDlg,WM_SETICON,ICON_BIG,(LPARAM)hIcon);

		SetEditText(hFileHdrDlg,IDC_EDIT_MACHINE,ntHdr.FileHeader.Machine);
		SetEditText(hFileHdrDlg,IDC_EDIT_CHARACTERISTICS,ntHdr.FileHeader.Characteristics);
		SetEditText(hFileHdrDlg,IDC_EDIT_NUMOFSECTIONS,ntHdr.FileHeader.NumberOfSections);
		SetEditText(hFileHdrDlg,IDC_EDIT_NUMOFSYMBOLS,ntHdr.FileHeader.NumberOfSymbols);
		SetEditText(hFileHdrDlg,IDC_EDIT_POINTERTOSYMBOLTABLE,ntHdr.FileHeader.PointerToSymbolTable);
		SetEditText(hFileHdrDlg,IDC_EDIT_SIZEOFOPTIONALHEADER,ntHdr.FileHeader.SizeOfOptionalHeader);
		SetEditText(hFileHdrDlg,IDC_EDIT_TIMESTAMP,ntHdr.FileHeader.TimeDateStamp);
		break;
	case WM_CLOSE:
		if (!EndDialog(hFileHdrDlg,0))
		{
			ShowError();
			ExitProcess(0);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_FILEHDR_OK:
			//add codes here
			ntHdr.FileHeader.NumberOfSections = (WORD)GetStrToNum(hFileHdrDlg,IDC_EDIT_NUMOFSECTIONS,&bRet);
			if (bRet)
			{
				ntHdr.FileHeader.Machine = (WORD)GetStrToNum(hFileHdrDlg,IDC_EDIT_MACHINE,&bRet);
				if (bRet)
				{
					ntHdr.FileHeader.Characteristics = (WORD)GetStrToNum(hFileHdrDlg,IDC_EDIT_CHARACTERISTICS,&bRet);
					if (bRet)
					{
						ntHdr.FileHeader.NumberOfSymbols = (DWORD)GetStrToNum(hFileHdrDlg,IDC_EDIT_NUMOFSYMBOLS,&bRet);
						if (bRet)
						{
							ntHdr.FileHeader.PointerToSymbolTable = (DWORD)GetStrToNum(hFileHdrDlg,IDC_EDIT_POINTERTOSYMBOLTABLE,&bRet);
							if (bRet)
							{
								ntHdr.FileHeader.SizeOfOptionalHeader = (WORD)GetStrToNum(hFileHdrDlg,IDC_EDIT_SIZEOFOPTIONALHEADER,&bRet);
								if (bRet)
								{
									ntHdr.FileHeader.TimeDateStamp = (DWORD)GetStrToNum(hFileHdrDlg,IDC_EDIT_TIMESTAMP,&bRet);
									if (bRet)
									{				
										hFile = CreateFile(szFileName,FILE_ALL_ACCESS,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
										dwPtr = SetFilePointer(hFile,dosHdr.e_lfanew + sizeof(ntHdr.Signature),NULL,FILE_BEGIN);
										if (dwPtr != INVALID_SET_FILE_POINTER)
										{
											WriteFile(hFile,&ntHdr.FileHeader,sizeof(ntHdr.FileHeader),&dwNumOfBytesWritten,NULL);																			

										}
									}
								}
							}
						}
					}
				}
			}

			ShowError();
			CloseHandle(hFile);
			if (!EndDialog(hFileHdrDlg,0))
			{
				ShowError();
				ExitProcess(0);
			}
			break;
		case IDC_BUTTON_DOSHEADER:
			//TODO:  add codes here
			ShowWindow(hFileHdrDlg,SW_HIDE);
			DialogBoxParam(hAppHwnd,(LPCTSTR)IDD_DIALOG_DOSHEADER,NULL,(DLGPROC)DosHdrDlg,NULL);

			if (!bWarned)
			{
				bWarned = TRUE;
				bIsPE = IsPEFile(szFileName);
				if (!bIsPE)
				{
					MessageBox(NULL,L"    Because of your last modification,\nthe current file is an invalide PE file now.",L"PEditor Warning",MB_OK|MB_ICONWARNING);
				}
			}
			/*
			else
			{
				bIsPE = IsPEFile(szFileName);
				if (bIsPE)
				{
					bWarned = FALSE;
					MessageBox(NULL,L"    Because of your last modification,\nthe current file is a valide PE file now." ,L"PEditor",MB_OK|MB_ICONINFORMATION);
				}
			}
			*/
			ShowWindow(hFileHdrDlg,SW_RESTORE);
			break;
		}
		break;
	}
	return 0;
}

