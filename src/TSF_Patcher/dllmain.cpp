// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "pattern_finder.h"
#include <msctf.h>

typedef void(__cdecl* GameUIHookProc_t)(int*);

static void __cdecl DeactiveKeyboard(int* lParam) {
	auto deactive = lParam && *lParam == 0;

	if (!deactive) {
		return;
	}

	ITfThreadMgr2* pThreadMgr = nullptr;

	HRESULT hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr2, (void**)&pThreadMgr);

	if (!SUCCEEDED(hr)) {
		return;
	}

	DWORD dwFlags = 0;
	hr = pThreadMgr->GetActiveFlags(&dwFlags);

	auto activated = (TF_TMF_ACTIVATED & dwFlags) != 0;

	if (activated) {
		hr = pThreadMgr->Deactivate();
	}

	pThreadMgr->Release();
}

static void __cdecl ActiveKeyboard(int* lParam) {
	auto active = lParam && *lParam != 0;

	ITfThreadMgr2* pThreadMgr = nullptr;

	HRESULT hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr2, (void**)&pThreadMgr);

	if (!SUCCEEDED(hr)) {
		return;
	}

	DWORD dwFlags = 0;
	hr = pThreadMgr->GetActiveFlags(&dwFlags);

	auto activated = (TF_TMF_ACTIVATED & dwFlags) != 0;

	TfClientId tfClientId = TF_CLIENTID_NULL;
	if (!activated) {
		hr = pThreadMgr->Activate(&tfClientId);

		if (!SUCCEEDED(hr)) {
			return;
		}

		ITfDocumentMgr* pDocMgr = nullptr;

		hr = pThreadMgr->CreateDocumentMgr(&pDocMgr);

		ITfContext* context = nullptr;
		TfEditCookie cookie{};
		hr = pDocMgr->CreateContext(tfClientId, 0, nullptr, &context, &cookie);

		if (!SUCCEEDED(hr)) {
			return;
		}
		hr = pDocMgr->Push(context);

		if (!SUCCEEDED(hr)) {
			return;
		}
		hr = pThreadMgr->SetFocus(pDocMgr);
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

	pGameUIHooks[31] = ActiveKeyboard;
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

