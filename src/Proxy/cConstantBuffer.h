#pragma once
#include <Vireio.h>
#include <qvector.h>
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
	QVector<float> registers;
	QVector<bool>  modified;

	void resize( int registerCount , bool mod );

	friend class cRule;
};
