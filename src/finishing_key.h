#pragma once
#include <windows.h>

// Own the remainder of a key gesture consumed by a move/size operation.
// TranslateMessage may already have queued WM_CHAR when KEYDOWN finishes it.
class FinishingKey {
    HWND owner_ = nullptr;
    WPARAM key_ = 0;
    bool system_ = false;
public:
    void arm(HWND owner, WPARAM key, bool system = false) {
        owner_ = owner; key_ = key; system_ = system;
    }
    bool consume(HWND owner, UINT message, WPARAM key) {
        if (owner != owner_) return false;
        if (message == WM_DESTROY || message == WM_CANCELMODE ||
            (message == WM_ACTIVATE && LOWORD(key) == WA_INACTIVE)) {
            owner_ = nullptr;
            return false;
        }
        if ((message == WM_KEYDOWN || message == WM_CHAR ||
             (system_ && (message == WM_SYSKEYDOWN || message == WM_SYSCHAR))) && key == key_)
            return true;
        if ((message == WM_KEYUP || (system_ && message == WM_SYSKEYUP)) && key == key_) {
            owner_ = nullptr;
            return true;
        }
        return false;
    }
};
