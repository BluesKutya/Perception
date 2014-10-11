#pragma once
#include <qstringlist.h>

class cConstantBuffer;
class cShader;
class cMenuItem;
class D3DProxyDevice;

class cRule{
public:
	QString         name;
	QStringList     constantsInclude;
	QStringList     shadersInclude;
	QStringList     shadersExclude;
	int             operation;
	bool            isMatrix;
	bool            transpose;
	bool            squishViewport;
	bool            shaderBlink;
	bool            shaderHide;

	cRule();

	D3DProxyDevice* device;
	cMenuItem*      item;
	cMenuItem*      itemConstants;
	cMenuItem*      itemShaders;
};