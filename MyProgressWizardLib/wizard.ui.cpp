#include "wizard.ui.hpp"
#include "wizard.meta.hpp"
#include "app.pool.hpp"

#include "../static/tool.h"
using namespace std;


static WCHAR szWnclMessaging[256]{}; // wncl = wnd class
static WCHAR szWnclProgWidget[256]{}; // wizard = widget
static HBRUSH hBruProgWidget;
static HFONT hCommonFont;


#pragma region memory utils
static PVOID allocMemoryBySize(size_t nSize) {
	PVOID newmem = ::VirtualAlloc(0, nSize, MEM_COMMIT, PAGE_READWRITE);
	if (!newmem) throw std::exception();
	return newmem;
}
static bool freeMemoryByAddress(PVOID pAddress) {
	return ::VirtualFree(pAddress, 0, MEM_RELEASE);
}
#define allocMemoryByItsType(t) ( reinterpret_cast<t*>( allocMemoryBySize(sizeof(t))))

#pragma endregion



#pragma region UI Message Handlers

static LRESULT CALLBACK UiMsg_Messaging(
	HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam
) {
	HMPRGOBJ hMprgObj = (HMPRGOBJ)(LONG_PTR)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_CREATE: {
		CREATESTRUCT* cr = (CREATESTRUCT*)lParam;
		if (!cr) return -1;
		HMPRGOBJ hObj = *(HMPRGOBJ*)cr->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hObj);
	}
		break;

	case WM_CLOSE:
		break;

	case MYWM_CREATE_WIZARD:
	{
		MPRG_CREATE_PARAMS* param = (MPRG_CREATE_PARAMS*)lParam;
		if (!param) return ERROR_INVALID_PARAMETER;
		MPRG_CREATE_PARAMS cdata{};
		bool gval = [] (MPRG_CREATE_PARAMS* ori, MPRG_CREATE_PARAMS* nw) -> bool {
			__try {
				volatile size_t test = ori->cb;
				test += 1;
				memcpy(nw, ori, sizeof(MPRG_CREATE_PARAMS));
				return true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return false;
			}
		}(param, &cdata);
		if (!gval) return EXCEPTION_ACCESS_VIOLATION;
		freeMemoryByAddress(param);
		param = nullptr;

		// 正式开始创建

		// 1. 创建MPRG_WIZARD_DATA
		PMPRG_WIZARD_DATA pWizData = allocMemoryByItsType(MPRG_WIZARD_DATA);
		if (!pWizData) {
			return ERROR_OUTOFMEMORY;
		}
		pWizData->attrs = new std::map<MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES, LONG_PTR>;

		// 2. 分配属性
		pWizData->cb = sizeof(MPRG_WIZARD_DATA);
		pWizData->width = cdata.width; pWizData->height = cdata.height;
		if (pWizData->width == 0) pWizData->width = 600;
		if (pWizData->height == 0) pWizData->height = 160;
		pWizData->max = cdata.max; pWizData->value = cdata.value;
		if (pWizData->max == (size_t)-1) pWizData->max = 0;
		else if (pWizData->max == 0) pWizData->max = 100;
		if (pWizData->value < 0 || pWizData->value > pWizData->max)
			pWizData->value = 0;
		pWizData->szText = cdata.szText;

		// 3. 查找descriptor
		UiThreadDataDescriptor* desc = mmUiTh2pData.at(mmUiObj2h.at(hMprgObj));

		// 4. 请求一个新的hWizard
		size_t newHandleNum = nMpwizNextUniqueInternalHandleValue++;
		HMPRGWIZ hWiz = (HMPRGWIZ)newHandleNum;

		// 5. 创建窗口
		PCWSTR szTitle = cdata.szTitle;
		if (!szTitle) szTitle = L"Please wait...";
		HWND hNewWnd = CreateWindowExW(0, szWnclProgWidget, szTitle,
			WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_SIZEBOX,
			0, 0, 1, 1, cdata.hParentWindow, 0, 0, 0);

		// 6. 更新记录
		desc->pData->hWiz2hWnd.insert(std::make_pair(hWiz, hNewWnd));
		desc->pData->hWnd2pData.insert(std::make_pair(hNewWnd, pWizData));
		mmUiAssignedWizBelongship.insert(std::make_pair(hWiz, hMprgObj));

		// 7. 初始化窗口
		SendMessage(hNewWnd, MYWM_INIT_WIZARD, 0, (LPARAM)hWiz);

		// 大功告成
		return (LONG_PTR)hWiz;
	}
		break;

	case MYWM_OPEN_WIZARD:
	{
		HWND hwnd = (HWND)lParam;
		if (!IsWindow(hwnd)) return ERROR_INVALID_PARAMETER;
		CenterWindow(hwnd, GetParent(hwnd));
		ShowWindow(hwnd, (int)wParam);
	}
		break;

	case MYWM_DESTROY_WIZARD:
	{
		HWND target = (HWND)lParam;
		if (!target || !IsWindow(target)) return -1;
		ShowWindow(target, SW_HIDE);
		HMPRGWIZ hWiz = (HMPRGWIZ)SendMessageW(target, MYWM_GET_WIZARD_HANDLE, 0, 0);
		DestroyWindow(target);

		try {
			HANDLE hThrd = mmUiObj2h.at(hMprgObj);
			auto pDescriptor = mmUiTh2pData.at(hThrd);
			PMPRG_WIZARD_DATA pData = pDescriptor->pData->hWnd2pData.at(target);

			// 6. 删除记录
			pDescriptor->pData->hWiz2hWnd.erase(hWiz);
			pDescriptor->pData->hWnd2pData.erase(target);
			mmUiAssignedWizBelongship.erase(hWiz);

			// 1. 销毁MPRG_WIZARD_DATA
			if (pData) {
				delete pData->attrs;
				freeMemoryByAddress(pData);
			}
		}
		catch (...) {
			return NULL;
		}
	}
		break;

	case MYWM_DELETE_WIZARD:
	{
		//if (((wParam << 8 | lParam << 5) != 0x4cdce1e0)) return ERROR_ACCESS_DENIED;
		if (wParam != 0x5841 || 0xda64e70f != lParam) return ERROR_WRONG_PASSWORD;
		 
		// 1. 销毁所有与该对象关联的窗口
		vector<HMPRGWIZ> wndsToDestroy;
		for (auto& i : mmUiAssignedWizBelongship) {
			if (i.second == hMprgObj) {
				wndsToDestroy.push_back(i.first);
			}
		}
		for (auto& i : wndsToDestroy) {
			DestroyMprgWizard(hMprgObj, i);
		}

		// 2. 通知退出
		PostQuitMessage(ERROR_REQUEST_ABORTED);

		// 3. 销毁messaging窗口
		DestroyWindow(hwnd);
	}
		break;

	case WM_QUERYENDSESSION:
	case WM_ENDSESSION:
		DeleteMprgObject(hMprgObj);
		return FALSE;
		break;

	case WM_DESTROY:
		PostQuitMessage(ERROR_UNIDENTIFIED_ERROR);
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}
static LRESULT CALLBACK UiMsg_ProgWidget(
	HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam
) {
	HMPRGWIZ hWiz = (HMPRGWIZ)(LONG_PTR)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_CREATE: {
		HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
		EnableMenuItem(hSysMenu, SC_CLOSE, MF_GRAYED | MF_DISABLED);

	}
		break;

	case MYWM_GET_WIZARD_HANDLE:
		return (LRESULT)hWiz;
		break;

	case MYWM_INIT_WIZARD: {
		if (hWiz) return ERROR_ACCESS_DENIED;
		HMPRGWIZ hWiz2 = (HMPRGWIZ)lParam;
		if (!hWiz2) return -1;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hWiz2);

		try {
			// 先用hWizard反查hObject
			HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWiz2);
			// 再通过hObj访问其他数据
			HANDLE hThrd = mmUiObj2h.at(hObj);
			auto pDescriptor = mmUiTh2pData.at(hThrd);
			auto wwdata = pDescriptor->pData->hWnd2pData.at(hwnd);

			SetWindowPos(hwnd, wwdata->attrs->operator[](
				MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::WizAttrTopmost) ==
				1 ? HWND_TOPMOST : HWND_TOP, 0, 0, wwdata->width,
				wwdata->height, SWP_NOACTIVATE);

#define MYCTLS_VAR_HINST NULL
#define MYCTLS_VAR_HFONT hCommonFont
#include "ctls.h"
			wwdata->hwTipText = text(L"Please wait", 0, 0, 1, 1, SS_LEFTNOWORDWRAP);
			wwdata->hwProgBar = custom(L"", PROGRESS_CLASSW);
			wwdata->hwCancelBtn = button(L"Cancel", IDCANCEL);
			EnableWindow(wwdata->hwCancelBtn, false);

			/*RECT rc{}; GetWindowRect(hwnd, &rc);
			SetWindowPos(hwnd, 0, rc.left, rc.top, rc.right - rc.left,
				rc.bottom - rc.top, SWP_FRAMECHANGED | SWP_NOZORDER);*/
			PostMessage(hwnd, WM_SIZE, 0, 0);
			PostMessage(hwnd, MYWM_UPDATE_WIZARD, 0, 0);
		}
		catch (...) {
			return NULL;
		}

	}
		break;

	case WM_SIZING:
	case WM_SIZE:
	{
		if (hWiz) 
		try {
			HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWiz);
			HANDLE hThrd = mmUiObj2h.at(hObj);
			auto pDescriptor = mmUiTh2pData.at(hThrd);
			auto wwdata = pDescriptor->pData->hWnd2pData.at(hwnd);
			
			RECT rc{}; GetClientRect(hwnd, &rc);
			SetWindowPos(wwdata->hwTipText, 0, 10, 10,
				rc.right - rc.left - 20, 20, SWP_NOACTIVATE);
			SetWindowPos(wwdata->hwProgBar, 0, 10, 40,
				rc.right - rc.left - 20, 30, SWP_NOACTIVATE);
			SetWindowPos(wwdata->hwCancelBtn, 0, rc.right-rc.left - 90,
				rc.bottom-rc.top - 40, 80, 30, SWP_NOACTIVATE);
		}
		catch (...) {}
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
		break;

	case MYWM_UPDATE_WIZARD:
	{
		if (hWiz) 
		try {
			HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWiz);
			HANDLE hThrd = mmUiObj2h.at(hObj);
			auto pDescriptor = mmUiTh2pData.at(hThrd);
			auto wwdata = pDescriptor->pData->hWnd2pData.at(hwnd);
			
			auto progLong = GetWindowLong(wwdata->hwProgBar, GWL_STYLE);
			constexpr auto mq = PBS_MARQUEE;
			if (wwdata->max == size_t(-1)) wwdata->max = 0;
			if (
				(((progLong & mq) == mq) && wwdata->max != 0) ||
				(((progLong & mq) == 0) && wwdata->max == 0)
			) {
				if (wwdata->max == 0) {
					progLong |= mq;
					SetWindowLong(wwdata->hwProgBar, GWL_STYLE, progLong);

					SendMessage(wwdata->hwProgBar, PBM_SETRANGE, 0, 2);
					SendMessage(wwdata->hwProgBar, PBM_SETMARQUEE, TRUE, 0);
				}
				else {
					progLong &= ~mq;
					SetWindowLong(wwdata->hwProgBar, GWL_STYLE, progLong);
				}
			}
			if (wwdata->max != 0) {
				SendMessage(wwdata->hwProgBar, PBM_SETSTEP, 1, 0);
				SendMessage(wwdata->hwProgBar, PBM_SETRANGE32, 0, wwdata->max);
				SendMessage(wwdata->hwProgBar, PBM_SETPOS, (WPARAM)wwdata->value, 0);
			}
			bool canCancel = wwdata->attrs->contains(
				MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::WizAttrCancelHandler);
			if (canCancel) canCancel = canCancel && wwdata->attrs->at(
				MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::WizAttrCancelHandler);
			EnableWindow(wwdata->hwCancelBtn, canCancel);
			HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
			EnableMenuItem(hSysMenu, SC_CLOSE, canCancel ? MF_ENABLED :
				(MF_GRAYED | MF_DISABLED));
			if (wwdata->szText)
				SendMessage(wwdata->hwTipText, WM_SETTEXT, 0, (LPARAM)wwdata->szText);
		}
		catch (...) {}
	}
		break;

	case MYWM_SET_WIZARD_VALUE:
	case MYWM_STEP_WIZARD_VALUE:
	{
		if (hWiz) 
		try {
			HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWiz);
			HANDLE hThrd = mmUiObj2h.at(hObj);
			auto pDescriptor = mmUiTh2pData.at(hThrd);
			auto wwdata = pDescriptor->pData->hWnd2pData.at(hwnd);
			
			wwdata->value = lParam;
			if (wwdata->max != 0)
			SendMessage(wwdata->hwProgBar, message == MYWM_SET_WIZARD_VALUE ?
				PBM_SETPOS : PBM_STEPIT, (WPARAM)lParam, 0);
		}
		catch (...) {}
	}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
		break;

	case WM_CLOSE:
	{
		if (hWiz) 
		try {
			HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWiz);
			HANDLE hThrd = mmUiObj2h.at(hObj);
			auto pDescriptor = mmUiTh2pData.at(hThrd);
			auto wwdata = pDescriptor->pData->hWnd2pData.at(hwnd);

			void* pfn = (PVOID)wwdata->attrs->at(
				MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::WizAttrCancelHandler);
			typedef bool(__stdcall* CancelHandler)(HMPRGWIZ hWizard, HMPRGOBJ hObj);
			CancelHandler cancelHandler = (CancelHandler)pfn;
			bool lambda_result = [&cancelHandler, hWiz, hObj] {
				__try {
					return cancelHandler(hWiz, hObj);
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return false;
				}
			}();
			if (lambda_result) {
				// the operation is being cancelled, clear handler
				SetMprgWizAttribute(hWiz, MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::
					WizAttrCancelHandler, 0);
			}
		}
		catch (...) {}
	}
		break;

	case WM_QUERYENDSESSION:
	case WM_ENDSESSION:
		return FALSE;
		break;

	case WM_DESTROY:
		return DefWindowProc(hwnd, message, wParam, lParam);
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}


#pragma endregion


#pragma region UI Thread Code
typedef struct {

} MPRG_UI_ThreadData;
DWORD WINAPI MPRG_UI_Thread(PVOID param) {
	UiThreadDataDescriptor* desc = (UiThreadDataDescriptor*)param;
	UiThreadDataContainer* pContainer = new UiThreadDataContainer();
	desc->pData = pContainer;

	desc->messagingWindow = CreateWindowExW(0, szWnclMessaging, L"My Progress Wizard: "
		"Interthreadational Messaging Window", WS_OVERLAPPED, 0, 0, 1, 1, 0, 0, 0,
		&desc->hMprgObjValue);
	if (!desc->messagingWindow) return GetLastError();

	MSG msg{};
	if (desc->shouldQuit) ExitThread(ERROR_CANCELLED);
	SetEvent(desc->hEvent_communication_readyEvent);
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	
	// 收尾
	delete pContainer;

	return (int)msg.wParam;
	return 0;
}
DWORD WINAPI MPRG_UIThreadLauncher(PVOID param); // in threading.cpp
#pragma endregion





#pragma comment(lib, "Comctl32.lib")
bool InitMprgComponent() {
	static bool bHasInited = false;
	if (bHasInited) return true;

	DWORD dwSuccess = FALSE;

	wcscpy_s(szWnclMessaging, IDS_STRING_WND_CLASS_THDMESSAGINGWINDOW);
	if (0 == s7::MyRegisterClassW(szWnclMessaging, UiMsg_Messaging)) {
		return false;
	}

	wcscpy_s(szWnclProgWidget, IDS_STRING_WND_CLASS_PROGWIDGET);
	if (!(hBruProgWidget = CreateSolidBrush(RGB(0xF0, 0xF0, 0xF0)))) {
		return false;
	}
	if (0 == s7::MyRegisterClassW(szWnclProgWidget, UiMsg_ProgWidget, WNDCLASSEXW{
		.hbrBackground = hBruProgWidget,
		})) {
		return false;
	}


	INITCOMMONCONTROLSEX icce{
		.dwSize = sizeof(icce),
		.dwICC = ICC_PROGRESS_CLASS,
	};
	InitCommonControlsEx(&icce);


	hCommonFont = CreateFontW(-14, -7, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
		L"Consolas");


	bHasInited = true;
	return true;
}



extern "C" HMPRGOBJ CreateMprgObject() {
	// allocate a UiThreadDataDescriptor object
	UiThreadDataDescriptor* pDescriptor = allocMemoryByItsType(UiThreadDataDescriptor);

	// allocate a UiThreadDataContainer object
	//UiThreadDataContainer* pContainer = allocMemoryByItsType(UiThreadDataContainer);

	// fill the descriptor
	pDescriptor->cb = sizeof(UiThreadDataDescriptor);
	//pDescriptor->pData = pContainer;
	pDescriptor->hEvent_communication_readyEvent = CreateEvent(0, true, false, 0);

	// create the thread
	HANDLE hThread = CreateThread(NULL, 0, MPRG_UIThreadLauncher,
		pDescriptor, CREATE_SUSPENDED, 0);

	// check the result
	if (!hThread) {
		DWORD errCode = GetLastError();
		// remember to free the memory that is no longer useful
		freeMemoryByAddress(pDescriptor);
		//freeMemoryByAddress(pContainer);
		// set the error code
		SetLastError(errCode);
		// failed
		return nullptr;
	}

	// assign the thread
	mmUiTh2pData.insert(std::pair<HANDLE, UiThreadDataDescriptor*>
		(hThread, pDescriptor));
	HMPRGOBJ hObj = HMPRGOBJ(nMpwizNextUniqueInternalHandleValue++);
	//reinterpret_cast<HMPRGOBJ>(hThread);
	mmUiObj2h.insert(std::make_pair(hObj, hThread));
	pDescriptor->hMprgObjValue = hObj;

	// resume the thread.
	ResumeThread(hThread);

	// return the handle. 
	//// I am lazy so I just used the hThread as the HMPRGOBJ.
	//// However, you shouldn't use the return value as a normal handle
	//// because the meaning of HMPRGOBJ might change in the future.
	// 2024-2-1: changed the hObj generator.
	return hObj;
}

extern "C" HMPRGWIZ CreateMprgWizard(
	HMPRGOBJ hObject, MPRG_CREATE_PARAMS params, DWORD dwTimeout
) {
	HANDLE hThrd = mmUiObj2h.at(hObject);
	auto pDescriptor = mmUiTh2pData.at(hThrd);
	HWND hMessaging = pDescriptor->messagingWindow;

	if (!hMessaging) {
		// 等待线程准备完成
		HANDLE hwaits[2]{ hThrd,pDescriptor->hEvent_communication_readyEvent };
		DWORD waitResult = WaitForMultipleObjects(2, hwaits, false, dwTimeout);
		if (waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED) {
			SetLastError(waitResult);
			return NULL;
		}
		GetExitCodeThread(hThrd, &waitResult);
		if (waitResult != STILL_ACTIVE) {
			SetLastError(ERROR_NOT_FOUND);
			return NULL;
		}
		hMessaging = pDescriptor->messagingWindow;
		if (!hMessaging) {
			SetLastError(ERROR_UNIDENTIFIED_ERROR);
			return NULL;
		}
	}

	DWORD_PTR result = 0;
	MPRG_CREATE_PARAMS* newmem = allocMemoryByItsType(MPRG_CREATE_PARAMS);
	memcpy_s(newmem, sizeof(MPRG_CREATE_PARAMS), &params, sizeof(MPRG_CREATE_PARAMS));
	if (!SendMessageTimeoutW(hMessaging, MYWM_CREATE_WIZARD, 0x1,
		(LPARAM)(PVOID)(newmem), SMTO_ABORTIFHUNG, 5000, &result)) {
		freeMemoryByAddress(newmem);
		return NULL;
	}

	HMPRGWIZ hWiz = (HMPRGWIZ)result;

	return hWiz;
}


extern "C" HWND GetMprgHwnd(HMPRGWIZ hWizard) {
	try {
		// 先用hWizard反查hObject
		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		// 再通过hObj访问其他数据
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);

		return pDescriptor->pData->hWiz2hWnd.at(hWizard);
	}
	catch (...) {
		return NULL;
	}
}

extern "C" PMPRG_WIZARD_DATA GetModifiableMprgWizardData(HMPRGWIZ hWizard) {
	try {
		// 先用hWizard反查hObject
		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		// 再通过hObj访问其他数据
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);

		return pDescriptor->pData->hWnd2pData.at(
			pDescriptor->pData->hWiz2hWnd.at(hWizard));
	}
	catch (...) {
		return NULL;
	}
}


extern "C" const PMPRG_WIZARD_DATA GetMprgWizardData(HMPRGWIZ hWizard) {
	return GetModifiableMprgWizardData(hWizard);
}


extern "C" LONG_PTR GetMprgWizAttribute(HMPRGWIZ hWizard,
	MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES nAttrName
){
	try {
		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);
		auto cdata = pDescriptor->pData->hWnd2pData.at(
			pDescriptor->pData->hWiz2hWnd.at(hWizard));
		return cdata->attrs->operator[](nAttrName);
	}
	catch (...) {
		return NULL;
	}
}

extern "C" LONG_PTR SetMprgWizAttribute(HMPRGWIZ hWizard,
	MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES nAttrName, LONG_PTR newValue
) {
	try {
		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);
		auto cdata = pDescriptor->pData->hWnd2pData.at(
			pDescriptor->pData->hWiz2hWnd.at(hWizard));
		LONG_PTR old = cdata->attrs->operator[](nAttrName);
		(*(cdata->attrs))[nAttrName] = newValue;
		UpdateMprgWizard(hWizard);
		return old;
	}
	catch (...) {
		return NULL;
	}
}


static unsigned long long GetCurrentSystemTime_ms() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	ULARGE_INTEGER li{};
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	return li.QuadPart / 10000;
}


static unsigned long long wizard_last_update_time__text = 0;
static unsigned long long wizard_last_update_time__value = 0;


extern "C" bool SetMprgWizardValue(
	HMPRGWIZ hWizard, size_t currentValue, bool bForceUpdate
) {
	try {
		if (!bForceUpdate) {
			auto currentTime = GetCurrentSystemTime_ms();
			if (currentTime - wizard_last_update_time__value
				< MINIMIUM_WIZARD_UPDATE_TIME) {
				return true;
			}
			wizard_last_update_time__value = currentTime;
		}

		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);
		auto hwnd = pDescriptor->pData->hWiz2hWnd.at(hWizard);
		return PostMessage(hwnd, MYWM_SET_WIZARD_VALUE, 0, currentValue);
	}
	catch (...) {
		return false;
	}
}


extern "C" bool StepMprgWizardValue(HMPRGWIZ hWizard, bool bForceUpdate) {
	try {
		if (!bForceUpdate) {
			auto currentTime = GetCurrentSystemTime_ms();
			if (currentTime - wizard_last_update_time__value
				< MINIMIUM_WIZARD_UPDATE_TIME) {
				return true;
			}
			wizard_last_update_time__value = currentTime;
		}

		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);
		auto hwnd = pDescriptor->pData->hWiz2hWnd.at(hWizard);
		auto wd = pDescriptor->pData->hWnd2pData.at(hwnd);
		return PostMessage(hwnd, MYWM_STEP_WIZARD_VALUE, 0, 0);
	}
	catch (...) {
		return false;
	}
}


extern "C" bool SetMprgWizardText(HMPRGWIZ hWizard, PCWSTR psz, bool bForceUpdate) {
	try {
		if (!bForceUpdate) {
			auto currentTime = GetCurrentSystemTime_ms();
			if (currentTime - wizard_last_update_time__text
				< MINIMIUM_WIZARD_UPDATE_TIME) {
				return true;
			}
			wizard_last_update_time__text = currentTime;
		}

		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);
		auto hwnd = pDescriptor->pData->hWiz2hWnd.at(hWizard);
		auto wd = pDescriptor->pData->hWnd2pData.at(hwnd);
		wd->szText = psz;
		return UpdateMprgWizard(hWizard);
	}
	catch (...) {
		return false;
	}
}


extern "C" bool UpdateMprgWizard(HMPRGWIZ hWizard) {
	try {
		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);
		auto hwnd = pDescriptor->pData->hWiz2hWnd.at(hWizard);
		return SendMessageTimeoutW(hwnd, MYWM_UPDATE_WIZARD, 0, (LPARAM)pDescriptor,
			SMTO_ABORTIFHUNG | SMTO_BLOCK, 500, NULL);
	}
	catch (...) {
		return NULL;
	}
}


extern "C"
bool OpenMprgWizard(HMPRGWIZ hWizard, int nShow) {
	try {
		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);

		return SendMessageTimeoutW(pDescriptor->messagingWindow, MYWM_OPEN_WIZARD,
			nShow, (LPARAM)pDescriptor->pData->hWiz2hWnd.at(hWizard),
			SMTO_NORMAL, 5000, NULL);
	}
	catch (...) {
		return false;
	}
}

extern "C" bool HideMprgWizard(HMPRGWIZ hWizard) {
	return OpenMprgWizard(hWizard, SW_HIDE);
}


extern "C" bool DestroyMprgWizard(HMPRGOBJ hObject, HMPRGWIZ hWizard) {
	try {
		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		HANDLE hThrd = mmUiObj2h.at(hObj);
		auto pDescriptor = mmUiTh2pData.at(hThrd);

		return SendMessageTimeoutW(pDescriptor->messagingWindow, MYWM_DESTROY_WIZARD,
			0xc0000005, (LPARAM)pDescriptor->pData->hWiz2hWnd.at(hWizard),
			SMTO_NORMAL, 5000, NULL);
	}
	catch (...) {
		return false;
	}
}


extern "C"
DWORD DeleteMprgObject(HMPRGOBJ hObject, bool bForceTerminateIfTimeout) {
	HANDLE hThrd = mmUiObj2h.at(hObject);
	auto pDescriptor = mmUiTh2pData.at(hThrd);
	PostMessage(pDescriptor->messagingWindow, MYWM_DELETE_WIZARD, 0x5841, 0xda64e70f);
	DWORD waitResult = WaitForSingleObject(hThrd, 30000);
	if (waitResult == WAIT_FAILED || waitResult == WAIT_TIMEOUT) {
		if (!bForceTerminateIfTimeout) return waitResult;
		// 进行麻烦的工作
		delete pDescriptor->pData;
	}

	// 进行一些收尾

	// 取消关联
	mmUiTh2pData.erase(hThrd);
	mmUiObj2h.erase(hObject);

	// 关闭句柄
	if (pDescriptor->hEvent_communication_readyEvent)
		CloseHandle(pDescriptor->hEvent_communication_readyEvent);

	// 释放内存
	freeMemoryByAddress(pDescriptor);

	return 0;
}



extern "C" HMPRGOBJ GetMprgObjectByWizard(HMPRGWIZ hWizard) {
	try {
		HMPRGOBJ hObj = mmUiAssignedWizBelongship.at(hWizard);
		return hObj;
	}
	catch (...) {
		return nullptr;
	}
}







#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

