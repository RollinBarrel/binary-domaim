#include <Windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include "config.h"

static int id = 0;
static SensConfig* cfg;
static char str[9];
enum SliderID {
	SID_BASE, SID_ACCEL, SID_ADS, SID_SNIPER
};
enum ButtonID {
	BID_XINV, BID_YINV, BID_ADS
};
static HWND trackbars[4];
static HWND edits[4];
static HWND btns[3];

void setSlider(SliderID id, int val) {
	SendMessage(trackbars[id], TBM_SETPOS, (WPARAM)true, (LPARAM)val);
	_itoa_s(val, str, 10);
	SendMessage(edits[id], WM_SETTEXT, NULL, (LPARAM)str);
}

void slider(HWND hwnd, SliderID sid, int val, char* label) {
	HWND track = CreateWindow(TRACKBAR_CLASS, NULL,
							  WS_CHILD | WS_VISIBLE, 10, 10 + id * 40, 200, 30,
						      hwnd, (HMENU)sid, NULL, NULL);
	SendMessage(track, TBM_SETRANGE, (WPARAM)true,
			    (LPARAM)MAKELONG(1, 500));
	SendMessage(track, TBM_SETPOS, (WPARAM)true, (LPARAM)val);

	_itoa_s(val, str, 10);
	HWND edit = CreateWindow("EDIT", str,
							 WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NUMBER,
							 220, 10 + id * 40 + 5, 36, 20, hwnd, (HMENU)sid, NULL, NULL);

	HWND updown = CreateWindow(UPDOWN_CLASS, NULL,
							   WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ARROWKEYS,
							   220 + 36, 10 + id * 40 + 5, 16, 20, hwnd, NULL, NULL, NULL);
	SendMessage(updown, UDM_SETBUDDY, (WPARAM)edit, 0);
	SendMessage(updown, UDM_SETRANGE, 0, (LPARAM)MAKELONG(0, 20000));

	CreateWindow("STATIC", label,
				 WS_CHILD | WS_VISIBLE | WS_TABSTOP,
				 220 + 36 + 16 + 10, 10 + id * 40 + 5, 200, 16,
				 hwnd, NULL, NULL, NULL);

	trackbars[sid] = track;
	edits[sid] = edit;
	++id;
}

HWND checkbox(HWND hwnd, ButtonID bid, bool* val, char* label) {
	HWND box = CreateWindow("BUTTON", label,
			     			WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
							20, 10 + id * 40, 200, 16,
							hwnd, (HMENU)bid, NULL, NULL);

	SendMessage(box, BM_SETCHECK, (WPARAM)*val, NULL);

	btns[bid] = box;
	++id;
	return box;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
						 WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				case EN_CHANGE: {
					SendMessage((HWND)lParam, WM_GETTEXT, (WPARAM)sizeof(str), (LPARAM)str);
					unsigned int val = atoi(str);
					SendMessage(trackbars[LOWORD(wParam)], TBM_SETPOS,
						        (WPARAM)true, (LPARAM)val);
					switch (LOWORD(wParam)) {
						case SID_BASE:
							cfg->base = val;
							break;
						case SID_ACCEL:
							cfg->accel = val;
							break;
						case SID_ADS:
							cfg->ads = val;
							break;
						case SID_SNIPER:
							cfg->sniper = val;
							break;
					}
					break;
				}
				case BN_CLICKED: {
					int state = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
					SendMessage((HWND)lParam, BM_SETCHECK, !state, NULL);
					switch (LOWORD(wParam)) {
						case BID_XINV:
							cfg->invertedX = !state;
							break;
						case BID_YINV:
							cfg->invertedY = !state;
							break;
						case BID_ADS:
							cfg->toggleADS = !state;
							break;
					}
					break;
				}
			}
			break;
		case WM_HSCROLL: {
			int i = 0;
			for (; i < 4; ++i) {
				if ((HWND)lParam == trackbars[i])
					break;
			}
			unsigned int val = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			_itoa_s(val, str, 10);
			SendMessage(edits[i], WM_SETTEXT, NULL, (LPARAM)str);
			switch (i) {
				case SID_BASE:
					cfg->base = val;
					break;
				case SID_ACCEL:
					cfg->accel = val;
					break;
				case SID_ADS:
					cfg->ads = val;
					break;
				case SID_SNIPER:
					cfg->sniper = val;
					break;
			}
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
			break;
	}

	return 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					 LPSTR lpCmdLine, int nCmdShow) {
	static char szWindowClass[] = "BinDomfig";
	static char szTitle[] = "Binary Domaim Configuration";

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	RegisterClassEx(&wcex);

	HWND hwnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		460, 320,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	cfg = initConfig();
	SensConfig mirror = *cfg;

	CreateWindow("STATIC", "Changes are applied automatically",
				 WS_CHILD | WS_VISIBLE | WS_TABSTOP,
				 10, 10, 400, 16, hwnd, NULL, NULL, NULL);
	++id;
	slider(hwnd, SID_BASE, cfg->base, "Mouse Sensitivity");
	//slider(hwnd, SID_ACCEL, &cfg->accel, "Mouse Acceleration (%)");
	slider(hwnd, SID_ADS, cfg->ads, "ADS Multiplier (%)");
	slider(hwnd, SID_SNIPER, cfg->sniper, "Sniper Multiplier (%)");
	checkbox(hwnd, BID_XINV, &cfg->invertedX, "Invert X Axis");
	checkbox(hwnd, BID_YINV, &cfg->invertedY, "Invert Y Axis");
	checkbox(hwnd, BID_ADS, &cfg->toggleADS, "Toggle ADS");

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		
		bool update = false;
		if (mirror.base != cfg->base) {
			setSlider(SID_BASE, cfg->base);
			update = true;
		}
		if (mirror.accel != cfg->accel) {
			setSlider(SID_ACCEL, cfg->accel);
			update = true;
		}
		if (mirror.ads != cfg->ads) {
			setSlider(SID_ADS, cfg->ads);
			update = true;
		}
		if (mirror.sniper != cfg->sniper) {
			setSlider(SID_SNIPER, cfg->sniper);
			update = true;
		}
		if (mirror.invertedX != cfg->invertedX) {
			SendMessage(btns[BID_XINV], BM_SETCHECK, (WPARAM)cfg->invertedX, NULL);
			update = true;
		}
		if (mirror.invertedY != cfg->invertedY) {
			SendMessage(btns[BID_YINV], BM_SETCHECK, (WPARAM)cfg->invertedY, NULL);
			update = true;
		}
		if (mirror.toggleADS != cfg->toggleADS) {
			SendMessage(btns[BID_ADS], BM_SETCHECK, (WPARAM)cfg->toggleADS, NULL);
			update = true;
		}

		if (update)
			mirror = *cfg;
	}

	return (int)msg.wParam;
}
