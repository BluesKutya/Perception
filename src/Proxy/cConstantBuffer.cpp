#include "cConstantBuffer.h"
#include <d3dx9.h>


void cConstantBuffer::resize( int registerCount , bool mod ){
	if( registerCount > modified.size() ){
		int oldCount = modified.count();

		modified.resize( registerCount );

		for( int c=oldCount ; c<registerCount ; c++ ){
			modified[c] = mod;
		}

		registers.resize( modified.count() * 4 );
	}
}



bool cConstantBuffer::isModified( int registerIndex , int registerCount ){
	for( int c=0 ; c<registerCount ; c++ ){
		if( modified[registerIndex+c] ){
			return true;
		}
	}
	return false;
}



void cConstantBuffer::setModified( int registerIndex , int registerCount ){
	for( int c=0 ; c<registerCount ; c++ ){
		modified[registerIndex+c] = true;
	}
}




float* cConstantBuffer::data( int registerIndex , int registerCount ){
	resize( registerIndex + registerCount , false );
	return registers.data() + registerIndex*4;
}



bool cConstantBuffer::set( int registerIndex , const float* ptr , int registerCount ){
	float* buf = data( registerIndex , registerCount );
	memcpy( buf , ptr , registerCount*4*sizeof(float) );
	setModified( registerIndex , registerCount );
	return true;
}


bool cConstantBuffer::get( int registerIndex , float* ptr , int registerCount ){
	float* buf = data( registerIndex , registerCount );
	memcpy( ptr , buf ,  registerCount * 4 * sizeof(float) );
	return true;
}



void cConstantBuffer::writeTo( IDirect3DDevice9* device , bool vs ){
	int begin = 0;
	int end   = 0;

	for(;;){
		while( begin < modified.size() && !modified[begin] ){
			begin++;
		}

		end = begin;

		while( end < modified.size() && modified[end] ){
			modified[end] = false;
			end++;
		}

		if( begin >= modified.size() ){
			return;
		}

		if( vs ){
			device->SetVertexShaderConstantF( begin , registers.data() + begin*4 , end - begin);
		}else{
			device->SetPixelShaderConstantF ( begin , registers.data() + begin*4 , end - begin);
		}

		begin = end;
	}
}