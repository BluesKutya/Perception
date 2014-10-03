#include "D3DProxyDevice.h"

#define IS_RENDER_TARGET(d3dusage) ((d3dusage & D3DUSAGE_RENDERTARGET) > 0 ? true : false)
#define IS_POOL_DEFAULT(d3dpool)   ((d3dpool & D3DPOOL_DEFAULT) > 0 ? true : false)


bool D3DProxyDevice::gameShouldDuplicateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality,BOOL Lockable, bool isSwapChainBackBuffer){
	switch(config.game_type)
	{
	case D3DProxyDevice::SOURCE:
		if (isSwapChainBackBuffer) {
			return true;
		}
		return Width != Height;

	case D3DProxyDevice::SOURCE_L4D:
		if (isSwapChainBackBuffer) {
			return true;
		}
		return Width != Height;

	case D3DProxyDevice::SOURCE_ESTER:
		return true;

	case D3DProxyDevice::UNREAL:
		if (isSwapChainBackBuffer) {
			return true;
		}
		return Width != Height;

	case D3DProxyDevice::UNREAL_MIRROR:
		if (isSwapChainBackBuffer) {
			return true;
		}
		return Width != Height;

	case D3DProxyDevice::UNREAL_UT3:
		if (isSwapChainBackBuffer) {
			return true;
		}
		return Width != Height;

	case D3DProxyDevice::UNREAL_BIOSHOCK:
		if (isSwapChainBackBuffer) {
			return true;
		}
		return Width != Height;

	case D3DProxyDevice::UNREAL_BORDERLANDS:
		if (isSwapChainBackBuffer) {
			return true;
		}
		return Width != Height;

	case D3DProxyDevice::EGO:
		return true;  

	case D3DProxyDevice::EGO_DIRT:
		return true; 

	case D3DProxyDevice::REALV:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::REALV_ARMA:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::UNITY:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::UNITY_SLENDER:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::GAMEBRYO:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::GAMEBRYO_SKYRIM:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::LFS:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::CDC:
		// NOT TESTED NOW !
		return true;

	default:
		return true;

	}
}

/**
* True if the by parameters described depth stencil surface is to be duplicated for the handled game.
* Currently, always returns true.
* @see D3DProxyDevice::CreateDepthStencilSurface()
***/
bool D3DProxyDevice::gameShouldDuplicateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard)
{
	switch(config.game_type)
	{
	case D3DProxyDevice::SOURCE:
		return Width != Height;

	case D3DProxyDevice::SOURCE_L4D:
		return Width != Height;

	case D3DProxyDevice::SOURCE_ESTER:
		return true;

	case D3DProxyDevice::UNREAL:
		return Width != Height;

	case D3DProxyDevice::UNREAL_MIRROR:
		return Width != Height;

	case D3DProxyDevice::UNREAL_UT3:
		return Width != Height;

	case D3DProxyDevice::UNREAL_BIOSHOCK:
		return Width != Height;

	case D3DProxyDevice::UNREAL_BORDERLANDS:
		return Width != Height;

	case D3DProxyDevice::EGO:
		return true;

	case D3DProxyDevice::EGO_DIRT:
		return true;

	case D3DProxyDevice::REALV:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::REALV_ARMA:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::UNITY:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::UNITY_SLENDER:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::GAMEBRYO:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::GAMEBRYO_SKYRIM:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::LFS:
		// NOT TESTED NOW !
		return true;

	case D3DProxyDevice::CDC:
		// NOT TESTED NOW !
		return true;

	default:
		return true;

	}
}

/**
* True if the by parameters described texture is to be duplicated for the handled game.
* Currently, returns true if texture is a render target.
* @see D3DProxyDevice::CreateTexture()
***/
bool D3DProxyDevice::gameShouldDuplicateTexture(UINT Width,UINT Height,UINT Levels,DWORD Usage, D3DFORMAT Format,D3DPOOL Pool)
{
	switch(config.game_type)
	{
	case D3DProxyDevice::SOURCE:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage) && (Width != Height);

	case D3DProxyDevice::SOURCE_L4D:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage) && (Width != Height);

	case D3DProxyDevice::SOURCE_ESTER:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::UNREAL:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage) && (Width != Height);

	case D3DProxyDevice::UNREAL_MIRROR:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::UNREAL_UT3:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage) && (Width != Height);

	case D3DProxyDevice::UNREAL_BIOSHOCK:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage) && (Width != Height);

	case D3DProxyDevice::UNREAL_BORDERLANDS:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage) && (Width != Height);

	case D3DProxyDevice::EGO:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::EGO_DIRT:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::REALV:
		// NOT TESTED NOW !
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::REALV_ARMA:
		// NOT TESTED NOW !
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::UNITY:
		// NOT TESTED NOW !
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::UNITY_SLENDER:
		// NOT TESTED NOW !
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::GAMEBRYO:
		// NOT TESTED NOW !
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::GAMEBRYO_SKYRIM:
		// NOT TESTED NOW !
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::LFS:
		// NOT TESTED NOW !
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::CDC:
		// NOT TESTED NOW !
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	default:
		if ((Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL)
			return true;
		return IS_RENDER_TARGET(Usage);

	}
}

/**
* True if the by parameters described cube texture is to be duplicated for the handled game.
* Currently, returns true if cube texture is a render target.
* @see D3DProxyDevice::CreateCubeTexture()
***/
bool D3DProxyDevice::gameShouldDuplicateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool)
{
	switch(config.game_type)
	{
	case D3DProxyDevice::SOURCE:
		return false;

	case D3DProxyDevice::SOURCE_L4D:
		return false;

	case D3DProxyDevice::SOURCE_ESTER:
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::UNREAL:
		return false;

	case D3DProxyDevice::UNREAL_MIRROR:
		return false;

	case D3DProxyDevice::UNREAL_UT3:
		return false;

	case D3DProxyDevice::UNREAL_BIOSHOCK:
		return false;

	case D3DProxyDevice::UNREAL_BORDERLANDS:
		return false;

	case D3DProxyDevice::EGO:
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::EGO_DIRT:
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::REALV:
		// NOT TESTED NOW !
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::REALV_ARMA:
		// NOT TESTED NOW !
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::UNITY:
		// NOT TESTED NOW !
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::UNITY_SLENDER:
		// NOT TESTED NOW !
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::GAMEBRYO:
		// NOT TESTED NOW !
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::GAMEBRYO_SKYRIM:
		// NOT TESTED NOW !
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::LFS:
		// NOT TESTED NOW !
		return IS_RENDER_TARGET(Usage);

	case D3DProxyDevice::CDC:
		// NOT TESTED NOW !
		return IS_RENDER_TARGET(Usage);

	default:
		return IS_RENDER_TARGET(Usage);

	}
}
