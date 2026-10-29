#include "ui_geometry.h"
#include <cstdio>

int main() {
    struct Case {int position,saved,live,viewport,design,expected;};
    const Case cases[]={
        {0,215,214,1920,1024,0}, // left edge/minimap
        {812,212,211,1920,1024,1709}, // party panel: right edge
        {424,176,175,1920,1024,872}, // centred panel
        {100,505,504,1920,1024,188}, // near-side proportional offset
        {718,50,49,767,768,718}, // bottom edge
        {812,212,165,800,1024,635}, // smaller 4:3 window
        {812,212,212,1024,1024,812}, // design-space identity
        {0,1024,799,800,1024,0},
        {718,50,90,60,768,0} // oversized live panel: nonnegative
    };
    for(const auto& c:cases) {
        const int actual=rootUiPosition(c.position,c.saved,c.live,c.viewport,c.design);
        if(actual!=c.expected) {
            std::printf("position %d: expected %d, got %d\n",c.position,c.expected,actual);
            return 1;
        }
    }
    return 0;
}
