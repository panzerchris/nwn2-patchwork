#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "multiplayer.h"
#include <cstdlib>
#include <vector>
static void check(bool ok){if(!ok)std::abort();}

// Exercise the actual installer on owned executable memory, including refusal
// before writes and restoration of the original page protection. No game needed.
int main() {
    SYSTEM_INFO info{};GetSystemInfo(&info);
    const size_t size=info.dwPageSize;
    auto page=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(page!=nullptr);
    auto context=page+32;
    auto reset=[&](const std::vector<unsigned char>& bytes) {
        DWORD prior{};
        check(VirtualProtect(page,size,PAGE_READWRITE,&prior)!=FALSE);
        std::memcpy(page,bytes.data(),size);
        check(VirtualProtect(page,size,PAGE_EXECUTE_READ,&prior)!=FALSE);
    };
    auto protectedPage=[&] {
        MEMORY_BASIC_INFORMATION memory{};
        check(VirtualQuery(page,&memory,sizeof(memory))==sizeof(memory));
        check(memory.Protect==PAGE_EXECUTE_READ);
    };
    std::vector<unsigned char> fixture(size,0xcc);
    std::memcpy(fixture.data()+32,multiplayer::expected.data(),multiplayer::expected.size());
    auto refuses=[&](const std::string& hash,const std::vector<unsigned char>& bytes) {
        reset(bytes);
        bool refused=false;
        try{multiplayer::detail::installAt(hash,context);}catch(const std::exception&){refused=true;}
        check(refused);
        check(std::memcmp(page,bytes.data(),size)==0);
        protectedPage();
    };
    for(const auto* hash:{"5bfcdac2cdcdbb460eb7995a7b0521e9a2398bee3918f9589c4e708f385d091e",
                         "bd599aa8cb680e44516e18a8847f0edd201a331227af94baf20fc5f74ff605ae"}) {
        for(size_t i=0;i<multiplayer::expected.size();++i) {
            auto changed=fixture;changed[32+i]^=1;
            refuses(hash,changed);
        }
        reset(fixture);
        multiplayer::detail::installAt(hash,context);
        auto patched=fixture;
        std::memcpy(patched.data()+32+multiplayer::patchOffset,
                    multiplayer::replacement.data(),multiplayer::replacement.size());
        check(std::memcmp(page,patched.data(),size)==0);
        protectedPage();
        refuses(hash,patched); // An already modified callsite is refused too.
    }
    refuses(std::string(64,'0'),fixture);
    check(VirtualFree(page,0,MEM_RELEASE)!=FALSE);
}
