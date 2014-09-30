#include <Vireio.h>
#include "D3D9ProxyVertexShader.h"


D3D9ProxyVertexShader::D3D9ProxyVertexShader(IDirect3DVertexShader9* pActualVertexShader, D3DProxyDevice *pOwningDevice, ShaderModificationRepository* pModLoader) :
	cBase( pActualVertexShader , pOwningDevice ) ,
	cShader( pOwningDevice , pActualVertexShader , 0 )
{
	if( pModLoader ){
		m_modifiedConstants = pModLoader->GetModifiedConstantsF(pActualVertexShader);
		m_bSquishViewport = pModLoader->SquishViewportForShader(pActualVertexShader);
	}else{
		m_bSquishViewport = false;
	}
}


METHOD_THRU( HRESULT , WINAPI , D3D9ProxyVertexShader , GetFunction , void* , pDate , UINT* , pSizeOfData )


