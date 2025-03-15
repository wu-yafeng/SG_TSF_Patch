// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "pattern_finder.h"
#include <msctf.h>

typedef void(__cdecl* GameUIHookProc_t)(int*);

static void __cdecl ToggleIME(int* lParam) {
	auto keepState = lParam && *lParam != 0;
	// enable IME or disable IME
	HRESULT hr{};
	ITfThreadMgr* pThreadMgr = nullptr;
	hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)&pThreadMgr);
	if (FAILED(hr) || pThreadMgr == nullptr) {
		printf("Failed to create ITfThreadMgr instance: %#X \n", hr);
		return;
	}
	TfClientId clientId = 0;
	if (!keepState) {
		hr = pThreadMgr->Deactivate();
	}

	pThreadMgr->Release();
}
static void __cdecl RequestTSF(int* open) {
	auto shouldEnableTSF = open && *open != 0;

	if (!shouldEnableTSF) {
		return;
	}

	// enable TSF or disable TSF
	HRESULT hr{};

	ITfThreadMgr* pThreadMgr = nullptr;

	hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)&pThreadMgr);

	if (FAILED(hr) || pThreadMgr == nullptr) {
		printf("Failed to create ITfThreadMgr instance: %#X \n", hr);
		return;
	}

	TfClientId clientId = 0;
	hr = pThreadMgr->Activate(&clientId);

	if (FAILED(hr)) {
		printf("Failed to %s TSF\n", shouldEnableTSF ? "activate" : "deactivate");
	}

	pThreadMgr->Release();
}

static void PatchAsync(LPARAM lParam) {

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

	pGameUIHooks[31] = RequestTSF;
	pGameUIHooks[2] = ToggleIME;

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

