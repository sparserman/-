#include <windows.h>
#include "resource.h"
#include <time.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("성기정");

BOOL CALLBACK LoginDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK JoinDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK UpdateDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

HWND hDlgMain, hWndMain, hEdit;

HWND hLogoutBtn, hLoginBtn, hJoinBtn, hDropBtn, hUpdateBtn;
HWND hID, hPW;

TCHAR Key[30];

SQLHENV hEnv;
SQLHDBC hDbc;
SQLHSTMT hStmt;

TCHAR g_ID[30], g_PW[30], g_Name[30], g_EMail[30];

SQLCHAR ID[30];
SQLCHAR PW[30];
SQLCHAR UserName[30];
SQLCHAR EMail[30];

SQLCHAR tempID[128];
SQLCHAR tempPW[128];
int level = 0;		// SQL용

SQLINTEGER lID, lPW, lLevel, lUserName;


HMENU hMenu;

HBITMAP hBit, BackBit;

WNDPROC OldEditProc1;

CRITICAL_SECTION crit;

int textnum = 0;
SQLCHAR textname[30];
SQLINTEGER ltextNum, ltextName;
TCHAR sstr[30];

int Level = 0;		// 난이도

int ClearText = 0;		// 레벨을 클리어하기위한 몇개를 맞추었는지 확인용
int Life = 3;	// 생명

TCHAR TextScan[30];		// 텍스트입력 확인용

bool GameOver = true;	// 게임오버를 한번만 띄우기 위한 변수

// 떨어지는 글자
struct Text
{
	int x = 0;		// 위치
	int y = 0;
	int speed = 0;		// 속도

	TCHAR name[30] = "a";		// 텍스트 글자내용

	bool check = false;		// 생성되어있는 지 체크
};

Text text[30];
int textint = 0;		// text클래스 배열에 순서대로 넣기위한 변수

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPED | WS_SYSMENU,
		//CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		350, 200, 1200, 690,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	hWndMain = hWnd;

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return (int)Message.wParam;
}

BOOL DBConnect()
{
	// 연결 설정을 위한 변수들
	SQLCHAR InCon[255];
	SQLCHAR OutCon[1024];
	SQLSMALLINT cbOutCon;
	TCHAR Dir[MAX_PATH];
	SQLRETURN Ret;

	// 환경 핸들을 할당하고 버전 속성을 설정한다.
	if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) != SQL_SUCCESS)
		return FALSE;
	if (SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3,
		SQL_IS_INTEGER) != SQL_SUCCESS)
		return FALSE;

	// 연결 핸들을 할당하고 연결한다.
	if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS)
		return FALSE;

	// MDB 파일에 연결하기
	GetCurrentDirectory(MAX_PATH, Dir);
	wsprintf((TCHAR*)InCon, "DRIVER={Microsoft Access Driver (*.mdb)};"
		"DBQ=%s\\Info.mdb;", Dir);
	Ret = SQLDriverConnect(hDbc, hWndMain, InCon, sizeof(InCon), OutCon, sizeof(OutCon),
		&cbOutCon, SQL_DRIVER_NOPROMPT);

	if ((Ret != SQL_SUCCESS) && (Ret != SQL_SUCCESS_WITH_INFO))
		return FALSE;

	// 명령 핸들을 할당한다.
	if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
		return FALSE;

	return TRUE;
}

void DBDisConnect()
{
	// 뒷정리
	if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
	if (hDbc) SQLDisconnect(hDbc);
	if (hDbc) SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
	if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}
// 입력텍스트가 일치하는지 확인
void TextCheck()
{
	TCHAR temp[30] = "";
	for (int i = 0; i < 30; i++)
	{
		if (strcmp(TextScan, text[i].name) == 0 && text[i].check)
		{
			text[i].check = false;
			ClearText++;
			strcpy_s(TextScan, temp);
			break;
		}
	}
}
// 계정 생성
void CreateUser()		// 계정 만들기
{
	SQLCHAR ID[30];
	SQLCHAR PW[30];
	SQLCHAR Name[30];
	SQLCHAR EMail[128];
	int Age;

	TCHAR szSQL[128];

	GetDlgItemText(hDlgMain, IDC_JOIN_ID_EDIT, (LPTSTR)ID, 30);		// ID 입력받기
	if (lstrlen((LPCTSTR)ID) == 0) {								// ID 칸이 비었는지 확인
		MessageBox(hDlgMain, "ID를 입력하세요.", "알림", MB_OK);
		return;
	}
	GetDlgItemText(hDlgMain, IDC_JOIN_PW_EDIT, (LPTSTR)PW, 30);		// PW 입력받기
	if (lstrlen((LPCTSTR)PW) == 0) {								// PW 칸이 비었는지 확인
		MessageBox(hDlgMain, "PW를 입력하세요.", "알림", MB_OK);
		return;
	}
	GetDlgItemText(hDlgMain, IDC_JOIN_NAME_EDIT, (LPTSTR)Name, 30);	// 이름 입력받기
	if (lstrlen((LPCTSTR)Name) == 0) {								// 이름 칸이 비었는지 확인
		MessageBox(hDlgMain, "이름을 입력하세요.", "알림", MB_OK);
		return;
	}
	GetDlgItemText(hDlgMain, IDC_JOIN_EMAIL_EDIT, (LPTSTR)EMail, 128);	// 이메일 입력받기
	if (lstrlen((LPCTSTR)EMail) == 0) {								// 이메일 칸이 비었는지 확인
		MessageBox(hDlgMain, "이메일을 입력하세요.", "알림", MB_OK);
		return;
	}

	// szSQL에 정보 입력
	wsprintf(szSQL, "Insert into userinfo (ID, PW, NAME, EMail) VALUES ('%s', '%s', '%s', '%s')",
		ID, PW, Name, EMail);

	if (SQLExecDirect(hStmt, (SQLCHAR*)szSQL, SQL_NTS) != SQL_SUCCESS)
	{
		MessageBox(hDlgMain, "이미 있는 ID 입니다.", "에러", MB_OK);
		return;
	}
	MessageBox(hDlgMain, "가입이 완료 되었습니다.", "알림", MB_OK);
	EndDialog(hDlgMain, 0);

	SQLCloseCursor(hStmt);
}
// 게임 클리어
void Clear()
{
	TCHAR szSQL[128];
	Level++;
	wsprintf(szSQL, "Update userinfo set CurrentLevel = %d where ID = '%s'", Level, Key);
	if (SQLExecDirect(hStmt, (SQLCHAR*)szSQL, SQL_NTS) != SQL_SUCCESS)
	{
		return;
	}
	if (Level == 0)
	{

	}
	else if (Level > 0 && Level != 4)
	{
		MessageBox(hWndMain, "클리어! 다음단계로 넘어갑니다", "알림", MB_OK);
	}
	else
	{
		MessageBox(hWndMain, "모든 단계를 클리어하셨습니다!", "알림", MB_OK);
		MessageBox(hWndMain, "게임이 종료됩니다.", "알림", MB_OK);
		PostQuitMessage(0);
	}

	for (int i = 0; i < 30; i++)
	{
		text[i].check = false;
	}
	GameOver = true;
	ClearText = 0;
}
// 로그인
void Login()
{
	TCHAR szSQL[128];

	GetDlgItemText(hDlgMain, IDC_ID_EDIT, (LPTSTR)ID, 30);		// 빈 칸 확인용
	if (lstrlen((LPCTSTR)ID) == 0) {
		MessageBox(hDlgMain, "ID를 입력하세요.", "알림", MB_OK);
		return;
	}
	GetDlgItemText(hDlgMain, IDC_PW_EDIT, (LPTSTR)PW, 30);
	if (lstrlen((LPCTSTR)PW) == 0) {
		MessageBox(hDlgMain, "PW를 입력하세요.", "알림", MB_OK);
		return;
	}
	GetDlgItemText(hDlgMain, IDC_ID_EDIT, Key, 30);		// 현재 계정 기억용

	SQLBindCol(hStmt, 1, SQL_C_CHAR, tempID, sizeof(tempID), &lID);
	SQLBindCol(hStmt, 2, SQL_C_CHAR, tempPW, sizeof(tempPW), &lPW);
	SQLBindCol(hStmt, 3, SQL_C_CHAR, UserName, sizeof(UserName), &lUserName);
	SQLBindCol(hStmt, 5, SQL_C_CHAR, EMail, sizeof(tempPW), &lPW);
	SQLBindCol(hStmt, 4, SQL_C_ULONG, &level, 0, &lLevel);

	SQLExecDirect(hStmt, (SQLCHAR*)"select * from userinfo", SQL_NTS);

	bool check = false;		// ID PW 일치하는 지 확인용
	while (SQLFetch(hStmt) != SQL_NO_DATA)
	{
		if (strcmp((TCHAR*)ID, (TCHAR*)tempID) == 0)
		{
			if (strcmp((TCHAR*)PW, (TCHAR*)tempPW) == 0)
			{
				EnableWindow(hLogoutBtn, TRUE);		// 버튼 활성/비활성 작업
				EnableWindow(hDropBtn, TRUE);
				EnableWindow(hUpdateBtn, TRUE);
				EnableWindow(hLoginBtn, FALSE);
				EnableWindow(hJoinBtn, FALSE);

				EnableWindow(hID, FALSE);		// 에디트 활성/비활성 작업
				EnableWindow(hPW, FALSE);

				strcpy_s(g_ID, (TCHAR*)tempID);
				strcpy_s(g_PW, (TCHAR*)tempPW);
				strcpy_s(g_Name, (TCHAR*)UserName);

				MessageBox(hDlgMain, "로그인 성공!", "알림", MB_OK);

				EnableMenuItem(hMenu, ID_LOGOUT, MF_ENABLED);		// 로그아웃을 활성화
				EnableMenuItem(hMenu, ID_LOGIN, MF_DISABLED);		// 로그인을 비활성화

				switch (level)
				{
				case 0:
					MessageBox(hDlgMain, "1단계부터 시작합니다.", "알림", MB_OK);
					Clear();
					Life = 3;	// 생명 초기화
					break;
				case 1:
					MessageBox(hDlgMain, "1단계를 하고있었습니다.", "알림", MB_OK);
					Level = level;
					Life = 3;
					break;
				case 2:
					MessageBox(hDlgMain, "2단계를 하고있었습니다.", "알림", MB_OK);
					Level = level;
					Life = 2;
					break;
				case 3:
					MessageBox(hDlgMain, "3단계를 하고있었습니다.", "알림", MB_OK);
					Level = level;
					Life = 1;
					break;
				}

				check = true;
				break;
			}
		}
	}

	SQLCloseCursor(hStmt);

	if (!check)		// IDPW 틀리면
	{
		MessageBox(hDlgMain, "아이디 또는 비밀번호가 일치하지않습니다.", "알림", MB_OK);
		SetDlgItemText(hDlgMain, IDC_ID_EDIT, "");
		SetDlgItemText(hDlgMain, IDC_PW_EDIT, "");
	}
	EndDialog(hDlgMain, NULL);
}
// 로그아웃
void Logout()
{
	TCHAR temp[30] = "";	// 아이디 정보 초기화용
	Level = 0;		// 게임 난이도 초기화
	strcpy_s(g_Name, "");
	strcpy_s(Key, temp);

	SetDlgItemText(hDlgMain, IDC_ID_EDIT, "");
	SetDlgItemText(hDlgMain, IDC_PW_EDIT, "");

	EnableWindow(hLogoutBtn, FALSE);		// 버튼 활성/비활성 작업
	EnableWindow(hDropBtn, FALSE);
	EnableWindow(hUpdateBtn, FALSE);
	EnableWindow(hLoginBtn, TRUE);
	EnableWindow(hJoinBtn, TRUE);

	EnableWindow(hID, TRUE);		// 에디트 활성/비활성 작업
	EnableWindow(hPW, TRUE);

	SQLCloseCursor(hStmt);
}
// 비트맵 그리기
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit)
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx, by;
	BITMAP bit;

	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);

	GetObject(hBit, sizeof(BITMAP), &bit);
	bx = bit.bmWidth;
	by = bit.bmHeight;

	BitBlt(hdc, x, y, bx, by, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}
// DB에서 글자 찾아오기
TCHAR* Find()
{
	int tempNum = rand() % 5 + 1;

	SQLBindCol(hStmt, 1, SQL_C_ULONG, &textnum, 0, &ltextNum);
	
	// 레벨에 따라 내려오는 글자가 변화
	SQLBindCol(hStmt, (Level + 3), SQL_C_CHAR, textname, sizeof(textname), &ltextName);


	SQLExecDirect(hStmt, (SQLCHAR*)"select * from textinfo", SQL_NTS);

	while (SQLFetch(hStmt) != SQL_NO_DATA)
	{
		if (tempNum == textnum)		// 랜덤숫자와 일치하는 번호의 글자를 카피
		{
			strcpy_s(sstr, (TCHAR*)textname);
			break;
		}
	}

	SQLCloseCursor(hStmt);

	return sstr;
}
// 텍스트 만들기
DWORD CALLBACK CreateText(LPVOID n)
{
	while (1)
	{
		if (Level > 0)
		{
			Sleep(4000 - (Level * 1000));
			text[textint].x = rand() % 1100;
			text[textint].y = 0;
			text[textint].speed = Level * 1;

			strcpy_s(text[textint].name, Find());
			text[textint].check = true;

			if (textint < 29)
			{
				textint++;		// 다음엔 다음 배열을 생성하기위함
			}
			else
			{
				textint = 0;
			}
		}
	}
}
// 생명 깎기
void LifeDown()
{
	for (int i = 0; i < 30; i++)
	{
		if (text[i].y > 610 && text[i].check)
		{
			text[i].check = false;
			Life--;
		}
	}
}
// 텍스트 내려가게하기, 폰트설정
void TextDown(HDC hMemDC)
{
	HFONT font, oldfont;

	SetBkMode(hMemDC, TRANSPARENT);
	font = CreateFont(25, 0, 0, 0, 0, 0, 0, 0, HANGEUL_CHARSET, 0, 0, 0,
		VARIABLE_PITCH | FF_ROMAN, "맑은 고딕");
	oldfont = (HFONT)SelectObject(hMemDC, font);
	SetTextColor(hMemDC, RGB(255, 255, 255));
	//DrawText(hMemDC, text, -1, &grt, DT_WORDBREAK);
	for (int i = 0; i < 30; i++)
	{
		if (text[i].check)
		{
			text[i].y += text[i].speed;
			TextOut(hMemDC, text[i].x, text[i].y, text[i].name, lstrlen(text[i].name));
		}
	}
	DeleteObject(SelectObject(hMemDC, oldfont));
}
// UI 표시
void UI(HDC hMemDC)
{
	TCHAR str[128];
	wsprintf(str, "이름 : %s", g_Name);
	TextOut(hMemDC, 1000, 10, str, lstrlen(str));

	wsprintf(str, "ID : %s", ID);
	TextOut(hMemDC, 1000, 40, str, lstrlen(str));

	wsprintf(str, "%d단계", Level);
	TextOut(hMemDC, 1000, 70, str, lstrlen(str));

	wsprintf(str, "%남은 생명 : %d", Life);
	TextOut(hMemDC, 1000, 100, str, lstrlen(str));
}
// 더블버퍼링
void OnTimer()
{
	RECT crt;
	HDC hdc, hMemDC;
	HBITMAP OldBit;

	GetClientRect(hWndMain, &crt);
	hdc = GetDC(hWndMain);

	if (!hBit)
	{
		hBit = CreateCompatibleBitmap(hdc, crt.right, crt.bottom);
	}

	hMemDC = CreateCompatibleDC(hdc);
	OldBit = (HBITMAP)SelectObject(hMemDC, hBit);

	DrawBitmap(hMemDC, 0, 0, BackBit);	// 배경그리기

	TextDown(hMemDC);

	TextCheck();

	if (Level > 0 && ClearText == Level * 10 && GameOver)
	{
		GameOver = false;
		Clear();
	}

	UI(hMemDC);

	LifeDown();			// 생명 깎기

	if (GameOver && Life == 0)
	{
		GameOver = false;
		MessageBox(hWndMain, "게임 오버", "알림", MB_OK);
		PostQuitMessage(0);
	}

	SelectObject(hMemDC, OldBit);
	DeleteDC(hMemDC);
	ReleaseDC(hWndMain, hdc);
	InvalidateRect(hWndMain, NULL, false);
}

BOOL CALLBACK LoginDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	hDlgMain = hDlg;

	hLogoutBtn = GetDlgItem(hDlgMain, IDC_LOGOUT);
	hLoginBtn = GetDlgItem(hDlgMain, IDC_LOGIN);
	hJoinBtn = GetDlgItem(hDlgMain, IDC_JOIN);
	hDropBtn = GetDlgItem(hDlgMain, IDC_DROP);
	hUpdateBtn = GetDlgItem(hDlgMain, IDC_UPDATE);

	hID = GetDlgItem(hDlgMain, IDC_ID_EDIT);
	hPW = GetDlgItem(hDlgMain, IDC_PW_EDIT);

	switch (iMessage) {
	case WM_INITDIALOG:
		if (DBConnect() == FALSE) {
			MessageBox(hWndMain, "데이터 베이스에 연결할 수 없습니다", "에러", MB_OK);
			return -1;
		}
		EnableWindow(hLogoutBtn, FALSE);
		EnableWindow(hDropBtn, FALSE);
		EnableWindow(hUpdateBtn, FALSE);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_LOGIN:
			Login();
			return TRUE;
		case IDC_JOIN:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), HWND_DESKTOP, JoinDlgProc);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

BOOL CALLBACK JoinDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	hDlgMain = hDlg;

	switch (iMessage) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			CreateUser();
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

LRESULT CALLBACK EditSubProc1(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	TCHAR str[128];
	switch (iMessage) {
	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			GetWindowText(hWnd, str, 128);		// 문자열을 받아와서
			SetWindowText(hWnd, TEXT(""));		// 에디트박스는 내용을 비우고

			strcpy_s(TextScan, str);		// 입력

			SetFocus(hWnd);		// 바로 입력가능하게 포커스
		}
		break;
	}
	return CallWindowProc(OldEditProc1, hWnd, iMessage, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	POINT pt = { LOWORD(lParam), HIWORD(lParam) };

	TCHAR str[128];

	DWORD ThreadID;
	HANDLE hThread;

	srand(time(NULL));

	switch (iMessage)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case ID_LOGIN:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), HWND_DESKTOP, LoginDlgProc);
			break;
		case ID_LOGOUT:
			EnableMenuItem(hMenu, ID_LOGIN, MF_ENABLED);		// 로그인을 활성화
			EnableMenuItem(hMenu, ID_LOGOUT, MF_DISABLED);		// 로그아웃을 비활성화
			Logout();
			break;
		case ID_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_CREATE:
		hMenu = GetMenu(hWnd);

		if (DBConnect() == FALSE) {
			MessageBox(hWndMain, "데이터 베이스에 연결할 수 없습니다", "에러", MB_OK);
			return -1;
		}

		// 서브클래스 ( 글자 입력창 )
		hEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_AUTOHSCROLL,
			500, 550, 200, 25, hWnd, (HMENU)100, g_hInst, NULL);

		// 쓰레드
		hThread = CreateThread(NULL, 0, CreateText, NULL, 0, NULL);
		CloseHandle(hThread);

		OldEditProc1 = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubProc1);

		BackBit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2));	// 배경 비트맵 로드

		EnableMenuItem(hMenu, ID_LOGOUT, MF_DISABLED);		// 로그아웃을 비활성화

		SetTimer(hWnd, 0, 30, NULL);
		break;
	case WM_TIMER:
		switch (wParam)
		{
		case 0:
			OnTimer();
			break;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		DrawBitmap(hdc, 0, 0, hBit);

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		DBDisConnect();
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}