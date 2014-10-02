#include "cRule.h"
#include "D3DProxyDevice.h"
#include "ViewAdjustment.h"
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


bool cRule::isMatrixOperation( ){
	return operation > 2;
}


void cRule::apply( float* src , float* dst , vireio::RenderPosition side ){
	bool            matrixTransform = false;
	ViewAdjustment* view            = device->m_spShaderViewAdjustment.get();
	bool            left            = (side == vireio::Left);
	float           separation      = view->SeparationInWorldUnits();;
	D3DXMATRIX&     out             = *(D3DXMATRIX*)dst;
	D3DXMATRIX      in;
	
	
	if( left ){
		separation = -separation;
	}



	if( operation > 2 && transpose ){
		D3DXMatrixTranspose( &in , (D3DXMATRIX*)src );
	}else{
		in = *(D3DXMATRIX*)src;
	}



	switch( operation ){

	case 0:
		out = *(D3DXMATRIX*)src;
		return;

	case 1:
		//Vector simple translate
		dst[0] = src[0] + separation;
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		break;

	case 2:
		//Vector eye shift (unity)
		dst[0] = src[0] + separation * 5000;
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		break;



	case 3:
		//Matrix simple translate
		matrixTransform = true;
		break;


	case 8:
		//Matrix gathered orthographic squash
		//fall thru...

	case 4:{
		//Matrix orthographic squash
		if( fabs(src[15]-1) > 0.00001 ){
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
			if( left ){
				in = in * view->LeftHUDMatrix();
			}else{
				in = in * view->RightHUDMatrix();
			}
		}else{ // GUI
			if ( config.guiBulletLabyrinth ){
				D3DXMatrixTranspose( &in , &in );
				in = view->BulletLabyrinth() * in;
				D3DXMatrixTranspose( &in , &in );
			}

			if( left ){
				in = in * view->LeftGUIMatrix();
			}else{
				in = in * view->RightGUIMatrix();
			}

		}
		break;
		}


	case 5:
		//Matrix hud slide
		if( left ){
			out = in * view->LeftHUDMatrix();
		}else{
			out = in * view->RightHUDMatrix();
		}
		break;


	case 6:
		//Matrix gui squash
		if( left ){
			out = in * view->LeftGUIMatrix();
		}else{
			out = in * view->RightGUIMatrix();
		}
		break;


	case 7:{
		//Matrix surface refraction transform
		// get gathered matrices
		D3DXMATRIX temp( left ? view->GatheredMatrixLeft() : view->GatheredMatrixRight() );

		// matrix to be transposed ?
		if( transpose ){
			D3DXMatrixTranspose( &temp , &temp );
		}

		// use gathered matrices to be scaled and translated
		out  = temp * scaleUV * translateUV;

		// transpose back
		if (transpose) {
			D3DXMatrixTranspose( &temp , &temp );
		}

		break;
		}


	
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
			if( left ){
				out = in * view->ProjectionInverse() * view->LeftHUD3DDepthShifted() * view->LeftViewTransform() * view->HUDDistance() *  view->Projection();
			}else{
				out = in * view->ProjectionInverse() * view->RightHUD3DDepthShifted() * view->RightViewTransform() * view->HUDDistance() * view->Projection();
			}
		}else{ // GUI
			if( config.guiBulletLabyrinth ){
				D3DXMATRIX tempMatrix;
				D3DXMatrixTranspose(&tempMatrix, &in);
				tempMatrix = view->BulletLabyrinth() * tempMatrix;
				D3DXMatrixTranspose(&tempMatrix, &tempMatrix);

				if( left ){
					out = tempMatrix * view->ProjectionInverse() * view->LeftGUI3DDepth() * view->Squash() * view->Projection();
				}else{
					out = tempMatrix * view->ProjectionInverse() * view->RightGUI3DDepth() * view->Squash() * view->Projection();
				}
				
				// SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
			}else{
				// simple squash
				if( left ){
					out = in * view->ProjectionInverse() * view->LeftGUI3DDepth() * view->Squash() * view->Projection();
				}else{
					out = in * view->ProjectionInverse() * view->RightGUI3DDepth() * view->Squash() * view->Projection();
				}
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

		if( left ){
			out = in * view->LeftHUDMatrix();
		}else{
			out = in * view->RightHUDMatrix();
		}
		break;

	case 11:
		//Matrix convergence offset
		if( left ){
			out = in * view->LeftView();
		}else{
			out = in * view->RightView();
		}
		break;
		
	case 12:
		//Matrix simple translate ignore ortho
		if( fabs( in[15] - 1 ) < 0.00001f ){
			out = *(D3DXMATRIX*)src;
			return;
		}
		matrixTransform = true;
		break;

	case 13:
		//Matrix roll only
		out = in * view->ProjectionInverse() * view->RollMatrix() * view->Projection();
		break;

	case 14:
		//Matrix roll only negative
		out = in * view->ProjectionInverse() * view->RollMatrixNegative() * view->Projection();
		break;

	case 15:
		//Matrix roll only half
		out = in * view->ProjectionInverse() * view->RollMatrixHalf() * view->Projection();
		break;

	case 16:
		//Matrix no roll
		if( left ){
			out = in * view->LeftAdjustmentMatrixNoRoll() * view->ProjectionInverse() * view->PositionMatrix() * view->Projection();
		}else{
			out = in * view->RightAdjustmentMatrixNoRoll() * view->ProjectionInverse() * view->PositionMatrix() * view->Projection();
		}
		break;

	}



	if( operation > 2 && matrixTransform ){
		if( left ){
			out = in * view->LeftAdjustmentMatrix()  * view->ProjectionInverse() * view->PositionMatrix() * view->Projection();
		}else{
			out = in * view->RightAdjustmentMatrix() * view->ProjectionInverse() * view->PositionMatrix() * view->Projection();
		}

	}


	if( operation > 2 && transpose ){
		D3DXMatrixTranspose( &out , &out );
	}


	if( operation == 8 ){
		if( left ){
			view->matGatheredLeft = out;
		}else{
			view->matGatheredRight = out;
		}
	}
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