/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <D3D9ProxyVolume.cpp> and
Class <D3D9ProxyVolume> :
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
#include <Vireio.h>
#include "D3DProxyDevice.h"
#include "D3D9ProxyVolume.h"

D3D9ProxyVolume::D3D9ProxyVolume(IDirect3DVolume9* pActualVolume, D3DProxyDevice* pOwningDevice, IUnknown* pWrappedContainer) :
	cBase( pActualVolume , pOwningDevice , pWrappedContainer )
{
}


METHOD_THRU( HRESULT , WINAPI , D3D9ProxyVolume , SetPrivateData , REFGUID , refguid , CONST void* , pData , DWORD , SizeOfData , DWORD , Flags )
METHOD_THRU( HRESULT , WINAPI , D3D9ProxyVolume , GetPrivateData , REFGUID , refguid , void* , pData , DWORD* , pSizeOfData )
METHOD_THRU( HRESULT , WINAPI , D3D9ProxyVolume , FreePrivateData , REFGUID , refguid )
METHOD_THRU( HRESULT , WINAPI , D3D9ProxyVolume , GetDesc , D3DVOLUME_DESC* , pDesc )
METHOD_THRU( HRESULT , WINAPI , D3D9ProxyVolume , LockBox , D3DLOCKED_BOX* , pLockedVolume, const D3DBOX* , pBox , DWORD , Flags )
METHOD_THRU( HRESULT , WINAPI , D3D9ProxyVolume , UnlockBox )
