// Exercise the actual branch thunk and policy without a game or graphics device.
#include "probe.cpp"
#include <cstdlib>
static unsigned peekResult=1, pathTaken, expectedStack;
static unsigned observed[9];
static __declspec(naked) void captureFixture() {
    __asm {
        pushfd
        pop dword ptr [observed+28]
        mov dword ptr [observed], eax
        mov dword ptr [observed+4], ecx
        mov dword ptr [observed+8], edx
        mov dword ptr [observed+12], ebx
        mov dword ptr [observed+16], esi
        mov dword ptr [observed+20], edi
        mov dword ptr [observed+24], ebp
        mov dword ptr [observed+32], esp
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
static __declspec(naked) void loopFixture() {
    __asm {
        mov pathTaken, 1
        pop edi
        jmp captureFixture
    }
}
static __declspec(naked) void exitFixture() {
    __asm {
        mov pathTaken, 2
        jmp captureFixture
    }
}
static __declspec(naked) void runFixture() {
    __asm {
        push ebp
        push ebx
        push esi
        push edi
        mov expectedStack, esp
        push 77777777h
        mov eax, peekResult
        mov ecx, 11111111h
        mov edx, 22222222h
        mov ebx, 33333333h
        mov esi, 44444444h
        mov edi, 55555555h
        mov ebp, 66666666h
        jmp pumpBackedgeHook
    }
}
static unsigned fixtureInputOwner=0x76543210;
static __declspec(naked) void peekFixture() {
    __asm {
        mov eax, peekResult
        ret
    }
}
static __declspec(naked) void gameplayLoopFixture() {
    __asm {
        mov pathTaken, 1
        jmp captureFixture
    }
}
static __declspec(naked) void gameplayBodyFixture() {
    __asm {
        mov ecx, fixtureInputOwner
        mov pathTaken, 2
        jmp captureFixture
    }
}
static __declspec(naked) void runGameplayFixture() {
    __asm {
        push ebp
        push ebx
        push esi
        push edi
        mov expectedStack, esp
        mov eax, peekResult
        mov ecx, 11111111h
        mov edx, 22222222h
        mov ebx, 33333333h
        mov esi, 44444444h
        mov edi, offset peekFixture
        mov ebp, 66666666h
        jmp gameplayPumpBackedgeHook
    }
}
static bool wrapperOk=true;
static unsigned __fastcall gameplayOriginalFixture(void* owner,void*) {
    wrapperOk=wrapperOk && owner==reinterpret_cast<void*>(0x1234) &&
        gameplayPumpCaller && gameplayPumpMessages==0 && GetLastError()==1234;
    gameplayPumpMessages=7;
    if(gameplayPumpDepth==1) {
        const uintptr_t caller=gameplayPumpCaller;
        const ULONGLONG started=gameplayPumpStarted;
        const unsigned inner=gameplayPumpHook(owner,nullptr);
        wrapperOk=wrapperOk && inner==0xabcdef02 && gameplayPumpDepth==1 &&
            gameplayPumpCaller==caller && gameplayPumpStarted==started &&
            gameplayPumpMessages==7;
    }
    return 0xabcdef00+gameplayPumpDepth;
}
// Reproduce the verified native local/saved-register frame, then execute the
// untouched tail. The pump must return MSG.wParam even when yielding early.
static __declspec(naked) void nativeExitFixture() {
    __asm {
        pop esi
        pop ebx
        mov eax, dword ptr [esp+0ch]
        pop ebp
        add esp, 1ch
        ret
    }
}
static __declspec(naked) void nativeLoopFixture() {
    __asm {
        pop edi
        jmp nativeExitFixture
    }
}
static __declspec(naked) unsigned nativeReturnFixture() {
    __asm {
        sub esp, 1ch
        mov dword ptr [esp+8], 89abcdefh
        push ebp
        push ebx
        push esi
        push edi
        mov eax, peekResult
        jmp pumpBackedgeHook
    }
}
static bool check(unsigned path,bool gameplay=false) {
    SetLastError(1234);
    if(gameplay)runGameplayFixture(); else runFixture();
    const unsigned flags=observed[7];
    const bool zero=(flags&0x40)!=0;
    const bool result=pathTaken==path && observed[0]==peekResult &&
        observed[1]==(gameplay && path==2?fixtureInputOwner:0x11111111) && observed[2]==0x22222222 &&
        observed[3]==0x33333333 && observed[4]==0x44444444 &&
        observed[5]==(gameplay?reinterpret_cast<unsigned>(peekFixture):0x77777777u) && observed[6]==0x66666666 &&
        observed[8]==expectedStack && zero==(peekResult==0) &&
        !(flags&0x801) && GetLastError()==1234;
    if(!result)std::fprintf(stderr,"path %u expected %u eax %x regs %x %x %x %x %x %x flags %x stack %x/%x error %lu\n",pathTaken,path,observed[0],observed[1],observed[2],observed[3],observed[4],observed[5],observed[6],flags,observed[8],expectedStack,GetLastError());
    return result;
}
static bool initialEmptyEntryRegression() {
    // Preserve the native initial JE's separate entry into the input body.
    // D143 crashed when the old 10-byte patch overlapped that target. This
    // executes both the branch that bypasses our patch and the patched path.
    static_assert(gameplayPumpBackedgeAddress+6==0x7135b4);
    unsigned char bytes[]={0x57,0xbf,0,0,0,0,0xa1,0,0,0,0,0x85,0xc0,0x74,0x06,
        0xff,0xd7,0x85,0xc0,0x75,0xfa,0xb9,0,0,0,0,0x5f,0xc3};
    const auto peek=reinterpret_cast<uintptr_t>(peekFixture);
    const auto result=reinterpret_cast<uintptr_t>(&peekResult);
    memcpy(bytes+2,&peek,4); memcpy(bytes+7,&result,4);
    memcpy(bytes+22,&fixtureInputOwner,4);
    auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,sizeof(bytes),
        MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!code)return false;
    memcpy(code,bytes,sizeof(bytes));
    DWORD old;
    bool ok=VirtualProtect(code,sizeof(bytes),PAGE_EXECUTE_READ,&old)!=FALSE &&
        FlushInstructionCache(GetCurrentProcess(),code,sizeof(bytes))!=FALSE;
    Hook patch{reinterpret_cast<uintptr_t>(code+15),"\xff\xd7\x85\xc0\x75\xfa",6,
        reinterpret_cast<void*>(gameplayPumpBackedgeHook),nullptr,true};
    ok=ok && patch.prepare() && patch.install();
    if(ok) {
        gameplayPumpBodyContinue=code+21; gameplayPumpLoopContinue=code+15;
        gameplayPumpDepth=1; gameplayPumpCaller=0x6ff46c;
        gameplayPumpStarted=GetTickCount64()-100; boundMoveSizePump=true;
        using Fn=unsigned(__cdecl*)(); const auto fn=reinterpret_cast<Fn>(code);
        peekResult=0; ok=fn()==0 && ok; // Initial JE skips the installed patch.
        peekResult=1; ok=fn()==1 && ok; // Patched backedge yields to same body.
    }
    VirtualFree(code,0,MEM_RELEASE);
    return ok;
}
int main() {
    pumpLoopContinue=reinterpret_cast<void*>(loopFixture);
    pumpExitContinue=reinterpret_cast<void*>(exitFixture);
    HWND window=CreateWindowExW(0,L"STATIC",L"pump-test",0,0,0,1,1,
        nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window)return EXIT_FAILURE;
    // A queued application message and quit must survive every policy decision.
    MSG ignored{}; while(PeekMessageW(&ignored,nullptr,0,0,PM_REMOVE)){}
    if(!PostThreadMessageW(GetCurrentThreadId(),WM_APP+7,0x1234,0x5678))return EXIT_FAILURE;
    PostQuitMessage(73);
    boundMoveSizePump=true; pumpDepth=1; pumpCaller=0x407413;
    movingWindow=window; pumpStarted=GetTickCount64()-100;
    bool ok=!pumpBudgetExceeded(63,7) && pumpBudgetExceeded(64,0) &&
        pumpBudgetExceeded(1,8) && check(2); // elapsed budget yields
    pumpMessages=62;
    ok=check(2) && pumpMessages==63 && ok;
    ok=check(2) && pumpMessages==64 && ok; // message bound
    pumpDepth=2; ok=check(1) && ok;
    pumpDepth=1; pumpCaller=0x407414; ok=check(1) && ok;
    pumpCaller=0x407413; movingWindow=nullptr; ok=check(1) && ok;
    movingWindow=GetDesktopWindow(); ok=check(1) && ok; // another window thread
    movingWindow=window; boundMoveSizePump=false; ok=check(1) && ok;
    boundMoveSizePump=true; peekResult=0; ok=check(2) && ok; // native empty exit
    pumpLoopContinue=reinterpret_cast<void*>(nativeLoopFixture);
    pumpExitContinue=reinterpret_cast<void*>(nativeExitFixture);
    peekResult=0; ok=nativeReturnFixture()==0x89abcdef && ok;
    peekResult=1; ok=nativeReturnFixture()==0x89abcdef && ok;
    boundMoveSizePump=false; ok=nativeReturnFixture()==0x89abcdef && ok;
    gameplayPumpLoopContinue=reinterpret_cast<void*>(gameplayLoopFixture);
    gameplayPumpBodyContinue=reinterpret_cast<void*>(gameplayBodyFixture);
    gameplayPumpDepth=1; gameplayPumpCaller=0x6ff46c;
    gameplayPumpStarted=GetTickCount64()-100;
    boundMoveSizePump=true; peekResult=1;
    const unsigned previousYields=gameplayPumpYields;
    ok=check(2,true) && gameplayPumpYields==previousYields+1 && ok;
    gameplayPumpDepth=2; ok=check(1,true) && ok;
    gameplayPumpDepth=1; gameplayPumpCaller=0x6ff46d; ok=check(1,true) && ok;
    gameplayPumpCaller=0x6ff46c; movingWindow=nullptr; ok=check(1,true) && ok;
    movingWindow=GetDesktopWindow(); ok=check(1,true) && ok;
    movingWindow=window; boundMoveSizePump=false; ok=check(1,true) && ok;
    boundMoveSizePump=true; peekResult=0; ok=check(2,true) && ok;
    gameplayPumpOriginal=reinterpret_cast<GameplayPumpFn>(gameplayOriginalFixture);
    gameplayPumpDepth=0; gameplayPumpCaller=0x4321;
    gameplayPumpStarted=56; gameplayPumpMessages=91;
    SetLastError(1234);
    const unsigned returned=gameplayPumpHook(reinterpret_cast<void*>(0x1234),nullptr);
    ok=wrapperOk && returned==0xabcdef01 && gameplayPumpDepth==0 &&
        gameplayPumpCaller==0x4321 && gameplayPumpStarted==56 &&
        gameplayPumpMessages==91 && GetLastError()==1234 && ok;
    // A manually continued branch hook must not allocate a copied relative
    // branch trampoline. It verifies the expected bytes before any installation.
    unsigned char branch[]={0x85,0xc0,0x75,0xc2,0x5f};
    Hook manual{reinterpret_cast<uintptr_t>(branch),"\x85\xc0\x75\xc2\x5f",5,
        reinterpret_cast<void*>(pumpBackedgeHook),nullptr,true};
    ok=manual.prepare() && !manual.trampoline && ok;
    branch[3]=0; ok=!manual.prepare() && ok;
    ok=initialEmptyEntryRegression() && ok;
    MSG message{};
    ok=PeekMessageW(&message,nullptr,0,0,PM_REMOVE) && message.message==WM_APP+7 &&
        message.wParam==0x1234 && message.lParam==0x5678 && ok;
    ok=PeekMessageW(&message,nullptr,0,0,PM_REMOVE) && message.message==WM_QUIT &&
        message.wParam==73 && ok;
    DestroyWindow(window); movingWindow=nullptr;
    if(!ok)std::fprintf(stderr,"Pump policy, ABI, flags, stack or queue preservation failed\n");
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
