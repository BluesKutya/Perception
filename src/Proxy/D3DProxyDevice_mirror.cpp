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


void D3DProxyDevice::mirrorInit(){
	mirrorWindow = 0;
}


void D3DProxyDevice::mirrorFree(){
	if( mirrorWindow ){
		UnregisterClass( ClassName , 0 );

		DestroyWindow( mirrorWindow );
		mirrorWindow = 0;
	}

	mirrorSwap.clear();
}




void D3DProxyDevice::mirrorUpdate(){
	//TODO check for leaks on device re-create
	//TODO merge with streamer

	if( !config.mirrorWindowEnable || config.mirrorMode == MIRROR_DISABLE ){
		mirrorFree();
		return;
	}

	if( !mirrorWindow ){
		WNDCLASSEXA wc;
		wc.cbSize        = sizeof(wc);
		wc.style		 = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc	 = main_window_proc;
		wc.cbClsExtra	 = 0;
		wc.cbWndExtra	 = 0;
		wc.hInstance	 = 0;
		wc.hIcon		 = 0;
		wc.hCursor		 = 0;
		wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wc.lpszMenuName	 = 0;
		wc.lpszClassName = ClassName;
		wc.hIconSm       = 0;

		RegisterClassExA( &wc );

		std::vector<RECT> screens = Vireio_getDesktops();
		RECT rect;

		if( config.mirrorWindowDesktop >= screens.size() ){
			rect = screens[0];
		}else{
			rect = screens[config.mirrorWindowDesktop];
		}

		D3DPRESENT_PARAMETERS params;
		activeSwapChains[0]->GetPresentParameters(&params);


		
		UINT style   = 0;
		UINT styleEx = 0;

		if( config.mirrorWindowFullscreen ){
			style = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
		}else{
			style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;

			int cx = (rect.left + rect.right)/2;
			int cy = (rect.top + rect.bottom)/2;
			int cw = params.BackBufferWidth;
			int ch = params.BackBufferHeight;
			
			rect.left   = cx - cw/2;
			rect.right  = cx - cw/2 + cw;
			rect.top    = cy - ch/2;
			rect.bottom = cy - ch/2 + ch;

			AdjustWindowRectEx( &rect , style , false , styleEx );
		}


		mirrorWindow = CreateWindowExA( styleEx , ClassName , "Vireio Mirror" , style , rect.left , rect.top ,  rect.right-rect.left , rect.bottom-rect.top , 0 , 0 , 0 , 0 );

		ShowWindow(mirrorWindow , SW_SHOW );

		params.SwapEffect             = D3DSWAPEFFECT_COPY;
		params.Windowed               = TRUE;
		params.hDeviceWindow          = mirrorWindow;
		params.MultiSampleType        = D3DMULTISAMPLE_NONE;
		params.EnableAutoDepthStencil = false;

		actual->CreateAdditionalSwapChain( &params , mirrorSwap.clearAndGetPtr() );
	}

	if( mirrorSwap && !activeSwapChains.empty() && activeSwapChains[0] ){
		cPtr<IDirect3DSurface9> src;
		cPtr<IDirect3DSurface9> dst;

		switch( config.mirrorMode ){
		case MIRROR_LEFT:
			src = activeSwapChains[0]->backBuffers[0]->actual;
			break;

		case MIRROR_RIGHT:
			src = activeSwapChains[0]->backBuffers[0]->right;
			break;

		case MIRROR_STEREO:
			activeSwapChains[0]->actual->GetBackBuffer( 0 , D3DBACKBUFFER_TYPE_MONO , src.clearAndGetPtr() );
			break;
		}

		mirrorSwap->GetBackBuffer( 0 , D3DBACKBUFFER_TYPE_MONO , dst.clearAndGetPtr() );
		
		if( src && dst ){
			actual->StretchRect( src , 0 , dst , 0 , D3DTEXF_POINT );
		}

		mirrorSwap->Present( 0 , 0 , 0 , 0 , 0 );
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