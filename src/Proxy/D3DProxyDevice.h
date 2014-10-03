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


class StereoView;
class D3D9ProxySwapChain;
class ShaderRegisters;
class GameHandler;


class D3DProxyDevice : public IDirect3DDevice9Ex {
public:




	D3D9ProxyPixelShader*  currentPS;
	D3D9ProxyVertexShader* currentVS;
	
	QList<cShader*>        shaders;
	cMenuItem*             shadersMenu;
	HWND                   windowHandle;

	bool isDrawHide    ( );
	

	cConstantBuffer vsConstantsOriginal;
	cConstantBuffer vsConstantsLeft;
	cConstantBuffer vsConstantsRight;
	cConstantBuffer psConstants;

	QList<cRule*>   rules;
	cMenuItem*      rulesMenu;
	cRule*          rulesAdd   ( );
	void            rulesDelete( cRule* );
	void            rulesInit  ( );
	void            rulesFree  ( );
	void            rulesSave  ( );
	void            rulesLoad  ( );
	void            rulesUpdate( );
	void            rulesApply ( );

	




	float viewProjMinZ;
	float viewProjMaxZ;
	float viewProjMinX;
	float viewProjMaxX;
	float viewProjMinY;
	float viewProjMaxY;

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
	
	void  viewInit( );
	void  viewUpdateProjectionMatrices( );
	void  viewComputeTransforms       ( );
	void  viewUpdateRotation          ( float pitch , float yaw , float roll );
	void  viewUpdatePosition          ( float pitch , float yaw , float roll , float x , float y , float z );
	void  viewComputeGui               ( );


	float convergenceInWorldUnits( );
	float separationInWorldUnits ( );
	float separationIPDAdjustment( );


	bool  gameShouldDuplicateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality,BOOL Lockable, bool isSwapChainBackBuffer);
	bool  gameShouldDuplicateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard);
	bool  gameShouldDuplicateTexture(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool);
	bool  gameShouldDuplicateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool);











	D3DProxyDevice(IDirect3DDevice9* pDevice,IDirect3DDevice9Ex* pDeviceEx, D3D9ProxyDirect3D* pCreatedBy );
	virtual ~D3DProxyDevice();

	friend class D3D9ProxyStateBlock;

	/*** IUnknown methods ***/
	virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppv);
	virtual ULONG   WINAPI AddRef();
	virtual ULONG   WINAPI Release();

	/*** IDirect3DDevice9 methods ***/
	virtual HRESULT WINAPI TestCooperativeLevel();
	virtual UINT    WINAPI GetAvailableTextureMem();
	virtual HRESULT WINAPI EvictManagedResources();
	virtual HRESULT WINAPI GetDirect3D(IDirect3D9** ppD3D9);
	virtual HRESULT WINAPI GetDeviceCaps(D3DCAPS9* pCaps);
	virtual HRESULT WINAPI GetDisplayMode(UINT iSwapChain,D3DDISPLAYMODE* pMode);
	virtual HRESULT WINAPI GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters);
	virtual HRESULT WINAPI SetCursorProperties(UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap);
	virtual void    WINAPI SetCursorPosition(int X,int Y,DWORD Flags);
	virtual BOOL    WINAPI ShowCursor(BOOL bShow);
	virtual HRESULT WINAPI CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain);
	virtual HRESULT WINAPI GetSwapChain(UINT iSwapChain,IDirect3DSwapChain9** pSwapChain);
	virtual UINT    WINAPI GetNumberOfSwapChains();
	virtual HRESULT WINAPI Reset(D3DPRESENT_PARAMETERS* pPresentationParameters);
	virtual HRESULT WINAPI Present(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	virtual HRESULT WINAPI GetBackBuffer(UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer);
	virtual HRESULT WINAPI GetRasterStatus(UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus);
	virtual HRESULT WINAPI SetDialogBoxMode(BOOL bEnableDialogs);
	virtual void    WINAPI SetGammaRamp(UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp);
	virtual void    WINAPI GetGammaRamp(UINT iSwapChain,D3DGAMMARAMP* pRamp);
	virtual HRESULT WINAPI CreateTexture(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle);
	virtual HRESULT WINAPI CreateVolumeTexture(UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle);
	virtual HRESULT WINAPI CreateCubeTexture(UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle);
	virtual HRESULT WINAPI CreateVertexBuffer(UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle);
	virtual HRESULT WINAPI CreateIndexBuffer(UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle);
	virtual HRESULT WINAPI CreateRenderTarget(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	virtual HRESULT WINAPI CreateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	virtual HRESULT WINAPI UpdateSurface(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint);
	virtual HRESULT WINAPI UpdateTexture(IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture);
	virtual HRESULT WINAPI GetRenderTargetData(IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface);
	virtual HRESULT WINAPI GetFrontBufferData(UINT iSwapChain,IDirect3DSurface9* pDestSurface);
	virtual HRESULT WINAPI StretchRect(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter);
	virtual HRESULT WINAPI ColorFill(IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color);
	virtual HRESULT WINAPI CreateOffscreenPlainSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	virtual HRESULT WINAPI SetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget);
	virtual HRESULT WINAPI GetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget);
	virtual HRESULT WINAPI SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil);
	virtual HRESULT WINAPI GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface);
	virtual HRESULT WINAPI BeginScene();
	virtual HRESULT WINAPI EndScene();
	virtual HRESULT WINAPI Clear(DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
	virtual HRESULT WINAPI SetTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
	virtual HRESULT WINAPI GetTransform(D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix);
	virtual HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
	virtual HRESULT WINAPI SetViewport(CONST D3DVIEWPORT9* pViewport);
	virtual HRESULT WINAPI GetViewport(D3DVIEWPORT9* pViewport);
	virtual HRESULT WINAPI SetMaterial(CONST D3DMATERIAL9* pMaterial);
	virtual HRESULT WINAPI GetMaterial(D3DMATERIAL9* pMaterial);
	virtual HRESULT WINAPI SetLight(DWORD Index,CONST D3DLIGHT9* pLight);
	virtual HRESULT WINAPI GetLight(DWORD Index,D3DLIGHT9* pLight);
	virtual HRESULT WINAPI LightEnable(DWORD Index,BOOL Enable);
	virtual HRESULT WINAPI GetLightEnable(DWORD Index,BOOL* pEnable);
	virtual HRESULT WINAPI SetClipPlane(DWORD Index,CONST float* pPlane);
	virtual HRESULT WINAPI GetClipPlane(DWORD Index,float* pPlane);
	virtual HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE State,DWORD Value);
	virtual HRESULT WINAPI GetRenderState(D3DRENDERSTATETYPE State,DWORD* pValue);
	virtual HRESULT WINAPI CreateStateBlock(D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB);
	virtual HRESULT WINAPI BeginStateBlock();
	virtual HRESULT WINAPI EndStateBlock(IDirect3DStateBlock9** ppSB);
	virtual HRESULT WINAPI SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus);
	virtual HRESULT WINAPI GetClipStatus(D3DCLIPSTATUS9* pClipStatus);
	virtual HRESULT WINAPI GetTexture(DWORD Stage,IDirect3DBaseTexture9** ppTexture);
	virtual HRESULT WINAPI SetTexture(DWORD Stage,IDirect3DBaseTexture9* pTexture);
	virtual HRESULT WINAPI GetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue);
	virtual HRESULT WINAPI SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);
	virtual HRESULT WINAPI GetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue);
	virtual HRESULT WINAPI SetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);
	virtual HRESULT WINAPI ValidateDevice(DWORD* pNumPasses);
	virtual HRESULT WINAPI SetPaletteEntries(UINT PaletteNumber,CONST PALETTEENTRY* pEntries);
	virtual HRESULT WINAPI GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries);
	virtual HRESULT WINAPI SetCurrentTexturePalette(UINT PaletteNumber);
	virtual HRESULT WINAPI GetCurrentTexturePalette(UINT *PaletteNumber);
	virtual HRESULT WINAPI SetScissorRect(CONST RECT* pRect);
	virtual HRESULT WINAPI GetScissorRect(RECT* pRect);
	virtual HRESULT WINAPI SetSoftwareVertexProcessing(BOOL bSoftware);
	virtual BOOL    WINAPI GetSoftwareVertexProcessing();
	virtual HRESULT WINAPI SetNPatchMode(float nSegments);
	virtual float   WINAPI GetNPatchMode();
	virtual HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
	virtual HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount);
	virtual HRESULT WINAPI DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	virtual HRESULT WINAPI DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	virtual HRESULT WINAPI ProcessVertices(UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags);
	virtual HRESULT WINAPI CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl);
	virtual HRESULT WINAPI SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl);
	virtual HRESULT WINAPI GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl);
	virtual HRESULT WINAPI SetFVF(DWORD FVF);
	virtual HRESULT WINAPI GetFVF(DWORD* pFVF);
	virtual HRESULT WINAPI CreateVertexShader(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader);
	virtual HRESULT WINAPI SetVertexShader(IDirect3DVertexShader9* pShader);
	virtual HRESULT WINAPI GetVertexShader(IDirect3DVertexShader9** ppShader);
	virtual HRESULT WINAPI SetVertexShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	virtual HRESULT WINAPI GetVertexShaderConstantF(UINT StartRegister,float* pConstantData,UINT Vector4fCount);
	virtual HRESULT WINAPI SetVertexShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
	virtual HRESULT WINAPI GetVertexShaderConstantI(UINT StartRegister,int* pConstantData,UINT Vector4iCount);
	virtual HRESULT WINAPI SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
	virtual HRESULT WINAPI GetVertexShaderConstantB(UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
	virtual HRESULT WINAPI SetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride);
	virtual HRESULT WINAPI GetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride);
	virtual HRESULT WINAPI SetStreamSourceFreq(UINT StreamNumber,UINT Setting);
	virtual HRESULT WINAPI GetStreamSourceFreq(UINT StreamNumber,UINT* pSetting);
	virtual HRESULT WINAPI SetIndices(IDirect3DIndexBuffer9* pIndexData);
	virtual HRESULT WINAPI GetIndices(IDirect3DIndexBuffer9** ppIndexData);
	virtual HRESULT WINAPI CreatePixelShader(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader);
	virtual HRESULT WINAPI SetPixelShader(IDirect3DPixelShader9* pShader);
	virtual HRESULT WINAPI GetPixelShader(IDirect3DPixelShader9** ppShader);
	virtual HRESULT WINAPI SetPixelShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	virtual HRESULT WINAPI GetPixelShaderConstantF(UINT StartRegister,float* pConstantData,UINT Vector4fCount);
	virtual HRESULT WINAPI SetPixelShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
	virtual HRESULT WINAPI GetPixelShaderConstantI(UINT StartRegister,int* pConstantData,UINT Vector4iCount);
	virtual HRESULT WINAPI SetPixelShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
	virtual HRESULT WINAPI GetPixelShaderConstantB(UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
	virtual HRESULT WINAPI DrawRectPatch(UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo);
	virtual HRESULT WINAPI DrawTriPatch(UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo);
	virtual HRESULT WINAPI DeletePatch(UINT Handle);
	virtual HRESULT WINAPI CreateQuery(D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery);



	
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


	void    ProxyPresent( );
	HRESULT ProxyReset( D3DPRESENT_PARAMETERS* pPresentationParameters , D3DDISPLAYMODEEX* pFullscreenDisplayMode , bool useEx );
	HRESULT ProxyCreateOffscreenPlainSurface(UINT Width , UINT Height , D3DFORMAT Format , D3DPOOL Pool , IDirect3DSurface9** ppSurface , HANDLE* pSharedHandle , DWORD Usage , bool useEx );
	HRESULT ProxyCreateRenderTarget         (UINT Width , UINT Height , D3DFORMAT Format , D3DMULTISAMPLE_TYPE MultiSample , DWORD MultisampleQuality , BOOL Lockable , IDirect3DSurface9** ppSurface , HANDLE* pSharedHandle , DWORD Usage , bool isSwapChainBackBuffer , bool useEx );
	HRESULT ProxyCreateDepthStencilSurface  (UINT Width , UINT Height , D3DFORMAT Format , D3DMULTISAMPLE_TYPE MultiSample , DWORD MultisampleQuality , BOOL Discard , IDirect3DSurface9** ppSurface , HANDLE* pSharedHandle ,  DWORD Usage , bool useEx );

	/*** BaseDirect3DDevice9 methods ***/
	IDirect3DDevice9* getActual();

	/*** D3DProxyDevice public methods ***/
	
	void           SetupHUD();
	virtual void   HandleControls(void);
	void           HandleTracking(void);
	void           HandleUpdateExtern();
	void           SaveConfiguration();


	/**
	* Game Types.
	* We use these ProxyTypes to determine either to draw BRASSA in EndScene() or in Present().
	* Will be also used for any game- or engine-specific things.
	***/
	enum ProxyTypes
	{
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
		DEBUG_LOG_FILE = 99999     /**< Debug log file output game type. For development causes. Do not use since slows down game extremely. */
	};
	/**
	* Mode of the BRASSA menu.
	*
	***/
	enum BRASSA_Modes
	{
		INACTIVE = 0,
		MAINMENU = 1,
		WORLD_SCALE_CALIBRATION,
		CONVERGENCE_ADJUSTMENT,
		SHADER_ANALYZER,
		HUD_CALIBRATION,
		GUI_CALIBRATION,
		OVERALL_SETTINGS,
		VRBOOST_VALUES,
		POS_TRACKING_SETTINGS,
		BRASSA_SHADER_ANALYZER_SUBMENU,
		CHANGE_RULES_SCREEN,
		PICK_RULES_SCREEN,
		SHOW_SHADERS_SCREEN,
		BRASSA_ENUM_RANGE
	};
	/**
	* HUD scale enumeration.
	* ENUM_RANGE = range of the enum
	***/
	enum HUD_3D_Depth_Modes
	{
		HUD_DEFAULT = 0,
		HUD_SMALL = 1,
		HUD_LARGE = 2,
		HUD_FULL = 3,
		HUD_ENUM_RANGE = 4
	};
	/**
	* GUI scale enumeration.
	* ENUM_RANGE = range of the enum
	***/
	enum GUI_3D_Depth_Modes
	{
		GUI_DEFAULT = 0,
		GUI_SMALL = 1,
		GUI_LARGE = 2,
		GUI_FULL = 3,
		GUI_ENUM_RANGE = 4
	};

	/**
	* VRBoost values. 
	* Set to public for future use in input device classes.
	***/
	float VRBoostValue[MAX_VRBOOST_VALUES];
	/**
	* Currently not used.
	***/
	float* currentMatrix;
	/**
	* View translation settings (yaw - 0 disabled, 1 enabled).
	**/
	int yaw_mode;			
	/**
	* View translation settings (pitch - 0 disabled, 1 enabled).
	**/
	int pitch_mode;			
	/**
	* Currently not used (For head translation).
	**/
	int translation_mode;	
	/**
	* Currently not used, eye shutter side from old code.
	**/
	int eyeShutter;
	/**
	* Currently not used aspect ratio.
	**/
	float aspectRatio;	
	/**
	* The chosen stereo renderer.
	* @see StereoView
	**/
	StereoView* stereoView;
	/**
	* The chosen motion tracker.
	* @see MotionTracker
	**/
	cTracker* tracker;


	/**
	* True floating GUI mode activated + Reset Values
	**/
	bool m_bfloatingMenu;
	float m_fFloatingPitch;
	float m_fFloatingYaw;
	
	/**
	* floating screen activated
	**/
	bool m_bfloatingScreen;
	float m_fFloatingScreenPitch;
	float m_fFloatingScreenYaw;
	InputControls controls;
	DirectInput dinput;


	/*** D3DProxyDevice protected methods ***/
	virtual void OnCreateOrRestore();	
	virtual bool setDrawingSide(vireio::RenderPosition side);
	bool         switchDrawingSide();
	bool         addRule(std::string constantName, bool allowPartialNameMatch, UINT startRegIndex, D3DXPARAMETER_CLASS constantType, UINT operationToApply, bool transpose);
	bool         modifyRule(std::string constantName, UINT operationToApply, bool transpose);
	bool         deleteRule(std::string constantName);
	void         saveShaderRules();
	void         ClearRect(vireio::RenderPosition renderPosition, D3DRECT rect, D3DCOLOR color);
	void         ClearEmptyRect(vireio::RenderPosition renderPosition, D3DRECT rect, D3DCOLOR color, int bw);
	void         DrawSelection(vireio::RenderPosition renderPosition, D3DRECT rect, D3DCOLOR color, int selectionIndex, int selectionRange);
	void         DrawScrollbar(vireio::RenderPosition renderPosition, D3DRECT rect, D3DCOLOR color, float scroll, int scrollbarSize);
	void         DrawTextShadowed(ID3DXFont* font, LPD3DXSPRITE sprite, LPCSTR lpchText, int cchText, LPRECT lprc, UINT format, D3DCOLOR color);

	void         BRASSA_NewFrame(UINT &entryID, UINT menuEntryCount);
	virtual void BRASSA_ShaderSubMenu(){}
	virtual void BRASSA_ChangeRules(){}
	virtual void BRASSA_PickRules(){}
	virtual void BRASSA_ShowActiveShaders(){}

	/** Whether the Frames Per Second counter is being shown */
	enum FPS_TYPE {
		FPS_NONE,
		FPS_COUNT,
		FPS_TIME
	};
	FPS_TYPE show_fps;

	










	/** Whether the calibrate tracker message is to be shown */
	bool calibrate_tracker;

	/**
	* The game handler.
	* @see GameHandler
	**/
	GameHandler* m_pGameHandler;
	/**
	* Current drawing side, only changed in setDrawingSide().
	**/
	vireio::RenderPosition m_currentRenderingSide;
	/**
	* Currently not used WorldViewTransform matrix.
	**/
	D3DXMATRIX* m_pCurrentMatViewTransform;

	/**
	* Proxy state block to capture various states.
	**/
	D3D9ProxyStateBlock* m_pCapturingStateTo;

	/**
	* BRASSA menu value.
	***/
	int viewportWidth;
	/**
	* BRASSA menu value.
	***/
	int viewportHeight;
	/**
	* True if BeginScene() is called the first time this frame.
	* @see BeginScene()
	**/
	bool m_isFirstBeginSceneOfFrame;
	/**
	* True if VRBoost is on.
	**/
	bool m_bVRBoostToggle;
	/**
	* Timespan the VRBoost indicator should be drawn.
	**/
	float m_fVRBoostIndicator;

	/**
	* Counts the current vertex shader set calls.
	* Used for VRboost security.
	***/
	UINT m_VertexShaderCount;
	/**
	* Counts the current vertex shader set calls (last frame).
	* Used for VRboost security.
	***/
	UINT m_VertexShaderCountLastFrame;

	/**
	* Struct commands device behavior.
	***/
	struct DeviceBehavior
	{
		/**
		* Determines when to render the brassa menu for that game profile.
		***/
		enum WhenToDo
		{
			PRESENT,
			BEGIN_SCENE,
			FIRST_BEGIN_SCENE,
			END_SCENE,
		};

		/**
		* Determines when to render the brassa menu for that game profile.
		***/
		WhenToDo whenToRenderBRASSA;
		/**
		* Determines when to handle head tracking for that game profile.
		***/
		WhenToDo whenToHandleHeadTracking;

	} m_deviceBehavior;


	/*** D3DProxyDevice private methods ***/
	void    BRASSA();
	void    BRASSA_MainMenu();
	void    BRASSA_WorldScale();
	void    BRASSA_Convergence();
	void    BRASSA_HUD();
	void    BRASSA_GUI();
	void    BRASSA_Settings();
	void    BRASSA_VRBoostValues();
	void	BRASSA_PosTracking();
	void    BRASSA_UpdateBorder();
	void    BRASSA_UpdateConfigSettings();
	void    BRASSA_UpdateDeviceSettings();
	void    BRASSA_AdditionalOutput();
	void    ReleaseEverything();
	bool    isViewportDefaultForMainRT(CONST D3DVIEWPORT9* pViewport);
	HRESULT SetStereoViewTransform(D3DXMATRIX pLeftMatrix, D3DXMATRIX pRightMatrix, bool apply);
	HRESULT SetStereoProjectionTransform(D3DXMATRIX pLeftMatrix, D3DXMATRIX pRightMatrix, bool apply);
	void    SetGUIViewport();
	float   RoundBrassaValue(float val);
	bool	InitBrassa();
	bool	InitVRBoost();

	//Calculate FPS, called every Present
	float fps;
	float CalcFPS();

	/*** VRboost function pointer typedefs ***/
	typedef HRESULT (WINAPI *LPVRBOOST_LoadMemoryRules)(std::string processName, std::string rulesPath);
	typedef HRESULT (WINAPI *LPVRBOOST_SaveMemoryRules)(std::string rulesPath);
	typedef HRESULT (WINAPI *LPVRBOOST_CreateFloatMemoryRule)(DWORD ruleType, UINT axisIndex, D3DXVECTOR4 constantVector, DWORD pointerAddress, DWORD* offsets, DWORD minValue, DWORD maxValue, DWORD comparisationPointer1, DWORD* comparisationOffsets1, int pointerDifference1, DWORD comparisationPointer2, DWORD* comparisationOffsets2, int pointerDifference2);
	typedef HRESULT (WINAPI *LPVRBOOST_SetProcess)(std::string processName, std::string moduleName);
	typedef HRESULT (WINAPI *LPVRBOOST_ReleaseAllMemoryRules)( void );
	typedef HRESULT (WINAPI *LPVRBOOST_ApplyMemoryRules)(UINT axisNumber, float** axis);

	LPVRBOOST_LoadMemoryRules m_pVRboost_LoadMemoryRules;
	LPVRBOOST_SaveMemoryRules m_pVRboost_SaveMemoryRules;
	LPVRBOOST_CreateFloatMemoryRule m_pVRboost_CreateFloatMemoryRule;
	LPVRBOOST_SetProcess m_pVRboost_SetProcess;
	LPVRBOOST_ReleaseAllMemoryRules m_pVRboost_ReleaseAllMemoryRules;
	LPVRBOOST_ApplyMemoryRules m_pVRboost_ApplyMemoryRules;
	HMODULE hmVRboost;

	struct 
	{
		bool VRBoost_Active;
		bool VRBoost_LoadRules;
		bool VRBoost_ApplyRules;
	} VRBoostStatus;

	ViewAdjustment* m_spShaderViewAdjustment;
	bool m_bActiveViewportIsDefault;
	bool m_bViewportIsSquished;
	bool m_VRboostRulesPresent;
	D3DVIEWPORT9 m_LastViewportSet;
	D3DVIEWPORT9 m_ViewportIfSquished;
	D3D9ProxySurface* m_pActiveStereoDepthStencil;
	D3D9ProxyIndexBuffer* m_pActiveIndicies;
	D3D9ProxyVertexDeclaration* m_pActiveVertexDeclaration;
	std::vector<D3D9ProxySwapChain*> m_activeSwapChains;
	std::vector<D3D9ProxySurface*> m_activeRenderTargets;	
	/**
	* Textures assigned to stages. 
	* (NULL is a valid entry in these containers. It indicates that the application has specifically 
	* cleared that stream/sampler. It is important that this information is available to the proxy 
	* StateBlock)
	* @see SetTexture()
	* @see GetTexture()
	**/
	std::unordered_map<DWORD, IDirect3DBaseTexture9*> m_activeTextureStages;
	/**
	* Active stored vertex buffers.
	**/
	std::unordered_map<UINT, D3D9ProxyVertexBuffer*> m_activeVertexBuffers;
	/**
	* True if BeginStateBlock() is called, false if EndStateBlock is called.
	* @see BeginStateBlock()
	* @see EndStateBlock()
	**/
	bool m_bInBeginEndStateBlock;
	/**
	* True if view transform is set via SetTransform().
	* @see SetTransform()
	**/
	bool m_bViewTransformSet;
	/**
	* True if projection transform is set via SetTransform().
	* @see SetTransform() 
	**/
	bool m_bProjectionTransformSet;
	/**
	* The stored left view transform set via SetTransform().
	**/
	D3DXMATRIX m_leftView;
	/**
	* The stored right view transform set via SetTransform().
	**/
	D3DXMATRIX m_rightView;
	/**
	* The stored left projection transform set via SetTransform(). 
	**/
	D3DXMATRIX m_leftProjection;
	/**
	* The stored right projection transform set via SetTransform(). 
	**/
	D3DXMATRIX m_rightProjection;
	/**
	* Either the left or right view, depending on active render side.
	**/
	D3DXMATRIX* m_pCurrentView;
	/**
	* Either the left or right projection, depending on active render side.	
	**/
	D3DXMATRIX* m_pCurrentProjection;
	/**
	* Backup of the current game profile.
	***/

	int screenshot;

	cMenu menu;

	/**
	* Actual Direct3D Device pointer embedded. 
	* Private to force you to think about whether you really need direct 
	* access to the actual device. Can be accessed via getActual(). 
	***/
	IDirect3DDevice9*   actual;
	IDirect3DDevice9Ex* actualEx;
	/**
	* Pointer to the D3D object that created the device. 
	***/
	D3D9ProxyDirect3D* m_pCreatedBy;
	/**
	* Internal reference counter. 
	***/
	ULONG m_nRefCount;
};
