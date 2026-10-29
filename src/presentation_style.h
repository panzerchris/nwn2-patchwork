#pragma once
#include <windows.h>

inline DWORD borderlessPresentationStyle(DWORD original) {
    // Remove the visible frame, not the existing close/minimize capabilities.
    // Retaining SYSMENU/MINIMIZEBOX permits normal taskbar/system-menu actions.
    return (original & ~(WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZE | WS_MAXIMIZEBOX)) | WS_POPUP;
}
