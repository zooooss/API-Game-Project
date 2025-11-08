#include <windows.h>
#include <tchar.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
POINT CenterPoint(RECT& r); //윈도우 중앙 위치 저장 함수

void DrawObject(HDC, RECT&, COLORREF, int);
void DrawObject(HDC, RECT&, COLORREF, COLORREF, int);
BOOL CheckClickX(POINT click, RECT& circle);

HINSTANCE g_hInst;
HWND hWndMain;
LPCTSTR lpszClass = _T("Catch Me");

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpszCmdParam, _In_ int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
300,100,1000,700,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	hWndMain = hWnd; // Store hWnd information in a global variable hWndMain!

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
	static RECT ballR,clientR;
	static int hitCount = 0;
	TCHAR scoreStr[64];
	TCHAR str[] = _T("Click Me!");
	// 중심 좌표 계산
	int centerX = (ballR.left + ballR.right) / 2;
	int centerY = (ballR.top + ballR.bottom) / 2;
	static int moveInterval = 1000; // 공 이동 초기 간격 (1초)

	static POINT p; //현재 클릭 위치

	switch (iMessage) {
	case WM_CREATE:
		GetClientRect(hWnd, &clientR);
		p = CenterPoint(clientR);
		SetRect(&ballR, p.x - 40, p.y - 40, p.x + 40, p.y + 40);
		SetTimer(hWnd, 1, moveInterval, NULL);
		SetTimer(hWnd, 2, 1000, NULL);
		SetTimer(hWnd, 3, 30000, NULL); // 30초 후 게임종료!

		return 0;
	case WM_LBUTTONDOWN:
		p.x = LOWORD(lParam);
		p.y = HIWORD(lParam);

		if (CheckClickX(p, ballR)) {
			hitCount++; // 원 안이면 점수 증가
			InvalidateRect(hWnd, NULL, TRUE); // 화면 갱신
		}
		return 0;
	case WM_TIMER:
		if (wParam == 1) { // 공 이동
			int radius = (ballR.right - ballR.left) / 2;
			int x = radius + rand() % (clientR.right - 2 * radius);
			int y = radius + rand() % (clientR.bottom - 2 * radius);
			SetRect(&ballR, x - radius, y - radius, x + radius, y + radius);
			InvalidateRect(hWnd, NULL, TRUE); // 화면 갱신
		}
		else if (wParam == 2) { // 2초마다 속도 증가
			if (moveInterval > 100) { // 최소 0.1초
				moveInterval -= 50;   // 50ms씩 빨라짐
				KillTimer(hWnd, 1);   // 공 이동 타이머 재설정
				SetTimer(hWnd, 1, moveInterval, NULL); // 공 이동 간격 변경
			}
		}
			else if (wParam == 3) { // 30초 타이머
				TCHAR msg[64];
				wsprintf(msg, _T("게임 종료!\n최종 점수: %d"), hitCount);
				MessageBox(hWnd, msg, _T("결과"), MB_OK);
				PostQuitMessage(0);
			}
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawObject(hdc, ballR, RGB(0, 0, 0), RGB(255, 0, 255), 1);


		// 문자열 만들기
		wsprintf(scoreStr, _T("HitCount: %d, Ball Center: (%d, %d)"), hitCount, centerX, centerY);

		// 출력
		TextOut(hdc, 10, 10, scoreStr, lstrlen(scoreStr));

		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		KillTimer(hWnd, 3);
		KillTimer(hWnd, 2);
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
void DrawObject(HDC hdc, RECT& r, COLORREF color, int type) {
	DrawObject(hdc, r, color, color,type);
}
void DrawObject(HDC hdc, RECT& r, COLORREF penC, COLORREF brushC, int type) {
	HPEN hPen, hOldPen;
	HBRUSH hBrush, hOldBrush;
	hPen = CreatePen(PS_SOLID, 1, penC);
	hOldPen = (HPEN)SelectObject(hdc, hPen);
	hBrush = CreateSolidBrush(brushC);
	hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	switch (type) {
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

BOOL CheckClickX(POINT click, RECT& circle)
{
	int centerX = (circle.left + circle.right) / 2;
	int centerY = (circle.top + circle.bottom) / 2;
	int radius = (circle.right - circle.left) / 2;

	int dx = click.x - centerX;
	int dy = click.y - centerY;

	return (dx * dx + dy * dy <= radius * radius);
}