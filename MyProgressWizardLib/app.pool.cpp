#include "app.pool.hpp"


std::atomic<size_t> nMpwizNextUniqueInternalHandleValue(1);
std::map<HANDLE, UiThreadDataDescriptor*> mmUiTh2pData;
std::map<HMPRGOBJ, HANDLE> mmUiObj2h;
std::map<HMPRGWIZ, HMPRGOBJ> mmUiAssignedWizBelongship;

