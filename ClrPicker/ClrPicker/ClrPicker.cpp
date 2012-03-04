#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "uxTheme.lib")
#pragma comment(lib, "dwmapi.lib")

#include <Windows.h>
#include <tchar.h>
#include <WindowsX.h>
#include <dwmapi.h>
#include <Uxtheme.h>
#include <vssym32.h>
#include <process.h>
#include <StrSafe.h>

#include "resource.h"

#define MAX_LOADSTRING 100

#ifdef UNICODE
#define CF_TCHAR CF_UNICODETEXT
#else
#define CF_TCHAR CF_TEXT
#endif

typedef unsigned (__stdcall *PTHREAD_START) (void *);

#define chBEGINTHREADEX(psa, cbStackSize, pfnStartAddr, \
	pvParam, dwCreateFlags, pdwThreadId)                 \
	((HANDLE)_beginthreadex(                          \
	(void *)        (psa),                         \
	(unsigned)      (cbStackSize),                 \
	(PTHREAD_START) (pfnStartAddr),                \
	(void *)        (pvParam),                     \
	(unsigned)      (dwCreateFlags),               \
	(unsigned *)    (pdwThreadId)))

HANDLE g_hWorker;
HANDLE g_hTimer = NULL;
BOOL g_bExit = FALSE, g_bUseHex = FALSE, g_bOnTop = TRUE;
COLORREF g_crPixel;


//
// Helper functions
//

BOOL CheckOS()
{
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_GREATER_EQUAL;
	BOOL bVer;

	// Initialize the OSVERSIONINFOEX structure.
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 0;


	// Initialize the condition mask.
	VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, op );
	VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, op );

	// Perform the test.
	bVer = VerifyVersionInfo(
				&osvi, 
				VER_MAJORVERSION | VER_MINORVERSION,
				dwlConditionMask);
	if( !bVer ) return bVer;

	DwmIsCompositionEnabled (&bVer);

	return bVer;
}

void ShowError(HWND hwnd, int idFormat)
{
	TCHAR szFormat[MAX_LOADSTRING], szMsg[MAX_LOADSTRING];
	TCHAR szAppName[MAX_LOADSTRING];
	HMODULE hModule = GetModuleHandle(NULL);

	LoadString(hModule, IDS_APP_NAME, szAppName, MAX_LOADSTRING);
	LoadString(hModule, idFormat, szFormat, MAX_LOADSTRING);

	StringCchPrintf(szMsg, MAX_LOADSTRING, szFormat, GetLastError());
	MessageBox(hwnd, szMsg, szAppName, MB_OK | MB_ICONEXCLAMATION);
}

void PaintColor(HWND hwnd,  COLORREF crPixel)
{
	TCHAR szBuffer[32];
	HDC hdc = GetDC(hwnd);
	COLORREF crText = RGB(56, 51, 153);
	TCHAR szDecFormat[16];
	TCHAR szHexFormat[32];

	HTHEME hTheme = OpenThemeData(hwnd, L"ControlPanelStyle");
	if( hTheme )
	{
		HDC hdcPaint = NULL;

		BP_PAINTPARAMS params = { sizeof(BP_PAINTPARAMS) };
		params.dwFlags = BPPF_ERASE;

		RECT rcClient;
		GetClientRect(hwnd, &rcClient);

		HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdc, &rcClient,
			BPBF_TOPDOWNDIB, &params, &hdcPaint);

		if( hdcPaint )
		{
			DTTOPTS DttOpts = { sizeof(DTTOPTS) };
			DttOpts.dwFlags = DTT_COMPOSITED | DTT_TEXTCOLOR | DTT_GLOWSIZE;
			DttOpts.crText = crText;
			DttOpts.iGlowSize = 12;

			LOGFONT lf;
			GetThemeSysFont(hTheme, TMT_CAPTIONFONT, &lf);
			HFONT hFont = CreateFontIndirect(&lf);
			HFONT hOldFont = (HFONT)SelectObject(hdcPaint, hFont);

			LoadString(GetModuleHandle(NULL), IDS_USE_DEC, szDecFormat, _countof(szDecFormat));
			LoadString(GetModuleHandle(NULL), IDS_USE_HEX, szHexFormat, _countof(szHexFormat));
			StringCchPrintf(szBuffer, _countof(szBuffer),
				g_bUseHex ? szHexFormat : szDecFormat, 
				GetRValue(crPixel), GetGValue(crPixel), GetBValue(crPixel));

			DrawThemeTextEx(hTheme, hdcPaint, 0, 0, szBuffer, _tcslen(szBuffer), 
				DT_CENTER | DT_WORDBREAK | DT_SINGLELINE, &rcClient, &DttOpts);

			DeleteObject( SelectObject(hdcPaint, hOldFont) );

		}

		EndBufferedPaint(hBufferedPaint, TRUE);
		ReleaseDC(hwnd, hdc);
	}	/* if( hTheme ) */

	CloseThemeData(hTheme);
}

DWORD Thread_Proc(PVOID pvParam)
{
	HWND hwnd = (HWND)pvParam;
	SHORT sKeyState;
	USHORT usMask = 0x8000;
	HDC hdcScreen = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
	POINT pt;

	while( !g_bExit )
	{
		WaitForSingleObject(g_hTimer, INFINITE);

		sKeyState = GetAsyncKeyState(VK_MENU);
		if( sKeyState & usMask )
		{
			GetCursorPos(&pt);

			g_crPixel = GetPixel(hdcScreen, pt.x, pt.y);
			RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
		}
	}

	DeleteDC(hdcScreen);

	return 0;
}

void PopupMenu(HWND hwnd)
{
	POINT pt;
	HMENU hMenu, hSubMenu;

	hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDM_POPUP));
	hSubMenu = GetSubMenu(hMenu, 0);

	CheckMenuItem( hMenu, ID_POPUP_SHOWINHEX, 
		g_bUseHex ? MF_CHECKED : MF_UNCHECKED );

	CheckMenuItem( hMenu, ID_POPUP_ONTOP, 
		g_bOnTop ? MF_CHECKED : MF_UNCHECKED );

	GetCursorPos(&pt);
	SetForegroundWindow(hwnd);
	TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_TOPALIGN, 
		pt.x, pt.y, 0,
		hwnd, NULL);

	DestroyMenu(hMenu);
}


//
// Handle windows messages
//

//
//  Process WM_COMMAND message for window/dialog: Wnd
//
void Wnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	static TCHAR szFormat[32], szText[32];
	HGLOBAL hGlobal = NULL;
	PTSTR pGlobal = NULL;
	int cch;

	switch(id)
	{
		case ID_POPUP_EXIT:
			DestroyWindow(hwnd);
			break;

		case ID_POPUP_COPYASCSSSTYLE:
		{
			LoadString( GetModuleHandle(NULL), IDS_COPY_CSS,
				szFormat, _countof(szFormat) );
			goto DO_COPY;	/* Fall through */
		}
		case ID_POPUP_COPY:
		{
			LoadString( GetModuleHandle(NULL), g_bUseHex ? IDS_COPY_HEX : IDS_COPY_DEC,
				szFormat, _countof(szFormat) );
			goto DO_COPY;	/* Fall through */
		}
DO_COPY:
		{
			StringCchPrintf( szText, _countof(szText), szFormat, 
				GetRValue(g_crPixel), GetGValue(g_crPixel), GetBValue(g_crPixel) );
			cch = _tcslen(szText) + 1;

			hGlobal = GlobalAlloc(GHND | GMEM_SHARE,
				cch * sizeof(TCHAR) );
			pGlobal = (PTSTR)GlobalLock(hGlobal);
			StringCchCopy(pGlobal, cch, szText);
			GlobalUnlock(hGlobal);

			OpenClipboard(hwnd);
			EmptyClipboard();
			SetClipboardData(CF_TCHAR, hGlobal);
			CloseClipboard();

			break;
		}

		case ID_POPUP_SHOWINHEX:
			g_bUseHex = !g_bUseHex;

			InvalidateRect(hwnd, NULL, TRUE);
			break;

		case ID_POPUP_ONTOP:
		{
			g_bOnTop = !g_bOnTop;

			SetForegroundWindow(hwnd);
			SetWindowPos(hwnd, g_bOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
				0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

			break;
		}
	}
}

//
//  Process WM_CREATE message for window/dialog: Wnd
//
BOOL Wnd_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	TCHAR szTimer[MAX_LOADSTRING];
	LARGE_INTEGER li;

	SetForegroundWindow(hwnd);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);

	LoadString(GetModuleHandle(NULL), IDS_TIMER, szTimer, MAX_LOADSTRING);

	g_hTimer = CreateWaitableTimer(NULL, FALSE, szTimer);
	if( !g_hTimer )
	{
		ShowError(hwnd, IDS_ERR_CREATE_TIMER);
		return FALSE;
	}
	li.QuadPart = -1;	// 100 nanoseconds
	SetWaitableTimer(g_hTimer, &li, 60, NULL, NULL, FALSE);

	g_hWorker = chBEGINTHREADEX(NULL, 0, (PTHREAD_START)Thread_Proc, hwnd, 0, NULL);

	return TRUE;
}

//
//  Process WM_DESTROY message for window/dialog: Wnd
//
void Wnd_OnDestroy(HWND hwnd)
{
	g_bExit = TRUE;
	WaitForSingleObject(g_hWorker, INFINITE);

	CloseHandle(g_hTimer);
	CloseHandle(g_hWorker);

	PostQuitMessage(0);
	BufferedPaintUnInit();
}


//
//  Process WM_NCRBUTTONDOWN message for window/dialog: Wnd
//
void Wnd_OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
{
	PopupMenu(hwnd);
}


//
//  Process WM_ERASEBKGND message for window/dialog: Wnd
//
BOOL Wnd_OnEraseBkgnd(HWND hwnd, HDC hdc)
{
	// prevent from erasing background
	return FALSE;
}


//
//  Process WM_PAINT message for window/dialog: Wnd
//
void Wnd_OnPaint(HWND hwnd)
{
	HDC hdc;
	PAINTSTRUCT ps;

	MARGINS marGlassInset = { -1, -1, -1, -1 };
	DwmExtendFrameIntoClientArea(hwnd, &marGlassInset);

	hdc =BeginPaint(hwnd, &ps);

	PaintColor(hwnd, g_crPixel);

	EndPaint(hwnd, &ps);
}

//
//  Process WM_NCHITTEST message for window/dialog: Wnd
//
UINT Wnd_OnNCHitTest(HWND hwnd, int x, int y)
{
	// TODO: Add your message processing code here...
	LRESULT lrSpot = DefWindowProc(hwnd, WM_NCHITTEST, 0, MAKELPARAM(x, y));
	lrSpot = (lrSpot == HTCLIENT) ? HTCAPTION : lrSpot;

	return lrSpot;
}


//
// Wnd  Window Procedure
//
LRESULT CALLBACK Wnd_WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		HANDLE_MSG (hwnd, WM_COMMAND, Wnd_OnCommand);
		HANDLE_MSG (hwnd, WM_CREATE, Wnd_OnCreate);
		HANDLE_MSG (hwnd, WM_DESTROY, Wnd_OnDestroy);
		HANDLE_MSG (hwnd, WM_PAINT, Wnd_OnPaint);
		HANDLE_MSG (hwnd, WM_ERASEBKGND, Wnd_OnEraseBkgnd);
		HANDLE_MSG (hwnd, WM_NCRBUTTONUP, Wnd_OnNCRButtonUp);
		HANDLE_MSG (hwnd, WM_NCHITTEST, Wnd_OnNCHitTest);

	default: return DefWindowProc (hwnd, msg, wParam, lParam);
	}
}

void FindWindowSize(int *pcxWindow, int *pcyWindow)
{
	HDC        hdcScreen ;
	TEXTMETRIC tm ;

	hdcScreen = CreateDC (TEXT ("DISPLAY"), NULL, NULL, NULL) ;
	GetTextMetrics (hdcScreen, &tm) ;
	DeleteDC (hdcScreen) ;

	* pcxWindow = 2 * GetSystemMetrics (SM_CXBORDER)  + 
		12 * tm.tmAveCharWidth ;

	* pcyWindow = 2 * GetSystemMetrics (SM_CYBORDER)  +
		GetSystemMetrics (SM_CYCAPTION) + 
		2 * tm.tmHeight ;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PTSTR pCmdLine, int nShowCmd)
{
	WNDCLASS wndclass;
	TCHAR szAppName[MAX_LOADSTRING];
	TCHAR szParentName[MAX_LOADSTRING];
	HWND hwnd = NULL, hwndParent = NULL;
	HACCEL hAccel;
	MSG msg;
	int cx, cy, x, y;

	if( !CheckOS() )
	{
		LoadString(hInstance, IDS_ERR_OS, szParentName, MAX_LOADSTRING);
		LoadString(hInstance, IDS_APP_NAME, szAppName, MAX_LOADSTRING);
		MessageBox(NULL, szParentName, szAppName, MB_OK | MB_ICONEXCLAMATION);
		return -1;
	}

	LoadString(hInstance, IDS_APP_NAME, szAppName, MAX_LOADSTRING);
	LoadString(hInstance, IDS_PARENT_WND_CLASS, szParentName, MAX_LOADSTRING);

	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_CLRPICKER) );
	wndclass.hInstance = hInstance;
	wndclass.lpfnWndProc = Wnd_WndProc;
	wndclass.lpszClassName = szAppName;
	wndclass.lpszMenuName = NULL;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClass(&wndclass);

	// Create an invisible parent window
	wndclass.lpfnWndProc = DefWindowProc;
	wndclass.lpszClassName = szParentName;
	wndclass.style = 0;

	RegisterClass(&wndclass);
	hwndParent = CreateWindow(szParentName, NULL, WS_OVERLAPPEDWINDOW,
		0, 0, 0, 0, NULL, NULL, hInstance, NULL);

	// determine the windows size based on system font
	FindWindowSize(&cx, &cy);
	x = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;
	y = GetSystemMetrics (SM_CYBORDER);

	hwnd = CreateWindow(szAppName, NULL,
		WS_POPUP | WS_VISIBLE,
		x, y, cx, cy,
		hwndParent, NULL, hInstance, NULL);

	if( !hwnd )
	{
		ShowError(NULL, IDS_ERR_CREATE_WND);
		return -1;
	}

	BufferedPaintInit();

	ShowWindow(hwnd, nShowCmd);
	UpdateWindow(hwnd);

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));

	while( GetMessage(&msg, NULL, 0, 0) )
	{
		if( !TranslateAccelerator(hwnd, hAccel, &msg) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DestroyWindow(hwndParent);

	return (int)msg.wParam;
}