#include <windows.h>
#include "resource.h"

extern IMAGE_DOS_HEADER	dosHdr;
extern TCHAR	szFileName[1024];
extern HINSTANCE hAppHwnd;

void ShowError();
BOOL SetEditText(HWND hWnd,int nDlgItemID,DWORD dwNumToSet);
DWORD GetStrToNum(HWND hDlg,int nIDDlgItem,BOOL* bRet);

LRESULT CALLBACK DosHdrDlg(HWND hDosHdrDlg, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK DosHdrDlg(HWND hDosHdrDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HICON hIcon;
	HANDLE hFile;
	DWORD dwPtr = 0;
	DWORD dwNumOfBytesWritten = 0;
	BOOL bRet;

	switch (message)
	{
	case WM_INITDIALOG:
		//load icon
		hIcon = LoadIcon(hAppHwnd,(LPCTSTR)IDI_ICON_PE);
		SendMessage(hDosHdrDlg,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
		//set the information
		SetEditText(hDosHdrDlg,IDC_EDIT_CBLP,dosHdr.e_cblp);
		SetEditText(hDosHdrDlg,IDC_EDIT_CP,dosHdr.e_cp);
		SetEditText(hDosHdrDlg,IDC_EDIT_CPARHDR,dosHdr.e_cparhdr);
		SetEditText(hDosHdrDlg,IDC_EDIT_CRLC,dosHdr.e_crlc);
		SetEditText(hDosHdrDlg,IDC_EDIT_CS,dosHdr.e_cs);
		SetEditText(hDosHdrDlg,IDC_EDIT_CSUM,dosHdr.e_csum);
		SetEditText(hDosHdrDlg,IDC_EDIT_IP,dosHdr.e_ip);
		SetEditText(hDosHdrDlg,IDC_EDIT_LFANEW,dosHdr.e_lfanew);
		SetEditText(hDosHdrDlg,IDC_EDIT_LFARLC,dosHdr.e_lfarlc);
		SetEditText(hDosHdrDlg,IDC_EDIT_MAGIC,dosHdr.e_magic);
		SetEditText(hDosHdrDlg,IDC_EDIT_MAXALLOC,dosHdr.e_maxalloc);
		SetEditText(hDosHdrDlg,IDC_EDIT_MINALLOC,dosHdr.e_minalloc);
		SetEditText(hDosHdrDlg,IDC_EDIT_OEMID,dosHdr.e_oemid);
		SetEditText(hDosHdrDlg,IDC_EDIT_OEMINFO,dosHdr.e_oeminfo);
		SetEditText(hDosHdrDlg,IDC_EDIT_OVNO,dosHdr.e_ovno);
		SetEditText(hDosHdrDlg,IDC_EDIT_SP,dosHdr.e_sp);
		SetEditText(hDosHdrDlg,IDC_EDIT_SS,dosHdr.e_ss);
/*
		//change the static lable font color
		//hWnd = GetDlgItem(hDosHdrDlg,IDC_STATIC_DOSHDR_NOTE);
		hWnd = hDosHdrDlg;
		if (!GetClientRect(hWnd,&rtRange))
		{
			ShowError();
			EndDialog(hDosHdrDlg,0);
		//}
		//hdc = GetDC(hWnd);
		//SetBkMode(hdc, TRANSPARENT);
		//SetTextColor(hdc,RGB(255,0,0));
		//BeginPath(hdc);
		//DrawText(hdc,text,sizeof(text),&rtRange,DT_CENTER);
		//EndPath(hdc);
		//ReleaseDC(hWnd, hdc); 
*/
		break;
	case WM_CLOSE:
		if (!EndDialog(hDosHdrDlg,0))
		{
			ShowError();
			ExitProcess(0);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_DOSHDR_OK:
			dosHdr.e_cblp = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_CBLP,&bRet);
			if (bRet)
			{
				dosHdr.e_cp = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_CP,&bRet);
				if (bRet)
				{
					dosHdr.e_cparhdr = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_CPARHDR,&bRet);
					if (bRet)
					{
						dosHdr.e_crlc = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_CRLC,&bRet);
						if (bRet)
						{
							dosHdr.e_cs = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_CS,&bRet);
							if (bRet)
							{
								dosHdr.e_csum = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_CSUM,&bRet);
								if (bRet)
								{
									dosHdr.e_ip = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_IP,&bRet);
									if (bRet)
									{
										dosHdr.e_lfanew = (LONG)GetStrToNum(hDosHdrDlg,IDC_EDIT_LFANEW,&bRet);
										if (bRet)
										{
											dosHdr.e_lfarlc = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_LFARLC,&bRet);
											if (bRet)
											{
												dosHdr.e_magic = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_MAGIC,&bRet);
												if (bRet)
												{
													dosHdr.e_maxalloc = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_MAXALLOC,&bRet);
													if (bRet)
													{
														dosHdr.e_minalloc = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_MINALLOC,&bRet);
														if (bRet)
														{
															dosHdr.e_oemid = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_OEMID,&bRet);
															if (bRet)
															{
																dosHdr.e_oeminfo = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_OEMINFO,&bRet);
																if (bRet)
																{
																	dosHdr.e_ovno = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_OVNO,&bRet);
																	if (bRet)
																	{
																		dosHdr.e_sp = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_SP,&bRet);
																		if (bRet)
																		{
																			dosHdr.e_ss = (WORD)GetStrToNum(hDosHdrDlg,IDC_EDIT_SS,&bRet);
																			if (bRet)
																			{
																				hFile = CreateFile(szFileName,FILE_ALL_ACCESS,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
																				WriteFile(hFile,&dosHdr,sizeof(dosHdr),&dwNumOfBytesWritten,NULL);
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}	
			ShowError();
			CloseHandle(hFile);
			if (!EndDialog(hDosHdrDlg,0))
			{
				ShowError();
				ExitProcess(0);
			}
			break;
		}
		break;
	}
	return 0;
}