#pragma once
#include <qstring.h>
#include <qmap.h>
#include <qvector.h>
#include <qstringlist.h>

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

	cConfig();

	bool load( const QString& file );
	bool save( const QString& file , QList<int> categories );

	bool loadProfile( );
	bool loadDevice ( );
	bool loadOculusSdk( );

	void        calculateValues();
	QString     getMainConfigFile( );
	QString     getGameConfigFile( const QString& gameExePath );
	QString     getProfileConfigFile( );
	QString     getShaderRuleFilePath();
	QString     getVRBoostRuleFilePath();
	QString     findProfileFileForExe( const QString& gameExePath );
	QStringList getAvailableProfiles( );
	QStringList getAvailableDevices ( );
};

