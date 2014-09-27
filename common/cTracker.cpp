#include "cTracker.h"

cTracker::cTracker(){
	currentYaw	= 0;
	currentPitch= 0;
	currentRoll	= 0;
	currentX	= 0;
	currentY	= 0;
	currentZ	= 0;
	offsetYaw	= 0;
	offsetPitch	= 0;
	offsetRoll	= 0;
	offsetX		= 0;
	offsetY		= 0;
	offsetZ		= 0;
}

cTracker::~cTracker(){
}


bool cTracker::reset ( ){
	return true;
}

void cTracker::beginFrame( ){
}


void cTracker::endFrame( ){
}