#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "sdk.h"

#include <gl/GL.h>

#include <MinHook.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>

struct Rect { float x1, y1, x2, y2; };
static Rect drawBox(SDK::PlayerEntity* player) {
	auto pos = player->origin;
	float hx = player->radius;
	float hy = player->radius;
	float eyeH = player->eyeHeight;
	float maxEye = player->maxEyeHeight;
	if (eyeH <= 0.0f) eyeH = 1.0f;
	if (maxEye <= 0.0f) maxEye = eyeH + 0.5f;
	float hminz = -eyeH;
	float hmaxz = maxEye - eyeH;
	float totalHeight = (maxEye - (-eyeH));
	float topPadding = totalHeight * 0.15f;
	if (topPadding < 0.05f) topPadding = 0.05f;
	if (topPadding > 0.5f) topPadding = 0.5f;
	hmaxz += topPadding;
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
	float yaw = player->yaw;
	float cosY = cosf(yaw);
	float sinY = sinf(yaw);
	SDK::Vec2 screenCorners[8]{};
	bool anyOnScreen = false;
	for (int i = 0; i < 8; ++i) {
		auto& L = cornersLocal[i];
		float wx = L.x * cosY - L.y * sinY;
		float wy = L.x * sinY + L.y * cosY;
		float wz = L.z;

		SDK::Vec3 world = { pos.x + wx, pos.y + wy, pos.z + wz };
		if (SDK::Utils::W2S(world, screenCorners[i])) anyOnScreen = true;
	}

	if (!anyOnScreen) return Rect{ 0,0,0,0 }; // not on screen
	float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
	for (int i = 0; i < 8; ++i) {
		minX = fminf(minX, screenCorners[i].x);
		minY = fminf(minY, screenCorners[i].y);
		maxX = fmaxf(maxX, screenCorners[i].x);
		maxY = fmaxf(maxY, screenCorners[i].y);
	}

	if (!(minX <= maxX && minY <= maxY)) return Rect{ 0,0,0,0 };
	/*ImGui::GetForegroundDrawList()->AddRect(
		ImVec2(minX, minY),
		ImVec2(maxX, maxY),
		IM_COL32(230, 230, 230, 200),
		2.f, 0, 2.f
	);*/

	// draw corners instead of a full box
	auto lineLen = (maxX - minX) * 0.2f;
	ImGui::GetForegroundDrawList()->AddLine(
		ImVec2(minX, minY),
		ImVec2(minX + lineLen, minY),
		IM_COL32(230, 230, 230, 200),
		2.f
	);
	ImGui::GetForegroundDrawList()->AddLine(
		ImVec2(minX, minY),
		ImVec2(minX, minY + lineLen),
		IM_COL32(230, 230, 230, 200),
		2.f
	);
	ImGui::GetForegroundDrawList()->AddLine(
		ImVec2(maxX, minY),
		ImVec2(maxX - lineLen, minY),
		IM_COL32(230, 230, 230, 200),
		2.f
	);
	ImGui::GetForegroundDrawList()->AddLine(
		ImVec2(maxX, minY),
		ImVec2(maxX, minY + lineLen),
		IM_COL32(230, 230, 230, 200),
		2.f
	);
	ImGui::GetForegroundDrawList()->AddLine(
		ImVec2(minX, maxY),
		ImVec2(minX + lineLen, maxY),
		IM_COL32(230, 230, 230, 200),
		2.f
	);
	ImGui::GetForegroundDrawList()->AddLine(
		ImVec2(minX, maxY),
		ImVec2(minX, maxY - lineLen),
		IM_COL32(230, 230, 230, 200),
		2.f
	);
	ImGui::GetForegroundDrawList()->AddLine(
		ImVec2(maxX, maxY),
		ImVec2(maxX - lineLen, maxY),
		IM_COL32(230, 230, 230, 200),
		2.f
	);
	ImGui::GetForegroundDrawList()->AddLine(
		ImVec2(maxX, maxY),
		ImVec2(maxX, maxY - lineLen),
		IM_COL32(230, 230, 230, 200),
		2.f
	);

	// Draw a light transparent filled box
	ImGui::GetForegroundDrawList()->AddRectFilled(
		ImVec2(minX, minY),
		ImVec2(maxX, maxY),
		IM_COL32(100, 100, 100, 50),
		2.f, 0
	);

	return Rect{ minX, minY, maxX, maxY };
}

bool isPlayerInFOV(SDK::PlayerEntity* target, float radFOV) {
	auto playerPos = target->origin;

	auto cameraPos = SDK::G::localCamera->origin;
	auto cameraYawDeg = SDK::G::localCamera->yaw - 90.0f;
	auto cameraPitchDeg = -SDK::G::localCamera->pitch;

	// Convert degrees to radians
	float cameraYaw = cameraYawDeg * (3.14159265f / 180.0f);
	float cameraPitch = cameraPitchDeg * (3.14159265f / 180.0f);
	// Calculate the forward vector from yaw and pitch
	SDK::Vec3 forward;
	forward.x = cosf(cameraPitch) * cosf(cameraYaw);
	forward.y = cosf(cameraPitch) * sinf(cameraYaw);
	forward.z = -sinf(cameraPitch);
	forward.normalize();

	// Vector from camera to player
	SDK::Vec3 toPlayer = playerPos - cameraPos;
	toPlayer.normalize();

	// Calculate the angle between the forward vector and the toPlayer vector
	float dotProduct = forward.dot(toPlayer);
	float angle = acosf(dotProduct); // angle in radians
	return angle < radFOV;
}

void aimLock() {
	constexpr float FOV = 0.3f; // radians

	auto localPlayer = SDK::PlayerEntity::getLocalPlayer();
	auto players = SDK::PlayerEntity::getPlayers();

	// Draw FOV circle in the center of the screen
	int screenWidth = *SDK::G::screenWidth;
	int screenHeight = *SDK::G::screenHeight;
	ImGui::GetForegroundDrawList()->AddCircle(
		ImVec2((float)screenWidth / 2, (float)screenHeight / 2),
		screenHeight * FOV * 0.5f,
		IM_COL32(200, 200, 200, 100),
		64,
		2.f
	);

	SDK::PlayerEntity* closestPlayer = nullptr;
	float closestDist = FLT_MAX;

	// Print for every player if they are in FOV
	for (auto player : players->span()) {
		if (player) {
			if (player->health <= 0) continue; // dead
			//if (player->team == localPlayer->team) continue; // same team

			if (isPlayerInFOV(player, FOV)) {
				float dist = localPlayer->origin.dist(player->origin);
				if (dist < closestDist) {
					closestDist = dist;
					closestPlayer = player;
				}
			}
		}
	}

	if (closestPlayer) { // we found a player in our FOV, yay
		auto targetPos = closestPlayer->origin;
		auto playerPos = localPlayer->origin;
		SDK::conoutf("%s @ (%.2f, %.2f, %.2f)", closestPlayer->name.c_str(), targetPos.x, targetPos.y, targetPos.z);
		SDK::conoutf("%s @ (%.2f, %.2f, %.2f)", localPlayer->name.c_str(), playerPos.x, playerPos.y, playerPos.z);

		auto toTarget = targetPos - playerPos;
		toTarget.normalize();

		float radPitch = -asinf(toTarget.z);
		float radYaw = atan2f(toTarget.y, toTarget.x);

		float degPitch = radPitch * (180.0f / 3.14159265f);
		float degYaw = radYaw * (180.0f / 3.14159265f) + 90.0f;

		localPlayer->pitch = -degPitch;
		localPlayer->yaw = degYaw;

		SDK::conoutf("Targeting %s with toTarget (%.2f, %.2f, %.2f)", closestPlayer->name.c_str(), toTarget.x, toTarget.y, toTarget.z);
	}
}

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

		io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 14.0f);
		io.Fonts->Build();

		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(WindowFromDC(hdc));
		ImGui_ImplOpenGL3_Init("#version 130");
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	auto localPlayer = SDK::PlayerEntity::getLocalPlayer();
	auto players = SDK::PlayerEntity::getPlayers();
	for (auto player : players->span()) {
		if (player && !player->name.empty()) {
			if (player->health <= 0) continue; // dead
			//if (player->team == localPlayer->team) continue; // same team

			Rect box = drawBox(player);
			if (box.x1 == 0 && box.y1 == 0 && box.x2 == 0 && box.y2 == 0) continue; // not on screen
			ImVec2 topLeft = ImVec2(box.x1, box.y1);
			ImVec2 bottomRight = ImVec2(box.x2, box.y2);

			// Draw name
			auto name = player->name.string_view();
			ImVec2 textSize = ImGui::CalcTextSize(name.data(), name.data() + name.size());
			ImGui::GetForegroundDrawList()->AddText(
				ImVec2((box.x1 + box.x2) / 2 - textSize.x / 2, box.y1 - textSize.y - 5),
				IM_COL32(255, 255, 255, 255),
				name.data()
			);

			// Draw health and armour on left side of box, green and blue, vertical, no text
			auto health = player->health;
			if (health < 0) health = 0;
			if (health > 100) health = 100;
			auto armour = player->armour;
			if (armour < 0) armour = 0;
			if (armour > 100) armour = 100;
			float boxHeight = bottomRight.y - topLeft.y;
			float healthHeight = boxHeight * (health / 100.0f);
			float armourHeight = boxHeight * (armour / 100.0f);
			ImGui::GetForegroundDrawList()->AddRectFilled(
				ImVec2(topLeft.x - 6, bottomRight.y - healthHeight),
				ImVec2(topLeft.x - 2, bottomRight.y),
				IM_COL32(50, 200, 50, 200),
				2.f, 0
			);
			if (armour > 10) {
				ImGui::GetForegroundDrawList()->AddRectFilled(
					ImVec2(topLeft.x - 10, bottomRight.y - armourHeight),
					ImVec2(topLeft.x - 6, bottomRight.y),
					IM_COL32(100, 100, 200, 200),
					2.f, 0
				);
			}

			auto weapon = player->weaponSel->info.title;
			ImVec2 weaponTextSize = ImGui::CalcTextSize(weapon, weapon + strlen(weapon));
			ImGui::GetForegroundDrawList()->AddText(
				ImVec2((box.x1 + box.x2) / 2 - weaponTextSize.x / 2, box.y2 + 5),
				IM_COL32(255, 255, 255, 200),
				weapon
			);
		}
	}

	aimLock();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return original_wglSwapBuffers(hdc);
}

DWORD MainThread(LPVOID lpParam) {
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