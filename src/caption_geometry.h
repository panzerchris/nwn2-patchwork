#pragma once
#include <windows.h>
#include <cstdint>
#include <climits>

inline bool captionDragStarted(POINT origin,POINT cursor,int cx,int cy) {
    const int64_t dx=int64_t(cursor.x)-origin.x,dy=int64_t(cursor.y)-origin.y;
    const int64_t x=cx<0?-int64_t(cx):cx,y=cy<0?-int64_t(cy):cy;
    return dx < -x || dx > x || dy < -y || dy > y;
}

// Keep the grabbed caption point near the same edge when the normal window is
// narrower than its maximized rectangle. Center the grip only when neither
// edge fits. This is our explicit drag policy, not a clone of Windows Snap.
inline bool restoredCaptionBounds(const RECT& maximized,const RECT& restored,
                                 POINT origin,POINT cursor,RECT& result) {
    const int64_t mw=int64_t(maximized.right)-maximized.left;
    const int64_t mh=int64_t(maximized.bottom)-maximized.top;
    const int64_t width=int64_t(restored.right)-restored.left;
    const int64_t height=int64_t(restored.bottom)-restored.top;
    if(mw<=0 || mh<=0 || width<=0 || height<=0 || width>LONG_MAX || height>LONG_MAX ||
       origin.x<maximized.left || origin.x>=maximized.right ||
       origin.y<maximized.top || origin.y>=maximized.bottom)return false;
    const int64_t fromLeft=int64_t(origin.x)-maximized.left;
    const int64_t fromRight=int64_t(maximized.right)-origin.x;
    const int64_t gripX=fromLeft<=mw/2?min(fromLeft,width/2):width-min(fromRight,width/2);
    const int64_t gripY=min(int64_t(origin.y)-maximized.top,height-1);
    const int64_t x=int64_t(cursor.x)-gripX,y=int64_t(cursor.y)-gripY;
    if(x<LONG_MIN || y<LONG_MIN || x+width>LONG_MAX || y+height>LONG_MAX)return false;
    result={static_cast<LONG>(x),static_cast<LONG>(y),
            static_cast<LONG>(x+width),static_cast<LONG>(y+height)};
    return true;
}
