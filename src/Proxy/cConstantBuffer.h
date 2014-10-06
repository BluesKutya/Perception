#pragma once
#include <Vireio.h>
#include <vector>

class IDirect3DDevice9;
class cRule;

class cConstantBuffer{
public:
	bool                  set            ( int registerIndex  , const float* ptr , int registerCount );
	bool                  get            ( int registerOffset ,       float* ptr , int registerCount );
	void                  writeTo        ( IDirect3DDevice9* device , bool vs );
	bool                  isModified     ( int registerIndex , int registerCount );
	void                  setModified    ( int registerIndex , int registerCount );
	float*                data           ( int registerIndex , int registerCount );
	int                   registerCount  ( );

private:
	std::vector<float>               registers;
	std::vector<std::pair<int,int>>  modified;

	void resize( int registerCount , bool mod );

	friend class cRule;
};
