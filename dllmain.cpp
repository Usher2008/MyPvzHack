#include "dllmain.h"

void mymain()
{
#ifdef _DEBUG
    MessageBox(0,0,0,0);
    InitOut();
#endif // DEBUG
    GetBase();
    D("[%s] start\n", __FUNCTION__);
    CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)HotKeyInit, 0, 0, 0));
    hookCallCreateDevice();
}

void HotKeyInitInGame()
{
    RegisterHotKey(g_GameHwnd, 0xc000, 0, VK_F1);
    RegisterHotKey(g_GameHwnd, 0xc001, 0, VK_HOME);
}

void HotKeyHandle(WPARAM wParam)
{
    switch (wParam)
    {
    case 0xc000:
        g_Esp = !g_Esp;
        break;
    case 0xc001:
        g_ShowMenu = !g_ShowMenu;
        break;
    }
}

void Draw(HDC hDC)
{
    WCHAR str[100] = L"";
    if (g_ShowMenu)
    {
        wsprintf(str, L"HOME显示/隐藏\n");
        lstrcatW(str, L"F1透视：");
        lstrcatW(str, g_Esp ? L"开\n" : L"关\n");
        RECT strRect = { 0, 0, 200, 200 };
        DrawText(hDC, str, wcslen(str), &strRect, DT_BOTTOM);
    }
    if (g_Esp)
    {
        std::vector<Zombie> Zombies = GetZombieList();
        if (Zombies.size())
        {
            DrawZombies(Zombies, hDC);
        }
    }
}

void GetBase()
{
    g_PlantsVsZombiesBase = GetModuleHandleA("PlantsVsZombies.exe");
    D("[%s]g_PlantsVsZombiesBase:0x%p\n", __FUNCTION__, g_PlantsVsZombiesBase);
}

void InitOut()
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
}


std::vector<Zombie> GetZombieList()
{
    std::vector<Zombie> Zombies;
    DWORD Base = ReadMemory<intptr_t>((DWORD)g_PlantsVsZombiesBase + 0x355E0C);
    if (Base != 0)
    {
        Base = ReadMemory<intptr_t>(Base + 0x868);
        if (Base != 0)
        {
            DWORD ListLen = ReadMemory<intptr_t>(Base + 0xAC);
            DWORD ListHead = ReadMemory<intptr_t>(Base + 0xa8);
            if (ListLen)
            {
                for (DWORD i = 0; i < ListLen; ++i)
                {
                    DWORD ZombBase = ListHead + i * 0x168;
                    float X = ReadMemory<float>(ZombBase + 0x2c);
                    float Y = ReadMemory<float>(ZombBase + 0x30);

                    int Health = *(int*)(ZombBase + 0xc8);
                    int HealthMax = *(int*)(ZombBase + 0xcc);

                    Zombie tZombie = { X,Y,Health,HealthMax };
                    Zombies.push_back(tZombie);
                    D("[%s]Zombie[%d]:(%f,%f)\n", __FUNCTION__, i, X, Y);
                }
            }
            else
            {
                return Zombies;
            }
        }
    }
    return Zombies;
}

intptr_t GetCallCreateDevice(HMODULE hModule)
{
    //PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    //PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((intptr_t)hModule + pDosHeader->e_lfanew);
    //
    //return (intptr_t)hModule + pNtHeader->OptionalHeader.AddressOfEntryPoint;
    return (intptr_t)hModule + 0x15C443;//call Direct3DCreateDevice
}

//替换的窗口过程
LRESULT CALLBACK HookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!g_WindowIsHook)
    {
        g_WindowIsHook = TRUE;
        
        HotKeyInitInGame();

        D("[%s]hooked!\n", __FUNCTION__);
    }
    
    //热键处理 hotkey是自己定义的热键标识 这样一来自己定义的热键处理 其他消息 包括其他热键都默认处理
    if (uMsg == WM_HOTKEY)
    {
        HotKeyHandle(wParam);
        CallWindowProc(g_oldProc, hwnd, uMsg, wParam, lParam);
    }
    else
    {
        CallWindowProc(g_oldProc, hwnd, uMsg, wParam, lParam);
    }

    return 1;
}

void HotKeyInit()
{
    while(!g_GameHwnd)
    g_GameHwnd = FindWindow(NULL, L"Plants vs. Zombies");

    D("[%s]g_GameHwnd:0x%p\n", __FUNCTION__, g_GameHwnd);

    DWORD threadId, processId;
    threadId = GetWindowThreadProcessId(g_GameHwnd, &processId);

    D("[%s]threadId:0x%p\n", __FUNCTION__, threadId);

    g_oldProc = (WNDPROC)SetWindowLong(g_GameHwnd, GWL_WNDPROC, (LONG)HookWndProc);
    
}

void hookCallCreateDevice()
{
    intptr_t CallCreateDevice = GetCallCreateDevice(GetModuleHandleA(0));
    D("CallCreateDevice:0x%p\n", CallCreateDevice);

    bool init_result = InitRegFilterInlineHook(&g_hook_CallCreateDevice_init, (LPVOID)CallCreateDevice, CallCreateDeviceEvent);
    D("[%s]init_result:%d\n", __FUNCTION__, init_result);

    bool install_result = InstallRegFilterInlineHook(&g_hook_CallCreateDevice_init);
    D("[%s]install_result:%d\n", __FUNCTION__, install_result);
}

void _stdcall CallCreateDeviceEvent(HookContext* ctx)
{
    UNREFERENCED_PARAMETER(ctx);

    D("[%s] hook!\n", __FUNCTION__);
    UninstallRegFilterInlineHook(&g_hook_CallCreateDevice_init);
    
    hookDirect3DCreateDevice();

}

void hookDirect3DCreateDevice()
{
    HMODULE d3dim700 = NULL;
    d3dim700 = GetModuleHandleA("d3dim700.dll");
    D("[%s] d3dim700:0x%p\n", __FUNCTION__, d3dim700);

    g_Direct3DCreateDevice_address = GetProcAddress(d3dim700, "Direct3DCreateDevice");
    D("[%s] Direct3DCreateDevice:0x%p\n", __FUNCTION__, g_Direct3DCreateDevice_address);

    BOOL init_result = InitInlineHookFunction(&g_hook_Direct3DCreateDevice_init, g_Direct3DCreateDevice_address, MyDirect3DCreateDevice);
    D("[%s] init_result:%d\n", __FUNCTION__, init_result);

    BOOL install_result = InstallInlineHookFunction(&g_hook_Direct3DCreateDevice_init);
    D("[%s] install_result:%d\n", __FUNCTION__, install_result);
}

int __stdcall MyDirect3DCreateDevice(
    const struct _GUID* a1,
    struct IUnknown* a2,
    struct IDirectDrawSurface* a3,
    struct IUnknown** a4,
    struct IUnknown* a5)
{

    D("[%s] hook!\n", __FUNCTION__);

    auto org = (decltype(&MyDirect3DCreateDevice))g_hook_Direct3DCreateDevice_init.pNewHookAddr;
    int ret_val = org(a1, a2, a3, a4, a5);

    UninstallInlineHookFunction(&g_hook_Direct3DCreateDevice_init);

    g_game_dx7_obj = ReadMemory<intptr_t>((intptr_t)a4);
    intptr_t* dx7_vtable = ReadMemory<intptr_t*>(g_game_dx7_obj);

    D("[%s] dx7_obj:0x%p\n", __FUNCTION__, g_game_dx7_obj);
    D("[%s] dx7_vtable:0x%p\n", __FUNCTION__, dx7_vtable);

    g_EndScene_address = dx7_vtable[6];
    D("[%s] g_EndScene_address:0x%p\n", __FUNCTION__, g_EndScene_address);

    hookEndScene();

    return ret_val;
}

void hookEndScene()
{
    bool init_result = InitRegFilterInlineHook(&g_reg_hook_EndScene, (LPVOID)g_EndScene_address, EndSceneEvent);
    D("[%s]init_result:%d\n", __FUNCTION__, init_result);

    bool install_result = InstallRegFilterInlineHook(&g_reg_hook_EndScene);
    D("[%s]install_result:%d\n", __FUNCTION__, install_result);
}

void _stdcall EndSceneEvent(HookContext* ctx)
{
    IDirect3DDevice7* thiz = ReadMemory<IDirect3DDevice7*>(ctx->uEsp + 0x4);
    LPDIRECTDRAWSURFACE7 lpDDS;
    thiz->GetRenderTarget(&lpDDS);
    if (lpDDS)
    {
        HDC hDC;
        lpDDS->GetDC(&hDC);

        Draw(hDC);

        lpDDS->ReleaseDC(hDC);
    }
}

void DrawZombies(std::vector<Zombie>Zombies, HDC hDC)
{
    HPEN hpan = CreatePen(0, 1, RGB(255, 0, 0));
    HGDIOBJ hbrush = GetStockObject(NULL_BRUSH);
    SelectObject(hDC, hpan);
    SelectObject(hDC, hbrush);
    size_t count = 0;
    for (auto i = Zombies.begin(); i != Zombies.end(); ++i)
    {
        int x = (int)i->x;
        int y = (int)i->y;
        count++;
        int healthNow = i->healthNow;
        int healthMax = i->healthMax;
        if (x > 1000)//屏幕外不画
        {
            continue;
        }
        Rectangle(hDC, x + 20, y, x + 90, y + 120);
        //char str[101];
        //GetTextFaceA(hDC, 100, str);
        //D("使用的字体为：%s\n", str);//System

        WCHAR str[100] = L"";
        wsprintf(str, L"[%d]hp:%d", count, healthNow);

        TextOut(hDC, x + 20, y, str, wcslen(str));
    }
    DeleteObject(hpan);
    DeleteObject(hbrush);
    return;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)mymain, 0, 0, 0));
        mymain();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

