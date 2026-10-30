#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "multiplayer.h"
#include <cstdint>

void multiplayer::detail::installAt(const std::string& hash, unsigned char* context) {
    validate(hash,context);
    auto site=context+patchOffset;
    DWORD prior{};
    if(!VirtualProtect(site,replacement.size(),PAGE_EXECUTE_READWRITE,&prior))
        throw std::runtime_error("Cannot make multiplayer callsite writable.");
    std::memcpy(site,replacement.data(),replacement.size());
    const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,replacement.size())!=FALSE;
    DWORD ignored{};
    const bool restored=VirtualProtect(site,replacement.size(),prior,&ignored)!=FALSE;
    if(!flushed || !restored)
        throw std::runtime_error("Cannot finalize multiplayer callsite; startup must stop.");
}

void multiplayer::install(const std::string& hash) {
    const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if(base!=0x400000)throw std::runtime_error("Unexpected multiplayer image base.");
    detail::installAt(hash,reinterpret_cast<unsigned char*>(base+contextRva));
}
