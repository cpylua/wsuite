#include <windows.h>
#include <tchar.h>
#include <shldisp.h>

typedef VOID (WINAPI *lpRUNFILEDLG)(    
	IN HWND    hwndOwner,       // Owner window, receives notifications
    IN HICON   hIcon,           // Dialog icon handle, if NULL default icon is used
    IN LPCTSTR lpszDirectory,   // Working directory
    IN LPCTSTR lpszTitle,       // Dialog title, if NULL default is displayed
    IN LPCTSTR lpszDescription, // Dialog description, if NULL default is displayed
    IN UINT    uFlags			// Dialog flags
	);
//You can specify one or more of the following values in the uFlags parameter: 
//
//RFF_NOBROWSE - Remove the browse button.
//RFF_NODEFAULT - No default item selected.
//RFF_CALCDIRECTORY - Calculate the working directory from the file name.
//RFF_NOLABEL - Remove the edit box label.
//RFF_NOSEPARATEMEM - Remove the Separate Memory Space check box, NT only.

// RunFileDlg flags
#define RFF_NOBROWSE		0x0001	// Removes the browse button
#define RFF_NODEFAULT		0x0002	// No default item selected
#define RFF_CALCDIRECTORY	0x0004	// Calculates the working directory from the file name
#define RFF_NOLABEL			0x0008	// Removes the edit box label
#define RFF_NOSEPARATEMEM	0x0020  // Removes the Separate Memory Space check box, NT only


int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,
				   int nShowCmd)
{
/*	lpRUNFILEDLG RunFileDlg;

	HMODULE hLib = LoadLibrary(TEXT("shell32.dll"));
	RunFileDlg = (lpRUNFILEDLG)GetProcAddress(hLib,LPCSTR(61));

	RunFileDlg(NULL,NULL,NULL,NULL,NULL,RFF_NODEFAULT);

	FreeLibrary(hLib)*/;

	IShellDispatch* pShellDisp = NULL;
	CoInitialize(NULL);

    HRESULT hr = CoCreateInstance( CLSID_Shell, NULL, CLSCTX_SERVER, 
		IID_IShellDispatch, (LPVOID *) &pShellDisp );

	if( hr == S_OK )
	{
		pShellDisp->FileRun();  
		pShellDisp->Release();
		pShellDisp = NULL;
	}
	CoUninitialize();   
	//DWORD dwError = GetLastError();
	//TCHAR szError[32];
	//_ultow_s(dwError,szError,32,10);
	//MessageBox(NULL,szError,TEXT("error"),MB_OK);



	return 0;
}

