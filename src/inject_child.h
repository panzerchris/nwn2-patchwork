// Own-child startup machinery carried forward from the tested research launcher.
// It never attaches to an existing game. The child is held before game entry.
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cwchar>
#include <string>
#include <stdexcept>
#include <vector>
static void require(bool ok,const char* what) {
    if(!ok)throw std::runtime_error(std::string(what)+" (Win32 "+std::to_string(GetLastError())+")");
}
static std::wstring absolute(const wchar_t* path) {
    wchar_t full[MAX_PATH];DWORD n=GetFullPathNameW(path,MAX_PATH,full,nullptr);
    require(n>0 && n<MAX_PATH,"absolute path");return full;
}
static void remoteWrite(HANDLE process,void* where,const void* bytes,size_t size) {
    SIZE_T written;require(WriteProcessMemory(process,where,bytes,size,&written)&&written==size,"remote write");
    require(FlushInstructionCache(process,where,size)!=FALSE,"instruction cache");
}
static DWORD invoke(HANDLE process,LPTHREAD_START_ROUTINE fn,void* argument) {
    HANDLE t=CreateRemoteThread(process,nullptr,0,fn,argument,0,nullptr);
    require(t!=nullptr,"remote thread");
    DWORD wait=WaitForSingleObject(t,15000),result=0;
    bool ok=wait==WAIT_OBJECT_0 && GetExitCodeThread(t,&result);
    CloseHandle(t);require(ok,"remote initialization timeout/failure");return result;
}
static void* remoteString(HANDLE process,const std::wstring& value) {
    size_t size=(value.size()+1)*sizeof(wchar_t);
    void* p=VirtualAllocEx(process,nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    require(p!=nullptr,"remote allocation");remoteWrite(process,p,value.c_str(),size);return p;
}
static uintptr_t remoteLoadLibrary(DWORD pid) {
    auto local=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    HMODULE owner{};
    require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCWSTR>(local),&owner)!=FALSE,"LoadLibrary owner");
    wchar_t path[MAX_PATH];require(GetModuleFileNameW(owner,path,MAX_PATH)!=0,"owner path");
    const wchar_t* name=wcsrchr(path,L'\\');name=name?name+1:path;
    HANDLE snap=INVALID_HANDLE_VALUE;
    // The loader's module list can change while the snapshot is copied.
    for(unsigned attempt=0;attempt<50;++attempt) {
        snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);
        if(snap!=INVALID_HANDLE_VALUE || GetLastError()!=ERROR_BAD_LENGTH)break;
        Sleep(10);
    }
    require(snap!=INVALID_HANDLE_VALUE,"module snapshot");
    MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);uintptr_t base=0;
    if(Module32FirstW(snap,&entry))do {
        if(!_wcsicmp(name,entry.szModule)){base=reinterpret_cast<uintptr_t>(entry.modBaseAddr);break;}
    }while(Module32NextW(snap,&entry));
    CloseHandle(snap);require(base!=0,"remote loader module");
    return base+reinterpret_cast<uintptr_t>(local)-reinterpret_cast<uintptr_t>(owner);
}
static void stopAtEntry(PROCESS_INFORMATION& pi) {
    void* entry=nullptr;unsigned char original=0;bool planted=false,ready=false;
    ULONGLONG deadline=GetTickCount64()+30000;
    while(!ready) {
        require(GetTickCount64()<deadline,"startup deadline");
        DEBUG_EVENT e{};
        if(!WaitForDebugEvent(&e,500)){require(GetLastError()==ERROR_SEM_TIMEOUT,"debug event");continue;}
        DWORD disposition=DBG_CONTINUE;
        if(e.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            entry=reinterpret_cast<void*>(e.u.CreateProcessInfo.lpStartAddress);
            SIZE_T read;
            require(ReadProcessMemory(pi.hProcess,entry,&original,1,&read)&&read==1,"entry byte");
            const unsigned char breakpoint=0xcc;remoteWrite(pi.hProcess,entry,&breakpoint,1);planted=true;
            if(e.u.CreateProcessInfo.hFile)CloseHandle(e.u.CreateProcessInfo.hFile);
        } else if(e.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(e.u.LoadDll.hFile)CloseHandle(e.u.LoadDll.hFile);
        } else if(e.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            // Debug API owns this handle and closes it on thread exit/detach.
            // Closing it early permits handle reuse before the API's cleanup.
        } else if(e.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            throw std::runtime_error("Child exited before entry point");
        } else if(e.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            auto& ex=e.u.Exception.ExceptionRecord;
            if(ex.ExceptionCode==EXCEPTION_BREAKPOINT && planted && ex.ExceptionAddress==entry) {
                require(e.dwThreadId==pi.dwThreadId,"entry on primary thread");
                CONTEXT context{};context.ContextFlags=CONTEXT_CONTROL;
                require(GetThreadContext(pi.hThread,&context)!=FALSE,"get entry context");
                remoteWrite(pi.hProcess,entry,&original,1);
                context.Eip=reinterpret_cast<DWORD>(entry);
                require(SetThreadContext(pi.hThread,&context)!=FALSE,"restore entry context");
                require(SuspendThread(pi.hThread)!=static_cast<DWORD>(-1),"hold primary thread");
                ready=true;
            } else if(ex.ExceptionCode!=EXCEPTION_BREAKPOINT) disposition=DBG_EXCEPTION_NOT_HANDLED;
        }
        require(ContinueDebugEvent(e.dwProcessId,e.dwThreadId,disposition)!=FALSE,"continue startup");
    }
    require(DebugActiveProcessStop(pi.dwProcessId)!=FALSE,"detach startup debugger");
}
static int launchOwnedGame(const std::wstring& game,const std::wstring& library,const std::wstring& settings) {
    PROCESS_INFORMATION pi{};bool started=false;
    try {
        auto exe=absolute(game.c_str()),dll=absolute(library.c_str()),ini=absolute(settings.c_str());
        // Refuse a second game session, irrespective of selected vendor variant.
        HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
        require(snap!=INVALID_HANDLE_VALUE,"process snapshot");
        PROCESSENTRY32W p{};p.dwSize=sizeof(p);bool running=false;
        if(Process32FirstW(snap,&p))do {if(!_wcsnicmp(p.szExeFile,L"nwn2main",8))running=true;}while(Process32NextW(snap,&p));
        CloseHandle(snap);require(!running,"existing game process");
        require(GetFileAttributesW(ini.c_str())!=INVALID_FILE_ATTRIBUTES,"runtime configuration exists");
        HMODULE mapped=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
        require(mapped!=nullptr,"map Patchwork DLL exports");
        FARPROC init=GetProcAddress(mapped,"_Initialize@4");
        require(init!=nullptr,"Initialize export");
        uintptr_t initRva=reinterpret_cast<uintptr_t>(init)-reinterpret_cast<uintptr_t>(mapped);
        FreeLibrary(mapped);
        std::wstring command=L"\""+exe+L"\"",directory=exe.substr(0,exe.find_last_of(L"\\/"));
        STARTUPINFOW si{};si.cb=sizeof(si);
        require(CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,DEBUG_ONLY_THIS_PROCESS,
          nullptr,directory.c_str(),&si,&pi)!=FALSE,"create owned child");started=true;
        stopAtEntry(pi);
        void* dllArg=remoteString(pi.hProcess,dll);
        DWORD base=invoke(pi.hProcess,reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteLoadLibrary(pi.dwProcessId)),dllArg);
        require(base!=0,"load Patchwork DLL");VirtualFreeEx(pi.hProcess,dllArg,0,MEM_RELEASE);
        void* iniArg=remoteString(pi.hProcess,ini);
        DWORD result=invoke(pi.hProcess,reinterpret_cast<LPTHREAD_START_ROUTINE>(base+initRva),iniArg);
        VirtualFreeEx(pi.hProcess,iniArg,0,MEM_RELEASE);
        if(result!=1)throw std::runtime_error("Patchwork initialization refused (code "+std::to_string(result)+"). See docs/troubleshooting.md.");
        require(ResumeThread(pi.hThread)!=static_cast<DWORD>(-1),"resume game");
        printf("NWN2 Patchwork started game PID %lu.\n",pi.dwProcessId);
        CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return 0;
    } catch(...) {
        if(started){TerminateProcess(pi.hProcess,1);WaitForSingleObject(pi.hProcess,5000);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}
        throw;
    }
}
