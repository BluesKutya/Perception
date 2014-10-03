#pragma once
#include <Vireio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <qmap.h>
#include <qset.h>

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

	D3D9ProxyStateBlock( IDirect3DStateBlock9* pActualStateBlock, D3DProxyDevice* pOwningDevice);

	// IDirect3DStateBlock9 methods
	HRESULT WINAPI Capture();
	HRESULT WINAPI Apply();


	int   type;
	bool  sideLeft;
	bool  sideRight;
	bool  selectAuto;
	bool  selectIndexBuffer;
	bool  selectViewport;
	bool  selectViewTransform;
	bool  selectProjTransform;
	bool  selectPixelShader;
	bool  selectVertexShader;
	bool  selectVertexDeclaration;

	
	QMap< int , ComPtr<IDirect3DBaseTexture9> > storedTextureStages;
	QMap< int , ComPtr<D3D9ProxyVertexBuffer> > storedVertexStreams;
	QMap< int , D3DXVECTOR4 >                   storedVsConstants;
	QMap< int , D3DXVECTOR4 >                   storedPsConstants;

	ComPtr<D3D9ProxyIndexBuffer>                  storedIndexBuffer;
	ComPtr<D3D9ProxyVertexShader>                 storedVertexShader;
	ComPtr<D3D9ProxyPixelShader>                  storedPixelShader;
	ComPtr<D3D9ProxyVertexDeclaration>            storedVertexDeclaration;
	D3DVIEWPORT9                                  storedViewport;
	D3DXMATRIX                                    storedLeftView;
	D3DXMATRIX                                    storedRightView;
	D3DXMATRIX                                    storedLeftProjection;
	D3DXMATRIX                                    storedRightProjection;


	void init();

	void captureSelected( );
	void captureIndexBuffer           ( D3D9ProxyIndexBuffer* ib );
	void captureViewport              ( D3DVIEWPORT9 viewport );
	void captureViewTransform         ( D3DXMATRIX left , D3DXMATRIX right );
	void captureProjTransform         ( D3DXMATRIX left , D3DXMATRIX right );
	void captureVertexShader          ( ComPtr<D3D9ProxyVertexShader> shader );
	void capturePixelShader           ( ComPtr<D3D9ProxyPixelShader> shader );
	void captureVertexDeclaration     ( ComPtr<D3D9ProxyVertexDeclaration> decl );
	void captureTextureSampler        ( int stage , ComPtr<IDirect3DBaseTexture9> texture );
	void captureVertexStream          ( int stream , ComPtr<D3D9ProxyVertexBuffer> buffer );
	void captureVertexShaderConstant  ( int index , const float* data , int count );
	void capturePixelShaderConstant   ( int index , const float* data , int count );
	
	void updateCaptureSideTracking();


};
