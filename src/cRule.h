#pragma once
#include <qstringlist.h>

class cConstantBuffer;
class cShader;
class cMenuItem;
class D3DProxyDevice;

class cRule{
public:
	#define  P(A,B,C) A B;
	#include "cRule.inc"
	#undef   P

	cRule();

	D3DProxyDevice* device;
	cMenuItem*      item;
	cMenuItem*      itemConstants;
	cMenuItem*      itemShaders;
};