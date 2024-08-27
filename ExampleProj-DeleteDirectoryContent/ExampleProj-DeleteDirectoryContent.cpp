// ExampleProj-DeleteDirectoryContent.cpp : 
// 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "../static/tool.h"
using namespace std;

#pragma comment(linker, "/entry:mainCRTStartup /subsystem:windows")

#pragma comment(lib, "../bin/MyProgressWizardLib64.lib")
#include "../MyProgressWizardLib/wizard.user.h"

#define MyAssert(x) if (!x) return GetLastError();


HMPRGOBJ hObj;
vector<wstring> vFilesToBeDeleted;
wchar_t vFilesToBeDeletedBuffer[2048]{};
wchar_t* vFilesToBeDeletedBufferPtr1 = vFilesToBeDeletedBuffer + 1;
void(__stdcall * ComputeDeleteContentOnIncrease)();
void(__stdcall * ComputeDeleteContentOnIncrease_Real)();
signed char __stdcall ComputeDeleteContentWorker(
	std::wstring szPath
) {
	signed char status = 0;
	WIN32_FIND_DATAW findd{};
	HANDLE hFind = FindFirstFileW((szPath + L"*").c_str(), &findd);
	if (!hFind || hFind == INVALID_HANDLE_VALUE) {
		status = -1;
		return status;
	}
	do {
		if (wcscmp(findd.cFileName, L".") == 0 ||
			wcscmp(findd.cFileName, L"..") == 0) continue;
		wstring wstrFileName(szPath);
		wstrFileName.append(findd.cFileName);
		if (findd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			wstrFileName.append(L"\\");
			auto result0 = ComputeDeleteContentWorker(wstrFileName);
			if (result0 != 0) status = (status == -1) ? -1 : 1;
		}
		else {
			if (findd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
				SetFileAttributesW(wstrFileName.c_str(), FILE_ATTRIBUTE_NORMAL);
			vFilesToBeDeletedBuffer[0] = L'\1';
			wcscpy_s(vFilesToBeDeletedBufferPtr1, 2047, wstrFileName.c_str());
			vFilesToBeDeleted.push_back(vFilesToBeDeletedBuffer);
			if (ComputeDeleteContentOnIncrease) ComputeDeleteContentOnIncrease();
		}
	} while (FindNextFileW(hFind, &findd));
	FindClose(hFind);
	vFilesToBeDeletedBuffer[0] = L'\2';
	wcscpy_s(vFilesToBeDeletedBufferPtr1, 2047, szPath.c_str());
	vFilesToBeDeleted.push_back(vFilesToBeDeletedBuffer);
	if (ComputeDeleteContentOnIncrease) ComputeDeleteContentOnIncrease();
	return status;
}
signed char __stdcall ComputeDeleteContent(
	std::wstring szPath
) {
	signed char status = 0;
	if (true) {
		vFilesToBeDeleted.clear();
		if (szPath.find(L"/") != wstring::npos) str_replace(szPath, L"/", L"\\");
		if (!szPath.ends_with(L"\\")) szPath += L"\\";
	}
	WIN32_FIND_DATAW findd{};
	HANDLE hFind = FindFirstFileW((szPath + L"*").c_str(), &findd);
	if (!hFind || hFind == INVALID_HANDLE_VALUE) {
		status = -1;
		return status;
	}
	do {
		if (wcscmp(findd.cFileName, L".") == 0 ||
			wcscmp(findd.cFileName, L"..") == 0) continue;
		wstring wstrFileName(szPath);
		//wstrFileName.append(L"\\");
		wstrFileName.append(findd.cFileName);
		if (findd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			wstrFileName.append(L"\\");
			auto result0 = ComputeDeleteContentWorker(wstrFileName);
			if (result0 != 0) status = (status == -1) ? -1 : 1;
		}
		else {
			vFilesToBeDeletedBuffer[0] = L'\1';
			wcscpy_s(vFilesToBeDeletedBufferPtr1, 2047, wstrFileName.c_str());
			vFilesToBeDeleted.push_back(vFilesToBeDeletedBuffer);
			if (ComputeDeleteContentOnIncrease) ComputeDeleteContentOnIncrease();
		}
	} while (FindNextFileW(hFind, &findd));
	FindClose(hFind);
	return status;
}

HMPRGWIZ ghGlobalWiz;
bool ghClearDeleteContentAntiTik;
DWORD WINAPI fClearDeleteContentAntiTik(PVOID) {
	Sleep(50);
	ComputeDeleteContentOnIncrease_Real();
	ghClearDeleteContentAntiTik = false;
	return 0;
}
bool ComputeDeleteContent_isCancelled;
bool __stdcall ComputeDeleteContentCancelHandler(HMPRGWIZ hWiz, HMPRGOBJ hObj) {
	ComputeDeleteContent_isCancelled = true;
	SetMprgWizardText(hWiz, L"Canceling...");
	return true;
}
#pragma comment(lib, "Winmm.lib")
bool RunSecurityCheckForDelete(wstring pDirName) {
	bool dangerous = false;
	wchar_t WinSysDir[128]{};
	(void)GetWindowsDirectoryW(WinSysDir, 127);
	wcscat_s(WinSysDir, L"\\");
	str_replace(pDirName, L"/", L"\\");
	for (size_t i = 0, l = pDirName.length(); i < l; ++i) {
		pDirName[i] = towlower(pDirName[i]);
	}
	for (size_t i = 0, l = wcslen(WinSysDir); i < l; ++i) {
		WinSysDir[i] = towlower(WinSysDir[i]);
	}

	if (!pDirName.ends_with(L"\\")) pDirName.append(L"\\");
	if (pDirName == WinSysDir) dangerous = true;

	if ((!dangerous) && pDirName.starts_with(WinSysDir)) {
		wcscat_s(WinSysDir, L"temp\\");
		if (!pDirName.starts_with(WinSysDir)) dangerous = true;
	}
	
	WinSysDir[3] = 0;
	if ((!dangerous) && (pDirName == (WinSysDir))) dangerous = true;

	if ((!dangerous) && pDirName.starts_with(WinSysDir)) {
		if (
			pDirName.ends_with(L"\\users\\") ||
			pDirName.ends_with(L"\\program files\\") ||
			pDirName.ends_with(L"\\program files (x86)\\") ||
			pDirName.ends_with(L"\\programdata\\") ||
			pDirName.ends_with(L"\\system volume information\\")
		) dangerous = true;
	}
	

	if (!dangerous) return true;

	int nf = 0;
	wstring warnText = L"(!) Bad things will happen if you don't look this!\n\n"
		L"You're trying to DELETE ALL FILES IN ";
	warnText.append(pDirName);
	warnText += L" .\nUsually, the operation is VERY DANGEROUS.\nIf continue, "
		"your computer ";
	warnText.append((pDirName.find(L"windows") != wstring::npos ||
		pDirName.length() == 2) ? L"will" : L"might");
	warnText += L" stop working and might be broken.\nYou will have to re-inst"
		"all Windows or many programs.\n\nIf you really want to cont"
		"inue for some other reasons, please click Retry button.";
	CloseHandleIfOk(CreateThread(0, 0, [](PVOID)->DWORD {return
		PlaySoundW((LPCWSTR)SND_ALIAS_SYSTEMHAND, 0, SND_ALIAS_ID);
	}, 0, 0, 0));
	TaskDialog(GetForegroundWindow(), 0, L"Warning - Delete Directory Content",
		L"Dangerous operation!!", warnText.c_str(), TDCBF_RETRY_BUTTON |
		TDCBF_YES_BUTTON | TDCBF_CANCEL_BUTTON, IDI_ERROR, &nf);

	if (nf != IDRETRY) return false;

	wstring rrb;
	retry1:
	if (!AllocConsole()) return MessageBox(0, LastErrorStr().c_str(),
		0, MB_ICONHAND) && 0;
	DWORD dwWritten = 0;
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"!!WARNING!!\nYou are trying to "
		"perform a dangerous operation.\nTo continue, please type the following code "
		"and press Enter (the \"l\" is number \"one\") :\n", 149, &dwWritten, 0);
	ZeroMemory(WinSysDir, sizeof(WinSysDir));
	srand((UINT)time(0));
	rrb = GenerateRandomString(16, L"0123456789abcdef"
		"ghjkmnopqrstuvwxyz"s) + L"\r";
	wcscat_s(WinSysDir, rrb.c_str());
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), WinSysDir,
		DWORD((wcslen(WinSysDir))), &dwWritten, 0);
	char sInBuf[128]{};
	ReadConsoleA(GetStdHandle(STD_INPUT_HANDLE), sInBuf, 128, &dwWritten, 0);
	sInBuf[16] = 0;
	WinSysDir[16] = 0;
	if ((WinSysDir != s2ws(sInBuf))) {
		FreeConsole();
		if (MessageBoxW(GetForegroundWindow(), L"Incorrect validation text.", 0,
			MB_ICONERROR | MB_RETRYCANCEL) == IDRETRY) goto retry1;
		else return false;
	}
	FreeConsole();

	if ((WinSysDir == s2ws(sInBuf))) return true;
	return false;
}
DWORD WINAPI MainThread(PVOID vpstr) {
	wstring pDirName = s2ws((PCSTR)vpstr);
	wstring szTitle = pDirName + L" - Delete Directory Content"s;

	HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
		.cb = sizeof(MPRG_CREATE_PARAMS),
		.szTitle = szTitle.c_str(),
		.szText = L"Processing your request",
		.max = 1000000,
	});
	MyAssert(hWiz);

	OpenMprgWizard(hWiz);

	if (!RunSecurityCheckForDelete(pDirName)) {
		MessageBoxTimeoutW(GetMprgHwnd(hWiz), L"Dangerous operation.", 0,
			MB_ICONERROR, 0, 2000);
		return ERROR_REQUEST_ABORTED;
	}

	wstring szText = L"Are you sure you want to delete ALL FILES in "s 
		+ pDirName + L"?";
	if (MessageBoxW(GetMprgHwnd(hWiz), szText.c_str(), szTitle.c_str(),
		MB_ICONWARNING | MB_YESNO) != IDYES) {
		DestroyMprgWizard(hObj, hWiz);
		return ERROR_CANCELLED;
	}

	auto wizData = GetModifiableMprgWizardData(hWiz);
	if (!wizData) {
		MessageBoxW(GetMprgHwnd(hWiz), L"Internal Error", NULL, MB_ICONERROR);
		DestroyMprgWizard(hObj, hWiz);
		return GetLastError();
	}
	wizData->max = 0;
	UpdateMprgWizard(hWiz);

	SetMprgWizardText(hWiz, L"Computing files to delete");
	ghGlobalWiz = hWiz;
	SetMprgWizAttribute(hWiz, MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::
		WizAttrCancelHandler, (LONG_PTR)(void*)ComputeDeleteContentCancelHandler);
	ComputeDeleteContentOnIncrease = []()->void {
		if (ghClearDeleteContentAntiTik) return;
		ghClearDeleteContentAntiTik = true;
		HANDLE hThread = CreateThread(0, 0, fClearDeleteContentAntiTik, 0, 0, 0);
		if (!hThread) {
			ghClearDeleteContentAntiTik = false; return;
		}
		CloseHandle(hThread);
	};
	ComputeDeleteContentOnIncrease_Real = []()->void {
		static wstring ws;
		if (ComputeDeleteContent_isCancelled) return;
		ws = L"Computing files to delete | Found: ";
		ws.append(to_wstring(vFilesToBeDeleted.size()));
		SetMprgWizardText(ghGlobalWiz, ws.c_str());
	};
	ComputeDeleteContent(pDirName);
	Sleep(1000);
	SetMprgWizAttribute(hWiz, MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::
		WizAttrCancelHandler, 0);
	if (ComputeDeleteContent_isCancelled) {
		DestroyMprgWizard(hObj, hWiz);
		return ERROR_CANCELLED;
	}

	// delete them
	PCWSTR pszBuffer = NULL; wstring wsBuffer1;
	SetMprgWizardText(hWiz, L"Deleting");
	wizData->max = 1000000;
	UpdateMprgWizard(hWiz);
	time_t lastUpdateTime = 0; size_t n = 0,
		vFilesToBeDeletedSize = vFilesToBeDeleted.size();
	for (auto& i : vFilesToBeDeleted) {
		++n;
		pszBuffer = i.c_str() + 1;
		if (n - lastUpdateTime >= 10 || n == vFilesToBeDeletedSize) {
			lastUpdateTime = n;
			double bfb = double(UINT((double(n) /
				double(vFilesToBeDeletedSize)) * 100000000)) / 1000000;
			wsBuffer1 = to_wstring(bfb) + L"% (" + to_wstring(n) + L"/"
				+ to_wstring(vFilesToBeDeletedSize) + L") Deleting ";
			wsBuffer1.append(pszBuffer);
			SetMprgWizardText(hWiz, wsBuffer1.c_str(), n == vFilesToBeDeletedSize);
			UINT fdz = UINT((double(n) / double(vFilesToBeDeletedSize)) * 1000000);
			SetMprgWizardValue(hWiz, fdz, n == vFilesToBeDeletedSize);
		}
		if (i[0] == L'\1') {
			if (GetFileAttributes(pszBuffer) & FILE_ATTRIBUTE_READONLY)
				SetFileAttributesW(pszBuffer, FILE_ATTRIBUTE_NORMAL);
			DeleteFileW(pszBuffer);
		}
		else if (i[0] == L'\2') {
			RemoveDirectoryW(pszBuffer);
		}
	}

	Sleep(2000);
	TaskDialog(0, 0, L"Delete Directory Content", LastErrorStrW().c_str(),
		pDirName.c_str(), TDCBF_CLOSE_BUTTON | TDCBF_CANCEL_BUTTON, 0, 0);

	return 0;
}

int main(int argc, char* argv[])
{
	InitMprgComponent(); // remember to init
	hObj = CreateMprgObject();
	MyAssert(hObj);
	HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
		.cb= sizeof(MPRG_CREATE_PARAMS),
		.szTitle= L"Delete Directory Content utility",
		.szText= L"Processing...",
		.max= size_t(-1),
	});
	MyAssert(hWiz);
	OpenMprgWizard(hWiz);

	if (argc < 2) {
		MessageBoxW(GetMprgHwnd(hWiz), L"No input file specified.", 0, MB_ICONERROR);
		DestroyMprgWizard(hObj, hWiz);
		DeleteObject(hObj);
		return ERROR_INVALID_PARAMETER;
	}

	HANDLE hThread = CreateThread(0, 0, MainThread, argv[1], 0, 0);
	if (!hThread) {
		MessageBoxW(GetMprgHwnd(hWiz), L"Failed to CreateThread.", 0, MB_ICONERROR);
		DestroyMprgWizard(hObj, hWiz);
		DeleteObject(hObj);
		return ERROR_INVALID_PARAMETER;
	}
	DWORD dwRet = -1;
	WaitForSingleObject(hThread, INFINITE);
	GetExitCodeThread(hThread, &dwRet);
	DestroyMprgWizard(hObj, hWiz);
	DeleteObject(hObj);
	return (int)dwRet;
}

