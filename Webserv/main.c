#include "resource/resource.h"
#include "sock.h"
#include "logic.h"
#include "union.h"
#include "reg.h"
#include "res.h"
#include "unit.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlobj.h>

_sgv Sgv;

INT_PTR CALLBACK
DlgProc(
	HWND		hDlg,
	UINT		message,
	WPARAM		wParam,
	LPARAM		lParam
);

char*
HTTP_MIME(
	const char* extensions
);

BOOLEAN
BrowserFolder(
	HWND		hWndOwner,
	LPCSTR		DialogTitle,
	LPSTR		ResultPath,
	size_t		PathLen
);

int APIENTRY 
WinMain(
	_In_ HINSTANCE		hInstance,
	_In_opt_ HINSTANCE	hPrevInstance,
	_In_ LPSTR			lpCmdLine,
	_In_ int			nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    return FALSE;
}

INT_PTR CALLBACK 
DlgProc(
	HWND		hDlg, 
	UINT		message, 
	WPARAM		wParam, 
	LPARAM		lParam
)
{
	static SOCKET ServerSock = INVALID_SOCKET;

    switch (message)
    {
		case WM_INITDIALOG:
		{
			Sgv._hDlg = hDlg;

			HWND hChildWnd = FindWindowEx(GetDlgItem(hDlg, IDC_SERVERIP), NULL, "Edit", NULL);

			if (hChildWnd)
				SendMessage(hChildWnd, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);

			// 레지스트리 불러오기
			CHAR Buf[256];

			memset(Buf, 0, sizeof(Buf));
			ReadReg(SERVER_NAME, "SERVERIP", "", Buf);
			GetHostIP(GetDlgItem(hDlg, IDC_SERVERIP), Buf);

			memset(Buf, 0, sizeof(Buf));
			ReadReg(SERVER_NAME, "PORT", "80", Buf);
			SendDlgItemMessage(hDlg, IDC_PORT, WM_SETTEXT, (WPARAM)0x02, (LPARAM)Buf);

			memset(Buf, 0, sizeof(Buf));
			ReadReg(SERVER_NAME, "UPLOADTIMEOUT", "6000", Buf);
			SendDlgItemMessage(hDlg, IDC_UPLOADTIMEOUT, WM_SETTEXT, (WPARAM)0x04, (LPARAM)Buf);

			memset(Buf, 0, sizeof(Buf));
			ReadReg(SERVER_NAME, "DOWNLOADTIMEOUT", "6000", Buf);
			SendDlgItemMessage(hDlg, IDC_DOWNLOADTIMEOUT, WM_SETTEXT, (WPARAM)0x04, (LPARAM)Buf);

			memset(Buf, 0, sizeof(Buf));
			ReadReg(SERVER_NAME, "UPLOAD_PATH", "", Buf);
			SendDlgItemMessage(hDlg, IDC_UPLOADPATH, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)Buf);

			memset(Buf, 0, sizeof(Buf));
			ReadReg(SERVER_NAME, "DOWNLOAD_PATH", "", Buf);
			SendDlgItemMessage(hDlg, IDC_DOWNLOADPATH, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)Buf);

			memset(Buf, 0, sizeof(Buf));
			ReadReg(SERVER_NAME, "ISUPLOAD", "0", Buf);
			SendDlgItemMessage(hDlg, IDC_ENALBEUPLOAD, BM_SETCHECK, (WPARAM)*Buf == '1', 0);

			EnableWindow(GetDlgItem(hDlg, IDC_UPLOADPATH), *Buf == '1');
			EnableWindow(GetDlgItem(hDlg, IDC_BROWSEUPPATH), *Buf == '1');

			memset(Buf, 0, sizeof(Buf));
			ReadReg(SERVER_NAME, "ISCHAT", "0", Buf);
			SendDlgItemMessage(hDlg, IDC_ENALBECHAT, BM_SETCHECK, (WPARAM)*Buf == '1', 0);

			if (*Sgv.SERVER_IP)
				SendDlgItemMessage(hDlg, IDC_SERVERIP, WM_SETTEXT, (WPARAM)sizeof(Sgv.SERVER_IP), (LPARAM)Sgv.SERVER_IP);

			if (*Sgv.UPLOAD_PATH)
			{
				SendDlgItemMessage(hDlg, IDC_UPLOADPATH, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)Sgv.UPLOAD_PATH);
				SendDlgItemMessage(hDlg, IDC_ENALBEUPLOAD, BM_SETCHECK, (WPARAM)TRUE, 0);
			}

			if (*Sgv.DOWNLOAD_PATH)
			{
				SendDlgItemMessage(hDlg, IDC_DOWNLOADPATH, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)Sgv.DOWNLOAD_PATH);
				SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_WEBSERVSTART, BN_CLICKED), 0);
				ShowWindow(hDlg, SW_HIDE);
			}

			break;
		}
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
					case IDC_WEBSERVSTART:
					{
						DWORD	send_timeout;
						DWORD	recv_timeout;
						DWORD	checkPath;

						if (Sgv.bStartServ = !(TRUE && Sgv.bStartServ))
						{
							send_timeout = (DWORD)GetDlgItemInt(hDlg, IDC_DOWNLOADTIMEOUT, NULL, FALSE);
							recv_timeout = (DWORD)GetDlgItemInt(hDlg, IDC_UPLOADTIMEOUT, NULL, FALSE);
							SendDlgItemMessage(hDlg, IDC_SERVERIP, WM_GETTEXT, (WPARAM)sizeof(Sgv.SERVER_IP), (LPARAM)Sgv.SERVER_IP);

							if (0 == *Sgv.SERVER_IP)
							{
								MessageBox(hDlg, "Please select an IP address.", "Alert", MB_ICONEXCLAMATION);
								Sgv.bStartServ = FALSE;
								return (INT_PTR)FALSE;
							}

							Sgv.UpChat = (IsDlgButtonChecked(hDlg, IDC_ENALBEUPLOAD) == BST_CHECKED) |
								((IsDlgButtonChecked(hDlg, IDC_ENALBECHAT) == BST_CHECKED) << 1);

							if (Sgv.UpChat & 0x01)
							{
								SendDlgItemMessage(hDlg, IDC_UPLOADPATH, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)Sgv.UPLOAD_PATH);

								if (INVALID_FILE_ATTRIBUTES == (checkPath = GetFileAttributes(Sgv.UPLOAD_PATH)) ||
									FALSE == (FILE_ATTRIBUTE_DIRECTORY & checkPath))
								{
									MessageBox(hDlg, "This is not a valid upload path.", "Alert", MB_ICONEXCLAMATION);
									Sgv.bStartServ = FALSE;
									return (INT_PTR)FALSE;
								}
							}

							SendDlgItemMessage(hDlg, IDC_DOWNLOADPATH, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)Sgv.DOWNLOAD_PATH);

							if (INVALID_FILE_ATTRIBUTES == (checkPath = GetFileAttributes(Sgv.DOWNLOAD_PATH)) ||
								FALSE == (FILE_ATTRIBUTE_DIRECTORY & checkPath))
							{
								MessageBox(hDlg, "This is not a valid download path.", "Alert", MB_ICONEXCLAMATION);
								Sgv.bStartServ = FALSE;
								return (INT_PTR)FALSE;
							}

							Sgv.SERVER_PORT = (WORD)GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);

							if (0 == Sgv.SERVER_PORT)
							{
								MessageBox(hDlg, "The range of port addresses is 1 to 65535.", "Alert", MB_ICONEXCLAMATION);
								Sgv.bStartServ = FALSE;
								return (INT_PTR)FALSE;
							}

							// 레지스트리 저장
							CHAR Buf[255];

							GetDlgItemText(hDlg, IDC_SERVERIP, Buf, sizeof(Buf));
							WriteReg(SERVER_NAME, "SERVERIP", Buf);

							GetDlgItemText(hDlg, IDC_PORT, Buf, sizeof(Buf));
							WriteReg(SERVER_NAME, "PORT", Buf);
						
							GetDlgItemText(hDlg, IDC_UPLOADTIMEOUT, Buf, sizeof(Buf));
							WriteReg(SERVER_NAME, "UPLOADTIMEOUT", Buf);

							GetDlgItemText(hDlg, IDC_DOWNLOADTIMEOUT, Buf, sizeof(Buf));
							WriteReg(SERVER_NAME, "DOWNLOADTIMEOUT", Buf);

							GetDlgItemText(hDlg, IDC_UPLOADPATH, Buf, sizeof(Buf));
							WriteReg(SERVER_NAME, "UPLOAD_PATH", Buf);

							GetDlgItemText(hDlg, IDC_DOWNLOADPATH, Buf, sizeof(Buf));
							WriteReg(SERVER_NAME, "DOWNLOAD_PATH", Buf);

							Buf[1] = 0;

							Buf[0] = IsDlgButtonChecked(hDlg, IDC_ENALBEUPLOAD) ? '1' : '0';
							WriteReg(SERVER_NAME, "ISUPLOAD", Buf);

							Buf[0] = IsDlgButtonChecked(hDlg, IDC_ENALBECHAT) ? '1' : '0';
							WriteReg(SERVER_NAME, "ISCHAT", Buf);

							// 서버 시작
							if (INVALID_SOCKET == (ServerSock = InitializeSocket()))
							{
								MessageBox(hDlg, "Server initialization failed!", "Error", MB_ICONERROR);
								Sgv.bStartServ = FALSE;
								return (INT_PTR)FALSE;
							}

							// 리소스 설정
							GetResourcesFile((void*)0, 0);
							HTTP_MIME((void*)0);

							if (FALSE == StartWebServer(ServerSock, Sgv.SERVER_PORT, send_timeout, recv_timeout))
							{
								MessageBox(hDlg, "Server initialization failed!", "Error", MB_ICONERROR);
								Sgv.bStartServ = FALSE;
							}
						}

						if (FALSE == Sgv.bStartServ)
						{
							StopWebServer(ServerSock);
							ServerSock = INVALID_SOCKET;

							// 리소스 해제
							GetResourcesFile(((void*)-1), 0);
							HTTP_MIME(((void*)-1));
						}

						SendDlgItemMessage(hDlg, IDC_WEBSERVSTART, WM_SETTEXT, (WPARAM)5, (LPARAM)(Sgv.bStartServ ? "Stop" : "Start"));

						for (DWORD Index = IDC_SERVERIP; Index <= IDC_PORT; ++Index)
							EnableWindow(GetDlgItem(hDlg, Index), !Sgv.bStartServ);

						EnableWindow(GetDlgItem(hDlg, IDC_UPLOADPATH), Sgv.UpChat & 0x01);
						EnableWindow(GetDlgItem(hDlg, IDC_BROWSEUPPATH), Sgv.UpChat & 0x01);

						break;
					}
					case IDC_BROWSEUPPATH:
					{
						if (BrowserFolder(hDlg, "Please select a folder.", Sgv.UPLOAD_PATH, MAX_PATH))
							SendDlgItemMessage(hDlg, IDC_UPLOADPATH, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)Sgv.UPLOAD_PATH);

						break;
					}
					case IDC_BROWSEDOWNPATH:
					{
						if (BrowserFolder(hDlg, "Please select a folder.", Sgv.DOWNLOAD_PATH, MAX_PATH))
							SendDlgItemMessage(hDlg, IDC_DOWNLOADPATH, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)Sgv.DOWNLOAD_PATH);

						break;
					}
					case IDC_ENALBEUPLOAD:
					{
						BOOLEAN bEnable;

						bEnable = BST_CHECKED == SendDlgItemMessage(hDlg, IDC_ENALBEUPLOAD, 
							BM_GETCHECK, (WPARAM)NULL, (LPARAM)NULL);

						EnableWindow(GetDlgItem(hDlg, IDC_UPLOADPATH), bEnable);
						EnableWindow(GetDlgItem(hDlg, IDC_BROWSEUPPATH), bEnable);

						break;
					}
				}
			}

			break;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			break;
		}
    }

    return (INT_PTR)FALSE;
}

INT CALLBACK
BrowseCallbackProc(
	HWND hwnd,
	UINT uMsg,
	LPARAM lParam,
	LPARAM lpData
)
{
	if (uMsg == BFFM_INITIALIZED)
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpData);

	return 0;
}

BOOLEAN
BrowserFolder(
	HWND		hWndOwner,
	LPCSTR		DialogTitle,
	LPSTR		ResultPath,
	size_t		PathLen
)
{
	LPITEMIDLIST	pidlBrowse;
	BROWSEINFO		brInfo = { 0 };

	DWORD attributes = GetFileAttributes(ResultPath);

	if (INVALID_FILE_ATTRIBUTES != attributes && 
		0 == (attributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		char* ptr = strrchr(ResultPath, '\\');

		if (ptr)
			*ptr = 0;
	}

	brInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_VALIDATE;
	brInfo.hwndOwner = hWndOwner;
	brInfo.pszDisplayName = ResultPath;
	brInfo.lpszTitle = DialogTitle;
	brInfo.lpfn = BrowseCallbackProc;
	brInfo.lParam = (LPARAM)ResultPath;

	if (pidlBrowse = SHBrowseForFolder(&brInfo))
	{
		SHGetPathFromIDList(pidlBrowse, ResultPath);
		return TRUE;
	}

	return FALSE;
}