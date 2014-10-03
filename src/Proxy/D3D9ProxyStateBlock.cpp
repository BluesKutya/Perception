#include "D3D9ProxyStateBlock.h"
#include <assert.h>

static void GetConstants( QMap<int,D3DXVECTOR4>* map , cConstantBuffer* buf , bool onlyExisting ){
	QList<int> indexes;
	if( onlyExisting ){
		indexes = map->keys();
	}else{
		for( int c=0 ; c<buf->registerCount() ; c++ ){
			indexes += c;
		}
	}

	for( int c : indexes ){
		float data[4];
		buf->get( c , data , 1 );
		(*map)[c] = D3DXVECTOR4(data);
	}
}


static void SetConstants( QMap<int,D3DXVECTOR4>* map , cConstantBuffer* buf ){
	for( int c : map->keys() ){
		buf->set( c , (*map)[c] , 1 );
	}
}




D3D9ProxyStateBlock::D3D9ProxyStateBlock( IDirect3DStateBlock9* pActualStateBlock , D3DProxyDevice *pOwningDevice ) :
	cBase( pActualStateBlock , pOwningDevice )
{
	type				    = 0;
	sideLeft			    = false;
	sideRight			    = false;
	selectAuto			    = false;
	selectIndexBuffer	    = false;
	selectViewport		    = false;
	selectViewTransform	    = false;
	selectProjTransform	    = false;
	selectPixelShader	    = false;
	selectVertexShader	    = false;
	selectVertexDeclaration = false;
}



void D3D9ProxyStateBlock::init( ){
	if( type == D3DSBT_ALL ){
		selectIndexBuffer		 = true;
		selectViewport			 = true;
		selectViewTransform      = true;
		selectProjTransform      = true;
		selectPixelShader		 = true;
		selectVertexShader		 = true;
		selectVertexDeclaration	 = true;
	}

	if( type == D3DSBT_VERTEXSTATE ){
		selectVertexShader		 = true;
		selectVertexDeclaration	 = true;
	}

	if( type == D3DSBT_PIXELSTATE ){
		selectPixelShader        = true;
	}

	if ( type == 0 ) {
		selectAuto = true;
	}else{
		captureSelected();
	}
}



METHOD_IMPL( HRESULT , WINAPI , D3D9ProxyStateBlock , Capture )
	HRESULT result = actual->Capture();
	if( SUCCEEDED(result) ){
		captureSelected();
	}
	return result;
}



METHOD_IMPL( HRESULT , WINAPI , D3D9ProxyStateBlock , Apply )
	if( sideLeft && !sideRight ){
		device->setDrawingSide( vireio::Left );
	}else
	if( sideRight && !sideRight ){
		device->setDrawingSide( vireio::Right );
	}

	HRESULT result = actual->Apply();
	if( FAILED(result) ){
		return result;
	}


	bool reApplyStereo = sideLeft && sideRight;

	if( selectIndexBuffer ){
		device->m_pActiveIndicies = storedIndexBuffer;
	}

	if( selectViewport ){
		device->m_LastViewportSet          = storedViewport;
		device->m_bActiveViewportIsDefault = device->isViewportDefaultForMainRT( &storedViewport );
	}

	if( selectViewTransform ){
		device->SetStereoViewTransform( storedLeftView , storedRightView , reApplyStereo );

	}

	if( selectProjTransform ){
		device->SetStereoProjectionTransform( storedLeftProjection , storedRightProjection , reApplyStereo );
	}

	if( selectPixelShader ){
		device->psCurrent = storedPixelShader;
	}

	if( selectVertexShader ){
		device->vsCurrent = storedVertexShader;
	}

	if( selectVertexDeclaration ){
		device->m_pActiveVertexDeclaration = storedVertexDeclaration;
	}


	device->m_activeVertexBuffers = storedVertexStreams;

	SetConstants( &storedVsConstants , &device->vsConstantsOriginal );
	SetConstants( &storedPsConstants , &device->psConstantsOriginal );
		
	if( reApplyStereo ){
		for( int id : storedTextureStages.keys() ){
			device->SetTexture( id , storedTextureStages[id] );
		}
	}else{
		device->m_activeTextureStages = storedTextureStages;
	}

	return D3D_OK;
}




void D3D9ProxyStateBlock::captureIndexBuffer( D3D9ProxyIndexBuffer* ib ){
	selectIndexBuffer |= selectAuto;
	storedIndexBuffer  = ib;
}


void D3D9ProxyStateBlock::captureViewport( D3DVIEWPORT9 viewport ){
	selectViewport |= selectAuto;
	storedViewport  = viewport;
}


void D3D9ProxyStateBlock::captureViewTransform( D3DXMATRIX left , D3DXMATRIX right ){
	selectViewTransform |= selectAuto;
	storedLeftView       = left;
	storedRightView      = right;

	updateCaptureSideTracking();
}


void D3D9ProxyStateBlock::captureProjTransform( D3DXMATRIX left , D3DXMATRIX right ){
	selectProjTransform  |= selectAuto;
	storedLeftProjection  = left;
	storedRightProjection = right;

	updateCaptureSideTracking();
}


void D3D9ProxyStateBlock::capturePixelShader( ComPtr<D3D9ProxyPixelShader> shader ){
	selectPixelShader |= selectAuto;
	storedPixelShader  = shader;
}


void D3D9ProxyStateBlock::captureVertexShader( ComPtr<D3D9ProxyVertexShader> shader ){
	selectVertexShader |= selectAuto;
	storedVertexShader  = shader;
}


void D3D9ProxyStateBlock::captureVertexDeclaration ( ComPtr<D3D9ProxyVertexDeclaration> decl ){
	selectVertexDeclaration |= selectAuto;
	storedVertexDeclaration  = decl;
}


void D3D9ProxyStateBlock::captureTextureSampler( int stage , ComPtr<IDirect3DBaseTexture9> texture ){
	storedTextureStages[stage]  = texture;
	updateCaptureSideTracking();
}


void D3D9ProxyStateBlock::captureVertexStream( int stream , ComPtr<D3D9ProxyVertexBuffer> buffer ){
	storedVertexStreams[stream]  = buffer;
}



void D3D9ProxyStateBlock::captureVertexShaderConstant( int index , const float* data , int count ){
	for( int c=0 ; c<count ; c++ ){
		storedVsConstants[index] = D3DXVECTOR4( data + c*4 );
	}
}



void D3D9ProxyStateBlock::capturePixelShaderConstant( int index , const float* data , int count ){
	for( int c=0 ; c<count ; c++ ){
		storedPsConstants[index] = D3DXVECTOR4( data + c*4 );
	}
}



void D3D9ProxyStateBlock::captureSelected( ){
	if( selectIndexBuffer ){
		storedIndexBuffer = device->m_pActiveIndicies;
	}

	if( selectViewport ){
		storedViewport = device->m_LastViewportSet;
	}

	if( selectViewTransform ){
		storedLeftView  = device->m_leftView;
		storedRightView = device->m_rightView;
	}

	if( selectProjTransform ){
		storedLeftProjection  = device->m_leftProjection;
		storedRightProjection = device->m_rightProjection;
	}

	if( selectPixelShader ){
		storedPixelShader = device->psCurrent;
	}

	if( selectVertexShader ){
		storedVertexShader = device->vsCurrent;
	}

	if( selectVertexDeclaration ){
		storedVertexDeclaration = device->m_pActiveVertexDeclaration;
	}





	if( type == D3DSBT_ALL ){
		storedTextureStages = device->m_activeTextureStages;
		storedVertexStreams = device->m_activeVertexBuffers;
	}

	if( type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE ){
		GetConstants( &storedVsConstants , &device->vsConstantsOriginal , false );
	}

	if( type == D3DSBT_ALL || type == D3DSBT_PIXELSTATE ){
		GetConstants( &storedPsConstants , &device->psConstantsOriginal , false );
	}


	if( type == 0 ){
		float vec[4];

		storedTextureStages.clear();
		storedVertexStreams.clear();
		storedVsConstants  .clear();
		storedPsConstants  .clear();

		for( int c : storedTextureStages.keys() ){
			storedTextureStages[c] = device->m_activeTextureStages[c];
		}

		for( int c : storedVertexStreams.keys() ){
			storedVertexStreams[c] = device->m_activeVertexBuffers[c];
		}

		GetConstants( &storedVsConstants , &device->vsConstantsOriginal , true );
		GetConstants( &storedPsConstants , &device->psConstantsOriginal , true );
	}


	if( device->m_currentRenderingSide == vireio::Left ){
		sideLeft  = true;
		sideRight = false;
	}else{
		sideLeft   = false;
		sideRight  = true;
	}
}






void D3D9ProxyStateBlock::updateCaptureSideTracking( ){
	if( sideLeft && sideRight ){
		return;
	}

	
	if( device->m_currentRenderingSide == vireio::Left ){
		sideLeft = true;
	}
	
	if( device->m_currentRenderingSide == vireio::Right ){
		sideRight = true;
	}
}

