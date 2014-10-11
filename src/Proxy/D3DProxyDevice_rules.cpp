#include "D3DProxyDevice.h"
#include "D3D9ProxyVertexShader.h"
#include "D3D9ProxyPixelShader.h"
#include <cPropsFile.h>
#include <qdir.h>



void D3DProxyDevice::rulesInit( ){
	rulesMenu = 0;

	if( config.shaderAnalyzer ){
		rulesMenu = menu.root.addSubmenu( "Shader rules" );

		rulesMenu->addAction( "Add new rule" )->callback = [this](){
			rulesAdd();
			rulesUpdate();
		};

		rulesMenu->addAction( "Save rules" )->callback = [this](){
			config.saveRules();
		};

		rulesMenu->addAction( "Load rules" )->callback = [this](){
			config.loadRules();
			rulesUpdate();
		};
	}

	config.loadRules();
	rulesUpdate();
}





void D3DProxyDevice::rulesAdd( ){
	config.rules += cRule();

	cRule& r = config.rules.last();
	r.name      = QString("Rule %1").arg( config.rules.count() );
	r.operation = 0;
	r.isMatrix  = true;
	r.transpose = config.shaderAnalyzerTranspose;

	rulesUpdate();

}



void D3DProxyDevice::rulesDelete( cRule& r ){
	if( r.item ){
		delete r.item;
	}

	for( int c=0 ; c<config.rules.count() ; c++ ){
		if( &r == &config.rules[c] ){
			config.rules.removeAt(c);
			break;
		}
	}

	rulesUpdate();
}


void D3DProxyDevice::rulesUpdate( ){


	if( config.shaderAnalyzer ){
		for( cRule& r : config.rules ){
			if( r.item ){
				continue;
			}

			cMenuItem* i;

			r.item = rulesMenu->addSubmenu( r.name );
	

			r.itemConstants = r.item->addSubmenu( "Select constants" );
			r.itemConstants->callback = [&r,this](){
				QStringList allConstants;

				for( cShader* s : shaders ){
					for( cShaderConstant& c : s->constants ){
						if( c.isMatrix() == r.isMatrix  &&
						   (r.constantsInclude.contains(c.name) || s->visible) &&
							!allConstants.contains(c.name)
						){
							allConstants += c.name;
						}
					}
				}

				r.itemConstants->removeChildren();

				for( QString constant : allConstants ){
					cMenuItem* cb = r.itemConstants->addCheckbox( constant , 0 );

					cb->internalBool = r.constantsInclude.contains( constant );

					cb->callback = [&r,constant,cb,this](){
						if( cb->internalBool ){
							r.constantsInclude.append( constant );
						}else{
							r.constantsInclude.removeAll( constant );
						}
						rulesUpdate();
					};
				}
			};





			r.itemShaders = r.item->addSubmenu( "Select shaders" );
			r.itemShaders->callback = [&r,this](){
				r.itemShaders->removeChildren();

				for( QString& se : r.shadersExclude ){
					printf("excl %s\n",se.toLocal8Bit().data());
				}


				for( cShader* s : shaders ){
					cMenuItem* sel = r.itemShaders->addSelect( (s->vs ? "VS " : "PS ") + s->name , 0 , QStringList()<<"ignore"<<"include"<<"exclude" );

					sel->visible = s->visible;

					s->item = sel;
		 
					sel->internalInt = 0;

					if( r.shadersInclude.contains( s->name ) ){
						sel->internalInt = 1;
					}

					if( r.shadersExclude.contains( s->name ) ){
						sel->internalInt = 2;
					}
		
					sel->callback = [&r,s,sel,this](){
						r.shadersInclude.removeAll( s->name );
						r.shadersExclude.removeAll( s->name );

						if( sel->internalInt == 1 ){
							r.shadersInclude += s->name;
						}
			
						if( sel->internalInt == 2 ){
							r.shadersExclude += s->name;
						}

						rulesUpdate();
					};
				}
			};


			r.item->addCheckbox ( "Constant type" , &r.isMatrix , "matrix" , "vector" )->callback = [this](){
				rulesUpdate();
			};

			r.item->addSelect  ( "Constant operation" , &r.operation , cRegisterModification::availableOperations() )->callback = [this](){
				rulesUpdate();
			};

			r.item->addCheckbox( "Constant transpose" , &r.transpose      )->callback = [this](){
				rulesUpdate();
			};

			r.item->addCheckbox( "Shader squish viewport" , &r.squishViewport )->callback = [this](){
				rulesUpdate();
			};

			r.item->addCheckbox( "Shader blink" , &r.shaderBlink )->callback = [this](){
				rulesUpdate();
			};

			r.item->addCheckbox( "Shader hide" , &r.shaderHide )->callback = [this](){
				rulesUpdate();
			};


			i = r.item->addAction( "Delete" );
			i->callback = [this,&r](){
				menu.goToMenu( rulesMenu );
				rulesDelete( r );
				rulesUpdate();
			};
		}
	}




	for( cShader* s : shaders ){
		s->modifications.clear();
		s->squishViewport = false;
		s->hide           = false;
		s->blink          = false;

		for( cShaderConstant& c : s->constants ){
			if( s->vs ){
				vsConstants.resize( c.RegisterIndex + c.RegisterCount );
			}else{
				psConstants.resize( c.RegisterIndex + c.RegisterCount );
			}
		}


		for( cRule& r : config.rules ){
			if( !r.shadersExclude.contains(s->name) &&
				(r.shadersInclude.isEmpty() || r.shadersInclude.contains(s->name))
			){
				
				for( cShaderConstant& c : s->constants ){
					if( c.isMatrix() == r.isMatrix && r.constantsInclude.contains(c.name) ){
						cRegisterModification m;
						m.start     = c.RegisterIndex;
						m.count     = c.RegisterCount;
						m.isMatrix  = r.isMatrix;
						m.transpose = r.transpose;
						m.operation = r.operation;
						s->modifications.push_back( m );

					}
				}
				
				s->squishViewport |= r.squishViewport;
				s->blink          |= r.shaderBlink;
				s->hide           |= r.shaderHide;
			}
		}
	}
}



void D3DProxyDevice::rulesApply( ){
	vsConstants.applyStereo();
	psConstants.applyStereo();
}