#define WIN32_LEAN_AND_MAEN
#include "ShowError.h"

extern TCHAR *szPathList;						//Array to store the module paths
extern size_t nCount;							//Additional module count

BOOL g_bColorChg = FALSE;			//flag to determine whether to change the color

typedef LONG NTSTATUS;
typedef DWORD (__stdcall *RTLNTSTATUSTODOSERROR) (NTSTATUS);
RTLNTSTATUSTODOSERROR		RtlNtStatusToDosError;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	//Make sure there's only one instance running
	//The window class name for dialog boxes is "#32770".
	HWND hWnd = FindWindow(TEXT("#32770"),g_szAppName);
	if (IsWindow(hWnd))
	{
		//An instance is already running,activate it		
		SendMessage(hWnd,ESM_POKECODEANDLOOKUP,_ttoi(lpCmdLine),0);			
	}
	else
	{
		//No instance is running
		DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DIALOG_SHOWERROR),NULL,Dlg_Proc,NULL);
	}
	return 0;
}


BOOL Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, WPARAM wParam)
{
	//Set the dialog icon
	chSETDLGICONS(hwnd,IDI_ERRORSHOW);

	HMODULE hDll = LoadLibrary(TEXT("ntdll.dll"));
	if( !hDll )
	{
		MessageBox(hwnd,TEXT("Failed to load ntdll.dll"),TEXT("Error"),MB_OK|MB_ICONERROR);
		ExitProcess(-1);
	}
	RtlNtStatusToDosError = (RTLNTSTATUSTODOSERROR)GetProcAddress(hDll,"RtlNtStatusToDosError");

	//Limit the the text length in the editbox to 5 digits
	Edit_LimitText(GetDlgItem(hwnd,IDC_EDIT_ERRCODE),10);

	// Initiate the combo box
	SendMessage(GetDlgItem(hwnd,IDC_CBL_CODE_TYPE),CB_ADDSTRING,0,(LPARAM)TEXT("NTSTATUS Code"));
	SendMessage(GetDlgItem(hwnd,IDC_CBL_CODE_TYPE),CB_ADDSTRING,0,(LPARAM)TEXT("DosError Code"));
	SendMessage(GetDlgItem(hwnd,IDC_CBL_CODE_TYPE),CB_SETCURSEL,1,0);

	//Look up the command-line passed error number
	SendMessage(hwnd,ESM_POKECODEANDLOOKUP,wParam,0);

	return TRUE;
}

void Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	DWORD dwErrorCode = 0;		//Store the error code
	HLOCAL pBuffer = NULL;		//Buffer that gets the error info
	BOOL fOK = FALSE;			//Flag indicates whether it is a network-related error
	HMODULE hDll = NULL;		//Handle to load the netmsg.dll
	TCHAR szErrorMsg[] = TEXT("Error code not found!");			//Error message
	const DWORD dwMaxErrorTextLen = sizeof(int) * 2 + 3;
	TCHAR szErrorText[dwMaxErrorTextLen];

	//The EDITBALLOONTIP structure contains information about a balloon tip 
	EDITBALLOONTIP Edit_Tip;	
	Edit_Tip.cbStruct = sizeof(EDITBALLOONTIP);
	Edit_Tip.pszText = TEXT("You can only enter at most 10 digits.");
	Edit_Tip.pszTitle = TEXT("Exceed Limitation");
	Edit_Tip.ttiIcon = TTI_ERROR;

	switch(id)
	{
	case IDC_BUTTON_CANCEL:
		//free the memory used to store the module list
		delete[] szPathList;

		//Exit
		EndDialog(hwnd,id);
		break;

	case IDC_BUTTON_MODULE:
		//Hide the main dialog
		//ShowWindow(hwnd,SW_HIDE);
		//Show the module dialog
		DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIALOG_MODULE),hwnd,Mod_Proc,NULL);

		
		break;

	case IDC_BUTTON_LOOKUP:
		//Get the error code
		GetDlgItemText(hwnd,IDC_EDIT_ERRCODE,szErrorText,dwMaxErrorTextLen);
		dwErrorCode = _tcstoul(szErrorText,0,0);

		if( SendMessage(GetDlgItem(hwnd,IDC_CBL_CODE_TYPE),CB_GETCURSEL,0,0) == 0 )
			dwErrorCode = RtlNtStatusToDosError(dwErrorCode);
	
		// Get the error code's textual description
		fOK = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,dwErrorCode,MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),(PTSTR)&pBuffer,0,NULL);

		//Is it a dll-related error?
		for (size_t i = 0;(!fOK) && (i != nCount); ++i)
		{
			hDll = LoadLibraryEx(&szPathList[i * MAX_PATH],NULL,DONT_RESOLVE_DLL_REFERENCES);
			if (hDll != NULL)
			{
				//Format the message from netmsg.dll
				fOK = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
								    hDll,dwErrorCode,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(PTSTR) &pBuffer,0,NULL);
				//Free the library
				FreeLibrary(hDll);
			}
		}

		if (pBuffer != NULL)
		{
			//Message formated successfully
			//Show the message
			g_bColorChg = FALSE;		//we needn't to change the color
			SetDlgItemText(hwnd,IDC_EDIT_ERRINFO,(PCTSTR) LocalLock(pBuffer));			//Lock the buffer
			//Free the buffer
			LocalFree(pBuffer);
		}
		else
		{
			//Oops, we cannot find the error code
			g_bColorChg = TRUE;			//we need to change the text color
			MessageBeep(MB_ICONASTERISK);
			SetDlgItemText(hwnd,IDC_EDIT_ERRINFO,szErrorMsg);
		}

		break;

	case IDC_CHECK_ONTOP:
		//Place the window topmost or not
		//SWP_NOMOVE ---- Retains the current size (ignores the x and y parameters).
		//SWP_NOSIZE ---- Retains the current size (ignores the cx and cy parameters).
		SetWindowPos(hwnd,IsDlgButtonChecked(hwnd,IDC_CHECK_ONTOP) ? HWND_TOPMOST : HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
		break;
		
	case IDC_EDIT_ERRCODE:
		switch (codeNotify)
		{
			case EN_MAXTEXT:
				//The user's inputs has exceeded the limitation, show a balloon tip
				Edit_ShowBalloonTip(hwndCtl,&Edit_Tip);
				break;
			case EN_CHANGE:
				//If the error code has been modified, clear the error text
				SetDlgItemText(hwnd,IDC_EDIT_ERRINFO,TEXT(""));
				break;
		}
		
		//See if there is no error code in the editbox,if so,disable the look up button
		EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_LOOKUP),Edit_GetTextLength(hwndCtl) > 0);
		break;

	case IDC_CBL_CODE_TYPE:
		switch(codeNotify)
		{
		case CBN_SELCHANGE:
			FORWARD_WM_COMMAND(hwnd,IDC_BUTTON_LOOKUP,GetDlgItem(hwnd,IDC_BUTTON_LOOKUP),BN_CLICKED,PostMessage);
			break;
		}
		break;
	}

	return;
}

INT_PTR WINAPI Dlg_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		//chHANDLE_DLGMSG is a macro defined in ShowError.h

		//Initiate the dialog
		chHANDLE_DLGMSG(hwnd,WM_INITDIALOG,Dlg_OnInitDialog);
		break;
		//Deal with the commands
		chHANDLE_DLGMSG(hwnd,WM_COMMAND,Dlg_OnCommand);
		break;

	case WM_CLOSE:
		//free the memory used to store the module list		
		delete[] szPathList;

		EndDialog(hwnd,0);
		break;

	case ESM_POKECODEANDLOOKUP:
		//Set the error code if it is passed through a SendMessage() function
		SetDlgItemInt(hwnd,IDC_EDIT_ERRCODE,(UINT) wParam,FALSE);
		
		//Post the message to be handled
		FORWARD_WM_COMMAND(hwnd,IDC_BUTTON_LOOKUP,GetDlgItem(hwnd,IDC_BUTTON_LOOKUP),BN_CLICKED,PostMessage);

		//Bring the window to the forground
		SetForegroundWindow(hwnd);
		break;

	case WM_CTLCOLORSTATIC:
		if (GetDlgItem(hwnd,IDC_EDIT_ERRINFO) == (HWND)lParam)
		{
			if (g_bColorChg)
			{		
				SetTextColor((HDC)wParam,RGB(255,0,0));						 //set the text color to red			
				SetBkColor((HDC)wParam,GetSysColor(COLOR_WINDOW));			 //use system default

				//the return value is a handle to a brush that the system uses to paint the background of the static control.
				return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);				
			}
			else
			{
				//use system default
				SetTextColor((HDC)wParam,(COLORREF) GetSysColor(COLOR_WINDOWTEXT));
				SetBkColor((HDC)wParam,(COLORREF)GetSysColor(COLOR_WINDOW));	

				//the return value is a handle to a brush that the system uses to paint the background of the static control.
				return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
			}
		}

		break;

	}

	return FALSE;
}