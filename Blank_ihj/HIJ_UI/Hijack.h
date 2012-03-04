#pragma once

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

#define HIJACK_CREATE	1
#define HIJACK_DELETE	2
#define HIJACK_DEL_BLANK	3
#define HIJACK_ADD_BLANK	4

const int LIST_WHIDTH = 383;
const int FIRST_COL_WIDTH = 100;
const int MAX_FILE_TITLE = MAX_KEY_LENGTH;
const int MAX_FILE_PATH = 1024;
const int WINDOW_WIDTH = 272;
const int WINDOW_HEIGHT = 188;
BOOL bRequireElevate = FALSE;
SHSTOCKICONINFO Staticssi = { sizeof(SHSTOCKICONINFO) };


struct HijackInfo
{
	DWORD cEntry;
	//DWORD cbFileMappingSize;
	DWORD dwOption;
};

enum HijackReturnCode
{
	HijackFailed,
	HijackCreateOk,	
	HijackDeleteOk
};

LPTSTR* plpHijackPath = 0;
LPTSTR* plpImage = 0;
DWORD dwEntrys = 0;

//-------------------------------------------------------------------------------
//registry operations
DWORD GetHijackNames(IN OUT LPTSTR *plpImageName,IN OUT LPTSTR *plpHijackName);

BOOL DeleteHijackEntry(IN LPCTSTR lpHijackName);

BOOL CreateHijackEntry(IN LPCTSTR lpHijackName);

VOID FreeHijackBuffer(IN LPTSTR* &plpBuffer1,IN LPTSTR* &plpBuffer2,IN DWORD cEntrys);

//----------------------------------------------------------------------------
//dialog functions
INT_PTR WINAPI Dlg_Proc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

VOID OnCommand(HWND hWnd,int id,HWND hWndCtrl,UINT nCodeNotify);

VOID OnClose(HWND hWnd);

BOOL OnInitDlg(HWND hWnd,HWND hWndFocus,WPARAM wParam);

DWORD RefreshListView(HWND hWndListView);

BOOL InitListView(HWND hWndListView);

VOID OnBrowse(HWND hWnd);

VOID OnAdd(HWND hWnd);

VOID OnDelete(HWND hWnd);

VOID OnEditBoxMsg(HWND hWnd,HWND hWndEditBox,UINT nCodeNotify);

VOID OnDragFiles(HWND hWnd,WPARAM wParam);

DWORD PopUpMenu(HWND hWnd);

VOID UpdateSysLinkState(HWND hWndLink);

void GetSetPositionInfoFromRegistry( BOOL fSave, POINT *lppt );

HANDLE SpawnWorker();

BOOL DoJobHijack(DWORD cEntry, PCTSTR* pszEntryNames, DWORD dwOption);

void FormatError(LPTSTR lpszFunction) ;

PTSTR GetWorkerFileName();

//BOOL ExtractBlank(IN LPCTSTR lpResType,IN LPCTSTR lpResName);

//-------------------------------------------------------------------------
//
BOOL ExtractRes(IN LPCTSTR lpResType,IN LPCTSTR lpResName, PCTSTR pszTargeTPath);

BOOL GetProcessElevation(TOKEN_ELEVATION_TYPE* pElevationType, BOOL* pIsAdmin) ;
//--------------------------------------------------------------------------------


//----------------------------------------------------------------------
//useful macros

// Sets the dialog box icons
inline void SETDLGICONS(HWND hwnd, int idi) {
   SendMessage(hwnd, WM_SETICON, TRUE,  (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
         MAKEINTRESOURCE(idi)));
   SendMessage(hwnd, WM_SETICON, FALSE, (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
      MAKEINTRESOURCE(idi)));
}

// The normal HANDLE_MSG macro in WindowsX.h does not work properly for dialog
// boxes because DlgProc return a BOOL instead of an LRESULT (like
// WndProcs). This chHANDLE_DLGMSG macro corrects the problem:
#define HANDLE_DLGMSG(hwnd, message, fn)                 \
   case (message): return (SetDlgMsgResult(hwnd, uMsg,     \
      HANDLE_##message((hwnd), (wParam), (lParam), (fn))))