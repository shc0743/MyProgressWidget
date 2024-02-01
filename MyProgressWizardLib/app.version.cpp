#include <Windows.h>

static wchar_t MPWLIB_VERSION[256] = L""
#include "version.txt"
"";

extern "C" PCWSTR GetMprgVersion() {
	return MPWLIB_VERSION;
}

