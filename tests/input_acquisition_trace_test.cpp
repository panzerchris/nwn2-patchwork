// ABI/snapshot test for the actual tracing thunks, without game or COM calls.
#include "probe.cpp"
#include <cstdlib>

static __declspec(naked) void fixtureExit() {
    __asm {
        pop esi
        pop ebp
        pop ebx
        pop ecx
        ret 4
    }
}
static __declspec(naked) void fixtureBody() {
    __asm {
        // Same saved-register frame as the verified native function. Synthetic
        // result registers deliberately differ from the return-register value.
        push ecx
        push ebx
        push ebp
        push esi
        mov esi, ecx
        mov ebp, 80070005h
        mov ebx, 1
        mov eax, 12345678h
        jmp inputAcquisitionExitHook
    }
}
int main() {
    wchar_t directory[MAX_PATH],path[MAX_PATH];
    if(!GetTempPathW(MAX_PATH,directory) ||
       !GetTempFileNameW(directory,L"n2i",0,path))return EXIT_FAILURE;
    logFile=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,nullptr);
    if(logFile==INVALID_HANDLE_VALUE){DeleteFileW(path);return EXIT_FAILURE;}
    alignas(void*) unsigned char owner[0x28]{};
    *reinterpret_cast<unsigned*>(owner+0x14)=0x11223344;
    *reinterpret_cast<unsigned*>(owner+0x18)=0x55667788;
    inputAcquisitionContinue=reinterpret_cast<void*>(fixtureBody);
    inputAcquisitionExitContinue=reinterpret_cast<void*>(fixtureExit);
    using RequestFn=unsigned(__thiscall*)(void*,unsigned);
    SetLastError(1234);
    const unsigned result=reinterpret_cast<RequestFn>(inputAcquisitionHook)(owner,1);
    const bool abiOk=result==0x12345678 && GetLastError()==1234 &&
        field<unsigned>(owner,4)==0;
    SetFilePointer(logFile,0,nullptr,FILE_BEGIN);
    char output[2048]{};DWORD read;
    const bool readOk=ReadFile(logFile,output,sizeof(output)-1,&read,nullptr)!=FALSE;
    CloseHandle(logFile);logFile=INVALID_HANDLE_VALUE;DeleteFileW(path);
    const bool traceOk=readOk && strstr(output,"input_acquisition_enter") &&
        strstr(output,"input_acquisition_leave") &&
        strstr(output,"\"request\":1,\"state\":0") &&
        strstr(output,"\"keyboard\":287454020,\"mouse\":1432778632") &&
        strstr(output,"\"keyboard_result_normalized\":2147942405,\"mouse_result_normalized\":1");
    if(!abiOk || !traceOk)std::fprintf(stderr,"Input trace ABI/snapshot failed: %s\n",output);
    return abiOk && traceOk?EXIT_SUCCESS:EXIT_FAILURE;
}
