#include "cRegisterModification.h"
#include "D3DProxyDevice.h"


static D3DXMATRIX translateUV;
static D3DXMATRIX scaleUV;
static bool       initialized;

enum{
	OP_VECTOR_SIMPLE    = 1<<0,
	OP_VECTOR_SIMPLE_5K = 1<<1,
	OP_SAVE				= 1<<2,
	OP_LOAD				= 1<<3,
	OP_TRANSPOSE		= 1<<4,
	OP_AUTO				= 1<<5,
	OP_VIEWPROJ			= 1<<6,
	OP_VIEWPROJ_NO_ROLL	= 1<<7,
	OP_PROJECTION		= 1<<8,
	OP_POSITION			= 1<<9,
	OP_ROLL				= 1<<10,
	OP_ROLL_NEG			= 1<<11,
	OP_ROLL_HALF		= 1<<12,
	OP_BULLET       	= 1<<13,
	OP_HUD				= 1<<14,
	OP_GUI				= 1<<15,
	OP_HUD_SHIFTED		= 1<<16,
	OP_GUI_SHIFTED		= 1<<17,
	OP_REFLECTION		= 1<<18,
	OP_CONV_SHIFT		= 1<<19,
};



cRegisterModification::cRegisterModification( ){
	if( !initialized ){
		initialized = true;

		D3DXMatrixTranslation( &translateUV , 0.5f , 0.5f , 0.5f );
		D3DXMatrixScaling    ( &scaleUV     , 0.5f , 0.5f , 0.0f );
	}

	start     = 0;
	count     = 0;
	isMatrix  = false;
	operation = 0;
}



QStringList cRegisterModification::availableOperations(){
	QStringList r;
	r += "Vector simple translate";
	r += "Vector simple translate * 5000";
	r += "Matrix save";
	r += "Matrix load previous";
	r += "Matrix transpose";
	r += "Matrix auto proj/gui/hud";
	r += "Matrix view projection";
	r += "Matrix view projection without roll";
	r += "Matrix projection/inv";
	r += "Matrix position";
	r += "Matrix roll only";
	r += "Matrix roll only negative";
	r += "Matrix roll only half";
	r += "Matrix bullet labyrinth";
	r += "Matrix hud";
	r += "Matrix gui";
	r += "Matrix hud shifted";
	r += "Matrix gui shifted";
	r += "Matrix reflection";
	r += "Matrix convergence shift";
	return r;
}



void cRegisterModification::modify( D3DProxyDevice* device , float* ptrData , float* ptrLeft , float* ptrRight ){

	if( !operation ){
		memcpy( ptrLeft  , ptrData , count * 4 * sizeof(float) );
		memcpy( ptrRight , ptrData , count * 4 * sizeof(float) );
		return;
	}



	if( !isMatrix ){
		if( operation & OP_VECTOR_SIMPLE ){
			ptrLeft[0] = ptrData[0] - device->separationInWorldUnits();
			ptrLeft[1] = ptrData[1];
			ptrLeft[2] = ptrData[2];
			ptrLeft[3] = ptrData[3];

			ptrRight[0] = ptrData[0] + device->separationInWorldUnits();
			ptrRight[1] = ptrData[1];
			ptrRight[2] = ptrData[2];
			ptrRight[3] = ptrData[3];
			return;
		}

		if( operation & OP_VECTOR_SIMPLE_5K ){
			ptrLeft[0]  = ptrData[0] - device->separationInWorldUnits() * 5000;
			ptrLeft[1]  = ptrData[1];
			ptrLeft[2]  = ptrData[2];
			ptrLeft[3]  = ptrData[3];

			ptrRight[0] = ptrData[0] + device->separationInWorldUnits() * 5000;
			ptrRight[1] = ptrData[1];
			ptrRight[2] = ptrData[2];
			ptrRight[3] = ptrData[3];
			return;
		}

		return;
	}




	D3DXMATRIX& outLeft  = *(D3DXMATRIX*)( ptrLeft );
	D3DXMATRIX& outRight = *(D3DXMATRIX*)( ptrRight );

	
	if( operation & OP_LOAD ){
		outLeft  = device->viewMatGatheredLeft;
		outRight = device->viewMatGatheredRight;
	}else{
		outLeft  = *(D3DXMATRIX*)( ptrData );
		outRight = *(D3DXMATRIX*)( ptrData );
	}


	if( operation & OP_TRANSPOSE ){
		D3DXMatrixTranspose( &outLeft  , &outLeft  );
		D3DXMatrixTranspose( &outRight , &outRight );
	}


	int selected = operation;

	if( operation & OP_AUTO ){
		if( fabs(ptrLeft[15]-1) > 0.00001 ){
			//Disable GUI / HUD transforms
			selected &= ~OP_HUD;
			selected &= ~OP_GUI;
			selected &= ~OP_BULLET;
			selected &= ~OP_HUD_SHIFTED;
			selected &= ~OP_GUI_SHIFTED;
		}else{
			float sum = 0;
			sum += abs(outLeft(3, 0)); // transX
			sum += abs(outLeft(3, 1)); // transY
			sum += abs(outLeft(3, 2)); // transZ

			sum += abs(outLeft(0, 0)); // scaleX
			sum += abs(outLeft(1, 1)); // scaleY
			sum += abs(outLeft(2, 2)); // scaleZ

			//Disable ortho projection
			selected &= ~OP_VIEWPROJ;
			selected &= ~OP_VIEWPROJ_NO_ROLL;
			selected &= ~OP_POSITION;
			selected &= ~OP_ROLL;
			selected &= ~OP_ROLL_NEG;
			selected &= ~OP_ROLL_HALF;

			if( sum > 3.0f ){
				selected &= ~OP_GUI;
				selected &= ~OP_GUI_SHIFTED;
				selected &= ~OP_BULLET;
			}else{
				selected &= ~OP_HUD;
				selected &= ~OP_HUD_SHIFTED;
			}
		}
	}



	if( selected & OP_VIEWPROJ ){
		outLeft  *= device->viewMatViewProjTransformLeft;
		outRight *= device->viewMatViewProjTransformRight;
	}


	if( selected & OP_VIEWPROJ_NO_ROLL ){
		outLeft  *= device->viewMatViewProjTransformLeftNoRoll;
		outRight *= device->viewMatViewProjTransformRightNoRoll;
	}


	if( selected & OP_PROJECTION ){
		outLeft  *= device->viewMatProjectionInv;
		outRight *= device->viewMatProjectionInv;
	}


	if( selected & OP_POSITION ){
		outLeft  *= device->viewMatPosition;
		outRight *= device->viewMatPosition;
	}


	if( selected & OP_ROLL ){
		outLeft  *= device->viewMatRoll;
		outRight *= device->viewMatRoll;
	}


	if( selected & OP_ROLL_NEG ){
		outLeft  *= device->viewMatRollNegative;
		outRight *= device->viewMatRollNegative;
	}


	if( selected & OP_ROLL_HALF ){
		outLeft  *= device->viewMatRollHalf;
		outRight *= device->viewMatRollHalf;
	}


	if ( selected & OP_BULLET ){
		D3DXMatrixTranspose( &outLeft  , &outLeft  );
		D3DXMatrixTranspose( &outRight , &outRight );

		outLeft  = device->viewMatBulletLabyrinth * outLeft;
		outRight = device->viewMatBulletLabyrinth * outRight;

		D3DXMatrixTranspose( &outLeft  , &outLeft  );
		D3DXMatrixTranspose( &outRight , &outRight );
	}


	if( selected & OP_HUD ){
		outLeft  *= device->viewMatHudLeft;
		outRight *= device->viewMatHudRight;
	}


	if( selected & OP_GUI ){
		outLeft  *= device->viewMatGuiLeft;
		outRight *= device->viewMatGuiRight;
	}


	if( selected & OP_HUD_SHIFTED ){
		outLeft  *= device->viewMatLeftHud3DDepthShifted  * device->viewMatTransformLeft  * device->viewMatHudDistance;
		outRight *= device->viewMatRightHud3DDepthShifted * device->viewMatTransformRight * device->viewMatHudDistance;
	}


	if( selected & OP_GUI_SHIFTED ){
		outLeft  *= device->viewMatLeftGui3DDepth  * device->viewMatSquash;
		outRight *= device->viewMatRightGui3DDepth * device->viewMatSquash;
	}


	if( selected & OP_REFLECTION ){
		outLeft  *= scaleUV * translateUV;
		outRight *= scaleUV * translateUV;
	}


	if( selected & OP_CONV_SHIFT ){
		outLeft  *= device->viewMatViewProjLeft;
		outRight *= device->viewMatViewProjRight;
	}


	if( selected & OP_PROJECTION ){
		outLeft  *= device->viewMatProjection;
		outRight *= device->viewMatProjection;
	}


	if( selected & OP_TRANSPOSE ){
		D3DXMatrixTranspose( &outLeft , &outLeft );
		D3DXMatrixTranspose( &outRight , &outRight );
	}


	if( selected & OP_SAVE ){
		device->viewMatGatheredLeft  = outLeft;
		device->viewMatGatheredRight = outRight;
	}
}