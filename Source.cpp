#include <iostream>
#include <windows.h>
#include <vector>
#include "MinHook.h"
#include <string>
#include <d3dx9math.h>
#pragma comment(lib, "d3dx9.lib")

#define OFFSET_CSEFUNC 0x143a6be50		//48 89 5c 24 10 48 89 74 24 18 57 48 83 ec 30 48 89 cb 48 8b 49 08
#define OFFSET_CHAMHOOK 0x145505b24		//48 89 d0 0f 28 00 66 41 0f 7f 01 f6 42 40 02 74 ??

bool IsValidPtr(void* addr) {
	return !IsBadReadPtr(reinterpret_cast<PVOID>(addr), sizeof(PVOID));
}

class ChamDefs {
public:
	const static byte ColorA = 240;	
	const static byte ColorB = 241;	
	const static byte ColorC = 242;	
};

class ChamColors {
public:
	D3DXVECTOR4 ColorA;
	D3DXVECTOR4 ColorB;
	D3DXVECTOR4 ColorC;
};

class ClientSoldier
{
public:
	char pad_0000[568]; //0x0000
	class ClientPlayer* clientPlayer; //0x0238
	char pad_0240[108]; //0x0240
	uint8_t EngineChams; //0x02AC
	char pad_02AD[819]; //0x02AD
	class ClientAimEntity* clientAimEntity; //0x05E0
	char pad_05E8[25]; //0x05E8
	int8_t Occluded; //0x0601

	bool IsVisible() {
		if (this == nullptr || this->clientPlayer == nullptr) return false;
		return !(bool)(this->Occluded);
	}
};

class ClientPlayer
{
public:
	char pad_0000[24]; //0x0000
	char* Name; //0x0018
	char pad_0020[11148]; //0x0020
	int32_t Team; //0x2BAC
}; //Size: 0x4048

class RCX
{
public:
	char pad_0000[8]; //0x0000
	class RBX
	{
	public:
		class ClientSoldier* CSE; //0x0000
	};
	RBX* rbx; // 0x8
};

typedef void(__stdcall * _ChamFunc)(DWORD64 ad1, ChamColors* chamcolors);
_ChamFunc oChamFunc = (_ChamFunc)(OFFSET_CHAMHOOK);

typedef void(__stdcall* tCSEFUNC)(RCX* rcx, int64_t rdx, int8_t r8b);
tCSEFUNC oCSEFUNC = (tCSEFUNC)OFFSET_CSEFUNC;

ClientPlayer* pLocalPlayer;

D3DXVECTOR4 setting_ColorA = { 0, .5f, 1, 0 };
D3DXVECTOR4 setting_ColorB = { 0, 1, 0, 0 };
D3DXVECTOR4 setting_ColorC = { 1, 0, 1, 0 };

void hkChamFunc(DWORD64 stuff, ChamColors* colors) {
	if (colors->ColorA != setting_ColorA || colors->ColorB != setting_ColorB || colors->ColorC != setting_ColorC) {
		colors->ColorA = setting_ColorA;
		colors->ColorB = setting_ColorB;
		colors->ColorC = setting_ColorC;
	}
	return oChamFunc(stuff, colors);
}

void __fastcall hkCSEFUNC(RCX* rcx, int64_t rdx, int8_t r8b)
{
	if (IsValidPtr(rcx) && IsValidPtr(rcx->rbx) && IsValidPtr(rcx->rbx->CSE) && IsValidPtr(rcx->rbx->CSE->clientPlayer)  &&
		rcx->rbx->CSE->clientPlayer->Team != 0) {
		if (IsValidPtr(rcx->rbx->CSE->clientAimEntity)) {
			pLocalPlayer = rcx->rbx->CSE->clientPlayer;
		}
		if (IsValidPtr(pLocalPlayer)) {
			if (rcx->rbx->CSE->clientPlayer->Team == pLocalPlayer->Team) {
				rcx->rbx->CSE->EngineChams = ChamDefs::ColorA;
			}
			else {
				rcx->rbx->CSE->EngineChams = (rcx->rbx->CSE->IsVisible()) ? ChamDefs::ColorB : ChamDefs::ColorC;
			}
		}
	}
	return oCSEFUNC(rcx, rdx, r8b);
}

DWORD WINAPI Looper(LPVOID lpParam)
{
	//AllocConsole();
	//freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	if (MH_Initialize() != MH_OK) { return 1; }
	if (MH_CreateHook(reinterpret_cast<LPVOID>(OFFSET_CHAMHOOK), &hkChamFunc, reinterpret_cast<LPVOID*>(&oChamFunc)) != MH_OK) return 1; 
	if (MH_CreateHook(reinterpret_cast<LPVOID>(OFFSET_CSEFUNC), &hkCSEFUNC, reinterpret_cast<LPVOID*>(&oCSEFUNC)) != MH_OK)  return 1; 
	if (MH_EnableHook(reinterpret_cast<LPVOID>(OFFSET_CHAMHOOK)) != MH_OK)  return 1; 
	if (MH_EnableHook(reinterpret_cast<LPVOID>(OFFSET_CSEFUNC)) != MH_OK)  return 1; 
	for (;;) {
		Sleep(10);
		if (GetAsyncKeyState(VK_END)) {
			FreeLibraryAndExitThread((HMODULE)lpParam, 0);
		}
	}
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwAttached, LPVOID lpvReserved)
{
	if (dwAttached == DLL_PROCESS_ATTACH) {
		CreateThread(0, 0, Looper, hModule, 0, NULL);
	}
	if (dwAttached == DLL_PROCESS_DETACH) {
		MH_DisableHook(reinterpret_cast<LPVOID>(OFFSET_CSEFUNC));
		MH_DisableHook(reinterpret_cast<LPVOID>(OFFSET_CHAMHOOK));
		std::cout << "Detaching Console and Exiting Thread" << std::endl;
		FreeConsole();
	}
	return 1;
}