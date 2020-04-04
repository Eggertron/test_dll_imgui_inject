/*
Used this video, https://youtu.be/b_1a3Wmi76U, to learn how to setup imgui folder for specific directx version and 
Create the DLLMain function and first thread.
Used this video, https://youtu.be/wrIPVBXXisc, to learn simple setup of memory read/write.
Used this video, https://youtu.be/hlioPJ_uB7M, to learn how to perform memory read/write.
*/

#include <iostream>
#include <string>
#include <Windows.h>
#include "detours.h"
//#include "Patternscaning.h"
#include <iostream>

#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

using namespace std;

// ---------------------- Global Variables ----------------------- //
const char* windowName = "Spelunky";
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(__stdcall * f_EndScene)(IDirect3DDevice9 * pDevice); // Our function prototype 
f_EndScene oEndScene; // Original Endscene
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
WNDPROC oWndProc;
FILE* console; // The file descriptor to the console output
uintptr_t moduleBase; // The start of the mem space for program injected to.
bool isMenuVisible;

// --------------------- Helper Functions -------------------//

void MyDebugFunction() {
	freopen_s(&console, "CONOUT$", "w", stdout);
	cout << "Testing... " << endl;
	fclose(console);
}

// -------------------- Main Hooked ------------------------- //
HRESULT __stdcall Hooked_EndScene(IDirect3DDevice9 * pDevice) // Our hooked endscene
{
	/*D3DRECT BarRect = { 100, 100, 200, 200 };
	pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 255, 0), 0.0f, 0);*/


	static bool init = true;
	if (init)
	{
		init = false;
		isMenuVisible = true;
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		ImGui_ImplWin32_Init(FindWindowA(NULL, windowName));
		ImGui_ImplDX9_Init(pDevice);
	}

	// Read Keystrokes
	if (GetAsyncKeyState(VK_END) & 1) {
		if (isMenuVisible) {
			isMenuVisible = false;
		}
		else {
			isMenuVisible = true;
		}
	}

	

	if (isMenuVisible) {
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Text("Hello, world!");
		if (ImGui::Button("debug"))
			MyDebugFunction();

		ImGui::EndFrame();
		ImGui::Render();

		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}

	return oEndScene(pDevice); // Call original ensdcene so the game can draw
}

// ------------------------- Boilerplate code below --------------------------
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


DWORD WINAPI MainThread(LPVOID param) // Our main thread
{
	moduleBase = (uintptr_t)GetModuleHandle(NULL); // Get the module base address that we are injected into.
	
	HWND  window = FindWindowA(NULL, windowName); // Find the Process Handle by windowName

	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);

	IDirect3D9 * pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (!pD3D)
		return false;

	D3DPRESENT_PARAMETERS d3dpp{ 0 };
	d3dpp.hDeviceWindow = window, d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD, d3dpp.Windowed = TRUE;

	IDirect3DDevice9 *Device = nullptr;
	if (FAILED(pD3D->CreateDevice(0, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &Device)))
	{
		pD3D->Release();
		return false;
	}

	void ** pVTable = *reinterpret_cast<void***>(Device);

	if (Device)
		Device->Release(), Device = nullptr;

	oEndScene = (f_EndScene)DetourFunction((PBYTE)pVTable[42], (PBYTE)Hooked_EndScene);


	return false;
}

// This command is run when dll is injected
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: // Gets runned when injected
		AllocConsole(); // Enables the console
		FILE* console;
		freopen_s(&console, "CONOUT$", "w", stdout);
		cout << "Loaded Spelunky Injection." << endl;
		fclose(console);
		CreateThread(0, 0, MainThread, hModule, 0, 0); // Creates our thread 
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
