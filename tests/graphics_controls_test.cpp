// Native-call fixtures only; no game or D3D device. The actual wrappers and
// ownership guards run around an owned hidden window and synthetic UI scene.
#include "probe.cpp"
#include <cstdlib>

static void* expectedPane;
static void* fixtureCheckbox;
static unsigned checks;
static bool checkedValue;
static bool validName=true;
static void* __fastcall findFixture(void* pane,void*,const UiNameRef* name) {
    validName=validName && pane==expectedPane && name->length==14 &&
        std::strcmp(name->text,"CHECK_WINDOWED")==0;
    return fixtureCheckbox;
}
static void __fastcall checkFixture(void* checkbox,void*,bool value) {
    if(checkbox!=fixtureCheckbox)validName=false;
    ++checks;checkedValue=value;
}
static void __cdecl populateFixture(void*) {}
static bool selectionCases() {
    const unsigned modes[][4]={{800,600,60,0},{1920,1080,60,0},{2560,1440,60,0}};
    const unsigned* modePointer=&modes[0][0];
    int modeCount=3;
    graphicsGlobals.modes=&modePointer;graphicsGlobals.modeCount=&modeCount;
    auto selected=graphicsGlobals.selected;
    selected[0]=2560;selected[1]=1440;
    fixGraphicsControls=true;
    unsigned w=2560,h=1421;
    preferGraphicsSelection(w,h,0x80c0b2);
    bool ok=w==2560 && h==1440;
    w=1520;h=1080;selected[0]=1920;selected[1]=1080;
    preferGraphicsSelection(w,h,0x80c0b2);
    ok=ok && w==1920 && h==1080;
    fixGraphicsControls=false;w=1520;h=1080;
    preferGraphicsSelection(w,h,0x80c0b2);
    ok=ok && w==1520 && h==1080;
    fixGraphicsControls=true;
    preferGraphicsSelection(w,h,0x1234); // Other/unknown caller must stay native.
    ok=ok && w==1520 && h==1080;
    selected[0]=1234;selected[1]=777;
    preferGraphicsSelection(w,h,0x80c0b2);
    ok=ok && w==1520 && h==1080; // Do not invent an unsupported fullscreen mode.
    selected[0]=1920;selected[1]=1080;
    modeCount=4097;
    preferGraphicsSelection(w,h,0x80c0b2);
    ok=ok && w==1520 && h==1080;
    modeCount=0;
    preferGraphicsSelection(w,h,0x80c0b2);
    ok=ok && w==1520 && h==1080;
    modeCount=3;
    modePointer=nullptr;
    preferGraphicsSelection(w,h,0x80c0b2);
    return ok && w==1520 && h==1080;
}
static bool checkboxCases(HWND window) {
    alignas(void*) unsigned char app[0x400]{},manager[0x100]{},pane[0x200]{},checkbox[0x280]{};
    unsigned char scene[8]{};
    unsigned char* scenes[]={scene};
    auto self=app+0x20;
    *graphicsGlobals.app=app;
    unsigned char* managerPointer=manager;
    graphicsGlobals.scenes=&managerPointer;
    *reinterpret_cast<unsigned char***>(manager+0x78)=scenes;
    *reinterpret_cast<unsigned*>(manager+0x80)=1;
    *reinterpret_cast<void**>(scene+4)=pane;
    *reinterpret_cast<uintptr_t*>(pane)=0x960ea4;
    *reinterpret_cast<uintptr_t*>(checkbox)=0x962554;
    *reinterpret_cast<unsigned*>(checkbox+0x264)=0x800;
    *reinterpret_cast<unsigned*>(checkbox+0x270)=0x12345678; // Other unsaved control data.
    self[0xac]=self[0xb4]=1;
    *reinterpret_cast<HWND*>(self+0xf0)=window;
    expectedPane=pane;fixtureCheckbox=checkbox;
    uiFind=reinterpret_cast<UiFindFn>(findFixture);
    uiCheck=reinterpret_cast<UiCheckFn>(checkFixture);
    graphicsPopulateOriginal=populateFixture;
    diagnostics=false;fixGraphicsControls=true;checks=0;
    graphicsPopulateHook(pane);
    bool ok=checks==1 && !checkedValue;
    self[0xac]=0;refreshFullscreenCheckbox(self);
    ok=ok && checks==2 && checkedValue;
    self[0xaf]=1;refreshFullscreenCheckbox(self);
    self[0xaf]=0;self[0xb4]=0;refreshFullscreenCheckbox(self);
    self[0xb4]=1;self[0xb2]=1;refreshFullscreenCheckbox(self);
    self[0xb2]=0;
    ok=ok && checks==2;
    *reinterpret_cast<unsigned*>(manager+0x80)=0;
    refreshFullscreenCheckbox(self); // Closed or not-yet-inserted pane: don't dereference.
    ok=ok && checks==2;
    *reinterpret_cast<unsigned*>(manager+0x80)=1;
    refreshFullscreenCheckbox(self);
    ok=ok && checks==3;
    *reinterpret_cast<uintptr_t*>(pane)=0;
    refreshFullscreenCheckbox(self);
    *reinterpret_cast<uintptr_t*>(pane)=0x960ea4;
    *reinterpret_cast<uintptr_t*>(checkbox)=0x961c04;
    refreshFullscreenCheckbox(self); // Superseded type assumption must refuse.
    *reinterpret_cast<uintptr_t*>(checkbox)=0x962554;
    *reinterpret_cast<unsigned*>(checkbox+0x264)=0x400;
    refreshFullscreenCheckbox(self); // A same-named non-checkbox is refused.
    *reinterpret_cast<unsigned*>(checkbox+0x264)=0x800;
    fixGraphicsControls=false;refreshFullscreenCheckbox(self);
    fixGraphicsControls=true;graphicsPaneThread=GetCurrentThreadId()+1;
    refreshFullscreenCheckbox(self);
    return ok && checks==3 && validName && field<unsigned>(checkbox,0x270)==0x12345678;
}
int main() {
    unsigned char* app=nullptr;
    unsigned selected[4]{};
    graphicsGlobals.app=&app;graphicsGlobals.selected=selected;
    HWND window=CreateWindowExW(0,L"STATIC",L"Graphics controls fixture",WS_OVERLAPPEDWINDOW,
        100,100,400,300,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window) { std::fprintf(stderr,"Fixture window error=%lu\n",GetLastError());return EXIT_FAILURE; }
    const bool ok=selectionCases() && checkboxCases(window);
    DestroyWindow(window);
    if(!ok)std::fprintf(stderr,"Graphics control selection/ownership regression\n");
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
