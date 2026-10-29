#define WIN32_LEAN_AND_MEAN
#include "resize_geometry.h"
#include <cstdio>

int main() {
    const RECT start{-1000,-500,0,300};
    const POINT minimum{800,600}, maximum{2000,1500};
    struct Case { UINT hit; int dx,dy; RECT expected; };
    const Case cases[]={
        {HTLEFT,100,100,{-900,-500,0,300}},
        {HTRIGHT,100,100,{-1000,-500,100,300}},
        {HTTOP,100,100,{-1000,-400,0,300}},
        {HTBOTTOM,100,100,{-1000,-500,0,400}},
        {HTTOPLEFT,100,100,{-900,-400,0,300}},
        {HTTOPRIGHT,100,100,{-1000,-400,100,300}},
        {HTBOTTOMLEFT,100,100,{-900,-500,0,400}},
        {HTBOTTOMRIGHT,100,100,{-1000,-500,100,400}},
        {HTTOPLEFT,3000,3000,{-800,-300,0,300}},
        {HTBOTTOMRIGHT,-3000,-3000,{-1000,-500,-200,100}},
        {HTTOPLEFT,-3000,-3000,{-2000,-1200,0,300}},
        {HTBOTTOMRIGHT,3000,3000,{-1000,-500,1000,1000}},
        {HTLEFT,0,0,{-1000,-500,0,300}},
    };
    unsigned failures=0;
    for(const auto& test:cases) {
        RECT actual{};
        if(!resizeBounds(start,test.hit,test.dx,test.dy,minimum,maximum,actual) ||
           actual.left!=test.expected.left || actual.top!=test.expected.top ||
           actual.right!=test.expected.right || actual.bottom!=test.expected.bottom) {
            std::printf("Geometry mismatch: hit=%u dx=%d dy=%d\n",test.hit,test.dx,test.dy);
            ++failures;
        }
    }
    RECT result{};
    if(resizeBounds(start,HTCAPTION,0,0,minimum,maximum,result) ||
       resizeBounds(start,HTLEFT,0,0,{0,600},maximum,result) ||
       resizeBounds(start,HTLEFT,0,0,minimum,{799,1500},result) ||
       resizeBounds({0,0,0,600},HTLEFT,0,0,minimum,maximum,result) ||
       resizeBounds({LONG_MIN,0,LONG_MIN+1000,800},HTLEFT,-100,0,minimum,maximum,result) ||
       resizeBounds({LONG_MAX-1000,0,LONG_MAX,800},HTRIGHT,100,0,minimum,maximum,result)) {
        std::puts("Invalid or overflowing geometry accepted");
        ++failures;
    }
    // Changing one dimension must not silently clamp the untouched dimension.
    if(!resizeBounds({0,0,1000,400},HTRIGHT,100,0,minimum,maximum,result) ||
       result.bottom!=400)++failures;
    std::printf("Resize geometry: %u failure(s)\n",failures);
    return failures?1:0;
}
