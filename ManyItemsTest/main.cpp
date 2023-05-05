#include "pch.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
}

std::vector<HWND> GetCapturableWindows();
bool IsCapturableWindow(HWND hwnd);

int wmain()
{
    winrt::init_apartment();

    auto interopFactory = winrt::get_activation_factory<
        winrt::GraphicsCaptureItem,
        IGraphicsCaptureItemInterop>();

    std::vector<winrt::GraphicsCaptureItem> items;
    while (true)
    {
        auto windows = GetCapturableWindows();

        items.reserve(windows.size());
        for (auto&& window : windows)
        {
            winrt::GraphicsCaptureItem item{ nullptr };
            winrt::check_hresult(interopFactory->CreateForWindow(
                window,
                winrt::guid_of<winrt::GraphicsCaptureItem>(),
                winrt::put_abi(item)));
            items.push_back(item);
        }

        Sleep(100);
        items.clear();
    }

    return 0;
}

std::vector<HWND> GetCapturableWindows()
{
    std::vector<HWND> windows;

    EnumWindows([](HWND hwnd, LPARAM lParam)
        {
            if (GetWindowTextLengthW(hwnd) > 0)
            {
                if (IsCapturableWindow(hwnd))
                {
                    auto windows = reinterpret_cast<std::vector<HWND>*>(lParam);
                    windows->push_back(hwnd);
                }
            }

            return TRUE;
        }, reinterpret_cast<LPARAM>(&windows));

    return windows;
}

bool IsCapturableWindow(HWND hwnd)
{
    if (hwnd == GetShellWindow() ||
        !IsWindowVisible(hwnd) || 
        GetAncestor(hwnd, GA_ROOT) != hwnd)
    {
        return false;
    }

    auto style = GetWindowLongW(hwnd, GWL_STYLE);
    if (style & WS_DISABLED)
    {
        return false;
    }

    auto exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW)    // No tooltips
    {
        return false;
    }

    auto classNameLength = 256;
    std::wstring className(classNameLength, 0);
    classNameLength = GetClassNameW(hwnd, className.data(), classNameLength);
    className.resize(classNameLength);

    // Check to see if the window is cloaked if it's a UWP
    if (wcscmp(className.c_str(), L"Windows.UI.Core.CoreWindow") == 0 ||
        wcscmp(className.c_str(), L"ApplicationFrameWindow") == 0)
    {
        DWORD cloaked = FALSE;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && (cloaked == DWM_CLOAKED_SHELL))
        {
            return false;
        }
    }

    return true;
}
