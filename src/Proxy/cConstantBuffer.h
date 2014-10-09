#pragma once
#include <Vireio.h>
#include <vector>
#include "cRegisterModification.h"
#include "cRule.h"

class IDirect3DDevice9;

class cConstantBuffer{
public:

	cConstantBuffer( D3DProxyDevice* device , bool isVertex );

	void resize               ( int count );

	bool isModified           ( int start , int count );
	void clearModified        ( );
	void setModified          ( int start , int count );
	void setStereoModified    ( );

	HRESULT set                  ( int start , const float* ptr , int count );
	HRESULT get                  ( int start ,       float* ptr , int count );

	void applyStereo          ( );

	int  registerCount        ( );

private:
	std::vector<float>                   original;
	std::vector<float>                   left;
	std::vector<float>                   right;
	std::vector<std::pair<int,int>>      modified;
	std::vector<cRegisterModification>*  modifications;
	D3DProxyDevice*                      device;
	bool                                 isVertex;



	
	HRESULT set  ( std::vector<float>& array , int start , const float* ptr , int count );
	void    write( std::vector<float>& array );

	friend class cRule;
};
