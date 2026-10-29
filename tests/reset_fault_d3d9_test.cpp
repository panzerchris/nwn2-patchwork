// Opt-in hardware integration calibration. Uses an owned hidden window/device,
// not NWN2. Not a CTest: requires a functioning interactive Direct3D9 adapter.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include "reset_fault.h"

static D3DPRESENT_PARAMETERS parameters(HWND window) {
    D3DPRESENT_PARAMETERS value{};
    value.BackBufferWidth=320;
    value.BackBufferHeight=240;
    value.BackBufferFormat=D3DFMT_UNKNOWN;
    value.BackBufferCount=1;
    value.SwapEffect=D3DSWAPEFFECT_DISCARD;
    value.hDeviceWindow=window;
    value.Windowed=TRUE;
    value.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    return value;
}
int main() {
    HWND window=CreateWindowExW(0,L"STATIC",L"NWN2 reset fault calibration",
        WS_OVERLAPPEDWINDOW,32,32,336,279,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window)return 1;
    IDirect3D9* d3d=Direct3DCreate9(D3D_SDK_VERSION);
    IDirect3DDevice9* device=nullptr;
    auto pp=parameters(window);
    HRESULT created=d3d?d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,&device):E_FAIL;
    std::printf("create=%08lx\n",static_cast<unsigned long>(created));
    bool ok=SUCCEEDED(created) && device;
    if(ok) {
        HeldResetBackbuffer held;
        HRESULT acquired=held.acquire(device);
        pp=parameters(window);
        HRESULT failed=held.active()?device->Reset(&pp):E_UNEXPECTED;
        HRESULT cooperative=device->TestCooperativeLevel();
        std::printf("acquire=%08lx reset_held=%08lx cooperative_after_failure=%08lx\n",
            static_cast<unsigned long>(acquired),static_cast<unsigned long>(failed),
            static_cast<unsigned long>(cooperative));
        held.release();
        pp=parameters(window);
        HRESULT recovered=device->Reset(&pp);
        HRESULT ready=device->TestCooperativeLevel();
        // After failed Reset, only Reset/TestCooperativeLevel/Release are valid.
        // Do not render until the real driver has accepted the second Reset.
        HRESULT cleared=E_UNEXPECTED,presented=E_UNEXPECTED;
        if(SUCCEEDED(recovered) && SUCCEEDED(ready)) {
            cleared=device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff206040,1.0f,0);
            presented=device->Present(nullptr,nullptr,nullptr,nullptr);
        }
        std::printf("reset_released=%08lx cooperative_ready=%08lx clear=%08lx present=%08lx\n",
            static_cast<unsigned long>(recovered),static_cast<unsigned long>(ready),
            static_cast<unsigned long>(cleared),static_cast<unsigned long>(presented));
        ok=SUCCEEDED(acquired) && failed==D3DERR_INVALIDCALL &&
            (cooperative==D3DERR_DEVICELOST || cooperative==D3DERR_DEVICENOTRESET) &&
            SUCCEEDED(recovered) && SUCCEEDED(ready) && SUCCEEDED(cleared) && SUCCEEDED(presented);
    }
    if(device)device->Release();
    if(d3d)d3d->Release();
    DestroyWindow(window);
    return ok?0:1;
}
