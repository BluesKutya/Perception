#include <qfileinfo.h>
#include <qdir.h>
#include "cConfig.h"
#include "cPropsFile.h"

cConfig config;
cConfig configBackup;


cConfig::cConfig(){

	PlayerIPD        = 0.0638;

	trackerYawMultiplier		= 1;
	trackerPitchMultiplier		= 1;
	trackerRollMultiplier		= 1;
	trackerXMultiplier	        = 0.001;
	trackerYMultiplier	        = 0.001;
	trackerZMultiplier	        = 0.001;
	trackerPositionEnable       = false;
	trackerRotationEnable       = true;
	trackerMouseYawMultiplier	= 1;
	trackerMousePitchMultiplier	= 1;
	trackerMouseEmulation		= false;

	guiBulletLabyrinth = false;
	guiSquash    	   = 1;
	guiDepth    	   = 0;
	hudDistance 	   = 0;
	hudDepth           = 0;

	showVRMouse = true;

}


bool cConfig::load( const QString& file ){
	cPropsFile props;
	if( !props.load( file ) ){
		return false;
	}

	#define P(t,n,s) props.get( n , #n );
	#define C(n)
	#include "cConfig.inc"
	#undef P
	#undef C

	//SB: This will need to be changed back when the memory modification stuff is updated, but for now
	//I am disabling the restore of the camera translation as it is causing confusion for a lot of people when
	//the game starts and they are detached from their avatar, or worse the scene doesn't render at all
	CameraTranslateX = 0.0f;
	CameraTranslateY = 0.0f;
	CameraTranslateZ = 0.0f;

	return true;
}



bool cConfig::save( const QString& file , QList<int> categories ){
	cPropsFile props;

	int cat;

	#define P(t,n,s) if( categories.contains(cat) ) props.set( n , #n );
	#define C(n)     cat = SAVE_##n;
	#include "cConfig.inc"
	#undef P
	#undef C

	return props.save( file );
}







void cConfig::calculateValues(){
	screenAspectRatio = resolutionWidth / (float)resolutionHeight;

	float physicalViewCenter = physicalWidth * 0.25f;
	float physicalOffset     = physicalViewCenter - physicalLensSeparation * 0.5f;

	// Range at this point would be -0.25 to 0.25 units. So multiply the last step by 4 to get the offset in a -1 to 1  range
	lensXCenterOffset = 4.0f * physicalOffset / physicalWidth;

	float radius       = -1 - lensXCenterOffset;
	float radiusSqared = radius * radius;
	float distort      = radius * (distortionCoefficients[0] + distortionCoefficients[1] * radiusSqared + distortionCoefficients[2] * radiusSqared * radiusSqared + distortionCoefficients[3] * radiusSqared * radiusSqared * radiusSqared);

	scaleToFillHorizontal = distort / radius;
}


QString cConfig::getMainConfigFile( ){
	return vireioDir + "config/main.ini";
}


QString cConfig::getGameConfigFile( const QString& gameExePath ){
	QString s = gameExePath;
	s.replace( ':'  , '_' );
	s.replace( '\\' , '_' );
	s.replace( '/'  , '_' );
	return vireioDir + "/games/" + s + ".ini";
}


QString cConfig::findProfileFileForExe( const QString& gameExePath ){
	QString exe = QFileInfo(gameExePath).fileName();

	for( QFileInfo& info : QDir(vireioDir+"./profiles").entryInfoList(QDir::Files) ){
		cConfig cfg;
		if( cfg.load( info.absoluteFilePath() ) ){
			if( cfg.exeName == exe ){
				return info.baseName();
			}
		}
	}

	return "";
}


QStringList cConfig::getAvailableProfiles( ){
	QStringList ret;
	for( QFileInfo& info : QDir(vireioDir+"/profiles").entryInfoList(QDir::Files) ){
		ret += info.baseName();
	}
	return ret;
}


QStringList cConfig::getAvailableDevices( ){
	QStringList ret;
	for( QFileInfo& info : QDir(vireioDir+"/modes").entryInfoList(QDir::Files) ){
		ret += info.baseName();
	}
	return ret;
}


QString cConfig::getProfileConfigFile( ){
	return vireioDir + "/profiles/" + profileName + ".ini";
}


QString cConfig::getShaderRuleFilePath( ){
	return vireioDir + "/shader_rules/" + shaderRule;
}


QString cConfig::getVRBoostRuleFilePath(){
	return vireioDir + "/VRboost_rules/" + VRboostRule;
}


bool cConfig::loadProfile( ){
	return load( vireioDir + "/profiles/" + profileName + ".ini" );
}


bool cConfig::loadDevice( ){
	if( !load( vireioDir + "/modes/" + stereoDevice + ".ini" ) ){
		return false;
	}

	if( useOvrDeviceSettings ){
		return loadOculusSdk();
	}

	return true;
}