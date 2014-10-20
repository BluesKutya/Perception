#include "D3DProxyDevice.h"
#include "StereoView.h"
#include "D3D9ProxySwapChain.h"

static LRESULT WINAPI main_window_proc( HWND window_handle , UINT message , WPARAM wparam , LPARAM lparam ){
	switch( message ){
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc( window_handle , message , wparam , lparam );
}


static const char* ClassName = "Vireio_mirror_class";


void D3DProxyDevice::mirrorFree(){
	if( mirrorWindow ){
		UnregisterClass( ClassName , 0 );

		DestroyWindow( mirrorWindow );
		mirrorWindow = 0;
	}

	mirrorSwap.clear();
}


void D3DProxyDevice::mirrorUpdate(){
	if( !mirrorWindow ){
		WNDCLASSA wc;
		wc.style		 = CS_OWNDC;
		wc.lpfnWndProc	 = main_window_proc;
		wc.cbClsExtra	 = 0;
		wc.cbWndExtra	 = 0;
		wc.hInstance	 = 0;
		wc.hIcon		 = 0;
		wc.hCursor		 = 0;
		wc.hbrBackground = 0;
		wc.lpszMenuName	 = 0;
		wc.lpszClassName = ClassName;

		RegisterClass( &wc );


		D3DPRESENT_PARAMETERS params;
		activeSwapChains[0]->GetPresentParameters(&params);
		params.Windowed = TRUE;

		std::vector<RECT> screens = Vireio_getDesktops();
		RECT rect;

		if( config.mirrorScreen >= screens.size() ){
			rect = screens[0];
		}else{
			rect = screens[config.mirrorScreen];
		}

		int width  = params.BackBufferWidth;
		int height = params.BackBufferHeight;
		int x      = (rect.left + rect.right)/2  -  width/2;
		int y      = (rect.top + rect.bottom)/2  +  height/2;

		UINT style;

		if( config.mirrorFullscreen ){
			style = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
		}else{
			style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}

		mirrorWindow = CreateWindowExA( WS_EX_TOPMOST | WS_EX_STATICEDGE , ClassName , "Vireio Mirror" , style , x , y ,  width , height , 0 , 0 , 0 , 0 );
		
		ShowWindow(mirrorWindow , SW_SHOW );

		


		actual->CreateAdditionalSwapChain( &params , mirrorSwap.clearAndGetPtr() );
	}



}


/*
+		if (mirrorToWindow != 0)
+		{
+			HRESULT hr = S_OK;
+
+			// Set the primary rendertarget to the first stereo backbuffer
+			IDirect3DSurface9* pBackBuffer = NULL;
+			hr = m_pMirrorSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
+			if (FAILED(hr))
+				throw std::exception("Could not get Mirror Window Swap Chain back buffer");
+
+			hr = BaseDirect3DDevice9::SetRenderTarget(0, pBackBuffer);
+			if (FAILED(hr))
+				throw std::exception("Could not set Mirror Window render target");
+			
+			switch (m_mirrorType)
+			{
+			case MW_LEFT_EYE:
+				{
+					IDirect3DSurface9* leftImage = static_cast<D3D9ProxySurface*>(pWrappedBackBuffer)->getActualLeft();
+					hr = BaseDirect3DDevice9::StretchRect(leftImage, NULL, pBackBuffer, NULL, D3DTEXF_NONE);
+				}
+				break;
+			case MW_RIGHT_EYE:
+				{
+					IDirect3DSurface9* rightImage = static_cast<D3D9ProxySurface*>(pWrappedBackBuffer)->getActualRight();
+					hr = BaseDirect3DDevice9::StretchRect(rightImage, NULL, pBackBuffer, NULL, D3DTEXF_NONE);
+				}
+				break;
+			case MW_RIFT_VIEW:
+				{
+					if (stereoView->initialized)
+					{
+						IDirect3DSurface9* riftImage = stereoView->GetBackBuffer();
+						hr = BaseDirect3DDevice9::StretchRect(riftImage, NULL, pBackBuffer, NULL, D3DTEXF_NONE);
+					}
+				}
+				break;
+			}
+			if (FAILED(hr))
+				throw std::exception("Mirroring failed on buffer copy");
+
+			pBackBuffer->Release();
+			pBackBuffer = NULL;
+
+			m_pMirrorSwapChain->Present(NULL, NULL, m_pMirrorWindow->window_handle, NULL, 0);
+		}
+*/