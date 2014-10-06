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

	storedIndexBuffer	    = nullptr;
	storedVertexShader	    = nullptr;
	storedPixelShader	    = nullptr;
	storedVertexDeclaration = nullptr;

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

D3D9ProxyStateBlock::~D3D9ProxyStateBlock(){
	SAFE_RELEASE( storedTextureStages );
	SAFE_RELEASE( storedVertexStreams );
	SAFE_RELEASE( storedIndexBuffer );
	SAFE_RELEASE( storedVertexShader );
	SAFE_RELEASE( storedPixelShader );
	SAFE_RELEASE( storedVertexDeclaration );
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
		SAFE_ASSIGN( device->m_pActiveIndicies , storedIndexBuffer );
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
		SAFE_ASSIGN( device->psCurrent , storedPixelShader );
	}

	if( selectVertexShader ){
		SAFE_ASSIGN( device->vsCurrent , storedVertexShader );
	}

	if( selectVertexDeclaration ){
		SAFE_ASSIGN( device->m_pActiveVertexDeclaration , storedVertexDeclaration );
	}

	SAFE_RELEASE( device->m_activeVertexBuffers.values() );
	device->m_activeVertexBuffers.clear();
	for( int key : storedVertexStreams.keys() ){
		device->m_activeVertexBuffers[key] = storedVertexStreams[key];
	}

	SetConstants( &storedVsConstants , &device->vsConstantsOriginal );
	SetConstants( &storedPsConstants , &device->psConstantsOriginal );
		
	if( reApplyStereo ){
		for( int id : storedTextureStages.keys() ){
			device->SetTexture( id , storedTextureStages[id] );
		}
	}else{
		SAFE_ASSIGN( device->m_activeTextureStages , storedTextureStages );
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


void D3D9ProxyStateBlock::capturePixelShader( D3D9ProxyPixelShader* shader ){
	selectPixelShader |= selectAuto;

	SAFE_ASSIGN( storedPixelShader , shader );
}


void D3D9ProxyStateBlock::captureVertexShader( D3D9ProxyVertexShader* shader ){
	selectVertexShader |= selectAuto;
	SAFE_ASSIGN( storedVertexShader , shader );
}


void D3D9ProxyStateBlock::captureVertexDeclaration ( D3D9ProxyVertexDeclaration* decl ){
	selectVertexDeclaration |= selectAuto;
	SAFE_ASSIGN( storedVertexDeclaration , decl );
}


void D3D9ProxyStateBlock::captureTextureSampler( int stage , IDirect3DBaseTexture9* texture ){
	SAFE_ASSIGN( storedTextureStages[stage] , texture );
	updateCaptureSideTracking();
}


void D3D9ProxyStateBlock::captureVertexStream( int stream , D3D9ProxyVertexBuffer* buffer ){
	SAFE_ASSIGN( storedVertexStreams[stream]  , buffer );
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
		SAFE_ASSIGN( storedIndexBuffer , device->m_pActiveIndicies );
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
		SAFE_ASSIGN( storedPixelShader , device->psCurrent );
	}

	if( selectVertexShader ){
		SAFE_ASSIGN( storedVertexShader , device->vsCurrent );
	}

	if( selectVertexDeclaration ){
		SAFE_ASSIGN( storedVertexDeclaration , device->m_pActiveVertexDeclaration );
	}

	if( type == D3DSBT_ALL ){
		for( int key : device->m_activeTextureStages.keys() ){
			SAFE_ASSIGN( storedTextureStages[key] , device->m_activeTextureStages[key] );
		}

		for( int key : device->m_activeVertexBuffers.keys() ){
			SAFE_ASSIGN( storedVertexStreams[key] , device->m_activeVertexBuffers[key] );
		}
	}

	if( type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE ){
		GetConstants( &storedVsConstants , &device->vsConstantsOriginal , false );
	}

	if( type == D3DSBT_ALL || type == D3DSBT_PIXELSTATE ){
		GetConstants( &storedPsConstants , &device->psConstantsOriginal , false );
	}


	if( type == 0 ){
		float vec[4];

		for( int c : storedTextureStages.keys() ){
			SAFE_ASSIGN( storedTextureStages[c] , device->m_activeTextureStages[c] );
		}

		for( int c : storedVertexStreams.keys() ){
			SAFE_ASSIGN( storedVertexStreams[c] , device->m_activeVertexBuffers[c] );
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

