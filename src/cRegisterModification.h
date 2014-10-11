#pragma once
#include <Vireio.h>

class D3DProxyDevice;

class cRegisterModification{
public:
	int    start;
	int    count;
	int    operation;
	bool   isMatrix;

	cRegisterModification( );

	void modify( D3DProxyDevice* device , float* ptrData , float* ptrLeft , float* ptrRight );

	static QStringList availableOperations();
};