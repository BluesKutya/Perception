#include <Vireio.h>
#include <cTracker.h>
#include <Windows.h>
#include <string>
#include <iostream>


namespace{
	struct TrackData{
		int DataID;	/**< increased every time data has been sent */

		float Yaw;
		float Pitch;
		float Roll;

		float X;
		float Y;
		float Z;
	};
}



class cTracker_smt : public cTracker {
public:

	bool       openSharedMemory();
	HANDLE     h;
	TrackData* buf;


	cTracker_smt(){
		h   = 0;
		buf = 0;
	}


	~cTracker_smt(){
		close();
	}


	bool open( ) override{
		h = CreateFileMappingA( INVALID_HANDLE_VALUE , NULL , PAGE_READWRITE , 0 , sizeof(TrackData) , "VireioSMTrack" );
		if( !h ){
			return false;
		}
			
		buf = (TrackData*)MapViewOfFile( h , FILE_MAP_ALL_ACCESS , 0 , 0 , sizeof(TrackData) );

		if( !buf ){
			close();
			return false;
		}

		return true;
	}


	void close( ) override{
		if( buf ){
			UnmapViewOfFile( buf );
			buf = 0;
		}

		if( h ){
			CloseHandle( h );
			h = 0;
		}
	}


	bool update( ) override{
		if( !buf ){
			return false;
		}
		currentYaw   = -(buf->Yaw   - offsetYaw);
		currentPitch = -(buf->Pitch - offsetPitch);
		currentRoll  =  (buf->Roll  - offsetRoll);
		return true;
	}


	bool reset( ) override{
		offsetYaw   = buf->Yaw;
		offsetPitch = buf->Pitch;
		offsetRoll  = buf->Roll;
		return true;
	}
};


cTracker* Vireio_Create_Tracker_SharedMemory(){
	return new cTracker_smt;
}