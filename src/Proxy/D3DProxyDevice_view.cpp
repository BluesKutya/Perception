#include "D3DProxyDevice.h"
#include "stereoview.h"

void D3DProxyDevice::viewInit( ){

	viewProjMinZ =  0.1;
	viewProjMaxZ = 10.0;
	viewProjMinX = -0.5;
	viewProjMaxX =  0.5;

	D3DXMatrixIdentity( &viewMatPosition					 );
	D3DXMatrixIdentity( &viewMatProjection					 );
	D3DXMatrixIdentity( &viewMatProjectionInv				 );
	D3DXMatrixIdentity( &viewMatProjectLeft				  	 );
	D3DXMatrixIdentity( &viewMatProjectRight				 );
	D3DXMatrixIdentity( &viewMatRoll					     );
	D3DXMatrixIdentity( &viewMatRollNegative			     );
	D3DXMatrixIdentity( &viewMatRollHalf				     );
	D3DXMatrixIdentity( &viewMatTransformLeft				 );
	D3DXMatrixIdentity( &viewMatTransformRight				 );
	D3DXMatrixIdentity( &viewMatViewProjLeft				 );
	D3DXMatrixIdentity( &viewMatViewProjRight				 );
	D3DXMatrixIdentity( &viewMatViewProjTransformLeft		 );
	D3DXMatrixIdentity( &viewMatViewProjTransformRight		 );
	D3DXMatrixIdentity( &viewMatViewProjTransformLeftNoRoll  );
	D3DXMatrixIdentity( &viewMatViewProjTransformRightNoRoll );
	D3DXMatrixIdentity( &viewMatGatheredLeft				 );
	D3DXMatrixIdentity( &viewMatGatheredRight				 );
	D3DXMatrixIdentity( &viewMatHudLeft					  	 );
	D3DXMatrixIdentity( &viewMatHudRight					 );
	D3DXMatrixIdentity( &viewMatGuiLeft					  	 );
	D3DXMatrixIdentity( &viewMatGuiRight					 );
	D3DXMatrixIdentity( &viewMatSquash						 );
	D3DXMatrixIdentity( &viewMatHudDistance				  	 );
	D3DXMatrixIdentity( &viewMatLeftHud3DDepth				 );
	D3DXMatrixIdentity( &viewMatRightHud3DDepth			  	 );
	D3DXMatrixIdentity( &viewMatLeftHud3DDepthShifted		 );
	D3DXMatrixIdentity( &viewMatRightHud3DDepthShifted		 );
	D3DXMatrixIdentity( &viewMatLeftGui3DDepth				 );
	D3DXMatrixIdentity( &viewMatRightGui3DDepth			  	 );
	D3DXMatrixIdentity( &viewMatBulletLabyrinth			  	 );
	
	viewUpdateProjectionMatrices( );

	viewComputeTransforms( );
}


void D3DProxyDevice::viewUpdateProjectionMatrices( ){
	float aspect = (float)stereoView->viewport.Width/(float)stereoView->viewport.Height;

	viewProjMinY =  0.5f / aspect;
	viewProjMaxY = -0.5f / aspect;

	D3DXMatrixPerspectiveOffCenterLH( &viewMatProjection    , viewProjMinX , viewProjMaxX , viewProjMaxY , viewProjMinY , viewProjMinZ , viewProjMaxZ );
	D3DXMatrixInverse               ( &viewMatProjectionInv , 0, &viewMatProjection );

	// ALL stated in meters here ! screen size = horizontal size
	float nearClippingPlaneDistance  = config.eyeToScreenDistance;
	float physicalScreenSizeInMeters = config.physicalWidth / 2;

	// if not HMD, set values to fullscreen defaults
	if ( !config.isHmd )   //stereo type > 100 reserved specifically for HMDs
	{
		// assumption here :
		// end user is placed 1 meter away from screen
		// end user screen is 1 meter in horizontal size
		nearClippingPlaneDistance  = 0.1; //for more flexibility
		physicalScreenSizeInMeters = 1;
	}

	// convergence frustum adjustment, based on NVidia explanations
	//
	// It is evident that the ratio of frustum shift to the near clipping plane is equal to the ratio of 
	// IOD/2 to the distance from the screenplane. (IOD=IPD) 
	// frustumAsymmetryInMeters = ((IPD/2) * nearClippingPlaneDistance) / convergence
	// <http://www.orthostereo.com/geometryopengl.html>
	//
	// (near clipping plane distance = physical screen distance)
	// (convergence = virtual screen distance)
	if ( config.stereoConvergence <= nearClippingPlaneDistance) config.stereoConvergence  = nearClippingPlaneDistance + 0.001f;
	float frustumAsymmetryInMeters = ((config.PlayerIPD/2) * nearClippingPlaneDistance) / config.stereoConvergence ;

	// divide the frustum asymmetry by the assumed physical size of the physical screen
	float frustumAsymmetryLeftInMeters  = - frustumAsymmetryInMeters / physicalScreenSizeInMeters;
	float frustumAsymmetryRightInMeters =   frustumAsymmetryInMeters / physicalScreenSizeInMeters;

	// get the horizontal screen space size and compute screen space adjustment
	float screenSpaceXSize = abs(viewProjMinX)+abs(viewProjMaxX);
	float multiplier = screenSpaceXSize/1; // = 1 meter
	float frustumAsymmetryLeft = frustumAsymmetryLeftInMeters * multiplier;
	float frustumAsymmetryRight = frustumAsymmetryRightInMeters * multiplier;

	// now, create the re-projection matrices for both eyes using this frustum asymmetry
	D3DXMatrixPerspectiveOffCenterLH( &viewMatProjectLeft  , viewProjMinX+frustumAsymmetryLeft, viewProjMaxX+frustumAsymmetryLeft, viewProjMaxY, viewProjMinY, viewProjMinZ, viewProjMaxZ);
	D3DXMatrixPerspectiveOffCenterLH( &viewMatProjectRight , viewProjMinX+frustumAsymmetryRight, viewProjMaxX+frustumAsymmetryRight, viewProjMaxY, viewProjMinY, viewProjMinZ, viewProjMaxZ);
}




void D3DProxyDevice::viewUpdateRotation( float pitch , float yaw , float roll ){
	D3DXMatrixTranslation( &viewMatBulletLabyrinth , -yaw , pitch , 0 );

	D3DXMatrixIdentity ( &viewMatRoll );
	D3DXMatrixRotationZ( &viewMatRoll , roll );

	D3DXMatrixIdentity ( &viewMatRollNegative );
	D3DXMatrixRotationZ( &viewMatRollNegative , -roll );

	D3DXMatrixIdentity ( &viewMatRollHalf );
	D3DXMatrixRotationZ( &viewMatRollHalf , roll * 0.5 );
}



void D3DProxyDevice::viewUpdatePosition( float pitch , float yaw , float roll , float x , float y , float z ){
	D3DXMATRIX rotationMatrixPitch;
	D3DXMATRIX rotationMatrixYaw;
	D3DXMATRIX rotationMatrixRoll;

	D3DXMatrixRotationX( &rotationMatrixPitch , pitch );
	D3DXMatrixRotationY( &rotationMatrixYaw   , yaw   );
	D3DXMatrixRotationZ( &rotationMatrixRoll  , -roll );

	//Need to invert X and Y
	D3DXVECTOR3 vec( x , y , z );

	D3DXMATRIX worldScale;
	D3DXMatrixScaling(&worldScale, -1.0f * config.stereoScale , -1.0f * config.stereoScale , config.stereoScale );
	D3DXVec3TransformNormal(&viewVecPositionTransform, &vec, &worldScale);

	D3DXMATRIX rotationMatrixPitchYaw;
	D3DXMatrixIdentity(&rotationMatrixPitchYaw);
	D3DXMatrixMultiply(&rotationMatrixPitchYaw, &rotationMatrixPitch, &rotationMatrixYaw);

	D3DXVec3TransformNormal(&viewVecPositionTransform, &viewVecPositionTransform, &rotationMatrixPitchYaw);

	//Still need to apply the roll, as the "no roll" param is just whether we use matrix roll translation or if
	//memory modification, either way, the view still rolls
	D3DXVec3TransformNormal(&viewVecPositionTransform, &viewVecPositionTransform, &rotationMatrixRoll);

	//Now apply game specific scaling for the X/Y/Z
	D3DXVECTOR3 gameScaleVec(3.0f, 2.0f, 1.0f);
	D3DXMATRIX gamescalingmatrix;
	D3DXMatrixScaling(&gamescalingmatrix, gameScaleVec.x, gameScaleVec.y, gameScaleVec.z);
	D3DXVec3TransformNormal(&viewVecPositionTransform, &viewVecPositionTransform, &gamescalingmatrix);
	
	D3DXMatrixTranslation(&viewMatPosition, viewVecPositionTransform.x, viewVecPositionTransform.y, viewVecPositionTransform.z);
}




/**
* Gets the view-projection transform matrices left and right.
* Unprojects, shifts view position left/right (using same matricies as (Left/Right)ViewRollAndShift)
* and reprojects using left/right projection.
* (matrix = projectionInverse * transform * projection)
***/
void D3DProxyDevice::viewComputeTransforms( ){

	// separation settings are overall (HMD and desktop), since they are based on physical IPD
	D3DXMatrixTranslation( &viewMatTransformLeft  , -separationInWorldUnits() , 0 , 0 );
	D3DXMatrixTranslation( &viewMatTransformRight , +separationInWorldUnits() , 0 , 0 );
	
	// projection transform, no roll
	viewMatViewProjTransformLeftNoRoll  = viewMatProjectionInv * viewMatTransformLeft  * viewMatProjectLeft;
	viewMatViewProjTransformRightNoRoll = viewMatProjectionInv * viewMatTransformRight * viewMatProjectRight;
	
	// head roll
	if( config.rollEnabled ){
		D3DXMatrixMultiply(&viewMatTransformLeft, &viewMatRoll, &viewMatTransformLeft);
		D3DXMatrixMultiply(&viewMatTransformRight, &viewMatRoll, &viewMatTransformRight);

		// projection 
		viewMatViewProjLeft  = viewMatProjectionInv * viewMatRoll * viewMatProjectLeft;
		viewMatViewProjRight = viewMatProjectionInv * viewMatRoll * viewMatProjectRight;
	}else{
		// projection 
		viewMatViewProjLeft  = viewMatProjectionInv * viewMatProjectLeft;
		viewMatViewProjRight = viewMatProjectionInv * viewMatProjectRight;
	}

	// projection transform
	viewMatViewProjTransformLeft  = viewMatProjectionInv * viewMatTransformLeft  * viewMatProjectLeft;
	viewMatViewProjTransformRight = viewMatProjectionInv * viewMatTransformRight * viewMatProjectRight;

	viewComputeGui( );

	// gui/hud matrices
	viewMatHudLeft  = viewMatProjectionInv * viewMatLeftHud3DDepth  * viewMatTransformLeft  * viewMatHudDistance * viewMatProjectLeft;
	viewMatHudRight = viewMatProjectionInv * viewMatRightHud3DDepth * viewMatTransformRight * viewMatHudDistance * viewMatProjectRight;
	viewMatGuiLeft  = viewMatProjectionInv * viewMatLeftGui3DDepth  * viewMatSquash                              * viewMatProjectLeft;
	viewMatGuiRight = viewMatProjectionInv * viewMatRightGui3DDepth * viewMatSquash                              * viewMatProjectRight;
}




void D3DProxyDevice::viewComputeGui(){
	float additionalSeparation = (1.5f - config.hudDistance) * config.lensXCenterOffset;


	D3DXMatrixScaling    ( &viewMatSquash                , config.guiSquash , config.guiSquash , 1 );
											         
	D3DXMatrixTranslation( &viewMatLeftGui3DDepth        ,   config.guiDepth + separationIPDAdjustment()    , 0 , 0 );
	D3DXMatrixTranslation( &viewMatRightGui3DDepth       , -(config.guiDepth + separationIPDAdjustment()) , 0 , 0 );
											         
	D3DXMatrixTranslation( &viewMatHudDistance           , 0 , 0 , config.hudDistance );

	D3DXMatrixTranslation( &viewMatLeftHud3DDepth         , -config.hudDepth  , 0 , 0 );
	D3DXMatrixTranslation( &viewMatRightHud3DDepth        ,  config.hudDepth  , 0 , 0 );

	D3DXMatrixTranslation( &viewMatLeftHud3DDepthShifted  ,  config.hudDepth + additionalSeparation , 0 , 0 );
	D3DXMatrixTranslation( &viewMatRightHud3DDepthShifted , -config.hudDepth - additionalSeparation , 0 , 0 );
}





float D3DProxyDevice::convergenceInWorldUnits( ) { 
	return config.stereoConvergence * config.stereoScale; 
}


float D3DProxyDevice::separationInWorldUnits( ){ 
	return  (config.PlayerIPD / 2.0f) * config.stereoScale; 
}


float D3DProxyDevice::separationIPDAdjustment( ){ 
	return  ((config.PlayerIPD-IPD_DEFAULT) / 2.0f) * config.stereoScale; 
}
