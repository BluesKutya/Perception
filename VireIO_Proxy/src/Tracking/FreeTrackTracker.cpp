#include <VireIO.h>
#include <cTracker.h>


class cTracker_freetrack : public cTracker {
public:

	typedef struct{
		unsigned long int dataID;
		long int camWidth;
		long int camHeight;
		float yaw;
		float pitch;
		float roll;
		float x;
		float y;
		float z;
		float rawyaw;
		float rawpitch;
		float rawroll;
		float rawx;
		float rawy;
		float rawz;
		float x1;
		float y1;
		float x2;
		float y2;
		float x3;
		float y3;
		float x4;
		float y4;
	} FreeTrackData;

	HINSTANCE      dll;
	FARPROC        getDataFunc;
	

	cTracker_freetrack( ){
	}

	~cTracker_freetrack( ){
		close();
	}


	bool open( ) override{
		dll = LoadLibraryA("FreeTrackClient.dll");
		if( !dll ){
			printf( "FreeTrack load failed\n" );
			close();
			return false;
		}

		getDataFunc = GetProcAddress( dll , "FTGetData" );
		if( !getDataFunc ){
			printf( "FreeTrack load failed\n" );
			close();
			return false;
		}

		return true;
	}


	void close( ) override{
		if( dll ){
			FreeLibrary( dll );
			dll = 0;
		}
	}


	bool update( ){
		FreeTrackData  data;

		if( ((bool(WINAPI*)(FreeTrackData*))getDataFunc)( &data ) ){
			currentYaw   = -data.yaw;
			currentPitch = -data.pitch;
			currentRoll  = -data.roll;
			return true;
		}else{
			return false;
		}
	}
};


cTracker* VireIO_Create_Tracker_FreeTrack(){
	return new cTracker_freetrack;
}