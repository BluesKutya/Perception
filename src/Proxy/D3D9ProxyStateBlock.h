#pragma once
#include <Vireio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <qmap.h>
#include <qset.h>
#include <map>

#include "D3DProxyDevice.h"
#include "D3D9ProxyVertexBuffer.h"
#include "D3D9ProxyIndexBuffer.h"
#include "D3D9ProxyPixelShader.h"
#include "D3D9ProxyVertexShader.h"
#include "D3D9ProxyVertexDeclaration.h"

class BaseDirect3DStateBlock9;
class D3DProxyDevice;
class D3D9ProxyPixelShader;
class D3D9ProxyVertexShader;
class D3D9ProxyIndexBuffer;
class D3D9ProxyVertexDeclaration;

class D3D9ProxyStateBlock : public cBase<IDirect3DStateBlock9> {
public: 

	D3D9ProxyStateBlock( IDirect3DStateBlock9* pActualStateBlock, D3DProxyDevice* pOwningDevice );
	void init();

	// IDirect3DStateBlock9 methods
	HRESULT WINAPI Capture();
	HRESULT WINAPI Apply();


	int                                  type;
	bool                                 selectAuto;
	
	bool                                 selectIndexBuffer;
	bool                                 selectViewport;
	bool                                 selectViewTransform;
	bool                                 selectProjTransform;
	bool                                 selectPixelShader;
	bool                                 selectVertexShader;
	bool                                 selectVertexDeclaration;

	std::map< int , D3DXVECTOR4 >        storedVsConstants;
	std::map< int , D3DXVECTOR4 >        storedPsConstants;

	std::map< int , cPtr<IDirect3DBaseTexture9> > storedTextures;
	std::map< int , cPtr<D3D9ProxyVertexBuffer> > storedVertexes;
	cPtr<D3D9ProxyIndexBuffer>                    storedIndexBuffer;
	cPtr<D3D9ProxyVertexShader>                   storedVertexShader;
	cPtr<D3D9ProxyPixelShader>                    storedPixelShader;
	cPtr<D3D9ProxyVertexDeclaration>              storedVertexDeclaration;
	D3DVIEWPORT9                                  storedViewport;

	bool                                          storedViewSet;
	D3DXMATRIX                                    storedViewOriginal;
	D3DXMATRIX                                    storedViewLeft;
	D3DXMATRIX                                    storedViewRight;

	bool                                          storedProjSet;
	D3DXMATRIX                                    storedProjLeft;
	D3DXMATRIX                                    storedProjRight;
	D3DXMATRIX                                    storedProjOriginal;

	void captureSelected( );
	void captureIndexBuffer           ( D3D9ProxyIndexBuffer* ib );
	void captureViewport              ( D3DVIEWPORT9 viewport );
	void captureViewTransform         ( );
	void captureProjTransform         ( );
	void captureVertexShader          ( D3D9ProxyVertexShader* shader );
	void capturePixelShader           ( D3D9ProxyPixelShader* shader );
	void captureVertexDeclaration     ( D3D9ProxyVertexDeclaration* decl );
	void captureTextureSampler        ( int stage , IDirect3DBaseTexture9* texture );
	void captureVertexStream          ( int stream , D3D9ProxyVertexBuffer* buffer );
	void captureVertexShaderConstant  ( int index , const float* data , int count );
	void capturePixelShaderConstant   ( int index , const float* data , int count );
};
