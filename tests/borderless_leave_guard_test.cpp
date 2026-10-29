// Exercise the actual private transition with an owned hidden Win32 window and
// a synthetic recovery fixture. No game, executable patch or D3D device is used.
#include "probe.cpp"
#include <cstdlib>

static bool refusedWithoutMutation(unsigned blockedField, unsigned char value) {
    HWND h=CreateWindowExW(0,L"STATIC",L"NWN2 transition guard test",
        WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX,160,160,320,240,
        nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!h)return false;
    alignas(void*) unsigned char recovery[0x300]{};
    recovery[0xac]=1;
    recovery[0xb4]=1;
    *reinterpret_cast<unsigned*>(recovery+0xd8)=1;
    *reinterpret_cast<HWND*>(recovery+0xf0)=h;
    *reinterpret_cast<void**>(recovery+0x100)=reinterpret_cast<void*>(1);
    RECT client{};
    if(!presentationResizeReady(h,recovery,client)) {
        DestroyWindow(h);
        return false; // The shared gate must also accept the valid fixture.
    }
    if(blockedField==0x100)*reinterpret_cast<void**>(recovery+0x100)=nullptr;
    else if(blockedField==0xd8)*reinterpret_cast<unsigned*>(recovery+0xd8)=0;
    else recovery[blockedField]=value;

    RECT before{},after{};
    GetWindowRect(h,&before);
    const LONG styleBefore=GetWindowLongW(h,GWL_STYLE);
    borderlessWindow=h;
    borderlessStyle=(styleBefore & ~WS_POPUP)|WS_OVERLAPPEDWINDOW;
    borderlessPlacement={};
    borderlessPlacement.length=sizeof(borderlessPlacement);
    GetWindowPlacement(h,&borderlessPlacement);
    borderlessPlacement.rcNormalPosition={32,32,832,632};
    diagnostics=false;
    const HRESULT result=leaveBorderless(recovery);
    GetWindowRect(h,&after);
    const bool ok=FAILED(result) && borderlessWindow==h &&
        !borderlessTransition && GetWindowLongW(h,GWL_STYLE)==styleBefore &&
        EqualRect(&before,&after) && !IsWindowVisible(h);
    if(!ok)std::fprintf(stderr,"Blocked recovery field 0x%x changed window or ownership\n",blockedField);
    borderlessWindow=nullptr;
    DestroyWindow(h);
    return ok;
}

static unsigned resizeAttempts;
static HRESULT recoveryResult;
static HRESULT __fastcall fakeRecovery(void*,void*) { return recoveryResult; }
static HRESULT __fastcall failResizeAfterDimensionUpdate(void* self,void*) {
    ++resizeAttempts;
    RECT client{};
    GetClientRect(field<HWND>(self,0xf0),&client);
    *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xb8)=client.right;
    *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xbc)=client.bottom;
    *reinterpret_cast<unsigned char*>(static_cast<char*>(self)+0xaf)=1;
    return static_cast<HRESULT>(0x88760868u);
}
static bool failureAfterPreflight(bool invalidPlacement) {
    HWND h=CreateWindowExW(0,L"STATIC",L"NWN2 exit failure test",
        WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX,160,160,320,240,
        nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!h)return false;
    alignas(void*) unsigned char recovery[0x300]{};
    recovery[0xac]=recovery[0xb4]=1;
    *reinterpret_cast<unsigned*>(recovery+0xd8)=1;
    *reinterpret_cast<HWND*>(recovery+0xf0)=h;
    *reinterpret_cast<void**>(recovery+0x100)=reinterpret_cast<void*>(1);
    *reinterpret_cast<unsigned*>(recovery+0xb8)=320;
    *reinterpret_cast<unsigned*>(recovery+0xbc)=240;
    RECT before{},after{};
    GetWindowRect(h,&before);
    const LONG styleBefore=GetWindowLongW(h,GWL_STYLE);
    borderlessWindow=h;
    borderlessStyle=(styleBefore & ~WS_POPUP)|WS_OVERLAPPEDWINDOW;
    borderlessPlacement={};borderlessPlacement.length=sizeof(borderlessPlacement);
    GetWindowPlacement(h,&borderlessPlacement);
    borderlessPlacement.rcNormalPosition={32,32,832,632};
    borderlessPlacement.showCmd=SW_HIDE;
    if(invalidPlacement)borderlessPlacement.length=0; // Real Win32 API refusal.
    resizeAttempts=0;
    resizeOriginal=reinterpret_cast<EngineFn>(failResizeAfterDimensionUpdate);
    diagnostics=false;syncWindowedMode=true;
    recovery[0x30]=1;
    *reinterpret_cast<unsigned*>(recovery+0x50)=2560;
    *reinterpret_cast<unsigned*>(recovery+0x54)=1440;
    const HRESULT result=leaveBorderless(recovery);
    GetWindowRect(h,&after);
    RECT client{};GetClientRect(h,&client);
    bool ok=invalidPlacement ?
        FAILED(result) && resizeAttempts==0 && borderlessWindow==h &&
            GetWindowLongW(h,GWL_STYLE)==styleBefore && EqualRect(&before,&after) :
        result==static_cast<HRESULT>(0x88760868u) && resizeAttempts==1 &&
            borderlessWindow==nullptr && GetWindowLongW(h,GWL_STYLE)==borderlessStyle &&
            field<unsigned char>(recovery,0xaf)==1 &&
            field<unsigned>(recovery,0xb8)==static_cast<unsigned>(client.right) &&
            field<unsigned>(recovery,0xbc)==static_cast<unsigned>(client.bottom) &&
            leaveBorderless(recovery)==S_OK && resizeAttempts==1;
    if(!invalidPlacement) {
        ok=pendingWindowedSyncOwner==recovery && pendingWindowedSyncWindow==h && ok;
        // Exercise the actual recovery wrapper after failed exit. Native
        // DEVICELOST may return S_OK while the lost flag remains set.
        recoveryOriginal=reinterpret_cast<EngineFn>(fakeRecovery);
        recoveryResult=S_OK;
        recoveryHook(recovery,nullptr);
        ok=field<unsigned>(recovery,0x50)==2560 && ok;
        recovery[0xaf]=0;recovery[0xb4]=0;
        recoveryHook(recovery,nullptr);
        ok=field<unsigned>(recovery,0x50)==2560 && ok;
        recovery[0xb4]=1;
        ++*reinterpret_cast<unsigned*>(recovery+0xb8);
        recoveryHook(recovery,nullptr);
        ok=field<unsigned>(recovery,0x50)==2560 && ok;
        --*reinterpret_cast<unsigned*>(recovery+0xb8);
        recoveryResult=E_FAIL;
        recoveryHook(recovery,nullptr);
        ok=field<unsigned>(recovery,0x50)==2560 && ok;
        recoveryResult=S_OK;
        *reinterpret_cast<HWND*>(recovery+0xf0)=nullptr;
        recoveryHook(recovery,nullptr);
        ok=field<unsigned>(recovery,0x50)==2560 && ok;
        *reinterpret_cast<HWND*>(recovery+0xf0)=h;
        recoveryHook(recovery,nullptr);
        ok=field<unsigned>(recovery,0x50)==static_cast<unsigned>(client.right) &&
            field<unsigned>(recovery,0x54)==static_cast<unsigned>(client.bottom) && ok;
        ok=pendingWindowedSyncOwner==nullptr && pendingWindowedSyncWindow==nullptr && ok;
        *reinterpret_cast<unsigned*>(recovery+0x50)=999;
        recoveryHook(recovery,nullptr);
        ok=field<unsigned>(recovery,0x50)==999 && ok; // Completed request is gone.
        recoveryOriginal=nullptr;
    }
    const bool hidden=!IsWindowVisible(h);
    if(!ok || !hidden)std::fprintf(stderr,"Post-preflight %s failure: result=%lx attempts=%u owner=%p style=%lx expected=%lx client=%ld,%ld engine=%u,%u lost=%u hidden=%u\n",
        invalidPlacement?"placement":"engine",static_cast<unsigned long>(result),resizeAttempts,
        borderlessWindow,GetWindowLongW(h,GWL_STYLE),borderlessStyle,client.right,client.bottom,
        field<unsigned>(recovery,0xb8),field<unsigned>(recovery,0xbc),
        field<unsigned char>(recovery,0xaf),hidden);
    borderlessWindow=nullptr;resizeOriginal=nullptr;
    DestroyWindow(h);
    return ok && hidden;
}
static bool failedOrdinaryResizeRecovery() {
    HWND h=CreateWindowExW(0,L"STATIC",L"NWN2 ordinary resize recovery test",
        WS_OVERLAPPEDWINDOW,100,100,640,480,
        nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!h)return false;
    alignas(void*) unsigned char recovery[0x300]{};
    recovery[0x30]=recovery[0xac]=recovery[0xb4]=1;
    *reinterpret_cast<unsigned*>(recovery+0xd8)=1;
    *reinterpret_cast<HWND*>(recovery+0xf0)=h;
    *reinterpret_cast<void**>(recovery+0x100)=reinterpret_cast<void*>(1);
    *reinterpret_cast<unsigned*>(recovery+0x50)=1920;
    *reinterpret_cast<unsigned*>(recovery+0x54)=1080;
    diagnostics=tracePresentation=borderlessHotkey=borderlessAltEnter=false;
    // A diagnostic fault request must never touch the fake device or interfere
    // with a proposed fix when diagnostics are off, even if explicitly armed.
    diagnosticHoldResizeBackbuffer=true;
    resetFaultOwner=recovery;resetFaultWindow=h;
    syncWindowedMode=true;
    pendingWindowedSyncOwner=nullptr;pendingWindowedSyncWindow=nullptr;
    resizeOriginal=reinterpret_cast<EngineFn>(failResizeAfterDimensionUpdate);
    recoveryOriginal=reinterpret_cast<EngineFn>(fakeRecovery);
    const HRESULT result=resizeHook(recovery,nullptr);
    bool ok=result==static_cast<HRESULT>(0x88760868u) &&
        pendingWindowedSyncOwner==recovery && pendingWindowedSyncWindow==h &&
        field<unsigned>(recovery,0x50)==1920;
    recoveryResult=S_OK;
    recoveryHook(recovery,nullptr); // S_OK with native lost1 must not synchronize.
    ok=field<unsigned>(recovery,0x50)==1920 && ok;
    recovery[0xaf]=0; // Synthetic native recovery completed, no patch flag write.
    recoveryHook(recovery,nullptr);
    RECT client{};GetClientRect(h,&client);
    ok=field<unsigned>(recovery,0x50)==static_cast<unsigned>(client.right) &&
        field<unsigned>(recovery,0x54)==static_cast<unsigned>(client.bottom) &&
        pendingWindowedSyncOwner==nullptr && pendingWindowedSyncWindow==nullptr && ok;
    // Disabling synchronization must not create a future synchronization request.
    syncWindowedMode=false;
    resizeHook(recovery,nullptr);
    ok=pendingWindowedSyncOwner==nullptr && pendingWindowedSyncWindow==nullptr && ok;
    if(!ok)std::fprintf(stderr,"Ordinary failed resize did not reconcile after ready native recovery\n");
    resizeOriginal=nullptr;recoveryOriginal=nullptr;
    pendingWindowedSyncOwner=nullptr;pendingWindowedSyncWindow=nullptr;
    DestroyWindow(h);
    diagnosticHoldResizeBackbuffer=false;resetFaultOwner=nullptr;resetFaultWindow=nullptr;
    return ok;
}
int main() {
    // Every condition previously failed only inside commitModelessSize, after
    // SetWindowLong/SetWindowPlacement had already changed the presentation.
    bool ok=true;
    ok=refusedWithoutMutation(0xaf,1) && ok;  // Device lost.
    ok=refusedWithoutMutation(0xb2,1) && ok;  // Native mode change in progress.
    ok=refusedWithoutMutation(0xb4,0) && ok;  // Resources not ready.
    ok=refusedWithoutMutation(0xac,0) && ok;  // Not windowed.
    ok=refusedWithoutMutation(0xd8,0) && ok;  // No windowed mode selection.
    ok=refusedWithoutMutation(0x100,0) && ok; // No device.
    ok=failureAfterPreflight(true) && ok;
    ok=failureAfterPreflight(false) && ok;
    ok=failedOrdinaryResizeRecovery() && ok;
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
