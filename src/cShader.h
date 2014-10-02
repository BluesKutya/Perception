#pragma once
#include <Vireio.h>
#include <qcryptographichash.h>
#include "cMenu.h"
#include "cRule.h"

class D3DProxyDevice;


class cShaderConstant : public D3DXCONSTANT_DESC {
public:
	QString       name;
	QString       typeName;
	QList<cRule*> rules;
	cMenuItem*    item;
	float         value[16];
};


class cShader {
public:
	D3DProxyDevice*          device;
	IDirect3DVertexShader9*  vs;
	IDirect3DPixelShader9*   ps;
	QByteArray               code;
	QString                  name;
	QVector<cShaderConstant> constants;
	bool                     blink;
	bool                     hide;
	bool                     used;
	bool                     visible;
	cMenuItem*               item;
	
				          
	cShader( D3DProxyDevice* d , IDirect3DVertexShader9* avs , IDirect3DPixelShader9* aps );
	void updateRules();
};