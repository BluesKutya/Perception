#include "D3DProxyDevice.h"
#include "D3D9ProxyVertexShader.h"
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
			rulesSave();
		};

		rulesMenu->addAction( "Load rules" )->callback = [this](){
			rulesLoad();
		};
	}
}





cRule* D3DProxyDevice::rulesAdd( ){
	cRule* r = new cRule(this);
	r->name          = QString("Rule %1").arg( rules.count() );
	r->operation     = 0;
	r->isMatrixRule  = true;
	r->transpose     = config.shaderAnalyzerTranspose;


	if( config.shaderAnalyzer ){
		cMenuItem* i;

		r->item = rulesMenu->addSubmenu( r->name );
		


		r->itemConstants = r->item->addSubmenu( "Select constants" );
		r->itemConstants->callback = [r,this](){
			QStringList allConstants;

			for( cShader* s : shaders ){
				for( cShaderConstant& c : s->constants ){
					if( c.isMatrix() == r->isMatrixRule  &&
					   (r->constantsInclude.contains(c.name) || s->visible) &&
					    !allConstants.contains(c.name)
					){
						allConstants += c.name;
					}
				}
			}

			r->itemConstants->removeChildren();

			for( QString constant : allConstants ){
				cMenuItem* cb = r->itemConstants->addCheckbox( constant , 0 );

				cb->internalBool = r->constantsInclude.contains( constant );

				cb->callback = [=](){
					if( cb->internalBool ){
						r->constantsInclude.append( constant );
					}else{
						r->constantsInclude.removeAll( constant );
					}
					rulesUpdate();
				};
			}
		};





		r->itemShaders = r->item->addSubmenu( "Select shaders" );
		r->itemShaders->callback = [r,this](){
			r->itemShaders->removeChildren();

			for( cShader* s : shaders ){
				cMenuItem* sel = r->itemShaders->addSelect( s->name , 0 , QStringList()<<"ignore"<<"include"<<"exclude" );

				sel->visible = s->visible;

				s->item = sel;
		 
				sel->internalInt = 0;

				if( r->shadersInclude.contains( s->name ) ){
					sel->internalInt = 1;
				}

				if( r->shadersExclude.contains( s->name ) ){
					sel->internalInt = 2;
				}
		
				sel->callback = [=](){
					r->shadersInclude.removeAll( s->name );
					r->shadersExclude.removeAll( s->name );

					if( sel->internalInt == 1 ){
						r->shadersInclude += s->name;
					}
			
					if( sel->internalInt == 2 ){
						r->shadersExclude += s->name;
					}

					rulesUpdate();
				};
			}
		};


		r->item->addCheckbox ( "Type" , &r->isMatrixRule , "matrix" , "vector" );

		r->item->addSelect  ( "Operation" , &r->operation , r->availableOperations() );

		r->item->addCheckbox( "Transpose" , &r->transpose );

		i = r->item->addAction( "Delete" );
		i->callback = [this,r](){
			menu.goToMenu( rulesMenu );
			rulesDelete( r );
			rulesUpdate();
		};
	}

	rules += r;

	return r;
}



void D3DProxyDevice::rulesDelete( cRule* r ){
	rules.removeAll( r );

	if( r->item ){
		delete r->item;
	}

	delete r;
}



void D3DProxyDevice::rulesFree( ){
	if( rulesMenu ){
		delete rulesMenu;
	}

	while( !rules.isEmpty() ){
		rulesDelete( rules.first() );
	}

	rulesUpdate();
}



void D3DProxyDevice::rulesSave( ){
	config.rules.clear();

	for( cRule* rule : rules ){
		rule->operationName = rule->availableOperations()[ rule->operation ];
		config.rules += *rule;
	}

	config.saveRules();
}



void D3DProxyDevice::rulesLoad( ){
	while( !rules.isEmpty() ){
		rulesDelete( rules.first() );
	}

	rules.clear();

	for( cRuleInfo& info : config.rules ){
		cRule* rule = rulesAdd();
		*static_cast<cRuleInfo*>(rule) = info;

		rule->operation = rule->availableOperations().indexOf( rule->operationName );
		if( rule->operation < 0 ){
			rule->operation = 0;
		}
	}

	rulesUpdate();
}







void D3DProxyDevice::rulesUpdate( ){
	
	for( cRule* r : rules ){
		r->registerIndexes.clear();

		for( cShader* s : shaders ){
			for( cShaderConstant& c : s->constants ){
				if( c.isMatrix() == r->isMatrixRule && r->constantsInclude.contains(c.Name) ){
					r->registerIndexes += c.RegisterIndex;
				}
			}
		}
	}

	for( cShader* s : shaders ){
		s->rules.clear();

		for( cRule* rule : rules ){

			if( !rule->shadersInclude.isEmpty() && !rule->shadersInclude.contains(s->name) ){
				continue;
			}

			if( rule->shadersExclude.contains(s->name) ){
				continue;
			}

			s->rules += rule;
		}
	}
}



void D3DProxyDevice::rulesApply( ){
	if( !currentVS ){
		return;
	}

	if( m_pCapturingStateTo ){
		printf("CAPTURE!\n");
		return;
	}
	
	QList<cRule*>* list;

	if( currentVS ){
		list = &currentVS->rules;
	}else{
		list = &rules;
	}

	for( cRule* rule : *list ){
		rule->applyTo( &vsConstantsOriginal , &vsConstantsLeft , &vsConstantsRight );
	}

	if( m_currentRenderingSide == vireio::Left ){
		vsConstantsLeft.writeTo( actual );
	}else{
		vsConstantsRight.writeTo( actual );
	}
}





			//float d[16];
			//rule->apply( constant.value , d , m_currentRenderingSide );
				
			//actual->SetVertexShaderConstantF( constant.RegisterIndex , constant.value , std::min((int)constant.RegisterCount,4) );
			//applied = true;

			/*
			printf("\n\nout:\n");
			for( int c=0 ; c<16 ; c++ ){
				printf("%12.4f " , constant.value[c] );
				if( (c&3)==3 ){
					printf("\n");
				}
			}*/

			//break;