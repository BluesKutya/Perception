#pragma once
#include "D3DProxyDevice.h"
#include "cBase.h"
#include "cShader.h"


class D3D9ProxyVertexShader : public cBase<IDirect3DVertexShader9> , public cShader{
public:	
	D3D9ProxyVertexShader( IDirect3DVertexShader9* pActualVertexShader , D3DProxyDevice* pOwningDevice );

	// IDirect3DVertexShader9 methods
	HRESULT WINAPI GetFunction(void *pDate, UINT *pSizeOfData);

	bool m_bSquishViewport;
};
