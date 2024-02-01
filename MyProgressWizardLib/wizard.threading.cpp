#include <Windows.h>


DWORD WINAPI MPRG_UI_Thread(PVOID param);
DWORD WINAPI MPRG_UIThreadLauncher(PVOID param) {
	return MPRG_UI_Thread(param);
}

