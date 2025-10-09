#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#undef ERROR

#include <filesystem>
#include <string_view>

#include <format>
#include <print>

#define DLL_NAME "ACIDLL.dll"

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

#define INFO(...)  std::println(GREEN "[+] " RESET __VA_ARGS__)
#define WARN(...)  std::println(YELLOW "[x] " RESET __VA_ARGS__)
#define ERROR(...) std::println(RED "[!] " RESET __VA_ARGS__)

static DWORD findProcess(std::string_view processName) {
	PROCESSENTRY32 processEntry{};
	processEntry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		ERROR("Failed to create process snapshot");
		return 0;
	}
	if (Process32First(hSnapshot, &processEntry)) {
		do {
			if (std::filesystem::path(processEntry.szExeFile).filename() == processName) {
				CloseHandle(hSnapshot);
				return processEntry.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &processEntry));
	} else {
		ERROR("Failed to retrieve first process");
	}
	CloseHandle(hSnapshot);
	WARN("Process `{}` not found", processName);
	return 0;
}

// Only returns false on failure, true if the DLL was not found or successfully ejected
static bool ejectDLL(std::string_view dllName, HANDLE hProcess) {
	// First we need to find the module in the target process (if it's there)

	auto pid = GetProcessId(hProcess);
	if (!pid) {
		ERROR("Failed to get process ID");
		return false;
	}

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		ERROR("Failed to create module snapshot");
		return false;
	}

	// Now we can iterate through the modules to find the one we want to eject (if any)
	MODULEENTRY32 moduleEntry{};
	moduleEntry.dwSize = sizeof(MODULEENTRY32);
	bool found = false;
	if (Module32First(hSnapshot, &moduleEntry)) {
		do {
			if (std::filesystem::path(moduleEntry.szModule).filename() == dllName) {
				found = true;
				break;
			}
		} while (Module32Next(hSnapshot, &moduleEntry));
	} else {
		ERROR("Failed to retrieve first module");
		CloseHandle(hSnapshot);
		return false;
	}

	CloseHandle(hSnapshot);
	if (!found) {
		INFO("DLL `{}` not found in target process", dllName);
		return true;
	}

	// Now we can eject the DLL, call FreeLibrary in the target process (use a remote thread)
	HMODULE hModule = moduleEntry.hModule;
	if (!hModule) {
		ERROR("Module handle is null");
		return false;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary"),
		hModule, 0, nullptr);
	if (!hThread || hThread == INVALID_HANDLE_VALUE) {
		ERROR("Failed to create remote thread");
		return false;
	}
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	INFO("Successfully ejected DLL `{}`", dllName);
}

static bool inject(std::string_view dllPath, HANDLE hProcess) {
	// First we need to allocate memory in the target process for the DLL path
	size_t pathSize = dllPath.size() + 1;
	LPVOID pRemotePath = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pRemotePath) {
		ERROR("Failed to allocate memory in target process");
		return false;
	}
	// Now we can write the DLL path to the allocated memory
	if (!WriteProcessMemory(hProcess, pRemotePath, dllPath.data(), pathSize, nullptr)) {
		ERROR("Failed to write DLL path to target process");
		VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
		return false;
	}
	// Now we can create a remote thread to load the DLL
	HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
		pRemotePath, 0, nullptr);
	if (!hThread || hThread == INVALID_HANDLE_VALUE) {
		ERROR("Failed to create remote thread");
		VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
		return false;
	}
	// Wait for the thread to finish and clean up
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
	INFO("Successfully injected DLL `{}`", dllPath);
	return true;
}

std::string getDLLPath() {
	auto currentPath = std::filesystem::current_path();
	auto dllPath = currentPath / DLL_NAME;
	if (!std::filesystem::exists(dllPath)) {
		std::filesystem::current_path(currentPath.parent_path());

		// recursive dir iterator to find the DLL
		for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path())) {
			if (entry.path().filename() == DLL_NAME) {
				return std::filesystem::absolute(entry.path()).string();
			}
		}

		ERROR("Could not find DLL `{}`", DLL_NAME);
		return "";
	}
	return std::filesystem::absolute(dllPath).string();
}

int main() {
	DWORD pid = findProcess("ac_client.exe");
	if (!pid) {
		ERROR("Could not find AC process");
		return 1;
	}

	INFO("Found AC with PID: {}", pid);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
		ERROR("Failed to open AC process");
		return 1;
	}

	if (!ejectDLL("ACI.dll", hProcess)) {
		ERROR("Failed to eject DLL");
		CloseHandle(hProcess);
		return 1;
	}

	std::string dllPath = getDLLPath();
	if (dllPath.empty()) {
		CloseHandle(hProcess);
		return 1;
	}

	if (!inject(dllPath, hProcess)) {
		ERROR("Failed to inject DLL");
		CloseHandle(hProcess);
		return 1;
	}

	return 0;
}