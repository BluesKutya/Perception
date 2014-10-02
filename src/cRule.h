#pragma once
#include <Vireio.h>
#include "cRuleInfo.h"

class cShader;
class cMenuItem;
class D3DProxyDevice;

class cRule : public cRuleInfo{
public:

	int             operation;

	D3DProxyDevice* device;
	cMenuItem*      item;
	cMenuItem*      itemConstants;
	cMenuItem*      itemShaders;

	static QStringList availableOperations();


	cRule( D3DProxyDevice* device );

	void updateConstantsMenu();
	void updateShadersMenu  ();
	void apply              ( float* src , float* dst , vireio::RenderPosition side );
	bool isMatrixOperation  ( );
	bool save               ( const QString& file );
	bool load               ( const QString& file );
};