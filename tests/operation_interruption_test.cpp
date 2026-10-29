// Actual modeless handler/finalizer with owned hidden windows. Exercise native
// capture-transfer/destruction reentrancy without a game or a D3D device.
#include "probe.cpp"
#include <cstdlib>

static unsigned char* owner;
static unsigned resizeCalls,captureMessages,destroyMessages;
static bool resizeSawActive;
static HRESULT __fastcall resizeFixture(void* self,void*) {
    ++resizeCalls;
    resizeSawActive=movingWindow!=nullptr || keyboardSizing || sizingHit!=0;
    RECT client{};GetClientRect(field<HWND>(self,0xf0),&client);
    *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xb8)=client.right;
    *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0xbc)=client.bottom;
    return S_OK;
}
static LRESULT CALLBACK fixtureProc(HWND h,UINT m,WPARAM w,LPARAM l) {
    if(owner && field<HWND>(owner,0xf0)==h) {
        if(m==WM_CAPTURECHANGED)++captureMessages;
        if(m==WM_DESTROY)++destroyMessages;
        if(handleModelessMove(h,m,w,l,owner))return 0;
    }
    return DefWindowProcW(h,m,w,l);
}
static void prepare(HWND h,unsigned char* recovery,bool sizing,bool deferred) {
    ZeroMemory(recovery,0x340);owner=recovery;
    recovery[0x30]=recovery[0xac]=recovery[0xb4]=1;
    *reinterpret_cast<HWND*>(recovery+0xf0)=h;
    *reinterpret_cast<void**>(recovery+0x100)=reinterpret_cast<void*>(1);
    *reinterpret_cast<unsigned*>(recovery+0xd8)=1;
    *reinterpret_cast<unsigned*>(recovery+0xb8)=330;
    *reinterpret_cast<unsigned*>(recovery+0xbc)=240;
    SetCapture(h);
    movingWindow=h;keyboardSizing=sizing;keyboardMove=!sizing;sizingHit=sizing?HTRIGHT:0;
    deferSizingReset=deferred;resizeCalls=captureMessages=destroyMessages=0;
    resizeSawActive=false;
    pendingMaximizedMove=moveBeganMaximized=false;
}
static bool cleared() {
    return !movingWindow && !keyboardSizing && !keyboardMove && sizingHit==0 &&
        !pendingMaximizedMove && !moveBeganMaximized && !resizeSawActive;
}
static bool maximizedCancelFixture(HINSTANCE instance,const wchar_t* className) {
    // Owned fixture only. NOACTIVATE keeps the user's foreground window intact.
    HWND h=CreateWindowExW(WS_EX_NOACTIVATE,className,L"Maximized cancellation fixture",
        WS_OVERLAPPEDWINDOW,120,120,640,480,nullptr,nullptr,instance,nullptr);
    if(!h)return false;
    owner=nullptr;
    ShowWindow(h,SW_MAXIMIZE);
    WINDOWPLACEMENT original{};original.length=sizeof(original);
    bool ok=GetWindowPlacement(h,&original)!=FALSE && IsZoomed(h);
    alignas(void*) unsigned char recovery[0x340]{};
    // Cover Escape before the drag threshold and after normal restoration/move.
    for(unsigned restored=0;restored<2;++restored) {
        if(restored) {
            ShowWindow(h,SW_RESTORE);
            ok=(SetWindowPos(h,nullptr,320,240,640,480,
                SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE) && ok;
        }
        prepare(h,recovery,false,true);
        RECT client{};GetClientRect(h,&client);
        *reinterpret_cast<unsigned*>(recovery+0xb8)=client.right;
        *reinterpret_cast<unsigned*>(recovery+0xbc)=client.bottom;
        moveBeganMaximized=true;pendingMaximizedMove=restored==0;
        movePlacement=original;
        moveBounds={1,2,3,4}; // The ordinary move rectangle must not win on cancel.
        const bool handled=handleModelessMove(h,WM_KEYDOWN,VK_ESCAPE,1,recovery);
        WINDOWPLACEMENT after{};after.length=sizeof(after);
        ok=handled && cleared() && !GetCapture() && IsZoomed(h) &&
            GetWindowPlacement(h,&after) &&
            EqualRect(&after.rcNormalPosition,&original.rcNormalPosition) &&
            resizeCalls==(restored?1u:0u) && ok;
        ok=handleModelessMove(h,WM_KEYUP,VK_ESCAPE,0,recovery) && ok;
    }
    ShowWindow(h,SW_RESTORE);
    WINDOWPLACEMENT normal{};normal.length=sizeof(normal);
    ok=!IsZoomed(h) && GetWindowPlacement(h,&normal) &&
        EqualRect(&normal.rcNormalPosition,&original.rcNormalPosition) && ok;
    owner=nullptr;DestroyWindow(h);
    if(!ok)std::fprintf(stderr,"Maximized Escape failed: resize=%u active=%p\n",
        resizeCalls,static_cast<void*>(movingWindow));
    return ok;
}
int main() {
    diagnostics=syncWindowedMode=diagnosticHoldResizeBackbuffer=false;
    resizeOriginal=reinterpret_cast<EngineFn>(resizeFixture);
    WNDCLASSW wc{};wc.lpfnWndProc=fixtureProc;wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"Nwn2OperationInterruptionFixture";
    if(!RegisterClassW(&wc))return EXIT_FAILURE;
    HWND h=CreateWindowW(wc.lpszClassName,L"Owned operation",WS_POPUP,
        0,0,320,240,nullptr,nullptr,wc.hInstance,nullptr);
    HWND other=CreateWindowW(wc.lpszClassName,L"Capture recipient",WS_POPUP,
        0,0,320,240,nullptr,nullptr,wc.hInstance,nullptr);
    if(!h || !other)return EXIT_FAILURE;
    alignas(void*) unsigned char recovery[0x340]{};
    bool ok=true;
    // Real capture theft must finalize once and preserve the recipient's capture.
    prepare(h,recovery,true,true);
    ok=ok && GetCapture()==h;
    SetCapture(other);
    ok=ok && cleared() && resizeCalls==1 && captureMessages==1 && GetCapture()==other;
    ReleaseCapture();
    // A pending maximized caption operation must be abandoned on real capture
    // transfer without restoring/resizing or retaining stale placement ownership.
    prepare(h,recovery,false,true);
    pendingMaximizedMove=moveBeganMaximized=true;
    SetCapture(other);
    ok=ok && cleared() && resizeCalls==0 && GetCapture()==other;
    ReleaseCapture();
    // CANCELMODE releases our capture synchronously; reentrant CAPTURECHANGED
    // must not commit twice. Activation and native teardown remain forwarded.
    prepare(h,recovery,true,true);
    bool handled=handleModelessMove(h,WM_CANCELMODE,0,0,recovery);
    ok=ok && !handled && cleared() && resizeCalls==1 && captureMessages==1 && !GetCapture();
    prepare(h,recovery,true,true);
    handled=handleModelessMove(h,WM_ACTIVATE,WA_INACTIVE,0,recovery);
    ok=ok && !handled && cleared() && resizeCalls==1 && captureMessages==1;
    prepare(h,recovery,true,true);
    handled=handleModelessMove(h,WM_SYSCOMMAND,SC_CLOSE|3,0,recovery);
    ok=ok && !handled && cleared() && resizeCalls==1 && captureMessages==1 && !GetCapture();
    prepare(h,recovery,true,true);
    handled=handleModelessMove(h,WM_CLOSE,0,0,recovery);
    ok=ok && !handled && cleared() && resizeCalls==1 && !GetCapture();
    // Move and immediate-resize interruptions must not initiate deferred resize.
    prepare(h,recovery,false,true);
    SetCapture(other);
    ok=ok && cleared() && resizeCalls==0 && GetCapture()==other;
    ReleaseCapture();
    prepare(h,recovery,true,false);
    SetCapture(other);
    ok=ok && cleared() && resizeCalls==0;
    ReleaseCapture();
    // A different window cannot terminate the owned operation.
    prepare(h,recovery,true,true);
    handled=handleModelessMove(other,WM_CANCELMODE,0,0,recovery);
    ok=ok && !handled && movingWindow==h && resizeCalls==0 && GetCapture()==h;
    // Real DestroyWindow must clear ownership/capture and never call resize.
    DestroyWindow(h);
    ok=ok && cleared() && resizeCalls==0 && destroyMessages==1 && !GetCapture();
    owner=nullptr;DestroyWindow(other);
    ok=maximizedCancelFixture(wc.hInstance,wc.lpszClassName) && ok;
    if(!ok)std::fprintf(stderr,"Operation interruption contract failed: resize=%u capture=%u destroy=%u active=%p\n",
        resizeCalls,captureMessages,destroyMessages,static_cast<void*>(movingWindow));
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
