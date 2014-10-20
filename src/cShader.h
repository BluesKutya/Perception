#pragma once
#include <Vireio.h>
#include <qcryptographichash.h>
#include "cMenu.h"
#include "cRule.h"
#include "cRegisterModification.h"

class D3DProxyDevice;


class cShaderConstant : public D3DXCONSTANT_DESC {
public:
	QString       name;
	QString       typeName;
	cMenuItem*    item;
	bool          isMatrix();
};


class cShader {
public:
	D3DProxyDevice*                    device;
	IDirect3DVertexShader9*            vs;
	IDirect3DPixelShader9*             ps;
	QString                            name;
	QVector<cShaderConstant>           constants;
	std::vector<cRegisterModification> modifications;
	bool                               squishViewport;
	bool                               blink;
	bool                               hide;
	bool                               used;
	bool                               doDisableZ;
	bool                               visible;
	cMenuItem*                         item;
	
				          
	cShader( D3DProxyDevice* d , IDirect3DVertexShader9* avs , IDirect3DPixelShader9* aps );
	~cShader();
};