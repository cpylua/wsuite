#include "resource.h"
#include <windows.h>
#include <cwchar>

const char DIRECTION_NOTCHANGED = 0;
const char DIRECTION_UPCHANGED = 1;
const char DIRECTION_DOWNCHANGED = -1;

LRESULT CALLBACK SectHdrDlg(HWND hSectHdrDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ShowError();
BOOL SetEditText(HWND hWnd,int nDlgItemID,DWORD dwNumToSet);
DWORD GetStrToNum(HWND hDlg,int nIDDlgItem,BOOL* bRet);
BOOL IsPEFile(TCHAR* lpFileName);
void UpdateData(HWND hDlg,int& nIndex,const char cDirection);

extern IMAGE_NT_HEADERS	ntHdr;
extern IMAGE_DOS_HEADER	dosHdr;
extern TCHAR	szFileName[1024];
extern HINSTANCE hAppHwnd;

IMAGE_SECTION_HEADER SectHdr;


LRESULT CALLBACK SectHdrDlg(HWND hSectHdrDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int nIndex = 1;
	int nTotal = ntHdr.FileHeader.NumberOfSections;
	HANDLE hFile;
	DWORD dwPtr = 0;
	DWORD dwNumOfBytesWritten = 0;
	BOOL bRet;
	char szName[8];
	int nIniIndex = 1;
	HICON hIcon;

	switch (message)
	{
	case WM_CLOSE:
		if (!EndDialog(hSectHdrDlg,0))
		{
			ShowError();
			ExitProcess(0);
		}
		nIndex = 1;
		break;
	case WM_INITDIALOG:
		//load icon
		hIcon = LoadIcon(hAppHwnd,(LPCTSTR)IDI_ICON_PE);
		SendMessage(hSectHdrDlg,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
		//disable buttons
		EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_PREV),FALSE);
		
		//update
		UpdateData(hSectHdrDlg,nIniIndex,DIRECTION_NOTCHANGED);

		//disable button,must be placed here
		EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_SECT_CANCEL:
			EndDialog(hSectHdrDlg,0);
			break;
		case IDC_BUTTON_SECT_NEXT:
			++nIndex;

				/*
			case 1:
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_PREV),FALSE);
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_DOWNCHANGED);
				//disable button,must be placed here
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
				break;
				*/
			if (nIndex== 2)
			{
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_PREV),TRUE);
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_UPCHANGED);
				//disable button,must be placed here
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
			}
			else if (nIndex == nTotal)
			{
				
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_NEXT),FALSE);
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_PREV),TRUE);
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_UPCHANGED);
				//disable button,must be placed here
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
			}
				/*
			case nTotal-1:
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_UPCHANGED);
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_NEXT),TRUE);
				//disable button,must be placed here
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
				break;
				*/
			else
			{
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_UPCHANGED);
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
				
			}
		break;
		case IDC_BUTTON_SECT_PREV:
			--nIndex;
			if (nIndex == nTotal-1)
			{
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_NEXT),TRUE);
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_PREV),TRUE);
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_DOWNCHANGED);
				//disable button,must be placed here
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
			}
			else if (nIndex == 2)
			{
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_PREV),TRUE);
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_DOWNCHANGED);
				//disable button,must be placed here
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
			}
			else if (nIndex == 1)
			{
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_DOWNCHANGED);
				//disable button,must be placed here
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_PREV),FALSE);
			}
			else
			{
				UpdateData(hSectHdrDlg,nIndex,DIRECTION_DOWNCHANGED);
				//disable button,must be placed here
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
			}
				/*
			default:
				EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_PREV),FALSE);
				MessageBox(NULL,L"Is it worth to do this?",L"Your Fault",MB_OK|MB_ICONQUESTION);
				nIndex = 1;
				break;
				*/
			
			break;
		case IDC_BUTTON_SECT_UPDATE:
			//disable button
			EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),FALSE);
			//get datas
			SectHdr.Characteristics = (DWORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_SECT_CHARACTERICIS,&bRet);
			SectHdr.Misc.PhysicalAddress =(DWORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_PHYADDR,&bRet);
			SectHdr.NumberOfLinenumbers = (WORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_NUMOFLINENUM,&bRet);
			SectHdr.NumberOfRelocations = (WORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_NUMOFRELOC,&bRet);
			SectHdr.PointerToLinenumbers = (DWORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_PTTOLINENUM,&bRet);
			SectHdr.PointerToRawData = (DWORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_POINTERTORAWDATA,&bRet);
			SectHdr.PointerToRelocations = (DWORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_PTTORELOC,&bRet);
			SectHdr.SizeOfRawData = (DWORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_SIZEOFRAWDATA,&bRet);
			SectHdr.VirtualAddress = (DWORD)GetStrToNum(hSectHdrDlg,IDC_EDIT_VIRTUALADDR,&bRet);
			if (GetDlgItemTextA(hSectHdrDlg,IDC_EDIT_NAME,szName,8))
			{
				strcpy_s((char*)SectHdr.Name,8,szName);
			}
			else
			{
				ShowError();
				break;
			}

			//save to file
			hFile = CreateFile(szFileName,FILE_ALL_ACCESS,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				dwPtr = SetFilePointer(hFile,dosHdr.e_lfanew + sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER) * (nIndex-1),NULL,FILE_BEGIN);
				if (dwPtr != INVALID_SET_FILE_POINTER)
				{
					WriteFile(hFile,&SectHdr,sizeof(IMAGE_SECTION_HEADER),&dwNumOfBytesWritten,NULL);
					CloseHandle(hFile);
					break;
				}
			}
			ShowError();
			break;

			case IDC_EDIT_NAME:
			case IDC_EDIT_PHYADDR:
			case IDC_EDIT_VIRTUALADDR:
			case IDC_EDIT_SIZEOFRAWDATA:
			case IDC_EDIT_POINTERTORAWDATA:
			case IDC_EDIT_PTTORELOC:
			case IDC_EDIT_PTTOLINENUM:
			case IDC_EDIT_NUMOFRELOC:
			case IDC_EDIT_NUMOFLINENUM:
			case IDC_EDIT_SECT_CHARACTERICIS:
				switch (HIWORD(wParam))
				{
				case EN_CHANGE:
					EnableWindow(GetDlgItem(hSectHdrDlg,IDC_BUTTON_SECT_UPDATE),TRUE);
					break;
				}
				break;
		}
		break;
	}
	return 0;
}

void UpdateData(HWND hDlg,int& nIndex,const char cDirection)		
{
		HANDLE hFile;
		DWORD dwNumOfBytesRead = 0;
		DWORD dwPtr = 0;

		hFile = CreateFile(szFileName,FILE_ALL_ACCESS,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			dwPtr = SetFilePointer(hFile,dosHdr.e_lfanew + sizeof(IMAGE_NT_HEADERS) +sizeof(IMAGE_SECTION_HEADER) * (nIndex-1),NULL,FILE_BEGIN);
			if (dwPtr != INVALID_SET_FILE_POINTER)
			{
				if (ReadFile(hFile,&SectHdr,sizeof(IMAGE_SECTION_HEADER),&dwNumOfBytesRead,NULL))
				{
					CloseHandle(hFile);
					SetDlgItemInt(hDlg,IDC_STATIC_CURRENTSECTION,nIndex,FALSE);
					SetDlgItemInt(hDlg,IDC_STATIC_TOTALSECTIONS,ntHdr.FileHeader.NumberOfSections,FALSE);
					SetEditText(hDlg,IDC_EDIT_PHYADDR,SectHdr.Misc.PhysicalAddress);
					SetEditText(hDlg,IDC_EDIT_VIRTUALADDR,SectHdr.VirtualAddress);
					SetEditText(hDlg,IDC_EDIT_SIZEOFRAWDATA,SectHdr.SizeOfRawData);
					SetEditText(hDlg,IDC_EDIT_POINTERTORAWDATA,SectHdr.PointerToRawData);
					SetEditText(hDlg,IDC_EDIT_POINTERTOSYMBOLTABLE,SectHdr.PointerToRelocations);
					SetEditText(hDlg,IDC_EDIT_PTTORELOC,SectHdr.PointerToRelocations);
					SetEditText(hDlg,IDC_EDIT_PTTOLINENUM,SectHdr.PointerToLinenumbers);
					SetEditText(hDlg,IDC_EDIT_NUMOFRELOC,SectHdr.NumberOfRelocations);
					SetEditText(hDlg,IDC_EDIT_NUMOFLINENUM,SectHdr.NumberOfLinenumbers);
					SetEditText(hDlg,IDC_EDIT_SECT_CHARACTERICIS,SectHdr.Characteristics);
					//MultiByteToWideChar(CP_UTF8,MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,(LPCSTR)SectHdr.Name,-1,szSectName,8);
					SetDlgItemTextA(hDlg,IDC_EDIT_NAME,(LPCSTR)SectHdr.Name);
				}
			}
			else
			{
				CloseHandle(hFile);
				switch (cDirection)
				{
				case DIRECTION_DOWNCHANGED:
					nIndex ++;
					break;
				case DIRECTION_NOTCHANGED:
					break;
				case DIRECTION_UPCHANGED:
					nIndex --;
					break;
				}
				ShowError();
			}
		}
		else
		{	
			switch (cDirection)
			{
			case DIRECTION_DOWNCHANGED:
				nIndex ++;
				break;
			case DIRECTION_NOTCHANGED:
				break;
			case DIRECTION_UPCHANGED:
				nIndex --;
				break;
			}
			ShowError();
		}
}
		
