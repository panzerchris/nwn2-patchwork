#pragma once
#include "supported_builds.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace multiplayer {
// Both supported files have this exact ordinary-login callsite. The preceding
// branch sends new-account requests elsewhere. Operation 2 (server connect) is
// a separate handler. Returning false here takes the native auth-unavailable
// path before an auth request/timer is created; that path owns UI continuation.
inline constexpr size_t contextRva=0x31b60e, patchOffset=2;
inline constexpr std::array<unsigned char,17> expected={
    0x8b,0xce,0xe8,0x8b,0xd3,0xff,0xff,0x84,0xc0,0x8b,0x0d,0x9c,0xe1,0xa0,0x00,0x74,0xde};
inline constexpr std::array<unsigned char,5> replacement={0x32,0xc0,0x90,0x90,0x90};
inline void validate(const std::string& hash,const void* context) {
    if(!supportedBuild(hash))throw std::runtime_error("Unsupported multiplayer executable.");
    if(std::memcmp(context,expected.data(),expected.size()))
        throw std::runtime_error("Multiplayer login callsite does not match expected bytes.");
}
void install(const std::string& hash);
namespace detail {
// Shared by the image-base-checked installer and owned-memory refusal fixtures.
void installAt(const std::string& hash, unsigned char* context);
}
}
