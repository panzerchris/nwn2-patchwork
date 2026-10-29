// Actual mode wrapper/operation finalizer. The native boundary must see no
// owned resize state, and deferred commit must precede native mode/input changes.
#include "probe.cpp"
#include <cstdlib>

static std::string order;
static unsigned resizeCalls,modeCalls;
static bool rejectResize,modeSawActive,resizeSawExclusive;
static HRESULT __fastcall resizeFixture(void* self,void*) {
    order+='R';++resizeCalls;
    resizeSawExclusive=field<unsigned char>(self,0xac)==0;
    if(rejectResize)return E_FAIL;
    RECT client{};GetClientRect(field<HWND>(self,0xf0),&client);
    *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xb8)=client.right;
    *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xbc)=client.bottom;
    return S_OK;
}
static HRESULT __fastcall modeFixture(void* self,void*) {
    order+='M';++modeCalls;
    modeSawActive=movingWindow==field<HWND>(self,0xf0);
    *reinterpret_cast<unsigned char*>(static_cast<char*>(self)+0xac)=0;
    return S_OK;
}
static bool runCase(HWND window,HWND operation,bool sizing,bool deferred,bool changed,bool failure) {
    alignas(void*) unsigned char recovery[0x340]{};
    recovery[0x30]=recovery[0xac]=recovery[0xb4]=1;
    *reinterpret_cast<HWND*>(recovery+0xf0)=window;
    *reinterpret_cast<void**>(recovery+0x100)=reinterpret_cast<void*>(1);
    *reinterpret_cast<unsigned*>(recovery+0xd8)=1;
    *reinterpret_cast<unsigned*>(recovery+0xb8)=changed?330:320;
    *reinterpret_cast<unsigned*>(recovery+0xbc)=240;
    movingWindow=operation;keyboardSizing=sizing;keyboardMove=!sizing;
    sizingHit=sizing?HTRIGHT:0;deferSizingReset=deferred;
    diagnostics=preservePlacement=fixGraphicsControls=syncWindowedMode=false;
    diagnosticHoldResizeBackbuffer=false;
    pendingWindowedSyncOwner=nullptr;pendingWindowedSyncWindow=nullptr;
    modeOriginal=reinterpret_cast<EngineFn>(modeFixture);
    resizeOriginal=reinterpret_cast<EngineFn>(resizeFixture);
    rejectResize=failure;modeSawActive=resizeSawExclusive=false;
    order.clear();resizeCalls=modeCalls=0;
    const bool owned=operation==window;
    const bool commit=owned && sizing && deferred && changed;
    const HRESULT result=modeHook(recovery,nullptr);
    bool ok=result==S_OK && modeCalls==1 && resizeCalls==(commit?1u:0u) &&
        order==(commit?"RM":"M") && !modeSawActive && !resizeSawExclusive && nesting==0;
    if(owned)ok=ok && !movingWindow && !keyboardSizing && sizingHit==0;
    else ok=ok && movingWindow==operation;
    movingWindow=nullptr;keyboardSizing=keyboardMove=false;sizingHit=0;
    if(!ok)std::fprintf(stderr,"Mode operation contract failed owned=%d sizing=%d defer=%d changed=%d fail=%d order=%s\n",
        owned,sizing,deferred,changed,failure,order.c_str());
    return ok;
}
int main() {
    HWND window=CreateWindowExW(0,L"STATIC",L"Mode operation fixture",WS_POPUP,
        0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    HWND other=CreateWindowExW(0,L"STATIC",L"Unrelated operation fixture",WS_POPUP,
        0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window || !other)return EXIT_FAILURE;
    const bool ok=runCase(window,window,true,true,true,false) &&
        runCase(window,window,true,true,false,false) &&
        runCase(window,window,true,false,false,false) &&
        runCase(window,window,false,true,true,false) &&
        runCase(window,nullptr,false,true,true,false) &&
        runCase(window,other,true,true,true,false) &&
        runCase(window,window,true,true,true,true);
    DestroyWindow(window);DestroyWindow(other);
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
