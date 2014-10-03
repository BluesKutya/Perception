#include "D3D9ProxyPixelShader.h"

D3D9ProxyPixelShader::D3D9ProxyPixelShader(IDirect3DPixelShader9* pActualPixelShader, D3DProxyDevice *pOwningDevice) :
	cBase( pActualPixelShader , pOwningDevice )  ,
	cShader( pOwningDevice , 0 , pActualPixelShader )
{

}


METHOD_THRU( HRESULT , WINAPI , D3D9ProxyPixelShader , GetFunction , void* , pDate , UINT* , pSizeOfData )
