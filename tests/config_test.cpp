#include "config.h"
#include "supported_builds.h"
#include <cstdlib>
#include <fstream>
#include <filesystem>
static void check(bool condition){if(!condition)std::abort();}
template<class Fn> static void refuses(Fn fn) {
    bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}check(rejected);
}
int main() {
    wchar_t temp[MAX_PATH]{},file[MAX_PATH]{};
    check(GetTempPathW(MAX_PATH,temp)>0);
    check(GetTempFileNameW(temp,L"npw",0,file)!=0);
    const std::wstring path=file;
    auto write=[&](const char* content){
        std::ofstream out(std::filesystem::path(path),std::ios::binary|std::ios::trunc);
        out<<content;out.close();check(static_cast<bool>(out));
        WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());
    };
    write("[Display]\nBorderlessAltEnter=1\n");
    auto config=DisplayConfig::read(path);check(config.get(L"BorderlessAltEnter")==1);
    check(config.get(L"NativeSizing")==1);
    check(!config.skipLegacyLoginCheck);
    const auto runtime=config.runtimeText(L"C:\\logs\\example.jsonl");
    check(runtime.find(L"[Experiment]")!=std::wstring::npos);
    check(runtime.find(L"ModelessMove=0")!=std::wstring::npos);
    check(runtime.find(L"Diagnostics=0")!=std::wstring::npos);
    check(runtime.find(L"Log=C:\\logs\\example.jsonl")!=std::wstring::npos);
    for(const char* bad:{"[Display]\nBorderlessAltEnter=1\nNativeSizing=0\nFitWindowedApply=0\n",
            "[Display]\nFitWindowedApply=1\nSyncWindowedMode=0\n",
            "[Display]\nNativeSizing=2\n", "[Display]\nNativeSizing=oops\n",
            "[Display]\nMinClientWidth=0\nMinClientHeight=600\n",
            "[Display]\nMinClientWidth=16385\n", "[Display]\nTypo=1\n",
            "[Audio]\nEnabled=1\n", "not-an-ini\n"}) {
        write(bad);refuses([&]{DisplayConfig::read(path);});
    }
    write("[Display]\nMinClientWidth=0\nMinClientHeight=0\n");
    check(DisplayConfig::read(path).get(L"MinClientWidth")==0);
    write("[Display]\nnativesizing=0\nFitWindowedApply=0\n");
    check(DisplayConfig::read(path).get(L"NativeSizing")==0);
    write("[Display]\n[Multiplayer]\nSkipLegacyLoginCheck=1\n");
    check(DisplayConfig::read(path).skipLegacyLoginCheck);
    write("[Display]\n[Multiplayer]\nSkipLegacyLoginCheck=0\n");
    check(!DisplayConfig::read(path).skipLegacyLoginCheck);
    for(const char* bad:{"[Display]\n[Multiplayer]\nSkipLegacyLoginCheck=2\n",
            "[Display]\n[Multiplayer]\nSkipLegacyLoginCheck=oops\n",
            "[Display]\n[Multiplayer]\nUnknown=1\n",
            "[Multiplayer]\nSkipLegacyLoginCheck=1\n"}) {
        write(bad);refuses([&]{DisplayConfig::read(path);});
    }
    check(DeleteFileW(path.c_str())!=FALSE);
    check(supportedBuild("5bfcdac2cdcdbb460eb7995a7b0521e9a2398bee3918f9589c4e708f385d091e")!=nullptr);
    check(supportedBuild("bd599aa8cb680e44516e18a8847f0edd201a331227af94baf20fc5f74ff605ae")!=nullptr);
    check(supportedBuild(std::string(64,'0'))==nullptr);
    std::puts("Configuration defaults, invalid/dependent options, runtime mapping and exact build hashes checked.");
}
