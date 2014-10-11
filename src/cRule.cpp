#include "cRule.h"
#include "D3DProxyDevice.h"
#include <cPropsFile.h>


cRule::cRule(){
	operation      = 0;
	isMatrix       = false;
	transpose      = false;
	squishViewport = false;

	device        = 0;
	item          = 0;
	itemConstants = 0;
	itemShaders   = 0;
	shaderBlink   = false;
	shaderHide    = false;

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