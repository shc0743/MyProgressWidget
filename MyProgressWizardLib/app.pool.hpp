#pragma once
#include <Windows.h>
#include <map>
#include <vector>
#include <atomic>
#include <map>
#include <unordered_map>
#include "wizard.user.h"


extern std::atomic<size_t> nMpwizNextUniqueInternalHandleValue; // to avoid bugs in multithreaded programs
using HWiz2HWND = std::map<HMPRGWIZ, HWND>;
using HWND2PDATA = std::map<HWND, PMPRG_WIZARD_DATA>;
using UiThreadDataItem = class {
public:
	HWiz2HWND hWiz2hWnd;
	HWND2PDATA hWnd2pData;
};
using UiThreadDataContainer = UiThreadDataItem;//std::vector<UiThreadDataItem>;
using UiThreadDataDescriptor = struct UiThreadDataDescriptor__ {
	DWORD cb;
	UiThreadDataContainer* pData;
	HWND messagingWindow;
	bool shouldQuit;
	HANDLE hEvent_communication_readyEvent;
	HMPRGOBJ hMprgObjValue;
};


extern std::map<HANDLE, UiThreadDataDescriptor*> mmUiTh2pData; // ui thread handle to Data*
extern std::map<HMPRGOBJ, HANDLE> mmUiObj2h; // HMPRGOBJ to thread handle
extern std::map<HMPRGWIZ, HMPRGOBJ> mmUiAssignedWizBelongship; // HMPRGWIZ to HMPRGOBJ

