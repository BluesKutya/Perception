#pragma once
#include <Vireio.h>
class cShader;
class cMenuItem;
class D3DProxyDevice;

class cRule{
public:
	QString         ruleName;
	QStringList     constantNames;
	QStringList     shadersInclude;
	QStringList     shadersExclude;
	int             operation;
	bool            transpose;

	D3DProxyDevice* device;
	cMenuItem*      item;
	cMenuItem*      itemConstants;
	cMenuItem*      itemShaders;

	static QStringList availableOperations();


	cRule( D3DProxyDevice* device );
	~cRule();

	void updateConstantsMenu();
	void updateShadersMenu  ();
};