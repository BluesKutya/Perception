#pragma once
#include <string>

class cTracker_Udp : public MotionTracker {
public:
	cTracker_Udp();
	~cTracker_Udp();


	int  getOrientation   ( float* yaw , float* pitch , float* roll );
	bool isAvailable      ( );
	void updateOrientation( );
	void init             ( );

private:
	SOCKET s;
};