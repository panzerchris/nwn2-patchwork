#define WIN32_LEAN_AND_MEAN
#include "presentation_style.h"
#include <cstdlib>
static void require(bool value) { if (!value) std::abort(); }
int main() {
    const DWORD original = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_MAXIMIZE;
    const DWORD result = borderlessPresentationStyle(original);
    require((result & WS_POPUP) != 0);
    require((result & (WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZE | WS_MAXIMIZEBOX)) == 0);
    require((result & (WS_SYSMENU | WS_MINIMIZEBOX)) == (WS_SYSMENU | WS_MINIMIZEBOX));
    require((result & (WS_VISIBLE | WS_CLIPCHILDREN)) == (WS_VISIBLE | WS_CLIPCHILDREN));
    // Do not add controls that were absent from the original window.
    require((borderlessPresentationStyle(WS_VISIBLE) & (WS_SYSMENU | WS_MINIMIZEBOX)) == 0);
    require(borderlessPresentationStyle(result) == result);
}
