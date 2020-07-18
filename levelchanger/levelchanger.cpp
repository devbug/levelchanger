/* 
 * 
 *  levelchanger.cpp
 *  levelchanger's main program
 *  
 *  
 *  levelchanger
 *  Copyright (C) 2010-2020  deVbug (devbug@devbug.me)
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License.
 *   
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 */

// levelchanger.cpp : ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "levelchanger.h"

#define MAX_LOADSTRING 100
#define MAX_BUFFER		2048

// ���� ����:
HINSTANCE hInst;								// ���� �ν��Ͻ��Դϴ�.
TCHAR szTitle[MAX_LOADSTRING];					// ���� ǥ���� �ؽ�Ʈ�Դϴ�.
TCHAR szWindowClass[MAX_LOADSTRING];			// �⺻ â Ŭ���� �̸��Դϴ�.
HWND hWndMain;

// �� �ڵ� ��⿡ ��� �ִ� �Լ��� ������ �����Դϴ�.
INT_PTR CALLBACK DlgProg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL GetFileName(HWND hPathCtrl, TCHAR lpszFilter[], DWORD dwFlags);
BOOL Is64OSVersion();
void ErrorCatcher(LPCTSTR lpszFunction, BOOL bForce);
INT32 MsgBox(HWND hWnd, UINT uType, LPCTSTR lpCaption, LPCTSTR format, ... );
VOID DebugString(LPCTSTR lpFormat, ...);


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: ���⿡ �ڵ带 �Է��մϴ�.

	// ���� ���ڿ��� �ʱ�ȭ�մϴ�.
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	hInst = hInstance; // �ν��Ͻ� �ڵ��� ���� ������ �����մϴ�.

	DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), HWND_DESKTOP, DlgProg);

	return 0;
}



typedef struct _stfilesign {
	unsigned char data00;
	unsigned char data01;
	unsigned char data02;
	unsigned char data03;
} stFileSign;

typedef union _ufilesign {
	stFileSign stfilesign;
	unsigned int dfilesign;
} uFileSign;

typedef struct _profile {
	unsigned char data00 : 8;
	unsigned char data01 : 8;
} stProfile;

typedef union _uprofile {
	stProfile stprofile;
	unsigned short shprofile : 16;
} uProfile;

typedef struct _h264info {
	unsigned char reserved : 8;
	uProfile profile; 
	unsigned char level : 8;
} stH264Info;

BOOL LoadInfo(LPCTSTR lpszPath);
BOOL LoadMP4(LPCTSTR lpszPath);
BOOL SetChange(LPCTSTR lpszPath);

// h264 raw, mkv, avi container signs
static uFileSign signs[] =  { { 0, 0, 0, 1 }, { 0x1A, 0x45, 0xDF, 0xA3 }, { 0x52, 0x49, 0x46, 0x46 } };
// baseline(42C0), baseline(42E0), main profile, extended profile, high profile
static uProfile profiles[] = { { 0x42, 0xC0 } , { 0x42, 0xE0 }, { 0x4D, 0x40 }, { 0x58, 0xA0 }, { 0x64, 0x00 } };
static LPCTSTR profile_names[] = { _T("Baseline(42C0) Profile"), _T("Baseline(42E0) Profile"), _T("Main Profile"), _T("Extended Profile"), _T("High Profile") };
// levels
static LPCTSTR pchlevels[] = { _T("5.1"), _T("5.0"), _T("4.2"), _T("4.1"), _T("4.0"),
						_T("3.2"), _T("3.1"), _T("3.0"), _T("2.2"), _T("2.1"),
						_T("2.0"), _T("1.3"), _T("1.2"), _T("1.1"), _T("1.0") };
static int dlevels[] = { 51, 50, 42, 41, 40, 32, 31, 30, 22, 21, 20, 13, 12, 11, 10 };


// ��ȭ ������ �޽��� ó�����Դϴ�.
INT_PTR CALLBACK DlgProg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	TCHAR szPath[MAX_BUFFER+1];
	int i;
	HDROP hDrop;

	ZeroMemory(szPath, MAX_BUFFER+1);

	switch (message) {
		case WM_INITDIALOG:
			hWndMain = hDlg;

			DragAcceptFiles(hWndMain, TRUE);

			SetDlgItemText(hDlg, IDC_STO, _T("\r\nH.264 / x264 / AVC �������� ��������, ���� ���� �ٲߴϴ�.\r\nmp4 �����̳� Ȥ�� H.264 RAW �����͸� �ٲ� �� �ֽ��ϴ�.\r\n\r\nby deVbug ( http://blog.devbug.me )"));

			for (i=0;i<_countof(profile_names);i++)
				ComboBox_AddString(GetDlgItem(hDlg, IDC_CMBPROFILE), profile_names[i]);
			
			for (i=0;i<_countof(pchlevels);i++)
				ComboBox_AddString(GetDlgItem(hDlg, IDC_CMBLEVEL), pchlevels[i]);

			SetDlgItemText(hDlg, IDC_STPROFILE, _T(""));
			SetDlgItemText(hDlg, IDC_STLEVEL, _T(""));
			SetDlgItemText(hDlg, IDC_STHEX, _T(""));

			EnableWindow(GetDlgItem(hWndMain, IDC_CMBPROFILE), FALSE);
			EnableWindow(GetDlgItem(hWndMain, IDC_CMBLEVEL), FALSE);
			EnableWindow(GetDlgItem(hWndMain, IDC_BTCHANGE), FALSE);

			SetFocus(GetDlgItem(hDlg, IDC_BTBROWSE));

			return (INT_PTR)TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					return (INT_PTR)TRUE;

				case IDC_BTBROWSE:
					if (FALSE == GetFileName(GetDlgItem(hDlg, IDC_EDPATH), TEXT("MP4 File\0*.mp4\0H.264 RAW File\0*.h264\0All Files\0*.*\0\0"),
								OFN_FILEMUSTEXIST | OFN_EXPLORER))
						break;
					Edit_SetSel(GetDlgItem(hDlg, IDC_EDPATH), MAXLONG, MAXLONG);

					GetDlgItemText(hDlg, IDC_EDPATH, szPath, MAX_BUFFER);

					LoadInfo(szPath);

					break;

				case IDC_BTCHANGE:
					GetDlgItemText(hDlg, IDC_EDPATH, szPath, MAX_BUFFER);

					SetChange(szPath);

					break;
			}
			break;

		case WM_DROPFILES:
			hDrop = (HDROP)wParam;

			// ���� ������ ���Ѵ�. ( 2�� �Ķ���� -1 = ����)
			i = DragQueryFile(hDrop, -1, 0, 0);
			if (i == 1) {
				DragQueryFile( hDrop, 0, szPath , MAX_BUFFER );
				
				SetDlgItemText(hDlg, IDC_EDPATH, szPath);

				LoadInfo(szPath);

				SetForegroundWindow(hDlg);
			}
			break;

		case WM_DESTROY:
			break;
	}
	return (INT_PTR)FALSE;
}

#define IS_UNK	0x101000
#define IS_MP4	0x101010
#define IS_H264	0x101011
#define IS_MKV	0x101012
#define IS_AVI	0x101013
static UINT64 offset;
static stH264Info info;
static int ffiletype;

BOOL LoadInfo(LPCTSTR lpszPath)
{
	uFileSign filesign;
	FILE *fp = NULL;
	BOOL rtn = TRUE;
	TCHAR temp[1000];
	unsigned char buf[1000];

	ZeroMemory(temp, 1000);
	ZeroMemory(buf, 1000);

	offset = 0;

	_tfopen_s(&fp, lpszPath, _T("rb"));
	if (fp == NULL) return FALSE;

	fread(&filesign, sizeof(stFileSign), 1, fp);

	fclose(fp);

	// h264
	if (filesign.dfilesign == signs[0].dfilesign) {
		SetDlgItemText(hWndMain, IDC_STINFO, _T("H.264 RAW File"));
		ffiletype = IS_H264;
		_tfopen_s(&fp, lpszPath, _T("rb"));
		if (fp){
			_fseeki64(fp, 4, 0);
			fread(&info, sizeof(stH264Info), 1, fp);
			fclose(fp);
			offset = sizeof(stFileSign);
		} else {
			rtn = FALSE;
			SetDlgItemText(hWndMain, IDC_STINFO, _T("H.264 RAW File :: open error!"));
		}
	} 
	// mkv
	else if (filesign.dfilesign == signs[1].dfilesign) {
		SetDlgItemText(hWndMain, IDC_STINFO, _T("Matroska(MKV) file"));
		rtn = FALSE;
		ffiletype = IS_MKV;
	} 
	// avi
	else if (filesign.dfilesign == signs[2].dfilesign) {
		SetDlgItemText(hWndMain, IDC_STINFO, _T("Microsoft AVI file"));
		rtn = FALSE;
		ffiletype = IS_AVI;
	} // mp4
	else {
		int flag;
		flag = LoadMP4(lpszPath);

		if (!flag) {
			rtn = FALSE;
			SetDlgItemText(hWndMain, IDC_STINFO, _T("file type not supported"));
			ffiletype = IS_UNK;
		} else {
			SetDlgItemText(hWndMain, IDC_STINFO, _T("MPEG4 mp4 file"));
			ffiletype = IS_MP4;
		}
	}

	if (rtn) {
		SetDlgItemText(hWndMain, IDC_STPROFILE, _T("Unknown Profile"));
		ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE), -1);

		for (int i=0;i<_countof(profiles);i++) {
			if (info.profile.shprofile == profiles[i].shprofile) {
				SetDlgItemText(hWndMain, IDC_STPROFILE, profile_names[i]);
				ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE), i);
				break;
			}
		}

		ZeroMemory(temp, 1000);
		_stprintf_s(temp, 1000, _T("%.1f"), info.level/10.0);
		
		ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBLEVEL), -1);

		for (int i=0;i<_countof(dlevels);i++) {
			if (dlevels[i] == info.level) {
				ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBLEVEL), i);
				break;
			}
		}

		SetDlgItemText(hWndMain, IDC_STLEVEL, temp);

		_stprintf_s(temp, 1000, _T("0x%02X%02X%02X%02X"), info.reserved, info.profile.stprofile.data00, info.profile.stprofile.data01, info.level);
		SetDlgItemText(hWndMain, IDC_STHEX, temp);

		EnableWindow(GetDlgItem(hWndMain, IDC_CMBPROFILE), (ffiletype == IS_H264 ? FALSE : TRUE));
		EnableWindow(GetDlgItem(hWndMain, IDC_CMBLEVEL), TRUE);
		EnableWindow(GetDlgItem(hWndMain, IDC_BTCHANGE), TRUE);
	} else {
		SetDlgItemText(hWndMain, IDC_STPROFILE, _T("ERROR!"));
		SetDlgItemText(hWndMain, IDC_STLEVEL, _T("ERROR!"));
		SetDlgItemText(hWndMain, IDC_STHEX, _T(""));
		ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE), -1);
		ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBLEVEL), -1);
		EnableWindow(GetDlgItem(hWndMain, IDC_CMBPROFILE), FALSE);
		EnableWindow(GetDlgItem(hWndMain, IDC_CMBLEVEL), FALSE);
		EnableWindow(GetDlgItem(hWndMain, IDC_BTCHANGE), FALSE);
	}

	return rtn;
}


#define MP4AVCSIGN	_T("avcC")	//0x61766343
#define MP4METASIGN	0x6d6f6f76	//_T("moov")
#define MP4TYPESIGN 0x66747970	//_T("ftyp")

typedef struct fourbyte {
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
	unsigned char byte4;
} stFourByte;
typedef struct eightbyte {
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
	unsigned char byte4;
	unsigned char byte5;
	unsigned char byte6;
	unsigned char byte7;
	unsigned char byte8;
} stEightByte;
typedef union _ufourbyte {
	stFourByte bytes;
	unsigned int ints;
} uFourByte;
typedef union _ueightbyte {
	stEightByte bytes;
	UINT64 ints; 
} uEightByte;
typedef struct _mp4box {
	uFourByte size;
	uFourByte type;
	uEightByte largesize;
} stMP4BOX;

BOOL isLittleEndian()
{
	uFourByte test;

	test.ints = 1;

	if (test.bytes.byte1 == 1) return TRUE;

	return FALSE;
}

VOID swap(unsigned char *a, unsigned char *b)
{
	unsigned char t;
	t = *a;
	*a = *b;
	*b = t;
}

VOID makeLE(stMP4BOX *box)
{
	swap(&box->size.bytes.byte1, &box->size.bytes.byte4);
	swap(&box->size.bytes.byte2, &box->size.bytes.byte3);
	swap(&box->type.bytes.byte1, &box->type.bytes.byte4);
	swap(&box->type.bytes.byte2, &box->type.bytes.byte3);
	swap(&box->largesize.bytes.byte1, &box->largesize.bytes.byte8);
	swap(&box->largesize.bytes.byte2, &box->largesize.bytes.byte7);
	swap(&box->largesize.bytes.byte3, &box->largesize.bytes.byte6);
	swap(&box->largesize.bytes.byte4, &box->largesize.bytes.byte5);
}

BOOL LoadMP4(LPCTSTR lpszPath)
{
	FILE *fp = NULL;
	stMP4BOX box;
	BOOL isMP4 = FALSE, isH264 = TRUE;
	//ULARGE_INTEGER off;
	UINT64 off = 0;
	int readsize = 0;
	BYTE *buf = NULL;

	_tfopen_s(&fp, lpszPath, _T("rb"));
	if (fp == NULL) return FALSE;

	while (1) {
		readsize = fread(&box, sizeof(stMP4BOX), 1, fp);
		
		if (readsize == 0) break;

		if (isLittleEndian()){
			makeLE(&box);
		}

		if (!isMP4 && box.type.ints == MP4TYPESIGN) isMP4 = TRUE;
		if (box.type.ints == MP4METASIGN) {
			if (buf) free(buf);
			buf = (BYTE *)malloc(box.size.ints);
			_fseeki64(fp, off, 0);
			fread(buf, box.size.ints, 1, fp);

			size_t i, j;
			BOOL flag = FALSE;
			for (i=0;i<box.size.ints;i++){
				flag = TRUE;
				for (j=0;j<4;j++){
					if (MP4AVCSIGN[j] != buf[i+j]){
						flag = FALSE;
						break;
					}
				}

				i += j;

				if (flag) break;
			}

			if (flag) {
				memcpy(&info, buf+i, sizeof(stH264Info));
				offset = off + i;
			}

			if (buf) free(buf);
			if (!flag){
				isH264 = FALSE;
				break;
			}
		} else {
			// largesize
			if (box.size.ints == 1) {
				off += box.largesize.ints;
			}
			// ������, �� ������ �ڽ�
			else if (box.size.ints == 0) {
				break;
			} else {
				off += box.size.ints;
			}

			_fseeki64(fp, off, 0);
		}
	}

	fclose(fp);

	if (!isMP4) return FALSE;
	if (!isH264) return FALSE;

	return TRUE;
}

BOOL SetChange(LPCTSTR lpszPath)
{
	int dProfile, dLevel;
	FILE *fp = NULL;
	
	dProfile = ComboBox_GetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE));
	dLevel = ComboBox_GetCurSel(GetDlgItem(hWndMain, IDC_CMBLEVEL));

	if (dProfile == -1 || dLevel == -1) {
		int rtn = MsgBox(hWndMain, MB_YESNOCANCEL, _T("Error!"), _T("���� �ùٷ� ���õ��� �ʾҽ��ϴ�. �״�� �����Ͽ� ���� �Ͻðڽ��ϱ�?"));

		if (rtn == IDNO || rtn == IDCANCEL) {
			if (rtn == IDNO)
				MsgBox(hWndMain, 0, _T("Information"), _T("���� �ٽ� �����ϼ���."));

			if (dProfile == -1) SetFocus(GetDlgItem(hWndMain, IDC_CMBPROFILE));
			else SetFocus(GetDlgItem(hWndMain, IDC_CMBLEVEL));

			return FALSE;
		}
	}

	if (dProfile != -1)
		info.profile.shprofile = profiles[dProfile].shprofile;
	
	if (0 <= dLevel && dLevel < _countof(dlevels))
		info.level = dlevels[dLevel];

	_tfopen_s(&fp, lpszPath, _T("r+b"));
	if (fp == NULL) return FALSE;

	_fseeki64(fp, offset, 0L);
	fwrite(&info, sizeof(stH264Info), 1, fp);

	fclose(fp);

	MsgBox(hWndMain, 0, _T("Complete!"), _T("��� �۾��� �Ϸ��Ͽ����ϴ�.\r\n������ �ٽ� �ε��մϴ�."));

	LoadInfo(lpszPath);

	return TRUE;
}

BOOL GetFileName(HWND hPathCtrl, TCHAR lpszFilter[], DWORD dwFlags)
{
	OPENFILENAME OFN;
	TCHAR szPath[MAX_BUFFER+1];
	static DWORD nFilterIndex = 1;

	ZeroMemory(szPath, MAX_BUFFER+1);

	GetWindowText(hPathCtrl, szPath, MAX_BUFFER);

	memset(&OFN, 0, sizeof(OFN));
	OFN.lStructSize = sizeof(OFN);
	OFN.hwndOwner = GetParent(hPathCtrl);
	OFN.lpstrFilter = lpszFilter;
	OFN.lpstrFile = szPath;
	OFN.nMaxFile = MAX_BUFFER;
	OFN.nFilterIndex = nFilterIndex;
	OFN.Flags = dwFlags;
	OFN.lpstrInitialDir = TEXT(".");

	if (GetOpenFileName(&OFN) == 0)
		return FALSE;

	nFilterIndex = OFN.nFilterIndex;
	SetWindowText(hPathCtrl, OFN.lpstrFile);

	return TRUE;
}

BOOL Is64OSVersion()
{
	BOOL bResult = FALSE;
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	SYSTEM_INFO si;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 5;
	osvi.dwMinorVersion = 2;
	// ref: https://en.wikipedia.org/wiki/List_of_Microsoft_Windows_versions

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

	GetNativeSystemInfo(&si);

	if (VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)) {
		//64��Ʈ�� �����ϴ� CPU�� 32��Ʈ �ü���� ��ġ�Ǹ� 32��Ʈ CPU��� ����
		if (PROCESSOR_ARCHITECTURE_IA64 == si.wProcessorArchitecture
			|| PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture) {
			bResult = TRUE;
		}
	}

	return bResult;
}

void ErrorCatcher(LPCTSTR lpszFunction, BOOL bForce)
{ 
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();

	if (!dw && !bForce)
		return;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	MsgBox(NULL, 0, TEXT("Error"), TEXT("%s �� ����: �����ڵ� %d, %s"), lpszFunction, dw, lpMsgBuf);

	LocalFree(lpMsgBuf);
}

INT32 MsgBox(HWND hWnd, UINT uType, LPCTSTR lpCaption, LPCTSTR format, ... )
{
	TCHAR	String[MAX_PATH];
	va_list argptr;

	va_start( argptr, format );
	wvsprintf( String, format, argptr );
	va_end( argptr );

	return MessageBox( hWnd, String, lpCaption, uType);
}

VOID DebugString(LPCTSTR lpFormat, ...)
{
	TCHAR	buf[MAX_PATH];
	va_list argptr;

	va_start( argptr, lpFormat );
	wvsprintf( buf, lpFormat, argptr );
	va_end( argptr );

	OutputDebugString(buf);
}

