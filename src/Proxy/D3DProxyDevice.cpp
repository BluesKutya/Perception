#include "D3DProxyDevice.h"
#include "D3D9ProxySurface.h"
#include "D3D9ProxySwapChain.h"
#include "D3D9ProxyDirect3D.h"
#include "D3D9ProxyTexture.h"
#include "D3D9ProxyVolumeTexture.h"
#include "D3D9ProxyCubeTexture.h"
#include "D3D9ProxyVertexBuffer.h"
#include "D3D9ProxyIndexBuffer.h"
#include "D3D9ProxyStateBlock.h" 
#include "D3D9ProxyQuery.h"
#include "StereoView.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <typeinfo>
#include <assert.h>
#include <comdef.h>
#include <tchar.h>
#include <cTracker.h>
#include <DxErr.h>
#include <qdir.h>



D3DProxyDevice::D3DProxyDevice(IDirect3DDevice9* pDevice,IDirect3DDevice9Ex* pDeviceEx, D3D9ProxyDirect3D* pCreatedBy ):
	cBase( pDevice , this , 0 ) ,
	actualEx( pDeviceEx ),
	m_pCreatedBy(pCreatedBy),
	controls(),
	dinput(),
	show_fps(FPS_NONE),
	calibrate_tracker(false),
	vsConstants( this , true  ) ,
	psConstants( this , false )
{

	tracker = 0;

	menu.init( this );

	if( config.VRboostEnable ){
		vrbInit();
	}


	// Check the maximum number of supported render targets
	D3DCAPS9 capabilities;
	actual->GetDeviceCaps(&capabilities);

	activeRenderTargets.resize( capabilities.NumSimultaneousRTs );

	

	vsConstants.resize( capabilities.MaxVertexShaderConst );

	switch( D3DSHADER_VERSION_MAJOR(capabilities.PixelShaderVersion) ){
	case 1:
		psConstants.resize( 8 );
		break;

	case 2:
		psConstants.resize( 32 );
		break;

	case 3:
	default:
		psConstants.resize( 224 );
		break;
	}


	transformViewSet = false;
	transformProjSet = false;

	D3DXMatrixIdentity( &transformProjOriginal );
	D3DXMatrixIdentity( &transformProjLeft     );
	D3DXMatrixIdentity( &transformProjRight    );

	D3DXMatrixIdentity( &transformViewOriginal );
	D3DXMatrixIdentity( &transformViewLeft     );
	D3DXMatrixIdentity( &transformViewRight    );
	
	

	m_bActiveViewportIsDefault = true;
	m_bViewportIsSquished = false;
	m_bInBeginEndStateBlock = false;
	stateBlock = NULL;
	m_isFirstBeginSceneOfFrame = true;


	screenshot = (int)false;
	m_bVRBoostToggle = true;
	m_fVRBoostIndicator = 0.0f;

	viewComputeTransforms();

	for (int i = 0; i < 16; i++){
		controls.xButtonsStatus[i] = false;
	}
	
	
	//Create Direct Input Mouse Device
	bool directInputActivated = dinput.Init(GetModuleHandle(NULL), ::GetActiveWindow());
	if(directInputActivated)
	{		
		dinput.Activate();		
	}	

	/*
	menu.showMessage( 
		"Vireio Perception: Stereoscopic 3D Driver\n"
		"This program is distributed in the hope that it will be useful,\n" 
		"but WITHOUT ANY WARRANTY; without even the implied warranty of \n" 
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
		"See the GNU LGPL: http://www.gnu.org/licenses/ for more details. "
	);*/


	m_bfloatingMenu = false;
	m_bfloatingScreen = false;
	m_fFloatingScreenPitch = 0;
	m_fFloatingScreenYaw   = 0;
	m_fFloatingScreenZ     = 0;

	// first time configuration
	stereoView                 = new StereoView();
	stereoView->HeadYOffset    = 0;
	
	viewInit();
	viewComputeTransforms();

	mirrorInit();

	vrbLoadValues();

	OnCreateOrRestore();

	//credits will go to world-scale menu, or maybe to startup screen!
	//"Brown Reischl and Schneider Settings Analyzer (B.R.A.S.S.A.)."

	rulesInit();
	rulesUpdate();


	cMenuItem* i;
	cMenuItem* m;
	

	
	if( config.shaderAnalyzer ){
		m = menu.root.addSubmenu( "Shader analyzer" );
		
		//shadersMenu = m->addSubmenu( "Shaders" );
		//shadersMenu->callback = [this](){
		//};

		m->addCheckbox( "Use transposed rules"           , &config.shaderAnalyzerTranspose         );
		m->addCheckbox( "Detect use of transposed rules" , &config.shaderAnalyzerDetectTranspose   );
		m->addCheckbox( "Show pixel shaders"             , &config.shaderAnalyzerShowPixelShaders  );
		m->addCheckbox( "Show unused shaders"            , &config.shaderAnalyzerShowUnusedShaders );
	}
	



	m = menu.root.addSubmenu( "Stereoscopic 3D calibration" );
	m->showCalibrator = true;
	
	i = m->addSpinner( "Separation" , &config.stereoScale , 0.0000001 , 100000 , 0.005 );
	i->callback = [this](){
		viewUpdateProjectionMatrices( );
		viewComputeTransforms       ( );
	};

	i = m->addSpinner( "Convergence" , &config.stereoConvergence , -100 , 100 , 0.01 );
	i->callback = [this](){
		viewUpdateProjectionMatrices( );
		viewComputeTransforms       ( );
	};





	m = menu.root.addSubmenu( "Image settings" );
	i = m->addCheckbox( "Swap eyes"         , &config.swap_eyes );

	i = m->addSpinner ( "IPD offset" , &config.IPDOffset        , 0.001 );
	i->callback = [this](){
		stereoView->PostReset();
	};

	i = m->addSpinner ( "Distortion scale" , &config.DistortionScale  , 0.01 );
	i->callback = [this](){
		stereoView->PostReset();
	};

	m->addCheckbox( "Chromatic aberration correction" , &config.chromaticAberrationCorrection );
	
	i = m->addCheckbox( "VRboost" , &config.VRboostEnable );
	i->callback = [this](){
		if( config.VRboostEnable ){
			vrbInit();
		}else{
			vrbFree();
			if( tracker ){
				tracker->reset();
			}
		}
	};


	i = m->addAction ( "Take screenshot" );
	i->callback = [this](){
		screenshot = 3;
		menu.show = false;
	};

	i = m->addSelect  ( "Mirror mode"       , &config.mirrorMode      , QStringList()<<"disable"<<"left eye"<<"right eye"<<"stereo" );
	i->callback = [this](){
		mirrorFree();
	};

	i = m->addCheckbox( "Mirror window enable" , &config.mirrorWindowEnable );
	i->callback = [this](){
		mirrorFree();
	};

	i = m->addCheckbox( "Mirror window fullscreen" , &config.mirrorWindowFullscreen );
	i->callback = [this](){
		mirrorFree();
	};

	i = m->addSpinner ( "Mirror window desktop"    , &config.mirrorWindowDesktop    , 0 , 0xFFFF , 1 );
	i->callback = [this](){
		mirrorFree();
	};



	m = menu.root.addSubmenu( "OSD and GUI settings" );
	m->addCheckbox( "Show VR mouse"            , &config.showVRMouse );
	m->addCheckbox( "Gui \"bullet labyrinth\"" , &config.guiBulletLabyrinth  );
	
	i = m->addSpinner ( "GUI squash"             , &config.guiSquash  , 0.01 );
	i->callback = [this](){
		viewComputeGui();
	};

	i = m->addSpinner ( "GUI depth"                , &config.guiDepth     , 0.01 );
	i->callback = [this](){
		viewComputeGui();
	};

	i = m->addSpinner ( "HUD distance"             , &config.hudDistance  , 0.01 );
	i->callback = [this](){
		viewComputeGui();
	};

	i = m->addSpinner ( "HUD depth"                , &config.hudDepth     , 0.01 );
	i->callback = [this](){
		viewComputeGui();
	};

	i = m->addCheckbox( "Floating menu"            , &m_bfloatingMenu );

	i = m->addCheckbox( "Floating screen"          , &m_bfloatingScreen );


	auto StoreVRBoostValues = [this](){
		viewComputeTransforms();
		vrbLoadValues();
	};

	m = menu.root.addSubmenu( "VRBoost values" );
	m->addSpinner( "World FOV"                 , &config.WorldFOV                 , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Player FOV"                , &config.PlayerFOV                , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Far plane FOV"             , &config.FarPlaneFOV              , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Camera translate X"        , &config.CameraTranslateX         , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Camera translate Y"        , &config.CameraTranslateY         , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Camera tanslate Z"         , &config.CameraTranslateZ         , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Camera distance"           , &config.CameraDistance           , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Camera zoom"               , &config.CameraZoom               , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Camera horizon adjustment" , &config.CameraHorizonAdjustment  , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Constant value 1"          , &config.ConstantValue1           , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Constant value 2"          , &config.ConstantValue2           , 0.01 )->callback = StoreVRBoostValues;
	m->addSpinner( "Constant value 2"          , &config.ConstantValue3           , 0.01 )->callback = StoreVRBoostValues;














	m = menu.root.addSubmenu( "Tracker Configuration" );
	

	m->addCheckbox( "Tracker rotation" , &config.trackerRotationEnable );
	m->addSpinner ( "Yaw   multiplier" , &config.trackerYawMultiplier   , 0.05 );
	m->addSpinner ( "Pitch multiplier" , &config.trackerPitchMultiplier , 0.05 );
	m->addSpinner ( "Roll  multiplier" , &config.trackerRollMultiplier  , 0.05 );
	m->addCheckbox( "Roll  thing..."   , &config.rollEnabled  );


	i = m->addCheckbox( "Tracker movement" , &config.trackerPositionEnable );
	i->callback = [this](){
		if( !config.trackerPositionEnable ){
			viewUpdatePosition( 0 , 0 , 0 , 0 , 0 , 0 );
		}
	};

	m->addSpinner ( "X multiplier"           , &config.trackerXMultiplier , 0.00001 );
	m->addSpinner ( "Y multiplier"           , &config.trackerYMultiplier , 0.00001 );
	m->addSpinner ( "Z multiplier"           , &config.trackerZMultiplier , 0.00001 );

	m->addCheckbox( "Mouse emulation"        , &config.trackerMouseEmulation );
	m->addSpinner ( "Mouse yaw   multiplier" , &config.trackerMouseYawMultiplier   , 0.05 );
	m->addSpinner ( "Mouse pitch multiplier" , &config.trackerMousePitchMultiplier , 0.05 );

	i = m->addAction( "Reset view"   );
	i->callback = [this](){
		if( tracker ){
			tracker->reset();
		}
	};

	i = m->addAction( "Center floating view"   );
	i->callback = [this](){
		if( tracker ){
			m_fFloatingScreenPitch = tracker->currentPitch;
			m_fFloatingScreenYaw   = tracker->currentYaw;
			m_fFloatingScreenZ     = tracker->currentZ;
		}
	};
	


	m = menu.root.addSubmenu( "Game-specific settings" );
	
	m->addSelect( "Render menu    on"          , &config.whenRenderMenu        , Vireio_enum_when() );
	m->addSelect( "Update tracker on"          , &config.whenUpdateTracker     , Vireio_enum_when() );
	m->addSelect( "Duplicate render target if" , &config.duplicateRenderTarget , Vireio_enum_duplicate() );
	m->addSelect( "Duplicate depth stencil if" , &config.duplicateDepthStencil , Vireio_enum_duplicate() );
	m->addSelect( "Duplicate texture       if" , &config.duplicateTexture      , Vireio_enum_duplicate() );
	m->addSelect( "Duplicate cube texture  if" , &config.duplicateCubeTexture  , Vireio_enum_duplicate() );
	m->addSelect( "State save method"          , &config.saveStateMethod       , Vireio_enum_saveState() );

	
	i= menu.root.addAction ( "Restore configuration" );
	i->callback = [this](){
		config = configBackup;
	};



	i= menu.root.addAction ( "Save configuration" );
	i->callback = [this](){
		SaveConfiguration();
	};

}



bool D3DProxyDevice::isDrawHide( ){
	return config.shaderAnalyzer && ((activeVertexShader && activeVertexShader->hide) || (activePixelShader && activePixelShader->hide));
}



void D3DProxyDevice::SaveConfiguration(){
	static char exe_path[MAX_PATH];
	GetModuleFileNameA( 0   , exe_path , MAX_PATH );
	menu.saveHotkeys( );
	config.save( config.getGameConfigFile(exe_path) , QList<int>() << cConfig::SAVE_GAME << cConfig::SAVE_PROFILE << cConfig::SAVE_USER );
}



D3DProxyDevice::~D3DProxyDevice(){
	ReleaseEverything();

	vrbFree();
}



std::vector<cPtr<D3D9ProxySurface>> D3DProxyDevice::storeAndClearRenderTargets(){
	auto r = activeRenderTargets;

	for( int c=1 ; c<activeRenderTargets.size() ; c++ ){
		SetRenderTarget( c , 0 );
	}

	return r;
}

void D3DProxyDevice::restoreRenderTargets( std::vector<cPtr<D3D9ProxySurface>>& list ){
	for( int c=0 ; c< list.size() ; c++ ){
		SetRenderTarget( c , list[c] );
	}
}











METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetDirect3D , IDirect3D9** , ppD3D9 )
	if (!m_pCreatedBy){
		return D3DERR_INVALIDCALL;
	}

	*ppD3D9 = m_pCreatedBy;
	m_pCreatedBy->AddRef();
	return D3D_OK;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetCursorProperties , UINT , XHotSpot , UINT , YHotSpot , IDirect3DSurface9* , pCursorBitmap )
	if (!pCursorBitmap){
		return actual->SetCursorProperties(XHotSpot, YHotSpot, NULL);
	}
	return actual->SetCursorProperties(XHotSpot, YHotSpot, static_cast<D3D9ProxySurface*>(pCursorBitmap)->actual );
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateAdditionalSwapChain , D3DPRESENT_PARAMETERS* , pPresentationParameters , IDirect3DSwapChain9** , pSwapChain )
	IDirect3DSwapChain9* pActualSwapChain;
	HRESULT result = actual->CreateAdditionalSwapChain(pPresentationParameters, &pActualSwapChain);

	if (SUCCEEDED(result)) {
		D3D9ProxySwapChain* wrappedSwapChain = new D3D9ProxySwapChain(pActualSwapChain, this, true);
		*pSwapChain = wrappedSwapChain;
		activeSwapChains.push_back( wrappedSwapChain );
		
		//since ref added in cPtr of activeSwapChains list
		wrappedSwapChain->Release();
	}

	return result;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetSwapChain , UINT , iSwapChain , IDirect3DSwapChain9** , pSwapChain )
	if( iSwapChain >= activeSwapChains.size() ){
		return D3DERR_INVALIDCALL;
	}

	*pSwapChain = activeSwapChains[iSwapChain]; 
	(*pSwapChain)->AddRef();
	return D3D_OK;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetBackBuffer , UINT , iSwapChain, UINT , iBackBuffer , D3DBACKBUFFER_TYPE , Type , IDirect3DSurface9** , ppBackBuffer )
	if( iSwapChain >= activeSwapChains.size() ){
		return D3DERR_INVALIDCALL;
	}

	return activeSwapChains[iSwapChain]->GetBackBuffer(iBackBuffer, Type, ppBackBuffer);
}




METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateTexture , UINT , Width , UINT , Height , UINT , Levels , DWORD , Usage , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DTexture9** , ppTexture , HANDLE* , pSharedHandle )
	HRESULT creationResult;
	IDirect3DTexture9* pLeftTexture = NULL;
	IDirect3DTexture9* pRightTexture = NULL;	

	// try and create left
	if (SUCCEEDED(creationResult = actual->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &pLeftTexture, pSharedHandle))) {

		// Does this Texture need duplicating?
		if( Vireio_shouldDuplicate( config.duplicateTexture , Width , Height , Usage , false) ){
			if (FAILED(actual->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &pRightTexture, pSharedHandle))) {
				OutputDebugStringA("Failed to create right eye texture while attempting to create stereo pair, falling back to mono\n");
				pRightTexture = NULL;
			}
		}
	}else {
		OutputDebugStringA("Failed to create texture\n"); 
	}

	if (SUCCEEDED(creationResult))
		*ppTexture = new D3D9ProxyTexture(pLeftTexture, pRightTexture, this);

	return creationResult;
}





METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVolumeTexture , UINT , Width , UINT , Height , UINT , Depth , UINT , Levels , DWORD , Usage , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DVolumeTexture9** , ppVolumeTexture , HANDLE* , pSharedHandle )

	IDirect3DVolumeTexture9* pActualTexture = NULL;
	HRESULT creationResult = actual->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, &pActualTexture, pSharedHandle);

	if (SUCCEEDED(creationResult))
		*ppVolumeTexture = new D3D9ProxyVolumeTexture(pActualTexture, this);

	return creationResult;
}




METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateCubeTexture , UINT , EdgeLength , UINT , Levels , DWORD , Usage , D3DFORMAT , Format, D3DPOOL , Pool , IDirect3DCubeTexture9** , ppCubeTexture , HANDLE* , pSharedHandle )

	HRESULT creationResult;
	IDirect3DCubeTexture9* pLeftCubeTexture = NULL;
	IDirect3DCubeTexture9* pRightCubeTexture = NULL;	

	printf("create cube %d %d %d %d %d\n",EdgeLength, Levels, Usage, Format, Pool);

	// try and create left
	if (SUCCEEDED(creationResult = actual->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, &pLeftCubeTexture, pSharedHandle))) {
		
		// Does this Texture need duplicating?
		if( Vireio_shouldDuplicate( config.duplicateCubeTexture , 0 , 0 , Usage , false ) ){
			if (FAILED(actual->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, &pRightCubeTexture, pSharedHandle))) {
				OutputDebugStringA("Failed to create right eye texture while attempting to create stereo pair, falling back to mono\n");
				pRightCubeTexture = NULL;
			}
		}
	}
	else {
		OutputDebugStringA("Failed to create texture\n"); 
	}

	if (SUCCEEDED(creationResult))
		*ppCubeTexture = new D3D9ProxyCubeTexture(pLeftCubeTexture, pRightCubeTexture, this);

	return creationResult;
}






METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVertexBuffer , UINT , Length , DWORD , Usage , DWORD , FVF , D3DPOOL , Pool, IDirect3DVertexBuffer9** , ppVertexBuffer , HANDLE* , pSharedHandle )
	IDirect3DVertexBuffer9* pActualBuffer = NULL;
	HRESULT creationResult = actual->CreateVertexBuffer(Length, Usage, FVF, Pool, &pActualBuffer, pSharedHandle);

	if (SUCCEEDED(creationResult))
		*ppVertexBuffer = new D3D9ProxyVertexBuffer(pActualBuffer, this);

	return creationResult;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateIndexBuffer , UINT , Length , DWORD , Usage , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DIndexBuffer9** , ppIndexBuffer , HANDLE* , pSharedHandle )
	IDirect3DIndexBuffer9* pActualBuffer = NULL;
	HRESULT creationResult = actual->CreateIndexBuffer(Length, Usage, Format, Pool, &pActualBuffer, pSharedHandle);

	if (SUCCEEDED(creationResult))
		*ppIndexBuffer = new D3D9ProxyIndexBuffer(pActualBuffer, this);

	return creationResult;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , UpdateSurface , IDirect3DSurface9* , pSourceSurface , CONST RECT* , pSourceRect , IDirect3DSurface9* , pDestinationSurface , CONST POINT* , pDestPoint )
	if (!pSourceSurface || !pDestinationSurface)
		return D3DERR_INVALIDCALL;

	IDirect3DSurface9* pSourceSurfaceLeft = static_cast<D3D9ProxySurface*>(pSourceSurface)->actual;
	IDirect3DSurface9* pSourceSurfaceRight = static_cast<D3D9ProxySurface*>(pSourceSurface)->right;
	IDirect3DSurface9* pDestSurfaceLeft = static_cast<D3D9ProxySurface*>(pDestinationSurface)->actual;
	IDirect3DSurface9* pDestSurfaceRight = static_cast<D3D9ProxySurface*>(pDestinationSurface)->right;

	HRESULT result = actual->UpdateSurface(pSourceSurfaceLeft, pSourceRect, pDestSurfaceLeft, pDestPoint);

	if (SUCCEEDED(result)) {
		if (!pSourceSurfaceRight && pDestSurfaceRight) {
			//OutputDebugStringA("INFO: UpdateSurface - Source is not stereo, destination is stereo. Copying source to both sides of destination.\n");

			if (FAILED(actual->UpdateSurface(pSourceSurfaceLeft, pSourceRect, pDestSurfaceRight, pDestPoint))) {
				OutputDebugStringA("ERROR: UpdateSurface - Failed to copy source left to destination right.\n");
			}
		} 
		else if (pSourceSurfaceRight && !pDestSurfaceRight) {
			//OutputDebugStringA("INFO: UpdateSurface - Source is stereo, destination is not stereo. Copied Left side only.\n");
		}
		else if (pSourceSurfaceRight && pDestSurfaceRight)	{
			if (FAILED(actual->UpdateSurface(pSourceSurfaceRight, pSourceRect, pDestSurfaceRight, pDestPoint))) {
				OutputDebugStringA("ERROR: UpdateSurface - Failed to copy source right to destination right.\n");
			}
		}
	}

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , UpdateTexture , IDirect3DBaseTexture9* , pSourceTexture , IDirect3DBaseTexture9* , pDestinationTexture )
	if (!pSourceTexture || !pDestinationTexture)
		return D3DERR_INVALIDCALL;


	IDirect3DBaseTexture9* pSourceTextureLeft = NULL;
	IDirect3DBaseTexture9* pSourceTextureRight = NULL;
	IDirect3DBaseTexture9* pDestTextureLeft = NULL;
	IDirect3DBaseTexture9* pDestTextureRight = NULL;

	vireio::UnWrapTexture(pSourceTexture, &pSourceTextureLeft, &pSourceTextureRight);
	vireio::UnWrapTexture(pDestinationTexture, &pDestTextureLeft, &pDestTextureRight);

	HRESULT result = actual->UpdateTexture(pSourceTextureLeft, pDestTextureLeft);

	if (SUCCEEDED(result)) {
		if (!pSourceTextureRight && pDestTextureRight) {
			//OutputDebugStringA("INFO: UpdateTexture - Source is not stereo, destination is stereo. Copying source to both sides of destination.\n");

			if (FAILED(actual->UpdateTexture(pSourceTextureLeft, pDestTextureRight))) {
				OutputDebugStringA("ERROR: UpdateTexture - Failed to copy source left to destination right.\n");
			}
		} 
		else if (pSourceTextureRight && !pDestTextureRight) {
			//OutputDebugStringA("INFO: UpdateTexture - Source is stereo, destination is not stereo. Copied Left side only.\n");
		}
		else if (pSourceTextureRight && pDestTextureRight)	{
			if (FAILED(actual->UpdateTexture(pSourceTextureRight, pDestTextureRight))) {
				OutputDebugStringA("ERROR: UpdateTexture - Failed to copy source right to destination right.\n");
			}
		}
	}

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetRenderTargetData , IDirect3DSurface9* , pRenderTarget , IDirect3DSurface9* , pDestSurface )
	if ((pDestSurface == NULL) || (pRenderTarget == NULL))
		return D3DERR_INVALIDCALL;

	D3D9ProxySurface* pWrappedRenderTarget = static_cast<D3D9ProxySurface*>(pRenderTarget);
	D3D9ProxySurface* pWrappedDest = static_cast<D3D9ProxySurface*>(pDestSurface);

	IDirect3DSurface9* pRenderTargetLeft = pWrappedRenderTarget->actual;
	IDirect3DSurface9* pRenderTargetRight = pWrappedRenderTarget->right;
	IDirect3DSurface9* pDestSurfaceLeft = pWrappedDest->actual;
	IDirect3DSurface9* pDestSurfaceRight = pWrappedDest->right;

	HRESULT result = actual->GetRenderTargetData(pRenderTargetLeft, pDestSurfaceLeft);

	if (SUCCEEDED(result)) {
		if (!pRenderTargetRight && pDestSurfaceRight) {
			//OutputDebugStringA("INFO: GetRenderTargetData - Source is not stereo, destination is stereo. Copying source to both sides of destination.\n");

			if (FAILED(actual->GetRenderTargetData(pRenderTargetLeft, pDestSurfaceRight))) {
				OutputDebugStringA("ERROR: GetRenderTargetData - Failed to copy source left to destination right.\n");
			}
		} 
		else if (pRenderTargetRight && !pDestSurfaceRight) {
			//OutputDebugStringA("INFO: GetRenderTargetData - Source is stereo, destination is not stereo. Copied Left side only.\n");
		}
		else if (pRenderTargetRight && pDestSurfaceRight)	{
			if (FAILED(actual->GetRenderTargetData(pRenderTargetRight, pDestSurfaceRight))) {
				OutputDebugStringA("ERROR: GetRenderTargetData - Failed to copy source right to destination right.\n");
			}
		}
	}

	return result;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetFrontBufferData , UINT , iSwapChain , IDirect3DSurface9* , pDestSurface )
	if( iSwapChain >= activeSwapChains.size() ){
		return D3DERR_INVALIDCALL;
	}

	return activeSwapChains[iSwapChain]->GetFrontBufferData(pDestSurface);
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , StretchRect , IDirect3DSurface9* , pSourceSurface , CONST RECT* , pSourceRect , IDirect3DSurface9* , pDestSurface , CONST RECT* , pDestRect , D3DTEXTUREFILTERTYPE , Filter )
	if (!pSourceSurface || !pDestSurface)
		return D3DERR_INVALIDCALL;

	D3D9ProxySurface* pWrappedSource = static_cast<D3D9ProxySurface*>(pSourceSurface);
	D3D9ProxySurface* pWrappedDest = static_cast<D3D9ProxySurface*>(pDestSurface);

	IDirect3DSurface9* pSourceSurfaceLeft = pWrappedSource->actual;
	IDirect3DSurface9* pSourceSurfaceRight = pWrappedSource->right;
	IDirect3DSurface9* pDestSurfaceLeft = pWrappedDest->actual;
	IDirect3DSurface9* pDestSurfaceRight = pWrappedDest->right;

	HRESULT result = actual->StretchRect(pSourceSurfaceLeft, pSourceRect, pDestSurfaceLeft, pDestRect, Filter);

	if (SUCCEEDED(result)) {
		if (!pSourceSurfaceRight && pDestSurfaceRight) {
			//OutputDebugStringA("INFO: StretchRect - Source is not stereo, destination is stereo. Copying source to both sides of destination.\n");

			if (FAILED(actual->StretchRect(pSourceSurfaceLeft, pSourceRect, pDestSurfaceRight, pDestRect, Filter))) {
				OutputDebugStringA("ERROR: StretchRect - Failed to copy source left to destination right.\n");
			}
		} 
		else if (pSourceSurfaceRight && !pDestSurfaceRight) {
			//OutputDebugStringA("INFO: StretchRect - Source is stereo, destination is not stereo. Copied Left side only.\n");
		}
		else if (pSourceSurfaceRight && pDestSurfaceRight)	{
			if (FAILED(actual->StretchRect(pSourceSurfaceRight, pSourceRect, pDestSurfaceRight, pDestRect, Filter))) {
				OutputDebugStringA("ERROR: StretchRect - Failed to copy source right to destination right.\n");
			}
		}
	}

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , ColorFill, IDirect3DSurface9* , pSurface , CONST RECT* , pRect , D3DCOLOR , color )
	HRESULT result;

	D3D9ProxySurface* pDerivedSurface = static_cast<D3D9ProxySurface*> (pSurface);
	if (SUCCEEDED(result = actual->ColorFill(pDerivedSurface->actual, pRect, color)))
	{
		if (pDerivedSurface->right){
			actual->ColorFill(pDerivedSurface->right, pRect, color);
		}
	}

	return result;
}






METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetRenderTarget , DWORD , RenderTargetIndex , IDirect3DSurface9* , pRenderTarget )
	D3D9ProxySurface* proxy;
	HRESULT           result;

	if( !pRenderTarget ){
		if( RenderTargetIndex == 0 ) {
			// main render target should never be set to NULL
			return D3DERR_INVALIDCALL; 
		}	

		proxy  = 0;
		result = actual->SetRenderTarget( RenderTargetIndex , 0 );
	}else{
		proxy = static_cast<D3D9ProxySurface*>(pRenderTarget);

		if( m_currentRenderingSide == vireio::Left || !proxy->right ){
			result = actual->SetRenderTarget( RenderTargetIndex , proxy->actual );
		}else{
			result = actual->SetRenderTarget( RenderTargetIndex , proxy->right );
		}
	}

	if (result == D3D_OK) {		
		m_bActiveViewportIsDefault             = true;
		activeRenderTargets[RenderTargetIndex] = proxy;
	}

	return result;
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetRenderTarget , DWORD , RenderTargetIndex , IDirect3DSurface9** , ppRenderTarget )
	if( RenderTargetIndex < 0 || RenderTargetIndex >= activeRenderTargets.size() ){
		return D3DERR_INVALIDCALL;
	}

	D3D9ProxySurface* proxy = activeRenderTargets[RenderTargetIndex];
	if( !proxy ){
		return D3DERR_NOTFOUND;
	}

	*ppRenderTarget = proxy;
	(*ppRenderTarget)->AddRef();

	return D3D_OK;
}






METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetDepthStencilSurface , IDirect3DSurface9* , pNewZStencil )
	D3D9ProxySurface* proxy;
	HRESULT           result;

	IDirect3DSurface9* pActualStencilForCurrentSide = NULL;
	if( pNewZStencil ) {
		proxy = static_cast<D3D9ProxySurface*>( pNewZStencil );

		if( m_currentRenderingSide == vireio::Left || !proxy->right ){
			result = actual->SetDepthStencilSurface( proxy->actual );
		}else{
			result = actual->SetDepthStencilSurface( proxy->right );
		}
	}else{
		proxy  = 0;
		result = actual->SetDepthStencilSurface( 0 );
	}
	
	if( SUCCEEDED(result) ){
		activeStereoDepthStencil = proxy;
	}

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetDepthStencilSurface , IDirect3DSurface9** , ppZStencilSurface )
	if( !activeStereoDepthStencil ){
		return D3DERR_NOTFOUND;
	}

	*ppZStencilSurface = activeStereoDepthStencil;
	(*ppZStencilSurface)->AddRef();

	return D3D_OK;
}




METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , BeginScene )
	if (tracker){
		tracker->beginFrame();
	}

	if (m_isFirstBeginSceneOfFrame) {

		// save screenshot before first clear() is called
		if (screenshot>0)
		{
			if (screenshot==1)
				stereoView->SaveScreen();
			screenshot--;
		}

		// set last frame vertex shader count
		m_VertexShaderCountLastFrame = m_VertexShaderCount;

		// avoid squished viewport in case of brassa menu being drawn
		if( m_bViewportIsSquished && menu.show )
		{
			if (m_bViewportIsSquished)
				actual->SetViewport(&m_LastViewportSet);
			m_bViewportIsSquished = false;
		}

		if( config.whenUpdateTracker == WHEN_BEGIN_SCENE || config.whenUpdateTracker == WHEN_FIRST_BEGIN_SCENE ){
			HandleTracking();
		}

		if( config.whenRenderMenu == WHEN_BEGIN_SCENE  || config.whenRenderMenu == WHEN_FIRST_BEGIN_SCENE ){
			menu.render();
		}

		// handle controls
		HandleControls();

		// set vertex shader call count to zero
		m_VertexShaderCount = 0;

		if( config.shaderAnalyzer ){
			for( cShader* s : shaders ){
				s->visible = (s->used || config.shaderAnalyzerShowUnusedShaders) && (s->vs || config.shaderAnalyzerShowPixelShaders);
				if( s->item ){
					s->item->visible = s->visible;
				}
				s->used = false;
			}
		}
	}else{
		// handle controls 
		if( config.whenUpdateTracker == WHEN_BEGIN_SCENE ){
			HandleTracking();
		}

		if( config.whenRenderMenu == WHEN_BEGIN_SCENE ){
			menu.render();
		}
	}

	

	return actual->BeginScene();
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , EndScene )
	if( config.whenUpdateTracker == WHEN_END_SCENE ){
		HandleTracking();
	}

	if( config.whenRenderMenu == WHEN_END_SCENE ){
		menu.render();
	}


	return actual->EndScene();
}





METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetTransform , D3DTRANSFORMSTATETYPE , State , CONST D3DMATRIX* , pMatrix )
	D3DXMATRIX  left;
	D3DXMATRIX  right;
	D3DXMATRIX* view         = 0;
	bool        isSet        = false;
	D3DXMATRIX& sourceMatrix = *(D3DXMATRIX*)pMatrix;



	if( !pMatrix ){
		D3DXMatrixIdentity( &left  );
		D3DXMatrixIdentity( &right );
	}else{
		if( D3DXMatrixIsIdentity(&sourceMatrix) ){
			D3DXMatrixIdentity( &left );
			D3DXMatrixIdentity( &right );
		}else{
			isSet = true;
		}
	}


	if( State == D3DTS_VIEW ){
		if( isSet ){
			left  = sourceMatrix * viewMatTransformLeft;
			right = sourceMatrix * viewMatTransformRight;
		}

		transformViewSet      = isSet;
		transformViewOriginal = sourceMatrix;
		transformViewLeft     = left;
		transformViewRight    = right;

		if( stateBlock ){
			stateBlock->captureViewTransform( );
		}	
	}else
	if( State == D3DTS_PROJECTION ){
		if( isSet ){
			left  = sourceMatrix;
			right = sourceMatrix;
		}

		transformProjSet      = isSet;
		transformProjOriginal = sourceMatrix;
		transformProjLeft     = left;
		transformProjRight    = right;

		if( stateBlock ){
			stateBlock->captureProjTransform( );
		}
	}

	if( !isSet ){
		actual->SetTransform( State , &left ); //identity
	}

	return D3D_OK;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetTransform , D3DTRANSFORMSTATETYPE , State , D3DMATRIX* , pMatrix )
	if( State == D3DTS_VIEW ){
		*pMatrix = transformViewOriginal;
		return D3D_OK;
	}

	if( State == D3DTS_PROJECTION ){
		*pMatrix = transformProjOriginal;
		return D3D_OK;
	}

	return D3DERR_INVALIDCALL;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , MultiplyTransform , D3DTRANSFORMSTATETYPE , State , CONST D3DMATRIX* , pMatrix );
	D3DXMATRIX& m = *(D3DXMATRIX*)pMatrix;

	if( State == D3DTS_VIEW ){
		transformViewOriginal *= m;
		transformViewLeft     *= m;
		transformViewRight    *= m;
	}else
	if( State == D3DTS_PROJECTION ){
		transformProjOriginal *= m;
		transformProjLeft     *= m;
		transformProjRight    *= m;
	}

	return D3D_OK;
}













METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetViewport , CONST D3DVIEWPORT9* , pViewport )	
	HRESULT result = actual->SetViewport(pViewport);

	if( SUCCEEDED(result) ){
		if( stateBlock ){
			stateBlock->captureViewport(*pViewport);
		}

		m_bActiveViewportIsDefault = isViewportDefaultForMainRT(pViewport);
		m_LastViewportSet         = *pViewport;
	}
		
	if( m_bViewportIsSquished ){
		SetGUIViewport();
	}
	
	return result;
}





METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateStateBlock , D3DSTATEBLOCKTYPE , Type , IDirect3DStateBlock9** , ppSB )
	IDirect3DStateBlock9* pActualStateBlock = NULL;
	HRESULT creationResult = actual->CreateStateBlock(Type, &pActualStateBlock);

	if (SUCCEEDED(creationResult)) {
		D3D9ProxyStateBlock* proxy = new D3D9ProxyStateBlock(pActualStateBlock, this);
		proxy->type = Type;
		proxy->init();
		*ppSB = proxy;
	}

	return creationResult;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , BeginStateBlock )
	HRESULT result;
	if (SUCCEEDED(result = actual->BeginStateBlock())) {
		m_bInBeginEndStateBlock = true;
		stateBlock = new D3D9ProxyStateBlock(NULL, this);
		stateBlock->type      = 0;
		stateBlock->init();
	}

	return result;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , EndStateBlock , IDirect3DStateBlock9** , ppSB )
	IDirect3DStateBlock9* pActualStateBlock = NULL;
	HRESULT creationResult = actual->EndStateBlock( &pActualStateBlock );

	if( SUCCEEDED(creationResult) ){
		stateBlock->actual = pActualStateBlock;
		*ppSB = stateBlock;
	}else {
		stateBlock->Release();
		if (stateBlock) delete stateBlock;
	}
	stateBlock              = 0;
	m_bInBeginEndStateBlock = false;

	return creationResult;
}





METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetTexture , DWORD , Stage , IDirect3DBaseTexture9** , ppTexture )
	if( !activeTextures.count(Stage) ){
		return D3DERR_INVALIDCALL;
	}

	*ppTexture = activeTextures[Stage];
	if( (*ppTexture) ){
		(*ppTexture)->AddRef();
	}

	return D3D_OK;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetTexture , DWORD , Stage , IDirect3DBaseTexture9* , pTexture )
	HRESULT result;
	if (pTexture) {

		IDirect3DBaseTexture9* pActualLeftTexture = NULL;
		IDirect3DBaseTexture9* pActualRightTexture = NULL;

		vireio::UnWrapTexture(pTexture, &pActualLeftTexture, &pActualRightTexture);

		// Try and Update the actual devices textures
		if ((pActualRightTexture == NULL) || (m_currentRenderingSide == vireio::Left)) // use left (mono) if not stereo or one left side
			result = actual->SetTexture(Stage, pActualLeftTexture);
		else
			result = actual->SetTexture(Stage, pActualRightTexture);

	}
	else {
		result = actual->SetTexture(Stage, NULL);
	}

	// Update m_activeTextureStages if new texture was successfully set
	if (SUCCEEDED(result)) {
		if( stateBlock ){
			stateBlock->captureTextureSampler( Stage , pTexture );
		}

		activeTextures[Stage] = pTexture;
	}

	return result;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , ProcessVertices , UINT , SrcStartIndex , UINT , DestIndex , UINT , VertexCount , IDirect3DVertexBuffer9* , pDestBuffer , IDirect3DVertexDeclaration9* , pVertexDecl , DWORD , Flags )
	if( !pDestBuffer ){
		return D3DERR_INVALIDCALL;
	}

	D3D9ProxyVertexBuffer*      proxy = static_cast<D3D9ProxyVertexBuffer*>(pDestBuffer);

	rulesApply();

	if( pVertexDecl ) {
		pVertexDecl = static_cast<D3D9ProxyVertexDeclaration*>(pVertexDecl)->actual;
	}

	return actual->ProcessVertices( SrcStartIndex , DestIndex , VertexCount , proxy->actual , pVertexDecl , Flags );
}




/**
* Creates base vertex declaration (D3D9ProxyVertexDeclaration).
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVertexDeclaration , CONST D3DVERTEXELEMENT9* , pVertexElements , IDirect3DVertexDeclaration9** , ppDecl )
	IDirect3DVertexDeclaration9* pActualVertexDeclaration = NULL;
	HRESULT creationResult = actual->CreateVertexDeclaration(pVertexElements, &pActualVertexDeclaration );

	if (SUCCEEDED(creationResult))
		*ppDecl = new D3D9ProxyVertexDeclaration(pActualVertexDeclaration, this);

	return creationResult;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetVertexDeclaration , IDirect3DVertexDeclaration9* , pDecl )
	D3D9ProxyVertexDeclaration* proxy;
	HRESULT                     result;

	if( pDecl ){
		proxy  = static_cast<D3D9ProxyVertexDeclaration*>(pDecl);
		result = actual->SetVertexDeclaration( proxy->actual );
	}else{
		proxy  = 0;
		result = actual->SetVertexDeclaration( 0 );
	}

	if( SUCCEEDED(result) ){
		if (stateBlock) {
			stateBlock->captureVertexDeclaration( proxy );
		}
		activeVertexDeclaration = proxy;
	}

	return result;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetVertexDeclaration , IDirect3DVertexDeclaration9** , ppDecl )
	if( !activeVertexDeclaration ){
		// TODO check this is the response if no declaration set
		//In Response to TODO:  JB, Jan 12. I believe it crashes most times this happens, tested by simply nulling out the ppDecl pointer and passing it into the base d3d method
		return D3DERR_INVALIDCALL; 
	}

	*ppDecl = activeVertexDeclaration;
	if( *ppDecl ){
		(*ppDecl)->AddRef();
	}

	return D3D_OK;
}





METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetStreamSource , UINT , StreamNumber , IDirect3DVertexBuffer9* , pStreamData , UINT , OffsetInBytes , UINT , Stride )	
	D3D9ProxyVertexBuffer* proxy;
	HRESULT                result;
	if( pStreamData ) {		
		proxy  = static_cast<D3D9ProxyVertexBuffer*>(pStreamData);
		result = actual->SetStreamSource( StreamNumber , proxy->actual , OffsetInBytes , Stride );
	}else {
		proxy  = 0;
		result = actual->SetStreamSource( StreamNumber , 0 , OffsetInBytes , Stride );
	}

	if (SUCCEEDED(result)) {
		if (stateBlock) {
			stateBlock->captureVertexStream( StreamNumber , proxy );
		}

		activeVertexes[StreamNumber] = proxy;
	}

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetStreamSource , UINT , StreamNumber , IDirect3DVertexBuffer9** , ppStreamData , UINT* , pOffsetInBytes , UINT* , pStride )
	// This whole methods implementation is highly questionable. Not sure exactly how GetStreamSource works
	if( !activeVertexes.count(StreamNumber) ) {
		return D3DERR_INVALIDCALL;
	}

	*ppStreamData = activeVertexes[StreamNumber];
	if( *ppStreamData ){
		(*ppStreamData)->AddRef();
	}

	return D3D_OK;
}




/**
* Sets indices and calls proxy state block to capture states.
* @see D3D9ProxyStateBlock::SelectAndCaptureState()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetIndices , IDirect3DIndexBuffer9* , pIndexData )
	D3D9ProxyIndexBuffer* proxy;
	HRESULT               result;

	if( pIndexData ){
		proxy  = static_cast<D3D9ProxyIndexBuffer*>(pIndexData);
		result = actual->SetIndices( proxy->actual );
	}else{
		proxy  = 0;
		result = actual->SetIndices( 0 );
	}

	if( SUCCEEDED(result) ){
		if (stateBlock) {
			stateBlock->captureIndexBuffer(proxy);
		}

		activeIndicies = proxy;
	}

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetIndices , IDirect3DIndexBuffer9** , ppIndexData )
	if( !activeIndicies ){
		return D3DERR_INVALIDCALL;
	}

	*ppIndexData = activeIndicies;
	if( *ppIndexData ){
		(*ppIndexData)->AddRef();
	}

	return D3D_OK;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateQuery , D3DQUERYTYPE , Type , IDirect3DQuery9** , ppQuery )
	// this seems a weird response to me but it's what the actual device does.
	if (!ppQuery)
		return D3D_OK;

	IDirect3DQuery9* pActualQuery = NULL;
	HRESULT creationResult = actual->CreateQuery(Type, &pActualQuery);

	if (SUCCEEDED(creationResult)) {
		*ppQuery = new D3D9ProxyQuery(pActualQuery, this);
	}

	return creationResult;
}



METHOD_IMPL( void , , D3DProxyDevice, HandleControls )
	controls.UpdateXInputs();
}


/**
* Updates selected motion tracker orientation.
***/
METHOD_IMPL( void , , D3DProxyDevice , HandleTracking )
	if( !tracker ){
		switch(config.trackerMode){
		case 10:
			tracker = Vireio_Create_Tracker_FreeSpace();
			break;

		case 20:
			tracker = Vireio_Create_Tracker_FreeTrack();
			break;

		case 30:
			tracker = Vireio_Create_Tracker_SharedMemory();
			break;

		case 40:
			tracker = Vireio_Create_Tracker_Oculus();
			break;

		}


		if( tracker ){
			if( !tracker->open() ){
				menu.showMessage("Tracker error: unable to open device");
				printf( "tracker: open failed\n" );
				tracker->close();
				delete tracker;
				tracker =0 ;
			}
		}

		if( tracker ){
			//Only advise calibration for positional tracking on DK2
			//if (tracker->SupportsPositionTracking()){
			//	calibrate_tracker = true;
				//TODO: add new tracker interface
			//}
 		}
	}

	if( !tracker ){
		return;
	}


	long prevYaw   = RADIANS_TO_DEGREES( tracker->currentYaw   ) * config.trackerMouseYawMultiplier;
	long prevPitch = RADIANS_TO_DEGREES( tracker->currentPitch ) * config.trackerMousePitchMultiplier;

	if( tracker->update() ){
		tracker->currentYaw   *= config.trackerYawMultiplier;
		tracker->currentPitch *= config.trackerPitchMultiplier;
		tracker->currentRoll  *= config.trackerRollMultiplier;

		//multiplied in m_spShaderViewAdjustment->UpdatePosition
		//tracker->currentX     *= config.trackerPositionMultiplier;
		//tracker->currentY     *= config.trackerPositionMultiplier;
		//tracker->currentZ     *= config.trackerPositionMultiplier;

		if( config.trackerMouseEmulation ){
			long currentYaw   = RADIANS_TO_DEGREES(tracker->currentYaw  ) * config.trackerMouseYawMultiplier;
			long currentPitch = RADIANS_TO_DEGREES(tracker->currentPitch) * config.trackerMousePitchMultiplier;

			long deltaYaw   = currentYaw   - prevYaw;
			long deltaPitch = currentPitch - prevPitch;

			deltaYaw   = ((deltaYaw   + 180) % 360) - 180;
			deltaPitch = ((deltaPitch + 180) % 360) - 180;

			if( abs(deltaYaw) > 100 ){
				deltaYaw = 0;
			}

			if( abs(deltaPitch) > 100 ){
				deltaPitch = 0;
			}
			
			INPUT i;
			i.type           = INPUT_MOUSE;
			i.mi.dx          = deltaYaw;
			i.mi.dy          = deltaPitch;
			i.mi.mouseData   = 0;
			i.mi.dwFlags     = MOUSEEVENTF_MOVE;
			i.mi.time        = 0;
			i.mi.dwExtraInfo = 0;

			SendInput( 1 , &i , sizeof(i) );
		}
	}
	
	if( calibrate_tracker ){
		menu.showMessage(
			"Please Calibrate HMD/Tracker:\n"
			"     -  Sit comfortably with your head facing forwards\n"
			"     -  Go to tracker configuration in main menu\n"
			"     -  Reset view \n"
			"     -  Adjust multipliers to comfortable values\n"
		);
	}

	if( config.trackerMouseEmulation ){
		if( config.rollEnabled ){
			viewUpdateRotation( 0 , 0, tracker->currentRoll );
		}
		return;
	}

	if( config.trackerRotationEnable ){
		viewUpdateRotation( tracker->currentPitch , tracker->currentYaw , (config.rollEnabled ? tracker->currentRoll : 0) );
	}else{
		viewUpdateRotation( 0 , 0 , 0 );
	}

	if( config.trackerPositionEnable ){
		viewUpdatePosition(
			tracker->currentPitch,
			tracker->currentYaw,
			tracker->currentRoll,
			
			//VRboost value here?
			tracker->currentX * config.trackerXMultiplier * config.stereoScale,
			tracker->currentY * config.trackerYMultiplier * config.stereoScale,
			tracker->currentZ * config.trackerZMultiplier * config.stereoScale
		);
	}
	

	if( m_bfloatingScreen ){
		float screenFloatMultiplierY = 0.75;
		float screenFloatMultiplierX = 0.5;
		float screenFloatMultiplierZ = 1.5;

		this->stereoView->HeadYOffset = (m_fFloatingScreenPitch - tracker->currentPitch) * screenFloatMultiplierY;
		this->stereoView->XOffset     = (m_fFloatingScreenYaw   - tracker->currentYaw  ) * screenFloatMultiplierX;
		this->stereoView->HeadZOffset = (m_fFloatingScreenZ     - tracker->currentZ    ) * screenFloatMultiplierZ;
		this->stereoView->PostReset();

		//m_ViewportIfSquished.X = (int)(vOut.x+centerX-(((m_fFloatingYaw - tracker->primaryYaw) * floatMultiplier) * (180 / PI)));
		//m_ViewportIfSquished.Y = (int)(vOut.y+centerY-(((m_fFloatingPitch - tracker->primaryPitch) * floatMultiplier) * (180 / PI)));
	}

	viewComputeTransforms();

	vrbUpdate();

	m_isFirstBeginSceneOfFrame = false;
}


/**
* Creates or restores class setup.
* Subclasses which override this method must call through to super method.
* Do not directly call this method in subclasses.
* This method should be used to re/create any resources that are held by the device proxy and deleted by Reset.
* 
* The only resources used like this are going to be extra resources that are used by the proxy and are not
* part of the actual calling application. 
* 
* Examples in D3DProxyDevice: The Font used in the BRASSA overlay and the stereo buffer.
* 
* Example of something you wouldn't create here:
* Render targets in the m_activeRenderTargets collection. They need to be released to successfully Reset
* the device, but they just wrap IDirect3DSurface9 objects from the underlying application and will be
* re/created by the underlying application.
* 
* This method will be called when the proxy device is initialised with Init (happens before device is
* passed back to actual application by CreateDevice) and after a successful device Reset.
***/
METHOD_IMPL( void , , D3DProxyDevice , OnCreateOrRestore )	
	m_currentRenderingSide     = vireio::Left;
	m_pCurrentMatViewTransform = &viewMatViewProjTransformLeft;

	// Wrap the swap chain
	IDirect3DSwapChain9* pActualPrimarySwapChain;
	if (FAILED(actual->GetSwapChain(0, &pActualPrimarySwapChain))) {
		OutputDebugStringA("Failed to fetch swapchain.\n");
		exit(1); 
	}

	activeSwapChains.clear();

	activeSwapChains.push_back( new D3D9ProxySwapChain(pActualPrimarySwapChain, this, false) );

	// Set the primary rendertarget to the first stereo backbuffer
	IDirect3DSurface9* pWrappedBackBuffer;
	activeSwapChains[0]->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pWrappedBackBuffer);
	SetRenderTarget(0, pWrappedBackBuffer);
	pWrappedBackBuffer->Release();
	pWrappedBackBuffer = NULL;


	actual->GetViewport(&m_LastViewportSet);

	// If there is an initial depth stencil
	IDirect3DSurface9* pDepthStencil;
	if (SUCCEEDED(actual->GetDepthStencilSurface(&pDepthStencil))) { 

		D3DSURFACE_DESC stencilDesc;
		pDepthStencil->GetDesc(&stencilDesc);
		pDepthStencil->Release();

		IDirect3DSurface9* pTemp = NULL;
		CreateDepthStencilSurface(stencilDesc.Width, stencilDesc.Height, stencilDesc.Format, stencilDesc.MultiSampleType, stencilDesc.MultiSampleQuality, false, &pTemp, NULL);
		SetDepthStencilSurface(pTemp);
		pTemp->Release();	
	}

	menu.createResources();

	stereoView->Init( this );

	viewUpdateProjectionMatrices( );
	viewComputeTransforms       ( );

	// set BRASSA main values
	menu.viewportWidth  = stereoView->viewport.Width;
	menu.viewportHeight = stereoView->viewport.Height;
}

/**
* Switches rendering to which ever side is specified by side.
* Use to specify the side that you want to draw to.
* Overriding classes should call the base implementation first and then makes any extra needed changes
* based on the result of the base implementation (if the base class doesn't change side then derived shouldn't 
* change either)
* 
* @return True if change succeeded, false if it fails. The switch will fail if you attempt to setDrawingSide(Right)
* when the current primary active render target (target 0  in m_activeRenderTargets) is not stereo.
* Attempting to switch to a side when that side is already the active side will return true without making any changes.
***/
METHOD_IMPL( bool , , D3DProxyDevice , setDrawingSide , vireio::RenderPosition , side )
	// Already on the correct eye
	if (side == m_currentRenderingSide) {
		return true;
	}

	if( !activeRenderTargets[0]->right && (side == vireio::Right) ){
		//should never try and render for the right eye if there is no render target for the main render targets right side
		return false;
	}

	// Everything hasn't changed yet but we set this first so we don't accidentally use the member instead of the local and break
	// things, as I have already managed twice.
	m_currentRenderingSide = side;

	// switch render targets to new side
	bool              renderTargetChanged = false;
	

	HRESULT result;

	for( int c=0 ; c<activeRenderTargets.size() ; c++ ){
		auto rt = activeRenderTargets[c];
		if( rt ){
			if( side == vireio::Left || !rt->right ){
				result = actual->SetRenderTarget( c , rt->actual ); 
			}else{
				result = actual->SetRenderTarget( c , rt->right );
			}
		}else{
			result = actual->SetRenderTarget( c , 0 ); 
		}

		if( FAILED(result) ){
			printf("Vireio: SetRenderTarget in setDrawingSide failed\n");
		}else{
			renderTargetChanged = true;
		}

	}



	// if a non-fullsurface viewport is active and a rendertarget changed we need to reapply the viewport
	if( renderTargetChanged && !m_bActiveViewportIsDefault ){
		actual->SetViewport( &m_LastViewportSet );
	}

	if( m_bViewportIsSquished ){
		SetGUIViewport();
	}


	if( activeStereoDepthStencil ) { 
		if( side == vireio::Left || !activeStereoDepthStencil->right ){
			result = actual->SetDepthStencilSurface( activeStereoDepthStencil->actual ); 
		}else{
			result = actual->SetDepthStencilSurface( activeStereoDepthStencil->right );
		}

		if( FAILED(result) ){
			printf("Vireio: SetDepthStencilSurface in setDrawingSide failed\n");
		}
	}

	
	for( auto& p : activeTextures ){
		if( p.second ){
			IDirect3DBaseTexture9* left  = 0;
			IDirect3DBaseTexture9* right = 0;

			vireio::UnWrapTexture( p.second  , &left , &right );

			if( side == vireio::Left || !right ){
				result = actual->SetTexture( p.first , left ); 
			}else{
				result = actual->SetTexture( p.first , right );
			}

			if( FAILED(result) ){
				printf("Vireio: SetTexture in setDrawingSide failed\n");
			}
		}

	}



	if( transformViewSet ){
		if( side == vireio::Left ){
			actual->SetTransform( D3DTS_VIEW , &transformViewLeft );
		}else{
			actual->SetTransform( D3DTS_VIEW , &transformViewRight );
		}
	}


	if( transformProjSet ){
		if( side == vireio::Left ){
			actual->SetTransform( D3DTS_PROJECTION , &transformProjLeft );
		}else{
			actual->SetTransform( D3DTS_PROJECTION , &transformProjRight );
		}
	}



	// Updated computed view translation (used by several derived proxies - see: ComputeViewTranslation)
	if( side == vireio::Left ){
		m_pCurrentMatViewTransform = &viewMatViewProjTransformLeft;
	}else{
		m_pCurrentMatViewTransform = &viewMatViewProjTransformRight;
	}


	rulesApply();

	return true;
}




METHOD_IMPL( bool , , D3DProxyDevice , switchDrawingSide )
	bool switched = false;

	if( m_currentRenderingSide == vireio::Left ){
		switched = setDrawingSide(vireio::Right);
	}else
	if( m_currentRenderingSide == vireio::Right ){
		switched = setDrawingSide(vireio::Left);
	}else{
		printf( "Vireio: unknown m_currentRenderingSide\n" );
	}

	if( !switched ){
		//printf( "Vireio: switch failed\n" );
	}


	return switched;
}



//FPS Calculator

#define MAXSAMPLES 100

float D3DProxyDevice::CalcFPS(){
	static bool init=false;
	static int tickindex=0;
	static LONGLONG ticksum=0;
	static LONGLONG ticklist[MAXSAMPLES];
	static LONGLONG prevTick;
	static LARGE_INTEGER perffreq;
	if (!init)
	{
		//Initialise - should only ever happen once
		memset(ticklist, 0, sizeof(LONGLONG) * MAXSAMPLES);
		QueryPerformanceFrequency(&perffreq);
		init=true;
	}

	//Get the new tick
	LARGE_INTEGER newtick;
	QueryPerformanceCounter(&newtick);
	
	ticksum -= ticklist[tickindex];
    ticksum += newtick.QuadPart - prevTick;
    ticklist[tickindex] = newtick.QuadPart - prevTick;
    tickindex = ++tickindex % MAXSAMPLES;
	prevTick = newtick.QuadPart;

	float FPS = (float)((double)MAXSAMPLES / ((double)ticksum / (double)perffreq.QuadPart));

	//char buffer[256];
	//sprintf(buffer, "FPS: %.1f\n", FPS);
	//OutputDebugStringA(buffer);

    return FPS;
}



/**
* Releases HUD font, shader registers, render targets, texture stages, vertex buffers, depth stencils, indices, shaders, declarations.
***/
METHOD_IMPL( void , , D3DProxyDevice , ReleaseEverything )
	// Fonts and any other D3DX interfaces should be released first.
	// They frequently hold stateblocks which are holding further references to other resources.
	menu.freeResources();

	//m_spManagedShaderRegisters->ReleaseResources();

	SAFE_RELEASE( stateBlock );


	for( cPtr<D3D9ProxySwapChain>& ptr : activeSwapChains ){
		ptr.releaseAndDelete();
	}

	activeSwapChains.clear();

	// one of these will still have a count of 1 until the backbuffer is released
	activeRenderTargets.clear();
	activeTextures     .clear();
	activeVertexes     .clear();


	activeStereoDepthStencil.clear();
	activeIndicies          .clear();
	activeVertexDeclaration .clear();
	activeVertexShader      .clear();
	activePixelShader       .clear();
	
}
/**
* Comparison made against active primary render target.
*
***/
METHOD_IMPL( bool , , D3DProxyDevice , isViewportDefaultForMainRT , CONST D3DVIEWPORT9* , pViewport )
	D3D9ProxySurface* pPrimaryRenderTarget = activeRenderTargets[0];
	D3DSURFACE_DESC pRTDesc;
	pPrimaryRenderTarget->GetDesc(&pRTDesc);

	return  ((pViewport->Height == pRTDesc.Height) && (pViewport->Width == pRTDesc.Width) &&
		(pViewport->MinZ <= 0.00001) && (pViewport->MaxZ >= 0.99999));
}


/**
* Sets the viewport to squish the GUI accordingly.
***/
METHOD_IMPL( void , , D3DProxyDevice , SetGUIViewport )
	// do not squish the viewport in case brassa menu is open - GBCODE Why?
	//if ((BRASSA_mode>=BRASSA_Modes::MAINMENU) && (BRASSA_mode<BRASSA_Modes::BRASSA_ENUM_RANGE))
	//	return;

	D3DXMATRIX mLeftShift;
	D3DXMATRIX mRightShift;

	// set shift by current gui 3d depth
	float shiftInPixels = config.guiDepth;
	D3DXMatrixTranslation(&mLeftShift, -shiftInPixels, 0, 0);
	D3DXMatrixTranslation(&mRightShift, shiftInPixels, 0, 0);

	// get matrix
	D3DXMATRIX mVPSquash;
	if( m_currentRenderingSide == vireio::Left ){
		mVPSquash = mLeftShift  * viewMatSquash;
	}else{
		mVPSquash = mRightShift * viewMatSquash;
	}

	// get viewport
	actual->GetViewport(&m_ViewportIfSquished);

	// get screen center and translate it
	float centerX = (((FLOAT)stereoView->viewport.Width-(FLOAT)stereoView->viewport.X)/2.0f);
	float centerY = (((FLOAT)stereoView->viewport.Height-(FLOAT)stereoView->viewport.Y)/2.0f);

	// get left/top viewport sides
	D3DXVECTOR3 vIn = D3DXVECTOR3((FLOAT)stereoView->viewport.X-centerX, (FLOAT)stereoView->viewport.Y-centerY,1);
	D3DXVECTOR4 vOut = D3DXVECTOR4();
	D3DXVec3Transform(&vOut,&vIn, &mVPSquash);
	float floatMultiplier = 4;
	int originalX = (int)(vOut.x+centerX);
	int originalY = (int)(vOut.y+centerY);
	if(m_bfloatingMenu && tracker )
	{
		/*char buf[64];
		LPCSTR psz = NULL;
		sprintf_s(buf, "yaw: %f, pitch: %f\n", tracker->primaryYaw, tracker->primaryPitch);
		psz = buf;*/		
		m_ViewportIfSquished.X = (int)(vOut.x+centerX-(((m_fFloatingYaw - tracker->currentYaw) * floatMultiplier) * (180 / M_PI)));
		m_ViewportIfSquished.Y = (int)(vOut.y+centerY-(((m_fFloatingPitch - tracker->currentPitch) * floatMultiplier) * (180 / M_PI)));
	}
	else
	{
		m_ViewportIfSquished.X = (int)(vOut.x+centerX);
		m_ViewportIfSquished.Y = (int)(vOut.y+centerY);
	}

	// get right/bottom viewport sides
	vIn = D3DXVECTOR3((FLOAT)(stereoView->viewport.Width+stereoView->viewport.X)-centerX, (FLOAT)(stereoView->viewport.Height+stereoView->viewport.Y)-centerY,1);
	vOut = D3DXVECTOR4();
	D3DXVec3Transform(&vOut,&vIn, &mVPSquash);
	m_ViewportIfSquished.Width = (int)(vOut.x+centerX) - originalX;
	m_ViewportIfSquished.Height = (int)(vOut.y+centerY) - originalY;

	// set viewport
	m_bViewportIsSquished = true;
	actual->SetViewport(&m_ViewportIfSquished);
}

/**
* Rounds the floats to make them more display friendly
**/
METHOD_IMPL( float , , D3DProxyDevice , RoundBrassaValue , float , val )
	return (float)floor(val * 1000.0f + 0.5f) / 1000.0f;
}








void D3DProxyDevice::ProxyPresent( D3D9ProxySwapChain* swapChain ){
	fps = CalcFPS();

	if( config.whenUpdateTracker == WHEN_PRESENT ){
		HandleTracking();
	}

	if( config.whenRenderMenu == WHEN_PRESENT ){
		menu.render();
	}

	if( stereoView->initialized ){
		stereoView->Draw( swapChain );
	}

	if( tracker ){
		tracker->endFrame();
	}

	m_isFirstBeginSceneOfFrame = true; 
 }


 






/**
* Creates a proxy (or wrapped) depth stencil surface (D3D9ProxySurface).
* Surface to be created only gets both stereo surfaces if game handler agrees.
* @see D3D9ProxySurface
* @see GameHandler::ShouldDuplicateDepthStencilSurface() 
***/
METHOD_IMPL( HRESULT , , D3DProxyDevice , ProxyCreateDepthStencilSurface , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Discard , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage , bool , useEx )
	IDirect3DSurface9* left  = 0;
	IDirect3DSurface9* right = 0;
	HRESULT            resultLeft;
	HRESULT            resultRight;

	if( useEx ){
		resultLeft = actualEx->CreateDepthStencilSurfaceEx( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &left, pSharedHandle , Usage );
	}else{
		resultLeft = actual  ->CreateDepthStencilSurface  ( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &left, pSharedHandle);
	}
	
	if( SUCCEEDED(resultLeft) ){

		// TODO Should we always duplicated Depth stencils? I think yes, but there may be exceptions
		if( Vireio_shouldDuplicate( config.duplicateDepthStencil , Width , Height , D3DUSAGE_DEPTHSTENCIL , false ) ){
			if( useEx ){
				resultRight = actualEx->CreateDepthStencilSurfaceEx( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &right, pSharedHandle , Usage );
			}else{
				resultRight = actual  ->CreateDepthStencilSurface  ( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &right, pSharedHandle);
			}

			if( FAILED(resultRight) ){
				OutputDebugStringA("Failed to create right eye Depth Stencil Surface while attempting to create stereo pair, falling back to mono\n");
				right = 0;
			}
		}

		*ppSurface = new D3D9ProxySurface(left, right, this, NULL);
	} else {
		OutputDebugStringA("Failed to create Depth Stencil Surface\n"); 
	}

	return resultLeft;
}



/**
* Creates proxy (wrapped) render target, if swapchain buffer returns StereoBackBuffer, otherwise D3D9ProxySurface.
* Duplicates render target if game handler agrees.
* @see GameHandler::ShouldDuplicateRenderTarget()
* @see StereoBackBuffer
* @see D3D9ProxySurface
***/
METHOD_IMPL( HRESULT , , D3DProxyDevice , ProxyCreateRenderTarget , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample ,
												  DWORD , MultisampleQuality , BOOL , Lockable , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage , bool , isSwapChainBackBuffer , bool , useEx )
	IDirect3DSurface9* left  = 0;
	IDirect3DSurface9* right = 0;
	HRESULT resultLeft;
	HRESULT resultRight;

	// create left/mono

	if( useEx ){
		resultLeft = actualEx->CreateRenderTargetEx(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &left, pSharedHandle , Usage );
	}else{
		resultLeft = actual  ->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &left, pSharedHandle);
	}

	if( SUCCEEDED(resultLeft) ){

		/* "If Needed" heuristic is the complicated part here.
		Fixed heuristics (based on type, format, size, etc) + game specific overrides + isForcedMono + magic? */
		// TODO Should we duplicate this Render Target? Replace "true" with heuristic
		if( Vireio_shouldDuplicate( config.duplicateRenderTarget , Width , Height , D3DUSAGE_RENDERTARGET , isSwapChainBackBuffer ) ){
			if( useEx ){
				resultRight = actualEx->CreateRenderTargetEx(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &right, pSharedHandle , Usage );
			}else{
				resultRight = actual  ->CreateRenderTarget  (Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &right, pSharedHandle);
			}

			if( FAILED(resultRight) ){
				OutputDebugStringA("Failed to create right eye render target while attempting to create stereo pair, falling back to mono\n");
				right = NULL;
			}
		}

		*ppSurface = new D3D9ProxySurface(left, right, this, NULL);
	}

	return resultLeft;
}



//Creates a wrapped mono surface with only one (left) side.
//OffscreenPlainSurfaces doesn't need to be Stereo. 
//They can't be used as render targets and they can't have rendertargets copied to them with stretch
//rect, so don't need to be stereo capable.
//See table at bottom of 
//<http://msdn.microsoft.com/en-us/library/windows/desktop/bb174471%28v=vs.85%29.aspx> 
//for stretch rect restrictions.
METHOD_IMPL( HRESULT , , D3DProxyDevice , ProxyCreateOffscreenPlainSurface , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage , bool , useEx )
	IDirect3DSurface9* surface = 0;
	HRESULT result;
	
	if( useEx ){
		result = actualEx->CreateOffscreenPlainSurfaceEx(Width, Height, Format, Pool, &surface, pSharedHandle , Usage );
	}else{
		result = actual  ->CreateOffscreenPlainSurface(Width, Height, Format, Pool, &surface, pSharedHandle );
	}

	if (SUCCEEDED(result)){
		*ppSurface = new D3D9ProxySurface(surface, NULL, this, NULL);
	}

	return result;
}


/**
* Calls release functions here and in stereo view class, releases swap chains and restores everything.
* Subclasses which override this method must call through to super method at the end of the subclasses
* implementation.
* @see ReleaseEverything()
* @see StereoView::ReleaseEverything()
* @see OnCreateOrRestore()
***/
METHOD_IMPL( HRESULT , , D3DProxyDevice , ProxyReset , D3DPRESENT_PARAMETERS* , pPresentationParameters , D3DDISPLAYMODEEX* , pFullscreenDisplayMode , bool , useEx )
	if(stereoView)
		stereoView->ReleaseEverything();

	ReleaseEverything();

	m_bInBeginEndStateBlock = false;

	HRESULT hr;
	if( useEx ){
		hr = actualEx->ResetEx(pPresentationParameters,pFullscreenDisplayMode);
	}else{
		hr = actual  ->Reset(pPresentationParameters);
	}

	// if the device has been successfully reset we need to recreate any resources we created
	if (hr == D3D_OK)  {
		OnCreateOrRestore();
		stereoView->PostReset();
	}else {

		OutputDebugStringA("Device reset failed");
	}

	return hr;
}







// IDirect3DDevice9 / IDirect3DDevice9Ex similar methods

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , Present , CONST RECT* , pSourceRect , CONST RECT* , pDestRect , HWND , hDestWindowOverride , CONST RGNDATA* , pDirtyRegion )
	if( activeSwapChains.empty() ){
		printf( "Present: No primary swap chain found. (Present probably called before device has been reset)\n" );
	}else{
		ProxyPresent( activeSwapChains[0] );
	}

	return actual->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , PresentEx , CONST RECT* , pSourceRect , CONST RECT* , pDestRect , HWND , hDestWindowOverride , CONST RGNDATA* , pDirtyRegion , DWORD , dwFlags )
	if( activeSwapChains.empty() ){
		printf( "Present: No primary swap chain found. (Present probably called before device has been reset)\n" );
	}else{
		ProxyPresent( activeSwapChains[0] );
	}

	return actualEx->PresentEx(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion , dwFlags );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateOffscreenPlainSurface , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle )	
	return ProxyCreateOffscreenPlainSurface( Width, Height, Format, Pool, ppSurface , pSharedHandle , 0 , false );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateOffscreenPlainSurfaceEx , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage )	
	return ProxyCreateOffscreenPlainSurface( Width, Height, Format, Pool, ppSurface , pSharedHandle , Usage , true );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateRenderTarget , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Lockable , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle )
	//The IDirect3DSurface9** ppSurface returned should always be a D3D9ProxySurface. Any class overloading
	//this method should ensure that this remains true.
	return ProxyCreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle, 0 , false , false );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateRenderTargetEx , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Lockable , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage )
	return ProxyCreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle, 0 , false , true );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateDepthStencilSurface , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Discard , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle )
	return ProxyCreateDepthStencilSurface( Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface , pSharedHandle , 0  , false );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateDepthStencilSurfaceEx , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Discard , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage )
	return ProxyCreateDepthStencilSurface( Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface , pSharedHandle , Usage , true );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , Reset , D3DPRESENT_PARAMETERS* , pPresentationParameters )
	return ProxyReset( pPresentationParameters , 0 , false );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , ResetEx , D3DPRESENT_PARAMETERS* , pPresentationParameters , D3DDISPLAYMODEEX* , pFullscreenDisplayMode )
	return ProxyReset( pPresentationParameters , pFullscreenDisplayMode , true );
}




// IDirect3DDevice9Ex base methods. are there any proxy required for those?
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , GetDisplayModeEx , UINT , iSwapChain , D3DDISPLAYMODEEX* , pMode , D3DDISPLAYROTATION* , pRotation )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , SetConvolutionMonoKernel , UINT , width , UINT , height , float* , rows , float* , columns )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , ComposeRects , IDirect3DSurface9* , pSrc , IDirect3DSurface9* , pDst , IDirect3DVertexBuffer9* , pSrcRectDescs , UINT , NumRects , IDirect3DVertexBuffer9* , pDstRectDescs , D3DCOMPOSERECTSOP , Operation , int , Xoffset , int , Yoffset )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , GetGPUThreadPriority , INT* , pPriority )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , SetGPUThreadPriority , INT , Priority )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , WaitForVBlank , UINT , iSwapChain )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , CheckResourceResidency , IDirect3DResource9** , pResourceArray , UINT32 , NumResources )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , SetMaximumFrameLatency , UINT , MaxLatency )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , GetMaximumFrameLatency , UINT* , pMaxLatency )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , CheckDeviceState , HWND , hDestinationWindow )



// IDirect3DDevice9 base methods
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , TestCooperativeLevel ) // The calling application will start releasing resources after TestCooperativeLevel returns D3DERR_DEVICENOTRESET.
METHOD_THRU( UINT    , WINAPI , D3DProxyDevice , GetAvailableTextureMem )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , EvictManagedResources )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetDeviceCaps , D3DCAPS9* , pCaps )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetDisplayMode , UINT , iSwapChain , D3DDISPLAYMODE* , pMode )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetCreationParameters , D3DDEVICE_CREATION_PARAMETERS* , pParameters )
METHOD_THRU( void    , WINAPI , D3DProxyDevice , SetCursorPosition , int , X , int , Y , DWORD , Flags )
METHOD_THRU( BOOL    , WINAPI , D3DProxyDevice , ShowCursor , BOOL , bShow )
METHOD_THRU( UINT    , WINAPI , D3DProxyDevice , GetNumberOfSwapChains )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetRasterStatus , UINT , iSwapChain , D3DRASTER_STATUS* , pRasterStatus )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetDialogBoxMode , BOOL , bEnableDialogs )
METHOD_THRU( void    , WINAPI , D3DProxyDevice , SetGammaRamp     , UINT , iSwapChain , DWORD , Flags , CONST D3DGAMMARAMP* , pRamp )
METHOD_THRU( void    , WINAPI , D3DProxyDevice , GetGammaRamp     , UINT , iSwapChain , D3DGAMMARAMP* , pRamp )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetViewport , D3DVIEWPORT9* , pViewport )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetMaterial , CONST D3DMATERIAL9* , pMaterial )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetMaterial , D3DMATERIAL9* , pMaterial )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetLight , DWORD , Index , CONST D3DLIGHT9* , pLight )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetLight , DWORD , Index , D3DLIGHT9* , pLight )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , LightEnable , DWORD , Index , BOOL , Enable )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetLightEnable , DWORD , Index , BOOL* , pEnable )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetClipPlane , DWORD , Index , CONST float* , pPlane )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetClipPlane , DWORD , Index , float* , pPlane )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetRenderState , D3DRENDERSTATETYPE , State , DWORD , Value )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetRenderState , D3DRENDERSTATETYPE , State , DWORD* , pValue )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetClipStatus , CONST D3DCLIPSTATUS9* , pClipStatus )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetClipStatus , D3DCLIPSTATUS9* , pClipStatus )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetTextureStageState , DWORD , Stage , D3DTEXTURESTAGESTATETYPE , Type , DWORD* , pValue )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetTextureStageState , DWORD , Stage , D3DTEXTURESTAGESTATETYPE , Type , DWORD  , Value )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetSamplerState , DWORD , Sampler , D3DSAMPLERSTATETYPE , Type , DWORD* , pValue)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetSamplerState,DWORD ,Sampler,D3DSAMPLERSTATETYPE ,Type,DWORD ,Value)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , ValidateDevice,DWORD* ,pNumPasses)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetPaletteEntries,UINT ,PaletteNumber,CONST PALETTEENTRY*, pEntries)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetPaletteEntries,UINT ,PaletteNumber,PALETTEENTRY* ,pEntries)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetCurrentTexturePalette,UINT, PaletteNumber)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetCurrentTexturePalette,UINT*,PaletteNumber)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetScissorRect,CONST RECT*, pRect)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetScissorRect,RECT*, pRect)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetSoftwareVertexProcessing,BOOL, bSoftware)
METHOD_THRU( BOOL    , WINAPI , D3DProxyDevice , GetSoftwareVertexProcessing)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetNPatchMode,float, nSegments)
METHOD_THRU( float   , WINAPI , D3DProxyDevice , GetNPatchMode)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetFVF , DWORD , FVF )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetFVF , DWORD* , pFVF )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetVertexShaderConstantI , UINT , StartRegister , CONST int*  , pConstantData , UINT , Vector4iCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetVertexShaderConstantI , UINT , StartRegister , int*        , pConstantData , UINT , Vector4iCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetVertexShaderConstantB , UINT , StartRegister , CONST BOOL* , pConstantData , UINT , BoolCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetVertexShaderConstantB , UINT , StartRegister , BOOL*       , pConstantData , UINT , BoolCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetStreamSourceFreq , UINT , StreamNumber , UINT  , Setting )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetStreamSourceFreq , UINT , StreamNumber , UINT* , pSetting )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetPixelShaderConstantI , UINT , StartRegister , CONST int* , pConstantData , UINT , Vector4iCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetPixelShaderConstantI , UINT , StartRegister , int*       , pConstantData , UINT , Vector4iCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetPixelShaderConstantB , UINT , StartRegister , CONST BOOL*, pConstantData , UINT , BoolCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetPixelShaderConstantB , UINT , StartRegister , BOOL*      , pConstantData , UINT , BoolCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , DeletePatch , UINT , Handle )

