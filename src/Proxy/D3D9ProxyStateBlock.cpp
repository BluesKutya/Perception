#include "D3D9ProxyStateBlock.h"
#include <assert.h>


static void ApplyExistingConstants( std::map< int , D3DXVECTOR4 >& map , cConstantBuffer& buf ){
	for( auto p : map ){
		buf.set( p.first , p.second , 1 );
	}

	buf.clearModified();
	buf.setStereoModified();
} 


static void GetSelectedConstants( std::map< int , D3DXVECTOR4 >& map , cConstantBuffer& buf ){
	for( auto& p : map ){
		buf.get( p.first , p.second , 1 );
	}
} 


static void GetAllConstants( std::map<int,D3DXVECTOR4>& map , cConstantBuffer& buf ){
	map.clear();
	for( int c=0 ; c<buf.registerCount() ; c++ ){
		buf.get( c , map[c] , 1 );
	}
}



template<class T>
static void GetSelected( std::map<int,T>& map , std::map<int,T>& other ){
	for( auto& p : map ){
		p.second = other[p.first];
	}
}


template<class T>
static void ApplySelected( std::map<int,T>& map , std::map<int,T>& other ){
	for( auto& p : map ){
		other[p.first] = p.second;
	}
}



D3D9ProxyStateBlock::D3D9ProxyStateBlock( IDirect3DStateBlock9* pActualStateBlock , D3DProxyDevice *pOwningDevice ) :
	cBase( pActualStateBlock , pOwningDevice )
{
	type				    = 0;
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


void D3D9ProxyStateBlock::captureViewTransform( ){
	selectViewTransform |= selectAuto;
	storedViewSet        = device->transformViewSet;
	storedViewOriginal   = device->transformViewOriginal;
	storedViewLeft       = device->transformViewLeft;
	storedViewRight      = device->transformViewRight;
}


void D3D9ProxyStateBlock::captureProjTransform( ){
	selectProjTransform  |= selectAuto;
	storedProjSet         = device->transformProjSet;
	storedProjOriginal    = device->transformProjOriginal;
	storedProjLeft        = device->transformProjLeft;
	storedProjRight       = device->transformProjRight;
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
		captureViewTransform();
	}

	if( selectProjTransform ){
		captureProjTransform();
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

	if( type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE ){
		GetAllConstants( storedVsConstants , device->vsConstants );
	}


	if( type == D3DSBT_ALL || type == D3DSBT_PIXELSTATE ){
		GetAllConstants( storedPsConstants , device->psConstants );
	}


	if( type == 0 ){
		GetSelected( storedTextures , device->activeTextures );
		GetSelected( storedVertexes , device->activeVertexes );

		GetSelectedConstants( storedVsConstants , device->vsConstants );
		GetSelectedConstants( storedPsConstants , device->psConstants );
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
	HRESULT result = actual->Apply();
	if( FAILED(result) ){
		return result;
	}

	if( device->stateBlock ){
		printf("Device state block!\n");
	}

	if( selectIndexBuffer ){
		device->activeIndicies = storedIndexBuffer;
	}

	if( selectViewport ){
		device->m_LastViewportSet          = storedViewport;
		device->m_bActiveViewportIsDefault = device->isViewportDefaultForMainRT( &storedViewport );
	}

	if( selectViewTransform ){
		device->transformViewSet      = storedViewSet;
		device->transformViewOriginal = storedViewOriginal;
		device->transformViewLeft     = storedViewLeft;
		device->transformViewRight    = storedViewRight;
	}

	if( selectProjTransform ){
		device->transformProjSet      = storedProjSet;
		device->transformProjOriginal = storedProjOriginal;
		device->transformProjLeft     = storedProjLeft;
		device->transformProjRight    = storedProjRight;
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
		ApplySelected( storedVertexes , device->activeVertexes );
		ApplySelected( storedTextures , device->activeTextures );
	}

	ApplyExistingConstants( storedVsConstants , device->vsConstants );
	ApplyExistingConstants( storedPsConstants , device->psConstants );

	device->switchDrawingSide();

	return D3D_OK;
}


