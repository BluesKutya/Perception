#pragma once
#include <qstring.h>

class cTracker{
public:
	//angles in radians

	QString name;
	float   currentYaw;
	float   currentPitch;
	float   currentRoll;
	float   currentX;
	float   currentY;
	float   currentZ;
	float   offsetYaw;
	float   offsetPitch;
	float   offsetRoll;
	float   offsetX;
	float   offsetY;
	float   offsetZ;
	

	cTracker();
	virtual ~cTracker();

	virtual bool open  ( )=0;
	virtual void close ( )=0;
	virtual bool update( )=0;
	virtual bool reset ( );

	virtual void beginFrame( );
	virtual void endFrame  ( );

};