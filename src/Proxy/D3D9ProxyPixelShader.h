#pragma once
#include "D3DProxyDevice.h"
#include "cBase.h"
#include "cShader.h"

class D3DProxyDevice;


class D3D9ProxyPixelShader : public cBase<IDirect3DPixelShader9> , public cShader {
public:	
	D3D9ProxyPixelShader ( IDirect3DPixelShader9* pActualPixelShader , D3DProxyDevice* pOwningDevice );
	~D3D9ProxyPixelShader( );

	// IDirect3DPixelShader9 methods
	HRESULT WINAPI GetFunction(void *pDate, UINT *pSizeOfData);
};