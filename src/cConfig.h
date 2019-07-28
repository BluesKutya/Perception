#pragma once
#include <qstring.h>
#include <qmap.h>
#include <qvector.h>
#include <qstringlist.h>
#include "cRule.h"



enum{
	DUPLICATE_ALWAYS                    = 0 ,
	DUPLICATE_NEVER                     = 1 ,
	DUPLICATE_IF_SWAP_OR_NOT_SQUARE     = 2 , 
	DUPLICATE_IF_NOT_SQUARE             = 3 ,
	DUPLICATE_IF_DEPTH_OR_RT_NOT_SQUARE = 4 ,
	DUPLICATE_IF_DEPTH_OR_RT            = 5 ,
	DUPLICATE_IF_RT                     = 6 ,
};

enum{
	WHEN_PRESENT           = 0,
	WHEN_BEGIN_SCENE       = 1,
	WHEN_FIRST_BEGIN_SCENE = 2,
	WHEN_END_SCENE         = 3,
};

enum{
	SAVE_STATE_BLOCK             = 0 ,
	SAVE_STATE_SELECTED_MANUALLY = 1 ,
	SAVE_STATE_ALL_MANUALLY      = 2 ,
	SAVE_STATE_DONT_SAVE         = 3 ,
};


enum{
	MIRROR_DISABLE = 0 ,
	MIRROR_LEFT    = 1 ,
	MIRROR_RIGHT   = 2 ,
	MIRROR_STEREO  = 3 ,
};


QStringList Vireio_enum_duplicate();
QStringList Vireio_enum_when     ();
QStringList Vireio_enum_saveState();
bool        Vireio_shouldDuplicate( int mode , int width , int height , int usage , bool isSwapChainBackBuffer );


class cConfig {
public:

	#define P(t,n,s) t n s;
	#define C(n)
	#include "cConfig.inc"
	#undef P
	#undef C

	enum{
		#define P(t,n,s)
		#define C(n) SAVE_##n ,
		#include "cConfig.inc"
		#undef P
		#undef C
	};

	QList<cRule> rules;

	cConfig();

	bool load( const QString& file );
	bool save( const QString& file , QList<int> categories );

	bool loadProfile  ( );
	bool loadDevice   ( );
	bool loadOculusSdk( );
	bool loadRules    ( );
	bool saveRules    ( );

	void        calculateValues();
	QString     getMainConfigFile( );
	QString     getGameConfigFile( const QString& gameExePath );
	QString     getProfileConfigFile( );
	QString     getVRBoostRuleFilePath();
	QString     getDeviceShaderPath();
	QString     findProfileFileForExe( const QString& gameExePath );
	QStringList getAvailableProfiles( );
	QStringList getAvailableDevices ( );
	QString     getShaderRulesPath  ( );
	QString     getShaderPath   ( );
	QString     getRulesPath    ( );
};

