#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "sdk.h"

#include <gl/GL.h>

#include <MinHook.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>

// wglSwapBuffers
typedef BOOL(WINAPI* wglSwapBuffers_t)(HDC hdc);
wglSwapBuffers_t original_wglSwapBuffers = nullptr;
BOOL WINAPI hooked_wglSwapBuffers(HDC hdc) {
	static bool init = false;
	if (!init) {
		init = true;
		SDK::conoutf("Initializing ImGui");
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(WindowFromDC(hdc));
		ImGui_ImplOpenGL3_Init("#version 130");
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	auto players = SDK::PlayerEntity::getPlayers();
	for (auto player : players->span()) {
		if (player && !player->name.empty()) {
			auto pos = player->origin;

			// Build oriented bounding box (OBB) corners in local/player space then rotate by yaw and translate to world
			float hx = player->radius; // half-width x
			float hy = player->radius; // half-depth y
			// The player origin in this engine is roughly at the neck/head area.
			// Use eyeHeight/maxEyeHeight to compute bottom (feet) and top (head) relative to origin.
			// bottom: move down by eyeHeight to reach feet (negative Z relative to origin)
			// top: maxEyeHeight - eyeHeight gives top relative to origin
			float eyeH = player->eyeHeight;
			float maxEye = player->maxEyeHeight;

			// Fallbacks if values are zero/invalid
			if (eyeH <= 0.0f) eyeH = 1.0f; // assume ~1m if not present
			if (maxEye <= 0.0f) maxEye = eyeH + 0.5f; // default head-top above eye

			float hminz = -eyeH; // bottom relative to player->origin (feet below origin)
			float hmaxz = maxEye - eyeH; // top relative to player->origin

			// Add mild top padding so box doesn't intersect the head.
			// Use a percentage of the player's total height (top-to-bottom) and clamp it.
			float totalHeight = (maxEye - (-eyeH)); // head top relative to feet
			float topPadding = totalHeight * 0.15f; // 15% padding above head
			// clamp padding to reasonable bounds
			if (topPadding < 0.05f) topPadding = 0.05f;
			if (topPadding > 0.5f) topPadding = 0.5f;
			hmaxz += topPadding;

			// Sanity clamps
			if (hminz > -0.1f) hminz = -0.1f;
			if (hmaxz < 0.1f) hmaxz = 0.1f;

			struct Vec3Local { float x, y, z; };
			Vec3Local cornersLocal[8] = {
				{ -hx, -hy, hminz },
				{  hx, -hy, hminz },
				{ -hx,  hy, hminz },
				{  hx,  hy, hminz },
				{ -hx, -hy, hmaxz },
				{  hx, -hy, hmaxz },
				{ -hx,  hy, hmaxz },
				{  hx,  hy, hmaxz },
			};

			// yaw rotation only (about Z axis). Assume player->yaw is in radians.
			float yaw = player->yaw;
			float cosY = cosf(yaw);
			float sinY = sinf(yaw);

			SDK::Vec2 screenCorners[8];
			bool anyOnScreen = false;

			for (int i = 0; i < 8; ++i) {
				auto &L = cornersLocal[i];
				float wx = L.x * cosY - L.y * sinY; // rotate local -> world (X,Y)
				float wy = L.x * sinY + L.y * cosY;
				float wz = L.z;

				SDK::Vec3 world = { pos.x + wx, pos.y + wy, pos.z + wz };
				if (SDK::Utils::W2S(world, screenCorners[i])) anyOnScreen = true;
			}

			if (!anyOnScreen) continue; // skip drawing if entirely off-screen

			// compute 2D bounding rect in screen space
			float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
			for (int i = 0; i < 8; ++i) {
				minX = fminf(minX, screenCorners[i].x);
				minY = fminf(minY, screenCorners[i].y);
				maxX = fmaxf(maxX, screenCorners[i].x);
				maxY = fmaxf(maxY, screenCorners[i].y);
			}

			// safety: ensure rect is sane
			if (!(minX <= maxX && minY <= maxY)) continue;

			ImGui::GetForegroundDrawList()->AddRect(
				ImVec2(minX, minY),
				ImVec2(maxX, maxY),
				IM_COL32(255, 0, 0, 255)
			);

			// draw name centered above box (clamp Y)
			float textX = minX;
			float textY = minY - 15.0f;
			ImGui::GetForegroundDrawList()->AddText(
				ImVec2(textX, textY),
				IM_COL32(255, 255, 255, 255),
				player->name.c_str()
			);
		}
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return original_wglSwapBuffers(hdc);
}

DWORD MainThread(LPVOID lpParam) {
	auto team1 = SDK::G::teamnames[0];
	SDK::conoutf("Team 1: %s", team1 ? team1 : "Unknown");

	auto players = SDK::PlayerEntity::getPlayers();
	for (auto player : players->span()) {
		if (player && !player->name.empty()) {
			SDK::conoutf("Player: %s (CN: %d, Team: %s)", player->name.c_str(), player->clientNum, team_basestring(player->team));
			SDK::conoutf("Player @ (%f, %f, %f)", player->origin.x, player->origin.y, player->origin.z);

			// w2s each player
			SDK::Vec2 screenPos;
			if (SDK::Utils::W2S(player->origin, screenPos)) {
				SDK::conoutf("Player %s is on screen at (%f, %f)", player->name.c_str(), screenPos.x, screenPos.y);
			} else {
				SDK::conoutf("Player %s is off screen", player->name.c_str());
			}
		}
	}

	// Hook opengl32.dll->wglSwapBuffers (create a mh api hook)
	if (MH_Initialize() != MH_OK) {
		SDK::conoutf("Failed to initialize MinHook");
		FreeLibraryAndExitThread((HMODULE)lpParam, 1);
		return 1;
	}

	if (MH_CreateHookApi(L"opengl32", "wglSwapBuffers", &hooked_wglSwapBuffers, reinterpret_cast<LPVOID*>(&original_wglSwapBuffers)) != MH_OK) {
		SDK::conoutf("Failed to create hook");
		MH_Uninitialize();
		FreeLibraryAndExitThread((HMODULE)lpParam, 1);
		return 1;
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
		SDK::conoutf("Failed to enable hook");
		MH_Uninitialize();
		FreeLibraryAndExitThread((HMODULE)lpParam, 1);
		return 1;
	}

	SDK::conoutf("Hooked wglSwapBuffers successfully");
	while (!GetAsyncKeyState(VK_END)) {
		Sleep(100);
	}

	MH_Uninitialize();

	Sleep(100);

	SDK::conoutf("Ejecting DLL");

	FreeLibraryAndExitThread((HMODULE)lpParam, 0);
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hinstDLL, 0, nullptr);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}