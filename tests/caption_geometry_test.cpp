#define WIN32_LEAN_AND_MEAN
#include "caption_geometry.h"
#include <cstdio>
int main() {
    unsigned failures=0;
    const POINT origin{-1400,10};
    if(captionDragStarted(origin,{-1399,11},4,4) ||
       captionDragStarted(origin,{-1396,14},4,4) ||
       !captionDragStarted(origin,{-1395,10},4,4) ||
       !captionDragStarted(origin,{-1400,5},4,4) ||
       !captionDragStarted({LONG_MIN,0},{LONG_MAX,0},4,4))++failures;
    const RECT maximum{-1928,-8,8,1040},normal{100,100,1716,1039};
    struct Case { POINT press,cursor; RECT expected; };
    const Case cases[]={
        {{-1420,9},{-1320,110},{-1828,93,-212,1032}},
        {{-100,9},{100,110},{-1408,93,208,1032}},
        {{-960,9},{500,110},{-308,93,1308,1032}},
    };
    for(const auto& c:cases) {
        RECT result{};
        if(!restoredCaptionBounds(maximum,normal,c.press,c.cursor,result) ||
           !EqualRect(&result,&c.expected))++failures;
    }
    RECT result{};
    if(restoredCaptionBounds(maximum,{0,0,0,600},{-1400,9},{0,0},result) ||
       restoredCaptionBounds(maximum,normal,{100,9},{0,0},result) ||
       restoredCaptionBounds(maximum,normal,{-1400,9},{LONG_MIN,0},result) ||
       restoredCaptionBounds(maximum,normal,{-1400,9},{LONG_MAX,0},result))++failures;
    std::printf("Maximized caption geometry: %u failure(s)\n",failures);
    return failures?1:0;
}
