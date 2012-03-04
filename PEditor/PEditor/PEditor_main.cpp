#include "resource.h"
#include <windows.h>

//normal functions
LRESULT CALLBACK MainDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void	FileOpenInitialize (HWND);
BOOL FileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName);
BOOL IsPEFile(TCHAR* lpFileName);
void EnableButtons(HWND hDlg,BOOL bEnable);
void CheckFuck(HWND hDlg,int nIDDlgItem);

//extern functions
BOOL SetEditText(HWND hWnd,int nDlgItemID,DWORD dwNumToSet);
LRESULT CALLBACK FileHdrDlg(HWND hFileHdrDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OptHdrDlg(HWND hOptHdrDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SectHdrDlg(HWND hSectHdrDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ShowError();




static	OPENFILENAME	ofn ;
TCHAR	szFileName[1024];
TCHAR	szFileTitle[512];
bool	bEnableBtn;
IMAGE_DOS_HEADER	dosHdr;
IMAGE_NT_HEADERS	ntHdr;
HINSTANCE hAppHwnd;


int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{

		hAppHwnd=GetModuleHandle(NULL);
		
		DialogBoxParam(hAppHwnd,(LPCTSTR)IDD_DIALOG_MAIN,NULL,(DLGPROC)MainDlg,NULL);
	
		return 1;
}

LRESULT CALLBACK MainDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HICON hIcon;
	switch (message)
	{
	case WM_INITDIALOG:
		bEnableBtn = false;					//set the flag to false in case the user enables the button

		FileOpenInitialize(hDlg);			//Initialize the OPENFILENAME structure

		//load icon
		hIcon = LoadIcon(hAppHwnd,(LPCTSTR)IDI_ICON_PE);
		SendMessage(hDlg,WM_SETICON,ICON_BIG,(LPARAM)hIcon);

		EnableButtons(hDlg,FALSE);			//disable the buttons

		break;
	case WM_CLOSE:
		if (!EndDialog(hDlg,0))
		{
			ShowError();
			ExitProcess(0);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_OPEN:
			bEnableBtn = false;
			if (FileOpenDlg(hDlg,szFileName,szFileTitle))
			{
				EnableButtons(hDlg,FALSE);
				SetDlgItemText(hDlg,IDC_STATIC_WARNING,L"");
				SetDlgItemText(hDlg,IDC_EDIT_FILEPATH,szFileName);
				if(!IsPEFile(szFileName))
				{
					SetDlgItemText(hDlg,IDC_STATIC_WARNING,L"It's an invalide PE file or PEditor can't access the file now!");
				}
				else
				{
					EnableButtons(hDlg,TRUE);
					bEnableBtn = true;
				}
				
			}
			break;
		case IDC_BUTTON_FILEHEADER:
			if (!bEnableBtn)
			{
				CheckFuck(hDlg,IDC_BUTTON_FILEHEADER);
			}
			else
			{
				IsPEFile(szFileName);
				ShowWindow(hDlg,SW_HIDE);
				DialogBoxParam(hAppHwnd,(LPCTSTR)IDD_DIALOG_FILEHEADER,NULL,(DLGPROC)FileHdrDlg,NULL);
				ShowWindow(hDlg,SW_RESTORE);
			}
			break;
		case IDC_BUTTON_OPTIONAL:
			if (!bEnableBtn)
			{
				CheckFuck(hDlg,IDC_BUTTON_OPTIONAL);
			}
			else
			{
				IsPEFile(szFileName);
				ShowWindow(hDlg,SW_HIDE);
				DialogBoxParam(hAppHwnd,(LPCTSTR)IDD_DIALOG_OPTIONALHDR,NULL,(DLGPROC)OptHdrDlg,NULL);
				ShowWindow(hDlg,SW_RESTORE);
			}
			break;
		case IDC_BUTTON_SECTION:
			if (!bEnableBtn)
			{
				CheckFuck(hDlg,IDC_BUTTON_SECTION);
			}
			else
			{
				IsPEFile(szFileName);
				ShowWindow(hDlg,SW_HIDE);
				DialogBoxParam(hAppHwnd,(LPCTSTR)IDD_DIALOG_SECTION,NULL,(DLGPROC)SectHdrDlg,NULL);
				ShowWindow(hDlg,SW_RESTORE);
			}			
			break;
		}
		break;
	}
	return 0;
}

void FileOpenInitialize (HWND hwnd)
{
	static WCHAR szFilter[] = TEXT ("Executable Files (*.exe;*.dll;*.ocx;*.cpl)\0*.exe;*.dll;*.ocx;*.cpl\0All Files (*.*)\0*.*\0\0");
	
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = hwnd ;
	ofn.hInstance         = NULL ;
	ofn.lpstrFilter       = szFilter ;
	ofn.lpstrCustomFilter = NULL ;
	ofn.nMaxCustFilter    = 0 ;
	ofn.nFilterIndex      = 0 ;
	ofn.lpstrFile         = NULL ;
	ofn.nMaxFile          = MAX_PATH ;
	ofn.lpstrFileTitle    = NULL ;
	ofn.nMaxFileTitle     = MAX_PATH ;
	ofn.lpstrInitialDir   = NULL ;
	ofn.lpstrTitle        = NULL ;
	ofn.Flags             = 0 ;
	ofn.nFileOffset       = 0 ;
	ofn.nFileExtension    = 0 ;
	ofn.lpstrDefExt       = NULL ;
	ofn.lCustData         = 0L ;
	ofn.lpfnHook          = NULL ;
	ofn.lpTemplateName    = NULL ;
}

BOOL FileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
	ofn.hwndOwner         = hwnd ;
	ofn.lpstrFile         = pstrFileName ;
	ofn.lpstrFileTitle    = pstrTitleName ;
	ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;
	
	return GetOpenFileName (&ofn) ;
}



void EnableButtons(HWND hDlg,BOOL bEnable)
{
	HWND hWnd;
	hWnd = GetDlgItem(hDlg,IDC_BUTTON_FILEHEADER);
	EnableWindow(hWnd,bEnable);
	CloseHandle(hWnd);
	hWnd = GetDlgItem(hDlg,IDC_BUTTON_OPTIONAL);
	EnableWindow(hWnd,bEnable);
	CloseHandle(hWnd);
	hWnd = GetDlgItem(hDlg,IDC_BUTTON_SECTION);
	EnableWindow(hWnd,bEnable);
	CloseHandle(hWnd);
}

void CheckFuck(HWND hDlg,int nIDDlgItem)
{
		MessageBox(NULL,L"You can't use this function while the file is invalid!",L"It's your fault",MB_ICONERROR|MB_OK);
		HWND hWnd = GetDlgItem(hDlg,nIDDlgItem);
		EnableWindow(hWnd,FALSE);
		SetFocus(GetDlgItem(hDlg,IDC_BUTTON_OPEN));
		CloseHandle(hWnd);
}

	