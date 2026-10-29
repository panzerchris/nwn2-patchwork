// DirectInput forwarding module. No work is done in DllMain; the tested runtime
// initializes once on the game's ordinary DirectInput8Create call.
#include "runtime_support.h"
#include "supported_builds.h"
#include "multiplayer.h"
#include <unknwn.h>
#include <intrin.h>
#include <atomic>
#include <cstdint>

extern "C" DWORD WINAPI Initialize(void*);
using CreateInputFn=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
using NoArgumentFn=HRESULT(WINAPI*)();
using ClassObjectFn=HRESULT(WINAPI*)(REFCLSID,REFIID,LPVOID*);
using DataFormatFn=const void*(WINAPI*)();
static INIT_ONCE systemOnce=INIT_ONCE_STATIC_INIT,patchworkOnce=INIT_ONCE_STATIC_INIT;
static HMODULE systemInput;
static CreateInputFn systemCreate;
static NoArgumentFn systemCanUnload,systemRegister,systemUnregister;
static ClassObjectFn systemClassObject;
static DataFormatFn systemJoystickFormat;
static DWORD systemError;
static std::atomic<bool> initialized{false};

static BOOL CALLBACK loadSystemInput(PINIT_ONCE,PVOID,PVOID*) {
    // Use a full system path: a basename-only lookup can find this already-loaded
    // application-local module before any directory-search flags are considered.
    wchar_t directory[MAX_PATH]{};
    const UINT length=GetSystemDirectoryW(directory,MAX_PATH);
    if(length==0 || length>=MAX_PATH){systemError=ERROR_INSUFFICIENT_BUFFER;return TRUE;}
    const auto path=patchwork::fs::path(directory)/L"dinput8.dll";
    systemInput=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!systemInput){systemError=GetLastError();return TRUE;}
    systemCreate=reinterpret_cast<CreateInputFn>(GetProcAddress(systemInput,"DirectInput8Create"));
    systemCanUnload=reinterpret_cast<NoArgumentFn>(GetProcAddress(systemInput,"DllCanUnloadNow"));
    systemClassObject=reinterpret_cast<ClassObjectFn>(GetProcAddress(systemInput,"DllGetClassObject"));
    systemRegister=reinterpret_cast<NoArgumentFn>(GetProcAddress(systemInput,"DllRegisterServer"));
    systemUnregister=reinterpret_cast<NoArgumentFn>(GetProcAddress(systemInput,"DllUnregisterServer"));
    systemJoystickFormat=reinterpret_cast<DataFormatFn>(GetProcAddress(systemInput,"GetdfDIJoystick"));
    if(!systemCreate || !systemCanUnload || !systemClassObject || !systemRegister || !systemUnregister || !systemJoystickFormat)
        systemError=ERROR_PROC_NOT_FOUND;
    return TRUE;
}
static bool ensureSystemInput() {
    if(!InitOnceExecuteOnce(&systemOnce,loadSystemInput,nullptr,nullptr))return false;
    if(systemError){SetLastError(systemError);return false;}
    return true;
}
static bool targetProcess() {
    const auto executable=patchwork::modulePath(nullptr);
    // Other executables in the installation may also import DirectInput.
    return !_wcsicmp(executable.filename().c_str(),L"nwn2main.exe");
}
static BOOL CALLBACK initializePatchwork(PINIT_ONCE,PVOID argument,PVOID*) {
    try {
        const auto executable=patchwork::modulePath(nullptr);
        const auto hash=patchwork::hashFile(executable);
        if(!supportedBuild(hash))throw std::runtime_error("Unsupported nwn2main.exe SHA-256: "+hash);
        HMODULE self{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&initializePatchwork),&self))
            throw std::runtime_error("Cannot locate the Patchwork module.");
        const auto home=patchwork::modulePath(self).parent_path();
        const auto config=DisplayConfig::read((home/L"nwn2-patchwork"/L"nwn2-patchwork.ini").wstring());
        if(hash=="bd599aa8cb680e44516e18a8847f0edd201a331227af94baf20fc5f74ff605ae" && !config.get(L"RevertV2"))
            throw std::runtime_error("RevertV2=1 is required for GOG 1.23 v2.");
        for(const wchar_t* name:{L"nwn2fixes.dll",L"dxwrapper.dll",L"ClientExtension.hdl",L"ClientExtension.dll",
                L"d3d9.dll",L"dsound.dll",L"ddraw.dll",L"winmm.dll",L"version.dll"})
            if(patchwork::fs::exists(home/name))
                throw std::runtime_error("Another runtime patch/proxy is present. See Patchwork's compatibility documentation.");
        if(config.skipLegacyLoginCheck && reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))!=0x400000)
            throw std::runtime_error("Unexpected multiplayer image base.");
        if(config.skipLegacyLoginCheck)
            multiplayer::validate(hash,reinterpret_cast<const void*>(0x400000+multiplayer::contextRva));
        const auto runtime=patchwork::createRuntimeConfig(config);
        std::ofstream receipt(runtime.parent_path()/L"bootstrap.json",std::ios::binary);
        receipt<<"{\"method\":\"DirectInput8Create\",\"caller\":\"0x"<<std::hex
            <<reinterpret_cast<uintptr_t>(argument)<<"\",\"pid\":"<<std::dec<<GetCurrentProcessId()
            <<",\"thread\":"<<GetCurrentThreadId()<<",\"exe_sha256\":\""<<hash
            <<"\",\"skip_legacy_login_check\":"<<(config.skipLegacyLoginCheck?"true":"false")<<"}\n";
        receipt.close();
        if(!receipt)throw std::runtime_error("Cannot write the bootstrap receipt.");
        const DWORD result=Initialize(const_cast<wchar_t*>(runtime.c_str()));
        if(result!=1)throw std::runtime_error("Patchwork initialization refused, code "+std::to_string(result));
        if(config.skipLegacyLoginCheck)multiplayer::install(hash);
        initialized.store(true,std::memory_order_release);
    } catch(const std::exception& error) {
        // Initialization can have installed some hooks before a write failure.
        // Do not continue a newly starting game with partial patches.
        MessageBoxA(nullptr,error.what(),"NWN2 Patchwork could not start",MB_OK|MB_ICONERROR);
        ExitProcess(ERROR_DLL_INIT_FAILED);
    }
    return TRUE;
}
extern "C" HRESULT WINAPI PatchworkDirectInput8Create(HINSTANCE instance,DWORD version,
        REFIID iid,LPVOID* output,LPUNKNOWN outer) {
    const DWORD incomingError=GetLastError();
    if(!ensureSystemInput())return HRESULT_FROM_WIN32(GetLastError());
    try {
        if(targetProcess() && !InitOnceExecuteOnce(&patchworkOnce,initializePatchwork,_ReturnAddress(),nullptr))
            return E_FAIL;
    } catch(const std::exception& error) {
        MessageBoxA(nullptr,error.what(),"NWN2 Patchwork could not start",MB_OK|MB_ICONERROR);
        ExitProcess(ERROR_DLL_INIT_FAILED);
    }
    SetLastError(incomingError);
    return systemCreate(instance,version,iid,output,outer);
}
extern "C" HRESULT WINAPI PatchworkDllCanUnloadNow() {
    const DWORD incomingError=GetLastError();
    if(!ensureSystemInput())return HRESULT_FROM_WIN32(GetLastError());
    SetLastError(incomingError);
    // The game keeps this import loaded; installed hooks must never outlive it.
    return initialized.load(std::memory_order_acquire)?S_FALSE:systemCanUnload();
}
extern "C" HRESULT WINAPI PatchworkDllGetClassObject(REFCLSID clsid,REFIID iid,LPVOID* output) {
    const DWORD incomingError=GetLastError();
    if(!ensureSystemInput())return HRESULT_FROM_WIN32(GetLastError());
    SetLastError(incomingError);return systemClassObject(clsid,iid,output);
}
extern "C" HRESULT WINAPI PatchworkDllRegisterServer() {
    if(!ensureSystemInput())return HRESULT_FROM_WIN32(GetLastError());
    return systemRegister();
}
extern "C" HRESULT WINAPI PatchworkDllUnregisterServer() {
    if(!ensureSystemInput())return HRESULT_FROM_WIN32(GetLastError());
    return systemUnregister();
}
extern "C" const void* WINAPI PatchworkGetdfDIJoystick() {
    const DWORD incomingError=GetLastError();
    if(!ensureSystemInput())return nullptr;
    SetLastError(incomingError);return systemJoystickFormat();
}
