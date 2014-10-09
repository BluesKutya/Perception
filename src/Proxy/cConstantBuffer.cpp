#include "cConstantBuffer.h"
#include "D3DProxyDevice.h"
#include "D3D9ProxyPixelShader.h"
#include "D3D9ProxyVertexShader.h"


cConstantBuffer::cConstantBuffer( D3DProxyDevice* d , bool v ){
	device        = d;
	isVertex      = v;
	modifications = 0;
}



HRESULT cConstantBuffer::set( std::vector<float>& array , int start , const float* ptr , int count ){
	resize( start + count );

	if( start >= registerCount()  ||  start + count > registerCount() ){
		printf("Vireio: cConstantBuffer::set out or range\n" );
		return D3DERR_INVALIDCALL;
	}

	memcpy( array.data() + start*4 , ptr , count*4*sizeof(float) );

	setModified( start , count );

	return D3D_OK;
}



void cConstantBuffer::write( std::vector<float>& array ){
	setStereoModified( );
	
	for( auto& range : modified ){
		HRESULT r;

		if( isVertex ){
			r = device->actual->SetVertexShaderConstantF( range.first , array.data() + range.first*4 , range.second - range.first );
		}else{
			r = device->actual->SetPixelShaderConstantF ( range.first , array.data() + range.first*4 , range.second - range.first );
		}

		if( FAILED(r) ){
			printf("Vireio: cConstantBuffer::write failed!\n");
		}
	}

	modified.clear();
}





void cConstantBuffer::clearModified( ){
	modified.clear();
}



bool cConstantBuffer::isModified( int start , int count ){
	int r1 = start;
	int r2 = start + count;
	for( auto& range : modified ){
		if( r1 <= range.second  &&  r2 >= range.first ){
			return true;
		}
	}
	return false;
}



void cConstantBuffer::setModified( int start , int count ){
	int r1 = start;
	int r2 = start + count;
	for( auto& range : modified ){
		if( r1 <= range.second  &&  r2 >= range.first ){
			range.first  = std::min( r1 , range.first  );
			range.second = std::max( r2 , range.second );
			return;
		}
	}
	modified.push_back( std::pair<int,int>( r1 , r2 ) );
}



void cConstantBuffer::setStereoModified( ){
	if( modifications ){
		for( cRegisterModification& m : *modifications ){
			setModified( m.start , m.count );
		}
	}
}









HRESULT cConstantBuffer::set( int start , const float* ptr , int count ){
	return set( original , start , ptr , count );
}



HRESULT cConstantBuffer::get( int start , float* ptr , int count ){
	if( start >= registerCount()  ||  start + count > registerCount() ){
		return D3DERR_INVALIDCALL;
	}

	memcpy( ptr , original.data() + start*4 ,  count*4*sizeof(float) );

	return D3D_OK;
}



void cConstantBuffer::resize( int count ){
	count *= 4;

	if( count <= original.size() ){
		return;
	}
	
	original.resize( count );
	left    .resize( count );
	right   .resize( count );
	modified.resize( count );
}








void cConstantBuffer::applyStereo( ){
	if( isVertex ){
		if( device->activeVertexShader ){
			modifications = &device->activeVertexShader->modifications;
		}else{
			modifications = 0;
		}
	}else{
		if( device->activeVertexShader ){
			modifications = &device->activePixelShader->modifications;
		}else{
			modifications = 0;
		}
	}


	for( auto& range :  modified ){
		memcpy( left .data() + range.first*4 , original.data() + range.first*4 , (range.second-range.first)*4*sizeof(float) );
		memcpy( right.data() + range.first*4 , original.data() + range.first*4 , (range.second-range.first)*4*sizeof(float) );
	}


	if( modifications ){
		for( cRegisterModification& m : *modifications ){
			if( isModified( m.start , m.count ) ){
				int offset  = m.start * 4;
				m.modify( device , original.data() + offset , left.data() + offset , right.data() + offset );
			}
		}
	}

	if( device->m_currentRenderingSide == vireio::Left ){
		write( left );
	}else{
		write( right );
	}
}



/*
bool cConstantBuffer::clearModified( int registerIndex , int registerCount ){
	int r1 = registerIndex;
	int r2 = registerIndex + registerCount;
	for( auto& range : modified ){
		if( r1 < range.second  &&  r2 > range.first ){
			if( r1 <= range.first && r2 >= range.second ){
				//clear whole modified block
				//faster than erase?
				range.second = range.first;
			}else
			if( r1 > range.first && r2 < range.second ){
				//clear mid-part of block

				//add right-part
				setModified( r2 , range.second - r2 );

				//modify this for left-part
				range.second = r1;
			}else{
				if( r1 <= range.first ){
					range.first = r2;
				}

				if( r2 >= range.second ){
					range.second = r1;
				}
			}
			return true;
		}
	}
	return false;
}







*/






int cConstantBuffer::registerCount( ){
	return original.size() / 4;
}

/*
void cConstantBuffer::readModifiedFrom( cConstantBuffer* buf ){
	for( auto& range : modified ){
		int      c = range.second-range.first;
		float* src = buf->data(range.first,c);
		float* dst = data(range.first,c);

		memcpy( dst , src , 4*c*sizeof(float) );
	}
}
*/