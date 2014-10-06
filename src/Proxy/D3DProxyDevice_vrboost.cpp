#include "D3DProxyDevice.h"
#include "VRBoostEnums.h"
#include "cTracker.h"

using namespace VRBoost;

METHOD_IMPL( void , , D3DProxyDevice , vrbInit )
	vrbLib = LoadLibraryA("VRboost.dll");
	if( !vrbLib ){
		return;
	}

	vrbFuncLoadMemoryRules       = GetProcAddress( vrbLib , "VRboost_LoadMemoryRules"       );
	vrbFuncSaveMemoryRules       = GetProcAddress( vrbLib , "VRboost_SaveMemoryRules"       );
	vrbFuncCreateFloatMemoryRule = GetProcAddress( vrbLib , "VRboost_CreateFloatMemoryRule" );
	vrbFuncSetProcess            = GetProcAddress( vrbLib , "VRboost_SetProcess"            );
	vrbFuncReleaseAllMemoryRules = GetProcAddress( vrbLib , "VRboost_ReleaseAllMemoryRules" );
	vrbFuncApplyMemoryRules      = GetProcAddress( vrbLib , "VRboost_ApplyMemoryRules"      );


	if( !vrbFuncLoadMemoryRules       ||
	 	!vrbFuncSaveMemoryRules       ||
	 	!vrbFuncCreateFloatMemoryRule ||
	 	!vrbFuncSetProcess            ||
	 	!vrbFuncReleaseAllMemoryRules ||
	 	!vrbFuncApplyMemoryRules     
	){
		vrbFree();
		menu.showMessage( "VRBoost load failed" );
		return;
	}


	// set common default VRBoost values
	ZeroMemory(&vrbValues[0], MAX_VRBOOST_VALUES*sizeof(float));
	vrbValues[VRboostAxis::Zero       ] = 0.0f;
	vrbValues[VRboostAxis::One        ] = 1.0f;
	vrbValues[VRboostAxis::WorldFOV   ] = 95.0f;
	vrbValues[VRboostAxis::PlayerFOV  ] = 125.0f;
	vrbValues[VRboostAxis::FarPlaneFOV] = 95.0f;

	vrbRulesLoaded = false;
	
}

METHOD_IMPL( void , , D3DProxyDevice , vrbFree )
	if( vrbLib ){
		if( vrbFuncReleaseAllMemoryRules ){
			((HRESULT(WINAPI*)(void))vrbFuncReleaseAllMemoryRules)();;
		}

		FreeLibrary( vrbLib );
		vrbLib = 0;
	}
}

METHOD_IMPL( void , , D3DProxyDevice , vrbUpdate )
	// update vrboost, if present, tracker available and shader count higher than the minimum
	if( !vrbLib ||
	    !config.VRboostEnable ||
		m_VertexShaderCountLastFrame < (UINT)config.VRboostMinShaderCount ||
		m_VertexShaderCountLastFrame > (UINT)config.VRboostMaxShaderCount
	){
		return;
	}
	
	HRESULT r;


	// apply VRboost memory rules if present
	vrbValues[VRboostAxis::TrackerYaw]   = tracker->currentYaw;
	vrbValues[VRboostAxis::TrackerPitch] = tracker->currentPitch;
	vrbValues[VRboostAxis::TrackerRoll]  = tracker->currentRoll;

	if( !vrbRulesLoaded ){
		r = ((HRESULT(WINAPI*)(std::string,std::string))vrbFuncLoadMemoryRules)(config.exeName.toStdString() , config.getVRBoostRuleFilePath().toStdString() );

		if( r == S_OK ){
			vrbRulesLoaded = true;
		}else{
			menu.showMessage(
				"VRBoost LoadRules Failed!\n"
				"To Enable head tracking, turn on tracker mouse emulation\n"
				"in tracker configuration menu"
			);
			return;
		}
	}

	r = ((HRESULT(WINAPI*)(UINT,float**))vrbFuncApplyMemoryRules)( MAX_VRBOOST_VALUES , (float**)&vrbValues );
	if( r != S_OK ){
		menu.showMessage(
			"VRBoost rules loaded but could not be applied\n"
			"To Enable head tracking, turn on tracker mouse emulation\n"
			"in tracker configuration menu"
		);
	}
}



METHOD_IMPL( void , , D3DProxyDevice , vrbLoadValues )
	vrbValues[VRboostAxis::WorldFOV]                = config.WorldFOV;
	vrbValues[VRboostAxis::PlayerFOV]               = config.PlayerFOV;
	vrbValues[VRboostAxis::FarPlaneFOV]             = config.FarPlaneFOV;
	vrbValues[VRboostAxis::CameraTranslateX]        = config.CameraTranslateX;
	vrbValues[VRboostAxis::CameraTranslateY]        = config.CameraTranslateY;
	vrbValues[VRboostAxis::CameraTranslateZ]        = config.CameraTranslateZ;
	vrbValues[VRboostAxis::CameraDistance]          = config.CameraDistance;
	vrbValues[VRboostAxis::CameraZoom]              = config.CameraZoom;
	vrbValues[VRboostAxis::CameraHorizonAdjustment] = config.CameraHorizonAdjustment;
	vrbValues[VRboostAxis::ConstantValue1]          = config.ConstantValue1;
	vrbValues[VRboostAxis::ConstantValue2]          = config.ConstantValue2;
	vrbValues[VRboostAxis::ConstantValue3]          = config.ConstantValue3;
}



METHOD_IMPL( void , , D3DProxyDevice , vrbSaveValues )
}

/*
	void     vrbLoadValues( )
	void     vrbSaveValues( )

VRboost function pointer typedefs 
	typedef 
	typedef HRESULT (WINAPI *LPVRBOOST_SaveMemoryRules)(std::string rulesPath);
	typedef HRESULT (WINAPI *LPVRBOOST_CreateFloatMemoryRule)(DWORD ruleType, UINT axisIndex, D3DXVECTOR4 constantVector, DWORD pointerAddress, DWORD* offsets, DWORD minValue, DWORD maxValue, DWORD comparisationPointer1, DWORD* comparisationOffsets1, int pointerDifference1, DWORD comparisationPointer2, DWORD* comparisationOffsets2, int pointerDifference2);
	typedef HRESULT (WINAPI *LPVRBOOST_SetProcess)(std::string processName, std::string moduleName);
	
	typedef HRESULT (WINAPI *LPVRBOOST_ApplyMemoryRules)(UINT axisNumber, float** axis);
		bool                                      m_VRboostRulesPresent;
	
	bool           VRBoost_Active;
	bool           VRBoost_LoadRules;
	bool           VRBoost_ApplyRules;
	*/