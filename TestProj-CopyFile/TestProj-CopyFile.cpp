// TestProj-CopyFile.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#pragma comment(lib, "../bin/MyProgressWizardLib64.lib")
#include "../MyProgressWizardLib/wizard.user.h"
#include "../../resource/tool.h"
using namespace std;


bool userCancelled = false;
bool __stdcall cancelHandler(HMPRGWIZ hWiz, HMPRGOBJ hObj) {
	SetMprgWizardText(hWiz, L"Canceling...");
	userCancelled = true;
	return true;
}

int work() {
	userCancelled = false;

	cout << "Creating object... ";
	HMPRGOBJ hObj = CreateMprgObject();
	if (!hObj) {
		cerr << "Failed!! err=" << GetLastError() << endl;
		return 2;
	}
	cout << "OK" << endl;


	cout << "Creating wizard... ";
	HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
		.cb = sizeof(MPRG_CREATE_PARAMS),
		.szTitle = L"Smart Wizard",
		.hParentWindow = GetConsoleWindow(),
		.max = 1000,
		});
	HMPRGWIZ hWiz2 = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{});
	HMPRGWIZ hWiz3 = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
		.hParentWindow = GetForegroundWindow(),
		.max = size_t(-1),
	});
	srand((UINT)time(0));
	HMPRGWIZ hWiz4 = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
		.hParentWindow = WindowFromPoint(POINT{.x = rand() % 200, .y = rand() % 300}),
		.max = 1000,
	});
	if (!hWiz) {
		DeleteMprgObject(hObj);
		cerr << "Failed!! err=" << GetLastError() << endl;
		return 3;
	}
	cout << "OK" << endl;


	SetMprgWizAttribute(hWiz, MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::
		WizAttrCancelHandler, (LONG_PTR)(PVOID)cancelHandler);


	cout << "Opening wizard... ";
	if (!OpenMprgWizard(hWiz)) {
		DestroyMprgWizard(hObj, hWiz);
		DeleteMprgObject(hObj);
		cerr << "Failed!! err=" << GetLastError() << endl;
		return 3;
	}
	OpenMprgWizard(hWiz2);
	OpenMprgWizard(hWiz3);
	OpenMprgWizard(hWiz4, SW_HIDE);
	cout << "OK" << endl;


	Sleep(2000);


	char buf[2]{}; wstringstream wss; wstring wsb;
	for (size_t i = 0; i < 1001; i += 10) {
		SetMprgWizardValue(hWiz, i);
		SetMprgWizardValue(hWiz2, i);
		SetMprgWizardValue(hWiz3, i);
		SetMprgWizardValue(hWiz4, i);
		cout << "Press Enter to step the progress... (" << i << "/1000)\n";
		wss = wstringstream();
		// low efficiency; this is only for testing use. Don't use in prod env.
		wss << L"Please wait while we are running the progress... " << i << L"/1000 ("
			<< double(double(i) / double(1000) * double(100)) << "%)";
		wsb = wss.str();
		SetMprgWizardText(hWiz, wsb.c_str());
		//cin.getline(buf, 2);
		if (i == 500) {
			SetMprgWizAttribute(hWiz, MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::
				WizAttrCancelHandler, 0);
			GetModifiableMprgWizardData(hWiz3)->max = 1000;
			UpdateMprgWizard(hWiz3);
		}
		Sleep(500);
		if (userCancelled) {
			cerr << "^C" << endl;
			break;
		}
	}


	cout << "Finished! Then we will destroy the window." << endl;
	Sleep(1000);
	DestroyMprgWizard(hObj, hWiz);
	hWiz = NULL;

	cout << "Finished! Then we will delete the object." << endl;
	pause;

	DeleteMprgObject(hObj);
	pause
	return 0;
}

int main()
{
	cout << "<TestProj> This is a test project for MyProgressWizardLib." << endl;
	cout << string(20, '=') << endl;
	cout << "Copy File: Copy a file." << endl;
	cout << "Library version: " << ws2s(GetMprgVersion()) << endl;
	cout << endl;

	cout << "Init... ";
	if (!InitMprgComponent()) {
		cerr << "Failed!! err=" << GetLastError() << endl;
		return 1;
	}
	cout << "OK" << endl;

	while (1) {
		int r = work();
		if (r) return r;
		cout << "Again? (y/N) ";
		char cch = _getch();
		if (cch == 'y' || cch == 'Y') continue;
		break;
	}

	return 0;
}
















