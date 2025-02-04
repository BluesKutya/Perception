#pragma once
#define MAX_VRBOOST_VALUES 256
#define BRASSA_PIXEL_WIDTH 1920
#define BRASSA_PIXEL_HEIGHT 1080

#include <Vireio.h>
#include <d3dx9.h>
#include <XInput.h>
#include <memory>
#include <unordered_map>
#include "InputControls.h"
#include "DirectInput.h"
#include <cConfig.h>
#include "cMenu.h"
#include "cShader.h"
#include "cConstantBuffer.h"
#include "cBase.h"


class StereoView;
class D3D9ProxySwapChain;
class ShaderRegisters;
class GameHandler;


class D3DProxyDevice : public cBase<IDirect3DDevice9Ex,IDirect3DDevice9> {
public:


	enum GameTypes{
		MONO = 0,                  /**<  !! */
		FIXED = 10,                /**< Default driver behavior. */
		SOURCE = 100,              /**< Source is a 3D video game engine developed by Valve Corporation. */
		SOURCE_L4D = 101,          /**<  !! */
		SOURCE_ESTER = 102,          /**<  !! */
		UNREAL = 200,              /**< The Unreal Engine is a game engine developed by Epic Games, first illustrated in the 1998 first-person shooter game Unreal. */
		UNREAL_MIRROR = 201,       /**<  !! */
		UNREAL_UT3 = 202,          /**<  !! */
		UNREAL_BIOSHOCK = 203,     /**<  !! */
		UNREAL_BORDERLANDS = 204,  /**< Borderlands(TM) */
		EGO = 300,                 /**< Ego Game Technology Engine (more commonly referred to as Ego Engine or EGO, stylised ego) is a video game engine developed by Codemasters. */
		EGO_DIRT = 301,            /**<  !! */
		REALV = 400,               /**< Real Virtuality is a proprietary computer game engine developed by Bohemia Interactive (BI), originally called Poseidon. */
		REALV_ARMA = 401,          /**<  !! */
		UNITY = 500,               /**< Unity is a cross-platform game engine with a built-in IDE developed by Unity Technologies. */
		UNITY_SLENDER = 501,       /**<  !! */
		GAMEBRYO = 600,            /**< Gamebryo 3D and LightSpeed engines are owned by Gamebase Co., Ltd. and Gamebase USA and have been used by several video game developers. */
		GAMEBRYO_SKYRIM = 601,     /**< Skyrim’s Creation Engine still has at least some Gamebryo in it. */
		LFS = 700,                 /**< Live for Speed (LFS) is a racing simulator developed by a three person team comprising Scawen Roberts, Eric Bailey, and Victor van Vlaardingen. */
		CDC = 800,                 /**< Proprietary game engine developed by Crystal Dynamics. */
	};

	enum FPS_TYPE {
		FPS_NONE,
		FPS_COUNT,
		FPS_TIME
	};


	StereoView*    stereoView;
	cTracker*      tracker;

	bool           m_bfloatingMenu;
	float          m_fFloatingPitch;
	float          m_fFloatingYaw;
		           
	bool           m_bfloatingScreen;
	float          m_fFloatingScreenPitch;
	float          m_fFloatingScreenYaw;
	float          m_fFloatingScreenZ;

	InputControls  controls;
	DirectInput    dinput;

	
	bool                                      m_bActiveViewportIsDefault;
	bool                                      m_bViewportIsSquished;
	D3DVIEWPORT9                              m_LastViewportSet;
	D3DVIEWPORT9                              m_ViewportIfSquished;

	cPtr<D3D9ProxySurface>                    activeStereoDepthStencil;
	cPtr<D3D9ProxyIndexBuffer>                activeIndicies;
	cPtr<D3D9ProxyVertexDeclaration>          activeVertexDeclaration;
	std::vector<cPtr<D3D9ProxySwapChain>>     activeSwapChains;
	std::vector<cPtr<D3D9ProxySurface>>       activeRenderTargets;	
	std::map<int,cPtr<IDirect3DBaseTexture9>> activeTextures;
	std::map<int,cPtr<D3D9ProxyVertexBuffer>> activeVertexes;
	cPtr<D3D9ProxyVertexShader>               activeVertexShader;
	cPtr<D3D9ProxyPixelShader>                activePixelShader;

	bool                            calibrate_tracker;
	vireio::RenderPosition          m_currentRenderingSide;
	D3DXMATRIX*                     m_pCurrentMatViewTransform;
	FPS_TYPE                        show_fps;
	int                             viewportWidth;
	int                             viewportHeight;
	bool                            m_isFirstBeginSceneOfFrame;
	bool                            m_bVRBoostToggle;
	float                           m_fVRBoostIndicator;
	UINT                            m_VertexShaderCount;
	UINT                            m_VertexShaderCountLastFrame;
	float                           fps;
	
	bool                            m_bInBeginEndStateBlock;

	bool                            transformViewSet;
	D3DXMATRIX                      transformViewOriginal;
	D3DXMATRIX                      transformViewLeft;
	D3DXMATRIX                      transformViewRight;

	bool                            transformProjSet;
	D3DXMATRIX                      transformProjLeft;
	D3DXMATRIX                      transformProjRight;
	D3DXMATRIX                      transformProjOriginal;

	int                             screenshot;
	IDirect3DDevice9Ex*             actualEx;
	D3D9ProxyDirect3D*              m_pCreatedBy;
	ULONG                           m_nRefCount;


	HWND                      mirrorWindow;
	cPtr<IDirect3DSwapChain9> mirrorSwap;
	void mirrorInit();
	void mirrorUpdate();
	void mirrorFree();
	
	HWND                          windowHandle;
	D3D9ProxyStateBlock*          stateBlock;

	
	QList<cShader*>               shaders;
	cMenuItem*                    shadersMenu;
	cMenu                         menu;


	/****    Shader and constant storage   ****/
	cConstantBuffer                    vsConstants;
	cConstantBuffer                    psConstants;
	std::vector<cRegisterModification> vsDefaultModifications;
	std::vector<cRegisterModification> psDefaultModifications;


	/****    Constant modification   ****/
	bool  rulesZDisable;
	DWORD rulesZPrev;

	cMenuItem*      rulesMenu;
	void            rulesAdd     ( );
	void            rulesDelete  ( cRule& );
	void            rulesInit    ( );
	void            rulesUpdate  ( );
	void            rulesApply   ( );
	void            rulesPreDraw ( );
	void            rulesPostDraw( );

	
	/****    VRBoost ****/
	
	FARPROC  vrbFuncLoadMemoryRules;
	FARPROC  vrbFuncSaveMemoryRules;
	FARPROC  vrbFuncCreateFloatMemoryRule;
	FARPROC  vrbFuncSetProcess;
	FARPROC  vrbFuncReleaseAllMemoryRules;
	FARPROC  vrbFuncApplyMemoryRules;
	HMODULE  vrbLib;
	bool     vrbRulesLoaded;
	float    vrbValues[MAX_VRBOOST_VALUES];

	void     vrbInit();
	void     vrbFree();
	void     vrbUpdate();
	void     vrbLoadValues( );
	void     vrbSaveValues( );



	/****    View modification   ****/

	float       viewProjMinZ;
	float       viewProjMaxZ;
	float       viewProjMinX;
	float       viewProjMaxX;
	float       viewProjMinY;
	float       viewProjMaxY;

	D3DXVECTOR3 viewVecPositionTransform;
	D3DXVECTOR3 viewVecGameScale;
	D3DXMATRIX  viewMatPosition;
	D3DXMATRIX  viewMatProjection;
	D3DXMATRIX  viewMatProjectionInv;
	D3DXMATRIX  viewMatProjectLeft;
	D3DXMATRIX  viewMatProjectRight;
	D3DXMATRIX  viewMatRoll;
	D3DXMATRIX  viewMatRollNegative;
	D3DXMATRIX  viewMatRollHalf;
	D3DXMATRIX  viewMatTransformLeft;
	D3DXMATRIX  viewMatTransformRight;
	D3DXMATRIX  viewMatViewProjLeft;
	D3DXMATRIX  viewMatViewProjRight;
	D3DXMATRIX  viewMatViewProjTransformLeft;
	D3DXMATRIX  viewMatViewProjTransformRight;
	D3DXMATRIX  viewMatViewProjTransformLeftNoRoll;
	D3DXMATRIX  viewMatViewProjTransformRightNoRoll;
	D3DXMATRIX  viewMatGatheredLeft;
	D3DXMATRIX  viewMatGatheredRight;
	D3DXMATRIX  viewMatHudLeft;
	D3DXMATRIX  viewMatHudRight;
	D3DXMATRIX  viewMatGuiLeft;
	D3DXMATRIX  viewMatGuiRight;
	D3DXMATRIX  viewMatSquash;
	D3DXMATRIX  viewMatHudDistance;
	D3DXMATRIX  viewMatLeftHud3DDepth;
	D3DXMATRIX  viewMatRightHud3DDepth;
	D3DXMATRIX  viewMatLeftHud3DDepthShifted;
	D3DXMATRIX  viewMatRightHud3DDepthShifted;
	D3DXMATRIX  viewMatLeftGui3DDepth;
	D3DXMATRIX  viewMatRightGui3DDepth;
	D3DXMATRIX  viewMatBulletLabyrinth;
	
	void        viewInit                    ( );
	void        viewUpdateProjectionMatrices( );
	void        viewComputeTransforms       ( );
	void        viewUpdateRotation          ( float pitch , float yaw , float roll );
	void        viewUpdatePosition          ( float pitch , float yaw , float roll , float x , float y , float z );
	void        viewComputeGui              ( );

	
	
	bool isDrawHide    ( );




	float convergenceInWorldUnits( );
	float separationInWorldUnits ( );
	float separationIPDAdjustment( );







	std::vector<cPtr<D3D9ProxySurface>>  storeAndClearRenderTargets();
	void                                 restoreRenderTargets( std::vector<cPtr<D3D9ProxySurface>>& list );




	D3DProxyDevice(IDirect3DDevice9* pDevice,IDirect3DDevice9Ex* pDeviceEx, D3D9ProxyDirect3D* pCreatedBy );
	~D3DProxyDevice();


	void    ProxyPresent( D3D9ProxySwapChain* swapChain );
	HRESULT ProxyReset( D3DPRESENT_PARAMETERS* pPresentationParameters , D3DDISPLAYMODEEX* pFullscreenDisplayMode , bool useEx );
	HRESULT ProxyCreateOffscreenPlainSurface(UINT Width , UINT Height , D3DFORMAT Format , D3DPOOL Pool , IDirect3DSurface9** ppSurface , HANDLE* pSharedHandle , DWORD Usage , bool useEx );
	HRESULT ProxyCreateRenderTarget         (UINT Width , UINT Height , D3DFORMAT Format , D3DMULTISAMPLE_TYPE MultiSample , DWORD MultisampleQuality , BOOL Lockable , IDirect3DSurface9** ppSurface , HANDLE* pSharedHandle , DWORD Usage , bool isSwapChainBackBuffer , bool useEx );
	HRESULT ProxyCreateDepthStencilSurface  (UINT Width , UINT Height , D3DFORMAT Format , D3DMULTISAMPLE_TYPE MultiSample , DWORD MultisampleQuality , BOOL Discard , IDirect3DSurface9** ppSurface , HANDLE* pSharedHandle ,  DWORD Usage , bool useEx );


	/*** D3DProxyDevice public methods ***/
	
	void           SetupHUD();
	virtual void   HandleControls(void);
	void           HandleTracking(void);
	void           HandleUpdateExtern();
	void           SaveConfiguration();
	void BRASSA_UpdateDeviceSettings();

	/**
	* Game Types.
	* We use these ProxyTypes to determine either to draw BRASSA in EndScene() or in Present().
	* Will be also used for any game- or engine-specific things.
	***/


	/*** D3DProxyDevice protected methods ***/
	void OnCreateOrRestore  ( );	
	bool setDrawingSide     ( vireio::RenderPosition side );
	bool switchDrawingSide  ( );
	void BRASSA_UpdateBorder( );



	/*** D3DProxyDevice private methods ***/
	void    ReleaseEverything();
	bool    isViewportDefaultForMainRT(CONST D3DVIEWPORT9* pViewport);
	HRESULT SetStereoViewTransform(D3DXMATRIX pLeftMatrix, D3DXMATRIX pRightMatrix, bool apply);
	HRESULT SetStereoProjectionTransform(D3DXMATRIX pLeftMatrix, D3DXMATRIX pRightMatrix, bool apply);
	void    SetGUIViewport();
	float   RoundBrassaValue(float val);
	bool	InitVRBoost();

	//Calculate FPS, called every Present
	
	float CalcFPS();










	/*** IDirect3DDevice9 methods ***/
	HRESULT WINAPI TestCooperativeLevel();
	UINT    WINAPI GetAvailableTextureMem();
	HRESULT WINAPI EvictManagedResources();
	HRESULT WINAPI GetDirect3D(IDirect3D9** ppD3D9);
	HRESULT WINAPI GetDeviceCaps(D3DCAPS9* pCaps);
	HRESULT WINAPI GetDisplayMode(UINT iSwapChain,D3DDISPLAYMODE* pMode);
	HRESULT WINAPI GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters);
	HRESULT WINAPI SetCursorProperties(UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap);
	void    WINAPI SetCursorPosition(int X,int Y,DWORD Flags);
	BOOL    WINAPI ShowCursor(BOOL bShow);
	HRESULT WINAPI CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain);
	HRESULT WINAPI GetSwapChain(UINT iSwapChain,IDirect3DSwapChain9** pSwapChain);
	UINT    WINAPI GetNumberOfSwapChains();
	HRESULT WINAPI Reset(D3DPRESENT_PARAMETERS* pPresentationParameters);
	HRESULT WINAPI Present(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	HRESULT WINAPI GetBackBuffer(UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer);
	HRESULT WINAPI GetRasterStatus(UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus);
	HRESULT WINAPI SetDialogBoxMode(BOOL bEnableDialogs);
	void    WINAPI SetGammaRamp(UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp);
	void    WINAPI GetGammaRamp(UINT iSwapChain,D3DGAMMARAMP* pRamp);
	HRESULT WINAPI CreateTexture(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle);
	HRESULT WINAPI CreateVolumeTexture(UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle);
	HRESULT WINAPI CreateCubeTexture(UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle);
	HRESULT WINAPI CreateVertexBuffer(UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle);
	HRESULT WINAPI CreateIndexBuffer(UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle);
	HRESULT WINAPI CreateRenderTarget(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	HRESULT WINAPI CreateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	HRESULT WINAPI UpdateSurface(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint);
	HRESULT WINAPI UpdateTexture(IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture);
	HRESULT WINAPI GetRenderTargetData(IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface);
	HRESULT WINAPI GetFrontBufferData(UINT iSwapChain,IDirect3DSurface9* pDestSurface);
	HRESULT WINAPI StretchRect(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter);
	HRESULT WINAPI ColorFill(IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color);
	HRESULT WINAPI CreateOffscreenPlainSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	HRESULT WINAPI SetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget);
	HRESULT WINAPI GetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget);
	HRESULT WINAPI SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil);
	HRESULT WINAPI GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface);
	HRESULT WINAPI BeginScene();
	HRESULT WINAPI EndScene();
	HRESULT WINAPI Clear(DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
	HRESULT WINAPI SetTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
	HRESULT WINAPI GetTransform(D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix);
	HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
	HRESULT WINAPI SetViewport(CONST D3DVIEWPORT9* pViewport);
	HRESULT WINAPI GetViewport(D3DVIEWPORT9* pViewport);
	HRESULT WINAPI SetMaterial(CONST D3DMATERIAL9* pMaterial);
	HRESULT WINAPI GetMaterial(D3DMATERIAL9* pMaterial);
	HRESULT WINAPI SetLight(DWORD Index,CONST D3DLIGHT9* pLight);
	HRESULT WINAPI GetLight(DWORD Index,D3DLIGHT9* pLight);
	HRESULT WINAPI LightEnable(DWORD Index,BOOL Enable);
	HRESULT WINAPI GetLightEnable(DWORD Index,BOOL* pEnable);
	HRESULT WINAPI SetClipPlane(DWORD Index,CONST float* pPlane);
	HRESULT WINAPI GetClipPlane(DWORD Index,float* pPlane);
	HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE State,DWORD Value);
	HRESULT WINAPI GetRenderState(D3DRENDERSTATETYPE State,DWORD* pValue);
	HRESULT WINAPI CreateStateBlock(D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB);
	HRESULT WINAPI BeginStateBlock();
	HRESULT WINAPI EndStateBlock(IDirect3DStateBlock9** ppSB);
	HRESULT WINAPI SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus);
	HRESULT WINAPI GetClipStatus(D3DCLIPSTATUS9* pClipStatus);
	HRESULT WINAPI GetTexture(DWORD Stage,IDirect3DBaseTexture9** ppTexture);
	HRESULT WINAPI SetTexture(DWORD Stage,IDirect3DBaseTexture9* pTexture);
	HRESULT WINAPI GetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue);
	HRESULT WINAPI SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);
	HRESULT WINAPI GetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue);
	HRESULT WINAPI SetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);
	HRESULT WINAPI ValidateDevice(DWORD* pNumPasses);
	HRESULT WINAPI SetPaletteEntries(UINT PaletteNumber,CONST PALETTEENTRY* pEntries);
	HRESULT WINAPI GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries);
	HRESULT WINAPI SetCurrentTexturePalette(UINT PaletteNumber);
	HRESULT WINAPI GetCurrentTexturePalette(UINT *PaletteNumber);
	HRESULT WINAPI SetScissorRect(CONST RECT* pRect);
	HRESULT WINAPI GetScissorRect(RECT* pRect);
	HRESULT WINAPI SetSoftwareVertexProcessing(BOOL bSoftware);
	BOOL    WINAPI GetSoftwareVertexProcessing();
	HRESULT WINAPI SetNPatchMode(float nSegments);
	float   WINAPI GetNPatchMode();
	HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
	HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount);
	HRESULT WINAPI DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	HRESULT WINAPI DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	HRESULT WINAPI ProcessVertices(UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags);
	HRESULT WINAPI CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl);
	HRESULT WINAPI SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl);
	HRESULT WINAPI GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl);
	HRESULT WINAPI SetFVF(DWORD FVF);
	HRESULT WINAPI GetFVF(DWORD* pFVF);
	HRESULT WINAPI CreateVertexShader(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader);
	HRESULT WINAPI SetVertexShader(IDirect3DVertexShader9* pShader);
	HRESULT WINAPI GetVertexShader(IDirect3DVertexShader9** ppShader);
	HRESULT WINAPI SetVertexShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	HRESULT WINAPI GetVertexShaderConstantF(UINT StartRegister,float* pConstantData,UINT Vector4fCount);
	HRESULT WINAPI SetVertexShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
	HRESULT WINAPI GetVertexShaderConstantI(UINT StartRegister,int* pConstantData,UINT Vector4iCount);
	HRESULT WINAPI SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
	HRESULT WINAPI GetVertexShaderConstantB(UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
	HRESULT WINAPI SetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride);
	HRESULT WINAPI GetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride);
	HRESULT WINAPI SetStreamSourceFreq(UINT StreamNumber,UINT Setting);
	HRESULT WINAPI GetStreamSourceFreq(UINT StreamNumber,UINT* pSetting);
	HRESULT WINAPI SetIndices(IDirect3DIndexBuffer9* pIndexData);
	HRESULT WINAPI GetIndices(IDirect3DIndexBuffer9** ppIndexData);
	HRESULT WINAPI CreatePixelShader(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader);
	HRESULT WINAPI SetPixelShader(IDirect3DPixelShader9* pShader);
	HRESULT WINAPI GetPixelShader(IDirect3DPixelShader9** ppShader);
	HRESULT WINAPI SetPixelShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	HRESULT WINAPI GetPixelShaderConstantF(UINT StartRegister,float* pConstantData,UINT Vector4fCount);
	HRESULT WINAPI SetPixelShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
	HRESULT WINAPI GetPixelShaderConstantI(UINT StartRegister,int* pConstantData,UINT Vector4iCount);
	HRESULT WINAPI SetPixelShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
	HRESULT WINAPI GetPixelShaderConstantB(UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
	HRESULT WINAPI DrawRectPatch(UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo);
	HRESULT WINAPI DrawTriPatch(UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo);
	HRESULT WINAPI DeletePatch(UINT Handle);
	HRESULT WINAPI CreateQuery(D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery);

	// IDirect3DDevice9Ex methods
	HRESULT WINAPI SetConvolutionMonoKernel(UINT width,UINT height,float* rows,float* columns);
    HRESULT WINAPI ComposeRects(IDirect3DSurface9* pSrc,IDirect3DSurface9* pDst,IDirect3DVertexBuffer9* pSrcRectDescs,UINT NumRects,IDirect3DVertexBuffer9* pDstRectDescs,D3DCOMPOSERECTSOP Operation,int Xoffset,int Yoffset);
    HRESULT WINAPI PresentEx(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags);
    HRESULT WINAPI GetGPUThreadPriority(INT* pPriority);
    HRESULT WINAPI SetGPUThreadPriority(INT Priority);
    HRESULT WINAPI WaitForVBlank(UINT iSwapChain);
    HRESULT WINAPI CheckResourceResidency(IDirect3DResource9** pResourceArray,UINT32 NumResources);
    HRESULT WINAPI SetMaximumFrameLatency(UINT MaxLatency);
    HRESULT WINAPI GetMaximumFrameLatency(UINT* pMaxLatency);
    HRESULT WINAPI CheckDeviceState(HWND hDestinationWindow);
    HRESULT WINAPI CreateRenderTargetEx(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage);
    HRESULT WINAPI CreateOffscreenPlainSurfaceEx(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage);
    HRESULT WINAPI CreateDepthStencilSurfaceEx(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage);
    HRESULT WINAPI ResetEx(D3DPRESENT_PARAMETERS* pPresentationParameters,D3DDISPLAYMODEEX *pFullscreenDisplayMode);
    HRESULT WINAPI GetDisplayModeEx(UINT iSwapChain,D3DDISPLAYMODEEX* pMode,D3DDISPLAYROTATION* pRotation);
};
