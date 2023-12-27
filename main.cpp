#include <Windows.h>
#include <thread>

//ImGui
#include <dwmapi.h>
#include <d3d11.h>
#include <windowsx.h>
#include "../external/ImGui/imgui.h"
#include "../external/ImGui/imgui_impl_dx11.h"
#include "../external/ImGui/imgui_impl_win32.h"

#include "memory.h"
#include "vector.h"
#include "render.h"
#include "bone.hpp"

int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT messagge, WPARAM w_Param, LPARAM l_Param) 
{
	if (ImGui_ImplWin32_WndProcHandler(window, messagge, w_Param, l_Param)) 
	{
		return 0L;
	}

	if (messagge == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0L;
	}
	switch (messagge)
	{
	case WM_NCHITTEST: {

		const LONG borderWidth = GetSystemMetrics(SM_CXSIZEFRAME);
		const LONG titleBarHeight = GetSystemMetrics(SM_CYCAPTION);

		POINT cursorPos = { GET_X_LPARAM(w_Param),GET_Y_LPARAM(l_Param) };
		RECT windowRect;
		GetWindowRect(window, &windowRect);

		if (cursorPos.y >= windowRect.top && cursorPos.y < windowRect.top + titleBarHeight)
			return HTCAPTION;

		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(window, messagge, w_Param, l_Param);
}

namespace offset
{
	constexpr std::ptrdiff_t dwLocalPlayer = 0x1810F48;
	constexpr std::ptrdiff_t dwEntityList = 0x17C1950;
	constexpr std::ptrdiff_t dwViewMatrix = 0x1820150;

	constexpr std::ptrdiff_t m_iHealth = 0x32C;
	constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x16C8F38;
	constexpr std::ptrdiff_t m_iTeamNum = 0x3BF;
	constexpr std::ptrdiff_t m_vecOrigin = 0x24;
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{
	memify mem("cs2.exe");
	uintptr_t base = mem.GetBase("client.dll");

	while (!mem.ProcessIsOpen("cs2.exe")) {
		std::cout << "Couldn't find CS2, waiting..." << std::endl;
		Sleep(2000);
	}


	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = instance;
	wc.lpszClassName = L"idk lol";

	RegisterClassExW(&wc);

	const HWND overlay = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		wc.lpszClassName,
		L"idk lol",
		WS_POPUP,
		0,
		0,
		screenWidth,
		screenHeight,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

	{
		RECT client_area{};
		GetClientRect(overlay, &client_area);

		RECT window_area{};
		GetWindowRect(overlay, &window_area);

		POINT diff{};
		ClientToScreen(overlay, &diff);

		const MARGINS margins{
		window_area.left + (diff.x - window_area.left),
		window_area.top + (diff.y - window_area.top),
		client_area.right,
		client_area.bottom,
		};

		DwmExtendFrameIntoClientArea(overlay, &margins);
	}

	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.RefreshRate.Numerator = 60U; // 60 fps
	sd.BufferDesc.RefreshRate.Denominator = 1U;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1U;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2U;
	sd.OutputWindow = overlay;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	constexpr D3D_FEATURE_LEVEL levels[2]{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_0,
	};

	ID3D11Device* device{ nullptr };
	ID3D11DeviceContext* device_context{ nullptr };
	IDXGISwapChain* swap_chain{ nullptr };
	ID3D11RenderTargetView* render_target_view{ nullptr };
	D3D_FEATURE_LEVEL level{};

	//create device and that
	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		&level,
		&device_context
	);

	ID3D11Texture2D* back_buffer{ nullptr };
	swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

	if (back_buffer) {
		device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
		back_buffer->Release();
	}
	else
		return 1;
	
	ShowWindow(overlay, cmd_show);
	UpdateWindow(overlay);

	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	ImGui_ImplWin32_Init(overlay);
	ImGui_ImplDX11_Init(device, device_context);

	bool running = true;

	while (running)
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				running = false;
			}
		}
		
		if (!running)
			break;

		uintptr_t localPlayer = mem.Read<uintptr_t>(base + offset::dwLocalPlayer);
		Vector3 Localorigin = mem.Read<Vector3>(localPlayer + offset::m_vecOrigin);
		view_matrix_t view_matrix = mem.Read<view_matrix_t>(base + offset::dwViewMatrix);
		uintptr_t entity_list = mem.Read<uintptr_t>(base + offset::dwEntityList);
		int localTeam = mem.Read<int>(localPlayer + offset::m_iTeamNum);
				
		ImGui_ImplDX11_NewFrame();
	    ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		for (int playerIndex = 1; playerIndex < 32; ++playerIndex)
		{
			uintptr_t listentry = mem.Read<uintptr_t>(entity_list + (8 * (playerIndex & 0x7FFF) >> 9) + 16);

			if (!listentry)
				continue;

			uintptr_t player = mem.Read<uintptr_t>(listentry + 120 * (playerIndex & 0x1FF));

			if (!player)
				continue;

			int playerteam = mem.Read<int>(player + offset::m_iTeamNum);

			//if (playerteam == localTeam)
			// 	continue;

			uint32_t playerPawn = mem.Read<uint32_t>(player + offset::dwLocalPlayerPawn);

			uintptr_t listentry2 = mem.Read<uintptr_t>(entity_list + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 16);

			if (!listentry2)
				continue;

			uintptr_t pCSPlayerPawn = mem.Read<uintptr_t>(listentry2 + 120 * (playerPawn & 0x1FF));

			if (!pCSPlayerPawn)
				continue;

			int health = mem.Read<int>(pCSPlayerPawn + offset::m_iHealth);

			if (health <= 0 || health > 100)
				continue;

			if (pCSPlayerPawn == localPlayer) 
				continue;

			uintptr_t gamescene = mem.Read<uintptr_t>(pCSPlayerPawn + 0x310);
			uintptr_t bonearray = mem.Read<uintptr_t>(gamescene + 0x160 + 0x80);

			Vector3 origin = mem.Read<Vector3>(pCSPlayerPawn + offset::m_vecOrigin);
			Vector3 head = mem.Read<Vector3>(bonearray + bones::head * 32);

			Vector3 screenPos = origin.WTS(view_matrix);
			Vector3 screenHead = head.WTS(view_matrix);

			float height = screenPos.y - screenHead.y; 
			float width = height / 2.4f;

			RGB enemy = { 255, 0, 0 }; 
			RGB white = { 255, 255, 255 };

			

			Render::DrawRect(
				screenHead.x - width / 2,
				screenHead.y,
				width,
				height,
				enemy,
				1.5 // thicknes of the box
			);

			for (int i = 0; i < sizeof(boneConnections) / sizeof(boneConnections[0]); i++)
			{
				int bone1 = boneConnections[i].bone1;
				int bone2 = boneConnections[i].bone2;

				Vector3 VectorBone1 = mem.Read<Vector3>(bonearray + bone1 * 32);
				Vector3 VectorBone2 = mem.Read<Vector3>(bonearray + bone2 * 32);

				Vector3 b1 = VectorBone1.WTS(view_matrix);
				Vector3 b2 = VectorBone2.WTS(view_matrix);

				Render::DrawLine(b1.x, b1.y, b2.x, b2.y, white, 1.5);
			}
		}

		ImGui::Render();
		float color[4]{ 0, 0, 0, 0 };
		device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
		device_context->ClearRenderTargetView(render_target_view, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		swap_chain->Present(0U, 0U); // if the first 0U is 1U VSync is enabled
	}	

	// extiting
	ImGui_ImplDX11_Shutdown(); 
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	if (swap_chain) {
		swap_chain->Release();
	}
	
	if (device_context) {
		device_context->Release();
	}

	if (device) {
		device->Release();
	}
	
	if (render_target_view) {
		render_target_view->Release();
	}
	
	DestroyWindow(overlay);
	UnregisterClassW(wc.lpszClassName, wc.hInstance);
}