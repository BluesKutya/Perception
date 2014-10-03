#pragma once
#include <Vireio.h>
#include "cRuleInfo.h"
class cConstantBuffer;
class cShader;
class cMenuItem;
class D3DProxyDevice;

class cRule : public cRuleInfo{
public:
	QVector<int> registerIndexes;
	int          operation;

	D3DProxyDevice* device;
	cMenuItem*      item;
	cMenuItem*      itemConstants;
	cMenuItem*      itemShaders;

	static QStringList availableOperations();


	cRule( D3DProxyDevice* device );

	void updateConstantsMenu();
	void updateShadersMenu  ();
	void applyTo            ( cConstantBuffer* buf , cConstantBuffer* bufLeft , cConstantBuffer* bufRight );
	bool save               ( const QString& file );
	bool load               ( const QString& file );


};