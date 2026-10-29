// Actual shared message wrapper: disabling the guard must preserve native
// delivery/return, while enabling it must defer only an owned empty restore.
// Uses owned hidden Win32 windows, no game or D3D device.
#include "probe.cpp"
#include <cstdlib>

static unsigned calls;
static void* deliveredOwner;
static HWND deliveredWindow;
static UINT deliveredMessage;
static WPARAM deliveredW;
static LPARAM deliveredL;
static LRESULT __fastcall nativeMessage(void* self,void*,HWND h,UINT m,WPARAM w,LPARAM l) {
    ++calls;deliveredOwner=self;deliveredWindow=h;
    deliveredMessage=m;deliveredW=w;deliveredL=l;
    // Model the observed premature flag mutation, so the guard test checks
    // preservation of state needed by the following valid native restore.
    *reinterpret_cast<unsigned char*>(static_cast<char*>(self)+0xae)=0;
    return 0x12345678;
}
static bool runCase(HWND window,HWND owner,bool enabled,bool windowed,UINT message,WPARAM w,bool skip) {
    alignas(void*) unsigned char recovery[0x120]{};
    recovery[0xac]=windowed?1:0;recovery[0xae]=1;
    *reinterpret_cast<HWND*>(recovery+0xf0)=owner;
    calls=0;guardEmptyRestore=enabled;
    messageOriginal=reinterpret_cast<MessageFn>(nativeMessage);
    const LPARAM l=0x13579;
    const LRESULT result=messageHook(recovery,nullptr,window,message,w,l);
    if(skip)return calls==0 && result==0 && recovery[0xae]==1 && nesting==0;
    return calls==1 && result==0x12345678 && recovery[0xae]==0 && nesting==0 &&
        deliveredOwner==recovery && deliveredWindow==window && deliveredMessage==message &&
        deliveredW==w && deliveredL==l;
}
int main() {
    diagnostics=false;
    HWND empty=CreateWindowExW(0,L"STATIC",L"Empty restore fixture",WS_POPUP,
        0,0,0,0,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    HWND valid=CreateWindowExW(0,L"STATIC",L"Valid restore fixture",WS_POPUP,
        0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!empty || !valid)return EXIT_FAILURE;
    const bool ok=runCase(empty,empty,false,true,WM_SIZE,SIZE_RESTORED,false) &&
        runCase(empty,empty,true,true,WM_SIZE,SIZE_RESTORED,true) &&
        runCase(empty,empty,true,true,WM_SIZE,SIZE_MAXIMIZED,true) &&
        runCase(valid,valid,true,true,WM_SIZE,SIZE_RESTORED,false) &&
        runCase(empty,valid,true,true,WM_SIZE,SIZE_RESTORED,false) &&
        runCase(empty,empty,true,false,WM_SIZE,SIZE_RESTORED,false) &&
        runCase(empty,empty,true,true,WM_SIZE,SIZE_MINIMIZED,false) &&
        runCase(empty,empty,true,true,WM_MOVE,0,false);
    DestroyWindow(empty);DestroyWindow(valid);
    if(!ok)std::fprintf(stderr,"Shared message wrapper violated guard/native-delivery contract\n");
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
