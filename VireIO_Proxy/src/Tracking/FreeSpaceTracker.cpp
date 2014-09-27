#include <freespace\freespace.h>
#include <VireIO.h>
#include <cTracker.h>


class cTracker_freespace : public cTracker{
public:
	FreespaceDeviceId   DeviceID;
	FreespaceDeviceInfo Device;

	bool open( ){
		DeviceID = -1;

		int err;
		int num_devices;

		err = freespace_init();
		if( err ){
			printf( "FreeSpace: error %d\n" , err );
			close();
			return false;
		}
		
		err = freespace_getDeviceList( &DeviceID , 1 , &num_devices );
		if( err ){
			printf( "FreeSpace: error %d\n" , err );
			close();
			return false;
		}

		if( num_devices == 0 ){
			printf( "FreeSpace: no devices\n" );
			close();
			return false;
		}

		err = freespace_openDevice(DeviceID);
		if( err ){
			printf( "FreeSpace: error %d\n" , err );
			close();
			return false;
		}

		err = freespace_getDeviceInfo(DeviceID, &Device);

		err = freespace_flush(DeviceID);

		// Turn on the orientation message stream
		freespace_message msg;
		memset( &msg , 0 , sizeof(msg) );

		if( Device.hVer >= 2 ){
			// version 2 protocol
			msg.messageType = FREESPACE_MESSAGE_DATAMODECONTROLV2REQUEST;
			msg.dataModeControlV2Request.packetSelect = 3;    // User Frame (orientation)
			msg.dataModeControlV2Request.modeAndStatus = 0;   // operating mode == full motion
		}else {
			// version 1 protocol
			msg.messageType = FREESPACE_MESSAGE_DATAMODEREQUEST;
			msg.dataModeRequest.enableUserPosition = 1;
			msg.dataModeRequest.inhibitPowerManager = 1;
		}

		err = freespace_sendMessage(DeviceID, &msg);
		if( err ){
			printf( "FreeSpace: error %d\n" , err );
			close();
			return false;
		}
		
		return true;
	}


	void close( ) override{
		if( DeviceID >= 0 ){
			// Shut off the data stream
			freespace_message msg;
			memset(&msg, 0, sizeof(msg));

			if( Device.hVer >= 2 ) {
				msg.messageType = FREESPACE_MESSAGE_DATAMODECONTROLV2REQUEST;
				msg.dataModeControlV2Request.packetSelect = 0;        // No output 
				msg.dataModeControlV2Request.modeAndStatus = 1 << 1;  // operating mode == sleep 
			}else {
				msg.messageType = FREESPACE_MESSAGE_DATAMODEREQUEST;
				msg.dataModeRequest.enableUserPosition = 0;
				msg.dataModeRequest.inhibitPowerManager = 0;
			}

			freespace_sendMessage(DeviceID, &msg);
			freespace_closeDevice(DeviceID);
			freespace_exit();

			DeviceID = -1;
		}
	}


	bool reset( ) override{
		close();
		return open();
	}


	bool update( ){
		freespace_message msg;

		int err = freespace_readMessage(DeviceID, &msg, 10);

		if (err == 0) 
		{
			// Check if this is a user frame message.
			if (msg.messageType == FREESPACE_MESSAGE_USERFRAME) 
			{
				// Convert from quaternion to Euler angles

				// Get the quaternion vector
				float mw = msg.userFrame.angularPosA;
				float mx = msg.userFrame.angularPosB;
				float my = msg.userFrame.angularPosC;
				float mz = msg.userFrame.angularPosD;

				// normalize the vector
				float len = sqrtf((mw*mw) + (mx*mx) + (my*my) + (mz*mz));
				mw /= len;
				mx /= len;
				my /= len;
				mz /= len;

				// The Freespace quaternion gives the rotation in terms of
				// rotating the world around the object. We take the conjugate to
				// get the rotation in the object's reference frame.
				mw =  mw;
				mx = -mx;
				my = -my;
				mz = -mz;

				// Convert to angles in radians
				float m11 = (2.0f * mw * mw) + (2.0f * mx * mx) - 1.0f;
				float m12 = (2.0f * mx * my) + (2.0f * mw * mz);
				float m13 = (2.0f * mx * mz) - (2.0f * mw * my);
				float m23 = (2.0f * my * mz) + (2.0f * mw * mx);
				float m33 = (2.0f * mw * mw) + (2.0f * mz * mz) - 1.0f;

				currentRoll  = atan2f(m23, m33);
				currentPitch = asinf(-m13);
				currentYaw   = atan2f(m12, m11);
				return true;   
			}

			// any other message types will just fall through and keep looping until the timeout is reached
		}

		return false;
	}
};


cTracker* VireIO_Create_Tracker_FreeSpace(){
	return new cTracker_freespace;
}

