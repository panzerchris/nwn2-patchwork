// Exercise the real resolution boundary around a model of native Apply's
// proven resize-then-overwrite order. Owned hidden Win32 window, no game or D3D.
#include "probe.cpp"
#include <cstdlib>

static unsigned nativeCalls;
static HRESULT resizeResult;
static bool mismatchBackbuffer;

static HRESULT __fastcall nativeResizeFixture(void* self,void*) {
    RECT client{};
    if(!GetClientRect(field<HWND>(self,0xf0),&client))return E_FAIL;
    *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xb8)=client.right;
    *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xbc)=client.bottom;
    if(mismatchBackbuffer)++*reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xb8);
    if(FAILED(resizeResult))*reinterpret_cast<unsigned char*>(static_cast<char*>(self)+0xaf)=1;
    return resizeResult;
}
static void __cdecl nativeApplyFixture(const unsigned* requested) {
    ++nativeCalls;
    auto app=*graphicsGlobals.app;
    auto selected=graphicsGlobals.selected;
    if(std::memcmp(selected,requested,3*sizeof(unsigned))==0)return;
    std::memcpy(selected,requested,4*sizeof(unsigned));
    auto self=app+0x20;
    resizeHook(self,nullptr);
    // 4427e6..442808: these stores occur even after failed native resize.
    std::memcpy(self+0x88,requested,3*sizeof(unsigned));
    std::memcpy(self+0x50,requested,2*sizeof(unsigned));
}
static bool runCase(HWND window,bool enabled,unsigned blockedField,
                    bool badBackbuffer,bool failResize,bool unchangedBorderless) {
    alignas(void*) unsigned char app[0x340]{};
    auto self=app+0x20;
    *graphicsGlobals.app=app;
    auto selected=graphicsGlobals.selected;
    const unsigned requested[4]={2560,1440,0xffffffffu,73};
    const unsigned initial[4]={1920,1080,60,0};
    std::memcpy(selected,unchangedBorderless?requested:initial,sizeof(initial));
    self[0x30]=self[0xac]=self[0xb4]=1;
    *reinterpret_cast<unsigned*>(self+0xd8)=1;
    *reinterpret_cast<HWND*>(self+0xf0)=window;
    *reinterpret_cast<void**>(self+0x100)=reinterpret_cast<void*>(1);
    *reinterpret_cast<unsigned*>(self+0xb8)=320;
    *reinterpret_cast<unsigned*>(self+0xbc)=240;
    *reinterpret_cast<unsigned*>(self+0x50)=640;
    *reinterpret_cast<unsigned*>(self+0x54)=480;
    if(blockedField)self[blockedField]=0;
    diagnostics=diagnosticHoldResizeBackbuffer=false;
    syncWindowedMode=enabled;
    borderlessWindow=unchangedBorderless?window:nullptr;
    pendingWindowedSyncOwner=nullptr;pendingWindowedSyncWindow=nullptr;
    nativeCalls=0;resizeResult=failResize?E_FAIL:S_OK;
    mismatchBackbuffer=badBackbuffer;
    resizeOriginal=reinterpret_cast<EngineFn>(nativeResizeFixture);
    resolutionOriginal=nativeApplyFixture;
    resolutionHook(requested);
    const bool shouldSync=enabled && !blockedField && !badBackbuffer &&
        !failResize && !unchangedBorderless;
    const unsigned expectedWidth=unchangedBorderless?640:shouldSync?320:2560;
    const unsigned expectedHeight=unchangedBorderless?480:shouldSync?240:1440;
    bool ok=nativeCalls==1 && field<unsigned>(self,0x50)==expectedWidth &&
        field<unsigned>(self,0x54)==expectedHeight &&
        std::memcmp(selected,requested,sizeof(requested))==0 &&
        field<unsigned char>(self,0xaf)==(failResize?1:0) &&
        field<unsigned char>(self,0xb4)==(blockedField==0xb4?0:1);
    if(!unchangedBorderless)
        ok=ok && std::memcmp(self+0x88,requested,3*sizeof(unsigned))==0;
    if(failResize)
        ok=ok && pendingWindowedSyncOwner==self && pendingWindowedSyncWindow==window;
    if(unchangedBorderless)ok=ok && borderlessWindow==window;
    if(!ok)std::fprintf(stderr,"Resolution boundary failed: enabled=%u blocked=%x mismatch=%u fail=%u borderless=%u actual=%u,%u expected=%u,%u\n",
        enabled,blockedField,badBackbuffer,failResize,unchangedBorderless,
        field<unsigned>(self,0x50),field<unsigned>(self,0x54),expectedWidth,expectedHeight);
    borderlessWindow=nullptr;pendingWindowedSyncOwner=nullptr;pendingWindowedSyncWindow=nullptr;
    return ok;
}
int main() {
    unsigned char* app=nullptr;
    unsigned selected[4]{};
    graphicsGlobals.app=&app;graphicsGlobals.selected=selected;
    HWND window=CreateWindowExW(0,L"STATIC",L"Resolution synchronization fixture",
        WS_POPUP,160,160,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window)return EXIT_FAILURE;
    const bool ok=runCase(window,true,0,false,false,false) &&
        runCase(window,false,0,false,false,false) &&
        runCase(window,true,0xb4,false,false,false) &&
        runCase(window,true,0xac,false,false,false) &&
        runCase(window,true,0x30,false,false,false) &&
        runCase(window,true,0,true,false,false) &&
        runCase(window,true,0,false,true,false) &&
        runCase(window,true,0,false,false,true);
    DestroyWindow(window);
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
