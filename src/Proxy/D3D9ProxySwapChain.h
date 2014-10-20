#pragma once
#include "D3D9ProxySurface.h"
#include "D3DProxyDevice.h"
#include <cBase.h>
#include <vector>

class D3D9ProxySwapChain : public cBase<IDirect3DSwapChain9>{
public:
	D3D9ProxySwapChain(IDirect3DSwapChain9* pActualSwapChain, D3DProxyDevice* pWrappedOwningDevice, bool isAdditionalChain);
	~D3D9ProxySwapChain();	
	
	HRESULT WINAPI Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags);
    HRESULT WINAPI GetFrontBufferData(IDirect3DSurface9* pDestSurface);
    HRESULT WINAPI GetBackBuffer(UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer);
    HRESULT WINAPI GetRasterStatus(D3DRASTER_STATUS* pRasterStatus);
    HRESULT WINAPI GetDisplayMode(D3DDISPLAYMODE* pMode);
    HRESULT WINAPI GetPresentParameters(D3DPRESENT_PARAMETERS* pPresentationParameters);


	bool                                isAdditionalChain;
	std::vector<cPtr<D3D9ProxySurface>> backBuffers;
};
