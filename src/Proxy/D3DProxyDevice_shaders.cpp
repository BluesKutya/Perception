#include "D3DProxyDevice.h"
#include "D3D9ProxyStateBlock.h"


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVertexShader , CONST DWORD* , pFunction , IDirect3DVertexShader9** , ppShader )
	IDirect3DVertexShader9* pActualVShader = NULL;
	HRESULT creationResult = actual->CreateVertexShader(pFunction, &pActualVShader);

	if (SUCCEEDED(creationResult)) {
		*ppShader = new D3D9ProxyVertexShader(pActualVShader, this);
	}

	return creationResult;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreatePixelShader , CONST DWORD* , pFunction , IDirect3DPixelShader9** , ppShader )
	IDirect3DPixelShader9* pActualPShader = NULL;
	HRESULT creationResult = actual->CreatePixelShader(pFunction, &pActualPShader);

	if (SUCCEEDED(creationResult)) {
		*ppShader = new D3D9ProxyPixelShader(pActualPShader, this);
	}

	return creationResult;
}





METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetVertexShader , IDirect3DVertexShader9* , pShader )
	D3D9ProxyVertexShader* proxy;
	HRESULT                result;

	if( pShader ){
		proxy  = static_cast<D3D9ProxyVertexShader*>(pShader);
		result = actual->SetVertexShader( proxy->actual );
	}else{
		proxy  = 0;
		result = actual->SetVertexShader( 0 );
	}

	// Update stored proxy Vertex shader
	if( SUCCEEDED(result) ){
		if (stateBlock) {
			stateBlock->captureVertexShader(proxy);
		}
		
		activeVertexShader = proxy;
	}

	if( proxy ){
		if( proxy->m_bSquishViewport ){
			SetGUIViewport();
		}else{
			if( m_bViewportIsSquished ){
				actual->SetViewport(&m_LastViewportSet);
			}
			m_bViewportIsSquished = false;
		}

		if( config.shaderAnalyzer ){
			proxy->used = true;

			if( proxy->blink ){
				proxy->hide = ((GetTickCount()%300)>150);
			}
		}
	}

	++m_VertexShaderCount;
	return result;
}





METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetPixelShader , IDirect3DPixelShader9* , pShader )
	D3D9ProxyPixelShader* proxy;
	HRESULT               result;

	if( pShader ){
		proxy  = static_cast<D3D9ProxyPixelShader*>(pShader);
		result = actual->SetPixelShader( proxy->actual );
	}else{
		proxy  = 0;
		result = actual->SetPixelShader(NULL);
	}

	if( SUCCEEDED(result) ){
		if (stateBlock) {
			stateBlock->capturePixelShader( proxy );
		}
		activePixelShader = proxy;
	}

	if( config.shaderAnalyzer && proxy ){
		proxy->used = true;

		if( proxy->blink ){
			proxy->hide = ((GetTickCount()%300)>150);
		}
	}

	return result;
}









METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetVertexShader , IDirect3DVertexShader9** , ppShader )
	if( !activeVertexShader ){
		return D3DERR_INVALIDCALL;
	}

	*ppShader = activeVertexShader;
	(*ppShader)->AddRef();

	return D3D_OK;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetPixelShader , IDirect3DPixelShader9** , ppShader )
	if( !activePixelShader ){
		return D3DERR_INVALIDCALL;
	}

	*ppShader = activePixelShader;
	(*ppShader)->AddRef();

	return D3D_OK;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetVertexShaderConstantF , UINT , StartRegister , CONST float* , pConstantData , UINT , Vector4fCount )
	if( stateBlock ){
		stateBlock->captureVertexShaderConstant( StartRegister , pConstantData , Vector4fCount );
	}

	return vsConstants.set( StartRegister , pConstantData , Vector4fCount );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetPixelShaderConstantF , UINT , StartRegister , CONST float* , pConstantData , UINT , Vector4fCount )
	if( stateBlock ){
		stateBlock->capturePixelShaderConstant( StartRegister , pConstantData , Vector4fCount );
	}

	return psConstants.set( StartRegister , pConstantData , Vector4fCount );
}




METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetVertexShaderConstantF , UINT , StartRegister , float* , pData , UINT , Vector4fCount )
	return vsConstants.get( StartRegister , pData , Vector4fCount );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetPixelShaderConstantF , UINT , StartRegister , float* , pData , UINT , Vector4fCount )
	return psConstants.get( StartRegister , pData , Vector4fCount );
}























		/*
		for( cShaderConstant& c : currentVS->constants ){
			int src1 = StartRegister;
			int src2 = StartRegister + Vector4fCount;

			int dst1 = c.RegisterIndex;
			int dst2 = c.RegisterIndex + std::min( (int)c.RegisterCount , 4 );

			if(  src2 > dst1  &&  src1 < dst2  ){
				int copy1 = std::max( src1 , dst1 );
				int copy2 = std::min( src2 , dst2 );

				memcpy(
					c.value       + (copy1 - c.RegisterIndex)*4 ,
					pConstantData + (copy1 - StartRegister  )*4 ,
					(copy2 - copy1) * 4 * sizeof(float)
				);
			}
		}*/

	/*
	if (m_pCapturingStateTo) {
		result = m_pCapturingStateTo->SelectAndCaptureStateVSConst(StartRegister, pConstantData, Vector4fCount);
	}
	else { 
		result = m_spManagedShaderRegisters->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	}
	*/


	//actual->SetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount);


	
/*	
	// Tests if the set constant is a transposed matrix and sets the relevant bool.
	// Is Matrix transposed ?
	// Affine transformation matrices have in the last row (0,0,0,1). World and view matrices are 
	// usually affine, since they are a combination of affine transformations (rotation, scale, 
	// translation ...).
	// Perspective projection matrices have in the last column (0,0,1,0) if left-handed and 
	// (0,0,-1,0) if right-handed.
	// Orthographic projection matrices have in the last column (0,0,0,1).
	// If those are transposed you find the entries in the last column/row.
	if( config.shaderAnalyzer && config.shaderAnalyzerDetectTranspose ){
		for( cShader* s : shaders ){
			for( cShaderConstant& c : s->constants ){
				if( s == currentVS ){
					if( c.RegisterIndex >= StartRegister  &&
						c.RegisterIndex < StartRegister + Vector4fCount
					){

						int i = 0;

						if( c.Class == D3DXPARAMETER_CLASS::D3DXPC_MATRIX_ROWS ){
							i = 14;
						}

						if( c.Class == D3DXPARAMETER_CLASS::D3DXPC_MATRIX_COLUMNS ){
							i = 12;
						}

						if( i ){
							D3DXMATRIX matrix = D3DXMATRIX( pConstantData + (c.RegisterIndex-StartRegister)*4*sizeof(float) );
					
							if( fabs( fabs(matrix[i]) - 1.0 ) > 0.00001 ){
								config.shaderAnalyzerTranspose = true;
							}
						}
					}
				}
			}
		}
	}
	*/