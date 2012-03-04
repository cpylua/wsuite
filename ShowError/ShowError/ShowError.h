#ifndef SHOW_ERROR_H
#define SHOW_ERROR_H

#define _WIN32_WINNT 0x501

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>

#include "resource.h"

//#include <vld.h>

#endif

#define ESM_POKECODEANDLOOKUP		(WM_USER + 100)					//MessageID for handle the look up command
const TCHAR g_szAppName[] =			TEXT("Show Error");				//App name

//Main module dialog procedure
INT_PTR WINAPI Mod_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//Function for WM_COMMAND
int Mod_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

//Function for WM_INITDIALOG
BOOL Mod_OnInitDialog(HWND hwnd, HWND hwndFocus, WPARAM wParam);



//Main dialog procedure
INT_PTR WINAPI Dlg_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//Function for WM_COMMAND
void Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

//Function for WM_INITDIALOG
BOOL Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, WPARAM wParam);

// convertion from small type to lager tyte
#pragma warning(disable:4312)

//“参数”: 从“LPARAM”转换到“LONG”，可能丢失数据
#pragma warning(disable:4244)

/////////////////////////// chHANDLE_DLGMSG Macro /////////////////////////////


// The normal HANDLE_MSG macro in WindowsX.h does not work properly for dialog
// boxes because DlgProc return a BOOL instead of an LRESULT (like
// WndProcs). This chHANDLE_DLGMSG macro corrects the problem:
#define chHANDLE_DLGMSG(hwnd, message, fn)                 \
   case (message): return (SetDlgMsgResult(hwnd, uMsg,     \
      HANDLE_##message((hwnd), (wParam), (lParam), (fn))))


//////////////////////// Dialog Box Icon Setting Macro ////////////////////////


// Sets the dialog box icons
inline void chSETDLGICONS(HWND hwnd, int idi) {
   SendMessage(hwnd, WM_SETICON, TRUE,  (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
         MAKEINTRESOURCE(idi)));
   SendMessage(hwnd, WM_SETICON, FALSE, (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
      MAKEINTRESOURCE(idi)));
}

// unreferenced formal parameter
#pragma warning(disable:4100)


