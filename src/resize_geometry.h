#pragma once
#include <windows.h>
#include <cstdint>
#include <climits>

// Pure geometry: preserve the edge opposite the dragged border. Use wide
// arithmetic before clamping, including for negative monitor coordinates.
inline bool resizeBounds(const RECT& start, UINT hit, int64_t dx, int64_t dy,
                         POINT minimum, POINT maximum, RECT& result) {
    const bool left=hit==HTLEFT || hit==HTTOPLEFT || hit==HTBOTTOMLEFT;
    const bool right=hit==HTRIGHT || hit==HTTOPRIGHT || hit==HTBOTTOMRIGHT;
    const bool top=hit==HTTOP || hit==HTTOPLEFT || hit==HTTOPRIGHT;
    const bool bottom=hit==HTBOTTOM || hit==HTBOTTOMLEFT || hit==HTBOTTOMRIGHT;
    if(!(left || right || top || bottom) || minimum.x<=0 || minimum.y<=0 ||
       maximum.x<minimum.x || maximum.y<minimum.y ||
       start.right<=start.left || start.bottom<=start.top)return false;
    int64_t width=int64_t(start.right)-start.left;
    int64_t height=int64_t(start.bottom)-start.top;
    if(left || right)width=max(int64_t(minimum.x),min(int64_t(maximum.x),width+(left?-dx:dx)));
    if(top || bottom)height=max(int64_t(minimum.y),min(int64_t(maximum.y),height+(top?-dy:dy)));
    const int64_t x=left?int64_t(start.right)-width:start.left;
    const int64_t y=top?int64_t(start.bottom)-height:start.top;
    if(x<LONG_MIN || y<LONG_MIN || x+width>LONG_MAX || y+height>LONG_MAX ||
       width>LONG_MAX || height>LONG_MAX)return false;
    result={static_cast<LONG>(x),static_cast<LONG>(y),
            static_cast<LONG>(x+width),static_cast<LONG>(y+height)};
    return true;
}
