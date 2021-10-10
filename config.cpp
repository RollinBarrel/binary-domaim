#include <Windows.h>
#include "config.h"

SensConfig* initConfig() {
	wchar_t exePath[MAX_PATH];
	int len = GetModuleFileNameW(NULL, exePath, MAX_PATH);
	for (int i = 0; i < len; ++i) {
		if (exePath[len - i] == L'\\') {
			exePath[len - i + 1] = L'\0';
			break;
		}
	}
	wchar_t cfgPath[MAX_PATH];
	wcscpy_s(cfgPath, exePath);
	wcscat_s(cfgPath, L"dom.aim");

	HANDLE cfgFile = CreateFileW(cfgPath, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (cfgFile == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		return NULL;
	}

	DWORD err = GetLastError();
	if (err == 0) {
		SensConfig empty = { 50, 75, 25, 100, false, false, false };
		DWORD a;
		WriteFile(cfgFile, &empty, sizeof(empty), &a, NULL);
	}

	HANDLE cfgMapping = CreateFileMappingW(cfgFile, NULL, PAGE_READWRITE,
		0, 0, NULL);
	if (cfgMapping == NULL) {
		DWORD err = GetLastError();
		return NULL;
	}

	return (SensConfig*)MapViewOfFile(cfgMapping, FILE_MAP_WRITE, 0, 0, sizeof(SensConfig));
}