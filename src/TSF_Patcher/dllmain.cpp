// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "pattern_finder.h"
#include <msctf.h>
#include <Windows.h>
#include <iostream>

typedef void(__cdecl* GameUIHookProc_t)(int*, void* wgt);
enum class GameUICallbackEvent
{
	UI_Callback_GetGameCursor = 0,
	UI_Callback_DrawCursor,
	UI_Callback_EnableIME,

	UI_Callback_Encrypt,//密码控件改进
	UI_Callback_Decrypt,//密码控件改进

	UI_Callback_IsKeyChar,
	UI_Callback_GetKeyChar,
	UI_Callback_DrawKeyChar,
	UI_Callback_HitKeyChar,
	UI_Callback_ValidKeyChar,

	UI_Callback_IsValidItem,

	UI_Callback_QueryItemInfo,
	UI_Callback_UpdateItemInfo,

	UI_Callback_ClickItem,
	UI_Callback_DClickItem,
	UI_Callback_RClickItem,
	UI_Callback_MoveItem,
	UI_Callback_DrawItem,

	UI_Callback_BeginDragItem,
	UI_Callback_EndDragItem,

	UI_Callback_GetItemPixel,

	UI_Callback_QueryDirty,

	UI_Callback_DrawCustomBuffer,
	UI_Callback_SetCustomBuffer,
	UI_Callback_GetCustomBuffer,
	UI_Callback_ParseCustomBuffer,
	UI_Callback_CustomBufferSelfShow,
	UI_Callback_CustomBufferDeinit,

	UI_Callback_QueryNode,
	UI_Callback_QueryNodePath,

	UI_Callback_PlaySound,

	UI_Callback_RequestSoftInput,

	//事件最大数目
	UI_Callback_MAXNUM,

};
inline static void ThreadMgr_EnableIME(ITfThreadMgr* pThreadMgr, bool bEnable) {
	// in-game ime: forbid input method editor (IME) to input text.
	static ITfDocumentMgr* s_DocMgr = nullptr;
	// user ime: for input text
	static ITfDocumentMgr* s_PrevDocMgr = nullptr;

	auto hWnd = GetActiveWindow();// Game Window
	// init in-game ime at first time.
	if (s_DocMgr == nullptr) {
		if (FAILED(pThreadMgr->CreateDocumentMgr(&s_DocMgr))) {
			return;
		}

		// set in-game ime as default document manager. user cannot input any text.
		pThreadMgr->AssociateFocus(hWnd, s_DocMgr, &s_PrevDocMgr);
	}

	ITfDocumentMgr* pTempDocMgr = NULL;
	if (bEnable) {
		// restore user ime.
		pThreadMgr->AssociateFocus(hWnd, s_PrevDocMgr, &pTempDocMgr);
		//pThreadMgr->SetFocus(s_PrevDocMgr);
	}
	else {
		// in-game ime
		pThreadMgr->AssociateFocus(hWnd, s_DocMgr, &pTempDocMgr);
		//pThreadMgr->SetFocus(s_DocMgr);
	}

	pTempDocMgr->Release();
}

static void OnFocusChanged(int* data, void* wgt) {
	auto bIme = *reinterpret_cast<bool*>(data);
	ITfThreadMgr* pThreadMgr = nullptr;
	if (FAILED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)&pThreadMgr))) {
		return;
	}

	TfClientId clientId = TF_CLIENTID_NULL;

	if (SUCCEEDED(pThreadMgr->Activate(&clientId))) {
		ThreadMgr_EnableIME(pThreadMgr, bIme);

		pThreadMgr->Deactivate();
	}

	pThreadMgr->Release();
}


static void PatchAsync(LPARAM lParam) {
	// 等10秒游戏窗口初始化完成
	Sleep(10000);

	auto hModule = GetModuleHandleA(NULL);

	auto pVecCallbackFuncsBase = (char*)pattern::search("8B 44 24 04 8B 04 85 ?? ?? ?? ?? 85 C0 74 0B 51", hModule) + 7;

	if (pVecCallbackFuncsBase == 0) {
		MessageBoxA(NULL, "Failed to find g_pVecCallbackFuncs", "Error", MB_OK);

		return;
	}

	auto g_pVecCallbackFuncs = *(GameUIHookProc_t**)pVecCallbackFuncsBase;
	// wait game initialize global variable: 'pGameUIHooks'
	while (g_pVecCallbackFuncs == 0 || g_pVecCallbackFuncs[31] == nullptr) {
		Sleep(100);
		g_pVecCallbackFuncs = *(GameUIHookProc_t**)pVecCallbackFuncsBase;
	}

	g_pVecCallbackFuncs[static_cast<int>(GameUICallbackEvent::UI_Callback_RequestSoftInput)] = OnFocusChanged;

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

