#include "D3DProxyDevice.h"
#include "D3D9ProxyStateBlock.h"


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVertexShader , CONST DWORD* , pFunction , IDirect3DVertexShader9** , ppShader )
	IDirect3DVertexShader9* pActualVShader = NULL;
	HRESULT creationResult = actual->CreateVertexShader(pFunction, &pActualVShader);

	printf("Craete shader\n");

	if (SUCCEEDED(creationResult)) {
		D3D9ProxyVertexShader* shader = new D3D9ProxyVertexShader(pActualVShader, this);
		*ppShader = shader;
		shaders += shader;

		rulesUpdate();
	}

	return creationResult;
}





METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetVertexShader , IDirect3DVertexShader9* , pShader )
	D3D9ProxyVertexShader* shader = static_cast<D3D9ProxyVertexShader*>(pShader);

	HRESULT result;

	if( shader ){
		result = actual->SetVertexShader( shader->actual );
	}else{
		result = actual->SetVertexShader( 0 );
	}

	// Update stored proxy Vertex shader
	if( SUCCEEDED(result) ){

		// If in a Begin-End StateBlock pair update the block state rather than the current proxy device state
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureState(shader);
		}else{
			if (currentVS) {
				currentVS->Release();
			}

			currentVS = shader;

			if (currentVS) {
				currentVS->AddRef();
			}

			//m_spManagedShaderRegisters->ActiveVertexShaderChanged( currentVS );
		}
	}

	if( shader ){
		if( shader->m_bSquishViewport ){
			SetGUIViewport();
		}else{
			if( m_bViewportIsSquished ){
				actual->SetViewport(&m_LastViewportSet);
			}
			m_bViewportIsSquished = false;
		}

		if( config.shaderAnalyzer ){
			shader->used = true;

			if( shader->blink ){
				shader->hide = ((GetTickCount()%300)>150);
			}
		}
	}

	// increase vertex shader call count
	++m_VertexShaderCount;
	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetVertexShader , IDirect3DVertexShader9** , ppShader )
	if( !currentVS ){
		return D3DERR_INVALIDCALL;
	}

	*ppShader = currentVS;
	return D3D_OK;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetVertexShaderConstantF , UINT , StartRegister , CONST float* , pConstantData , UINT , Vector4fCount )
	vsConstantsOriginal.set( StartRegister , pConstantData , Vector4fCount );
	vsConstantsLeft    .set( StartRegister , pConstantData , Vector4fCount );
	vsConstantsRight   .set( StartRegister , pConstantData , Vector4fCount );
	return D3D_OK;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetVertexShaderConstantF , UINT , StartRegister , float* , pData , UINT , Vector4fCount )
	vsConstantsOriginal.get(StartRegister, pData, Vector4fCount);
	return D3D_OK;
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