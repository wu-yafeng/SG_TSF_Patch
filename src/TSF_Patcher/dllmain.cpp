// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "pattern_finder.h"
#include <msctf.h>

typedef void(__cdecl* GameUIHookProc_t)(int*);


static void _OpenKeyboard(ITfCompartmentMgr* _CompartmentMgr, TfClientId _ClientId, bool _OpenVal) {
	if (_CompartmentMgr == nullptr) {
		return;
	}
	ITfCompartment* pCompartment = nullptr;
	VARIANT var;

	if (SUCCEEDED(_CompartmentMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pCompartment))) {
		if (SUCCEEDED(pCompartment->GetValue(&var))) {
			if (var.vt == VT_I4) {
				if (var.lVal != _OpenVal) {
					var.lVal = _OpenVal;
					pCompartment->SetValue(_ClientId, &var);
				}
			}
		}

		pCompartment->Release();
	}
}

static void _SwitchConversion(ITfCompartmentMgr* _CompartmentMgr, TfClientId _ClientId, bool _OpenVal) {
	if (_CompartmentMgr == nullptr) {
		return;
	}
	ITfCompartment* pCompartment = nullptr;
	VARIANT var;

	if (SUCCEEDED(_CompartmentMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION, &pCompartment))) {
		if (SUCCEEDED(pCompartment->GetValue(&var))) {
			if (var.vt == VT_I4) {
				if (_OpenVal) {
					var.lVal = TF_CONVERSIONMODE_SYMBOL | TF_CONVERSIONMODE_NATIVE;
					pCompartment->SetValue(_ClientId, &var);
				}
				else {
					var.lVal = TF_CONVERSIONMODE_ALPHANUMERIC;
					pCompartment->SetValue(_ClientId, &var);
				}
			}
		}

		pCompartment->Release();
	}
}

static void OnFocusChange(int* lParam) {
	bool hasFocus = *lParam;
	ITfThreadMgr* pThreadMgr = nullptr;
	ITfCompartmentMgr* pCompMgr = nullptr;
	ITfCompartment* pCompartment = nullptr;
	TfClientId clientId;
	// 创建 ITfThreadMgr 对象
	if (SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)&pThreadMgr))) {
		// 激活 TSF
		if (SUCCEEDED(pThreadMgr->Activate(&clientId))) {
			// 获取 ITfCompartmentMgr 接口
			if (SUCCEEDED(pThreadMgr->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr))) {
				_OpenKeyboard(pCompMgr, clientId, hasFocus);

				_SwitchConversion(pCompMgr, clientId, true);

				pCompMgr->Release();
			}
			pThreadMgr->Deactivate();
		}
		pThreadMgr->Release();
	}
}

static void DeactiveKeyboard(int* lParam) {
	auto _OpenVal = *lParam;
	ITfThreadMgr* pThreadMgr = nullptr;
	ITfCompartmentMgr* pCompMgr = nullptr;
	ITfCompartment* pCompartment = nullptr;
	TfClientId clientId;
	// 创建 ITfThreadMgr 对象
	if (SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)&pThreadMgr))) {
		// 激活 TSF
		if (SUCCEEDED(pThreadMgr->Activate(&clientId))) {
			// 获取 ITfCompartmentMgr 接口
			if (SUCCEEDED(pThreadMgr->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr))) {
				_OpenKeyboard(pCompMgr, clientId, _OpenVal);
				pCompMgr->Release();
			}
			pThreadMgr->Deactivate();
		}
		pThreadMgr->Release();
	}
}

static void PatchAsync(LPARAM lParam) {
	// 等10秒游戏窗口初始化完成
	Sleep(10000);

	auto hModule = GetModuleHandleA(NULL);

	auto pGameUIHooksBase = (char*)pattern::search("8B 44 24 04 8B 04 85 ?? ?? ?? ?? 85 C0 74 0B 51", hModule) + 7;

	if (pGameUIHooksBase == 0) {
		MessageBoxA(NULL, "Failed to find GameUIHooksBase", "Error", MB_OK);

		return;
	}

	auto pGameUIHooks = *(GameUIHookProc_t**)pGameUIHooksBase;
	// wait game initialize global variable: 'pGameUIHooks'
	while (pGameUIHooks == 0 || pGameUIHooks[31] == nullptr || pGameUIHooks[2] == nullptr) {
		Sleep(100);
		pGameUIHooks = *(GameUIHookProc_t**)pGameUIHooksBase;
	}

	pGameUIHooks[31] = OnFocusChange;
	pGameUIHooks[2] = DeactiveKeyboard;

	printf("TSF patch success\n");
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	FILE* file;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PatchAsync, NULL, 0, NULL);

		// alloac console: only for testing.

		/*AllocConsole();

		freopen_s(&file,"CONOUT$", "w", stdout);*/

		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

