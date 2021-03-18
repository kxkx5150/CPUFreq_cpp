#include "stdafx.h"
#include "CPUFreq.h"
#include <string>
#include <iostream>
#include <windows.h>
#include <sstream>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#define MAX_LOADSTRING 100
#define	WM_USER_SHELLICON WM_USER + 1
#define _WIN32_DCOM

using namespace std;

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
HICON               CreateTextIcon(HWND hWnd);
HICON               CreateImageIcon(COLORREF iconColor, int freq, int usage, int mem);
void                ChangeIcon();
double              GetCPUFrequecyPercent();
double              GetCPUMaxFrequecy();
double              GetCPUFrequency();
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

HINSTANCE hInst;
NOTIFYICONDATA nidApp;
HMENU hPopMenu;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
TCHAR szApplicationToolTip[MAX_LOADSTRING];
BOOL bDisable = FALSE;
double cpuMaxFreq = GetCPUMaxFrequecy();

double GetCPUFrequency() {
	double cpufreq = GetCPUFrequecyPercent() * cpuMaxFreq / 100;
	wstringstream wss;
	wss << cpufreq;
	OutputDebugString(wss.str().c_str());
	OutputDebugString(_T("\n"));
	return 0;
}
double GetCPUFrequecyPercent() {
	HRESULT hres;

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres)) {
		return 0;
	}
	hres = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
	);

	if (FAILED(hres)) {
		CoUninitialize();
		return 0;
	}
	IWbemLocator* pLoc = NULL;
	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres)) {
		CoUninitialize();
		return 0;
	}
	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL,
		NULL,
		0,
		NULL,
		0,
		0,
		&pSvc
	);

	if (FAILED(hres)) {
		pLoc->Release();
		CoUninitialize();
		return 0;
	}
	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 0;
	}
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM  Win32_PerfFormattedData_Counters_ProcessorInformation"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 0;
	}

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;

	int cpucount = 0;
	double cpufreqs = 0;

	while (pEnumerator) {
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (0 == uReturn) {
			break;
		}

		VARIANT vtProp;
		VariantInit(&vtProp);
		hr = pclsObj->Get(L"PercentProcessorPerformance", 0, &vtProp, 0, 0);
		cpucount++;
		cpufreqs += _wtoi(vtProp.bstrVal);
		VariantClear(&vtProp);
		pclsObj->Release();
	}

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();
	return cpufreqs / cpucount;
}
double GetCPUMaxFrequecy() {
	HRESULT hres;

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres)) {
		return 0;
	}
	hres = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
	);

	if (FAILED(hres)) {
		CoUninitialize();
		return 0;
	}
	IWbemLocator* pLoc = NULL;
	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres)) {
		CoUninitialize();
		return 0;
	}
	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL,
		NULL,
		0,
		NULL,
		0,
		0,
		&pSvc
	);

	if (FAILED(hres)) {
		pLoc->Release();
		CoUninitialize();
		return 0;
	}
	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 0;
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_Processor"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);
	IWbemClassObject* pclsObj;
	ULONG uReturn = 0;

	double cpufreqs = 0;
	while (pEnumerator) {
		HRESULT hr = pEnumerator->Next(
			WBEM_INFINITE,
			1,
			&pclsObj,
			&uReturn
		);
		if (uReturn == 0) { break; }
		VARIANT vtProp;
		VariantInit(&vtProp);
		hr = pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
		cpufreqs = (unsigned int)vtProp.bstrVal;
		VariantClear(&vtProp);
		pclsObj->Release();
	}

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();
	return cpufreqs;
}

static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks) {
	static unsigned long long _previousTotalTicks = 0;
	static unsigned long long _previousIdleTicks = 0;

	unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
	unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;

	float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

	_previousTotalTicks = totalTicks;
	_previousIdleTicks = idleTicks;
	return ret;
}
static unsigned long long FileTimeToInt64(const FILETIME& ft) { return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime); }
double GetCPUUsage() {
	FILETIME idleTime, kernelTime, userTime;
	return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
}
int GetMemoryUsage() {
	MEMORYSTATUSEX ms = { sizeof(MEMORYSTATUSEX) };
	GlobalMemoryStatusEx(&ms);
	return ms.dwMemoryLoad;
}

void ChangeIcon() {
	double percent = GetCPUFrequecyPercent();
	double usage = GetCPUUsage();
	int mem = GetMemoryUsage();

	int f = 16 - ceil(16 * percent / 100);
	int u = 16 - ceil(16 * usage);
	int m = 16 - ceil(16 * mem / 100);

	nidApp.hIcon = CreateImageIcon(0x0000FF00, f, u, m);
	Shell_NotifyIcon(NIM_MODIFY, &nidApp);

	wstringstream wss;
	wss << m;
	OutputDebugString(wss.str().c_str());
	OutputDebugString(_T("\n"));
}
HICON CreateImageIcon(COLORREF iconColor, int freq, int usage, int mem) {
	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hbmp = CreateCompatibleBitmap(hdcScreen, 16, 16);
	HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmp);

	HBRUSH hbrush = CreateSolidBrush(0x00000000);
	HBRUSH hbrushOld = (HBRUSH)SelectObject(hdcMem, hbrush);
	Rectangle(hdcMem, 0, 0, 16, 16);
	SelectObject(hdcMem, hbrushOld);

	HPEN hpen = CreatePen(PS_SOLID, 2, iconColor);
	HPEN hpenOld = (HPEN)SelectObject(hdcMem, hpen);
	MoveToEx(hdcMem, 3, freq, NULL);
	LineTo(hdcMem, 3, 16);
	SelectObject(hdcMem, hpenOld);

	HPEN hpen2 = CreatePen(PS_SOLID, 2, RGB(0xff, 0x80, 0x80));
	HPEN hpenOld2 = (HPEN)SelectObject(hdcMem, hpen2);
	MoveToEx(hdcMem, 8, usage, NULL);
	LineTo(hdcMem, 8, 16);
	SelectObject(hdcMem, hpenOld2);

	HPEN hpen3 = CreatePen(PS_SOLID, 2, 0x0000FFFF);
	HPEN hpenOld3 = (HPEN)SelectObject(hdcMem, hpen3);
	MoveToEx(hdcMem, 13, mem, NULL);
	LineTo(hdcMem, 13, 16);
	SelectObject(hdcMem, hpenOld3);

	DeleteObject(hbrush);
	DeleteObject(hpen);
	DeleteObject(hpen2);
	DeleteObject(hpen3);

	HBITMAP hbmpMask = CreateCompatibleBitmap(hdcScreen, 16, 16);
	ICONINFO ii;
	ii.fIcon = TRUE;
	ii.hbmMask = hbmpMask;
	ii.hbmColor = hbmp;
	HICON hIcon = CreateIconIndirect(&ii);
	DeleteObject(hbmpMask);

	SelectObject(hdcMem, hbmpOld);
	DeleteObject(hbmp);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
	return hIcon;
}
HICON CreateTextIcon(HWND hWnd) {
	static TCHAR* szText = TEXT("100");
	HDC hdc, hdcMem;
	HBITMAP hBitmap = NULL;
	HBITMAP hOldBitMap = NULL;
	HBITMAP hBitmapMask = NULL;
	ICONINFO iconInfo;
	HFONT hFont;
	HICON hIcon;

	hdc = GetDC(hWnd);
	hdcMem = CreateCompatibleDC(hdc);
	hBitmap = CreateCompatibleBitmap(hdc, 16, 16);
	hBitmapMask = CreateCompatibleBitmap(hdc, 16, 16);
	ReleaseDC(hWnd, hdc);
	hOldBitMap = (HBITMAP)SelectObject(hdcMem, hBitmap);
	PatBlt(hdcMem, 0, 0, 16, 16, WHITENESS);

	hFont = CreateFont(12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		TEXT("Consolas"));
	hFont = (HFONT)SelectObject(hdcMem, hFont);
	TextOut(hdcMem, 0, 0, szText, lstrlen(szText));

	SelectObject(hdc, hOldBitMap);
	hOldBitMap = NULL;

	iconInfo.fIcon = TRUE;
	iconInfo.xHotspot = 0;
	iconInfo.yHotspot = 0;
	iconInfo.hbmMask = hBitmapMask;
	iconInfo.hbmColor = hBitmap;

	hIcon = CreateIconIndirect(&iconInfo);

	DeleteObject(SelectObject(hdcMem, hFont));
	DeleteDC(hdcMem);
	DeleteDC(hdc);
	DeleteObject(hBitmap);
	DeleteObject(hBitmapMask);

	return hIcon;
}
ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SYSTRAYDEMO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SYSTRAYDEMO);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	HWND hWnd;
	hInst = hInstance;
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return FALSE;
	}
	nidApp.cbSize = sizeof(NOTIFYICONDATA);
	nidApp.hWnd = (HWND)hWnd;
	nidApp.uID = IDI_SYSTRAYDEMO;
	nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nidApp.hIcon = CreateImageIcon(0x0000FF00, 0,0,0);
	nidApp.uCallbackMessage = WM_USER_SHELLICON;
	LoadString(hInstance, IDS_APPTOOLTIP, nidApp.szTip, MAX_LOADSTRING);
	Shell_NotifyIcon(NIM_ADD, &nidApp);
	SetTimer(hWnd, 1, 1000, NULL);
	return TRUE;
}
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	MSG msg;
	HACCEL hAccelTable;
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SYSTRAYDEMO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SYSTRAYDEMO));

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int wmId, wmEvent;
	POINT lpClickPoint;

	switch (message) {

	case WM_USER_SHELLICON:
		switch (LOWORD(lParam)) {
		case WM_RBUTTONDOWN:
			UINT uFlag = MF_BYPOSITION | MF_STRING;
			GetCursorPos(&lpClickPoint);
			hPopMenu = CreatePopupMenu();
			//InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_ABOUT, _T("About"));
			if (bDisable == TRUE) {
				uFlag |= MF_GRAYED;
			}
			//InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, IDM_SEP, _T("SEP"));
			InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_EXIT, _T("Exit"));
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
			return TRUE;

		}
		break;
	case WM_TIMER:
		ChangeIcon();
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId) {
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			Shell_NotifyIcon(NIM_DELETE, &nidApp);
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
