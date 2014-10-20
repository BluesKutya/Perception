#include "D3D9ProxySwapChain.h"
#include "StereoView.h"

D3D9ProxySwapChain::D3D9ProxySwapChain(IDirect3DSwapChain9* pActualSwapChain, D3DProxyDevice* pWrappedOwningDevice, bool is_add ) : 
	cBase( pActualSwapChain , pWrappedOwningDevice )
{

	isAdditionalChain = is_add;

	D3DPRESENT_PARAMETERS params;
	actual->GetPresentParameters( &params );

	UINT backCount = params.BackBufferCount;
	if( backCount == 0 ){
		backCount = 1;
	}

	backBuffers.resize( backCount );

	for( UINT c=0 ; c<backCount ; c++ ){
	
		IDirect3DSurface9* backBuf;
		D3DSURFACE_DESC    backDesc;
		IDirect3DSurface9* backSurf;
		D3D9ProxySurface*  backProxy;

		actual->GetBackBuffer( c , D3DBACKBUFFER_TYPE_MONO , &backBuf );
				
		backBuf->GetDesc(&backDesc);
		
		backBuf->Release();

		device->ProxyCreateRenderTarget( backDesc.Width , backDesc.Height , backDesc.Format , backDesc.MultiSampleType , backDesc.MultiSampleQuality , false , &backSurf , 0 , 0 , true , false );

		backProxy = (D3D9ProxySurface*)backSurf;

		backBuffers[c] = backProxy;

		//ref added in backBuffers
		backProxy->Release();
	}
}


D3D9ProxySwapChain::~D3D9ProxySwapChain(){

}


METHOD_IMPL( HRESULT , WINAPI , D3D9ProxySwapChain , Present , CONST RECT* , pSourceRect , CONST RECT* , pDestRect , HWND , hDestWindowOverride , CONST RGNDATA* , pDirtyRegion , DWORD , dwFlags )
	device->ProxyPresent( this );
	return actual->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

/**
* Gets the front buffer data from both (left/right) surface.
* TODO Might be able to use a frame delayed backbuffer (copy last back buffer?) to get proper 
* left/right images. Much pondering required, and some testing.
*	
***/
HRESULT WINAPI D3D9ProxySwapChain::GetFrontBufferData( IDirect3DSurface9* pDestSurface ){
	if( !pDestSurface ){
		return D3DERR_INVALIDCALL;
	}

	D3D9ProxySurface* proxy = (D3D9ProxySurface*)pDestSurface;
	HRESULT           result;
	
	result = actual->GetFrontBufferData( proxy->actual );

	if( proxy->right ){
		result = actual->GetFrontBufferData( proxy->right );
	}
	
	// TODO Might be able to use a frame delayed backbuffer (copy last back buffer?) to get proper left/right images. Much pondering required, and some testing
	//OutputDebugStringA("SwapChain::GetFrontBufferData; Caution Will Robinson. The result of this method at the moment is wrapped surfaces containing what the user would see on a monitor. Example: A side-by-side warped image for the rift in the left and right surfaces of the output surface.\n");
	return result;
}





HRESULT WINAPI D3D9ProxySwapChain::GetBackBuffer( UINT iBackBuffer , D3DBACKBUFFER_TYPE Type , IDirect3DSurface9** ppBackBuffer ){
	if( iBackBuffer >= backBuffers.size() ){
		return D3DERR_INVALIDCALL;
	}

	*ppBackBuffer = backBuffers[iBackBuffer];
	(*ppBackBuffer)->AddRef();

	return D3D_OK;
}



METHOD_THRU( HRESULT , WINAPI , D3D9ProxySwapChain , GetRasterStatus , D3DRASTER_STATUS* , pRasterStatus )
METHOD_THRU( HRESULT , WINAPI , D3D9ProxySwapChain , GetDisplayMode , D3DDISPLAYMODE* , pMode )
METHOD_THRU( HRESULT , WINAPI , D3D9ProxySwapChain , GetPresentParameters , D3DPRESENT_PARAMETERS* , pPresentationParameters )