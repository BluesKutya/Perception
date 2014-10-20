#include "StereoView.h"
#include <Streamer.h>
#include "D3D9ProxySurface.h"
#include "D3D9ProxySwapChain.h"
#include <DxErr.h>

StereoView::StereoView( ){
	OutputDebugStringA("Created SteroView\n");
	initialized = false;
	XOffset = 0;

	// set all member pointers to NULL to prevent uninitialized objects being used
	actual = NULL;
	proxy = 0;
	leftTexture = NULL;
	rightTexture = NULL;

	leftSurface = NULL;
	rightSurface = NULL;

	screenVertexBuffer = NULL;
	viewEffect = NULL;

	m_pStreamer = new Streamer( );
}


StereoView::~StereoView(){
	delete m_pStreamer;
}


void StereoView::Init(D3DProxyDevice* d){
	if( initialized ){
		return;
	}

	actual = d->actual;
	proxy  = d;


	if( FAILED(D3DXCreateEffectFromFileA(actual, config.getShaderPath().toLocal8Bit(), NULL, NULL, D3DXFX_DONOTSAVESTATE, NULL, &viewEffect, NULL))) {
		printf( "Vireio: Effect creation failed\n" );
	}

	InitTextureBuffers();
	InitVertexBuffers();
	CalculateShaderVariables();

	initialized = true;
}

/**
* Releases all Direct3D objects.
***/
void StereoView::ReleaseEverything(){
	viewEffect->OnLostDevice();

	SAFE_RELEASE( leftTexture );
	SAFE_RELEASE( rightTexture );
	SAFE_RELEASE( leftSurface );
	SAFE_RELEASE( rightSurface );
	SAFE_RELEASE( backBuffer );
	SAFE_RELEASE( screenVertexBuffer );
	SAFE_RELEASE( viewEffect );
	
	initialized = false;
}




void StereoView::Draw( D3D9ProxySwapChain* chain ){
 	D3D9ProxySurface* backStereo = chain->backBuffers[0];

	IDirect3DSurface9*    leftImage  = backStereo->actual;
	IDirect3DSurface9*    rightImage = backStereo->right;
	D3DXMATRIX	          identity;
	IDirect3DStateBlock9* state;
	float                 resolution[2];
	UINT                  numPasses;

	actual->StretchRect( leftImage                           , 0 , leftSurface  , 0 , D3DTEXF_NONE );
	actual->StretchRect( rightImage ? rightImage : leftImage , 0 , rightSurface , 0 , D3DTEXF_NONE );

	D3DXMatrixIdentity( &identity );

	actual->CreateStateBlock( D3DSBT_ALL , &state );



	actual->SetTransform        ( D3DTS_WORLD            , &identity );
	actual->SetTransform        ( D3DTS_VIEW             , &identity);
	actual->SetTransform        ( D3DTS_PROJECTION       , &identity);
	actual->SetRenderState      ( D3DRS_LIGHTING         , FALSE);
	actual->SetRenderState      ( D3DRS_CULLMODE         , D3DCULL_NONE);
	actual->SetRenderState      ( D3DRS_ZENABLE          , FALSE);
	actual->SetRenderState      ( D3DRS_ZWRITEENABLE     , FALSE);
	actual->SetRenderState      ( D3DRS_ALPHABLENDENABLE , FALSE);
	actual->SetRenderState      ( D3DRS_ALPHATESTENABLE  , FALSE);
	actual->SetRenderState      ( D3DRS_STENCILENABLE    , FALSE); 

	actual->SetTextureStageState( 0 , D3DTSS_COLOROP   , D3DTOP_SELECTARG1 );
	actual->SetTextureStageState( 0 , D3DTSS_COLORARG1 , D3DTA_TEXTURE     );
	actual->SetTextureStageState( 0 , D3DTSS_ALPHAOP   , D3DTOP_SELECTARG1 );
	actual->SetTextureStageState( 0 , D3DTSS_ALPHAARG1 , D3DTA_CONSTANT    );
	actual->SetTextureStageState( 0 , D3DTSS_CONSTANT  , 0xffffffff        );

	actual->SetRenderState( D3DRS_ALPHABLENDENABLE , FALSE );
	actual->SetRenderState( D3DRS_ZENABLE          , D3DZB_FALSE );
	actual->SetRenderState( D3DRS_ZWRITEENABLE     , FALSE );
	actual->SetRenderState( D3DRS_ALPHATESTENABLE  , FALSE );  

	//gamma stuff. maybe keep it?
	actual->SetRenderState ( D3DRS_SRGBWRITEENABLE , 0 );  // will cause visual errors in HL2
	actual->SetSamplerState( 0 , D3DSAMP_SRGBTEXTURE, 0 );
	actual->SetSamplerState( 1 , D3DSAMP_SRGBTEXTURE, 0 );

	actual->SetSamplerState( 0 , D3DSAMP_ADDRESSU , D3DTADDRESS_CLAMP );
	actual->SetSamplerState( 0 , D3DSAMP_ADDRESSV , D3DTADDRESS_CLAMP );
	actual->SetSamplerState( 0 , D3DSAMP_ADDRESSW , D3DTADDRESS_CLAMP );
	actual->SetSamplerState( 1 , D3DSAMP_ADDRESSU , D3DTADDRESS_CLAMP );
	actual->SetSamplerState( 1 , D3DSAMP_ADDRESSV , D3DTADDRESS_CLAMP );
	actual->SetSamplerState( 1 , D3DSAMP_ADDRESSW , D3DTADDRESS_CLAMP );

	// TODO Need to check device capabilities if we want a prefered order of fallback rather than 
	// whatever the default is being used when a mode isn't supported.
	// Example - GeForce 660 doesn't appear to support D3DTEXF_ANISOTROPIC on the MAGFILTER (at least
	// according to the spam of error messages when running with the directx debug runtime)
	actual->SetSamplerState( 0 , D3DSAMP_MAGFILTER , D3DTEXF_ANISOTROPIC );
	actual->SetSamplerState( 1 , D3DSAMP_MAGFILTER , D3DTEXF_ANISOTROPIC );
	actual->SetSamplerState( 0 , D3DSAMP_MINFILTER , D3DTEXF_ANISOTROPIC );
	actual->SetSamplerState( 1 , D3DSAMP_MINFILTER , D3DTEXF_ANISOTROPIC );
	actual->SetSamplerState( 0 , D3DSAMP_MIPFILTER , D3DTEXF_NONE );
	actual->SetSamplerState( 1 , D3DSAMP_MIPFILTER , D3DTEXF_NONE );

	actual->SetVertexShader     ( 0 );
	actual->SetPixelShader      ( 0 );
	actual->SetVertexDeclaration( 0 );
	
	actual->SetFVF( D3DFVF_TEXVERTEX );

	auto prevRt = proxy->storeAndClearRenderTargets();

	IDirect3DSurface9* actualBack = 0;

	chain->actual->GetBackBuffer( 0 , D3DBACKBUFFER_TYPE_MONO , &actualBack );




	if( config.swap_eyes ){
		actual->SetTexture( 0 , leftTexture  );
		actual->SetTexture( 1 , rightTexture );
	}else{
		actual->SetTexture( 0 , rightTexture );
		actual->SetTexture( 1 , leftTexture  );
	}

	if( FAILED(actual->SetRenderTarget( 0 , actualBack )) ) {
		printf( "Virieo: SetRenderTarget backbuffer failed\n" );
	}

	if( FAILED(actual->SetStreamSource( 0 , screenVertexBuffer , 0 , sizeof(TEXVERTEX) )) ){
		printf( "Virieo: SetStreamSource failed\n");
	}

	if( FAILED(viewEffect->SetTechnique("ViewShader")) ){
		printf( "Virieo: SetTechnique failed\n");
	}


	viewEffect->SetInt       ( "viewWidth"       , viewport.Width );
	viewEffect->SetInt       ( "viewHeight"      , viewport.Height );
	viewEffect->SetFloatArray( "LensCenter"      , LensCenter , 2 );
	viewEffect->SetFloatArray( "Scale"           , Scale , 2);
	viewEffect->SetFloatArray( "ScaleIn"         , ScaleIn , 2);
	viewEffect->SetFloatArray( "HmdWarpParam"    , config.distortionCoefficients , 4 );
	viewEffect->SetFloat     ( "ViewportXOffset" , -ViewportXOffset );
	viewEffect->SetFloat     ( "ViewportYOffset" , -ViewportYOffset );

	if( config.chromaticAberrationCorrection ){
		viewEffect->SetFloatArray( "Chroma",  config.chromaCoefficients , 4 );
	}else{
		static float noChroma[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		viewEffect->SetFloatArray( "Chroma" , noChroma , 4 );
	}


	resolution[0] = config.resolutionWidth;
	resolution[1] = config.resolutionHeight;
	viewEffect->SetFloatArray( "Resolution" , resolution , 2 );




	

	if( FAILED(viewEffect->Begin( &numPasses , 0 )) ){
		numPasses = 0;
		printf( "Vireio: Begin failed\n" );
	}

	for( int c=0 ; c<numPasses ; c++ ){
		if( FAILED(viewEffect->BeginPass(c)) ){
			printf( "Vireio: Beginpass failed\n");
		}

		if (FAILED(actual->DrawPrimitive( D3DPT_TRIANGLEFAN , 0 , 2 ))) {
			printf( "Vireio: Draw failed\n");
		}

		if (FAILED(viewEffect->EndPass())) {
			printf( "Vireio: Beginpass failed\n");
		}
	}

	if (FAILED(viewEffect->End())) {
		printf( "Vireio: End failed\n");
	}

	m_pStreamer->send( actual );

	proxy->mirrorUpdate( );

	state->Apply();
	state->Release();

	proxy->restoreRenderTargets( prevRt );

	actualBack->Release();
}




void StereoView::SaveScreen()
{
	static int screenCount = 0;
	++screenCount;

	char fileName[32];
	sprintf(fileName, "%d_final.bmp", screenCount);
	char fileNameLeft[32];
	sprintf(fileNameLeft, "%d_left.bmp", screenCount);
	char fileNameRight[32];
	sprintf(fileNameRight, "%d_right.bmp", screenCount);

#ifdef _DEBUG
	OutputDebugStringA(fileName);
	OutputDebugStringA("\n");
#endif	

	D3DXSaveSurfaceToFileA(fileNameLeft, D3DXIFF_BMP, leftSurface, NULL, NULL);
	D3DXSaveSurfaceToFileA(fileNameRight, D3DXIFF_BMP, rightSurface, NULL, NULL);
	D3DXSaveSurfaceToFileA(fileName, D3DXIFF_BMP, backBuffer, NULL, NULL);
}

/**
* Calls ID3DXEffect::OnResetDevice.
***/
void StereoView::PostReset()
{
	CalculateShaderVariables();
	viewEffect->OnResetDevice();
}

/**
* Inits the left and right texture buffer.
* Also gets viewport data and back buffer render target.
***/
void StereoView::InitTextureBuffers()
{
	actual->GetViewport(&viewport);
	D3DSURFACE_DESC pDesc = D3DSURFACE_DESC();
	actual->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
	backBuffer->GetDesc(&pDesc);

#ifdef _DEBUG
	char buf[32];
	LPCSTR psz = NULL;

	sprintf(buf,"viewport width: %d",viewport.Width);
	psz = buf;
	OutputDebugStringA(psz);
	OutputDebugStringA("\n");

	sprintf(buf,"backbuffer width: %d",pDesc.Width);
	psz = buf;
	OutputDebugStringA(psz);
	OutputDebugStringA("\n");
#endif

	actual->CreateTexture(pDesc.Width, pDesc.Height, 0, D3DUSAGE_RENDERTARGET, pDesc.Format, D3DPOOL_DEFAULT, &leftTexture, NULL);
	leftTexture->GetSurfaceLevel(0, &leftSurface);

	actual->CreateTexture(pDesc.Width, pDesc.Height, 0, D3DUSAGE_RENDERTARGET, pDesc.Format, D3DPOOL_DEFAULT, &rightTexture, NULL);
	rightTexture->GetSurfaceLevel(0, &rightSurface);
}

/**
* Inits a simple full screen vertex buffer containing 4 vertices.
***/
void StereoView::InitVertexBuffers()
{
	OutputDebugStringA("SteroView initVertexBuffers\n");

	HRESULT result = actual->CreateVertexBuffer(sizeof(TEXVERTEX) * 4, 0 ,
		D3DFVF_TEXVERTEX, D3DPOOL_MANAGED , &screenVertexBuffer, NULL);

	if( FAILED(result) ){
		printf("Vireio: SteroView initVertexBuffers failed %s\n" ,  DXGetErrorString(result) );
	}

	TEXVERTEX* vertices;

	screenVertexBuffer->Lock(0, 0, (void**)&vertices, NULL);

	float scale = 1.0f;

	RECT* rDest = new RECT();
	rDest->left = 0;
	rDest->right = int(viewport.Width*scale);
	rDest->top = 0;
	rDest->bottom = int(viewport.Height*scale);

	//Setup vertices
	vertices[0].x = (float) rDest->left - 0.5f;
	vertices[0].y = (float) rDest->top - 0.5f;
	vertices[0].z = 0.0f;
	vertices[0].rhw = 1.0f;
	vertices[0].u = 0.0f;
	vertices[0].v = 0.0f;

	vertices[1].x = (float) rDest->right - 0.5f;
	vertices[1].y = (float) rDest->top - 0.5f;
	vertices[1].z = 0.0f;
	vertices[1].rhw = 1.0f;
	vertices[1].u = 1.0f;
	vertices[1].v = 0.0f;

	vertices[2].x = (float) rDest->right - 0.5f;
	vertices[2].y = (float) rDest->bottom - 0.5f;
	vertices[2].z = 0.0f;
	vertices[2].rhw = 1.0f;
	vertices[2].u = 1.0f;	
	vertices[2].v = 1.0f;

	vertices[3].x = (float) rDest->left - 0.5f;
	vertices[3].y = (float) rDest->bottom - 0.5f;
	vertices[3].z = 0.0f;
	vertices[3].rhw = 1.0f;
	vertices[3].u = 0.0f;
	vertices[3].v = 1.0f;

	screenVertexBuffer->Unlock();
}




/**
* Calculate all vertex shader constants.
***/ 
void StereoView::CalculateShaderVariables() {
	// Center of half screen is 0.25 in x (halfscreen x input in 0 to 0.5 range)
	// Lens offset is in a -1 to 1 range. Using in shader with a 0 to 0.5 range so use 25% of the value.
	LensCenter[0] = 0.25f + (config.lensXCenterOffset * 0.25f) - (config.lensIPDCenterOffset - config.IPDOffset);

	// Center of halfscreen range is 0.5 in y (halfscreen y input in 0 to 1 range)
	LensCenter[1] = config.lensYCenterOffset - config.YOffset;

	
	
	ViewportXOffset = XOffset;
	ViewportYOffset = HeadYOffset;

	D3DSURFACE_DESC eyeTextureDescriptor;
	leftSurface->GetDesc(&eyeTextureDescriptor);

	float inputTextureAspectRatio = (float)eyeTextureDescriptor.Width / (float)eyeTextureDescriptor.Height;
	
	// Note: The range is shifted using the LensCenter in the shader before the scale is applied so you actually end up with a -1 to 1 range
	// in the distortion function rather than the 0 to 2 I mention below.
	// Input texture scaling to sample the 0 to 0.5 x range of the half screen area in the correct aspect ratio in the distortion function
	// x is changed from 0 to 0.5 to 0 to 2.
	ScaleIn[0] = 4.0f;
	// y is changed from 0 to 1 to 0 to 2 and scaled to account for aspect ratio
	ScaleIn[1] = 2.0f / (inputTextureAspectRatio * 0.5f); // 1/2 aspect ratio for differing input ranges
	
	float scaleFactor = (1.0f / (config.scaleToFillHorizontal + config.DistortionScale));

	// Scale from 0 to 2 to 0 to 1  for x and y 
	// Then use scaleFactor to fill horizontal space in line with the lens and adjust for aspect ratio for y.
	Scale[0] = (1.0f / 4.0f) * scaleFactor;
	Scale[1] = (1.0f / 2.0f) * scaleFactor * inputTextureAspectRatio;
} 