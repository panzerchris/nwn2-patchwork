#pragma once
#include "config.h"
#include <bcrypt.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace patchwork {
namespace fs=std::filesystem;
inline fs::path modulePath(HMODULE module) {
    wchar_t path[MAX_PATH]{};
    const DWORD size=GetModuleFileNameW(module,path,MAX_PATH);
    if(size==0 || size>=MAX_PATH)throw std::runtime_error("Module path exceeds the supported path limit.");
    return path;
}
inline std::string hashFile(const fs::path& path) {
    std::ifstream file(path,std::ios::binary);
    if(!file)throw std::runtime_error("Cannot read the game executable.");
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    unsigned char digest[32]{};char buffer[65536];
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    while(ok && file) {
        file.read(buffer,sizeof(buffer));
        ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer),static_cast<ULONG>(file.gcount()),0)>=0;
    }
    if(file.bad())ok=false;
    if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash)BCryptDestroyHash(hash);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    if(!ok)throw std::runtime_error("Cannot compute executable SHA-256.");
    std::ostringstream text;
    for(auto value:digest)text<<std::hex<<std::setfill('0')<<std::setw(2)<<static_cast<unsigned>(value);
    return text.str();
}
inline fs::path createRuntimeConfig(const DisplayConfig& config) {
    PWSTR local{};
    if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&local)))
        throw std::runtime_error("Cannot locate Local AppData for the startup log.");
    const fs::path base=fs::path(local)/L"NWN2 Patchwork"/L"logs";
    CoTaskMemFree(local);
    SYSTEMTIME now{};GetSystemTime(&now);wchar_t name[96]{};
    swprintf_s(name,L"%04u%02u%02u-%02u%02u%02u-%lu-%llu",now.wYear,now.wMonth,now.wDay,
        now.wHour,now.wMinute,now.wSecond,GetCurrentProcessId(),GetTickCount64());
    fs::create_directories(base);
    const fs::path session=base/name;
    if(!fs::create_directory(session))throw std::runtime_error("Cannot create a unique startup log folder.");
    const fs::path ini=session/L"runtime.ini",log=session/L"patchwork.jsonl";
    if(ini.wstring().size()>=MAX_PATH || log.wstring().size()>=MAX_PATH)
        throw std::runtime_error("Local AppData path exceeds the supported 259-character path limit.");
    const auto content=config.runtimeText(log.wstring());
    std::ofstream output(ini,std::ios::binary);
    const unsigned short bom=0xfeff;
    output.write(reinterpret_cast<const char*>(&bom),sizeof(bom));
    output.write(reinterpret_cast<const char*>(content.data()),static_cast<std::streamsize>(content.size()*sizeof(wchar_t)));
    output.close();
    if(!output)throw std::runtime_error("Cannot write the runtime configuration.");
    return ini;
}
}
