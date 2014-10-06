#include "cConstantBuffer.h"
#include <d3dx9.h>


void cConstantBuffer::resize( int registerCount , bool mod ){
	if( registerCount*4 > registers.size() ){
		registers.resize( registerCount*4 );
	}
}



bool cConstantBuffer::isModified( int registerIndex , int registerCount ){
	int r1 = registerIndex;
	int r2 = registerIndex + registerCount;
	for( auto& range : modified ){
		if( r1 < range.second  &&  r2 > range.first ){
			return true;
		}
	}
	return false;
}



void cConstantBuffer::setModified( int registerIndex , int registerCount ){
	int r1 = registerIndex;
	int r2 = registerIndex + registerCount;
	for( auto& range : modified ){
		if( r1 < range.second  &&  r2 > range.first ){
			range.first  = std::min( r1 , range.first  );
			range.second = std::max( r2 , range.second );
			return;
		}
	}

	modified.push_back( std::pair<int,int>( r1 , r2 ) );
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
	for( auto& range : modified ){
		if( vs ){
			device->SetVertexShaderConstantF( range.first , registers.data() + range.first*4 , range.second - range.first );
		}else{
			device->SetPixelShaderConstantF ( range.first , registers.data() + range.first*4 , range.second - range.first );
		}
	}
	modified.clear();
}



int cConstantBuffer::registerCount( ){
	return registers.size() / 4;
}