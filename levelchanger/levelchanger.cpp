/* 
 * 
 *  levelchanger.cpp
 *  levelchanger's main program
 *  
 *  
 *  levelchanger
 *  Copyright (C) 2010  deVbug (devbug@devbug.me)
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

// levelchanger.cpp : 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "levelchanger.h"

#define MAX_LOADSTRING 100
#define MAX_BUFFER		2048

// 전역 변수:
HINSTANCE hInst;								// 현재 인스턴스입니다.
TCHAR szTitle[MAX_LOADSTRING];					// 제목 표시줄 텍스트입니다.
TCHAR szWindowClass[MAX_LOADSTRING];			// 기본 창 클래스 이름입니다.
HWND hWndMain;

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
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

 	// TODO: 여기에 코드를 입력합니다.

	// 전역 문자열을 초기화합니다.
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

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
// baseline, main profile, extended profile, high profile
static uProfile profiles[] = { { 0x42, 0xE0 }, { 0x4D, 0x40 }, { 0x58, 0xA0 }, { 0x64, 0x00 } };
// levels
#define NO_OF_LVL		15
static TCHAR *pchlevels[] = { _T("5.1"), _T("5.0"), _T("4.2"), _T("4.1"), _T("4.0"),
						_T("3.2"), _T("3.1"), _T("3.0"), _T("2.2"), _T("2.1"),
						_T("2.0"), _T("1.3"), _T("1.2"), _T("1.1"), _T("1.0") };
static int dlevels[] = { 51, 50, 42, 41, 40, 32, 31, 30, 22, 21, 20, 13, 12, 11, 10 };


// 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK DlgProg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	TCHAR szPath[MAX_BUFFER+1];
	int i;
	HDROP hDrop;

	ZeroMemory(szPath, MAX_BUFFER+1);

	switch (message)
	{
		case WM_INITDIALOG:
			hWndMain = hDlg;

			DragAcceptFiles(hWndMain, TRUE);

			SetDlgItemText(hDlg, IDC_STO, _T("\r\nH.264 / x264 / AVC 데이터의 프로파일, 레벨 값을 바꿉니다.\r\nmp4 컨테이너 혹은 H.264 RAW 데이터만 바꿀 수 있습니다.\r\n\r\nby deVbug ( http://devbug.me )"));

			ComboBox_AddString(GetDlgItem(hDlg, IDC_CMBPROFILE), _T("Baseline Profile"));
			ComboBox_AddString(GetDlgItem(hDlg, IDC_CMBPROFILE), _T("Main Profile"));
			ComboBox_AddString(GetDlgItem(hDlg, IDC_CMBPROFILE), _T("Extended Profile"));
			ComboBox_AddString(GetDlgItem(hDlg, IDC_CMBPROFILE), _T("High Profile"));
			
			for (i=0;i<NO_OF_LVL;i++)
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
			switch(LOWORD(wParam)){
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					return (INT_PTR)TRUE;

				case IDC_BTBROWSE:
					if(FALSE == GetFileName(GetDlgItem(hDlg, IDC_EDPATH), TEXT("MP4 File\0*.mp4\0H.264 RAW File\0*.h264\0All Files\0*.*\0\0"),
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

			// 먼저 갯수를 구한다. ( 2번 파라미터 -1 = 갯수)
			i = DragQueryFile(hDrop, -1, 0, 0);
			if(i == 1) {
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
	if(fp == NULL) return FALSE;

	fread(&filesign, sizeof(stFileSign), 1, fp);

	fclose(fp);

	// h264
	if (filesign.dfilesign == signs[0].dfilesign) {
		SetDlgItemText(hWndMain, IDC_STINFO, _T("H.264 RAW File"));
		ffiletype = IS_H264;
		_tfopen_s(&fp, lpszPath, _T("rb"));
		if(fp){
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
		if (info.profile.shprofile == profiles[0].shprofile) {
			SetDlgItemText(hWndMain, IDC_STPROFILE, _T("Baseline Profile"));
			ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE), 0);
		} else if (info.profile.shprofile == profiles[1].shprofile) {
			SetDlgItemText(hWndMain, IDC_STPROFILE, _T("Main Profile"));
			ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE), 1);
		} else if (info.profile.shprofile == profiles[2].shprofile) {
			SetDlgItemText(hWndMain, IDC_STPROFILE, _T("Extended Profile"));
			ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE), 2);
		} else if (info.profile.shprofile == profiles[3].shprofile) {
			SetDlgItemText(hWndMain, IDC_STPROFILE, _T("High Profile"));
			ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE), 3);
		} else {
			SetDlgItemText(hWndMain, IDC_STPROFILE, _T("Unknown Profile"));
			ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE), -1);
		}

		ZeroMemory(temp, 1000);
		_stprintf_s(temp, 1000, _T("%.1f"), info.level/10.0);
		
		int ind = 0;
		for(ind=0;ind<NO_OF_LVL;ind++)
			if(dlevels[ind] == info.level) break;
		if(ind >= NO_OF_LVL) ind = -1;

		ComboBox_SetCurSel(GetDlgItem(hWndMain, IDC_CMBLEVEL), ind);

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

	if(test.bytes.byte1 == 1) return TRUE;

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
	if(fp == NULL) return FALSE;

	while(1) {
		readsize = fread(&box, sizeof(stMP4BOX), 1, fp);
		
		if(readsize == 0) break;

		if(isLittleEndian()){
			makeLE(&box);
		}

		if(!isMP4 && box.type.ints == MP4TYPESIGN) isMP4 = TRUE;
		if(box.type.ints == MP4METASIGN) {
			if(buf) free(buf);
			buf = (BYTE *)malloc(box.size.ints);
			_fseeki64(fp, off, 0);
			fread(buf, box.size.ints, 1, fp);

			int i, j, flag;
			for(i=0;i<box.size.ints;i++){
				flag = 1;
				for(j=0;j<4;j++){
					if(MP4AVCSIGN[j] != buf[i+j]){
						flag = 0;
						break;
					}
				}

				i += j;

				if(flag) break;
			}

			if(flag) {
				memcpy(&info, buf+i, sizeof(stH264Info));
				offset = off + i;
			}

			if(buf) free(buf);
			if(!flag){
				isH264 = FALSE;
				break;
			}
		} else {
			// largesize
			if(box.size.ints == 1) {
				off += box.largesize.ints;
			}
			// 끝까지, 즉 마지막 박스
			else if (box.size.ints == 0) {
				break;
			} else {
				off += box.size.ints;
			}

			_fseeki64(fp, off, 0);
		}
	}

	fclose(fp);

	if(!isMP4) return FALSE;
	if(!isH264) return FALSE;

	return TRUE;
}

BOOL SetChange(LPCTSTR lpszPath)
{
	int dProfile, dLevel;
	FILE *fp = NULL;
	
	dProfile = ComboBox_GetCurSel(GetDlgItem(hWndMain, IDC_CMBPROFILE));
	dLevel = ComboBox_GetCurSel(GetDlgItem(hWndMain, IDC_CMBLEVEL));

	if (dProfile == -1 || dLevel == -1) {
		int rtn = MsgBox(hWndMain, MB_YESNOCANCEL, _T("Error!"), _T("값이 올바로 선택되지 않았습니다. 그대로 유지하여 변경 하시겠습니까?"));

		if (rtn == IDNO || rtn == IDCANCEL) {
			if (rtn == IDNO)
				MsgBox(hWndMain, 0, _T("Information"), _T("값을 다시 선택하세요."));

			if (dProfile == -1) SetFocus(GetDlgItem(hWndMain, IDC_CMBPROFILE));
			else SetFocus(GetDlgItem(hWndMain, IDC_CMBLEVEL));

			return FALSE;
		}
	}

	if(dProfile != -1)
		info.profile.shprofile = profiles[dProfile].shprofile;
	
	if(dLevel > 0 && dLevel < NO_OF_LVL)
		info.level = dlevels[dLevel];

	_tfopen_s(&fp, lpszPath, _T("r+b"));
	if(fp == NULL) return FALSE;

	_fseeki64(fp, offset, 0L);
	fwrite(&info, sizeof(stH264Info), 1, fp);

	fclose(fp);

	MsgBox(hWndMain, 0, _T("Complete!"), _T("모든 작업을 완료하였습니다.\r\n정보를 다시 로드합니다."));

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
	OSVERSIONINFO stOSVersionInfo;
	SYSTEM_INFO si;

	stOSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &stOSVersionInfo );
	GetNativeSystemInfo(&si);

	if( ( 6 <= stOSVersionInfo.dwMajorVersion ) ||
		( 5 == stOSVersionInfo.dwMajorVersion && 2 == stOSVersionInfo.dwMinorVersion ) )
	{
		//64비트를 지원하는 CPU라도 32비트 운영체제가 설치되면 32비트 CPU라고 나옴
		if ( PROCESSOR_ARCHITECTURE_IA64 == si.wProcessorArchitecture ) {
			bResult = TRUE;
		}
		else if ( PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture ) {
			bResult = TRUE;
		}
	}

	return bResult;
}

void ErrorCatcher(LPCTSTR lpszFunction, BOOL bForce)
{ 
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();

	if(!dw && !bForce)
		return;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	MsgBox(NULL, 0, TEXT("Error"), TEXT("%s 중 실패: 에러코드 %d, %s"), lpszFunction, dw, lpMsgBuf);

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

