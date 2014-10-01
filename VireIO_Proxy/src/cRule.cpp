#include "cRule.h"
#include "D3DProxyDevice.h"


cRule::cRule( D3DProxyDevice* d ){
	item      = 0;
	device    = d;
	operation = 0;

	device->rules += this;

	if( config.shaderAnalyzer ){
		cMenuItem* i;

		item = device->rulesMenu->addSubmenu( QString("Rule %1").arg( device->rules.count() ) );
		
		itemConstants = item->addSubmenu( "Select constants" );
		itemConstants->callback = [this](){
			updateConstantsMenu();
		};

		itemShaders = item->addSubmenu( "Select shaders" );
		itemShaders->callback = [this](){
			updateShadersMenu();
		};

		item->addSelect  ( "Operation" , &operation , availableOperations() );
		item->addCheckbox( "Transpose" , &transpose );
	}

	device->applyAllRules();
}



cRule::~cRule(){
	device->rules.removeAll( this );
	
	device->applyAllRules();
}



void cRule::updateConstantsMenu(){
	if( !config.shaderAnalyzer ){
		return;
	}


	QStringList allConstants;

	for( cShader* s : device->shaders ){
		for( cShaderConstant& c : s->constants ){
			if( !allConstants.contains( c.name ) ){
				allConstants += c.name;
			}
		}
	}

	itemConstants->removeChildren();

	for( QString constant : allConstants ){
		cMenuItem* cb = itemConstants->addCheckbox( constant , 0 );

		cb->internalBool = constantNames.contains( constant );

		cb->callback = [=](){
			if( cb->internalBool ){
				this->constantNames.append( constant );
			}else{
				this->constantNames.removeAll( constant );
			}
			device->applyAllRules();
		};
	}

}


void cRule::updateShadersMenu(){
	if( !config.shaderAnalyzer ){
		return;
	}

	itemShaders->removeChildren();


	for( cShader* s : device->shaders ){
		cMenuItem* sel = itemShaders->addSelect( s->name , 0 , QStringList()<<"ignore"<<"include"<<"exclude" );

		sel->visible = s->used;

		s->item = sel;
		 
		sel->internalInt = 0;

		if( shadersInclude.contains( s->name ) ){
			sel->internalInt = 1;
		}

		if( shadersExclude.contains( s->name ) ){
			sel->internalInt = 2;
		}
		
		sel->callback = [=](){
			this->shadersInclude.removeAll( s->name );
			this->shadersExclude.removeAll( s->name );

			if( sel->internalInt == 1 ){
				this->shadersInclude += s->name;
			}
			
			if( sel->internalInt == 2 ){
				this->shadersExclude += s->name;
			}

			device->applyAllRules();
		};
	}

}



QStringList cRule::availableOperations(){
	QStringList r;
	r += "do nothing";

	r += "Vector simple translate";          // Default modification is simple translate
	r += "Vector eye shift (unity)";         //

	r += "Matrix simple translate";
	r += "Matrix orthographic squash";           // Squashes matrix if orthographic, otherwise simple translate.
	r += "Matrix hud slide";                     // Modification to slide the head up display (HUD) into the head mounted display (HMD) output.
	r += "Matrix gui squash";                    // Modification to squash the graphical user interface (GUI).
	r += "Matrix surface refraction transform";  // Modification to fix surface refraction in pixel shaders.
	r += "Matrix gathered orthographic squash";  // Squashes matrix if orthographic, otherwise simple translate. Result will be gathered to be used in other modifications.
	r += "Matrix orthographic squash shifted";   // Squashes matrix if orthographic, otherwise simple translate - shift accordingly.
	r += "Matrix orthographic squash hud";       // Squashes matrix if orthographic, otherwise simple translate - matrices treated as beeing for HUD.
	r += "Matrix convergence offset";            // Fixes far away objects using the convergence offset.
	r += "Matrix simple translate ignore ortho"; // Modification to ignore orthographic matrices.
	r += "Matrix roll only";                     // Modification applies only the head roll.
	r += "Matrix roll only negative";            // Modification applies only the head roll. (negative)
	r += "Matrix roll only half";                // Modification applies only the head roll. (half roll)
	r += "Matrix no roll";                       // Default modification without head roll
	r += "Matrix no show";                       // Causes shader to not be displayed
	return r;
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