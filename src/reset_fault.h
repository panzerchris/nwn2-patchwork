#pragma once
#include <d3d9.h>

// Research-only fault: retain one implicit surface across a native Reset, so
// D3D9 itself rejects that Reset. Release before native recovery is allowed to
// run. Never substitute a HRESULT or change the engine's device/lost state.
class HeldResetBackbuffer {
    IDirect3DSurface9* surface_ = nullptr;
public:
    HeldResetBackbuffer() = default;
    HeldResetBackbuffer(const HeldResetBackbuffer&) = delete;
    HeldResetBackbuffer& operator=(const HeldResetBackbuffer&) = delete;
    ~HeldResetBackbuffer() { release(); }
    HRESULT acquire(IDirect3DDevice9* device) {
        if(surface_ || !device)return E_UNEXPECTED;
        return device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&surface_);
    }
    bool active() const { return surface_!=nullptr; }
    void release() {
        if(surface_) {
            auto surface=surface_;
            surface_=nullptr;
            surface->Release();
        }
    }
};
