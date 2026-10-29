// Public launcher: validate first, then inject only into the child we create.
#include "inject_child.h"
#include "config.h"
#include "supported_builds.h"
#include <bcrypt.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace fs=std::filesystem;
static std::string fileHash(const fs::path& path) {
    std::ifstream file(path,std::ios::binary);
    if(!file)throw std::runtime_error("Cannot open nwn2main.exe. Extract Patchwork into the game folder, or use --game PATH.");
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
    unsigned char digest[32]{};char buffer[65536];
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    while(ok && file) {
        file.read(buffer,sizeof(buffer));
        ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer),static_cast<ULONG>(file.gcount()),0)>=0;
    }
    if(file.bad())ok=false;
    if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash)BCryptDestroyHash(hash);
    if(alg)BCryptCloseAlgorithmProvider(alg,0);
    if(!ok)throw std::runtime_error("Cannot compute executable SHA-256.");
    std::ostringstream text;
    for(auto value:digest)text<<std::hex<<std::setfill('0')<<std::setw(2)<<static_cast<unsigned>(value);
    return text.str();
}
static void requireNoGame() {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    require(snapshot!=INVALID_HANDLE_VALUE,"process snapshot");
    PROCESSENTRY32W process{};process.dwSize=sizeof(process);bool running=false;
    if(Process32FirstW(snapshot,&process))do {
        if(!_wcsnicmp(process.szExeFile,L"nwn2main",8))running=true;
    }while(Process32NextW(snapshot,&process));
    CloseHandle(snapshot);
    if(running)throw std::runtime_error("Close the existing NWN2 game before starting Patchwork.");
}
static fs::path runtimeConfig(const DisplayConfig& config) {
    PWSTR local{};
    require(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&local)),"local application data");
    fs::path base=fs::path(local)/L"NWN2 Patchwork"/L"logs";
    CoTaskMemFree(local);
    SYSTEMTIME now{};GetSystemTime(&now);wchar_t name[96]{};
    swprintf_s(name,L"%04u%02u%02u-%02u%02u%02u-%lu-%llu",now.wYear,now.wMonth,now.wDay,
        now.wHour,now.wMinute,now.wSecond,GetCurrentProcessId(),GetTickCount64());
    fs::create_directories(base);
    const fs::path session=base/name;
    require(fs::create_directory(session),"create unique log folder");
    const fs::path ini=session/L"runtime.ini",log=session/L"patchwork.jsonl";
    if(ini.wstring().size()>=MAX_PATH || log.wstring().size()>=MAX_PATH)
        throw std::runtime_error("Local application data path exceeds the supported 259-character path limit.");
    const auto content=config.runtimeText(log.wstring());
    std::ofstream output(ini,std::ios::binary);
    const unsigned short bom=0xfeff;
    output.write(reinterpret_cast<const char*>(&bom),sizeof(bom));
    output.write(reinterpret_cast<const char*>(content.data()),static_cast<std::streamsize>(content.size()*sizeof(wchar_t)));
    output.close();
    if(!output)throw std::runtime_error("Could not write the runtime configuration.");
    return ini;
}
int wmain(int argc,wchar_t** argv) {
    const bool interactive=argc==1;
    try {
        wchar_t ownPath[MAX_PATH]{};
        const DWORD length=GetModuleFileNameW(nullptr,ownPath,MAX_PATH);
        require(length>0 && length<MAX_PATH,"launcher path");
        const fs::path home=fs::path(ownPath).parent_path();
        fs::path game=home/L"nwn2main.exe",ini=home/L"nwn2-patchwork.ini";
        if(!fs::is_regular_file(game))game=home.parent_path()/L"nwn2main.exe";
        bool check=false;
        for(int i=1;i<argc;++i) {
            const std::wstring arg(argv[i]);
            if(arg==L"--help" || arg==L"-h") {
                std::puts("NWN2 Patchwork " PATCHWORK_VERSION "\nUsage: nwn2-patchwork.exe [--game PATH_TO_EXE] [--config PATH_TO_INI] [--check]\n"
                          "Extract into the game folder and run. --check validates without starting the game.\n"
                          "Only the two documented GOG 1.23 executable hashes are supported.");return 0;
            }
            if(arg==L"--version"){std::puts("NWN2 Patchwork " PATCHWORK_VERSION);return 0;}
            if(arg==L"--check"){check=true;continue;}
            if((arg==L"--game" || arg==L"--config") && i+1<argc) {
                const fs::path value=fs::absolute(argv[++i]);
                if(arg==L"--game")game=value;else ini=value;
            }else throw std::runtime_error("Unknown or incomplete option. Run with --help.");
        }
        game=fs::absolute(game);ini=fs::absolute(ini);
        if(_wcsicmp(game.filename().c_str(),L"nwn2main.exe"))
            throw std::runtime_error("--game must point to the installed nwn2main.exe, not a renamed copy.");
        const fs::path dll=home/L"nwn2-patchwork.dll";
        if(game.wstring().size()>=MAX_PATH || ini.wstring().size()>=MAX_PATH)
            throw std::runtime_error("Game/configuration path exceeds the supported 259-character path limit.");
        const auto config=DisplayConfig::read(ini.wstring());
        const auto hash=fileHash(game);const char* build=supportedBuild(hash);
        if(!build)throw std::runtime_error("Unsupported nwn2main.exe SHA-256: "+hash+". Only the documented GOG 1.23 and GOG 1.23 v2 builds are supported; the executable has not been modified.");
        if(std::string(build)=="GOG 1.23 v2" && !config.get(L"RevertV2"))
            throw std::runtime_error("RevertV2=1 is required for GOG 1.23 v2.");
        if(!fs::is_regular_file(dll))throw std::runtime_error("Missing nwn2-patchwork.dll. Extract the complete release ZIP.");
        for(const wchar_t* name:{L"nwn2fixes.dll",L"dxwrapper.dll",L"ClientExtension.hdl",L"ClientExtension.dll",
                L"d3d9.dll",L"dinput8.dll",L"dsound.dll",L"ddraw.dll",L"winmm.dll",L"version.dll"})
            if(fs::exists(game.parent_path()/name))
                throw std::runtime_error("Another patch/proxy is present in the game folder. See docs/compatibility.md before combining mods.");
        requireNoGame();
        std::printf("NWN2 Patchwork " PATCHWORK_VERSION ": %s\n",build);
        if(check){std::puts("Preflight passed. No game was started and no runtime files were created.");return 0;}
        const auto generated=runtimeConfig(config);
        return launchOwnedGame(game.wstring(),dll.wstring(),generated.wstring());
    } catch(const std::exception& error) {
        std::fprintf(stderr,"NWN2 Patchwork: %s\n",error.what());
        if(interactive)MessageBoxA(nullptr,error.what(),"NWN2 Patchwork",MB_OK|MB_ICONERROR);
        return 1;
    }
}
