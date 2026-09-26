#include "pch.h"
#include "../Public/GUI.h"
#include "../../FortniteGame/Public/BattleRoyaleGamePhaseLogic.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_impl_dx11.h"
#include "../../ImGui/imgui_impl_win32.h"
#include "../Public/Configuration.h"
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

UINT g_ResizeWidth = 0;
UINT g_ResizeHeight = 0;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

auto WindowWidth = 430;
auto WindowHeight = 260;

void GUI::Init()
{
    ImGui_ImplWin32_EnableDpiAwareness();

    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASS wc{};
    wc.lpszClassName = L"PlatinumWC";
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);

    RegisterClass(&wc);
    wchar_t buffer[67];

    swprintf_s(buffer,
               VersionInfo.EngineVersion >= 5.0 ? L"Platinum (FN %.2f, UE %.1f)"
                                                : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? L"Platinum (FN %.2f, UE %.2f)" : L"Platinum (FN %.1f, UE %.2f)"),
               VersionInfo.FortniteVersion, VersionInfo.EngineVersion);

    HWND hWnd = CreateWindow(wc.lpszClassName, buffer, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, 100, 100, (int)(WindowWidth * main_scale), (int)(WindowHeight * main_scale), nullptr, nullptr,
                             wc.hInstance, nullptr);

    if (!hWnd)
        return;

    IDXGISwapChain* g_pSwapChain = nullptr;
    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;

    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res =
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED)
    {
        res =
            D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }

    if (res != S_OK)
    {
        DestroyWindow(hWnd);
        return;
    }

    ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
    ID3D11Texture2D* pBackBuffer = nullptr;

    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);

    pBackBuffer->Release();

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    DWORD dwMyID = GetCurrentThreadId();
    DWORD dwCurID = GetWindowThreadProcessId(hWnd, nullptr);

    AttachThreadInput(dwCurID, dwMyID, TRUE);

    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);

    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    SetActiveWindow(hWnd);

    AttachThreadInput(dwCurID, dwMyID, FALSE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImFontConfig FontConfig{};
    FontConfig.FontDataOwnedByAtlas = false;

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)font, sizeof(font), 15.0f, &FontConfig);

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(10, 8);
    style.FramePadding = ImVec2(7, 4);
    style.ItemSpacing = ImVec2(7, 5);
    style.ItemInnerSpacing = ImVec2(6, 4);

    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    style.ScrollbarSize = 9.0f;
    style.GrabMinSize = 9.0f;

    const ImVec4 Platinum = ImVec4(0.141f, 1.000f, 0.773f, 1.000f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.88f, 0.96f, 0.94f, 1.00f);

    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.55f, 0.53f, 1.00f);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.035f, 0.033f, 1.00f);

    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.030f, 0.045f, 0.043f, 1.00f);

    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.035f, 0.050f, 0.048f, 1.00f);

    style.Colors[ImGuiCol_Border] = ImVec4(0.10f, 0.20f, 0.18f, 1.00f);

    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.055f, 0.075f, 0.070f, 1.00f);

    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.075f, 0.110f, 0.100f, 1.00f);

    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.090f, 0.130f, 0.120f, 1.00f);

    style.Colors[ImGuiCol_Button] = ImVec4(0.055f, 0.075f, 0.070f, 1.00f);

    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.141f, 1.000f, 0.773f, 0.20f);

    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.141f, 1.000f, 0.773f, 0.30f);

    style.Colors[ImGuiCol_Header] = ImVec4(0.141f, 1.000f, 0.773f, 0.16f);

    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.141f, 1.000f, 0.773f, 0.23f);

    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.141f, 1.000f, 0.773f, 0.30f);

    style.Colors[ImGuiCol_CheckMark] = Platinum;

    style.Colors[ImGuiCol_Separator] = ImVec4(0.10f, 0.20f, 0.18f, 1.00f);

    style.Colors[ImGuiCol_Tab] = ImVec4(0.035f, 0.050f, 0.048f, 1.00f);

    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.141f, 1.000f, 0.773f, 0.20f);

    style.Colors[ImGuiCol_TabSelected] = ImVec4(0.141f, 1.000f, 0.773f, 0.14f);

    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.141f, 1.000f, 0.773f, 0.25f);

    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.018f, 0.025f, 0.023f, 1.00f);

    bool done = false;
    bool g_SwapChainOccluded = false;

    while (!done)
    {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                done = true;
        }

        if (done)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            Sleep(10);
            continue;
        }

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_mainRenderTargetView->Release();

            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);

            g_ResizeWidth = 0;
            g_ResizeHeight = 0;

            ID3D11Texture2D* pBackBuffer = nullptr;

            g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);

            pBackBuffer->Release();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

        ImGui::SetNextWindowSize(ImVec2(WindowWidth * main_scale, WindowHeight * main_scale), ImGuiCond_Always);
        ImGui::Begin("Platinum", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
        ImGui::TextColored(Platinum, "Platinum");

        ImGui::SameLine();

        ImGui::TextDisabled("Gameserver");

        ImGui::Separator();

        auto World = UWorld::GetWorld();

        AFortGameMode* GameMode = World ? (AFortGameMode*)World->AuthorityGameMode : nullptr;

        if (GameMode)
        {
            //ImGui::Spacing();

            //ImGui::TextColored(Platinum, "SERVER");

            ImGui::Separator();

            ImGui::Text("Status");
            ImGui::SameLine(145.0f);

            if (gsStatus == NotReady)
            {
                ImGui::TextColored(Platinum, "Setting up");
            }
            else if (gsStatus == Joinable)
            {
                ImGui::TextColored(Platinum, "Joinable");
            }
            else
            {
                ImGui::TextColored(Platinum, "Match Started");
            }

            ImGui::Text("Game Mode");
            ImGui::SameLine(145.0f);

            ImGui::TextColored(Platinum, "Battle Royale");

            ImGui::Text("Port");
            ImGui::SameLine(145.0f);

            ImGui::TextColored(Platinum, "%d", FConfiguration::Port);

            int PlayerCount = 0;

            if (GameMode->HasAlivePlayers())
            {
                PlayerCount = GameMode->AlivePlayers.Num();
            }

            ImGui::Text("Players");
            ImGui::SameLine(145.0f);

            ImGui::TextColored(Platinum, "%d", PlayerCount);
        }
        else
        {
            ImGui::TextColored(Platinum, "SERVER");

            ImGui::Separator();

            ImGui::TextDisabled("Waiting for game server...");
        }

        ImGui::End();

        ImGui::Render();

        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);

        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_mainRenderTargetView)
        g_mainRenderTargetView->Release();

    if (g_pSwapChain)
        g_pSwapChain->Release();

    if (g_pd3dDeviceContext)
        g_pd3dDeviceContext->Release();

    if (g_pd3dDevice)
        g_pd3dDevice->Release();

    DestroyWindow(hWnd);

    UnregisterClass(wc.lpszClassName, wc.hInstance);

    TerminateProcess(GetCurrentProcess(), 0);
}