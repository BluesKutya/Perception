#ifndef STEREOVIEW_H_INCLUDED
#define STEREOVIEW_H_INCLUDED

#include "D3DProxyDevice.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <map>
#include <string.h>
#include <assert.h>
#include <cConfig.h>


class Streamer;
class D3DProxyDevice;


/**
* Stereo-view render class.
* Basic class to render in stereo.
*/
class StereoView
{
public:
	 StereoView( );
	~StereoView( );

	/*** StereoView public methods ***/
	virtual void Init(D3DProxyDevice* d);
	virtual void ReleaseEverything();
	virtual void Draw(D3D9ProxySwapChain* chain);
	virtual void SaveScreen();
	virtual void PostReset();





	D3DVIEWPORT9      viewport;
	IDirect3DDevice9* actual;
	D3DProxyDevice*   proxy;
	bool initialized;
	float HeadYOffset;	
	float HeadZOffset;
	float XOffset;	
	float LensCenter[2];
	float ViewportXOffset;
	float ViewportYOffset;
	float Scale[2];
	float ScaleIn[2];
	Streamer* m_pStreamer;


protected:
	/*** StereoView protected methods ***/
	void InitTextureBuffers();
	void InitVertexBuffers();
	void InitShaderEffects();
	void SetViewEffectInitialValues(); 
	void CalculateShaderVariables();
	void SaveState();
	void SetState();
	void RestoreState();
	void SaveAllRenderStates(LPDIRECT3DDEVICE9 pDevice);
	void SetAllRenderStatesDefault(LPDIRECT3DDEVICE9 pDevice);
	void RestoreAllRenderStates(LPDIRECT3DDEVICE9 pDevice);
	
	IDirect3DSurface9*      backBuffer;
	IDirect3DTexture9*      leftTexture;
	IDirect3DTexture9*      rightTexture;
	IDirect3DSurface9*      leftSurface;
	IDirect3DSurface9*      rightSurface;
	IDirect3DVertexBuffer9* screenVertexBuffer;
	ID3DXEffect*            viewEffect;


};

/**
* Declaration of texture vertex used for full screen render.
***/
const DWORD D3DFVF_TEXVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;
/**
* Texture vertex used for full screen render.
***/
struct TEXVERTEX
{
	float x;
	float y;
	float z;
	float rhw;
	float u;
	float v;
};

#endif
