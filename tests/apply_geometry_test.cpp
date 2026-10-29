#include "probe.cpp"
#include <cstdlib>

static HWND fixtureWindow;
static uintptr_t beforeStack,afterStack;
static unsigned afterB,afterS,afterD;
static BOOL moveResult;
static __declspec(naked) void finishMoveFixture() {
    __asm {
        mov moveResult,eax
        mov afterStack,esp
        mov afterB,ebx
        mov afterS,esi
        mov afterD,edi
        pop edi
        pop esi
        pop ebx
        ret
    }
}
static __declspec(naked) void exerciseMoveThunk() {
    __asm {
        push ebx
        push esi
        push edi
        mov beforeStack,esp
        mov ebx,11223344h
        mov esi,22334455h
        mov edi,33445566h
        push 0
        push 2000
        push 3000
        push 400
        push 600
        push dword ptr [fixtureWindow]
        // The installed site JMPs with native arguments already on the stack.
        // CALLing the thunk here would incorrectly insert an extra argument.
        jmp applyMoveWindowThunk
    }
}
static bool geometryCases() {
    RECT out{},work={0,0,2560,1392};
    bool ok=fitApplyBounds(770,250,2576,1479,work,out) &&
        out.left==0 && out.top==0 && out.right==2560 && out.bottom==1392;
    const RECT negative={-1920,-100,-320,900};
    ok=ok && fitApplyBounds(-1800,50,800,600,negative,out) && out.left==-1800 && out.top==50;
    ok=ok && fitApplyBounds(-3000,-500,2000,1500,negative,out) && EqualRect(&out,&negative);
    ok=ok && fitApplyBounds(INT_MAX,INT_MAX,800,600,negative,out) &&
        out.left==-1120 && out.top==300;
    const RECT invalid={LONG_MIN,0,LONG_MAX,100};
    return ok && !fitApplyBounds(0,0,0,100,work,out) &&
        !fitApplyBounds(0,0,100,100,invalid,out);
}
static bool thunkAndForwarding(HWND window) {
    alignas(void*) unsigned char recovery[0x300]{};
    recovery[0xac]=1;
    *reinterpret_cast<HWND*>(recovery+0xf0)=window;
    applyFitContext={recovery,window,{100,100,900,700},true};
    fitWindowedApply=true;diagnostics=false;
    applyMoveContinue=reinterpret_cast<void*>(finishMoveFixture);
    fixtureWindow=window;
    exerciseMoveThunk();
    RECT actual{},expected={100,100,900,700};
    GetWindowRect(window,&actual);
    bool ok=moveResult && beforeStack==afterStack && afterB==0x11223344 &&
        afterS==0x22334455 && afterD==0x33445566 && EqualRect(&actual,&expected);
    // Disabled, unowned and fullscreen paths forward the native geometry.
    expected={600,400,1100,800};
    fitWindowedApply=false;
    ok=ok && applyMoveWindowHook(window,600,400,500,400,FALSE);
    GetWindowRect(window,&actual);ok=ok && EqualRect(&actual,&expected);
    fitWindowedApply=true;applyFitContext.active=false;
    ok=ok && applyMoveWindowHook(window,600,400,500,400,FALSE);
    GetWindowRect(window,&actual);ok=ok && EqualRect(&actual,&expected);
    applyFitContext.active=true;recovery[0xac]=0;
    ok=ok && applyMoveWindowHook(window,600,400,500,400,FALSE);
    GetWindowRect(window,&actual);
    return ok && EqualRect(&actual,&expected) && !IsWindowVisible(window);
}
int main() {
    HWND window=CreateWindowExW(0,L"STATIC",L"Apply fit fixture",WS_OVERLAPPEDWINDOW,
        100,100,400,300,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window)return EXIT_FAILURE;
    const bool ok=geometryCases() && thunkAndForwarding(window);
    DestroyWindow(window);
    if(!ok)std::fprintf(stderr,"Apply geometry/native stdcall regression\n");
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
