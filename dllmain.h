#pragma once
#include <d3d.h>
#include <d3d9.h>
#include <windows.h>
#include <iostream>
#include "Inlinehook.h"
#include <vector>
#pragma warning(disable:4996)

#ifdef _DEBUG
#define ENABLE_LOG
#endif // DEBUG

#ifdef ENABLE_LOG
#define D(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define D(fmt, ...)
#endif

template <typename T>
__forceinline T ReadMemory(intptr_t address)
{
    return *(T*)address;
}

template <typename T>
__forceinline void WriteMemory(intptr_t address, T get_value)
{
    *(T*)address = get_value;
}

typedef struct _Zombie
{
    float x;
    float y;
    int healthNow;
    int healthMax;
}Zombie;

extern HMODULE g_PlantsVsZombiesBase = NULL;
extern intptr_t g_EndScene_address = NULL;
extern InlineRegFilterHookSt g_reg_hook_EndScene = {};
extern LPVOID g_Direct3DCreateDevice_address = NULL;
extern intptr_t g_game_dx7_obj = NULL;
extern InlineHookFunctionSt g_hook_Direct3DCreateDevice_init = {};
extern InlineRegFilterHookSt g_hook_CallCreateDevice_init = {};

extern bool g_WindowIsHook = false;
extern HWND g_GameHwnd = NULL;
extern WNDPROC g_oldProc = NULL;

extern bool g_ShowMenu = true;
extern bool g_Esp = true;

void InitOut();
void GetBase();
void hookCallCreateDevice();
void _stdcall CallCreateDeviceEvent(HookContext* ctx);
void hookDirect3DCreateDevice();
int __stdcall MyDirect3DCreateDevice(const struct _GUID*, struct IUnknown*, struct IDirectDrawSurface*, struct IUnknown**, struct IUnknown*);
void DrawZombies(std::vector<Zombie> Zombies, HDC hDC);
void _stdcall EndSceneEvent(HookContext* ctx);
void Draw(HDC hDC);
void hookEndScene();
std::vector<Zombie> GetZombieList();

void HotKeyInit();
LRESULT CALLBACK HookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void HotKeyInitInGame();
void HotKeyHandle(WPARAM wParam);