#define WIN32_LEAN_AND_MEAN
#include "finishing_key.h"
#include <cstdlib>
#include <initializer_list>
static void require(bool value) { if (!value) std::abort(); }
int main() {
    auto first = reinterpret_cast<HWND>(1);
    auto second = reinterpret_cast<HWND>(2);
    FinishingKey key;
    for (WPARAM code : {WPARAM(VK_RETURN), WPARAM(VK_ESCAPE)}) {
        key.arm(first, code);
        require(!key.consume(second, WM_CHAR, code));
        require(!key.consume(first, WM_KEYDOWN, 'I'));
        require(key.consume(first, WM_CHAR, code));
        require(key.consume(first, WM_KEYDOWN, code));
        require(key.consume(first, WM_KEYUP, code));
        require(!key.consume(first, WM_KEYDOWN, code));
        key.arm(first, code);
        require(!key.consume(first, WM_ACTIVATE, WA_INACTIVE));
        require(!key.consume(first, WM_CHAR, code));
        key.arm(first, code);
        require(!key.consume(first, WM_DESTROY, 0));
        require(!key.consume(first, WM_KEYUP, code));
    }
    // Alt+Enter owns system characters/repeats, but Alt may be released before
    // Enter, turning the final release into an ordinary WM_KEYUP.
    for (UINT release : {UINT(WM_SYSKEYUP), UINT(WM_KEYUP)}) {
        key.arm(first, VK_RETURN, true);
        require(!key.consume(second, WM_SYSKEYUP, VK_RETURN));
        require(!key.consume(first, WM_SYSKEYUP, VK_MENU));
        require(key.consume(first, WM_SYSCHAR, VK_RETURN));
        require(key.consume(first, WM_SYSKEYDOWN, VK_RETURN));
        require(key.consume(first, release, VK_RETURN));
        require(!key.consume(first, WM_SYSCHAR, VK_RETURN));
        require(!key.consume(first, WM_KEYDOWN, VK_RETURN));
    }
    key.arm(first, VK_RETURN);
    require(!key.consume(first, WM_SYSCHAR, VK_RETURN));
    require(!key.consume(first, WM_SYSKEYUP, VK_RETURN));
    require(key.consume(first, WM_KEYUP, VK_RETURN));
}
