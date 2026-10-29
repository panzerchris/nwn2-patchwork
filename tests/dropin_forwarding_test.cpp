// Exercise the actual drop-in DLL and real Windows DirectInput, without a game.
// No registry exports are called and no device is enumerated or acquired.
#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <string>
using CreateFn=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
static void check(bool value,const char* message){if(!value){std::fprintf(stderr,"%s\n",message);std::abort();}}
int wmain(int argc,wchar_t** argv) {
    check(argc==2,"Expected drop-in DLL path");
    HMODULE proxy=LoadLibraryW(argv[1]);
    check(proxy!=nullptr,"Proxy must load first");
    unsigned ordinal=0;
    for(const char* name:{"DirectInput8Create","DllCanUnloadNow","DllGetClassObject","DllRegisterServer","DllUnregisterServer","GetdfDIJoystick"}) {
        auto function=GetProcAddress(proxy,name);
        check(function!=nullptr,"Missing forwarded export");
        check(function==GetProcAddress(proxy,MAKEINTRESOURCEA(++ordinal)),"Export ordinal differs from system ABI");
    }
    auto forwarded=reinterpret_cast<CreateFn>(GetProcAddress(proxy,"DirectInput8Create"));
    LPVOID first=nullptr;
    const HRESULT firstResult=forwarded(GetModuleHandleW(nullptr),0,IID_IDirectInput8W,&first,nullptr);
    check(FAILED(firstResult) && first==nullptr,"Initial invalid-version call must return, not recurse");
    wchar_t directory[MAX_PATH]{};
    check(GetSystemDirectoryW(directory,MAX_PATH)>0,"System directory must be available");
    std::wstring path=std::wstring(directory)+L"\\dinput8.dll";
    HMODULE system=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    check(system && system!=proxy,"Distinct system and proxy modules must load");
    auto direct=reinterpret_cast<CreateFn>(GetProcAddress(system,"DirectInput8Create"));
    for(DWORD version:{DWORD(0),DWORD(0x0800),DWORD(0x0900),DWORD(0x0800)}) {
        LPVOID expected=nullptr,actual=nullptr;
        SetLastError(1234);
        const HRESULT a=direct(GetModuleHandleW(nullptr),version,IID_IDirectInput8W,&expected,nullptr);
        const DWORD expectedError=GetLastError();
        SetLastError(1234);
        const HRESULT b=forwarded(GetModuleHandleW(nullptr),version,IID_IDirectInput8W,&actual,nullptr);
        check(a==b,"HRESULT differs from Windows DirectInput");
        check(expectedError==GetLastError(),"Last-error behavior differs from Windows DirectInput");
        check((expected==nullptr)==(actual==nullptr),"Interface result differs from Windows DirectInput");
        if(expected)static_cast<IDirectInput8W*>(expected)->Release();
        if(actual)static_cast<IDirectInput8W*>(actual)->Release();
    }
    using FormatFn=LPCDIDATAFORMAT(WINAPI*)();
    const auto directFormat=reinterpret_cast<FormatFn>(GetProcAddress(system,"GetdfDIJoystick"));
    const auto forwardedFormat=reinterpret_cast<FormatFn>(GetProcAddress(proxy,"GetdfDIJoystick"));
    check(directFormat()==forwardedFormat(),"Joystick data format was not forwarded to the real module");
    FreeLibrary(proxy);FreeLibrary(system);
    std::puts("Six exports/ordinals match; invalid/valid/repeated creation forwards HRESULT, interface and last-error behavior. Non-game host left unpatched.");
}
