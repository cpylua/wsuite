#include "ShowError.h"

static	OPENFILENAME	ofn ;	
static TCHAR	szFileName[MAX_PATH];				//store the full path
static TCHAR	szFileTitle[MAX_PATH];				//store the dll name only

TCHAR *szPathList = NULL;							//Array to store the module paths
size_t nCount;										//Additional module count

static void FileOpenInitialize (HWND hwnd);			//initiate the structure
static BOOL FileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName);


INT_PTR WINAPI Mod_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		//chHANDLE_DLGMSG is a macro defined in ShowError.h

		//Initiate the dialog
		chHANDLE_DLGMSG(hwnd,WM_INITDIALOG,Mod_OnInitDialog);	

		//Deal with the commands
		chHANDLE_DLGMSG(hwnd,WM_COMMAND,Mod_OnCommand);
		
	case WM_CLOSE:
		EndDialog(hwnd,0);
		ShowWindow(FindWindow(TEXT("#32770"),g_szAppName),SW_SHOW);
		break;

	case WM_VKEYTOITEM:
		switch(wParam)
		{
			case VK_UP:
			case VK_LEFT:
				if (ListBox_GetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES)) >= 1)
					ListBox_SetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES),ListBox_GetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES)) - 1);

				break;

			case VK_DOWN:
			case VK_RIGHT:
				//something may be wrong here,I don't know what's happening when it comes to the first element
				//so i need to do separate oprations here to make it work properly
				if (ListBox_GetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES)) > 0)
					ListBox_SetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES),ListBox_GetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES)) + 1);
				else
					ListBox_SetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES),0);

				break;
		}
	
		return -1;
		break;

	}

	return FALSE;
}

int Mod_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	TCHAR szModulePath[MAX_PATH];		//Used for add a string to the listbox
	static BOOL bIsTyping = TRUE;				//Define whether the user is typing in the editbox or not

	switch (id)
	{
	case IDC_BUTTON_ADD:
		//See if the number is larger than 64
		//if(ListBox_GetCount(GetDlgItem(hwnd,IDC_LIST_MODULES)) < MAX_COUNT)
		//{
			//Add the path or name to the listbox
			GetDlgItemText(hwnd,IDC_EDIT_PATH,szModulePath,MAX_PATH);
			ListBox_AddString(GetDlgItem(hwnd,IDC_LIST_MODULES),szModulePath);
			
			//Disable the Add button
			//EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_ADD),FALSE);

			//Clear the text in the edit box
			Edit_SetText(GetDlgItem(hwnd,IDC_EDIT_PATH),TEXT(""));

			SetFocus(GetDlgItem(hwnd,IDC_EDIT_PATH));
		//}
		//else
		//	MessageBox(hwnd,TEXT("Too many modules!"),TEXT("Error"),MB_OK | MB_ICONERROR);
		return codeNotify;
		break;

	case IDC_BUTTON_REMOVE:
		
		ListBox_SetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES),ListBox_GetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES)) - 1);
		ListBox_DeleteString(GetDlgItem(hwnd,IDC_LIST_MODULES),ListBox_GetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES)) + 1);

		//Check whether it's the first element in the listbox
		//if so,set current self to 0
		if (ListBox_GetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES)) < 0)
			ListBox_SetCurSel(GetDlgItem(hwnd,IDC_LIST_MODULES),0);

		//Set focus to the listbox
		SetFocus(GetDlgItem(hwnd,IDC_LIST_MODULES));

		//Disable the Remove button if the listbox has no elements
		//and srt focus to the edit box,so the user can enter their modules 
		if ( !ListBox_GetCount(GetDlgItem(hwnd,IDC_LIST_MODULES)) )
		{
				EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_REMOVE),FALSE);
				SetFocus(GetDlgItem(hwnd,IDC_EDIT_PATH));
		}

		return codeNotify;
		break;

	case IDCANCEL:
		EndDialog(hwnd,id);
		ShowWindow(FindWindow(TEXT("#32770"),g_szAppName),SW_SHOW);

		return codeNotify;
		break;

	case IDOK:
		//Store the items 
		if (nCount = ListBox_GetCount(GetDlgItem(hwnd,IDC_LIST_MODULES)))
		{
			delete[] szPathList;
			szPathList = new TCHAR[nCount * MAX_PATH];
			for (size_t i = 0;i != nCount; ++i)
				ListBox_GetText(GetDlgItem(hwnd,IDC_LIST_MODULES),i,&szPathList[i * MAX_PATH]);
		}

		EndDialog(hwnd,0);
		ShowWindow(FindWindow(TEXT("#32770"),g_szAppName),SW_SHOW);

		return codeNotify;
		break;

	case IDC_BUTTON_BROWSE:
		bIsTyping = FALSE;			//the user is not typing in the editbox

		//Show the Open File diaglog
		if ( FileOpenDlg(hwnd,szFileName,szFileTitle) )
			SetDlgItemText(hwnd,IDC_EDIT_PATH,szFileName);
		
		return codeNotify;
		break;

	case IDC_EDIT_PATH:
		switch (codeNotify)
		{
		case EN_CHANGE:
			EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_ADD),Edit_GetTextLength(GetDlgItem(hwnd,IDC_EDIT_PATH)) ? TRUE : FALSE);				
			
			break;
		}

		return codeNotify;
		break;

	case IDC_LIST_MODULES:
		switch (codeNotify)
		{
		case LBN_SELCHANGE:
				EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_REMOVE),TRUE);
				//SetFocus(GetDlgItem(hwnd,IDC_EDIT_PATH));

			break;

		case LBN_SELCANCEL:
			EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_REMOVE),FALSE);
			
			break;

		case LBN_KILLFOCUS:
			//ListBox_SetCurSel(hwndCtl,-1);
			//EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_REMOVE),FALSE);
			break;
		}
		
		return codeNotify;
		break;

	}

	return -1;
}

BOOL Mod_OnInitDialog(HWND hwnd, HWND hwndFocus, WPARAM wParam)
{
	//Disable the add button
	EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_ADD),FALSE);

	//Disable the remove button if there's no item in the path list
	if ( !ListBox_GetCount(GetDlgItem(hwnd,IDC_LIST_MODULES)))
		EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_REMOVE),FALSE);

	//Initiate the structure
	FileOpenInitialize(hwnd);

	//Set the listbox's data
	for (size_t i = 0;i != nCount;++i)
		ListBox_AddString(GetDlgItem(hwnd,IDC_LIST_MODULES),&szPathList[i * MAX_PATH]);

	return TRUE;
}

static void FileOpenInitialize (HWND hwnd)
{
	static WCHAR szFilter[] = TEXT ("Dynamic Link Libraries (*.dll)\0*.dll\0All Files (*.*)\0*.*\0\0");
	
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

static BOOL FileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
	ofn.hwndOwner         = hwnd ;
	ofn.lpstrFile         = pstrFileName ;
	ofn.lpstrFileTitle    = pstrTitleName ;
	ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;
	
	return GetOpenFileName (&ofn) ;
}

