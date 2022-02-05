#include "framework.h"
#include <Wbemidl.h>
#include <comdef.h>
#include <iostream>
#include <powersetting.h>
#include <powrprof.h>
#include <sstream>
#include <string>
#include <windows.h>
#include <shellapi.h>
#include "CPUFreq_cpp.h"
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "powrprof.lib")

#define MAX_LOADSTRING 100
#define WM_USER_SHELLICON WM_USER + 1
#define _WIN32_DCOM

using namespace std;

HINSTANCE hInst;
NOTIFYICONDATA nidApp;
HMENU hPopMenu;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szApplicationToolTip[MAX_LOADSTRING];
BOOL bDisable = FALSE;
IWbemLocator* pLoc = NULL;
IWbemServices* pSvc = NULL;
double cpuMaxFreq = 0;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
HICON CreateImageIcon(COLORREF iconColor, int freq, int usage, int mem);
void ChangeIcon();
double GetCPUFrequecyPercent();
double GetCPUMaxFrequecy();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
int initWMIConnection();
void finalizeWMIConnection();
void changePowerPlan(int id);
void createContextMenu();
int setStartUp(void);

int initWMIConnection()
{
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
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
        NULL);

    if (FAILED(hres)) {
        CoUninitialize();
        return 0;
    }
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres)) {
        CoUninitialize();
        return 0;
    }
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc);

    if (FAILED(hres)) {
        finalizeWMIConnection();
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
        EOAC_NONE);

    if (FAILED(hres)) {
        finalizeWMIConnection();
        return 0;
    }

    return 1;
}
void finalizeWMIConnection()
{
    pLoc->Release();
    pSvc->Release();
    CoUninitialize();
}
double GetCPUFrequecyPercent()
{
    if (pLoc == NULL || pSvc == NULL)
        return 0;
    HRESULT hres;
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM  Win32_PerfFormattedData_Counters_ProcessorInformation"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres)) {
        finalizeWMIConnection();
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

    pEnumerator->Release();
    return cpufreqs / cpucount;
}
double GetCPUMaxFrequecy()
{
    if (pLoc == NULL || pSvc == NULL)
        return 0;
    HRESULT hres;
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
            &uReturn);
        if (uReturn == 0) {
            break;
        }
        VARIANT vtProp;
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
        cpufreqs = (unsigned int)vtProp.bstrVal;
        VariantClear(&vtProp);
        pclsObj->Release();
    }

    pEnumerator->Release();
    return cpufreqs;
}
static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
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
double GetCPUUsage()
{
    FILETIME idleTime, kernelTime, userTime;
    return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
}
int GetMemoryUsage()
{
    MEMORYSTATUSEX ms = { sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&ms);
    return ms.dwMemoryLoad;
}
void ChangeIcon()
{
    double percent = GetCPUFrequecyPercent();
    double usage = GetCPUUsage();
    int mem = GetMemoryUsage();

    int f = 16 - ceil(16 * percent / 100);
    int u = 16 - ceil(16 * usage);
    int m = 16 - ceil(16 * mem / 100);

    HICON icon = CreateImageIcon(0x0000FF00, f, u, m);
    nidApp.hIcon = icon;
    Shell_NotifyIcon(NIM_MODIFY, &nidApp);

    DestroyIcon(icon);
}
HICON CreateImageIcon(COLORREF iconColor, int freq, int usage, int mem)
{
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

    SelectObject(hdcMem, hbmpOld);
    DeleteObject(hbmp);
    DeleteObject(hbmpMask);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return hIcon;
}
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SYSTRAYDEMO);
    wcex.lpszClassName = L"CPU_Freq_cpp";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    return RegisterClassEx(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    hInst = hInstance;
    hWnd = CreateWindow(L"CPU_Freq_cpp", szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 400, 330, NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return FALSE;
    }
    initWMIConnection();
    cpuMaxFreq = GetCPUMaxFrequecy();
    HICON icon = CreateImageIcon(0x0000FF00, 0, 0, 0);

    nidApp.cbSize = sizeof(NOTIFYICONDATA);
    nidApp.hWnd = (HWND)hWnd;
    nidApp.uID = IDI_SYSTRAYDEMO;
    nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nidApp.hIcon = icon;
    nidApp.uCallbackMessage = WM_USER_SHELLICON;
    LoadString(hInstance, IDS_APPTOOLTIP, nidApp.szTip, MAX_LOADSTRING);
    createContextMenu();
    Shell_NotifyIcon(NIM_ADD, &nidApp);
    SetTimer(hWnd, 1, 1000, NULL);
    DestroyIcon(icon);

    return TRUE;
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SYSTRAYDEMO));
    MSG msg;
    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
void changePowerPlan(int id)
{
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    wchar_t path[] = L"C:\\Windows\\System32\\cmd.exe";

    switch (id) {
    case IDM_HIGH: {

        CheckMenuItem(hPopMenu, IDM_HIGH, MFS_CHECKED);
        CheckMenuItem(hPopMenu, IDM_BALANCED, MFS_UNCHECKED);
        CheckMenuItem(hPopMenu, IDM_SAVE, MFS_UNCHECKED);
        TCHAR param1[] = L"C:\\Windows\\System32\\cmd.exe /K C:\\Windows\\System32\\powercfg.exe -setactive 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c";
        CreateProcess(path, param1, NULL, NULL, false, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    } break;
    case IDM_SAVE: {

        CheckMenuItem(hPopMenu, IDM_HIGH, MFS_UNCHECKED);
        CheckMenuItem(hPopMenu, IDM_BALANCED, MFS_UNCHECKED);
        CheckMenuItem(hPopMenu, IDM_SAVE, MFS_CHECKED);
        TCHAR param2[] = L"C:\\Windows\\System32\\cmd.exe /K C:\\Windows\\System32\\powercfg.exe -setactive a1841308-3541-4fab-bc81-f71556f20b4a";
        CreateProcess(path, param2, NULL, NULL, false, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    } break;
    default: {

        CheckMenuItem(hPopMenu, IDM_HIGH, MFS_UNCHECKED);
        CheckMenuItem(hPopMenu, IDM_BALANCED, MFS_CHECKED);
        CheckMenuItem(hPopMenu, IDM_SAVE, MFS_UNCHECKED);
        TCHAR param3[] = L"C:\\Windows\\System32\\cmd.exe /K C:\\Windows\\System32\\powercfg.exe -setactive 381b4222-f694-41f0-9685-ff5bb260df2e";
        CreateProcess(path, param3, NULL, NULL, false, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    }
    }

    if (::WaitForSingleObject(pi.hProcess, 500) == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 0);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}
void createContextMenu()
{
    hPopMenu = CreatePopupMenu();
    InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING | MFS_UNCHECKED, IDM_HIGH, _T("Power Plan : High performance"));
    InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING | MFS_UNCHECKED, IDM_BALANCED, _T("Power Plan : Balanced"));
    InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING | MFS_UNCHECKED, IDM_SAVE, _T("Power Plan : Power saver"));
    InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, IDM_SEP, _T("SEP"));
    InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_ABOUT, _T("About"));
    InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_EXIT, _T("Exit"));
}
int setStartUp(void)
{
    TCHAR szPath[MAX_PATH];
    DWORD pathLen = 0;
    pathLen = GetModuleFileName(NULL, szPath, MAX_PATH);
    if (pathLen == 0) {
        OutputDebugString(L"Error1");
        return -1;
    }
    HKEY newValue;
    if (RegOpenKey(HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
            &newValue)
        != ERROR_SUCCESS) {
        OutputDebugString(L"Error2");
        return -1;
    }
    DWORD pathLenInBytes = pathLen * sizeof(*szPath);
    if (RegSetValueEx(newValue,
            TEXT("CPU_Freq_cpp"),
            0,
            REG_SZ,
            (LPBYTE)szPath,
            pathLenInBytes)
        != ERROR_SUCCESS) {
        RegCloseKey(newValue);
        return -1;
    }

    RegCloseKey(newValue);

    return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    POINT lpClickPoint;

    switch (message) {
    case WM_USER_SHELLICON:
        switch (LOWORD(lParam)) {
        case WM_RBUTTONDOWN:
            UINT uFlag = MF_BYPOSITION | MF_STRING;
            GetCursorPos(&lpClickPoint);
            if (bDisable == TRUE) {
                uFlag |= MF_GRAYED;
            }

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

        case IDM_HIGH:
            changePowerPlan(IDM_HIGH);
            break;
        case IDM_BALANCED:
            changePowerPlan(IDM_BALANCED);
            break;
        case IDM_SAVE:
            changePowerPlan(IDM_SAVE);
            break;
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
        finalizeWMIConnection();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (LOWORD(wParam) == IDOK) {
                setStartUp();
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
