#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <map>
#include <string>
#include <stdexcept>
#include <cwchar>

struct DisplayConfig {
    std::map<std::wstring, unsigned> values{
        {L"RevertV2",1}, {L"GuardEmptyRestore",1}, {L"NativeSizing",1},
        {L"SyncWindowedMode",1}, {L"PreservePlacement",1},
        {L"ThrottleLostDevice",1}, {L"FixMinimapHeight",1},
        {L"FixGraphicsControls",1}, {L"FitWindowedApply",1},
        {L"BorderlessAltEnter",0}, {L"MinClientWidth",800}, {L"MinClientHeight",600}
    };
    unsigned get(const wchar_t* key) const { return values.at(key); }
    void validate() const {
        if(get(L"FitWindowedApply") && (!get(L"NativeSizing") || !get(L"SyncWindowedMode")))
            throw std::runtime_error("FitWindowedApply=1 requires NativeSizing=1 and SyncWindowedMode=1.");
        if(get(L"BorderlessAltEnter") && (!get(L"NativeSizing") || !get(L"SyncWindowedMode")))
            throw std::runtime_error("BorderlessAltEnter=1 requires NativeSizing=1 and SyncWindowedMode=1.");
        const auto width=get(L"MinClientWidth"),height=get(L"MinClientHeight");
        if(width>16384 || height>16384 || ((width==0)!=(height==0)))
            throw std::runtime_error("Minimum client dimensions must both be zero, or both be 1..16384.");
    }
    static DisplayConfig read(const std::wstring& path) {
        if(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)
            throw std::runtime_error("Missing nwn2-patchwork.ini. Extract the complete release ZIP.");
        wchar_t sections[4096]{};
        const DWORD size=GetPrivateProfileSectionNamesW(sections,4096,path.c_str());
        if(size==0 || size>=4094)throw std::runtime_error("Configuration must contain a [Display] section.");
        for(const wchar_t* section=sections;*section;section+=wcslen(section)+1)
            if(_wcsicmp(section,L"Display"))throw std::runtime_error("Unknown configuration section; expected [Display].");
        wchar_t entries[16384]{};
        if(GetPrivateProfileSectionW(L"Display",entries,16384,path.c_str())>=16382)
            throw std::runtime_error("Display configuration is too large.");
        DisplayConfig config;
        std::map<std::wstring,bool> seen;
        for(const wchar_t* entry=entries;*entry;entry+=wcslen(entry)+1) {
            const wchar_t* equals=wcschr(entry,L'=');
            if(!equals)throw std::runtime_error("Expected Key=Value in [Display].");
            const std::wstring name(entry,equals);
            auto found=config.values.end();
            for(auto it=config.values.begin();it!=config.values.end();++it)
                if(!_wcsicmp(it->first.c_str(),name.c_str())){found=it;break;}
            if(found==config.values.end())throw std::runtime_error("Unknown display option; check nwn2-patchwork.ini against docs/configuration.md.");
            if(!seen.emplace(found->first,true).second)throw std::runtime_error("Duplicate display option.");
            std::wstring value(equals+1);
            if(value.empty() || value.find_first_not_of(L"0123456789")!=std::wstring::npos || value.size()>5)
                throw std::runtime_error("Display values must be decimal integers; use 0 or 1 for switches.");
            const unsigned number=static_cast<unsigned>(std::stoul(value));
            const bool dimension=found->first==L"MinClientWidth" || found->first==L"MinClientHeight";
            if(!dimension && number>1)throw std::runtime_error("Display switches must be 0 or 1.");
            found->second=number;
        }
        config.validate();return config;
    }
    std::wstring runtimeText(const std::wstring& log) const {
        // Preserve the tested DLL's internal ABI/schema; public settings use [Display].
        std::wstring result=L"[Experiment]\r\n";
        for(const auto& setting:values)
            result+=setting.first+L"="+std::to_wstring(setting.second)+L"\r\n";
        result+=L"ModelessMove=0\r\nModelessSizing=0\r\nDeferSizingReset=0\r\nBoundMoveSizePump=0\r\n"
                L"Diagnostics=0\r\nUniformSmallUi=0\r\nUniformSmallUiAnchors=0\r\nBorderlessHotkey=0\r\nLog=";
        result+=log+L"\r\n";return result;
    }
};
