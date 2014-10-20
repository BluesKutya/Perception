#include "D3DProxyDevice.h"


#define CallStereo( f )				 \
	HRESULT result = f;				 \
	if( FAILED(result) ){			 \
		return result;				 \
	}								 \
	if( switchDrawingSide() ) {      \
		result = f;                  \
	}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , Clear , DWORD , Count , CONST D3DRECT* , pRects , DWORD , Flags , D3DCOLOR , Color , float , Z , DWORD , Stencil )
	CallStereo( actual->Clear( Count , pRects , Flags , Color , Z , Stencil ) );
	return result;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawPrimitive , D3DPRIMITIVETYPE , PrimitiveType , UINT , StartVertex , UINT , PrimitiveCount )
	if( isDrawHide() ){
		return S_OK;
	}

	rulesApply();
	rulesPreDraw();

	CallStereo( actual->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount) );

	rulesPostDraw();

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawIndexedPrimitive , D3DPRIMITIVETYPE , PrimitiveType , INT , BaseVertexIndex , UINT , MinVertexIndex , UINT , NumVertices , UINT , startIndex , UINT , primCount )
	if( isDrawHide() ){
		return S_OK;
	}

	rulesApply();
	rulesPreDraw();

	CallStereo( actual->DrawIndexedPrimitive(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount) );

	rulesPostDraw();

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawPrimitiveUP , D3DPRIMITIVETYPE , PrimitiveType , UINT , PrimitiveCount , CONST void* , pVertexStreamZeroData , UINT , VertexStreamZeroStride )
	if( isDrawHide() ){
		return S_OK;
	}

	rulesApply();
	rulesPreDraw();

	CallStereo( actual->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride) );

	rulesPostDraw();

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawIndexedPrimitiveUP , D3DPRIMITIVETYPE , PrimitiveType , UINT , MinVertexIndex , UINT , NumVertices , UINT , PrimitiveCount , CONST void* , pIndexData , D3DFORMAT , IndexDataFormat , CONST void* , pVertexStreamZeroData , UINT , VertexStreamZeroStride )
	if( isDrawHide() ){
		return S_OK;
	}

	rulesApply();
	rulesPreDraw();

	CallStereo( actual->DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride) );

	rulesPostDraw();

	return result;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawRectPatch , UINT , Handle , CONST float* , pNumSegs , CONST D3DRECTPATCH_INFO* , pRectPatchInfo )
	rulesApply();
	rulesPreDraw();

	CallStereo( actual->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo) );

	rulesPostDraw();

	return result;
}



METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawTriPatch , UINT , Handle , CONST float* , pNumSegs , CONST D3DTRIPATCH_INFO* , pTriPatchInfo )
	rulesApply();
	rulesPreDraw();

	CallStereo( actual->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo) );

	rulesPostDraw();

	return result;
}



