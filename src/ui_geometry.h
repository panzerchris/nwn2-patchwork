#pragma once

// D071: retain the engine's design-space anchoring choices, using saved size
// for classification and live (uniformly scaled) size for final placement.
inline int rootUiPosition(int savedPosition, int savedSize, int liveSize,
                          int viewportSize, int designSize) {
    if(savedPosition<=0)return 0;
    const int farMargin=designSize-savedPosition-savedSize;
    const float ratio=static_cast<float>(viewportSize)/designSize;
    int result;
    if(farMargin==0)result=viewportSize-liveSize;
    else if(savedPosition<farMargin)
        result=static_cast<int>((savedPosition+1)*ratio-1.0f);
    else
        result=static_cast<int>((savedPosition+savedSize*0.5f)*ratio-liveSize*0.5f);
    return result<0 ? 0 : result;
}
