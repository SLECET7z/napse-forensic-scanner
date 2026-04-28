#define IMGUI_DEFINE_MATH_OPERATORS
#include "pch.h"
#include "scanner.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <dwmapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global scanner
Scanner g_Scanner;
bool g_Scanning = false;

void ApplyPremiumStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 0.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 12.0f;
    style.WindowPadding = ImVec2(0, 0);
    style.ItemSpacing = ImVec2(10, 10);

    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(168/255.f, 85/255.f, 247/255.f, 0.20f);
    colors[ImGuiCol_Button] = ImVec4(168/255.f, 85/255.f, 247/255.f, 0.10f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(168/255.f, 85/255.f, 247/255.f, 0.20f);
    colors[ImGuiCol_ButtonActive] = ImVec4(168/255.f, 85/255.f, 247/255.f, 0.30f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_Separator] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
}

struct Particle {
    ImVec2 pos;
    ImVec2 vel;
    float alpha;
};

std::vector<Particle> g_Particles;

void UpdateParticles() {
    if (g_Particles.empty()) {
        for (int i = 0; i < 50; i++) {
            g_Particles.push_back({ ImVec2((float)(rand() % 600), (float)(rand() % 800)), ImVec2((rand() % 20 - 10) * 0.05f, (rand() % 20 - 10) * 0.05f), (rand() % 100) / 100.0f });
        }
    }

    for (auto& p : g_Particles) {
        p.pos.x += p.vel.x;
        p.pos.y += p.vel.y;
        if (p.pos.x < 0) p.pos.x = 600;
        if (p.pos.x > 600) p.pos.x = 0;
        if (p.pos.y < 0) p.pos.y = 800;
        if (p.pos.y > 800) p.pos.y = 0;
    }
}

void DrawParticles() {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    for (auto& p : g_Particles) {
        draw->AddCircleFilled(p.pos, 1.5f, ImColor(1.0f, 1.0f, 1.0f, p.alpha * 0.3f));
    }
}

void DrawNapseSpinner(float radius, float speed) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiContext& g = *GImGui;
    ImVec2 centre = ImGui::GetItemRectMin();
    centre.x += radius;
    centre.y += radius;
    
    float t = (float)g.Time * speed;
    for (int i = 0; i < 3; i++) {
        float r = radius - (i * 8.0f);
        float a_min = t * (1.2f + i * 0.5f);
        float a_max = a_min + IM_PI * 1.2f;
        window->DrawList->PathClear();
        window->DrawList->PathArcTo(centre, r, a_min, a_max, 30);
        window->DrawList->PathStroke(ImColor(168, 85, 247, 255), false, 3.0f);
    }
}

void DrawSpinner(float radius, float thickness, int num_segments, float speed) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;
    
    ImGuiContext& g = *GImGui;
    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + radius * 2, window->DC.CursorPos.y + radius * 2));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, 0)) return;

    window->DrawList->PathClear();
    int start = (int)abs(sin(g.Time * speed) * (num_segments - 5));
    const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;
    const ImVec2 centre = ImVec2(bb.Min.x + radius, bb.Min.y + radius);

    for (int i = 0; i < num_segments; i++) {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        window->DrawList->PathLineTo(ImVec2(centre.x + cos(a + g.Time * 8) * radius, centre.y + sin(a + g.Time * 8) * radius));
    }
    window->DrawList->PathStroke(ImGui::GetColorU32(ImGuiCol_Text), false, thickness);
}

int main(int, char**) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"OceanScanner", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(0, wc.lpszClassName, L"FORENSIC", WS_POPUP, 100, 100, 460, 280, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL; // Disable imgui.ini
    
    ApplyPremiumStyle();
    ImGui::GetStyle().WindowRounding = 12.0f; // Smoother rounded corners

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool bRunning = true;
    int activeTab = 0;

    while (bRunning) {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) bRunning = false;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Streamlined Window: Narrower (600) and Taller (800)
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(460, 280));
        ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
        {
            float contentWidth = 460.0f;
            float contentHeight = 280.0f;
            ImGui::BeginGroup();
            {
                // Background Branding removed as requested

                // Purple 'x' Close Button
                ImGui::SetCursorPos(ImVec2(contentWidth - 25, 5));
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(168, 85, 247));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));
                if (ImGui::Button("x", ImVec2(20, 20))) bRunning = false;
                ImGui::PopStyleColor(4);

                if (activeTab == 0) {
                    UpdateParticles();
                    DrawParticles();

                    if (g_Scanning) {
                        ImGui::SetCursorPos(ImVec2(contentWidth / 2 - 25, 120));
                        ImGui::Dummy(ImVec2(50, 50));
                        DrawNapseSpinner(25.0f, 2.5f);

                        ImGui::SetCursorPos(ImVec2(contentWidth / 2 - 60, 185));
                        ImGui::SetWindowFontScale(0.8f);
                        ImGui::TextColored(ImVec4(1,1,1,0.25f), "SCANNING SYSTEM...");
                        ImGui::SetWindowFontScale(1.0f);
                    } else {
                        // Centered Start Button - Ultra Clean
                        ImGui::SetCursorPos(ImVec2(contentWidth / 2 - 75, 125));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.2f);
                        ImGui::PushStyleColor(ImGuiCol_Border, (ImVec4)ImColor(168, 85, 247, 120));
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                        if (ImGui::Button("Start", ImVec2(150, 50))) {
                            g_Scanning = true;
                            std::thread([]() { 
                                g_Scanner.StartScan(); 
                                g_Scanning = false;
                                g_Scanner.GenerateReport("ocean_report.html");
                            }).detach();
                        }
                        ImGui::PopStyleColor(2);
                        ImGui::PopStyleVar(2);
                    }
                }
            }
            ImGui::EndGroup();

            // Custom Rounded Progress Bar (Style Matching)
            if (g_Scanning) {
                ImDrawList* draw = ImGui::GetWindowDrawList();
                ImVec2 barPos = ImVec2(20, 240);
                ImVec2 barSize = ImVec2(420, 20);
                float progress = g_Scanner.GetProgress();
                
                // Background
                ImVec2 barEnd; barEnd.x = barPos.x + barSize.x; barEnd.y = barPos.y + barSize.y;
                draw->AddRectFilled(barPos, barEnd, (ImU32)ImColor(30, 25, 35, 255), 6.0f);
                // Shadow / Border
                draw->AddRect(barPos, barEnd, (ImU32)ImColor(168, 85, 247, 50), 6.0f);
                // Fill
                if (progress > 0.01f) {
                    ImVec2 fillEnd; fillEnd.x = barPos.x + (barSize.x * progress); fillEnd.y = barPos.y + barSize.y;
                    draw->AddRectFilled(barPos, fillEnd, (ImU32)ImColor(168, 85, 247, 255), 6.0f);
                }
                
                // Percentage Text
                char buf[32]; sprintf_s(buf, "%.0f%%", progress * 100.0f);
                ImVec2 txtSize = ImGui::CalcTextSize(buf);
                draw->AddText(ImVec2(barPos.x + (barSize.x - txtSize.x) / 2, barPos.y + (barSize.y - txtSize.y) / 2), ImColor(255,255,255,200), buf);
            }
        }
        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions (standard D3D11 setup)
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
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
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt; pt.x = LOWORD(lParam); pt.y = HIWORD(lParam);
            ScreenToClient(hWnd, &pt);
            if (pt.y < 60) return HTCAPTION;
        }
        return hit;
    }
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
