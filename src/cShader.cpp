#include "cShader.h"
#include "D3DProxyDevice.h"
#include <qfile.h>
#include <qtextstream.h>


bool cShaderConstant::isMatrix(){
	return Class != D3DXPC_VECTOR;
}

cShader::cShader( D3DProxyDevice* d , IDirect3DVertexShader9* avs , IDirect3DPixelShader9* aps ){

	device = d;
	blink  = false;
	hide   = false;
	used   = true;
	vs     = avs;
	ps     = aps;
	item   = 0;

	ID3DXConstantTable* table = 0;

	UINT len;
	if( vs ){
		vs->GetFunction( 0 , &len );
	}else{
		ps->GetFunction( 0 , &len );			
	}

	code.resize( len );

	if( vs ){
		vs->GetFunction( code.data() , &len );
		name = "VS ";
	}else{
		ps->GetFunction( code.data() , &len );	
		name = "PS ";
	}

	
	name += QCryptographicHash::hash( code , QCryptographicHash::Md5 ).toHex().toUpper();

	D3DXGetShaderConstantTable( reinterpret_cast<DWORD*>(code.data()) , &table );


	if( table ){
		D3DXCONSTANTTABLE_DESC tableDesc;
		table->GetDesc(&tableDesc);

		for( int c=0 ; c<tableDesc.Constants ; c++ ){
			D3DXHANDLE handle = table->GetConstant( 0 , c );
			if( !handle ){
				continue;
			}

			D3DXCONSTANT_DESC constantDesc[512];
			UINT              constantCount = 512;

			table->GetConstantDesc( handle , constantDesc , &constantCount );
			
			if( constantCount >= 512 ){
				printf("Need larger constant description buffer");
			}


			for( int i=0 ; i<constantCount ; i++ ){
				D3DXCONSTANT_DESC& desc = constantDesc[i];

				

				if( desc.RegisterSet != D3DXRS_FLOAT4 ){
					continue;
				}


				QString typeName;

				switch( desc.Class ){
				case D3DXPC_VECTOR:
					typeName = "Vector";
					break;

				case D3DXPC_MATRIX_ROWS:
					typeName = "MatrixR";
					break;

				case D3DXPC_MATRIX_COLUMNS:
					typeName = "MatrixC";
					break;
				}

				if( typeName.isEmpty() ){
					continue;
				}

				constants.append( cShaderConstant() );
				
				cShaderConstant* n = &constants.last();
				*(D3DXCONSTANT_DESC*)n = desc;
				n->name     = n->Name;//QString("r%1 - %2").arg(n->RegisterIndex).arg(n->Name);
				n->typeName = typeName;
				n->item     = 0;
			}
		}
		
		table->Release();
	}

	device->shaders += this;

	device->rulesUpdate();
}


cShader::~cShader( ){
	device->shaders.removeAll( this );
}





/*

				if( config.shaderAnalyzer ){
					n->item = item->addSubmenu( "Constant \"" + n->name + "\"" );

					n->item->addAction( "Add rule" );
					n->item->addAction( "Add rule for all constants like this" );
					n->item->addAction( "Create rule" );
					*
					n->item->addCheckbox( "Apply rule" , &n->applyRule );
					n->item->callbackValueChanged = [=](){
						if( n->applyRule ){
							bool transpose = config.shaderAnalyzerTranspose;
							if( n->desc.Class == D3DXPARAMETER_CLASS::D3DXPC_VECTOR ){
								transpose = false;
							}
							addRule( n->name.toStdString() , true , n->desc.RegisterIndex , n->desc.Class , 1 , transpose );
						}else{
							deleteRule( n->name.toStdString() );
						}
					};
					*
				}*/

		/*
	if( config.shaderAnalyzer ){
		item = device->shadersMenu->addSubmenu( QString(vs?"Vertex":"Pixel") + " shader " + name );

		item->addCheckbox( "Do not draw" , &hide );
	
		item->addCheckbox( "Blink" , &blink ); 

		cMenuItem* mi = item->addAction( "Save shader to \"" + name + ".vs\"" ); 
		mi->callbackValueChanged = [=](){
			QFile file( name + ".vs" );
			if( file.open( QFile::WriteOnly ) ){

				ID3DXBuffer* buf; 
				D3DXDisassembleShader( reinterpret_cast<DWORD*>(code.data()) , false , 0 , &buf ); 

				file.write( (char*)buf->GetBufferPointer() , std::max( buf->GetBufferSize()-1 , 0UL ) );

				buf->Release();
			}
		};
	}
	*/