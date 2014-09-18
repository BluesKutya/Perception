/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <D3D9ProxyCubeTexture.cpp> and
Class <D3D9ProxyCubeTexture> :
Copyright (C) 2013 Chris Drain

Vireio Perception Version History:
v1.0.0 2012 by Andres Hernandez
v1.0.X 2013 by John Hicks, Neil Schneider
v1.1.x 2013 by Primary Coding Author: Chris Drain
Team Support: John Hicks, Phil Larkson, Neil Schneider
v2.0.x 2013 by Denis Reischl, Neil Schneider, Joshua Brown

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

#include "D3D9ProxyCubeTexture.h"
#include "D3DProxyDevice.h"

/**
* Constructor.
* @see D3D9ProxySurface::D3D9ProxySurface
***/
D3D9ProxyCubeTexture::D3D9ProxyCubeTexture(IDirect3DCubeTexture9* pActualTextureLeft, IDirect3DCubeTexture9* pActualTextureRight, D3DProxyDevice* pOwningDevice) :
	cBase( pActualTextureLeft , pOwningDevice ) , 
	right( pActualTextureRight )
{

}

/**
* Destructor.
* Deletes wrapped surface levels, releases texures.
***/
D3D9ProxyCubeTexture::~D3D9ProxyCubeTexture(){
	// delete all surfaces in m_levels
	auto it = m_wrappedSurfaceLevels.begin();
	while (it != m_wrappedSurfaceLevels.end()) {
		// we have to explicitly delete the Surfaces here as the Release behaviour of the surface would get stuck in a loop
		// calling back to the container Release.
		delete it->second;
		it = m_wrappedSurfaceLevels.erase(it);
	}

	if (right)
		right->Release();
}



/**
* Sets private data on both (left/right) textures.
***/
HRESULT WINAPI D3D9ProxyCubeTexture::SetPrivateData(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags)
{
	if( right ){
		right->SetPrivateData(refguid, pData, SizeOfData, Flags);
	}

	return actual->SetPrivateData(refguid, pData, SizeOfData, Flags);
}


/**
* Base GetPrivateData functionality. 
***/
HRESULT WINAPI D3D9ProxyCubeTexture::GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData)
{
	return actual->GetPrivateData(refguid, pData, pSizeOfData);
}


/**
* Frees private data on both (left/right) textures.
***/
HRESULT WINAPI D3D9ProxyCubeTexture::FreePrivateData(REFGUID refguid)
{
	if( right ){
		right->FreePrivateData(refguid);
	}

	return actual->FreePrivateData(refguid);
}

/**
* Sets priority on both (left/right) textures.
***/
DWORD WINAPI D3D9ProxyCubeTexture::SetPriority(DWORD PriorityNew)
{
	if( right ){
		right->SetPriority(PriorityNew);
	}

	return actual->SetPriority(PriorityNew);
}


/**
* Calls method on both (left/right) textures.
***/
void WINAPI D3D9ProxyCubeTexture::PreLoad()
{
	if( right ){
		right->PreLoad();
	}

	return actual->PreLoad();
}



/**
* Sets LOD on both (left/right) texture.
***/
DWORD WINAPI D3D9ProxyCubeTexture::SetLOD(DWORD LODNew)
{
	if( right ){
		right->SetLOD(LODNew);
	}

	return actual->SetLOD(LODNew);
}

/**
* Base GetLOD functionality. 
***/
DWORD WINAPI D3D9ProxyCubeTexture::GetLOD()
{
	return actual->GetLOD();
}

/**
* Base GetLevelCount functionality. 
***/
DWORD WINAPI D3D9ProxyCubeTexture::GetLevelCount()
{
	return actual->GetLevelCount();
}


/**
* Sets filter type on both (left/right) texture.
***/
HRESULT WINAPI D3D9ProxyCubeTexture::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType)
{
	if( right ){
		right->SetAutoGenFilterType(FilterType);
	}

	return actual->SetAutoGenFilterType(FilterType);
}

/**
* Base GetAutoGenFilterType functionality. 
***/
D3DTEXTUREFILTERTYPE WINAPI D3D9ProxyCubeTexture::GetAutoGenFilterType()
{
	return actual->GetAutoGenFilterType();
}

/**
* Generates sub levels on both (left/right) texture.
***/
void WINAPI D3D9ProxyCubeTexture::GenerateMipSubLevels()
{
	if( right ){
		right->GenerateMipSubLevels();
	}

	return actual->GenerateMipSubLevels();
}

/**
* Base GetLevelDesc functionality. 
***/
HRESULT WINAPI D3D9ProxyCubeTexture::GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc)
{
	return actual->GetLevelDesc(Level, pDesc);
}


/**
* If proxy surface is already stored on this level, return this one, otherwise create it.
* To create a new stored surface level, call the method on both (left/right) actual textures.
***/
HRESULT WINAPI D3D9ProxyCubeTexture::GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9** ppCubeMapSurface)
{
	HRESULT finalResult;

	CubeSurfaceKey key = CubeSurfaceKey(FaceType,Level);
	// Have we already got a Proxy for this surface level?
	if (m_wrappedSurfaceLevels.count(key) == 1) { // yes

		*ppCubeMapSurface = m_wrappedSurfaceLevels[key];
		(*ppCubeMapSurface)->AddRef();

		finalResult = D3D_OK;
	}
	else {
		// Get underlying surfaces (stereo pair made from the surfaces at the same level in the left and right textues), 
		//  wrap, then store in m_wrappedSurfaceLevels and return the wrapped surface
		IDirect3DSurface9* pActualSurfaceLevelLeft = NULL;
		IDirect3DSurface9* pActualSurfaceLevelRight = NULL;

		HRESULT leftResult = actual->GetCubeMapSurface(FaceType, Level, &pActualSurfaceLevelLeft);

		if( right ){
			HRESULT resultRight = right->GetCubeMapSurface(FaceType, Level, &pActualSurfaceLevelRight);
			assert (leftResult == resultRight);
		}


		if (SUCCEEDED(leftResult)) {

			D3D9ProxySurface* pWrappedSurfaceLevel = new D3D9ProxySurface(pActualSurfaceLevelLeft, pActualSurfaceLevelRight, device, this);

			if(m_wrappedSurfaceLevels.insert(std::pair<CubeSurfaceKey, D3D9ProxySurface*>(key, pWrappedSurfaceLevel)).second) {
				// insertion of wrapped surface level into m_wrappedSurfaceLevels succeeded
				*ppCubeMapSurface = pWrappedSurfaceLevel;
				(*ppCubeMapSurface)->AddRef();
				finalResult = D3D_OK;
			}
			else {
				// Failure to insert should not be possible. In this case we could still return the wrapped surface,
				// however, if we did and it was requested again a new wrapped instance will be returned and things would explode
				// at some point. Better to fail fast.
				OutputDebugStringA(__FUNCTION__);
				OutputDebugStringA("\n");
				OutputDebugStringA("Unable to store surface level.\n");
				assert(false);

				finalResult = D3DERR_INVALIDCALL;
			}
		}
		else { 
			OutputDebugStringA(__FUNCTION__);
			OutputDebugStringA("\n");
			OutputDebugStringA("Error fetching actual surface level.\n");
			finalResult = leftResult;
		}
	}

	return finalResult;
}

/**
* Locks rectangle on both (left/right) textures.
***/
HRESULT WINAPI D3D9ProxyCubeTexture::LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags)
{
	if( right ){
		right->LockRect(FaceType, Level, pLockedRect, pRect, Flags);
	}

	return actual->LockRect(FaceType, Level, pLockedRect, pRect, Flags);
}

/**
* Unlocks rectangle on both (left/right) textures.
***/
HRESULT WINAPI D3D9ProxyCubeTexture::UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level)
{
	if( right ){
		right->UnlockRect(FaceType, Level);
	}

	return actual->UnlockRect(FaceType, Level);
}

/**
* Adds dirty rectangle on both (left/right) textures.
***/
HRESULT WINAPI D3D9ProxyCubeTexture::AddDirtyRect(D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect)
{
	if( right ){
		right->AddDirtyRect(FaceType, pDirtyRect);
	}

	return actual->AddDirtyRect(FaceType, pDirtyRect);
}

DWORD WINAPI D3D9ProxyCubeTexture::GetPriority(){
	return actual->GetPriority();
}

D3DRESOURCETYPE WINAPI D3D9ProxyCubeTexture::GetType(){
	return actual->GetType();
}