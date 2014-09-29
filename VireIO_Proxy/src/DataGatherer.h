/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <DataGatherer.cpp> and
Class <DataGatherer> :
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
#include <unordered_set>
#include <fstream>
#include "D3DProxyDevice.h"
#include "MurmurHash3.h"
#include <cConfig.h>
#include <qmap.h>
#include <qvector.h>

/**
* Data gatherer class, outputs relevant shader data to dump file (.csv format) .
* Outputs Shader Hash,Constant Name,ConstantType,Start Register,Register Count to "shaderDump.csv".
* Used ".csv" file format to easily open and sort using OpenOffice (for example). These informations let 
* you create new shader rules.
* (if compiled to debug, it outputs shader code to "VS(hash).txt" or "PS(hash).txt")
*/
class DataGatherer : public D3DProxyDevice
{
public:
	DataGatherer(IDirect3DDevice9* pDevice, IDirect3DDevice9Ex* pDeviceEx , D3D9ProxyDirect3D* pCreatedBy );
	virtual ~DataGatherer();

	/*** IDirect3DDevice9 methods ***/
	HRESULT WINAPI Present(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	HRESULT WINAPI BeginScene();
	HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
	HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount);
	HRESULT WINAPI DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	HRESULT WINAPI DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	HRESULT WINAPI CreateVertexShader(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader);
	HRESULT WINAPI SetVertexShader(IDirect3DVertexShader9* pShader);
	HRESULT WINAPI SetVertexShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	HRESULT WINAPI CreatePixelShader(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader);
	HRESULT WINAPI SetPixelShader(IDirect3DPixelShader9* pShader);


private:
	/*** DataGatherer private methods ***/
	void Analyze();
	void GetCurrentShaderRules( bool allStartRegisters );
	void MarkShaderAsUsed     ( int hash , bool isVertex );



	struct Shader{
		IUnknown*    ptr;
		QByteArray   code;
		QByteArray   hash;
		bool         isVertex;
		bool         blink;
		bool         hide;
		bool         used;
		cMenuItem*   item;
	};

	struct ShaderConstant{
		Shader*            shader;
		
		D3DXCONSTANT_DESC  desc;
		QString            name;
		cMenuItem*         item;
		bool               nodeCreated;
		bool               applyRule;
		QString            ruleName;
		UINT               ruleOperation;
		bool               ruleTranspose;
	};


	QList<Shader*>         shaders;
	QList<ShaderConstant*> constants;
	Shader*                currentVS;
	Shader*                currentPS;
	cMenuItem*             shadersMenu;
	cMenuItem*             rulesMenu;
	bool                   showUnusedShaders;
	bool                   showPixelShaders;
	
	void ShaderUse   ( IUnknown* ptr , Shader** current );
	void ShaderCreate( IUnknown* ptr , bool isVertex );
	bool isDrawHide  ( );
	void UpdateRuleDisplay( ShaderConstant* c );
	/**
	* Vector of all relevant vertex shader constants.
	***/
	std::vector<ShaderConstant> m_relevantVSConstants;
	/**
	* Vector of all added vertex shader constants (rules).
	***/
	std::vector<ShaderConstant> m_addedVSConstants;
	/**
	* Vector of all relevant vertex shader constants, each name only once.
	***/
	std::vector<ShaderConstant> m_relevantVSConstantNames;

	/**
	* True if Draw() calls should be skipped currently.
	***/
	bool m_bAvoidDraw;
	/**
	* Pixel shader helper for m_bAvoidDraw.
	***/
	bool m_bAvoidDrawPS;
	/**
	* Array of possible world-view-projection matrix shader constant names.
	***/
	std::string* m_wvpMatrixConstantNames;
	/**
	* Array of matrix substring names to be avoided.
	***/
	std::string* m_wvpMatrixAvoidedSubstrings;
	/**
	* True if analyzing tool is activated.
	***/
	bool m_startAnalyzingTool;
	/**
	* Frame counter for analyzing.
	***/
	UINT m_analyzingFrameCounter;
	/**
	* Set of recorded vertex shaders, to avoid double output.
	***/
	std::unordered_set<IDirect3DVertexShader9*> m_recordedVShaders;
	/**
	* Set of recorded shaders, to avoid double output.
	***/
	std::unordered_set<IDirect3DPixelShader9*> m_recordedPShaders;
	/**
	* Set of recorded vertex shaders, to avoid double debug log output.
	***/
	std::unordered_set<IDirect3DVertexShader9*> m_recordedSetVShaders;
	/**
	* The shader dump file (.csv format).
	***/
	std::ofstream m_shaderDumpFile;
	/**
	* The hash code of the vertex shader currently set.
	***/
	uint32_t m_currentVertexShaderHash;
	/**
	* True if data gatherer should output (ALL!) shader code.
	***/
	bool m_bOutputShaderCode;
};