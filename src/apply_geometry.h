#pragma once
#include <windows.h>
#include <cstdint>
#include <climits>

// Fit the normal outer window, preserving position wherever it still fits.
// Work-area coordinates may be negative; never narrow arithmetic before checks.
inline bool fitApplyBounds(int x,int y,int width,int height,const RECT& work,RECT& out) {
    const int64_t availableWidth=int64_t(work.right)-work.left;
    const int64_t availableHeight=int64_t(work.bottom)-work.top;
    if(width<=0 || height<=0 || availableWidth<=0 || availableHeight<=0 ||
       availableWidth>INT_MAX || availableHeight>INT_MAX)return false;
    const int64_t w=min(int64_t(width),availableWidth);
    const int64_t h=min(int64_t(height),availableHeight);
    const int64_t left=max(int64_t(work.left),min(int64_t(x),int64_t(work.right)-w));
    const int64_t top=max(int64_t(work.top),min(int64_t(y),int64_t(work.bottom)-h));
    if(left<LONG_MIN || top<LONG_MIN || left+w>LONG_MAX || top+h>LONG_MAX)return false;
    out={static_cast<LONG>(left),static_cast<LONG>(top),
         static_cast<LONG>(left+w),static_cast<LONG>(top+h)};
    return true;
}
