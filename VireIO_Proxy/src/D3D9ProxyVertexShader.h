/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <D3D9ProxyVertexShader.h> and
Class <D3D9ProxyVertexShader> :
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

#pragma once
#include <d3d9.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "D3DProxyDevice.h"
#include "ShaderRegisters.h"
#include "D3DProxyDevice.h"
#include "ShaderModificationRepository.h"
#include <cBase.h>
#include "cShader.h"

/**
*  Direct 3D proxy vertex shader class.
*  Overwrites BaseDirect3DVertexShader9 and handles modified constants.
*/
class D3D9ProxyVertexShader : public cBase<IDirect3DVertexShader9> , public cShader
{
public:	
	D3D9ProxyVertexShader(IDirect3DVertexShader9* pActualVertexShader, D3DProxyDevice* pOwningDevice, ShaderModificationRepository* pModLoader);

	/*** IDirect3DVertexShader9 methods ***/
	HRESULT WINAPI GetFunction(void *pDate, UINT *pSizeOfData);


	std::map<UINT, StereoShaderConstant<>> m_modifiedConstants;
	bool m_bSquishViewport;
};
