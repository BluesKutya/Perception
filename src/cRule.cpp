#include "cRule.h"
#include "D3DProxyDevice.h"
#include <cPropsFile.h>


static D3DXMATRIX translateUV;
static D3DXMATRIX scaleUV;
static bool       initialized;

cRule::cRule( D3DProxyDevice* d ){

	if( !initialized ){
		initialized = true;

		D3DXMatrixTranslation( &translateUV , 0.5f , 0.5f , 0.5f );
		D3DXMatrixScaling    ( &scaleUV     , 0.5f , 0.5f , 0.0f );
	}


	item      = 0;
	device    = d;
	operation = 0;

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
	//r += "Matrix no show";                       // Causes shader to not be displayed
	return r;
}



void cRule::modify( int registerIndex , cConstantBuffer* buf , cConstantBuffer* bufLeft , cConstantBuffer* bufRight ){
	int registerCount = isMatrixRule ? 4 : 1;
	float*          ptrData  = buf     ->data( registerIndex , registerCount );
	float*          ptrLeft  = bufLeft ->data( registerIndex , registerCount );
	float*          ptrRight = bufRight->data( registerIndex , registerCount );

	bufLeft ->setModified( registerIndex , registerCount );
	bufRight->setModified( registerIndex , registerCount );

	if( !buf->isModified(registerIndex,registerCount) ){
		//return;
	}


	if( !operation ){
		memcpy( ptrLeft  , ptrData , registerCount * 4 * sizeof(float) );
		memcpy( ptrRight , ptrData , registerCount * 4 * sizeof(float) );
		return;
	}

	if( !isMatrixRule ){
		switch( operation ){
		case 1: //Vector simple translate
			ptrLeft[0] = ptrData[0] - device->separationInWorldUnits();
			ptrLeft[1] = ptrData[1];
			ptrLeft[2] = ptrData[2];
			ptrLeft[3] = ptrData[3];

			ptrRight[0] = ptrData[0] + device->separationInWorldUnits();
			ptrRight[1] = ptrData[1];
			ptrRight[2] = ptrData[2];
			ptrRight[3] = ptrData[3];
			break;

		case 2://Vector eye shift (unity)
			ptrLeft[0]  = ptrData[0] - device->separationInWorldUnits() * 5000;
			ptrLeft[1]  = ptrData[1];
			ptrLeft[2]  = ptrData[2];
			ptrLeft[3]  = ptrData[3];

			ptrRight[0] = ptrData[0] + device->separationInWorldUnits() * 5000;
			ptrRight[1] = ptrData[1];
			ptrRight[2] = ptrData[2];
			ptrRight[3] = ptrData[3];
			break;
		}
		return;
	}


	bool matrixTransform = false;


	D3DXMATRIX      in                       ( ptrData );
	D3DXMATRIX&     outLeft  = *(D3DXMATRIX*)( ptrLeft );
	D3DXMATRIX&     outRight = *(D3DXMATRIX*)( ptrRight );
	
	
	/*printf("\n\nin:\n");
	for( int c=0 ; c<16 ; c++ ){
		printf("%12.4f " , in[c] );
		if( (c&3)==3 ){
			printf("\n");
		}
	}*/


	if( transpose ){
		D3DXMatrixTranspose( &in , &in );
	}


	switch( operation ){
	case 3: //Matrix simple translate
		matrixTransform = true;
		break;


	case 8: //Matrix gathered orthographic squash
		//fall thru...

	case 4:{ //Matrix orthographic squash
		if( fabs(in[15]-1) > 0.00001 ){
			matrixTransform = true;
			break;
		}

		

		// add all translation and scale matrix entries 
		// (for the GUI this should be 3.0f, for the HUD above)
		float allAbs = abs(in(3, 0)); // transX
		allAbs += abs(in(3, 1)); // transY
		allAbs += abs(in(3, 2)); // transZ

		allAbs += abs(in(0, 0)); // scaleX
		allAbs += abs(in(1, 1)); // scaleY
		allAbs += abs(in(2, 2)); // scaleZ

		// HUD
		if( allAbs > 3.0f ){
			// separation -> distance translation
			outLeft  = in * device->viewMatHudLeft;
			outRight = in * device->viewMatHudRight;
		}else{ // GUI
			if ( config.guiBulletLabyrinth ){
				D3DXMatrixTranspose( &in , &in );
				in = device->viewMatBulletLabyrinth * in;
				D3DXMatrixTranspose( &in , &in );
			}

			outLeft  = in * device->viewMatGuiLeft;
			outRight = in * device->viewMatGuiRight;
		}
		break;
		}


	case 5:	//Matrix hud slide
		outLeft  = in * device->viewMatHudLeft;
		outRight = in * device->viewMatHudRight;
		break;


	case 6:	//Matrix gui squash
		outLeft  = in * device->viewMatGuiLeft;
		outRight = in * device->viewMatGuiRight;
		break;


	case 7:	//Matrix surface refraction transform
		// get gathered matrices
		outLeft  = device->viewMatGatheredLeft;
		outRight = device->viewMatGatheredRight;

		// matrix to be transposed ?
		if( transpose ){
			D3DXMatrixTranspose( &outLeft  , &outLeft  );
			D3DXMatrixTranspose( &outRight , &outRight );
		}

		// use gathered matrices to be scaled and translated
		outLeft  = outLeft  * scaleUV * translateUV;
		outRight = outRight * scaleUV * translateUV;

		// transpose back
		if (transpose) {
			D3DXMatrixTranspose( &outLeft  , &outLeft  );
			D3DXMatrixTranspose( &outRight , &outRight );
		}

		break;


	
	case 9:{
		//Matrix orthographic squash shifted
		if( fabs(in[15]-1) > 0.00001f ){
			matrixTransform = true;
			break;
		}
		
		// add all translation and scale matrix entries 
		// (for the GUI this should be 3.0f, for the HUD above)
		float allAbs = abs(in(3, 0)); // transX
		allAbs += abs(in(3, 1)); // transY
		allAbs += abs(in(3, 2)); // transZ

		allAbs += abs(in(0, 0)); // scaleX
		allAbs += abs(in(1, 1)); // scaleY
		allAbs += abs(in(2, 2)); // scaleZ

		// TODO !! compute these two following matrices in the ViewAdjustment class :

		// HUD
		if( allAbs > 3.0f ){
			// separation -> distance translation
			outLeft  = in * device->viewMatProjectionInv * device->viewMatLeftHud3DDepthShifted  * device->viewMatTransformLeft  * device->viewMatHudDistance * device->viewMatProjection;
			outRight = in * device->viewMatProjectionInv * device->viewMatRightHud3DDepthShifted * device->viewMatTransformRight * device->viewMatHudDistance * device->viewMatProjection;
		}else{ // GUI
			if( config.guiBulletLabyrinth ){
				D3DXMATRIX tempMatrix;
				D3DXMatrixTranspose(&tempMatrix, &in);
				tempMatrix = device->viewMatBulletLabyrinth * tempMatrix;
				D3DXMatrixTranspose(&tempMatrix, &tempMatrix);

				outLeft  = tempMatrix * device->viewMatProjectionInv * device->viewMatLeftGui3DDepth  * device->viewMatSquash * device->viewMatProjection;
				outRight = tempMatrix * device->viewMatProjectionInv * device->viewMatRightGui3DDepth * device->viewMatSquash * device->viewMatProjection;
				
				// SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
			}else{
				// simple squash
				outLeft  = in * device->viewMatProjectionInv * device->viewMatLeftGui3DDepth  * device->viewMatSquash * device->viewMatProjection;
				outRight = in * device->viewMatProjectionInv * device->viewMatRightGui3DDepth * device->viewMatSquash * device->viewMatProjection;
			}
		}

		break;
		}


	case 10:
		//Matrix orthographic squash hud
		if( fabs(in[15]-1) > 0.00001f ){
			matrixTransform = true;
			break;
		}
		
		outLeft  = in * device->viewMatHudLeft;
		outRight = in * device->viewMatHudRight;
		break;

	case 11:
		//Matrix convergence offset
		outLeft  = in * device->viewMatViewProjLeft;
		outRight = in * device->viewMatViewProjRight;
		break;
		
	case 12:
		//Matrix simple translate ignore ortho
		if( fabs( in[15] - 1 ) < 0.00001f ){
			outLeft  = in;
			outRight = in;
		}else{
			matrixTransform = true;
		}
		break;

	case 13:
		//Matrix roll only
		outLeft  = in * device->viewMatProjectionInv * device->viewMatRoll * device->viewMatProjection;
		outRight = outLeft;
		break;

	case 14:
		//Matrix roll only negative
		outLeft  = in * device->viewMatProjectionInv * device->viewMatRollNegative * device->viewMatProjection;
		outRight = outLeft;
		break;

	case 15:
		//Matrix roll only half
		outLeft  = in * device->viewMatProjectionInv * device->viewMatRollHalf * device->viewMatProjection;
		outRight = outLeft;
		break;

	case 16:
		//Matrix no roll
		outLeft  = in * device->viewMatViewProjTransformLeftNoRoll  * device->viewMatProjectionInv * device->viewMatPosition * device->viewMatProjection;
		outRight = in * device->viewMatViewProjTransformRightNoRoll * device->viewMatProjectionInv * device->viewMatPosition * device->viewMatProjection;
		break;

	}



	if( matrixTransform ){
		outLeft  = in * device->viewMatViewProjTransformLeft  * device->viewMatProjectionInv * device->viewMatPosition * device->viewMatProjection;
		outRight = in * device->viewMatViewProjTransformRight * device->viewMatProjectionInv * device->viewMatPosition * device->viewMatProjection;
	}


	if( transpose ){
		D3DXMatrixTranspose( &outLeft , &outLeft );
		D3DXMatrixTranspose( &outRight , &outRight );
	}


	if( operation == 8 ){
		device->viewMatGatheredLeft  = outLeft;
		device->viewMatGatheredRight = outRight;
	}

	buf->setModified( registerIndex , registerCount );

	/*
	printf("\n\nout:\n");
	for( int c=0 ; c<16 ; c++ ){
		printf("%12.4f " , out[c] );
		if( (c&3)==3 ){
			printf("\n");
		}
	}


	memcpy( dst , &out[0] , 4*4*sizeof(float) );*/
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