#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <strsafe.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <uxtheme.h>

#include "resource.h"
#include "hijack.h"
#include <VSStyle.h>
#include <vssym32.h>

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib, "uxtheme.lib")

#define LIST_CTRL GetDlgItem(hWnd,LIST_IHJS)

using namespace Gdiplus;

int WINAPI _tWinMain(HINSTANCE hInstance,HINSTANCE,LPTSTR /*lpCmdLine*/,int /*nCmdShow*/)
{	
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

   	CoInitialize(NULL);
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	DialogBoxParam(hInstance,MAKEINTRESOURCE(DLG_UI),NULL,Dlg_Proc,0);

	GdiplusShutdown(gdiplusToken);
	CoUninitialize();

	return 0;
}

INT_PTR WINAPI Dlg_Proc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LPNMLVKEYDOWN lpnml_kd;
	LPNMHDR lpnmh;
	BOOL bFlag = FALSE;
	WCHAR szUrl[] = L"http://whislter.spaces.live.com";

	switch(uMsg)
	{
		HANDLE_DLGMSG(hWnd,WM_CLOSE,OnClose);
		break;
		
		HANDLE_DLGMSG(hWnd,WM_COMMAND,OnCommand);
		break;

		HANDLE_DLGMSG(hWnd,WM_INITDIALOG,OnInitDlg);
		break;

	// Handle dropped filenames
	case WM_DROPFILES:
		OnDragFiles(hWnd,wParam);
		break;

	case WM_NOTIFY:
		lpnmh = (LPNMHDR)lParam;
		switch(lpnmh->idFrom)
		{
		case LIST_IHJS:
			switch(lpnmh->code)
			{
			case LVN_KEYDOWN:
				lpnml_kd = (LPNMLVKEYDOWN)lParam;
				if(lpnml_kd->wVKey == VK_DELETE)
					OnDelete(hWnd);
				//SetFocus(GetDlgItem(hWnd,BTN_BROWSE));
				break;

			case NM_RCLICK:
			case NM_RDBLCLK:
				PopUpMenu(hWnd);
				break;

			}
			break;

		case SYSLINK_DELETE:
			switch(lpnmh->code)
			{
			case NM_CLICK:
				bFlag = DoJobHijack(0,NULL,HIJACK_DEL_BLANK);
				UpdateSysLinkState(GetDlgItem(hWnd,SYSLINK_DELETE));
				break;
			}
			break;

		case SYSLINK_ABOUT:
			switch(lpnmh->code)
			{
			case NM_CLICK:
				ShellExecute(hWnd,TEXT("open"),szUrl,NULL,NULL,SW_SHOW);

				break;
			}
			break;

		case SYSLINK_RESTORE:
			switch(lpnmh->code)
			{
			case NM_CLICK:
				DoJobHijack(0,NULL,HIJACK_ADD_BLANK);
				UpdateSysLinkState(GetDlgItem(hWnd,SYSLINK_DELETE));
				break;
			}
			break;	
		}
		break;
	}

	return 0;
}

VOID OnClose(HWND hWnd)
{
	// Stop accepting dropped files
	DragAcceptFiles(hWnd,FALSE);

	// Save off the window's x,y coordinates for next time
	RECT rect;
	GetWindowRect(hWnd,&rect);
	GetSetPositionInfoFromRegistry(TRUE,(LPPOINT)&rect);

	FreeHijackBuffer(plpHijackPath,plpImage,dwEntrys);
	EndDialog(hWnd,0);

	DestroyIcon(Staticssi.hIcon);
}

BOOL OnInitDlg(HWND hWnd,HWND /*hWndFocus*/,WPARAM /*wParam*/)
{
	TOKEN_ELEVATION_TYPE ElevateType = TokenElevationTypeDefault;
	BOOL bIsAdmin = FALSE;
	POINT pt;
	TCHAR szAdminSuffix[] = TEXT("   (Administrator)");
	TCHAR szTitle[64];

	if( GetProcessElevation(&ElevateType,&bIsAdmin) )
	{
		if( bIsAdmin )
		{
			if( ElevateType != TokenElevationTypeFull )
			{
				// Set the shield icon
				Button_SetElevationRequiredState(GetDlgItem(hWnd,BTN_ADD),TRUE);

				bRequireElevate = TRUE;

				if( SHGetStockIconInfo(SIID_SHIELD,SHGSI_SMALLICON | SHGSI_ICON,&Staticssi) != S_OK )
					FormatError(TEXT("SHGetStockIcon"));

				Static_SetIcon(GetDlgItem(hWnd,IDC_STATIC_SHIELD),Staticssi.hIcon);
			}
			else
			{
				// Modify the window title
				GetWindowText(hWnd,szTitle,_countof(szTitle));
				StringCchCat(szTitle,_countof(szTitle),szAdminSuffix);
				SetWindowText(hWnd,szTitle);
				
				// Hide the icon
				ShowWindow(GetDlgItem(hWnd,IDC_STATIC_SHIELD),SW_HIDE);
			}
		}
	}

	// Get the window coordinates where Hijack It! was last running,
	// and move the window to that spot
	GetSetPositionInfoFromRegistry(FALSE,&pt);
	SetWindowPos(hWnd,NULL,pt.x,pt.y,0,0,
				SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER | SWP_NOACTIVATE);

	SETDLGICONS(hWnd,IDI_IHJ);

	// Set us up to accept dropped filesnames
	DragAcceptFiles(hWnd,TRUE);

	InitListView(GetDlgItem(hWnd,LIST_IHJS));

	Button_Enable(GetDlgItem(hWnd,BTN_ADD),FALSE);

	UpdateSysLinkState(GetDlgItem(hWnd,SYSLINK_DELETE));

	return TRUE;

}

VOID OnCommand(HWND hWnd,int id,HWND hWndCtrl,UINT nCodeNotify)
{
	switch(id)
	{
	case BTN_BROWSE:
		OnBrowse(hWnd);
		break;

	case BTN_ADD:
		OnAdd(hWnd);
		break;

	case EDIT_IMNAME:
		OnEditBoxMsg(hWnd,hWndCtrl,nCodeNotify);
		break;
		
	case IDM_DELETE:
		OnDelete(hWnd);
		break;

	case IDM_REFRESH:
		RefreshListView(GetDlgItem(hWnd,LIST_IHJS));
		break;

	case CHK_ON_TOP:
		SetWindowPos(hWnd,IsDlgButtonChecked(hWnd,CHK_ON_TOP) ? HWND_TOPMOST : HWND_NOTOPMOST,
					 0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
		break;
	}
}

VOID OnDragFiles(HWND hWnd, WPARAM wParam)
{
	LPTSTR lpszFileName;	// Pointer to a buffer
	LPTSTR lpszFilePath;	

	UINT cchFilePath;		// The filepath length 
	UINT cFiles;			// The number of files being dropped
	HDROP hDrop;
	DWORD dwBytesReturned;


	hDrop = (HDROP)wParam;
	// Get the total number of files that's being dropped
	cFiles = DragQueryFile(hDrop,(UINT)0xFFFFFFFF,NULL,0);

	TCHAR szTempPath[MAX_PATH],szTempFileName[MAX_PATH];

	GetTempPath(MAX_PATH,szTempPath);
	GetTempFileName(szTempPath,TEXT("WHH"),0,szTempFileName);

	HANDLE hFile = CreateFile(szTempFileName,FILE_ALL_ACCESS,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hFile == INVALID_HANDLE_VALUE )
	{
		FormatError(TEXT("CreateFile"));
		return ;
	}

	// Write header
	HijackInfo hi;
	hi.cEntry = cFiles;
	hi.dwOption = HIJACK_CREATE;
	WriteFile(hFile,&hi,sizeof(hi),&dwBytesReturned,NULL);

	// Process each dropped file
	while( cFiles > 0 )
	{
		cFiles--;
		cchFilePath = DragQueryFile(hDrop,cFiles,NULL,0);
		// Plus the NULL
		cchFilePath++;

		lpszFilePath = (LPTSTR)LocalAlloc(LPTR,cchFilePath * sizeof(TCHAR));
		DragQueryFile(hDrop,cFiles,lpszFilePath,cchFilePath);
		
		if( !PathIsDirectory(lpszFilePath) )
		{
			// Get the file name from the path
			lpszFileName = lpszFilePath + cchFilePath - 2;
			while( *lpszFileName-- != L'\\' );
			lpszFileName += 2;

			//SetDlgItemText(hWnd,EDIT_IMNAME,lpszFileName);
			//OnAdd(hWnd);
			WriteFile(hFile,lpszFileName,
					  (_tcslen(lpszFileName) + 1) * sizeof(TCHAR),
					  &dwBytesReturned,NULL);
		}
		LocalFree(lpszFilePath);
	}
	DragFinish(hDrop);

	CloseHandle(hFile);

	// Spawn the worker process
	SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	sei.lpParameters = szTempFileName;
	sei.lpVerb = TEXT("runas");		// Ask for privileges elevation.

	PTSTR pszWorkerPath = GetWorkerFileName();
	sei.lpFile = pszWorkerPath;
	if( !ShellExecuteEx(&sei) )
	{
		FormatError(TEXT("ShellExcuteEx"));
		return;
	}
	LocalFree(pszWorkerPath);
	HANDLE hWorkerProc = sei.hProcess;

	WaitForSingleObject(hWorkerProc,INFINITE);

	DWORD dwExitCode = 0;
	GetExitCodeProcess(hWorkerProc,&dwExitCode);
	CloseHandle(hWorkerProc);

	RefreshListView(LIST_CTRL);

	DeleteFile(szTempFileName);

	DWORD dw = dwExitCode - HijackCreateOk;
	TCHAR szMsgBuf[64];
	if( (dw = hi.cEntry - dw) > 0 )
	{
		StringCchPrintf(szMsgBuf,_countof(szMsgBuf),TEXT("%u entries did NOT added!"),dw);
		MessageBox(hWnd,szMsgBuf,TEXT("Hijack It!"),MB_OK | MB_ICONWARNING);
	}
}

BOOL InitListView(HWND hWndListView) 
{ 
	//add columns
    LVCOLUMN lvc; 
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = FIRST_COL_WIDTH;		//width
	lvc.iSubItem = 0;
	
	//column 0
	lvc.iOrder = 0;
	lvc.pszText = TEXT("Hijack Entry");
	ListView_InsertColumn(hWndListView, 0, &lvc);

	//column 1
	lvc.cx = LIST_WHIDTH - FIRST_COL_WIDTH;
	lvc.iOrder = 1;
	lvc.pszText = TEXT("Hijack Path");
	ListView_InsertColumn(hWndListView,1,&lvc);
	 
	//full row select and gridlines
	ListView_SetExtendedListViewStyle(hWndListView,LVS_EX_GRIDLINES); 
	ListView_SetExtendedListViewStyle(hWndListView,LVS_EX_FULLROWSELECT);

	//update list view column items
	RefreshListView(hWndListView);

    return TRUE; 
} 

DWORD RefreshListView(HWND hWndListView)
{
	//free memory
	FreeHijackBuffer(plpImage,plpHijackPath,dwEntrys);

	//get hijack informations
	dwEntrys = GetHijackNames(NULL,NULL);
	plpImage = (LPTSTR*)LocalAlloc(LPTR,dwEntrys * sizeof(LPTSTR));
	plpHijackPath = (LPTSTR*)LocalAlloc(LPTR,dwEntrys * sizeof(LPTSTR));
	GetHijackNames(plpImage,plpHijackPath);

	//delete all items
	ListView_DeleteAllItems(hWndListView);

	//add items in column 0
	LVITEM lvi = {0};
	lvi.mask = LVIF_TEXT;
	
	for(DWORD i =0; i != dwEntrys;++i)
	{
		lvi.iItem = i;
		lvi.pszText = plpImage[i];
		ListView_InsertItem(hWndListView,&lvi);
	}

	//add items in colum 1
	for(DWORD i =0; i != dwEntrys;++i)
		ListView_SetItemText(hWndListView,i,1,plpHijackPath[i]);

	return 0;
}

VOID OnBrowse(HWND hWnd)
{
	TCHAR szFilter[] = TEXT ("Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0\0");
	OPENFILENAME ofn ;

	TCHAR	szFileTitle[MAX_FILE_TITLE];
	ZeroMemory(&szFileTitle,sizeof(szFileTitle));

	TCHAR	szFilePath[MAX_FILE_PATH];
	ZeroMemory(&szFilePath,sizeof(szFilePath));

	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = hWnd ;
	ofn.lpstrFilter       = szFilter ;
	ofn.lpstrFile		  = szFilePath;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile		  = MAX_FILE_PATH;
	ofn.nMaxFileTitle     = MAX_FILE_TITLE ;
	ofn.lpstrFileTitle    = szFileTitle;
	ofn.lpstrInitialDir	  = NULL;
	ofn.Flags             = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if( GetOpenFileName (&ofn) == TRUE)
	{
		SetDlgItemText(hWnd,EDIT_IMNAME,szFileTitle);
		SetFocus(GetDlgItem(hWnd,BTN_ADD));
	}
}

VOID OnAdd(HWND hWnd)
{
	BOOL bFlag = TRUE;
	TCHAR szFileName[MAX_FILE_TITLE];
	ZeroMemory(szFileName,sizeof(szFileName));

	UpdateSysLinkState(GetDlgItem(hWnd,SYSLINK_DELETE));

	bFlag = FALSE;
	GetDlgItemText(hWnd,EDIT_IMNAME,szFileName,MAX_FILE_TITLE);

	if( _tcscmp(szFileName,TEXT("")) )
	{
		PTSTR pszFileName = szFileName;
		bFlag = DoJobHijack(1,(PCTSTR*)&pszFileName,HIJACK_CREATE);

		if(!bFlag)
			MessageBox(hWnd,TEXT("Can not create hijack entry.\n"),TEXT("Hjack It!"),MB_OK|MB_ICONERROR);
		else
			SetDlgItemText(hWnd,EDIT_IMNAME,TEXT(""));
			RefreshListView(GetDlgItem(hWnd,LIST_IHJS));

		Button_Enable(GetDlgItem(hWnd,BTN_ADD),!bFlag);
	}
	else
	{
		Button_Enable(GetDlgItem(hWnd,BTN_ADD),bFlag);
		MessageBox(hWnd,TEXT("Choose an image first !"),TEXT("Hijack It!"),MB_OK|MB_ICONINFORMATION);
	}
}


VOID OnDelete(HWND hWnd)
{
	TCHAR szHijackName[MAX_KEY_LENGTH];
	DWORD dwBytesReturned;
	TCHAR szTempPath[MAX_PATH],szTempFileName[MAX_PATH];

	GetTempPath(MAX_PATH,szTempPath);
	GetTempFileName(szTempPath,TEXT("WHH"),0,szTempFileName);

	HANDLE hFile = CreateFile(szTempFileName,FILE_ALL_ACCESS,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hFile == INVALID_HANDLE_VALUE )
	{
		FormatError(TEXT("CreateFile"));
		return ;
	}

	// Write header
	DWORD dwSelectedItems = ListView_GetSelectedCount(LIST_CTRL);
	HijackInfo hi;
	hi.cEntry = dwSelectedItems;
	hi.dwOption = HIJACK_DELETE;
	WriteFile(hFile,&hi,sizeof(hi),&dwBytesReturned,NULL);

	// Write names
	int Index = -1;
	for(int i = 0; i != (int)dwSelectedItems; ++i)
	{
		Index = ListView_GetNextItem(LIST_CTRL,-1,LVNI_SELECTED);
		if( Index == -1 )
		{
			FormatError(TEXT("ListView_GetNextItem"));
			break;
		}

		ListView_GetItemText(LIST_CTRL,Index,0,szHijackName,MAX_KEY_LENGTH);
		ListView_DeleteItem(LIST_CTRL,Index);

		WriteFile(hFile,szHijackName,
				  (_tcslen(szHijackName) + 1) * sizeof(TCHAR),
				  &dwBytesReturned,NULL);

	}
	CloseHandle(hFile);

	// Spawn the worker process
	SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	sei.lpParameters = szTempFileName;
	sei.lpVerb = TEXT("runas");		// Ask for privileges elevation.

	PTSTR pszWorkerPath = GetWorkerFileName();
	sei.lpFile = pszWorkerPath;

	if( !ShellExecuteEx(&sei) )
	{
		RefreshListView(LIST_CTRL);
		FormatError(TEXT("ShellExcuteEx"));
		return;
	}
	LocalFree(pszWorkerPath);
	HANDLE hWorkerProc = sei.hProcess;

	WaitForSingleObject(hWorkerProc,INFINITE);

	DWORD dwExitCode = 0;
	GetExitCodeProcess(hWorkerProc,&dwExitCode);
	CloseHandle(hWorkerProc);

	if( dwExitCode == HijackFailed )
		MessageBox(hWnd,TEXT("Failed to delete the selected entries."),TEXT("Hijack It!"),MB_OK | MB_ICONEXCLAMATION);

	RefreshListView(LIST_CTRL);

	DWORD dw = dwExitCode - HijackDeleteOk;
	TCHAR szMsgBuf[64];
	if( (dw = dwSelectedItems - dw) > 0 )
	{
		StringCchPrintf(szMsgBuf,_countof(szMsgBuf),TEXT("%u entries did NOT delete!"),dw);
		MessageBox(hWnd,szMsgBuf,TEXT("Hijack It!"),MB_OK | MB_ICONWARNING);
	}
	
	SetFocus(GetDlgItem(hWnd,BTN_BROWSE));

	DeleteFile(szTempFileName);
}

VOID OnEditBoxMsg(HWND hWnd,HWND hWndEditBox,UINT nCodeNotify)
{
	TCHAR szImageName[MAX_FILE_TITLE] = {0};
	BOOL bFlag = FALSE;

	switch(nCodeNotify)
	{
	case EN_CHANGE:
		Edit_GetText(hWndEditBox,szImageName,MAX_FILE_TITLE);
		bFlag = _tcscmp(szImageName,TEXT(""));
		Button_Enable(GetDlgItem(hWnd,BTN_ADD),bFlag);
	}

}

HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp)
{
	*phBmp = NULL;

	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	bmi.bmiHeader.biWidth = psize->cx;
	bmi.bmiHeader.biHeight = psize->cy;
	bmi.bmiHeader.biBitCount = 32;

	HDC hdcUsed = hdc ? hdc : GetDC(NULL);
	if (hdcUsed)
	{
		*phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
		if (hdc != hdcUsed)
		{
			ReleaseDC(NULL, hdcUsed);
		}
	}
	return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

bool HasAlpha(__in ARGB *pargb, SIZE& sizImage, int cxRow)
{
	ULONG cxDelta = cxRow - sizImage.cx;
	for (ULONG y = sizImage.cy; y; --y)
	{
		for (ULONG x = sizImage.cx; x; --x)
		{
			if (*pargb++ & 0xFF000000)
			{
				return true;
			}
		}

		pargb += cxDelta;
	}

	return false;
}

HRESULT ConvertToPARGB32(HDC hdc, __inout ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow)
{
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	bmi.bmiHeader.biWidth = sizImage.cx;
	bmi.bmiHeader.biHeight = sizImage.cy;
	bmi.bmiHeader.biBitCount = 32;

	HRESULT hr = E_OUTOFMEMORY;
	HANDLE hHeap = GetProcessHeap();
	void *pvBits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
	if (pvBits)
	{
		hr = E_UNEXPECTED;
		if (GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, pvBits, &bmi, DIB_RGB_COLORS) == bmi.bmiHeader.biHeight)
		{
			ULONG cxDelta = cxRow - bmi.bmiHeader.biWidth;
			ARGB *pargbMask = static_cast<ARGB *>(pvBits);

			for (ULONG y = bmi.bmiHeader.biHeight; y; --y)
			{
				for (ULONG x = bmi.bmiHeader.biWidth; x; --x)
				{
					if (*pargbMask++)
					{
						// transparent pixel
						*pargb++ = 0;
					}
					else
					{
						// opaque pixel
						*pargb++ |= 0xFF000000;
					}
				}

				pargb += cxDelta;
			}

			hr = S_OK;
		}

		HeapFree(hHeap, 0, pvBits);
	}

	return hr;
}

HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon)
{
	RGBQUAD *prgbQuad;
	int cxRow;
	HRESULT hr = GetBufferedPaintBits(hPaintBuffer, &prgbQuad, &cxRow);
	if (SUCCEEDED(hr))
	{
		ARGB *pargb = reinterpret_cast<ARGB *>(prgbQuad);
		if (!HasAlpha(pargb, sizIcon, cxRow))
		{
			ICONINFO info;
			if (GetIconInfo(hicon, &info))
			{
				if (info.hbmMask)
				{
					hr = ConvertToPARGB32(hdc, pargb, info.hbmMask, sizIcon, cxRow);
				}

				DeleteObject(info.hbmColor);
				DeleteObject(info.hbmMask);
			}
		}
	}

	return hr;
}

HBITMAP IconToBitmapPARGB32(HICON hIcon)
{
	SIZE sizIcon;
	sizIcon.cx = GetSystemMetrics(SM_CXSMICON);
	sizIcon.cy = GetSystemMetrics(SM_CYSMICON);

	RECT rcIcon;
	SetRect(&rcIcon, 0, 0, sizIcon.cx, sizIcon.cy);
	HBITMAP hBmp = NULL;

	HDC hdcDest = CreateCompatibleDC(NULL);
	if (hdcDest)
	{
		if (SUCCEEDED(Create32BitHBITMAP(hdcDest, &sizIcon, NULL, &hBmp)))
		{
			HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcDest, hBmp);
			if (hbmpOld)
			{
				BLENDFUNCTION bfAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
				BP_PAINTPARAMS paintParams = {0};
				paintParams.cbSize = sizeof(paintParams);
				paintParams.dwFlags = BPPF_ERASE;
				paintParams.pBlendFunction = &bfAlpha;

				HDC hdcBuffer;
				HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
				if (hPaintBuffer)
				{
					if (DrawIconEx(hdcBuffer, 0, 0, hIcon, sizIcon.cx, sizIcon.cy, 0, NULL, DI_NORMAL))
					{
						// If icon did not have an alpha channel we need to convert buffer to PARGB
						ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hIcon, sizIcon);
					}

					// This will write the buffer contents to the destination bitmap
					EndBufferedPaint(hPaintBuffer, TRUE);
				}

				SelectObject(hdcDest, hbmpOld);
			}
		}

		DeleteDC(hdcDest);
	}

	DestroyIcon(hIcon);

	return hBmp;
}

 DWORD PopUpMenu(HWND hWnd)
 {
	HMENU hmenu;            // top-level menu 
	HMENU hmenuTrackPopup;  // shortcut menu 
	POINT CurPos;			//Cursor position
	int iIndex = 0;
	SHSTOCKICONINFO ssi = { sizeof(ssi) };
	MENUITEMINFO mii = { sizeof(mii) };

	// Load the menu resource. 		 
	hmenu = LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(MENU_POPUP));
	if(hmenu)
	{	 
		// TrackPopupMenu cannot display the menu bar so get a handle to the first shortcut menu. 
		hmenuTrackPopup = GetSubMenu(hmenu, 0); 
		GetCursorPos(&CurPos);			//get the cursor position
		
		SetForegroundWindow(hWnd);

		iIndex = ListView_GetNextItem(LIST_CTRL,-1,LVNI_SELECTED);

		if( iIndex != -1 && bRequireElevate )
		{
			// Add the shield icon to the menu
			//if( !GetMenuItemInfo(hmenuTrackPopup,IDM_DELETE,FALSE,&mii) )
			//	FormatError(TEXT("GetMenuItemInfo"));

			if( SHGetStockIconInfo(SIID_SHIELD,SHGSI_SMALLICON | SHGSI_ICON,&ssi) != S_OK )
				FormatError(TEXT("SHGetStockIcon"));

			HICON hShieldIcon = ssi.hIcon;
			if( hShieldIcon )
			{
				/*Bitmap icon(hShieldIcon);
				Bitmap bmp(16, 16, PixelFormat32bppARGB);
				Graphics g(&bmp);
				g.DrawImage(&icon, 0, 0, 16, 16);
				COLORREF color;

				HBITMAP hBmp = NULL;
				HTHEME hTheme = OpenThemeData(hWnd, L"MenuStyle");
				if (hTheme == NULL)
				{
					MessageBox(hWnd, TEXT("Failed to open theme data"), TEXT("Error"), MB_OK);
				}
				GetThemeColor(hTheme, MENU_POPUPCHECKBACKGROUND, MCB_BITMAP, TMT_COLOR, &color);
				CloseThemeData(hTheme);
				bmp.GetHBITMAP(Color(GetRValue(color), GetGValue(color), GetBValue(color)), &hBmp);*/

				HBITMAP hBmp = NULL;
				hBmp = IconToBitmapPARGB32(hShieldIcon);
				
				mii.fMask |= MIIM_BITMAP;
				mii.hbmpItem = hBmp;
			}

			if( !SetMenuItemInfo(hmenuTrackPopup,IDM_DELETE,FALSE,&mii) )
				FormatError(TEXT("SetMenuItemInfo"));
		}

		//disable the delete menu when right click on the blank area
		if(iIndex == -1)
		{
				EnableMenuItem(hmenuTrackPopup,IDM_DELETE,MF_GRAYED);
				SetFocus(GetDlgItem(hWnd,LIST_IHJS));
		}

		// Display the shortcut menu. Track the right mouse button. 		 
		TrackPopupMenu(hmenuTrackPopup,TPM_LEFTALIGN|TPM_TOPALIGN,CurPos.x, CurPos.y, 0, hWnd, NULL); 
		PostMessage(hWnd,WM_NULL,0,0);
	 
		// Destroy the menu.		 
		DestroyMenu(hmenu); 
	
		if( iIndex != -1 && bRequireElevate )
			DestroyIcon(ssi.hIcon);
	}

	return 0;
}

 /*===============================================================================

*	Function			GetHijackNames
*	Purpose				Retrieve system Image File Execution Options entrys
*	Return				Return two arrays,one for ihj names,one for ihj paths

*/
//===============================================================================
DWORD GetHijackNames(IN OUT LPTSTR *plpImageName,IN OUT LPTSTR *plpHijackName)
{
	DWORD nEntrys = 0;

	TCHAR szHijackKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
	HKEY hHijackKey = 0;

	DWORD cSubKeys = 0;
	DWORD cMaxSubKeyLen = 0;
	LPTSTR lpSubKeyName = 0;
	DWORD cSubKeyLen = 0;
//	DWORD dwType = REG_SZ;

	BOOL bNoRet = FALSE;
	
	if(!plpHijackName || !plpImageName)
		bNoRet = TRUE;

	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,szHijackKey,0,KEY_READ,&hHijackKey) == ERROR_SUCCESS )
	{
		
		if( RegQueryInfoKey(hHijackKey,NULL,NULL,0,&cSubKeys,&cMaxSubKeyLen,NULL,NULL,NULL,NULL,NULL,NULL) == ERROR_SUCCESS )
		{
			//Enumerate subkeys, and see whether it's a hijack entry
 			if(cSubKeys)
			{
				cMaxSubKeyLen = cMaxSubKeyLen ? cMaxSubKeyLen:MAX_KEY_LENGTH;

				//eumerate keys
				for(DWORD i = 0; i != cSubKeys;++i)
				{
					cSubKeyLen = cMaxSubKeyLen + 1;
					lpSubKeyName = (LPTSTR)LocalAlloc(LPTR,cSubKeyLen * sizeof(TCHAR) );

					if( RegEnumKeyEx(hHijackKey,i,lpSubKeyName,&cSubKeyLen,0,NULL,NULL,NULL) == ERROR_SUCCESS)
					{
						HKEY hSubKey;
						LPTSTR szHijackPath;
						DWORD cbData = 0;
						LONG ret;

						LPTSTR lpszSubKey = (LPTSTR)LocalAlloc(LPTR,sizeof(szHijackKey) + cSubKeyLen * sizeof(TCHAR));
						StringCchPrintf(lpszSubKey,_countof(szHijackKey) + cSubKeyLen,TEXT("%s%s"),szHijackKey,lpSubKeyName);
						RegOpenKeyEx(HKEY_LOCAL_MACHINE,lpszSubKey,0,KEY_READ,&hSubKey);
						LocalFree(lpszSubKey);
						do
						{
							cbData += MAX_PATH * sizeof(TCHAR);
							szHijackPath = (LPTSTR)LocalAlloc(LPTR,cbData);
						}
						while( (ret = RegQueryValueEx(hSubKey,TEXT("Debugger"),NULL,NULL,(LPBYTE)szHijackPath,&cbData)) == ERROR_MORE_DATA );
						RegCloseKey(hSubKey);

						if( szHijackPath != NULL && _tcscmp(szHijackPath,TEXT("")) )
						{
							if( !bNoRet )
							{
								plpHijackName[nEntrys] = szHijackPath;
								plpImageName[nEntrys] = lpSubKeyName;
							}
							nEntrys ++;
						}

					}
				}
			}
		}
		RegCloseKey(hHijackKey);
	}	
	return nEntrys;
}

/*=========================================================================

*	Function			CreateHijackEntry
*	Purpose				Create an ihj entry in the system registry
*	Return				If the function succeeds, the return value is TRUE
						If the funciton fails, the return value is FALSE

*	Requirement			Need administrator privilege

*/
//=========================================================================
BOOL CreateHijackEntry(IN LPCTSTR lpHijackName)
{
	BOOL bRet = 0;
	TCHAR szHijackKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
	TCHAR szHijackExec[MAX_PATH] = {0};
	size_t dwSize = 0;
	HKEY hSubKey = NULL;

	if( !GetWindowsDirectory(szHijackExec,MAX_PATH) )
		return FALSE;
	StringCchCat(szHijackExec,MAX_PATH,TEXT("\\blank_ihj.exe"));

	StringCchLength(lpHijackName,MAX_VALUE_NAME,&dwSize);
	dwSize += _countof(szHijackKey);
	LPTSTR szSubKey = (LPTSTR)LocalAlloc(LPTR,dwSize * sizeof(TCHAR));
	StringCbCopy(szSubKey,LocalSize((HLOCAL)szSubKey),szHijackKey);
	StringCbCat(szSubKey,LocalSize((HLOCAL)szSubKey),lpHijackName);

	if( RegCreateKeyEx(HKEY_LOCAL_MACHINE,szSubKey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&hSubKey,NULL) == ERROR_SUCCESS)
	{
		if( RegSetValueEx(hSubKey,TEXT("Debugger"),0,REG_SZ,(BYTE*)szHijackExec,sizeof(szHijackExec) )== ERROR_SUCCESS)
			bRet = TRUE;
		
		RegCloseKey(hSubKey);
	}

	LocalFree((HLOCAL)szSubKey);
	return bRet;
}

/*=========================================================================

*	Function			DeleteHijackEntry
*	Purpose				Delete an ihj entry in the system registry
*	Return				If the function succeeds, the return value is TRUE
						If the funciton fails, the return value is FALSE

*	Requirement			Need administrator privilege

*/
//=========================================================================
BOOL DeleteHijackEntry(IN LPCTSTR lpHijackName)
{
	BOOL bRet = FALSE;
	TCHAR szHijackKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
	size_t dwSize = 0;

	StringCchLength(lpHijackName,MAX_VALUE_NAME,&dwSize);
	dwSize += _countof(szHijackKey);
	LPTSTR szSubKey = (LPTSTR)LocalAlloc(LPTR,dwSize * sizeof(TCHAR));
	StringCbCopy(szSubKey,LocalSize((HLOCAL)szSubKey),szHijackKey);
	StringCbCat(szSubKey,LocalSize((HLOCAL)szSubKey),lpHijackName);

	if( RegDeleteKey(HKEY_LOCAL_MACHINE,szSubKey) == ERROR_SUCCESS )
		bRet =TRUE;

	LocalFree((HLOCAL)szSubKey);

	return bRet;
}

/*=====================================================================================

*	Function			FreeHijackBuffer
*	Purpose				Free the memory that used to store hij informations.
*						These memory must be allocated with LocalAlloc.
*	Return				This function has no return value.

*/
//=====================================================================================
VOID FreeHijackBuffer(IN LPTSTR* &plpBuffer1,IN LPTSTR* &plpBuffer2,IN DWORD cEntrys)
{
	if(plpBuffer1)
		for(DWORD i = 0; i != cEntrys;++i)
			if(plpBuffer1[i])
				LocalFree((HLOCAL)plpBuffer1[i]);
	LocalFree((HLOCAL)plpBuffer1);
	plpBuffer1 = 0;

	if(plpBuffer2)
		for(DWORD i = 0; i != cEntrys;++i)
			if(plpBuffer2[i])
				LocalFree((HLOCAL)plpBuffer2[i]);
	LocalFree((HLOCAL)plpBuffer2);
	plpBuffer2 = 0;

}

//BOOL ExtractRes(IN LPCTSTR lpResType,IN LPCTSTR lpResName)
//{
//	TCHAR szFilePath[MAX_PATH] = {0};
//	UINT uRetVal = 0;
//	TCHAR szError[16];
//
//	HRSRC hRes = FindResource(NULL,lpResName,lpResType);
//	if(!hRes)
//	{
//		MessageBox(NULL,TEXT("Can not find the specific resource."),TEXT("Hijack It!"),MB_OK | MB_ICONEXCLAMATION);
//		return FALSE;
//	}
//
//	HGLOBAL hResGlobal = LoadResource(NULL,hRes);
//	if(!hResGlobal)
//	{
//		MessageBox(NULL,TEXT("Can not load the specific resource."),TEXT("Hijack It!"),MB_OK | MB_ICONEXCLAMATION);
//		return FALSE;
//	}
//
//	DWORD dwResSize = SizeofResource(NULL,hRes);
//	LPBYTE pResPtr = (LPBYTE)LockResource(hResGlobal);
//	if(!pResPtr)
//	{
//		MessageBox(NULL,TEXT("Can not lock the specific resource."),TEXT("Hijack It!"),MB_OK | MB_ICONEXCLAMATION);
//		return FALSE;
//	}
//
//	uRetVal = GetWindowsDirectory(szFilePath,MAX_PATH); 
//	if(!uRetVal)
//	{
//		MessageBox(NULL,TEXT("Failed to retrieve the windows directory."),TEXT("Hijack It!"),MB_OK | MB_ICONEXCLAMATION);
//		return FALSE;
//	}
//	StringCchCat(szFilePath,MAX_PATH,TEXT("\\blank_ihj.exe"));
//	
//	HANDLE hFile = CreateFile(szFilePath,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
//	if(hFile == INVALID_HANDLE_VALUE)
//	{
//		_ultow_s(GetLastError(),szError,_countof(szError),10);
//		MessageBox(NULL,szError,TEXT("error"),MB_OK);
//		MessageBox(NULL,TEXT("Can not create blank_ihj.exe."),TEXT("Hijack It!"),MB_OK | MB_ICONEXCLAMATION);
//		return FALSE;
//	}
//
//	//write to the temp file
//	DWORD dwBytesWritten = 0;
//	WriteFile(hFile,pResPtr,dwResSize,&dwBytesWritten,NULL);
//
//	UnlockResource(hResGlobal);
//	CloseHandle(hFile);
//	
//	return TRUE;
//}

VOID UpdateSysLinkState(HWND hWndLink)
{
	HANDLE hFile;
	TCHAR szFilePath[MAX_PATH];
	
	if( GetWindowsDirectory(szFilePath,MAX_PATH) )
	{
		StringCchCat(szFilePath,MAX_PATH,TEXT("\\blank_ihj.exe"));
		hFile = CreateFile(szFilePath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			if(GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				ShowWindow(hWndLink,SW_HIDE);
				ShowWindow(GetDlgItem(GetParent(hWndLink),SYSLINK_RESTORE),SW_SHOW);
			}
		}
		else
		{
			ShowWindow(GetDlgItem(GetParent(hWndLink),SYSLINK_RESTORE),SW_HIDE);
			ShowWindow(hWndLink,SW_SHOW);
			CloseHandle(hFile);
		}
	}
}

BOOL GetProcessElevation(TOKEN_ELEVATION_TYPE* pElevationType, BOOL* pIsAdmin) 
{
   HANDLE hToken = NULL;
   DWORD dwSize; 

   // Get current process token
   if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
      return(FALSE);

   BOOL bResult = FALSE;

   // Retrieve elevation type information 
   if (GetTokenInformation(hToken, TokenElevationType,pElevationType, sizeof(TOKEN_ELEVATION_TYPE), &dwSize)) 
   {
      // Create the SID corresponding to the Administrators group
      byte adminSID[SECURITY_MAX_SID_SIZE];
      dwSize = sizeof(adminSID);

      CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSID,&dwSize);

      if (*pElevationType == TokenElevationTypeLimited) 
	  {
         // Get handle to linked token (will have one if we are lua)
         HANDLE hUnfilteredToken = NULL;
         GetTokenInformation(hToken, TokenLinkedToken, (VOID*)&hUnfilteredToken, sizeof(HANDLE), &dwSize);

         // Check if this original token contains admin SID
         if (CheckTokenMembership(hUnfilteredToken, &adminSID, pIsAdmin)) 
            bResult = TRUE;
         
         // Don't forget to close the unfiltered token
         CloseHandle(hUnfilteredToken);
      } 
	  else 
	  {
         *pIsAdmin = IsUserAnAdmin();
         bResult = TRUE;
      }
   }

   // Don't forget to close the process token
   CloseHandle(hToken);

   return(bResult);
}

void GetSetPositionInfoFromRegistry( BOOL fSave, POINT *lppt )
{
    HKEY hKey;
    DWORD dataSize, err, disposition;
    TCHAR szKeyName[] = TEXT("DlgCoordinates");
	TCHAR szRegistryKey[] = TEXT("Software\\WSuit\\Hijack It!");

    // In case the key's not there yet, we'll
	// return center pos for the coordinates
    if ( !fSave ) 
	{
		lppt->x = (GetSystemMetrics(SM_CXVIRTUALSCREEN) - WINDOW_WIDTH) / 2;
		lppt->y = (GetSystemMetrics(SM_CYVIRTUALSCREEN) - WINDOW_HEIGHT) / 2;
	}

    // Open the registry key (or create it if the first time being used)
    err = RegCreateKeyEx( HKEY_CURRENT_USER, szRegistryKey, 0, 0,
                         REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                         0, &hKey, &disposition );
    if ( ERROR_SUCCESS != err )
        return;

    if ( fSave )            // Save out coordinates
    {
        RegSetValueEx( hKey, szKeyName, 0, REG_BINARY,
                       (PBYTE)lppt, sizeof(*lppt) );
    }
    else                    // read in coordinates
    {
        dataSize = sizeof(*lppt);
        RegQueryValueEx( hKey, szKeyName, 0, 0, (PBYTE)lppt, &dataSize );
    }
}

BOOL DoJobHijack(DWORD cEntry, PCTSTR* pszEntryNames, DWORD dwOption)
{
	TCHAR szTempPath[MAX_PATH],szTempFileName[MAX_PATH];
	HANDLE hFile;
	HijackInfo hi;
	DWORD dwBytesReturn;
	DWORD cbStrLen = 0;
	SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };

	switch( dwOption )
	{
	case HIJACK_CREATE:
	case HIJACK_DELETE:		
		GetTempPath(MAX_PATH,szTempPath);
		GetTempFileName(szTempPath,TEXT("WHH"),0,szTempFileName);

		hFile = CreateFile(szTempFileName,FILE_ALL_ACCESS,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if( hFile == INVALID_HANDLE_VALUE )
		{
			FormatError(TEXT("CreateFile"));
			return FALSE;
		}
		
		hi.cEntry = cEntry;
		hi.dwOption = dwOption;

		// Write the header
		WriteFile(hFile,&hi,sizeof(HijackInfo),&dwBytesReturn,NULL);

		// Write names
		for(DWORD i = 0; i != cEntry; ++i)
		{
			cbStrLen = (_tcslen(pszEntryNames[i]) + 1) * sizeof(TCHAR);
			WriteFile(hFile,pszEntryNames[i],cbStrLen,&dwBytesReturn,NULL);
		}
		CloseHandle(hFile);		
		break;

	case HIJACK_ADD_BLANK:
	case HIJACK_DEL_BLANK:
		StringCchPrintf(szTempFileName,MAX_PATH,TEXT("???%u"),dwOption);
		break;
	}

	sei.lpParameters = szTempFileName;	
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	sei.lpVerb = TEXT("runas");		// Ask for privileges elevation.
	PTSTR pszWorkerPath = GetWorkerFileName();
	sei.lpFile = pszWorkerPath;
	//MessageBox(NULL,pszWorkerPath,NULL,0);
	if( !ShellExecuteEx(&sei) )
	{
		FormatError(TEXT("ShellExcuteEx"));
		return FALSE;
	}
	LocalFree(pszWorkerPath);
	HANDLE hWorkerProc = sei.hProcess;

	WaitForSingleObject(hWorkerProc,INFINITE);

	DWORD dwExitCode = 0;
	GetExitCodeProcess(hWorkerProc,&dwExitCode);
	CloseHandle(hWorkerProc);

	DeleteFile(szTempFileName);

	if( dwExitCode == HijackFailed )
		return FALSE;
	else
		return TRUE;
}


void FormatError(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (_tcslen((LPCTSTR)lpMsgBuf)+_tcslen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Worker"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

PTSTR GetWorkerFileName()
{
	TCHAR szWorkerName[] = TEXT("DoHijack.exe");
	
	DWORD cchSize = GetTempPath(0,NULL);
	PTSTR pszTempPath = (PTSTR)LocalAlloc(LPTR,cchSize * sizeof(TCHAR));
	GetTempPath(cchSize,pszTempPath);

	DWORD cchWorker = _tcslen(pszTempPath) + _countof(szWorkerName);
	PTSTR pszWorkerFilenName = (PTSTR)LocalAlloc(LPTR,cchWorker * sizeof(TCHAR));
	StringCchPrintf(pszWorkerFilenName,cchWorker,TEXT("%s%s"),pszTempPath,szWorkerName);

	if ( ExtractRes(TEXT("BINARY"),MAKEINTRESOURCE(IDR_WORKER),pszWorkerFilenName) )
		return pszWorkerFilenName;
	else
		return NULL;
}

BOOL ExtractRes(IN LPCTSTR lpResType,IN LPCTSTR lpResName, PCTSTR pszTargeTPath)
{
	HRSRC hRes = FindResource(NULL,lpResName,lpResType);
	if(!hRes)
	{
		FormatError(TEXT("FindResource"));
		return FALSE;
	}

	HGLOBAL hResGlobal = LoadResource(NULL,hRes);
	if(!hResGlobal)
	{
		FormatError(TEXT("LoadResource"));
		return FALSE;
	}

	DWORD dwResSize = SizeofResource(NULL,hRes);
	LPBYTE pResPtr = (LPBYTE)LockResource(hResGlobal);
	if(!pResPtr)
	{
		FormatError(TEXT("LockResource"));
		return FALSE;
	}
	
	HANDLE hFile = CreateFile(pszTargeTPath,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		FormatError(TEXT("CreateFile"));
		return FALSE;
	}

	//write to the temp file
	DWORD dwBytesWritten = 0;
	WriteFile(hFile,pResPtr,dwResSize,&dwBytesWritten,NULL);

	UnlockResource(hResGlobal);
	CloseHandle(hFile);
	
	return TRUE;
}

//BOOL ExtractBlank(IN LPCTSTR lpResType,IN LPCTSTR lpResName)
//{
//	TCHAR szFilePath[MAX_PATH];
//
//	DWORD uRetVal = GetWindowsDirectory(szFilePath,MAX_PATH); 
//	if(!uRetVal)
//	{
//		FormatError(TEXT("GetWindowsDirectory"));
//		return FALSE;
//	}
//	StringCchCat(szFilePath,MAX_PATH,TEXT("\\blank_ihj.exe"));
//
//	return ExtractRes(lpResType,lpResName,szFilePath);
//}

