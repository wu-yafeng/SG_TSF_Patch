// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "pattern_finder.h"
#include <msctf.h>

typedef void(__cdecl* GameUIHookProc_t)(int*);

static ITfDocumentMgr* s_docMgr;

static void OnFocusChanged(int* lParam) {
	bool hasFocus = *lParam;
	ITfThreadMgr* pThreadMgr = nullptr;
	TfClientId clientId;
	// 创建 ITfThreadMgr 对象
	if (SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)&pThreadMgr))) {
		// 激活 TSF
		if (SUCCEEDED(pThreadMgr->Activate(&clientId))) {
			
			if (s_docMgr == nullptr) {
				// 自定义文档，什么都没设置，所以不能输中文
				pThreadMgr->CreateDocumentMgr(&s_docMgr);
			}

			if (hasFocus) {
				// 设置为默认文档
				pThreadMgr->SetFocus(NULL);
			}
			else {
				// 设置为自定义文档
				pThreadMgr->SetFocus(s_docMgr);
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
	while (pGameUIHooks == 0 || pGameUIHooks[31] == nullptr) {
		Sleep(100);
		pGameUIHooks = *(GameUIHookProc_t**)pGameUIHooksBase;
	}

	pGameUIHooks[31] = OnFocusChanged;

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

