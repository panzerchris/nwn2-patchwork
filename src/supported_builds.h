#pragma once
#include <string>
inline const char* supportedBuild(const std::string& hash) {
    if(hash=="5bfcdac2cdcdbb460eb7995a7b0521e9a2398bee3918f9589c4e708f385d091e")return "GOG 1.23";
    if(hash=="bd599aa8cb680e44516e18a8847f0edd201a331227af94baf20fc5f74ff605ae")return "GOG 1.23 v2";
    return nullptr;
}
