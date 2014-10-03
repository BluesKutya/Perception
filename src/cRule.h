#pragma once
#include <Vireio.h>
#include "cRuleInfo.h"
class cConstantBuffer;
class cShader;
class cMenuItem;
class D3DProxyDevice;

class cRule : public cRuleInfo{
public:
	QVector<int> vsRegisters;
	QVector<int> psRegisters;
	int          operation;

	D3DProxyDevice* device;
	cMenuItem*      item;
	cMenuItem*      itemConstants;
	cMenuItem*      itemShaders;

	cRule( D3DProxyDevice* device );

	void modify( int registerIndex , cConstantBuffer* buf , cConstantBuffer* bufLeft , cConstantBuffer* bufRight );
	static QStringList availableOperations();
};