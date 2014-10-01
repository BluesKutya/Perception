#pragma once
#include <Vireio.h>
#include <qcryptographichash.h>
#include "cMenu.h"

class D3DProxyDevice;

class cShaderRule{
public:
	QString     ruleName;
	QString     constantName;
	QString     constantType;
	QStringList shadersInclude;
	QStringList shadersExclude;
	int         startRegister;
	int         operation;
	bool        transpose;
	

	enum{
		DoNothing,                   // Simple modification that does not apply anything.
		SimpleTranslate,             // Default modification is simple translate
		EyeShiftUnity,               //
		OrthographicSquash ,         // Squashes matrix if orthographic, otherwise simple translate. **/
		HudSlide,                    // Modification to slide the head up display (HUD) into the head mounted display (HMD) output.  **/
		GuiSquash ,                  // Modification to squash the graphical user interface (GUI). **/
		SurfaceRefractionTransform , // Modification to fix surface refraction in pixel shaders. **/
		GatheredOrthographicSquash,  // Squashes matrix if orthographic, otherwise simple translate. Result will be gathered to be used in other modifications.**/
		OrthographicSquashShifted,   // Squashes matrix if orthographic, otherwise simple translate - shift accordingly. **/
		OrthographicSquashHud,       // Squashes matrix if orthographic, otherwise simple translate - matrices treated as beeing for HUD. **/
		ConvergenceOffset ,          // Fixes far away objects using the convergence offset. **/
		SimpleTranslateIgnoreOrtho,  // Modification to ignore orthographic matrices. **/
		RollOnly,                    // Modification applies only the head roll. **/
		RollOnlyNegative,            // Modification applies only the head roll. (negative)**/
		RollOnlyHalf,                // Modification applies only the head roll. (half roll)**/
		NoRoll ,                     // Default modification without head roll. **/
		NoShow 					     // Causes shader to not be displayed. **/
	};

};


class cShaderConstant : public D3DXCONSTANT_DESC {
public:
	QString             name;
	QList<cShaderRule*> rules;
	cMenuItem*          item;
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