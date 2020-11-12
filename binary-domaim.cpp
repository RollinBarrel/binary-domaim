
#include <Windows.h>
#include <windowsx.h>
uintptr_t mainModule;

float x = 0;
float y = 0;
int sens = 50;
int showFor = 0;

uintptr_t routeReturn;
void __declspec(naked) route() {
    _asm {
        fld [ebx + 0x628]
        fadd [y]
        mov [y], 0
        fstp [ebx + 0x628]
        fadd [x]
        mov [x], 0
        fstp [ebx + 0x62C]
        jmp [routeReturn]
    }
}

uintptr_t howitzerRouteReturn;
void __declspec(naked) howitzerRoute() {
    __asm {
        movss [ebx + 0x628], xmm0
        fld [ebx + 0x628]
        fadd [y]
        mov [y], 0
        fstp [ebx + 0x628]
        fld [ebx + 0x62C]
        fadd [x]
        mov [x], 0
        fstp [ebx + 0x62C]
        jmp [howitzerRouteReturn]
    }
}

bool stickAffectsCamera = true;
uintptr_t toggleReturn;
void __declspec(naked) toggle() {
    _asm {
        test [stickAffectsCamera], 1
        jne og
        mov dword ptr [eax + 0x18], 0
        mov dword ptr [eax + 0x1C], 0
        mov [stickAffectsCamera], 1
        og:
        mov ecx, [eax + 0x318]
        jmp [toggleReturn]
    }
}

uintptr_t howitzerToggleReturn;
void __declspec(naked) howitzerToggle() {
    _asm {
        test [stickAffectsCamera], 1
        jne og
        mov dword ptr[eax + 0x18], 0
        mov dword ptr[eax + 0x1C], 0
        mov[stickAffectsCamera], 1
        og:
        movss [esp + 0x08], xmm1
        jmp [howitzerToggleReturn]
    }
}

uintptr_t showReturn;
void __declspec(naked) show() {
    _asm {
        cmp [showFor], 0
        je og
        dec [showFor]
        mov edx, [sens]
        og:
        mov [ebp + 0x18], edx
        mov edx, [esp + 0x28]
        jmp[showReturn]
    }
}

void hook(uintptr_t jump, void* func) {
    DWORD oldProt;
    VirtualProtect((void*)jump, 6, PAGE_EXECUTE_READWRITE, &oldProt);
    uintptr_t relJump = ((uintptr_t)func - jump) - 5;
    *(byte*)jump = 0xE9;
    *(uintptr_t*)(jump + 1) = relJump;
    VirtualProtect((void*)jump, 6, oldProt, &oldProt);
}

HANDLE config;
inline void updateConfig() {
    DWORD a;
    SetFilePointer(config, 0, NULL, FILE_BEGIN);
    WriteFile(config, &sens, sizeof(sens), &a, NULL);
}
inline void loadConfig() {
    DWORD a;
    ReadFile(config, &sens, sizeof(sens), &a, NULL);
}

WNDPROC ogProc;
LRESULT WINAPI WindProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEMOVE:
            return 0;
        case WM_INPUT: {
            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            BYTE* lpb = new BYTE[dwSize];
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
            RAWINPUT* raw = (RAWINPUT*)lpb;

            if (raw->header.dwType == RIM_TYPEMOUSE) {
                float sensMod = 1.f;

                static uintptr_t aimModePath[] = {0xC, 0x108, 0x0, 0x20, 0xB0, 0x10, 0xD00 + 0x14};
                uintptr_t aimMode = mainModule + 0x2B9B8E4;
                for (int i = 0; i < 7; ++i) {
                    if (aimMode < mainModule) {
                        aimMode = NULL;
                        break;
                    }
                    aimMode = *(uintptr_t*)aimMode;
                    aimMode += aimModePath[i];
                }
                uintptr_t butts = *(uintptr_t*)(mainModule + 0xEA4BA4) + 0x80;

                if (butts != 0x17FFF81 && (*(int*)butts & 0b00010000) != 0 && aimMode != NULL) {
                    switch (*(int*)aimMode) {
                        case 3: {
                            sensMod = 0.75f;
                            break;
                        }
                        case 8: {
                            sensMod = 0.33f;
                            break;
                        }
                    }
                }
                sensMod *= 0.10f * sens;
                x -= raw->data.mouse.lLastX * sensMod;
                y -= raw->data.mouse.lLastY * sensMod * 0.5625f;
                if (x != 0 || y != 0)
                    stickAffectsCamera = false;
            } else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
                if (raw->data.keyboard.Flags == RI_KEY_BREAK) {
                    switch (raw->data.keyboard.VKey) {
                        case VK_OEM_PLUS:
                        case VK_ADD: {
                            ++sens;
                            showFor = 180;
                            updateConfig();
                            break;
                        }
                        case VK_OEM_MINUS:
                        case VK_SUBTRACT: {
                            --sens;
                            showFor = 180;
                            updateConfig();
                            break;
                        }
                    }
                }
            }
        }
        default: {
            return CallWindowProc(ogProc, hWnd, uMsg, wParam, lParam);
        }
    }
}

typedef HRESULT(WINAPI* FDI8Create)(HINSTANCE hinst, DWORD dwVersion,
                                    REFIID riidltf, LPVOID* ppvOut,
                                    LPUNKNOWN punkOuter);
FDI8Create oDI8Create = NULL;
HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion,
                                       REFIID riidltf, LPVOID* ppvOut,
                                       LPUNKNOWN punkOuter) {
    return oDI8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

void InitDI8Proxy() {
    HMODULE di8 = LoadLibrary("C:\\Windows\\System32\\DINPUT8.dll");
    oDI8Create = (FDI8Create)GetProcAddress(di8, "DirectInput8Create");
}

DWORD WINAPI Init(HMODULE hModule) {
    mainModule = (uintptr_t)GetModuleHandle("BinaryDomain.exe");
    if (mainModule == NULL)
        return 0;

    config = CreateFileA("./dom.aim", GENERIC_READ | GENERIC_WRITE, 0,
                                NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (config == NULL)
        return 1;
    DWORD err = GetLastError();
    if (err == 0) {
        updateConfig();
    } else if (err == ERROR_ALREADY_EXISTS) {
        loadConfig();
    }

    routeReturn = mainModule + 0xA3ADD3;
    hook(mainModule + 0xA3ADCD, &route);

    howitzerRouteReturn = mainModule + 0xABDEDF;
    hook(mainModule + 0xABDED7, &howitzerRoute);

    toggleReturn = mainModule + 0x72F6;
    hook(mainModule + 0x72F0, &toggle);

    howitzerToggleReturn = mainModule + 0xABDE07;
    hook(mainModule + 0xABDE01, &howitzerToggle);

    showReturn = mainModule + 0x384807;
    hook(mainModule + 0x384800, &show);

    while (true) {
        UINT deviceCount = 0;
        UINT devicesWritten = GetRegisteredRawInputDevices(NULL, &deviceCount, sizeof(RAWINPUTDEVICE));
        RAWINPUTDEVICE* devices = new RAWINPUTDEVICE[devicesWritten];
        devicesWritten = GetRegisteredRawInputDevices(devices, &deviceCount, sizeof(RAWINPUTDEVICE));
        HWND hWnd = 0;

        if (deviceCount > 0) {
            hWnd = devices[0].hwndTarget;
        }
        if (hWnd != 0) {
            ogProc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LONG)WindProc);
            break;
        }
        Sleep(100);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            InitDI8Proxy();
            CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Init, hModule, 0, nullptr);
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}