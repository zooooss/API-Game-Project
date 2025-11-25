#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <MMSystem.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
POINT CenterPoint(RECT& r); //윈도우 중앙 위치 저장 함수

void DrawObject(HDC, RECT&, COLORREF, int, int ropCode = R2_XORPEN);
void DrawObject(HDC, RECT&, COLORREF, COLORREF, int, int ropCode = R2_XORPEN);
void DisplayCount(HDC, int, RECT&, COLORREF color = RGB(255, 255, 255));
int CheckStrikeX(RECT&, RECT&);
int CheckBallBoundX(RECT&, RECT&);	// ball이 좌/우 벽에 부딪침 확인
int CheckBallBoundY(RECT&, RECT&);	// ball이 상/하 벽에 부딪침 확인 
int HitTest(RECT&, RECT&);			// ball과 bar 부딪침 확인 

HINSTANCE g_hInst;
HWND hWndMain;
LPCTSTR lpszClass = _T("Block Game with Sound");

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpszCmdParam, _In_ int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	  // 검은색 배경  
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
		300, 100, 1000, 700,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	hWndMain = hWnd; // hWnd 정보도 전역변수에 저장!

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage,
	WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;

	static RECT textR;				// 추가: 점수가 출력될 사각형 
	static RECT barR, clientR;		// 사각 bar,  윈도우 정보 저장 clientR
	static RECT ballR;				// ball
	static COLORREF barColor;		// bar 색상

	static int alphaX;			// 화살표 왼쪽 -1, 오른쪽 1에 쓰이는 toggle
	static int ballToggleX;		// toggle: 공의 좌/우 방향 전환
	static int ballToggleY;		// toggle: 공의 상/하 방향 전환 

	static POINT p{};					//  1) 중앙 위치 구한 후, bar 설정에 사용, 2) 현재 클릭 위치
	static POINT q{};					//  bar 선택 후, 드래그 할 때 위치
	static BOOL dragFlag = FALSE;		//  toggle: bar 영역이 클릭된 상태에서, 드래그 되는지 여부

	int vOffset{};						// 드래그 할 때 발생하는 offset 값 저장
	int hitPosition;					// bar와 ball이 부딪친 위치 정보 저장(bar 옆=1, 위=2) 
	static int hitNumCount;			// bar와 ball이 부딪친 횟수 저장  

	TCHAR str[100] = _T("");			// 점수 저장 및 점수 출력시 사용  

	switch (iMessage) {
	case WM_CREATE:
		InvalidateRect(hWnd, NULL, TRUE); // 재시작시 이전 화면 정보 모두 리셋

		barColor = RGB(255, 0, 0);
		ballToggleX = 1;	// 공의 최초 이동 방향: 오른쪽
		ballToggleY = 1;	// 공의 최초 이동 방향: 아래쪽, 즉, 45도 우측! 
		GetClientRect(hWnd, &clientR);
		p = CenterPoint(clientR);
		textR = clientR;
		textR.bottom = textR.top + 20;		// 점수 출력 영역 textR의 bottom 재설정 

		SetRect(&barR, p.x - 50, p.y - 15, p.x + 50, p.y + 15);
		SetRect(&ballR, p.x - 10, p.y - 10, p.x + 10, p.y + 10);		// ball 크기 설정
		OffsetRect(&barR, 0, 300);				// 중심 위치의 bar를 아래로 300픽셀 이동
		SetTimer(hWnd, 1, 10, NULL);			// ball 이동을 위한 타이머, 0.01초마다 메세지
		SetTimer(hWnd, 2, 15 * 1000, NULL);	// 자동 종료를 위한 타이머 id 2: 15초 종료 

		hitNumCount = 0;		// ball과 bar 부딪친 횟수 0으로 초기화. 
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawObject(hdc, barR, barColor, 0);		// 4번째 인자: 0 - 사각형
		DrawObject(hdc, ballR, RGB(255, 255, 255), RGB(255, 0, 255), 1);	// ball  그리기 [수정] 

		// 기존 점수 출력 등 코드 삭제 후, 점수 출력 전용 함수 호출
		DisplayCount(hdc, hitNumCount, clientR);	// clientR 사각형 위치에 점수 출력 

		EndPaint(hWnd, &ps);
		return 0;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			alphaX = -1;		// x축 왼쪽[-1] 이동 설정

			if (CheckStrikeX(barR, clientR) == 1)
				break;		// 왼쪽 벽 부딪치면, bar 위치 정보 변화 없음
			else {
				OffsetRect(&barR, 5 * alphaX, 0);
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		case VK_RIGHT:
			alphaX = 1;		// x축 오른쪽[1] 이동 설정

			if (CheckStrikeX(barR, clientR) == 2)
				break;		// 오른쪽 벽 부딪치면, bar 위치 정보 변화 없음
			else {
				OffsetRect(&barR, 5 * alphaX, 0);
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		}
		return 0;
	case WM_LBUTTONDOWN:
		p.x = LOWORD(lParam);
		p.y = HIWORD(lParam);
		if (PtInRect(&barR, p))
			dragFlag = TRUE;	// bar 내부에서 클릭됨: TRUE 로 설정
		return 0;
	case WM_MOUSEMOVE:
		hdc = GetDC(hWnd);		// bar 그리기 위해 GetDC( ) 이용하여, DC 권한 취득 
		if (dragFlag) {
			q.x = LOWORD(lParam);
			q.y = HIWORD(lParam);

			DrawObject(hdc, barR, barColor, 0);

			vOffset = q.x - p.x;

			OffsetRect(&barR, vOffset, 0);
			DrawObject(hdc, barR, barColor, 0);
			//	InvalidateRect(hWnd, NULL, TRUE);

			p = q;		// Old Point 정보인 p를 업데이트!
		}
		ReleaseDC(hWnd, hdc);
		return 0;
	case WM_LBUTTONUP:
		if (dragFlag)
			dragFlag = FALSE;
		return 0;
	case WM_TIMER:
		hdc = GetDC(hWnd);
		switch (wParam) {
		case 1:
			hitPosition = HitTest(ballR, barR);		// ball과 bar 부딪친 위치 정보!

			DrawObject(hdc, ballR, RGB(255, 255, 255), RGB(255, 0, 255), 1);	// ball 그리기 추가 1 

			if (CheckBallBoundX(ballR, clientR) || hitPosition == 1)
				ballToggleX *= -1;		// ball 윈도우 좌/우 벽 [OR] bar 옆에 부딪치면 toggle 전환

			if (CheckBallBoundY(ballR, clientR) || hitPosition == 2)
				ballToggleY *= -1;		// ball 윈도우 상/하 벽 [OR] bar 상단에 부딪치면 toggle 전환

			OffsetRect(&ballR, 3 * ballToggleX, 3 * ballToggleY);		// 공 45도로 변화

			if (hitPosition) {
				hitNumCount++;		 				// ball과 bar가 부딪치면 점수 증가!
				InvalidateRect(hWnd, &textR, TRUE); 	// 점수 출력을 위한 WM_PAINT 발생 시킴
			}
			DrawObject(hdc, ballR, RGB(255, 255, 255), RGB(255, 0, 255), 1);	// ball 그리기 추가 2 
			//	InvalidateRect(hWnd, NULL, TRUE); 
			break;
		case 2:	// 종료 타이머(ID: 2) 메시지 처리!
			KillTimer(hWnd, 1);
			KillTimer(hWnd, 2);
			_stprintf_s(str, _T("Hit Count: %6d \n\n Do you want to restart?"), hitNumCount);  // str: 게임 종료 메시지

			// 메세지 박스가 출력되고, No가 눌려지면 IDNO 매크로 값이 리턴됨!
			// hitPosition 정보가 더이상 필요 없으므로, 메시지 박스 리턴값을 저장하는데 사용.
			hitPosition = MessageBox(hWnd, str, _T("confirm"),
				MB_YESNO | MB_ICONQUESTION);

			if (hitPosition == IDNO)
				DestroyWindow(hWnd);
			else
				SendMessage(hWnd, WM_CREATE, 0, 0);	// 다시 초기화 메시지 강제 발생!

			break;
		}
		return 0;
	case WM_DESTROY:
		//	KillTimer(hWnd, 1);
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

POINT CenterPoint(RECT& r) {
	POINT p;
	p.x = (r.left + r.right) / 2;
	p.y = (r.top + r.bottom) / 2;
	return p;
}

void DrawObject(HDC hdc, RECT& r, COLORREF color, int type, int ropCode)
{
	DrawObject(hdc, r, color, color, type);
}

void DrawObject(HDC hdc, RECT& r, COLORREF penC, COLORREF brushC, int type, int ropCode) {
	HPEN hPen, hOldPen;
	HBRUSH hBrush, hOldBrush;
	SetROP2(hdc, ropCode);		// ROP 설정 함수 추가 

	hPen = CreatePen(PS_SOLID, 1, penC);
	hOldPen = (HPEN)SelectObject(hdc, hPen);
	hBrush = CreateSolidBrush(brushC);
	hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	switch (type) {		// type이 0: 사각형, 1: 원
	case 0:
		Rectangle(hdc, r.left, r.top, r.right, r.bottom);
		break;
	case 1:
		Ellipse(hdc, r.left, r.top, r.right, r.bottom);
		break;
	}

	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBrush);
	DeleteObject(hPen);
	DeleteObject(hBrush);
}

int CheckStrikeX(RECT& bar, RECT& client)
{
	if (bar.left <= client.left)
		return 1;
	else if (bar.right >= client.right)
		return 2;
	else
		return 0;
}

int CheckBallBoundX(RECT& ball, RECT& client)
{
	if (ball.left <= client.left || ball.right >= client.right)
		return 1;
	else
		return 0;
}

int CheckBallBoundY(RECT& ball, RECT& client)
{
	if (ball.top <= client.top || ball.bottom >= client.bottom)
		return 1;
	else
		return 0;
}

int HitTest(RECT& ball, RECT& bar)
{
	RECT temp;
	int w, h;

	if (IntersectRect(&temp, &ball, &bar))
	{
		w = temp.right - temp.left;
		h = temp.bottom - temp.top;

		if (w < h)
			return 1;	 // ball이 bar의 옆에 부딪침
		else
			return 2;	 // ball이 bar의 상단에 부딪침
	}
	else return 0;	  // 부딪치지 않음
}

void DisplayCount(HDC hdc, int count, RECT& client, COLORREF color)
{
	TCHAR str[100] = _T("");
	_stprintf_s(str, _T("Hit Count : % 6d "), count);	// count 점수를 포함한 str 문자열 생성 

	SetBkMode(hdc, TRANSPARENT);			// 출력 글자 배경: 투명 
	SetTextColor(hdc, RGB(255, 255, 255));		// 흰색 글자 

	DrawText(hdc, str, _tcslen(str), &client, DT_SINGLELINE | DT_RIGHT | DT_TOP);		// 점수 출력 
}

