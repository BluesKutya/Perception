#pragma once
#include <Vireio.h>
#include <qcryptographichash.h>
#include "cMenu.h"

class D3DProxyDevice;

class cShaderRule{
public:
	QString name;
	int     operation;
	bool    transpose;
};


class cShaderConstant : public D3DXCONSTANT_DESC {
public:
	QString            name;
	cMenuItem*         item;
	cShaderRule*       rule;
};


class cShader {
public:
	D3DProxyDevice*          device;
	IDirect3DVertexShader9*  vs;
	IDirect3DPixelShader9*   ps;
	QByteArray               code;
	QByteArray               hash;
	QVector<cShaderConstant> constants;
	bool                     blink;
	bool                     hide;
	bool                     used;
	cMenuItem*               item;
	
				          
	cShader( D3DProxyDevice* d , IDirect3DVertexShader9* avs , IDirect3DPixelShader9* aps );

};