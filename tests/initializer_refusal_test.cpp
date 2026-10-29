// Load the actual DLL into an owned, unsupported executable at the game's base.
// Initialize must reject the hash before touching any game-specific address.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdint>
int wmain(int argc,wchar_t** argv) {
    if(argc!=2 || reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))!=0x400000) {
        std::fprintf(stderr,"Fixture requires one DLL path and image base400000\n");
        return 1;
    }
    auto module=LoadLibraryW(argv[1]);
    if(!module)return 2;
    using InitializeFn=DWORD(WINAPI*)(void*);
    auto initialize=reinterpret_cast<InitializeFn>(GetProcAddress(module,"_Initialize@4"));
    if(!initialize){FreeLibrary(module);return 3;}
    const DWORD result=initialize(const_cast<wchar_t*>(L"unsupported-fixture-no-ini"));
    FreeLibrary(module);
    if(result!=3) {
        std::fprintf(stderr,"Unsupported executable returned%lu; expected3\n",result);
        return 4;
    }
    std::puts("Actual DLL rejected unsupported executable hash with initializer3");
    return 0;
}
