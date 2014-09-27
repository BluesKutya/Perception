#pragma  once
#include <cTracker.h>
#include <VireIO.h>
#include <OVR.h>

using namespace OVR;

class cTracker_ovr : public cTracker {
public:
    ovrHmd              hmd;
	char                trackerDescription[256];
	QString             name;
	ovrFrameTiming      FrameRef;


	cTracker_ovr( ){
		hmd = 0;
	}


	~cTracker_ovr( ){
		close();
	}


	void close() override{
		if( hmd ){
			ovrHmd_Destroy(hmd);
			hmd = 0;
		}
		ovr_Shutdown();
	}


	bool open( ) override{
		/*ovrBool res = ovr_Initialize(); // start LibOVR

		if( !ovr_Initialize() ){
			printf( "OculusTracker Initialize call failed\n" );
			close();
			return false;
		}
	 
		if( ovrHmd_Detect() == 0 ){
			printf( "No HMD detected, use a dummy DK1\n" );
			hmd  = ovrHmd_CreateDebug(ovrHmd_DK1);
			name = "No HMD Detected";
			return true;
		}


		hmd  = ovrHmd_Create(0);
		name = QString(hmd->ProductName) + "   Serial: " + hmd->SerialNumber;

		if( !ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position , 0) ){
			printf( "oculus tracker failed to initialise tracking\n" );
			close() ;
			return false;
		}
		*/
		reset();

		return true;
	}


	bool reset( ) override{
		/*if( !hmd ){
			return false;
		}*/

		offsetYaw   = 0.0f;
		offsetPitch = 0.0f;
		offsetRoll  = 0.0f;
		offsetX     = 0.0f;
		offsetY     = 0.0f;
		offsetZ     = 0.0f;
		/*
		//Force OVR positional reset
		ovrHmd_RecenterPose(hmd);

		ovrTrackingState ts = ovrHmd_GetTrackingState(hmd,FrameRef.ScanoutMidpointSeconds);
	
		if (ts.StatusFlags & ovrStatus_OrientationTracked)
		{
			Quatf hmdOrient=ts.HeadPose.ThePose.Orientation;
			hmdOrient.GetEulerAngles<Axis_Y,Axis_X,Axis_Z>(&offsetYaw, &offsetPitch, &offsetRoll);
		}
	
		if (ts.StatusFlags & ovrStatus_PositionConnected)
		{
			if (ts.StatusFlags & ovrStatus_PositionTracked)
			{
				offsetX = ts.HeadPose.ThePose.Position.x;
				offsetY = ts.HeadPose.ThePose.Position.y;
				offsetZ = ts.HeadPose.ThePose.Position.z;
			}
		}
		*/
		return true;
	}


	bool update( ){

		static float data[] = {
		0.081748,0.103166,0.146354,0.081748,0.103166,0.146354,0.081757,0.103164,0.146335,0.081808,0.103261,0.146323,0.081808,0.103261,0.146323,0.081808,0.103261,0.146323,0.081805,0.103276,0.146309,0.081805,0.103276,0.146309,0.081805,0.103276,0.146309,0.081805,0.103276,0.146309,0.081845,0.103339,0.146322,0.081845,0.103339,0.146322,0.081845,0.103339,0.146322,0.081845,0.103339,0.146322,0.081845,0.103339,0.146322,0.081845,0.103339,0.146322,0.081817,0.103320,0.146320,0.081817,0.103320,0.146320,0.081817,0.103320,0.146320,0.081817,0.103320,0.146320,0.081817,0.103320,0.146320,0.081817,0.103320,0.146320,0.081735,0.103263,0.145988,0.081734,0.103263,0.145982,0.081734,0.103291,0.145991,0.081729,0.103391,0.146074,0.081720,0.103425,0.146100,0.081720,0.103425,0.146100,0.081720,0.103425,0.146100,0.081731,0.103455,0.146101,0.081731,0.103455,0.146101,0.081731,0.103455,0.146101,0.081731,0.103455,0.146101,0.081731,0.103455,0.146101,0.081725,0.103469,0.146086,0.081725,0.103469,0.146086,0.081725,0.103469,0.146086,0.081725,0.103469,0.146086,0.081725,0.103469,0.146086,0.081725,0.103469,0.146086,0.081725,0.103469,0.146086,0.081708,0.103522,0.146079,0.081708,0.103522,0.146079,0.081708,0.103522,0.146079,0.081708,0.103522,0.146079,0.081708,0.103522,0.146079,0.081708,0.103522,0.146079,0.081708,0.103522,0.146079,0.081710,0.103525,0.146085,0.081708,0.103549,0.146089,0.081698,0.103566,0.146082,0.081670,0.103429,0.146103,0.081670,0.103429,0.146103,0.081670,0.103429,0.146103,0.081687,0.103497,0.146102,0.081687,0.103497,0.146102,0.081687,0.103497,0.146102,0.081683,0.103490,0.146095,0.081683,0.103490,0.146095,0.081683,0.103490,0.146095,0.081683,0.103490,0.146095,0.081683,0.103435,0.146111
		};

		static int c=0;

		c+=3;

		if( c+6 >= sizeof(data)/4 ){
			c=0;
		}
		printf("%d\n",c);
		currentYaw   = data[c+0];
		currentPitch = data[c+1];
		currentRoll  = data[c+2];


		/*if( !hmd ){
			return false;
		}

		ovrTrackingState ts = ovrHmd_GetTrackingState( hmd , FrameRef.ScanoutMidpointSeconds );

		if( ts.StatusFlags & ovrStatus_OrientationTracked ){
			Quatf hmdOrient=ts.HeadPose.ThePose.Orientation;
			hmdOrient.GetEulerAngles<Axis_Y,Axis_X,Axis_Z>(&currentYaw, &currentPitch, &currentRoll);
			currentYaw   = -(currentYaw   - offsetYaw  );
			currentPitch = -(currentPitch - offsetPitch);
			currentRoll  = -(currentRoll  - offsetRoll );
			printf("oculus track: %f %f %f  %f %f %f\n" , currentYaw , currentPitch , currentRoll , offsetYaw , offsetPitch , offsetRoll );
		}

		if( ts.StatusFlags & ovrStatus_PositionConnected ){
			if( !(ts.StatusFlags & ovrStatus_CameraPoseTracked) ){
				//Camera still initialising/calibrating
				//Should probably warn user if this doesn't get set after a period of time
				static DWORD tick = GetTickCount();
				if (((tick - GetTickCount()) / 1000) > 15){
					return false;
				}
			}else
			if( ts.StatusFlags & ovrStatus_PositionTracked ){
				currentX = ts.HeadPose.ThePose.Position.x - offsetX;
				currentY = ts.HeadPose.ThePose.Position.y - offsetY;
				currentZ = ts.HeadPose.ThePose.Position.z - offsetZ;
			}
		}*/

		return true;
	}



	void beginFrame(){
		if( hmd ){
			FrameRef = ovrHmd_BeginFrameTiming(hmd,0);
		}
	}



	void endFrame(){
		if( hmd ){
			ovrHmd_EndFrameTiming(hmd);
		}
	}
};


cTracker* VireIO_Create_Tracker_Oculus(){
	return new cTracker_ovr;
}

