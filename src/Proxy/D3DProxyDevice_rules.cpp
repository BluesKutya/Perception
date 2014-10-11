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
						   (r.constants.contains(c.name) || s->visible) &&
							!allConstants.contains(c.name)
						){
							allConstants += c.name;
						}
					}
				}

				r.itemConstants->removeChildren();

				for( QString constant : allConstants ){
					cMenuItem* cb = r.itemConstants->addCheckbox( constant , 0 );

					cb->internalBool = r.constants.contains( constant );

					cb->callback = [&r,constant,cb,this](){
						r.constants.removeAll( constant );

						if( cb->internalBool ){
							r.constants.append( constant );
						}

						rulesUpdate();
					};
				}
			};



			i = r.item->addCheckbox( "Apply for vertex shaders" , &r.shadersVs );
			i->callback = [this](){
				rulesUpdate();
			};

			i = r.item->addCheckbox( "Apply for pixel  shaders" , &r.shadersPs );
			i->callback = [this](){
				rulesUpdate();
			};

			i = r.item->addCheckbox( "Apply for shaders" , &r.shadersIsExclude , "all, except selected" , "selected only" );
			i->callback = [this](){
				rulesUpdate();
			};


			r.itemShaders = r.item->addSubmenu( "Select shaders" );
			r.itemShaders->callback = [&r,this](){
				r.itemShaders->removeChildren();

				for( cShader* s : shaders ){
					if( !r.shaders.contains(s->name) ){
						if( !s->visible || (s->vs && !r.shadersVs)  ||  (s->ps && !r.shadersPs) ){
							continue;
						}
					}

					cMenuItem* sel = r.itemShaders->addCheckbox( (s->vs ? "VS " : "PS ") + s->name , 0 );
					s->item = sel;
		 
					sel->internalBool = r.shaders.contains( s->name );
		
					sel->callback = [&r,s,sel,this](){
						r.shaders.removeAll( s->name );

						if( sel->internalBool == 1 ){
							r.shaders += s->name;
						}

						rulesUpdate();
					};
				}
			};


			r.item->addCheckbox ( "Constant type" , &r.isMatrix , "matrix" , "vector" )->callback = [this](){
				rulesUpdate();
			};


			QStringList ops = cRegisterModification::availableOperations();

			for( int c=0 ; c<ops.count() ; c++ ){
				i = r.item->addCheckbox( ops[c] , 0 );
				
				i->internalBool = (r.operation & (1<<c));

				i->callback = [this,&r,c,i](){
					r.operation = (r.operation & ~(1<<c)) | (i->internalBool<<c);
					rulesUpdate();
				};
			}

			r.item->addCheckbox( "Shader squish viewport" , &r.squishViewport )->callback = [this](){
				rulesUpdate();
			};

			r.item->addCheckbox( "Shader blink" , &r.shaderBlink )->callback = [this](){
				rulesUpdate();
			};

			r.item->addCheckbox( "Shader hide" , &r.shaderHide )->callback = [this](){
				rulesUpdate();
			};

			r.item->addCheckbox( "Shader disable z" , &r.shaderDisableZ )->callback = [this](){
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
		s->doDisableZ     = false;

		for( cShaderConstant& c : s->constants ){
			if( s->vs ){
				vsConstants.resize( c.RegisterIndex + c.RegisterCount );
			}else{
				psConstants.resize( c.RegisterIndex + c.RegisterCount );
			}
		}


		for( cRule& r : config.rules ){
			if( s->vs && !r.shadersVs ){
				continue;
			}

			if( s->ps && !r.shadersPs ){
				continue;
			}

			if( r.shadersIsExclude ){
				if( r.shaders.contains(s->name) ){
					continue;
				}
			}else{
				if( !r.shaders.contains(s->name) ){
					continue;
				}
			}
			
							
			for( cShaderConstant& c : s->constants ){
				if( c.isMatrix() == r.isMatrix && r.constants.contains(c.name) ){
					cRegisterModification m;
					m.start     = c.RegisterIndex;
					m.count     = c.RegisterCount;
					m.isMatrix  = r.isMatrix;
					m.operation = r.operation;
					s->modifications.push_back( m );

				}
			}
				
			s->squishViewport |= r.squishViewport;
			s->blink          |= r.shaderBlink;
			s->hide           |= r.shaderHide;
			s->doDisableZ     |= r.shaderDisableZ;
		}
	}
}



void D3DProxyDevice::rulesApply( ){
	vsConstants.applyStereo();
	psConstants.applyStereo();
}

void D3DProxyDevice::rulesPreDraw( ){
	if( (activePixelShader && activePixelShader->doDisableZ) ||
		(activeVertexShader && activeVertexShader->doDisableZ)
	){
		rulesZDisable = true;
		GetRenderState( D3DRS_ZENABLE , &rulesZPrev );
		SetRenderState( D3DRS_ZENABLE , FALSE );

	}else{
		rulesZDisable = false;
	}
}

void D3DProxyDevice::rulesPostDraw( ){
	if( rulesZDisable ){
		SetRenderState( D3DRS_ZENABLE , rulesZPrev );
	}
}