#include <windows.h>
#include "resource.h"

LRESULT CALLBACK OptHdrDlg(HWND hOptHdrDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ShowError();
BOOL SetEditText(HWND hWnd,int nDlgItemID,DWORD dwNumToSet);
DWORD GetStrToNum(HWND hDlg,int nIDDlgItem,BOOL* bRet);
BOOL IsPEFile(TCHAR* lpFileName);

extern IMAGE_NT_HEADERS	ntHdr;
extern IMAGE_DOS_HEADER	dosHdr;
extern TCHAR	szFileName[1024];
extern HINSTANCE hAppHwnd;


TCHAR* szStrToAdd[] = {L"EXPORT",L"IMPORT",L"RESOURCE",L"EXCEPTION",L"SECURITY",L"BASERELOC",L"DEBUG",
					  L"ARCHITECTURE",L"GLOBALPTR",L"TLS",L"LOAD_CONFIG",L"BOUND_IMPORT",L"IAT",L"DELAY_IMPORT",
					  L"COM_DESCRIPTOR"};


LRESULT CALLBACK OptHdrDlg(HWND hOptHdrDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet;
	HANDLE hFile;
	DWORD dwPtr = 0;
	DWORD dwNumOfBytesWritten = 0;
	LRESULT Index ;
	HICON hIcon;

	switch (message)
	{
	case WM_INITDIALOG:
		//load icon
		hIcon = LoadIcon(hAppHwnd,(LPCTSTR)IDI_ICON_PE);
		SendMessage(hOptHdrDlg,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
		//initialize the datas

		SetEditText(hOptHdrDlg,IDC_EDIT_ADDRESSOFENTRYPOINT,ntHdr.OptionalHeader.AddressOfEntryPoint);
		SetEditText(hOptHdrDlg,IDC_EDIT_BASEOFCODE,ntHdr.OptionalHeader.BaseOfCode);
		SetEditText(hOptHdrDlg,IDC_EDIT_BASEOFDATA,ntHdr.OptionalHeader.BaseOfData);
		SetEditText(hOptHdrDlg,IDC_EDIT_CHECKSUM,ntHdr.OptionalHeader.CheckSum);
		SetEditText(hOptHdrDlg,IDC_EDIT_DLLCHARATERRISTICS,ntHdr.OptionalHeader.DllCharacteristics);
		SetEditText(hOptHdrDlg,IDC_EDIT_FILESLIGNMENT,ntHdr.OptionalHeader.FileAlignment);
		SetEditText(hOptHdrDlg,IDC_EDIT_IMAGEBASE,ntHdr.OptionalHeader.ImageBase);
		SetEditText(hOptHdrDlg,IDC_EDIT_LOADFLAG,ntHdr.OptionalHeader.LoaderFlags);
		SetEditText(hOptHdrDlg,IDC_EDIT_MAGIC,ntHdr.OptionalHeader.Magic);
		SetEditText(hOptHdrDlg,IDC_EDIT_MAJORIMAGEVERSION,ntHdr.OptionalHeader.MajorImageVersion);
		SetEditText(hOptHdrDlg,IDC_EDIT_MINORIMAGEVERSION,ntHdr.OptionalHeader.MinorImageVersion);
		SetEditText(hOptHdrDlg,IDC_EDIT_MAJORLINKER,ntHdr.OptionalHeader.MajorLinkerVersion);
		SetEditText(hOptHdrDlg,IDC_EDIT_MAJOROPERATONVER,ntHdr.OptionalHeader.MajorOperatingSystemVersion);
		SetEditText(hOptHdrDlg,IDC_EDIT_MAJORSUBSYSTEMVERSION,ntHdr.OptionalHeader.MajorSubsystemVersion);
		SetEditText(hOptHdrDlg,IDC_EDIT_MINORLINKERVERSION,ntHdr.OptionalHeader.MinorLinkerVersion);
		SetEditText(hOptHdrDlg,IDC_EDIT_MINOROPERATIONVER,ntHdr.OptionalHeader.MinorOperatingSystemVersion);
		SetEditText(hOptHdrDlg,IDC_EDIT_MINORSUBSYSTEMVERSION,ntHdr.OptionalHeader.MinorSubsystemVersion);
		SetEditText(hOptHdrDlg,IDC_EDIT_NUMOFRVAANDSIZE,ntHdr.OptionalHeader.NumberOfRvaAndSizes);
		SetEditText(hOptHdrDlg,IDC_EDIT_SECTIONALIGNMENT,ntHdr.OptionalHeader.SectionAlignment);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEODCODE,ntHdr.OptionalHeader.SizeOfCode);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEOFHEADERS,ntHdr.OptionalHeader.SizeOfHeaders);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEOFHEAPCOMMIT,ntHdr.OptionalHeader.SizeOfHeapCommit);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEOFHEAPRESERVE,ntHdr.OptionalHeader.SizeOfHeapReserve);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEOFIMAGE,ntHdr.OptionalHeader.SizeOfImage);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEOFINILIZEDATA,ntHdr.OptionalHeader.SizeOfInitializedData);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEOFSTACKCOMMIT,ntHdr.OptionalHeader.SizeOfStackCommit);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEOFSTACKRESERVE,ntHdr.OptionalHeader.SizeOfStackReserve);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZEOFUNINILIZEDDATA,ntHdr.OptionalHeader.SizeOfUninitializedData);
		SetEditText(hOptHdrDlg,IDC_EDIT_SUBSYSTEM,ntHdr.OptionalHeader.Subsystem);
		SetEditText(hOptHdrDlg,IDC_EDIT_WIN32VERSIONVALUE,ntHdr.OptionalHeader.Win32VersionValue);
		SetEditText(hOptHdrDlg,IDC_EDIT_VIRTUALADDRESS,ntHdr.OptionalHeader.DataDirectory[0].VirtualAddress);
		SetEditText(hOptHdrDlg,IDC_EDIT_SIZE,ntHdr.OptionalHeader.DataDirectory[0].Size);
		//set the combobox strings
		for (int i = 0;i<15;++i)
		{
		SendDlgItemMessage(hOptHdrDlg,IDC_COMBO_DATADIR,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)szStrToAdd[i]);		
		}
		SendDlgItemMessage(hOptHdrDlg,IDC_COMBO_DATADIR,CB_SETCURSEL,0,(LPARAM)0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_OPTHDR_OK:
			Index = SendDlgItemMessage(hOptHdrDlg,IDC_COMBO_DATADIR,CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
			if (Index != CB_ERR)
			{
					ntHdr.OptionalHeader.DataDirectory[Index].VirtualAddress = GetStrToNum(hOptHdrDlg,IDC_EDIT_VIRTUALADDRESS,&bRet);
					ntHdr.OptionalHeader.DataDirectory[Index].Size = GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZE,&bRet);
			}
			ShowError();
			ntHdr.OptionalHeader.AddressOfEntryPoint = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_ADDRESSOFENTRYPOINT,&bRet);
			
			ntHdr.OptionalHeader.BaseOfCode = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_BASEOFCODE,&bRet);
			ntHdr.OptionalHeader.BaseOfData = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_BASEOFDATA,&bRet);
								
			ntHdr.OptionalHeader.CheckSum = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_CHECKSUM,&bRet);
									
			ntHdr.OptionalHeader.DllCharacteristics = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_DLLCHARATERRISTICS,&bRet);
										
			ntHdr.OptionalHeader.FileAlignment= (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_FILESLIGNMENT,&bRet);
											
			ntHdr.OptionalHeader.ImageBase = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_IMAGEBASE,&bRet);
												
			ntHdr.OptionalHeader.LoaderFlags = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_LOADFLAG,&bRet);
													
			ntHdr.OptionalHeader.Magic = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_MAGIC,&bRet);
														
			ntHdr.OptionalHeader.MajorImageVersion = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_MAJORIMAGEVERSION,&bRet);
															
			ntHdr.OptionalHeader.MajorLinkerVersion = (BYTE)GetStrToNum(hOptHdrDlg,IDC_EDIT_MAJORLINKER,&bRet);
																
			ntHdr.OptionalHeader.MajorOperatingSystemVersion = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_MAJOROPERATONVER,&bRet);
																
			ntHdr.OptionalHeader.MajorSubsystemVersion = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_MAJORSUBSYSTEMVERSION,&bRet);
																
			ntHdr.OptionalHeader.MinorImageVersion = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_MINORIMAGEVERSION,&bRet);
																			
			ntHdr.OptionalHeader.MinorLinkerVersion = (BYTE)GetStrToNum(hOptHdrDlg,IDC_EDIT_MINORLINKERVERSION,&bRet);
																
			ntHdr.OptionalHeader.MinorOperatingSystemVersion = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_MINOROPERATIONVER,&bRet);
																				
			ntHdr.OptionalHeader.MinorSubsystemVersion = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_MINORSUBSYSTEMVERSION,&bRet);
																						
			ntHdr.OptionalHeader.NumberOfRvaAndSizes = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_NUMOFRVAANDSIZE,&bRet);
																							
			ntHdr.OptionalHeader.SectionAlignment = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SECTIONALIGNMENT,&bRet);
																								
			ntHdr.OptionalHeader.SizeOfCode = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEODCODE,&bRet);
																									
			ntHdr.OptionalHeader.SizeOfHeaders = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEOFHEADERS,&bRet);
																										
			ntHdr.OptionalHeader.SizeOfHeapCommit = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEOFHEAPCOMMIT,&bRet);
																											
			ntHdr.OptionalHeader.SizeOfHeapReserve= (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEOFHEAPRESERVE,&bRet);
																												
			ntHdr.OptionalHeader.SizeOfImage = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEOFIMAGE,&bRet);
			ntHdr.OptionalHeader.SizeOfInitializedData = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEOFINILIZEDATA,&bRet);
																											
			ntHdr.OptionalHeader.SizeOfStackCommit = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEOFSTACKCOMMIT,&bRet);
																														
			ntHdr.OptionalHeader.SizeOfStackReserve = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEOFSTACKRESERVE,&bRet);
																															
			ntHdr.OptionalHeader.SizeOfUninitializedData = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SIZEOFUNINILIZEDDATA,&bRet);
																																	
			ntHdr.OptionalHeader.Subsystem = (WORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_SUBSYSTEM,&bRet);
																																
			ntHdr.OptionalHeader.Win32VersionValue = (DWORD)GetStrToNum(hOptHdrDlg,IDC_EDIT_WIN32VERSIONVALUE,&bRet);
																																	
			hFile = CreateFile(szFileName,FILE_ALL_ACCESS,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			dwPtr = SetFilePointer(hFile,dosHdr.e_lfanew + sizeof(ntHdr.Signature) + sizeof(ntHdr.FileHeader),NULL,FILE_BEGIN);
			if (dwPtr != INVALID_SET_FILE_POINTER)
			{
					WriteFile(hFile,&ntHdr.OptionalHeader,sizeof(ntHdr.OptionalHeader),&dwNumOfBytesWritten,NULL);
			}	
			else
			{
			ShowError();
			}
			CloseHandle(hFile);
			if (!EndDialog(hOptHdrDlg,0))
			{
				ShowError();
				ExitProcess(0);
			}
			break;
			case IDC_COMBO_DATADIR:
					switch (HIWORD(wParam))
					{
					case CBN_SELCHANGE:
						Index = SendDlgItemMessage(hOptHdrDlg,IDC_COMBO_DATADIR,CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
						if (Index != CB_ERR)
						{
							SetEditText(hOptHdrDlg,IDC_EDIT_VIRTUALADDRESS,ntHdr.OptionalHeader.DataDirectory[Index].VirtualAddress);
							SetEditText(hOptHdrDlg,IDC_EDIT_SIZE,ntHdr.OptionalHeader.DataDirectory[Index].Size);
							break;
						}
						ShowError();
						break;
					}
					break;
		}
		break;
		case WM_CLOSE:
			if (!EndDialog(hOptHdrDlg,0))
			{
				ShowError();
				ExitProcess(0);
			}
			break;

	}
	return 0;
}



									

												

