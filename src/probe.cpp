// Experimental telemetry. Install only before the owned child's main thread runs.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <intrin.h>
#include "resize_geometry.h"
#include "caption_geometry.h"
#include "ui_geometry.h"
#include "finishing_key.h"
#include "presentation_style.h"
#include "reset_fault.h"
#include "apply_geometry.h"
static FinishingKey finishingKey;

static HANDLE logFile = INVALID_HANDLE_VALUE;
static SRWLOCK logLock = SRWLOCK_INIT;
static LONG sequence;
static bool guardEmptyRestore;
static bool nativeSizing;
static bool modelessMove;
static bool modelessSizing;
static bool deferSizingReset;
static bool keyboardSizing;
static UINT sizingHit;
static HWND sizingLimitsWindow;
static LONG sizingLimitsStyle;
static MINMAXINFO sizingLimits{}, activeSizingLimits{};
static HWND movingWindow;
static bool keyboardMove;
static POINT moveOrigin;
static RECT moveBounds;
static bool pendingMaximizedMove,moveBeganMaximized;
static WINDOWPLACEMENT movePlacement{};
static RECT moveMaximizedBounds{};
static UINT minClientWidth=800, minClientHeight=600;
static bool syncWindowedMode;
static bool preservePlacement;
static bool fixGraphicsControls;
static bool traceGraphicsControls;
static bool fitWindowedApply;
static void refreshFullscreenCheckbox(void* recovery);
static bool borderlessHotkey;
static bool borderlessAltEnter;
static HWND borderlessWindow;
static LONG borderlessStyle;
static WINDOWPLACEMENT borderlessPlacement{};
static bool borderlessTransition;
static HRESULT leaveBorderless(void* recovery);
static HRESULT commitModelessSize(HWND h,void* recovery);
static void finishModelessMove(void* recovery,bool cancel,bool destroying=false);
static bool presentationResizeReady(HWND h,void* recovery,RECT& client);
static void* pendingWindowedSyncOwner;
static HWND pendingWindowedSyncWindow;
static bool tracePresentation;
static bool throttleLostDevice;
static bool traceActivation;
static bool traceInputMessages;
static bool traceInputAcquisition;
static void* inputAcquisitionContinue;
static void* inputAcquisitionExitContinue;
static bool traceSurfaceMessages;
static unsigned surfaceMessages;
static bool diagnostics=true;
// Explicit diagnostic intervention, never enabled by a proposed fix option.
static bool diagnosticHoldResizeBackbuffer;
static void* resetFaultOwner;
static HWND resetFaultWindow;
static WINDOWPLACEMENT savedPlacement{};
static HWND savedPlacementWindow;
static void* savedPlacementOwner;
static thread_local unsigned nesting;
// D042: observe modal-loop starvation without invoking engine work recursively.
static thread_local unsigned presentCalls, pumpDepth;
static thread_local uintptr_t pumpCaller;
using PumpFn = int(__cdecl*)();
static PumpFn pumpOriginal;
static bool boundMoveSizePump;
static thread_local ULONGLONG pumpStarted;
static thread_local unsigned pumpMessages;
static thread_local unsigned pumpYields;
static thread_local unsigned gameplayPumpDepth, gameplayPumpMessages, gameplayPumpYields;
static thread_local uintptr_t gameplayPumpCaller;
static thread_local ULONGLONG gameplayPumpStarted;
using GameplayPumpFn = unsigned(__thiscall*)(void*);
static GameplayPumpFn gameplayPumpOriginal;
static constexpr uintptr_t gameplayPumpBackedgeAddress=0x7135ae;
static void* gameplayPumpLoopContinue=reinterpret_cast<void*>(0x713576);
static void* gameplayPumpBodyContinue=reinterpret_cast<void*>(0x7135b4);
static void* pumpLoopContinue=reinterpret_cast<void*>(0x402b28);
static void* pumpExitContinue=reinterpret_cast<void*>(0x402b67);
// Only the verified main frame caller may yield. Loading/recursive pumps retain
// native draining. The current message has already been dispatched; the pending
// PeekMessage lookahead is never removed by this decision.
static bool pumpBudgetExceeded(unsigned messages,ULONGLONG elapsed) {
    return messages>=64 || elapsed>=8;
}
static bool shouldYieldPump(unsigned depth,uintptr_t caller,uintptr_t expectedCaller,
                            ULONGLONG started,unsigned& messages,unsigned& yields) {
    if(!boundMoveSizePump || depth!=1 || caller!=expectedCaller || !movingWindow)
        return false;
    const DWORD error=GetLastError();
    const bool owned=GetWindowThreadProcessId(movingWindow,nullptr)==GetCurrentThreadId();
    const bool yield=owned && pumpBudgetExceeded(++messages,GetTickCount64()-started);
    if(yield)++yields;
    SetLastError(error);
    return yield;
}
static bool __cdecl shouldYieldMoveSizePump() {
    return shouldYieldPump(pumpDepth,pumpCaller,0x407413,pumpStarted,pumpMessages,pumpYields);
}
static bool __cdecl shouldYieldGameplayPump() {
    return shouldYieldPump(gameplayPumpDepth,gameplayPumpCaller,0x6ff46c,
        gameplayPumpStarted,gameplayPumpMessages,gameplayPumpYields);
}
// Reproduce test/jne/pop explicitly: the overwritten short branch cannot be
// copied into an ordinary trampoline. Keep native flags and all register values
// on both branches, including the last MSG.wParam consumed by the native exit.
static __declspec(naked) void pumpBackedgeHook() {
    __asm {
        test eax, eax
        jz finished
        pushfd
        pushad
        call shouldYieldMoveSizePump
        test al, al
        jnz yielded
        popad
        popfd
        jmp dword ptr [pumpLoopContinue]
    yielded:
        popad
        popfd
    finished:
        pop edi
        jmp dword ptr [pumpExitContinue]
    }
}
// Gameplay has its own drain inside the input-update function. Yield to its
// normal post-pump input body, not its function return/quit cleanup. Start at
// the indirect PeekMessage call: the initial empty-queue branch also enters
// 7135b4, so the ECX load there MUST remain untouched and directly executable.
static __declspec(naked) void gameplayPumpBackedgeHook() {
    __asm {
        call edi
        test eax, eax
        jz inputBody
        pushfd
        pushad
        call shouldYieldGameplayPump
        test al, al
        jnz yielded
        popad
        popfd
        jmp dword ptr [gameplayPumpLoopContinue]
    yielded:
        popad
        popfd
    inputBody:
        jmp dword ptr [gameplayPumpBodyContinue]
    }
}
using MessageFn = LRESULT(__thiscall*)(void*, HWND, UINT, WPARAM, LPARAM);
using EngineFn = HRESULT(__thiscall*)(void*);
using ResolutionFn = void(__cdecl*)(const unsigned*);
static ResolutionFn resolutionOriginal;
using PresentFn = HRESULT(__thiscall*)(void*, HWND);
static PresentFn presentOriginal;
static EngineFn recoveryOriginal;
static MessageFn messageOriginal;
static EngineFn resizeOriginal, resetOriginal, modeOriginal;
static WNDPROC outerOriginal;
static unsigned captionTraceCount;

template<class T> static T field(void* object, size_t offset) {
    return *reinterpret_cast<T*>(static_cast<char*>(object) + offset);
}
static void record(const char* kind, void* object, UINT msg=0, WPARAM wp=0,
                   LPARAM lp=0, HRESULT result=0, uintptr_t caller=0) {
    if(!diagnostics)return;
    HWND window=field<HWND>(object,0xf0);
    RECT client{}, bounds{};
    BOOL gotClient=GetClientRect(window,&client);
    GetWindowRect(window,&bounds);
    WINDOWPLACEMENT placement{};
    placement.length=sizeof(placement);
    BOOL gotPlacement=GetWindowPlacement(window,&placement);
    RECT cachedBounds=field<RECT>(object,0x25c);
    LONG payload[10]{};
    if(lp && (msg==WM_SIZING || msg==WM_MOVING))
        memcpy(payload,reinterpret_cast<const void*>(lp),sizeof(RECT));
    else if(lp && msg==WM_GETMINMAXINFO)
        memcpy(payload,reinterpret_cast<const void*>(lp),sizeof(MINMAXINFO));
    else if(lp && (msg==WM_STYLECHANGING || msg==WM_STYLECHANGED))
        memcpy(payload,reinterpret_cast<const void*>(lp),sizeof(STYLESTRUCT));
    else if(lp && (msg==WM_WINDOWPOSCHANGING || msg==WM_WINDOWPOSCHANGED)) {
        const auto pos=reinterpret_cast<const WINDOWPOS*>(lp);
        payload[0]=pos->x; payload[1]=pos->y;
        payload[2]=pos->cx; payload[3]=pos->cy;
        payload[4]=static_cast<LONG>(pos->flags);
    }
    char line[2048];
    AcquireSRWLockExclusive(&logLock);
    int count=snprintf(line,sizeof(line),
      "{\"seq\":%ld,\"ms\":%llu,\"tid\":%lu,\"depth\":%u,\"kind\":\"%s\","
      "\"message\":%u,\"wparam\":%u,\"lparam\":%u,\"result\":%u,"
      "\"object\":%u,\"hwnd\":%u,\"client_ok\":%d,\"client\":[%ld,%ld],"
      "\"bounds\":[%ld,%ld,%ld,%ld],\"iconic\":%d,\"style\":%lu,"
      "\"width\":%u,\"height\":%u,\"windowed\":%u,\"lost\":%u,"
      "\"minimized\":%u,\"maximized\":%u,\"ready\":%u,"
      "\"cached_style\":%u,\"mode_changing\":%u,"
      "\"windowed_mode\":[%u,%u],"
      "\"placement_ok\":%d,\"show_cmd\":%u,\"placement_flags\":%u,"
      "\"normal_bounds\":[%ld,%ld,%ld,%ld],"
      "\"cached_bounds\":[%ld,%ld,%ld,%ld],"
      "\"caller\":%u,\"window_proc\":%u,"
      "\"message_payload\":[%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld]}\n",
      ++sequence,GetTickCount64(),GetCurrentThreadId(),nesting,kind,msg,
      static_cast<unsigned>(wp),static_cast<unsigned>(lp),static_cast<unsigned>(result),
      reinterpret_cast<unsigned>(object),reinterpret_cast<unsigned>(window),gotClient,
      client.right-client.left,client.bottom-client.top,bounds.left,bounds.top,bounds.right,bounds.bottom,
      IsIconic(window),static_cast<unsigned long>(GetWindowLongW(window,GWL_STYLE)),
      field<unsigned>(object,0xb8),field<unsigned>(object,0xbc),field<unsigned>(object,0xd8),
      field<unsigned char>(object,0xaf),field<unsigned char>(object,0xb0),
      field<unsigned char>(object,0xb1),field<unsigned char>(object,0xb4),
      field<unsigned>(object,0x258),field<unsigned char>(object,0xb2),
      field<unsigned>(object,0x50),field<unsigned>(object,0x54),
      gotPlacement,placement.showCmd,placement.flags,
      placement.rcNormalPosition.left,placement.rcNormalPosition.top,
      placement.rcNormalPosition.right,placement.rcNormalPosition.bottom,
      cachedBounds.left,cachedBounds.top,cachedBounds.right,cachedBounds.bottom,
      static_cast<unsigned>(caller),static_cast<unsigned>(GetWindowLongW(window,GWL_WNDPROC)),
      payload[0],payload[1],payload[2],payload[3],payload[4],
      payload[5],payload[6],payload[7],payload[8],payload[9]);
    DWORD written;
    if(count>0 && count<static_cast<int>(sizeof(line))) WriteFile(logFile,line,count,&written,nullptr);
    ReleaseSRWLockExclusive(&logLock);
}
static void activationRecord(UINT message, WPARAM wp, LPARAM lp) {
    DWORD savedError=GetLastError();
    void* frames[16]{};
    USHORT count=CaptureStackBackTrace(0,16,frames,nullptr);
    HWND foreground=GetForegroundWindow();
    DWORD process=0;
    DWORD thread=GetWindowThreadProcessId(foreground,&process);
    char line[1024];
    AcquireSRWLockExclusive(&logLock);
    int length=snprintf(line,sizeof(line),
        "{\"seq\":%ld,\"ms\":%llu,\"tid\":%lu,\"kind\":\"activation_stack\","
        "\"message\":%u,\"wparam\":%u,\"lparam\":%u,\"foreground\":%u,"
        "\"foreground_pid\":%lu,\"foreground_tid\":%lu,"
        "\"focus\":%u,\"active\":%u,\"capture\":%u,\"frames\":[",
        ++sequence,GetTickCount64(),GetCurrentThreadId(),message,
        static_cast<unsigned>(wp),static_cast<unsigned>(lp),
        reinterpret_cast<unsigned>(foreground),process,thread,
        reinterpret_cast<unsigned>(GetFocus()),
        reinterpret_cast<unsigned>(GetActiveWindow()),
        reinterpret_cast<unsigned>(GetCapture()));
    for(USHORT i=0;i<count && length>0 && length<900;++i)
        length+=snprintf(line+length,sizeof(line)-length,"%s%u",
            i ? "," : "",reinterpret_cast<unsigned>(frames[i]));
    if(length>0 && length<900) {
        length+=snprintf(line+length,sizeof(line)-length,"]}\n");
        DWORD written;
        WriteFile(logFile,line,static_cast<DWORD>(length),&written,nullptr);
    }
    ReleaseSRWLockExclusive(&logLock);
    SetLastError(savedError);
}
// D120: register snapshots from pushfd/pushad, before either original span.
// Entry has the native caller/argument at stack+0/+4. At the common epilogue
// four saved registers precede them. EBP/EBX are normalized primary results;
// on an equal-state early return they are initialized zero, not COM outcomes.
static void inputAcquisitionRecord(unsigned exiting,const unsigned* registers) {
    DWORD savedError=GetLastError();
    void* owner=reinterpret_cast<void*>(registers[exiting?1:6]);
    unsigned request=registers[exiting?14:10];
    unsigned state=field<unsigned>(owner,4);
    HWND foreground=GetForegroundWindow();
    DWORD process=0;
    GetWindowThreadProcessId(foreground,&process);
    char line[768];
    AcquireSRWLockExclusive(&logLock);
    int count=snprintf(line,sizeof(line),
        "{\"seq\":%ld,\"ms\":%llu,\"tid\":%lu,\"kind\":\"input_acquisition_%s\","
        "\"owner\":%u,\"request\":%u,\"state\":%u,\"keyboard_inhibited\":%u,"
        "\"keyboard\":%u,\"mouse\":%u,\"keyboard_result_normalized\":%u,"
        "\"mouse_result_normalized\":%u,\"caller\":%u,\"foreground\":%u,"
        "\"foreground_pid\":%lu}\n",
        ++sequence,GetTickCount64(),GetCurrentThreadId(),exiting?"leave":"enter",
        reinterpret_cast<unsigned>(owner),request,state,field<unsigned>(owner,8),
        field<unsigned>(owner,0x14),field<unsigned>(owner,0x18),
        exiting?registers[2]:0xffffffffu,exiting?registers[4]:0xffffffffu,
        registers[exiting?13:9],reinterpret_cast<unsigned>(foreground),process);
    DWORD written;
    if(count>0 && count<static_cast<int>(sizeof(line)))
        WriteFile(logFile,line,static_cast<DWORD>(count),&written,nullptr);
    ReleaseSRWLockExclusive(&logLock);
    SetLastError(savedError);
}
static __declspec(naked) void inputAcquisitionHook() {
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        push 0
        call inputAcquisitionRecord
        add esp, 8
        popad
        popfd
        jmp dword ptr [inputAcquisitionContinue]
    }
}
static __declspec(naked) void inputAcquisitionExitHook() {
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        push 1
        call inputAcquisitionRecord
        add esp, 8
        popad
        popfd
        jmp dword ptr [inputAcquisitionExitContinue]
    }
}
static bool interesting(UINT m) {
    return m==WM_SIZE || m==WM_MOVE || m==WM_WINDOWPOSCHANGED ||
      m==WM_WINDOWPOSCHANGING || m==WM_ENTERSIZEMOVE || m==WM_EXITSIZEMOVE ||
      m==WM_SIZING || m==WM_MOVING || m==WM_ACTIVATE || m==WM_ACTIVATEAPP ||
      m==WM_SYSCOMMAND || m==WM_DISPLAYCHANGE || m==WM_GETMINMAXINFO ||
      m==WM_STYLECHANGING || m==WM_STYLECHANGED;
}
static LRESULT __fastcall messageHook(void* self, void*, HWND h, UINT m, WPARAM w, LPARAM l) {
    bool trace=interesting(m);
    if(trace)record("message_enter",self,m,w,l,0,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    // D012: an early SIZE_RESTORED can still describe the minimized rectangle.
    // Keep the engine's prior minimized/maximized flags until the valid WM_SIZE;
    // that message then takes the original complete resize/resource lifecycle.
    // No dimensions or engine flags are synthesized by this experiment.
    if(guardEmptyRestore && m==WM_SIZE && (w==SIZE_RESTORED || w==SIZE_MAXIMIZED)
       && field<unsigned char>(self,0xac) && h==field<HWND>(self,0xf0)) {
        RECT client{};
        if(GetClientRect(h,&client) &&
           (IsIconic(h) || client.right<=client.left || client.bottom<=client.top)) {
            record("skip_empty_restore",self,m,w,l);
            return 0;
        }
    }
    ++nesting;
    LRESULT r=messageOriginal(self,h,m,w,l);
    --nesting;
    if(trace)record("message_leave",self,m,w,l,static_cast<HRESULT>(r));
    return r;
}
static bool synchronizeWindowedDimensions(void* self) {
    // D020: native resize updates the backbuffer and cached rectangles, but
    // leaves the windowed mode record stale. 4419C0 consumes that record on
    // the next mode switch. 442760's resolution-Apply path owns these same
    // width/height fields; synchronize only after successful resize or pending
    // failed-resize recovery, with initialized/ready/client dimensions verified.
    if(syncWindowedMode && field<unsigned char>(self,0x30)) {
        HWND h=field<HWND>(self,0xf0);
        RECT client{};
        if(presentationResizeReady(h,self,client) &&
           field<unsigned>(self,0xb8)==static_cast<unsigned>(client.right-client.left) &&
           field<unsigned>(self,0xbc)==static_cast<unsigned>(client.bottom-client.top)) {
            *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0x50)=field<unsigned>(self,0xb8);
            *reinterpret_cast<unsigned*>(static_cast<char*>(self)+0x54)=field<unsigned>(self,0xbc);
            record("sync_windowed_mode",self);
            if(pendingWindowedSyncOwner==self) {
                pendingWindowedSyncOwner=nullptr;
                pendingWindowedSyncWindow=nullptr;
            }
            return true;
        }
    }
    return false;
}
static void acquireResizeFault(void* self,HeldResetBackbuffer& held) {
    if(!diagnostics || !diagnosticHoldResizeBackbuffer || resetFaultOwner!=self ||
       resetFaultWindow!=field<HWND>(self,0xf0) ||
       !field<unsigned char>(self,0x30))return;
    RECT client{};
    if(!presentationResizeReady(resetFaultWindow,self,client) ||
       (field<unsigned>(self,0xb8)==static_cast<unsigned>(client.right) &&
        field<unsigned>(self,0xbc)==static_cast<unsigned>(client.bottom)))return;
    // Consume exactly one eligible attempt before any COM call/reentrant message.
    resetFaultOwner=nullptr;resetFaultWindow=nullptr;
    HRESULT result=held.acquire(field<IDirect3DDevice9*>(self,0x100));
    record("fault_resize_backbuffer_acquired",self,0,held.active()?1:0,0,result);
}
static HRESULT __fastcall resizeHook(void* self, void*) {
    HeldResetBackbuffer held;
    acquireResizeFault(self,held);
    record("resize_enter",self); ++nesting;
    HRESULT r=resizeOriginal(self); --nesting;
    if(held.active()) {
        held.release(); // Native recovery may run on the very next frame.
        record("fault_resize_backbuffer_released",self,0,0,0,r);
    }
    if(SUCCEEDED(r))synchronizeWindowedDimensions(self);
    else if(syncWindowedMode && field<unsigned char>(self,0x30) &&
            field<unsigned char>(self,0xac)) {
        // D131: ordinary sizing has the same native dimension-before-reset
        // failure path as borderless exit. Reconcile only after the original
        // recovery lifecycle passes the shared readiness/dimension checks.
        pendingWindowedSyncOwner=self;
        pendingWindowedSyncWindow=field<HWND>(self,0xf0);
    }
    record("resize_leave",self,0,0,0,r); return r;
}
static HRESULT __fastcall resetHook(void* self, void*) {
    record("engine_reset_enter",self); ++nesting;
    HRESULT r=resetOriginal(self); --nesting;
    record("engine_reset_leave",self,0,0,0,r); return r;
}
struct SampleState {
    ULONGLONG lastTime=0;
    HRESULT lastResult=0;
    unsigned calls=0;
    bool lastWindowed=false;
    bool lastLost=false;
};
static void sample(const char* kind, void* recovery, HRESULT result, SampleState& state) {
    ++state.calls;
    ULONGLONG now=GetTickCount64();
    bool windowed=field<unsigned char>(recovery,0xac)!=0;
    bool lost=field<unsigned char>(recovery,0xaf)!=0;
    if(state.calls==1 || now-state.lastTime>=1000 || result!=state.lastResult ||
       windowed!=state.lastWindowed || lost!=state.lastLost) {
        // wparam is this thread's cumulative call count, not a window message.
        record(kind,recovery,0,state.calls,0,result);
        state.lastTime=now; state.lastResult=result;
        state.lastWindowed=windowed; state.lastLost=lost;
    }
}
static HRESULT __fastcall presentHook(void* app, void*, HWND overrideWindow) {
    HRESULT result=presentOriginal(app,overrideWindow);
    if(tracePresentation)++presentCalls;
    void* recovery=static_cast<char*>(app)+0x20;
    if(tracePresentation) {
        static thread_local SampleState state;
        sample("present_sample",recovery,result,state);
    }
    HWND window=field<HWND>(recovery,0xf0);
    if(throttleLostDevice && result==static_cast<HRESULT>(0x88760868u) &&
       field<unsigned char>(recovery,0xaf) && IsIconic(window) &&
       GetWindowThreadProcessId(window,nullptr)==GetCurrentThreadId()) {
        // D026: the original loop repeatedly presents a lost, minimized device.
        // Wake for queued input; the original message pump/recovery still owns it.
        // Never wait indefinitely, dispatch messages here, or synthesize success.
        DWORD wait=MsgWaitForMultipleObjectsEx(0,nullptr,16,QS_ALLINPUT,MWMO_INPUTAVAILABLE);
        if(tracePresentation) {
            static thread_local SampleState state;
            sample("lost_wait_sample",recovery,static_cast<HRESULT>(wait),state);
        }
    }
    return result;
}
static HRESULT __fastcall recoveryHook(void* self, void*) {
    HRESULT result=recoveryOriginal(self);
    // D124/D131: reconcile only a failed windowed-resize request, after native
    // recovery and full ready/client/backbuffer checks. No Reset or state
    // forcing here; S_OK alone is insufficient while the lost flag is set.
    if(pendingWindowedSyncOwner==self &&
       pendingWindowedSyncWindow==field<HWND>(self,0xf0) && SUCCEEDED(result))
        synchronizeWindowedDimensions(self);
    // This is the engine recovery result, NOT raw TestCooperativeLevel:
    // the engine maps DEVICELOST to S_OK while retaining its lost flag.
    if(tracePresentation) {
        static thread_local SampleState state;
        sample("recovery_sample",self,result,state);
    }
    return result;
}
static int __cdecl pumpHook() {
    const uintptr_t previous=pumpCaller;
    pumpCaller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    ++pumpDepth;
    const ULONGLONG previousStarted=pumpStarted;
    const unsigned previousMessages=pumpMessages;
    if(boundMoveSizePump) {
        const DWORD error=GetLastError();
        pumpStarted=GetTickCount64(); pumpMessages=0;
        SetLastError(error);
    }
    int result=pumpOriginal();
    pumpStarted=previousStarted; pumpMessages=previousMessages;
    --pumpDepth;
    pumpCaller=previous;
    return result;
}
static unsigned __fastcall gameplayPumpHook(void* self,void*) {
    const uintptr_t previous=gameplayPumpCaller;
    const ULONGLONG previousStarted=gameplayPumpStarted;
    const unsigned previousMessages=gameplayPumpMessages;
    gameplayPumpCaller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    ++gameplayPumpDepth;
    const DWORD error=GetLastError();
    gameplayPumpStarted=GetTickCount64(); gameplayPumpMessages=0;
    SetLastError(error);
    const unsigned result=gameplayPumpOriginal(self);
    --gameplayPumpDepth;
    gameplayPumpCaller=previous;
    gameplayPumpStarted=previousStarted; gameplayPumpMessages=previousMessages;
    return result;
}
using GraphicsListFn=void(__cdecl*)(void*,unsigned,unsigned);
using GraphicsPopulateFn=void(__cdecl*)(void*);
static GraphicsListFn graphicsListOriginal;
static GraphicsPopulateFn graphicsPopulateOriginal;
struct GraphicsGlobals {
    unsigned char** app;
    unsigned* selected;
    const unsigned** modes;
    const int* modeCount;
    unsigned char** scenes;
};
static GraphicsGlobals graphicsGlobals={
    reinterpret_cast<unsigned char**>(0xa04104),reinterpret_cast<unsigned*>(0x9c58f8),
    reinterpret_cast<const unsigned**>(0x9c5a04),reinterpret_cast<const int*>(0x9c5a0c),
    reinterpret_cast<unsigned char**>(0xa0e19c)};
static void* graphicsPane;
static HWND graphicsPaneWindow;
static DWORD graphicsPaneThread;
struct UiNameRef { const char* text; unsigned length; };
static_assert(sizeof(UiNameRef)==8,"Native string reference ABI");
using UiFindFn=void*(__thiscall*)(void*,const UiNameRef*);
using UiCheckFn=void(__thiscall*)(void*,bool);
static UiFindFn uiFind=reinterpret_cast<UiFindFn>(0x4d5880);
static UiCheckFn uiCheck=reinterpret_cast<UiCheckFn>(0x4f1b30);

static void preferGraphicsSelection(unsigned& width,unsigned& height,uintptr_t caller) {
    if(!fixGraphicsControls || caller!=0x80c0b2)return;
    // D159: native80b1f0 selects by exact actual client dimensions, then falls
    // back to row0 when a drag/clamped Apply has no enumerated match. Pass the
    // separately selected mode through its existing native population path.
    const auto selected=graphicsGlobals.selected;
    const auto modes=*graphicsGlobals.modes;
    const int count=*graphicsGlobals.modeCount;
    if(!modes || count<=0 || count>4096 || !selected[0] || !selected[1])return;
    for(int i=0;i<count;++i)if(modes[4*i]==selected[0] && modes[4*i+1]==selected[1]) {
        width=selected[0];height=selected[1];return;
    }
}
static void __cdecl graphicsListHook(void* list,unsigned width,unsigned height) {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    auto app=*graphicsGlobals.app;
    void* self=app?app+0x20:nullptr;
    if(traceGraphicsControls && self)record("graphics_list_input",self,0,width,height);
    preferGraphicsSelection(width,height,caller);
    if(traceGraphicsControls && self)record("graphics_list_preferred",self,0,width,height);
    graphicsListOriginal(list,width,height);
    if(traceGraphicsControls && self && list)
        record("graphics_list_selected",self,0,field<unsigned>(list,0x234),field<unsigned>(list,0x204));
}
static void refreshFullscreenCheckbox(void* self) {
    if(!fixGraphicsControls || !self || !graphicsPane ||
       graphicsPaneThread!=GetCurrentThreadId() ||
       graphicsPaneWindow!=field<HWND>(self,0xf0) ||
       GetWindowThreadProcessId(graphicsPaneWindow,nullptr)!=GetCurrentThreadId() ||
       !field<unsigned char>(self,0xb4) || field<unsigned char>(self,0xaf) ||
       field<unsigned char>(self,0xb2) || IsIconic(graphicsPaneWindow))return;
    // The cached pane is weak. Never dereference it unless the current scene
    // manager still owns that exact root on this same UI thread. Lookup the
    // checkbox afresh, and use its native setter; other unsaved controls survive.
    auto manager=*graphicsGlobals.scenes;
    if(!manager)return;
    const auto scenes=field<unsigned char**>(manager,0x78);
    const unsigned count=field<unsigned>(manager,0x80);
    if(!scenes || count>1024)return;
    bool owned=false;
    for(unsigned i=0;i<count;++i)if(scenes[i] && field<void*>(scenes[i],4)==graphicsPane) {
        owned=true;break;
    }
    if(!owned)return; // Initial population can precede scene-array insertion.
    if(field<uintptr_t>(graphicsPane,0)!=0x960ea4 || field<void*>(graphicsPane,4))return;
    // 4d5880/4287a0 only read this string reference; they do not retain/free it.
    const UiNameRef name={"CHECK_WINDOWED",14};
    void* checkbox=uiFind(graphicsPane,&name);
    // D160 live CHECK_WINDOWED and fresh native4f1cf0 constructor identify
    // this UIButton table. The earlier 961c04 assumption refused the real UI.
    if(!checkbox || field<uintptr_t>(checkbox,0)!=0x962554 ||
       (field<unsigned>(checkbox,0x264)&0x1c00)!=0x800)return;
    const bool fullscreen=field<unsigned char>(self,0xac)==0;
    uiCheck(checkbox,fullscreen);
    record("graphics_fullscreen_synced",self,0,fullscreen);
}
static void __cdecl graphicsPopulateHook(void* pane) {
    graphicsPopulateOriginal(pane);
    auto app=*graphicsGlobals.app;
    if(!app)return;
    auto self=app+0x20;
    graphicsPane=pane;graphicsPaneWindow=field<HWND>(self,0xf0);
    graphicsPaneThread=GetCurrentThreadId();
    refreshFullscreenCheckbox(self);
}

struct ApplyFitContext { void* owner;HWND window;RECT work;bool active; };
static thread_local ApplyFitContext applyFitContext{};
static void* applyMoveContinue=reinterpret_cast<void*>(0x4427df);
static BOOL __stdcall applyMoveWindowHook(HWND h,int x,int y,int width,int height,BOOL repaint) {
    const DWORD before=GetLastError();
    const auto context=applyFitContext;
    RECT fitted{};
    if(fitWindowedApply && context.active && h==context.window && context.owner &&
       GetWindowThreadProcessId(h,nullptr)==GetCurrentThreadId() &&
       field<HWND>(context.owner,0xf0)==h && field<unsigned char>(context.owner,0xac) &&
       !IsIconic(h) && !IsZoomed(h) &&
       fitApplyBounds(x,y,width,height,context.work,fitted)) {
        x=fitted.left;y=fitted.top;width=fitted.right-fitted.left;height=fitted.bottom-fitted.top;
        record("fit_apply_bounds",context.owner,0,width,height);
    }
    SetLastError(before);
    // This replaces only native442760's MoveWindow call, before its complete
    // resize/Reset. No second resize, mode flag writes or recovery queue needed.
    const BOOL result=MoveWindow(h,x,y,width,height,repaint);
    const DWORD error=GetLastError();
    if(context.active)record("fit_apply_move",context.owner,0,0,0,
        result?S_OK:HRESULT_FROM_WIN32(error));
    SetLastError(error);
    return result;
}
static __declspec(naked) void applyMoveWindowThunk() {
    __asm {
        // Native six stdcall arguments already exist; the displaced CALL had
        // not pushed its return address. Our CALL supplies it, callee cleans24.
        call applyMoveWindowHook
        jmp dword ptr [applyMoveContinue]
    }
}
static HRESULT __fastcall modeHook(void* self, void*) {
    // Native exclusive mode retains its original lifecycle and input policy.
    if(borderlessWindow) {
        HRESULT restored=leaveBorderless(self);
        if(FAILED(restored))return restored;
    }
    HWND h=field<HWND>(self,0xf0);
    // D171: native AltEnter is delivered during keyboard Size. Finish the
    // owned operation while still windowed, before native style/reset/input
    // changes. Otherwise the first fullscreen click ends stale sizing and
    // attempts a windowed commit in exclusive mode (E_FAIL).
    if(movingWindow && movingWindow==h &&
       GetWindowThreadProcessId(h,nullptr)==GetCurrentThreadId())
        finishModelessMove(self,false);
    record("mode_enter",self);
    const bool wasWindowed=field<unsigned char>(self,0xac)!=0;
    WINDOWPLACEMENT before{};
    before.length=sizeof(before);
    bool captured=false;
    if(preservePlacement && wasWindowed && !field<unsigned char>(self,0xb2)) {
        // D023: removing WS_MAXIMIZE and the subsequent fullscreen Reset
        // overwrite Windows' normal placement. Preserve it before either runs.
        savedPlacementOwner=nullptr;
        captured=!IsIconic(h) && GetWindowPlacement(h,&before) &&
            (before.showCmd==SW_SHOWNORMAL || before.showCmd==SW_SHOWMAXIMIZED) &&
            before.rcNormalPosition.right>before.rcNormalPosition.left &&
            before.rcNormalPosition.bottom>before.rcNormalPosition.top;
    }
    ++nesting;
    HRESULT r=modeOriginal(self);
    --nesting;
    const bool isWindowed=field<unsigned char>(self,0xac)!=0;
    if(preservePlacement && SUCCEEDED(r) && h==field<HWND>(self,0xf0) &&
       !field<unsigned char>(self,0xb2)) {
        if(wasWindowed && !isWindowed && captured) {
            savedPlacement=before;
            savedPlacementWindow=h;
            savedPlacementOwner=self;
            record("save_window_placement",self);
        } else if(!wasWindowed && isWindowed && savedPlacementOwner==self &&
                  savedPlacementWindow==h && field<unsigned char>(self,0xb4) &&
                  !field<unsigned char>(self,0xaf)) {
            // Use Windows' complete placement API and its original coordinates.
            BOOL restored=SetWindowPlacement(h,&savedPlacement);
            DWORD error=restored?ERROR_SUCCESS:GetLastError();
            record("restore_window_placement",self,0,0,0,
                   restored?S_OK:HRESULT_FROM_WIN32(error));
            if(restored) {
                savedPlacementOwner=nullptr;
                // D091: Windows can clamp oversized normal placement. Ordinary
                // SIZE_RESTORED then skips resize if neither min/max flag is set,
                // leaving the backbuffer at the old fullscreen dimensions.
                // Reuse the checked complete resize lifecycle after placement
                // settles; it is a no-op when client/backbuffer already agree.
                HRESULT sized=commitModelessSize(h,self);
                record("restore_window_size",self,0,0,0,sized);
                if(FAILED(sized))r=sized;
            }
        }
    }
    if(SUCCEEDED(r) && wasWindowed!=isWindowed)refreshFullscreenCheckbox(self);
    record("mode_leave",self,0,0,0,r);
    return r;
}
static void __cdecl resolutionHook(const unsigned* requested) {
    // D104: wrap the settings boundary before it writes resolution globals.
    // Hooking 442760 was too late to keep them unchanged if borderless exit
    // failed. 435dc0 takes/copies four words, compares only width/height/refresh,
    // and calls the native resize only when those selected values change.
    unsigned mode[4];
    memcpy(mode,requested,sizeof(mode));
    auto app=*graphicsGlobals.app;
    void* self=app?app+0x20:nullptr;
    const auto selected=graphicsGlobals.selected;
    const bool changed=mode[0]!=selected[0] || mode[1]!=selected[1] ||
                       mode[2]!=selected[2];
    if(borderlessWindow && changed) {
        HRESULT restored=leaveBorderless(self);
        if(FAILED(restored)) {
            record("resolution_borderless_failed",self,0,mode[0],mode[1],restored);
            return;
        }
    }
    if(self)record("resolution_enter",self,0,mode[0],mode[1]);
    const ApplyFitContext previousFit=applyFitContext;
    applyFitContext={};
    if(fitWindowedApply && self && changed && !borderlessWindow) {
        HWND h=field<HWND>(self,0xf0);RECT client{};
        MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
        if(presentationResizeReady(h,self,client) && field<unsigned char>(self,0x30) &&
           !IsZoomed(h) && !movingWindow && (GetWindowLongW(h,GWL_STYLE)&WS_CAPTION) &&
           GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&monitor))
            applyFitContext={self,h,monitor.rcWork,true};
    }
    resolutionOriginal(mode);
    applyFitContext=previousFit;
    // D156: 442760 writes requested windowed50/54 AFTER resizeOriginal returns.
    // Windows may have clamped the real client (e.g. desktop-sized captioned
    // windows), overwriting resizeHook's correct synchronization with a stale
    // requested height. Reconcile at the completed settings boundary, using
    // the same ready/owned-window/client==backbuffer gate. Never modify the
    // separate fullscreen selection, or unchanged borderless ownership.
    if(self && changed && !borderlessWindow)synchronizeWindowedDimensions(self);
    // The native boundary has no HRESULT contract; report resulting geometry,
    // not a fabricated success return from its incidental EAX value.
    if(self)record("resolution_leave",self,0,mode[0],mode[1]);
}
// D045 prototype: return to the ordinary frame loop between caption mouse
// messages. No timer, recursive engine tick, synthetic modal loop, or D3D call.
static bool presentationResizeReady(HWND h,void* recovery,RECT& client) {
    return recovery && h==field<HWND>(recovery,0xf0) &&
       GetWindowThreadProcessId(h,nullptr)==GetCurrentThreadId() &&
       field<unsigned char>(recovery,0xac) && field<unsigned>(recovery,0xd8) &&
       !field<unsigned char>(recovery,0xb2) && !field<unsigned char>(recovery,0xaf) &&
       field<unsigned char>(recovery,0xb4) && field<void*>(recovery,0x100) &&
       !IsIconic(h) && GetClientRect(h,&client) && client.right>0 && client.bottom>0;
}
static HRESULT commitModelessSize(HWND h,void* recovery) {
    static thread_local bool committing=false;
    RECT client{};
    if(committing || !presentationResizeReady(h,recovery,client))return E_FAIL;
    if(field<unsigned>(recovery,0xb8)==static_cast<unsigned>(client.right) &&
       field<unsigned>(recovery,0xbc)==static_cast<unsigned>(client.bottom))return S_OK;
    // D054: ordinary SIZE_RESTORED does not commit unless min/max flags were
    // set. Native EXITSIZEMOVE calls 442460 directly without changing flags.
    // Use that complete lifecycle after SetWindowPos, not a synthetic message
    // or raw D3D Reset. resizeHook also performs the proved mode-record sync.
    committing=true;
    HRESULT result=resizeHook(recovery,nullptr);
    committing=false;
    record("modeless_size_commit",recovery,0,0,0,result);
    return result;
}
static void finishModelessMove(void* recovery, bool cancel, bool destroying) {
    HWND h=movingWindow;
    const bool wasSizing=sizingHit!=0 || keyboardSizing;
    const bool wasMaximized=moveBeganMaximized;
    const WINDOWPLACEMENT originalPlacement=movePlacement;
    pendingMaximizedMove=moveBeganMaximized=false;
    keyboardMove=false;
    keyboardSizing=false;
    sizingHit=0;
    movingWindow=nullptr; // ReleaseCapture synchronously sends CAPTURECHANGED.
    if(!h)return;
    if(GetCapture()==h)ReleaseCapture();
    HRESULT result=S_OK;
    if(cancel && wasMaximized && !destroying && IsWindow(h)) {
        // Clear ownership before the synchronous placement/WM_SIZE lifecycle.
        // Restore both show state and the original normal rectangle on Escape.
        if(!SetWindowPlacement(h,&originalPlacement))result=HRESULT_FROM_WIN32(GetLastError());
        else result=commitModelessSize(h,recovery);
    }
    else if(cancel && IsWindow(h)) {
        BOOL restored=SetWindowPos(h,nullptr,moveBounds.left,moveBounds.top,
                     moveBounds.right-moveBounds.left,moveBounds.bottom-moveBounds.top,
                     (wasSizing?0:SWP_NOSIZE)|SWP_NOZORDER|SWP_NOACTIVATE);
        if(!restored)result=HRESULT_FROM_WIN32(GetLastError());
        else if(wasSizing)result=commitModelessSize(h,recovery);
    }
    else if(wasSizing && deferSizingReset && !destroying && IsWindow(h)) {
        // D063: keep the original backbuffer during the operation, then use
        // the complete engine lifecycle once at the final client dimensions.
        // Never reset from WM_DESTROY; leave destruction to the engine.
        result=commitModelessSize(h,recovery);
    }
    if(recovery)record(wasSizing?(cancel?"modeless_size_cancel":"modeless_size_end"):
                       (cancel?"modeless_move_cancel":"modeless_move_end"),
                       recovery,0,presentCalls,0,result);
    if(recovery && boundMoveSizePump && tracePresentation)
        record("pump_bound_operation_end",recovery,0,pumpYields,
            static_cast<LPARAM>(gameplayPumpYields),0,
            gameplayPumpDepth?gameplayPumpCaller:pumpCaller);
}
static bool setPresentationStyle(HWND h,LONG style) {
    SetLastError(ERROR_SUCCESS);
    return SetWindowLongW(h,GWL_STYLE,style)!=0 || GetLastError()==ERROR_SUCCESS;
}
static HRESULT leaveBorderless(void* recovery) {
    HWND h=borderlessWindow;
    if(!h)return S_OK;
    RECT client{};
    // D118: refuse pre-existing loss/unready/minimized state before changing
    // style or placement. A failed commit after those mutations is too late.
    // Keep ownership so a later valid exit can still restore saved placement.
    if(borderlessTransition || movingWindow ||
       !presentationResizeReady(h,recovery,client))return E_FAIL;
    WINDOWPLACEMENT before{};before.length=sizeof(before);
    RECT bounds{};
    if(!GetWindowPlacement(h,&before) || !GetWindowRect(h,&bounds))return E_FAIL;
    const LONG styleBefore=GetWindowLongW(h,GWL_STYLE);
    const bool wasVisible=IsWindowVisible(h)!=FALSE;
    const unsigned widthBefore=field<unsigned>(recovery,0xb8);
    const unsigned heightBefore=field<unsigned>(recovery,0xbc);
    borderlessTransition=true;
    bool ok=setPresentationStyle(h,borderlessStyle) &&
        SetWindowPlacement(h,&borderlessPlacement) &&
        SetWindowPos(h,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|
                     SWP_NOACTIVATE|SWP_FRAMECHANGED);
    HRESULT result=E_FAIL;
    if(ok) {
        // D122: window presentation has exited even if the subsequent engine
        // reset fails. Native resize writes dimensions before that failure;
        // retain its state/lifecycle and report the error, not stale ownership
        // or a geometric rollback that disagrees with the recovery dimensions.
        result=commitModelessSize(h,recovery);
        borderlessWindow=nullptr;
        if(FAILED(result) && syncWindowedMode) {
            pendingWindowedSyncOwner=recovery;
            pendingWindowedSyncWindow=h;
        }
    } else {
        // Restore an API refusal only while the engine is still ready and its
        // dimensions are unchanged. Synchronous window messages can perform
        // engine work: never roll the window back across observed device loss.
        bool restored=false;
        if(presentationResizeReady(h,recovery,client) &&
           field<unsigned>(recovery,0xb8)==widthBefore &&
           field<unsigned>(recovery,0xbc)==heightBefore) {
            if(!wasVisible)before.showCmd=SW_HIDE;
            const bool styleOk=setPresentationStyle(h,styleBefore);
            const bool placementOk=SetWindowPlacement(h,&before)!=FALSE;
            const bool boundsOk=SetWindowPos(h,nullptr,bounds.left,bounds.top,
                bounds.right-bounds.left,bounds.bottom-bounds.top,
                SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED)!=FALSE;
            restored=styleOk && placementOk && boundsOk;
        }
        record("borderless_leave_window_rollback",recovery,0,0,0,restored?S_OK:E_FAIL);
    }
    borderlessTransition=false;
    record("borderless_leave",recovery,0,0,0,result);
    return result;
}
static HRESULT enterBorderless(HWND h,void* recovery) {
    RECT client{};
    if(borderlessTransition || movingWindow || GetForegroundWindow()!=h ||
       GetCapture()!=nullptr || !presentationResizeReady(h,recovery,client))return E_FAIL;
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
    WINDOWPLACEMENT placement{};placement.length=sizeof(placement);
    if(!GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&monitor) ||
       !GetWindowPlacement(h,&placement))return E_FAIL;
    borderlessStyle=GetWindowLongW(h,GWL_STYLE);
    borderlessPlacement=placement;
    borderlessWindow=h;
    borderlessTransition=true;
    // Keep engine/windowed input state intact. Only presentation geometry changes.
    if(IsZoomed(h))ShowWindow(h,SW_RESTORE);
    const RECT& r=monitor.rcMonitor;
    LONG style=static_cast<LONG>(borderlessPresentationStyle(static_cast<DWORD>(borderlessStyle)));
    bool ok=setPresentationStyle(h,style) &&
        SetWindowPos(h,nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,
                     SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
    HRESULT result=ok?commitModelessSize(h,recovery):E_FAIL;
    borderlessTransition=false;
    if(FAILED(result)) {
        HRESULT rollback=leaveBorderless(recovery);
        record("borderless_rollback",recovery,0,0,0,rollback);
    }
    record("borderless_enter",recovery,0,0,0,result);
    return result;
}
static bool handleModelessMove(HWND h,UINT m,WPARAM w,LPARAM l,void* recovery) {
    if(finishingKey.consume(h,m,w))return true;
    if(movingWindow==h) {
        if(m==WM_CAPTURECHANGED || m==WM_CANCELMODE || m==WM_DESTROY ||
           m==WM_CLOSE || (m==WM_SYSCOMMAND && (w&0xfff0)==SC_CLOSE) ||
           (m==WM_ACTIVATE && LOWORD(w)==WA_INACTIVE)) {
            // D175: native SC_CLOSE is ineffective while our operation retains
            // capture. Finish before forwarding close; never destroy or post
            // quit ourselves. WM_DESTROY alone must skip graphics work.
            finishModelessMove(recovery,false,m==WM_DESTROY);
            return false; // Preserve engine activation/destruction handling.
        }
        if(m==WM_KEYDOWN && w==VK_ESCAPE) {
            finishingKey.arm(h,w);
            finishModelessMove(recovery,true);
            return true;
        }
        if(keyboardSizing) {
            if(m==WM_KEYDOWN) {
                if(w==VK_RETURN) {
                    finishingKey.arm(h,w);
                    finishModelessMove(recovery,false);
                }
                else if(w==VK_LEFT || w==VK_RIGHT || w==VK_UP || w==VK_DOWN) {
                    if(!sizingHit) {
                        sizingHit=w==VK_LEFT?HTLEFT:(w==VK_RIGHT?HTRIGHT:
                                  (w==VK_UP?HTTOP:HTBOTTOM));
                        record("modeless_size_select",recovery,sizingHit,presentCalls);
                    } else {
                        RECT current{},desired{};
                        const int step=(GetKeyState(VK_CONTROL)&0x8000)?1:10;
                        const int dx=w==VK_LEFT?-step:(w==VK_RIGHT?step:0);
                        const int dy=w==VK_UP?-step:(w==VK_DOWN?step:0);
                        bool ok=GetCapture()==h && GetWindowRect(h,&current) &&
                            resizeBounds(current,sizingHit,dx,dy,
                                activeSizingLimits.ptMinTrackSize,
                                activeSizingLimits.ptMaxTrackSize,desired);
                        if(ok && !EqualRect(&current,&desired))
                            ok=SetWindowPos(h,nullptr,desired.left,desired.top,
                                desired.right-desired.left,desired.bottom-desired.top,
                                SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE;
                        if(ok && !deferSizingReset)ok=SUCCEEDED(commitModelessSize(h,recovery));
                        if(!ok)finishModelessMove(recovery,false);
                    }
                }
                return true;
            }
            if(m==WM_LBUTTONUP) {
                finishModelessMove(recovery,false);
                return true;
            }
            if(m==WM_KEYUP || m==WM_CHAR || m==WM_MOUSEMOVE || m==WM_LBUTTONDOWN)
                return true;
        }
        if(sizingHit && (m==WM_MOUSEMOVE || m==WM_LBUTTONUP)) {
            POINT cursor{};
            RECT desired{},current{};
            bool ok=GetCapture()==h && GetCursorPos(&cursor) &&
                resizeBounds(moveBounds,sizingHit,int64_t(cursor.x)-moveOrigin.x,
                    int64_t(cursor.y)-moveOrigin.y,activeSizingLimits.ptMinTrackSize,
                    activeSizingLimits.ptMaxTrackSize,desired) && GetWindowRect(h,&current);
            if(ok && !EqualRect(&desired,&current))
                ok=SetWindowPos(h,nullptr,desired.left,desired.top,
                    desired.right-desired.left,desired.bottom-desired.top,
                    SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE;
            if(ok && !deferSizingReset)ok=SUCCEEDED(commitModelessSize(h,recovery));
            if(!ok || m==WM_LBUTTONUP || !(w&MK_LBUTTON))
                finishModelessMove(recovery,false);
            return true;
        }
        if(sizingHit && (m==WM_LBUTTONDOWN || m==WM_KEYDOWN ||
                         m==WM_KEYUP || m==WM_CHAR))return true;
        if(keyboardMove) {
            if(m==WM_KEYDOWN) {
                if(w==VK_RETURN) {
                    finishingKey.arm(h,w);
                    finishModelessMove(recovery,false);
                } else if(w==VK_LEFT || w==VK_RIGHT || w==VK_UP || w==VK_DOWN) {
                    RECT current{};
                    const int step=(GetKeyState(VK_CONTROL)&0x8000)?1:10;
                    int dx=w==VK_LEFT?-step:(w==VK_RIGHT?step:0);
                    int dy=w==VK_UP?-step:(w==VK_DOWN?step:0);
                    if(!GetWindowRect(h,&current) ||
                       !SetWindowPos(h,nullptr,current.left+dx,current.top+dy,0,0,
                                     SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE))
                        finishModelessMove(recovery,false);
                }
                return true;
            }
            if(m==WM_KEYUP || m==WM_CHAR)return true;
            // Menu Move starts a keyboard operation. Ignore pointer motion;
            // a click finishes it without forwarding a click to the world.
            if(m==WM_LBUTTONUP) {
                finishModelessMove(recovery,false);
                return true;
            }
            if(m==WM_MOUSEMOVE || m==WM_LBUTTONDOWN)return true;
        }
        if(m==WM_MOUSEMOVE || m==WM_LBUTTONUP) {
            POINT cursor{};
            if(pendingMaximizedMove) {
                if(GetCapture()!=h || !GetCursorPos(&cursor) ||
                   (m==WM_MOUSEMOVE && !(w&MK_LBUTTON))) {
                    finishModelessMove(recovery,false);
                    return true;
                }
                if(!captionDragStarted(moveOrigin,cursor,GetSystemMetrics(SM_CXDRAG),
                                       GetSystemMetrics(SM_CYDRAG))) {
                    if(m==WM_LBUTTONUP)finishModelessMove(recovery,false);
                    return true; // A caption click must leave the window maximized.
                }
                RECT check{},restored{},desired{};
                if(!presentationResizeReady(h,recovery,check)) {
                    finishModelessMove(recovery,false);
                    return true;
                }
                pendingMaximizedMove=false;
                // ShowWindow owns restoration and native WM_SIZE owns the full
                // engine reset. It can synchronously change capture/activation.
                ShowWindow(h,SW_RESTORE);
                if(movingWindow!=h)return true;
                if(GetCapture()!=h || IsZoomed(h) || !GetWindowRect(h,&restored) ||
                   !restoredCaptionBounds(moveMaximizedBounds,restored,moveOrigin,cursor,desired) ||
                   !presentationResizeReady(h,recovery,check) ||
                   FAILED(commitModelessSize(h,recovery))) {
                    finishModelessMove(recovery,false);
                    return true;
                }
                if(movingWindow!=h || GetCapture()!=h)return true;
                moveBounds=desired;
                moveOrigin=cursor;
                record("modeless_maximized_restore",recovery,0,presentCalls);
            }
            if(GetCapture()!=h || !GetCursorPos(&cursor) ||
               !SetWindowPos(h,nullptr,moveBounds.left+cursor.x-moveOrigin.x,
                            moveBounds.top+cursor.y-moveOrigin.y,0,0,
                            SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE)) {
                finishModelessMove(recovery,false);
                return true;
            }
            if(m==WM_LBUTTONUP || !(w&MK_LBUTTON))
                finishModelessMove(recovery,false);
            return true; // Do not turn caption motion/release into world clicks.
        }
    }
    const bool caption=m==WM_NCLBUTTONDOWN && w==HTCAPTION;
    const bool menuMove=m==WM_SYSCOMMAND && (w&0xfff0)==SC_MOVE;
    const bool menuSize=m==WM_SYSCOMMAND && w==SC_SIZE;
    const bool border=m==WM_NCLBUTTONDOWN && w>=HTLEFT && w<=HTBOTTOMRIGHT;
    if(movingWindow || !((modelessMove && (caption || menuMove)) ||
                         (modelessSizing && (border || menuSize))) ||
       !recovery || h!=field<HWND>(recovery,0xf0) ||
       !field<unsigned char>(recovery,0xac) ||
       field<unsigned char>(recovery,0xb2) || IsIconic(h) ||
       (IsZoomed(h) && !caption) ||
       GetForegroundWindow()!=h || GetCapture()!=nullptr)return false;
    if(!GetWindowRect(h,&moveBounds))return false;
    const bool maximizedCaption=caption && IsZoomed(h);
    WINDOWPLACEMENT placement{};placement.length=sizeof(placement);
    if(maximizedCaption) {
        RECT check{};
        if(!presentationResizeReady(h,recovery,check) ||
           !GetWindowPlacement(h,&placement) || placement.showCmd!=SW_SHOWMAXIMIZED ||
           placement.rcNormalPosition.right<=placement.rcNormalPosition.left ||
           placement.rcNormalPosition.bottom<=placement.rcNormalPosition.top)return false;
    }
    if(border || menuSize) {
        // Use limits from a real OS WM_GETMINMAXINFO, after NativeSizing's
        // correction. Never fabricate a zero-filled sizing notification.
        RECT check{};
        if(!field<unsigned char>(recovery,0xb4) || field<unsigned char>(recovery,0xaf) ||
           sizingLimitsWindow!=h || sizingLimitsStyle!=GetWindowLongW(h,GWL_STYLE) ||
           !(sizingLimitsStyle&WS_THICKFRAME) ||
           !resizeBounds(moveBounds,border?static_cast<UINT>(w):HTRIGHT,0,0,
               sizingLimits.ptMinTrackSize,sizingLimits.ptMaxTrackSize,check))return false;
        activeSizingLimits=sizingLimits;
        if(deferSizingReset) {
            RECT client{};
            // Start only with a consistent normal-window state. Pending native
            // min/max transitions may commit from WM_SIZE and are not previews.
            if(field<unsigned char>(recovery,0xb0) || field<unsigned char>(recovery,0xb1) ||
               !GetClientRect(h,&client) || client.right<=0 || client.bottom<=0 ||
               field<unsigned>(recovery,0xb8)!=static_cast<unsigned>(client.right) ||
               field<unsigned>(recovery,0xbc)!=static_cast<unsigned>(client.bottom))return false;
        }
    }
    // Use the event's screen point: by dispatch time the cursor may already
    // have reached the drag endpoint. Signed words preserve negative monitors.
    moveOrigin.x=static_cast<short>(LOWORD(l));
    moveOrigin.y=static_cast<short>(HIWORD(l));
    SetCapture(h);
    if(GetCapture()!=h)return false;
    movingWindow=h;
    pendingMaximizedMove=moveBeganMaximized=maximizedCaption;
    movePlacement=placement;
    moveMaximizedBounds=moveBounds;
    sizingHit=border?static_cast<UINT>(w):0;
    keyboardSizing=menuSize;
    keyboardMove=menuMove;
    record((border || menuSize)?"modeless_size_begin":"modeless_move_begin",recovery,
           sizingHit,presentCalls,(keyboardMove || keyboardSizing)?1:0);
    if(boundMoveSizePump && tracePresentation)
        record("pump_bound_operation_begin",recovery,0,pumpYields,
            static_cast<LPARAM>(gameplayPumpYields),0,
            gameplayPumpDepth?gameplayPumpCaller:pumpCaller);
    return true;
}
static LRESULT CALLBACK outerHook(HWND h,UINT m,WPARAM w,LPARAM l) {
    if(traceActivation && (m==WM_ACTIVATE || m==WM_ACTIVATEAPP ||
                           m==WM_SETFOCUS || m==WM_KILLFOCUS ||
                           m==WM_CAPTURECHANGED || m==WM_CANCELMODE ||
                           m==WM_ENTERMENULOOP || m==WM_EXITMENULOOP))
        activationRecord(m,w,l);
    auto app=*reinterpret_cast<unsigned char**>(0xa04104);
    void* recovery=app?app+0x20:nullptr;
    // D184: observe native maximized-caption semantics before replacing them.
    // Event coordinates and dispatch-time cursor coordinates can differ. Keep
    // the native procedure/capture/placement completely untouched in this trace.
    const bool captionTrace=tracePresentation && recovery && captionTraceCount<256 &&
        ((m==WM_NCLBUTTONDOWN && w==HTCAPTION) || m==WM_MOVING ||
         (m==WM_SYSCOMMAND && (w&0xfff0)==SC_MOVE));
    if(captionTrace) {
        ++captionTraceCount;
        DWORD savedError=GetLastError();
        record("caption_enter",recovery,m,w,l);
        POINT cursor{};
        const BOOL cursorOk=GetCursorPos(&cursor);
        record("caption_cursor",recovery,0,static_cast<WPARAM>(cursor.x),
            cursor.y,cursorOk?S_OK:E_FAIL);
        record("caption_capture",recovery,0,reinterpret_cast<WPARAM>(GetCapture()),
            GetKeyState(VK_LBUTTON)&0x8000);
        record("caption_progress",recovery,0,presentCalls,
            MAKELPARAM(GetSystemMetrics(SM_CXDRAG),GetSystemMetrics(SM_CYDRAG)));
        SetLastError(savedError);
    }
    // Bounded observation only: never validate, redraw or print a window here.
    const bool surfaceTrace=traceSurfaceMessages && recovery &&
        (m==WM_PAINT || m==WM_ERASEBKGND || m==WM_NCPAINT ||
         m==WM_PRINT || m==WM_PRINTCLIENT || m==WM_CHANGEUISTATE ||
         m==WM_UPDATEUISTATE || m==WM_QUERYUISTATE);
    const bool logSurface=surfaceTrace && surfaceMessages<512;
    if(surfaceTrace && surfaceMessages<=512) {
        DWORD savedError=GetLastError();
        record(logSurface?"surface_enter":"surface_limit",recovery,m,w,l);
        ++surfaceMessages;
        SetLastError(savedError);
    }
    // D051: observe only test control keys, without reading device state or
    // changing acquisition. lParam retains scan code and transition flags.
    const bool inputTrace=traceInputMessages && recovery &&
        (m==WM_KEYDOWN || m==WM_KEYUP || m==WM_SYSKEYDOWN || m==WM_SYSKEYUP ||
         m==WM_CHAR || m==WM_SYSCHAR) &&
        (w==VK_ESCAPE || w==VK_RETURN || w==VK_SPACE || w=='I' ||
         w==VK_LEFT || w==VK_RIGHT || w==VK_UP || w==VK_DOWN || w==VK_CONTROL);
    if(inputTrace)record("input_message",recovery,m,w,l);
    if(m==WM_DESTROY && h==borderlessWindow)borderlessWindow=nullptr;
    if(m==WM_DESTROY && h==pendingWindowedSyncWindow) {
        pendingWindowedSyncOwner=nullptr;
        pendingWindowedSyncWindow=nullptr;
    }
    if(m==WM_DESTROY && h==resetFaultWindow) {
        resetFaultOwner=nullptr;resetFaultWindow=nullptr;
    }
    // Diagnostic one-shot arm, separate from ordinary fixes. The next eligible
    // changed windowed resize calls the real native Reset with one extra live
    // backbuffer reference, then immediately releases it. No HRESULT spoofing.
    if(diagnostics && diagnosticHoldResizeBackbuffer && recovery &&
       m==WM_KEYDOWN && w==VK_F8) {
        if(!(l & (1L<<30))) {
            finishingKey.arm(h,w);
            RECT client{};
            bool ready=recovery && field<unsigned char>(recovery,0x30) &&
                !movingWindow && !borderlessTransition && GetForegroundWindow()==h &&
                GetCapture()==nullptr && presentationResizeReady(h,recovery,client);
            if(ready) { resetFaultOwner=recovery;resetFaultWindow=h; }
            record("fault_resize_armed",recovery,m,w,l,ready?S_OK:E_FAIL);
        }
        return 0;
    }
    // D114: opt-in shortcut policy, not a replacement for native exclusive
    // mode or the Graphics checkbox. Native Alt+Enter posts message 0x407;
    // intercept the real key gesture before that post, keeping native mode,
    // input cooperation and serialized FullScreen selection windowed.
    if(borderlessAltEnter && recovery && h==field<HWND>(recovery,0xf0) &&
       field<unsigned char>(recovery,0xac) &&
       m==WM_SYSKEYDOWN && w==VK_RETURN && (l & (1L<<29))) {
        if(!(l & (1L<<30))) {
            finishingKey.arm(h,w,true);
            HRESULT result=borderlessWindow?leaveBorderless(recovery):enterBorderless(h,recovery);
            record("borderless_alt_enter",recovery,m,w,l,result);
        }
        return 0;
    }
    if(borderlessHotkey && recovery && m==WM_KEYDOWN && w==VK_F11 &&
       !(GetKeyState(VK_MENU)&0x8000) && !(GetKeyState(VK_CONTROL)&0x8000)) {
        if(!(l & (1L<<30))) {
            finishingKey.arm(h,w);
            HRESULT result=borderlessWindow?leaveBorderless(recovery):enterBorderless(h,recovery);
            record("borderless_hotkey",recovery,m,w,l,result);
        }
        return 0;
    }
    const bool inputWasSizing=sizingHit!=0 || keyboardSizing;
    if(handleModelessMove(h,m,w,l,recovery)) {
        if(inputTrace)record(inputWasSizing?"input_consumed_size":"input_consumed_move",
                             recovery,m,w,l);
        return 0;
    }
    if(tracePresentation && recovery &&
       (m==WM_ENTERSIZEMOVE || m==WM_EXITSIZEMOVE)) {
        // Count completed hooked Present calls, including failures. caller is
        // the active message-pump caller, not an inferred render entry point.
        record(m==WM_ENTERSIZEMOVE?"modal_enter":"modal_exit",recovery,m,
               presentCalls,static_cast<LPARAM>(pumpDepth),0,pumpCaller);
    }
    bool trace=recovery && (m==WM_GETMINMAXINFO || m==WM_SIZING);
    bool unlock=nativeSizing && recovery && m==WM_GETMINMAXINFO && l &&
        field<unsigned char>(recovery,0xac) && h==field<HWND>(recovery,0xf0);
    MINMAXINFO systemLimits{};
    if(unlock)systemLimits=*reinterpret_cast<MINMAXINFO*>(l);
    if(trace)record("outer_enter",recovery,m,w,l);
    LRESULT r=outerOriginal(h,m,w,l);
    if(captionTrace) {
        DWORD savedError=GetLastError();
        record("caption_leave",recovery,m,w,l,static_cast<HRESULT>(r));
        SetLastError(savedError);
    }
    if(logSurface) {
        DWORD savedError=GetLastError();
        record("surface_leave",recovery,m,w,l,static_cast<HRESULT>(r));
        SetLastError(savedError);
    }
    if(inputTrace)record("input_forwarded",recovery,m,w,l,static_cast<HRESULT>(r));
    if(unlock) {
        auto limits=reinterpret_cast<MINMAXINFO*>(l);
        limits->ptMinTrackSize=systemLimits.ptMinTrackSize;
        limits->ptMaxTrackSize=systemLimits.ptMaxTrackSize;
        // Native tracking must not collapse the game's UI to the OS titlebar
        // minimum. Convert client pixels to decorated window dimensions, while
        // retaining Windows' maximum tracking size and maximize placement.
        RECT minimum{0,0,static_cast<LONG>(minClientWidth),static_cast<LONG>(minClientHeight)};
        if(minClientWidth && minClientHeight && AdjustWindowRectEx(&minimum,
           static_cast<DWORD>(GetWindowLongW(h,GWL_STYLE)),GetMenu(h)!=nullptr,
           static_cast<DWORD>(GetWindowLongW(h,GWL_EXSTYLE)))) {
            limits->ptMinTrackSize.x=max(limits->ptMinTrackSize.x,
                min(minimum.right-minimum.left,limits->ptMaxTrackSize.x));
            limits->ptMinTrackSize.y=max(limits->ptMinTrackSize.y,
                min(minimum.bottom-minimum.top,limits->ptMaxTrackSize.y));
        }
        record("unlock_native_sizing",recovery,m,w,l);
        sizingLimits=*limits;
        sizingLimitsWindow=h;
        sizingLimitsStyle=GetWindowLongW(h,GWL_STYLE);
    }
    if(trace)record("outer_leave",recovery,m,w,l,static_cast<HRESULT>(r));
    return r;
}
static bool writeCode(void* target,const void* bytes,size_t size) {
    DWORD old,unused;
    if(!VirtualProtect(target,size,PAGE_EXECUTE_READWRITE,&old))return false;
    memcpy(target,bytes,size);
    bool ok=FlushInstructionCache(GetCurrentProcess(),target,size)!=FALSE;
    return VirtualProtect(target,size,old,&unused)!=FALSE && ok;
}
// D068: only the short-window branch's local scale arguments are changed.
// Keep manager+D8/DC intact: other paths use them for screen positioning.
static void* uniformSmallUiContinue;
static void* uniformSmallUiAnchorsContinue;
static const float uniformSmallUiOne=1.0f;
static __declspec(naked) void uniformSmallUiHook() {
    __asm {
        pushfd
        sub esp, 16
        movups xmmword ptr [esp], xmm0
        // Original locals are [esp+0C] (X) and [esp+14] (Y).
        movss xmm0, dword ptr [esp+20h]
        minss xmm0, dword ptr [esp+28h]
        minss xmm0, dword ptr [uniformSmallUiOne]
        movss dword ptr [esp+20h], xmm0
        movss dword ptr [esp+28h], xmm0
        movups xmm0, xmmword ptr [esp]
        add esp, 16
        popfd
        jmp dword ptr [uniformSmallUiContinue]
    }
}
static void __cdecl repositionSmallUiRoot(void* scene) {
    auto root=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(scene)+4);
    auto manager=*reinterpret_cast<unsigned char**>(0xa0622c);
    if(!root || !manager)return;
    // Only the UIPane root type observed in D068/D070, with no parent.
    if(*reinterpret_cast<uintptr_t*>(root)!=0x960ea4 ||
       *reinterpret_cast<uintptr_t*>(root+4)!=0)return;
    const auto fields=reinterpret_cast<const short*>(root);
    const int width=*reinterpret_cast<short*>(manager+0x70);
    const int height=*reinterpret_cast<short*>(manager+0x72);
    if(width<=0 || height<=0 || fields[0x20/2]<0 || fields[0x22/2]<0 ||
       fields[0x14/2]<0 || fields[0x16/2]<0)return;
    const int x=rootUiPosition(fields[0x24/2],fields[0x20/2],fields[0x14/2],width,1024);
    const int y=rootUiPosition(fields[0x26/2],fields[0x22/2],fields[0x16/2],height,768);
    // Original 4d30a0 uses these two virtual methods to place and propagate.
    auto vtable=*reinterpret_cast<uintptr_t**>(root);
    using PositionFn=void(__thiscall*)(void*,int,int);
    using LayoutFn=void(__thiscall*)(void*);
    reinterpret_cast<PositionFn>(vtable[0x0c/4])(root,x,y);
    reinterpret_cast<LayoutFn>(vtable[0x30/4])(root);
}
static __declspec(naked) void uniformSmallUiAnchorsHook() {
    __asm {
        // At 4c8ba6 the subtree virtual call has returned. Subsequent original
        // code uses no incoming floating-point values; preserve integer state.
        pushfd
        pushad
        push esi
        call repositionSmallUiRoot
        add esp, 4
        popad
        popfd
        jmp dword ptr [uniformSmallUiAnchorsContinue]
    }
}
struct Hook {
    uintptr_t address; const char* expected; size_t length; void* replacement; void* trampoline{};
    bool manualContinuation{};
    bool prepare() {
        if(memcmp(reinterpret_cast<void*>(address),expected,length))return false;
        if(manualContinuation)return true;
        auto p=static_cast<unsigned char*>(VirtualAlloc(nullptr,length+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
        if(!p)return false;
        memcpy(p,expected,length); p[length]=0xe9;
        *reinterpret_cast<uint32_t*>(p+length+1)=static_cast<uint32_t>(address+length-reinterpret_cast<uintptr_t>(p+length+5));
        DWORD old;
        if(!VirtualProtect(p,length+5,PAGE_EXECUTE_READ,&old))return false;
        if(!FlushInstructionCache(GetCurrentProcess(),p,length+5))return false;
        trampoline=p; return true;
    }
    bool install() {
        unsigned char patch[16]; memset(patch,0x90,length);patch[0]=0xe9;
        *reinterpret_cast<uint32_t*>(patch+1)=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(replacement)-address-5);
        return writeCode(reinterpret_cast<void*>(address),patch,length);
    }
};
static std::string exeHash() {
    wchar_t path[MAX_PATH];
    if(!GetModuleFileNameW(nullptr,path,MAX_PATH))return {};
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    if(f==INVALID_HANDLE_VALUE)return {};
    BCRYPT_ALG_HANDLE alg{}; BCRYPT_HASH_HANDLE hash{};
    unsigned char digest[32]; char hex[65]{}; unsigned char buffer[65536]; DWORD n;
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    while(ok) {
        if(!ReadFile(f,buffer,sizeof(buffer),&n,nullptr)){ok=false;break;}
        if(!n)break;
        ok=BCryptHashData(hash,buffer,n,0)>=0;
    }
    if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);
    if(!ok)return {};
    for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",digest[i]);
    return hex;
}
extern "C" __declspec(dllexport) DWORD WINAPI Initialize(void* argument) {
    if(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))!=0x400000)return 2;
    auto ini=static_cast<const wchar_t*>(argument);
    std::string hash=exeHash();
    bool v2=hash=="bd599aa8cb680e44516e18a8847f0edd201a331227af94baf20fc5f74ff605ae";
    bool original=hash=="5bfcdac2cdcdbb460eb7995a7b0521e9a2398bee3918f9589c4e708f385d091e";
    // Our wrappers change stack layout. Never observe unmodified v2 through them.
    if(!original && !v2)return 3;
    bool revert=GetPrivateProfileIntW(L"Experiment",L"RevertV2",0,ini)!=0;
    bool fixMinimapHeight=GetPrivateProfileIntW(L"Experiment",L"FixMinimapHeight",0,ini)!=0;
    bool uniformSmallUi=GetPrivateProfileIntW(L"Experiment",L"UniformSmallUi",0,ini)!=0;
    bool uniformSmallUiAnchors=GetPrivateProfileIntW(L"Experiment",L"UniformSmallUiAnchors",0,ini)!=0;
    if(uniformSmallUiAnchors && !uniformSmallUi)return 17;
    // Verify the complete branch selection as well as the overwritten site.
    const unsigned char scaleBranchExpected[]={0x0f,0x8c,0xf2,0,0,0,
        0x81,0xfb,0,3,0,0,0x0f,0x8c,0xe6,0,0,0};
    if(uniformSmallUi && memcmp(reinterpret_cast<void*>(0x4c8a8f),
        scaleBranchExpected,sizeof(scaleBranchExpected)))return 16;
    // Mini scene initialization clears saved height (+22) after copying it.
    // Match the area-map initialization: clear saved Y (+26) instead.
    // This is applied before game entry, never to an existing scene.
    const unsigned char minimapExpected[]={0x8b,0x8e,0xa0,0,0,0,0x66,0x89,0x59,0x22};
    if(fixMinimapHeight && memcmp(reinterpret_cast<void*>(0x70950f),minimapExpected,sizeof(minimapExpected)))return 12;
    guardEmptyRestore=GetPrivateProfileIntW(L"Experiment",L"GuardEmptyRestore",0,ini)!=0;
    nativeSizing=GetPrivateProfileIntW(L"Experiment",L"NativeSizing",0,ini)!=0;
    modelessMove=GetPrivateProfileIntW(L"Experiment",L"ModelessMove",0,ini)!=0;
    modelessSizing=GetPrivateProfileIntW(L"Experiment",L"ModelessSizing",0,ini)!=0;
    if(modelessSizing && !nativeSizing)return 14;
    boundMoveSizePump=GetPrivateProfileIntW(L"Experiment",L"BoundMoveSizePump",0,ini)!=0;
    if(boundMoveSizePump && !modelessMove && !modelessSizing)return 19;
    // Verify the caller and the full native exit, not just the branch overwrite.
    if(boundMoveSizePump && (memcmp(reinterpret_cast<void*>(0x40740e),
        "\xe8\xdd\xb6\xff\xff",5) || memcmp(reinterpret_cast<void*>(0x402b62),
        "\x85\xc0\x75\xc2\x5f\x5e\x5b\x8b\x44\x24\x0c\x5d\x83\xc4\x1c\xc3",16)))return 20;
    if(boundMoveSizePump && (memcmp(reinterpret_cast<void*>(0x6ff467),
        "\xe8\xc4\x40\x01\x00",5) || memcmp(reinterpret_cast<void*>(0x71356c),
        "\x85\xc0\x74\x44",4) || memcmp(reinterpret_cast<void*>(0x7135b4),
        "\x8b\x0d\x58\x36\xa0\x00",6) || memcmp(reinterpret_cast<void*>(0x713587),
        "\x85\xc0\x0f\x84\x7e\x02\x00\x00",8) ||
        memcmp(reinterpret_cast<void*>(0x71380d),
        "\xc7\x05\x40\x36\xa0\x00\x01\x00\x00\x00\xe9\x7a\x03\x00\x00",15)))return 21;
    deferSizingReset=GetPrivateProfileIntW(L"Experiment",L"DeferSizingReset",0,ini)!=0;
    if(deferSizingReset && !modelessSizing)return 15;
    minClientWidth=GetPrivateProfileIntW(L"Experiment",L"MinClientWidth",800,ini);
    minClientHeight=GetPrivateProfileIntW(L"Experiment",L"MinClientHeight",600,ini);
    if(nativeSizing && (minClientWidth>16384 || minClientHeight>16384 ||
       ((minClientWidth==0)!=(minClientHeight==0))))return 11;
    syncWindowedMode=GetPrivateProfileIntW(L"Experiment",L"SyncWindowedMode",0,ini)!=0;
    preservePlacement=GetPrivateProfileIntW(L"Experiment",L"PreservePlacement",0,ini)!=0;
    fixGraphicsControls=GetPrivateProfileIntW(L"Experiment",L"FixGraphicsControls",0,ini)!=0;
    fitWindowedApply=GetPrivateProfileIntW(L"Experiment",L"FitWindowedApply",0,ini)!=0;
    if(fitWindowedApply && (!nativeSizing || !syncWindowedMode))return 22;
    if(fixGraphicsControls && (memcmp(reinterpret_cast<void*>(0x80c0ad),
        "\xe8\x3e\xf1\xff\xff",5) || memcmp(reinterpret_cast<void*>(0x4d5880),
        "\x55\x8b\x6c\x24\x08\x57\x8b\xf9",8) ||
        memcmp(reinterpret_cast<void*>(0x4f1b30),"\x8b\x81\x64\x02\x00\x00",6) ||
        memcmp(reinterpret_cast<void*>(0x4287a0),"\x8b\x01\x85\xc0\x8b\x4c\x24\x04\x8b\x09",10)))return 23;
    borderlessHotkey=GetPrivateProfileIntW(L"Experiment",L"BorderlessHotkey",0,ini)!=0;
    borderlessAltEnter=GetPrivateProfileIntW(L"Experiment",L"BorderlessAltEnter",0,ini)!=0;
    const bool borderlessEnabled=borderlessHotkey || borderlessAltEnter;
    const bool recoveryEnabled=borderlessEnabled || syncWindowedMode;
    const bool resolutionEnabled=borderlessEnabled || syncWindowedMode;
    if(borderlessEnabled && (!nativeSizing || !syncWindowedMode))return 18;
    tracePresentation=GetPrivateProfileIntW(L"Experiment",L"TracePresentation",0,ini)!=0;
    throttleLostDevice=GetPrivateProfileIntW(L"Experiment",L"ThrottleLostDevice",0,ini)!=0;
    traceActivation=GetPrivateProfileIntW(L"Experiment",L"TraceActivation",0,ini)!=0;
    traceInputMessages=GetPrivateProfileIntW(L"Experiment",L"TraceInputMessages",0,ini)!=0;
    traceInputAcquisition=GetPrivateProfileIntW(L"Experiment",L"TraceInputAcquisition",0,ini)!=0;
    traceGraphicsControls=GetPrivateProfileIntW(L"Experiment",L"TraceGraphicsControls",0,ini)!=0;
    traceSurfaceMessages=GetPrivateProfileIntW(L"Experiment",L"TraceSurfaceMessages",0,ini)!=0;
    diagnostics=GetPrivateProfileIntW(L"Experiment",L"Diagnostics",1,ini)!=0;
    diagnosticHoldResizeBackbuffer=diagnostics && GetPrivateProfileIntW(
        L"Experiment",L"DiagnosticHoldResizeBackbuffer",0,ini)!=0;
    if(!diagnostics) {
        tracePresentation=false; traceActivation=false;
        traceInputMessages=false; traceSurfaceMessages=false;
        traceInputAcquisition=false;
        traceGraphicsControls=false;
    }
    if(v2 && !revert)return 4;
    const unsigned char v2Bytes[]={0x8b,0x84,0xe4,0x84,0,0,0,0x8b,0x94,0xe4,0x88,0,0,0,0xeb,0x28,0x90,0x90};
    const unsigned char originalBytes[]={0x8b,0x86,0x74,0x02,0,0,0x8b,0x4c,0x24,0x14,0x2b,0x07,0x2b,0x4c,0x24,0x0c,0x3b,0xc8};
    if(memcmp(reinterpret_cast<void*>(0x4424bc),v2?v2Bytes:originalBytes,18))return 5;
    std::vector<Hook> hooks={
      {0x442c70,"\x8b\x44\x24\x08\x3d\x12\x01\x00\x00",9,reinterpret_cast<void*>(messageHook)},
      {0x442460,"\x83\xec\x10\x53\x56",5,reinterpret_cast<void*>(resizeHook)},
      {0x441aa0,"\x83\xec\x14\x56\x8b\xf1",6,reinterpret_cast<void*>(resetHook)},
      {0x403940,"\x8b\x0d\x04\x41\xa0\x00\x8b\x41\x20",9,reinterpret_cast<void*>(outerHook)},
      {0x4430a0,"\x56\x8b\xf1\x80\xbe\xac\x00\x00\x00\x00",10,reinterpret_cast<void*>(modeHook)}};
    if(tracePresentation || throttleLostDevice) {
        hooks.push_back({0x433990,"\x8b\x54\x24\x04\x56",5,reinterpret_cast<void*>(presentHook)});
    }
    if(tracePresentation) {
        hooks.push_back({0x441cb0,"\x56\x8b\xf1\x80\xbe\xaf\x00\x00\x00\x00",10,reinterpret_cast<void*>(recoveryHook)});
        hooks.push_back({0x402af0,"\x83\xec\x1c\x33\xc0",5,reinterpret_cast<void*>(pumpHook)});
    }
    const size_t resolutionHookIndex=hooks.size();
    if(resolutionEnabled) {
        hooks.push_back({0x435dc0,"\x8b\x15\x04\x41\xa0\x00",6,
            reinterpret_cast<void*>(resolutionHook)});
    }
    const size_t pendingRecoveryHookIndex=hooks.size();
    if(recoveryEnabled && !tracePresentation) {
        hooks.push_back({0x441cb0,"\x56\x8b\xf1\x80\xbe\xaf\x00\x00\x00\x00",10,
            reinterpret_cast<void*>(recoveryHook)});
    }
    if(uniformSmallUi) {
        hooks.push_back({0x4c8b87,"\x8b\x4e\x04\xd9\x44\x24\x14",7,
            reinterpret_cast<void*>(uniformSmallUiHook)});
    }
    if(uniformSmallUiAnchors) {
        hooks.push_back({0x4c8ba6,"\x8b\x16\x8b\x42\x28",5,
            reinterpret_cast<void*>(uniformSmallUiAnchorsHook)});
    }
    const size_t acquisitionHookIndex=hooks.size();
    if(traceInputAcquisition) {
        hooks.push_back({0x4263a0,"\x51\x53\x55\x56\x8b\xf1",6,
            reinterpret_cast<void*>(inputAcquisitionHook)});
        hooks.push_back({0x42648a,"\x5e\x5d\x5b\x59\xc2\x04\x00",7,
            reinterpret_cast<void*>(inputAcquisitionExitHook)});
    }
    const size_t quietPumpHookIndex=hooks.size();
    if(boundMoveSizePump && !tracePresentation)
        hooks.push_back({0x402af0,"\x83\xec\x1c\x33\xc0",5,reinterpret_cast<void*>(pumpHook)});
    if(boundMoveSizePump)
        hooks.push_back({0x402b62,"\x85\xc0\x75\xc2\x5f",5,
            reinterpret_cast<void*>(pumpBackedgeHook),nullptr,true});
    const size_t gameplayPumpHookIndex=hooks.size();
    if(boundMoveSizePump) {
        hooks.push_back({0x713530,"\x55\x8b\xec\x83\xe4\xf8",6,
            reinterpret_cast<void*>(gameplayPumpHook)});
        hooks.push_back({gameplayPumpBackedgeAddress,"\xff\xd7\x85\xc0\x75\xc2",6,
            reinterpret_cast<void*>(gameplayPumpBackedgeHook),nullptr,true});
    }
    // Append new hooks after existing fixed trace/UI/input/pump indices.
    const size_t graphicsListHookIndex=hooks.size();
    if(fixGraphicsControls || traceGraphicsControls)
        hooks.push_back({0x80b1f0,"\x83\xec\x28\x56\x57",5,reinterpret_cast<void*>(graphicsListHook)});
    const size_t graphicsPopulateHookIndex=hooks.size();
    if(fixGraphicsControls)
        hooks.push_back({0x80c060,"\x83\xec\x60\x53\x55",5,reinterpret_cast<void*>(graphicsPopulateHook)});
    if(fitWindowedApply)
        hooks.push_back({0x4427d9,"\xff\x15\x78\x87\x94\x00",6,
            reinterpret_cast<void*>(applyMoveWindowThunk),nullptr,true});
    for(auto& hook:hooks)if(!hook.prepare())return 6;
    if(uniformSmallUi)uniformSmallUiContinue=hooks[acquisitionHookIndex-(uniformSmallUiAnchors?2:1)].trampoline;
    if(uniformSmallUiAnchors)uniformSmallUiAnchorsContinue=hooks[acquisitionHookIndex-1].trampoline;
    if(traceInputAcquisition) {
        inputAcquisitionContinue=hooks[acquisitionHookIndex].trampoline;
        inputAcquisitionExitContinue=hooks[acquisitionHookIndex+1].trampoline;
    }
    wchar_t logPath[MAX_PATH];
    if(!GetPrivateProfileStringW(L"Experiment",L"Log",L"",logPath,MAX_PATH,ini))return 7;
    logFile=CreateFileW(logPath,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(logFile==INVALID_HANDLE_VALUE)return 8;
    char header[768];DWORD written;
    int count=snprintf(header,sizeof(header),"{\"kind\":\"probe_start\",\"sha256\":\"%s\",\"revert_v2\":%d,\"guard_empty_restore\":%d,\"native_sizing\":%d,\"sync_windowed_mode\":%d,\"preserve_placement\":%d,\"trace_presentation\":%d,\"throttle_lost_device\":%d,\"trace_activation\":%d,\"trace_input_messages\":%d,\"diagnostics\":%d,\"min_client\":[%u,%u],\"fix_minimap_height\":%d,\"modeless_move\":%d,\"modeless_sizing\":%d,\"pid\":%lu}\n",hash.c_str(),v2&&revert,guardEmptyRestore,nativeSizing,syncWindowedMode,preservePlacement,tracePresentation,throttleLostDevice,traceActivation,traceInputMessages,diagnostics,minClientWidth,minClientHeight,fixMinimapHeight,modelessMove,modelessSizing,GetCurrentProcessId());
    WriteFile(logFile,header,count,&written,nullptr);
    count=snprintf(header,sizeof(header),
        "{\"kind\":\"surface_config\",\"enabled\":%d,\"message_limit\":512}\n",
        traceSurfaceMessages);
    WriteFile(logFile,header,count,&written,nullptr);
    count=snprintf(header,sizeof(header),
        "{\"kind\":\"sizing_config\",\"defer_reset\":%d}\n",deferSizingReset);
    WriteFile(logFile,header,count,&written,nullptr);
    count=snprintf(header,sizeof(header),
        "{\"kind\":\"ui_scaling_config\",\"uniform_small_ui\":%d,\"root_anchors\":%d}\n",uniformSmallUi,uniformSmallUiAnchors);
    WriteFile(logFile,header,count,&written,nullptr);
    count=snprintf(header,sizeof(header),
        "{\"kind\":\"borderless_config\",\"hotkey\":%d,\"alt_enter\":%d}\n",
        borderlessHotkey,borderlessAltEnter);
    WriteFile(logFile,header,count,&written,nullptr);
    messageOriginal=reinterpret_cast<MessageFn>(hooks[0].trampoline);
    count=snprintf(header,sizeof(header),
        "{\"kind\":\"input_acquisition_config\",\"enabled\":%d}\n",
        traceInputAcquisition);
    WriteFile(logFile,header,count,&written,nullptr);
    resizeOriginal=reinterpret_cast<EngineFn>(hooks[1].trampoline);
    count=snprintf(header,sizeof(header),
        "{\"kind\":\"reset_fault_config\",\"hold_resize_backbuffer\":%d}\n",
        diagnosticHoldResizeBackbuffer);
    WriteFile(logFile,header,count,&written,nullptr);
    count=snprintf(header,sizeof(header),
        "{\"kind\":\"pump_bound_config\",\"enabled\":%d,\"milliseconds\":8,\"messages\":64,\"gameplay_pump\":%d}\n",
        boundMoveSizePump,boundMoveSizePump);
    WriteFile(logFile,header,count,&written,nullptr);
    if(boundMoveSizePump && !tracePresentation)
        pumpOriginal=reinterpret_cast<PumpFn>(hooks[quietPumpHookIndex].trampoline);
    if(boundMoveSizePump)
        gameplayPumpOriginal=reinterpret_cast<GameplayPumpFn>(hooks[gameplayPumpHookIndex].trampoline);
    resetOriginal=reinterpret_cast<EngineFn>(hooks[2].trampoline);
    outerOriginal=reinterpret_cast<WNDPROC>(hooks[3].trampoline);
    modeOriginal=reinterpret_cast<EngineFn>(hooks[4].trampoline);
    if(resolutionEnabled)
        resolutionOriginal=reinterpret_cast<ResolutionFn>(hooks[resolutionHookIndex].trampoline);
    if(fixGraphicsControls || traceGraphicsControls)
        graphicsListOriginal=reinterpret_cast<GraphicsListFn>(hooks[graphicsListHookIndex].trampoline);
    if(fixGraphicsControls)
        graphicsPopulateOriginal=reinterpret_cast<GraphicsPopulateFn>(hooks[graphicsPopulateHookIndex].trampoline);
    count=snprintf(header,sizeof(header),
        "{\"kind\":\"graphics_controls_config\",\"fix\":%d,\"trace\":%d,\"fit_windowed_apply\":%d}\n",
        fixGraphicsControls,traceGraphicsControls,fitWindowedApply);
    WriteFile(logFile,header,count,&written,nullptr);
    if(tracePresentation || throttleLostDevice) {
        presentOriginal=reinterpret_cast<PresentFn>(hooks[5].trampoline);
    }
    if(tracePresentation) {
        recoveryOriginal=reinterpret_cast<EngineFn>(hooks[6].trampoline);
        pumpOriginal=reinterpret_cast<PumpFn>(hooks[7].trampoline);
    }
    else if(recoveryEnabled)
        recoveryOriginal=reinterpret_cast<EngineFn>(hooks[pendingRecoveryHookIndex].trampoline);
    if(v2 && !writeCode(reinterpret_cast<void*>(0x4424bc),originalBytes,18))return 9;
    const unsigned char minimapY=0x26;
    if(fixMinimapHeight && !writeCode(reinterpret_cast<void*>(0x709518),&minimapY,1))return 13;
    for(auto& hook:hooks)if(!hook.install())return 10;
    return 1;
}
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
