
#include <Windows.h>
#include <windowsx.h>
#include "config.h"

uintptr_t mainModule;

SensConfig* cfg;
float x = 0;
float y = 0;
bool holdADS = false;
int ammoReplace = 0;
int showFor = 0;
char wepSelection = -1;
int mouseScroll;

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

uintptr_t btnModReturn;
void __declspec(naked) btnMod() {
    _asm {
        cmp byte ptr[wepSelection], -1
        je og
        or byte ptr[esi + 0x01], 0x10
        cmp [wepSelection], 0
        jne wep2
        or byte ptr[esi + 0x05], 0x10
        jmp og
        wep2:
        cmp [wepSelection], 1
        jne wep3
        or byte ptr[esi + 0x05], 0x20
        jmp og
        wep3:
        cmp [wepSelection], 2
        jne wep4
        or byte ptr[esi + 0x05], 0x40
        jmp og
        wep4:
        or byte ptr[esi + 0x05], 0x80
        og:
        and eax, dword ptr[ecx]
        test dword ptr[esi + 04], eax
        jmp [btnModReturn]
    }
}

uintptr_t adsModReturn;
void __declspec(naked) adsMod() {
    _asm {
        mov ecx, [cfg]
        test [ecx + 0x10], 1 //cfg->toggleADS
        je og
        mov ecx, [esi]
        and ecx, ~16
        test [holdADS], 1
        jne writeback
        or ecx, 16
        writeback:
        mov [esi], ecx
        og:
        mov ecx, dword ptr[esi + 0x318]
        jmp [adsModReturn]
    }
}

uintptr_t showReturn;
void __declspec(naked) show() {
    _asm {
        cmp [showFor], 0
        je og
        dec [showFor]
        mov edx, [ammoReplace]
        og:
        mov [ebp + 0x18], edx
        mov edx, [esp + 0x28]
        jmp[showReturn]
    }
}

uintptr_t wepDisplayReturn;
void __declspec(naked) wepDisplay() {
    _asm {
        movzx ebp, byte ptr[wepSelection]
        mov byte ptr[wepSelection], -1
        cmp ebp, -1
        je back
        mov ebp, [esp + 0x10]
        back:
        cmp ebp, 4
        jmp [wepDisplayReturn]
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

int getADSState() {
    int res = 0;
    uintptr_t btn = *(uintptr_t*)(mainModule + 0xE68B84);
    if (btn != NULL) btn += 0xF4;
    if (btn != NULL) {
        btn = *(uintptr_t*)btn;
        if (btn != NULL && (*(int*)btn & 16) != 0) {
            res |= 1;
        }
    }
    static uintptr_t aimModePath[] = {0xC, 0x100, 0x20, 0x8, 0x84, 0x70, 0xE10 + 0x14};
    uintptr_t aimMode = mainModule + 0x0114BBE4;
    for (int i = 0; i < 7; ++i) {
        if (aimMode < mainModule) {
            aimMode = NULL;
            break;
        }
        aimMode = *(uintptr_t*)aimMode;
        aimMode += aimModePath[i];
    }
    if (aimMode != NULL) {
        switch (*(int*)aimMode) {
        case 3:
            res |= 2;
            break;
        case 8:
            res |= 4;
            break;
        }
    }
    return res;
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
                if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN && 
                    cfg->toggleADS) 
                    holdADS = !holdADS;
                /*if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                    signed char sel = (short)raw->data.mouse.usButtonData / WHEEL_DELTA;
                    if (sel != 0) {
                        uintptr_t wep = *(uintptr_t*)(mainModule + 0xEF9684) + 0x70;
                        if (wep != NULL)
                            wep = *(uintptr_t*)wep;
                        uintptr_t btn = *(uintptr_t*)(mainModule + 0xE68B84) + 0xF4;
                        uintptr_t anim = *(uintptr_t*)(mainModule + 0xEA8B84) + 0x64;
                        if (wep != NULL && btn != NULL && anim != NULL) {
                            wep = wep + 0x68;
                            btn = *(uintptr_t*)btn + 0x1;
                            anim = *(uintptr_t*)anim + 0x94;
                            *(signed char*)(btn + 0x1) |= 0x10;
                            static int wepOrder[] = {2, 0, 3, 1};
                            static int wepIntOrder[] = {1, 2, 0, 3};
                            if (wepSelection == -1) {
                                signed char aa = *(char*)wep;
                                signed char bb = wepIntOrder[aa];
                                signed char cc = bb - sel;
                                signed char dd = cc % 4;
                                if (dd < 0)
                                    dd = 4 + dd;
                                wepSelection = wepOrder[dd];
                            } else {
                                wepSelection = wepOrder[(wepIntOrder[wepSelection] + sel) % 4];
                            }
                            if (*(char*)anim == 0x80)
                                *(char*)anim = 0;
                        }
                        sel = 0;
                    }
                }*/
                int ads = getADSState();
                float sensMod = 1.f;
                if (ads == (1 | 2)) {
                    sensMod = cfg->ads * 0.01f;
                } else if (ads == (1 | 4)) {
                    sensMod = cfg->sniper * 0.01f;
                }
                sensMod *= 0.10f * cfg->base;
                x -= raw->data.mouse.lLastX * sensMod * (cfg->invertedX ? -1 : 1);
                y -= raw->data.mouse.lLastY * sensMod * (cfg->invertedY ? -1 : 1)/* * 0.5625f*/;
                if (x != 0 || y != 0)
                    stickAffectsCamera = false;
            } else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
                if (raw->data.keyboard.Flags == RI_KEY_MAKE) {
                    switch (raw->data.keyboard.VKey) {
                        case VK_OEM_PLUS:
                        case VK_ADD: {
                            int ads = getADSState();
                            ammoReplace = ++cfg->base;
                            /*if (ads & 2) {
                                ammoReplace = ++cfg->ads;
                            } else if (ads & 4) {
                                ammoReplace = ++cfg->sniper;
                            } else {
                                ammoReplace = ++cfg->base;
                            }*/
                            showFor = 180;
                            break;
                        }
                        case VK_OEM_MINUS:
                        case VK_SUBTRACT: {
                            int ads = getADSState();
                            ammoReplace = --cfg->base;
                            /*if (ads & 2) {
                                ammoReplace = --cfg->ads;
                            } else if (ads & 4) {
                                ammoReplace = --cfg->sniper;
                            } else {
                                ammoReplace = --cfg->base;
                            }*/
                            showFor = 180;
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

    cfg = initConfig();

    routeReturn = mainModule + 0xA3ADD3;
    hook(mainModule + 0xA3ADCD, &route);

    howitzerRouteReturn = mainModule + 0xABDEDF;
    hook(mainModule + 0xABDED7, &howitzerRoute);

    toggleReturn = mainModule + 0x72F6;
    hook(mainModule + 0x72F0, &toggle);

    howitzerToggleReturn = mainModule + 0xABDE07;
    hook(mainModule + 0xABDE01, &howitzerToggle);

    btnModReturn = mainModule + 0x928680;
    hook(mainModule + 0x92867B, &btnMod);

    adsModReturn = mainModule + 0x92A5BB;
    hook(mainModule + 0x92A5B5, &adsMod);

    showReturn = mainModule + 0x384807;
    hook(mainModule + 0x384800, &show);

    /*wepDisplayReturn = mainModule + 0x3881CD;
    hook(mainModule + 0x3881C6, &wepDisplay);*/
        


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