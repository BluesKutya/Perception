#pragma  once
#include <cTracker.h>
#include <Vireio.h>
#include <OVR.h>

using namespace OVR;

class cTracker_ovr : public cTracker {
public:
    ovrHmd              hmd;
	char                trackerDescription[256];
	QString             name;
	ovrFrameTiming      FrameRef;
	UINT                frameId;


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
		frameId = 0;

		ovrBool res = ovr_Initialize(); // start LibOVR

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

		reset();

		return true;
	}


	bool reset( ) override{
		if( !hmd ){
			return false;
		}

		offsetYaw   = 0.0f;
		offsetPitch = 0.0f;
		offsetRoll  = 0.0f;
		offsetX     = 0.0f;
		offsetY     = 0.0f;
		offsetZ     = 0.0f;

		//Force OVR positional reset
		ovrHmd_RecenterPose(hmd);

		ovrTrackingState ts = ovrHmd_GetTrackingState( hmd , ovr_GetTimeInSeconds() );
	
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

		return true;
	}


	bool update( ){
		if( !hmd ){
			return false;
		}

		ovrTrackingState ts = ovrHmd_GetTrackingState( hmd , (config.trackerTimewarp ? FrameRef.ScanoutMidpointSeconds : ovr_GetTimeInSeconds()) );

		if( ts.StatusFlags & ovrStatus_OrientationTracked ){
			Quatf hmdOrient=ts.HeadPose.ThePose.Orientation;
			hmdOrient.GetEulerAngles<Axis_Y,Axis_X,Axis_Z>(&currentYaw, &currentPitch, &currentRoll);
			
			currentYaw   =  currentYaw - offsetYaw;
			currentPitch =  currentPitch;
			currentRoll  = -currentRoll ;
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
		}

		return true;
	}



	void beginFrame(){
		if( hmd && config.trackerTimewarp ){
			FrameRef = ovrHmd_BeginFrameTiming( hmd , frameId++ );
		}
	}



	void endFrame(){
		if( hmd && config.trackerTimewarp ){
			ovrHmd_EndFrameTiming(hmd);
		}
	}
};


cTracker* Vireio_Create_Tracker_Oculus(){
	return new cTracker_ovr;
}

