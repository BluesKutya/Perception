#include "D3D9ProxyStateBlock.h"
#include <assert.h>


static void ApplyExistingConstants( std::map< int , D3DXVECTOR4 >& stored , cConstantBuffer& dst ){
	for( auto p : stored ){
		dst.set( p.first , p.second , 1 );
	}

	dst.clearModified();
	dst.setStereoModified();
} 


static void GetSelectedConstants( std::map< int , D3DXVECTOR4 >& stored , cConstantBuffer& dst ){
	for( auto& p : stored ){
		dst.get( p.first , p.second , 1 );
	}
} 

template<class T>
static void CopyMap( std::map<int,T>& src , std::map<int,T>& dst ){
	for( auto& p : src ){
		dst[p.first] = p.second;
	}
}



template<class T>
static void UpdateExistingFromMap( std::map<int,T>& map , std::map<int,T>& other ){
	for( auto& p : map ){
		map[p.first] = other[p.first];
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


void D3D9ProxyStateBlock::init(){

	if( type == D3DSBT_ALL ){
		selectIndexBuffer		 = true;
		selectViewport			 = true;
		selectViewTransform      = true;
		selectProjTransform      = true;

		storedTextures = device->activeTextures;
		storedVertexes = device->activeVertexes;
	}


	if( type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE ){
		selectVertexShader		 = true;
		selectVertexDeclaration	 = true;

		for( int c=0 ; c< device->vsConstants.registerCount() ; c++ ){
			storedVsConstants[c] = D3DXVECTOR4();
		}
	}


	if( type == D3DSBT_ALL || type == D3DSBT_PIXELSTATE ){
		selectPixelShader = true;

		for( int c=0 ; c< device->psConstants.registerCount() ; c++ ){
			storedPsConstants[c] = D3DXVECTOR4();
		}
	}


	if ( type == 0 ) {
		selectAuto = true;
	}else{
		captureSelected();
	}
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


void D3D9ProxyStateBlock::capturePixelShader( D3D9ProxyPixelShader* shader ){
	selectPixelShader |= selectAuto;
	storedPixelShader  = shader;
}


void D3D9ProxyStateBlock::captureVertexShader( D3D9ProxyVertexShader* shader ){
	selectVertexShader |= selectAuto;
	storedVertexShader  = shader;
}


void D3D9ProxyStateBlock::captureVertexDeclaration ( D3D9ProxyVertexDeclaration* decl ){
	selectVertexDeclaration |= selectAuto;
	storedVertexDeclaration  = decl;
}


void D3D9ProxyStateBlock::captureTextureSampler( int stage , IDirect3DBaseTexture9* texture ){
	storedTextures[stage] = texture;

	updateCaptureSideTracking();
}


void D3D9ProxyStateBlock::captureVertexStream( int stream , D3D9ProxyVertexBuffer* buffer ){
	storedVertexes[stream] = buffer;
}



void D3D9ProxyStateBlock::captureVertexShaderConstant( int index , const float* data , int count ){
	for( int c=0 ; c<count ; c++ ){
		storedVsConstants[index+c] = D3DXVECTOR4(data + c*4);
	}
}



void D3D9ProxyStateBlock::capturePixelShaderConstant( int index , const float* data , int count ){
	for( int c=0 ; c<count ; c++ ){
		storedPsConstants[index+c] = D3DXVECTOR4(data + c*4);
	}
}



void D3D9ProxyStateBlock::captureSelected( ){

	if( selectIndexBuffer ){
		storedIndexBuffer = device->activeIndicies;
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
		storedPixelShader = device->activePixelShader;
	}

	if( selectVertexShader ){
		storedVertexShader = device->activeVertexShader;
	}

	if( selectVertexDeclaration ){
		storedVertexDeclaration = device->activeVertexDeclaration;
	}

	if( type == D3DSBT_ALL ){
		storedTextures = device->activeTextures;
		storedVertexes = device->activeVertexes;
	}

	if( type == 0 ){
		UpdateExistingFromMap( storedTextures , device->activeTextures );
		UpdateExistingFromMap( storedVertexes , device->activeVertexes );
	}

	if( type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE || type == 0 ){
		GetSelectedConstants( storedVsConstants , device->vsConstants );
	}

	if( type == D3DSBT_ALL || type == D3DSBT_PIXELSTATE || type == 0 ){
		GetSelectedConstants( storedPsConstants , device->psConstants );
	}


	if( device->m_currentRenderingSide == vireio::Left ){
		sideLeft  = true;
		sideRight = false;
	}else{
		sideLeft   = false;
		sideRight  = true;
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
		device->activeIndicies = storedIndexBuffer;
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
		device->activePixelShader = storedPixelShader;
	}

	if( selectVertexShader ){
		device->activeVertexShader = storedVertexShader;
	}

	if( selectVertexDeclaration ){
		device->activeVertexDeclaration = storedVertexDeclaration;
	}

	if( type == D3DSBT_ALL ){
		device->activeVertexes = storedVertexes;
		device->activeTextures = storedTextures;
	}

	if( type == 0 ){
		UpdateExistingFromMap( device->activeVertexes , storedVertexes );
		UpdateExistingFromMap( device->activeTextures , storedTextures );
	}


	if( reApplyStereo ){
		for( auto& p : storedTextures ){
			device->SetTexture( p.first , p.second );
		}
	}

	ApplyExistingConstants( storedVsConstants , device->vsConstants );
	ApplyExistingConstants( storedPsConstants , device->psConstants );

	if( reApplyStereo ){
		device->switchDrawingSide();
	}

	return D3D_OK;
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

