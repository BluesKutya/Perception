#include "D3DProxyDevice.h"
#include "D3D9ProxySurface.h"
#include "D3D9ProxySwapChain.h"
#include "D3D9ProxyDirect3D.h"
#include "D3D9ProxyTexture.h"
#include "D3D9ProxyVolumeTexture.h"
#include "D3D9ProxyCubeTexture.h"
#include "D3D9ProxyVertexBuffer.h"
#include "D3D9ProxyIndexBuffer.h"
#include "D3D9ProxyStateBlock.h" 
#include "D3D9ProxyQuery.h"
#include "VRBoostEnums.h"
#include "StereoShaderConstant.h"
#include "StereoBackBuffer.h"
#include "GameHandler.h"
#include "ShaderRegisters.h"
#include "ViewAdjustment.h"
#include "StereoView.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <typeinfo>
#include <assert.h>
#include <comdef.h>
#include <tchar.h>
#include <cTracker.h>
#include <DxErr.h>

#define SMALL_FLOAT 0.001f
#define	SLIGHTLY_LESS_THAN_ONE 0.999f

#define OUTPUT_HRESULT(hr) { _com_error err(hr); LPCTSTR errMsg = err.ErrorMessage(); OutputDebugStringA(errMsg); }

#define MAX_PIXEL_SHADER_CONST_2_0 32
#define MAX_PIXEL_SHADER_CONST_2_X 32
#define MAX_PIXEL_SHADER_CONST_3_0 224

using namespace VRBoost;


/**
* Constructor : creates game handler and sets various states.
***/
D3DProxyDevice::D3DProxyDevice(IDirect3DDevice9* pDevice,IDirect3DDevice9Ex* pDeviceEx,  D3D9ProxyDirect3D* pCreatedBy ) :
	actual(pDevice),
	actualEx(pDeviceEx),
	m_pCreatedBy(pCreatedBy),
	m_nRefCount(1),
	m_activeRenderTargets (1, NULL),
	m_activeTextureStages(),
	m_activeVertexBuffers(),
	m_activeSwapChains(),
	controls(),
	dinput(),
	show_fps(FPS_NONE),
	calibrate_tracker(false)
{

	tracker = 0;

	menu.init( this );

	InitVRBoost();

	m_spShaderViewAdjustment = std::make_shared<ViewAdjustment>( );
	m_pGameHandler = new GameHandler();

	// Check the maximum number of supported render targets
	D3DCAPS9 capabilities;
	actual->GetDeviceCaps(&capabilities);
	DWORD maxRenderTargets = capabilities.NumSimultaneousRTs;
	m_activeRenderTargets.resize(maxRenderTargets, NULL);

	D3DXMatrixIdentity(&m_leftView);
	D3DXMatrixIdentity(&m_rightView);
	D3DXMatrixIdentity(&m_leftProjection);
	D3DXMatrixIdentity(&m_rightProjection);	

	m_currentRenderingSide = vireio::Left;
	m_pCurrentMatViewTransform = &m_spShaderViewAdjustment->LeftAdjustmentMatrix(); 
	m_pCurrentView = &m_leftView;
	m_pCurrentProjection = &m_leftProjection;

	// get pixel shader max constants
	auto major_ps=D3DSHADER_VERSION_MAJOR(capabilities.PixelShaderVersion);
	auto minor_ps=D3DSHADER_VERSION_MINOR(capabilities.PixelShaderVersion);
	DWORD MaxPixelShaderConst = MAX_PIXEL_SHADER_CONST_2_0;
	if ((major_ps>=2) && (minor_ps>0)) MaxPixelShaderConst = MAX_PIXEL_SHADER_CONST_2_X;
	if ((major_ps>=3) && (minor_ps>=0)) MaxPixelShaderConst = MAX_PIXEL_SHADER_CONST_3_0;

	m_spManagedShaderRegisters = std::make_shared<ShaderRegisters>(MaxPixelShaderConst, capabilities.MaxVertexShaderConst, pDevice);	
	m_pActiveStereoDepthStencil = NULL;
	m_pActiveIndicies = NULL;
	m_pActiveVertexDeclaration = NULL;
	m_bActiveViewportIsDefault = true;
	m_bViewportIsSquished = false;
	m_bViewTransformSet = false;
	m_bProjectionTransformSet = false;
	m_bInBeginEndStateBlock = false;
	m_pCapturingStateTo = NULL;
	m_isFirstBeginSceneOfFrame = true;


	currentVS         = 0;
	currentPS         = 0;
	showUnusedShaders = false;
	showPixelShaders  = false;







	InitBrassa();
	//Create Direct Input Mouse Device
	bool directInputActivated = dinput.Init(GetModuleHandle(NULL), ::GetActiveWindow());
	if(directInputActivated)
	{		
		dinput.Activate();		
	}	

	/*
	menu.showMessage( 
		"Vireio Perception: Stereoscopic 3D Driver\n"
		"This program is distributed in the hope that it will be useful,\n" 
		"but WITHOUT ANY WARRANTY; without even the implied warranty of \n" 
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
		"See the GNU LGPL: http://www.gnu.org/licenses/ for more details. "
	);*/

	eyeShutter = 1;
	m_bfloatingMenu = false;
	m_bfloatingScreen = false;

	// first time configuration
	m_pGameHandler->Load(config, m_spShaderViewAdjustment);
	stereoView                 = new StereoView();
	stereoView->HeadYOffset    = 0;
	
	BRASSA_UpdateDeviceSettings();
	OnCreateOrRestore();

	//credits will go to world-scale menu, or maybe to startup screen!
	//"Brown Reischl and Schneider Settings Analyzer (B.R.A.S.S.A.)."


	cMenuItem* i;
	cMenuItem* m;
	

	m = menu.root.addSubmenu( "Stereoscopic 3D calibration" );
	m->showCalibrator = true;
	
	i = m->addSpinner( "Separation" , &config.stereoScale , 0.0000001 , 100000 , 0.005 );
	i->callbackValueChanged = [this](){
		m_spShaderViewAdjustment->UpdateProjectionMatrices((float)stereoView->viewport.Width/(float)stereoView->viewport.Height);
		m_spShaderViewAdjustment->ComputeViewTransforms();
	};

	i = m->addSpinner( "Convergence" , &config.stereoConvergence , -100 , 100 , 0.01 );
	i->callbackValueChanged = [this](){
		m_spShaderViewAdjustment->UpdateProjectionMatrices((float)stereoView->viewport.Width/(float)stereoView->viewport.Height);
		m_spShaderViewAdjustment->ComputeViewTransforms();
	};





	m = menu.root.addSubmenu( "Image settings" );
	i = m->addCheckbox( "Swap eyes"         , &config.swap_eyes );

	i = m->addSpinner ( "IPD offset" , &config.IPDOffset        , 0.001 );
	i->callbackValueChanged = [this](){
		stereoView->PostReset();
	};

	i = m->addSpinner ( "Distortion scale" , &config.DistortionScale  , 0.01 );
	i->callbackValueChanged = [this](){
		stereoView->PostReset();
	};

	m->addCheckbox( "Chromatic aberration correction" , &config.chromaticAberrationCorrection );
	
	i = m->addCheckbox( "VRboost" , &m_bVRBoostToggle );
	i->callbackValueChanged = [this](){
		if (hmVRboost!=NULL){
			m_pVRboost_ReleaseAllMemoryRules();
			if( tracker ){
				tracker->reset();
			}
		}
	};
	

	i = m->addAction ( "Take screenshot" );
	i->callbackValueChanged = [this](){
		screenshot = 3;
		menu.show = false;
	};




	m = menu.root.addSubmenu( "OSD and GUI settings" );
	m->addCheckbox( "Show VR mouse"            , &config.showVRMouse );
	m->addCheckbox( "Gui \"bullet labyrinth\"" , &config.guiBulletLabyrinth  );
	
	i = m->addSpinner ( "GUI squash"             , &config.guiSquash  , 0.01 );
	i->callbackValueChanged = [this](){
		m_spShaderViewAdjustment->UpdateGui();
	};

	i = m->addSpinner ( "GUI depth"                , &config.guiDepth     , 0.01 );
	i->callbackValueChanged = [this](){
		m_spShaderViewAdjustment->UpdateGui();
	};

	i = m->addSpinner ( "HUD distance"             , &config.hudDistance  , 0.01 );
	i->callbackValueChanged = [this](){
		m_spShaderViewAdjustment->UpdateGui();
	};

	i = m->addSpinner ( "HUD depth"                , &config.hudDepth     , 0.01 );
	i->callbackValueChanged = [this](){
		m_spShaderViewAdjustment->UpdateGui();
	};





	m = menu.root.addSubmenu( "VRBoost values" );
	m->callbackCloseSubmenu = [this](){
		BRASSA_UpdateConfigSettings();
	};

	m->addSpinner( "World FOV"                 , VRBoostValue + 24 , 0.01);
	m->addSpinner( "Player FOV"                , VRBoostValue + 25 , 0.01);
	m->addSpinner( "Far plane FOV"             , VRBoostValue + 26 , 0.01);
	m->addSpinner( "Camera translate X"        , VRBoostValue + 27 , 0.01);
	m->addSpinner( "Camera translate Y"        , VRBoostValue + 28 , 0.01);
	m->addSpinner( "Camera tanslate Z"         , VRBoostValue + 29 , 0.01);
	m->addSpinner( "Camera distance"           , VRBoostValue + 30 , 0.01);
	m->addSpinner( "Camera zoom"               , VRBoostValue + 31 , 0.01);
	m->addSpinner( "Camera horizon adjustment" , VRBoostValue + 32 , 0.01);
	m->addSpinner( "Constant value 1"          , VRBoostValue + 33 , 0.01);
	m->addSpinner( "Constant value 2"          , VRBoostValue + 34 , 0.01);
	m->addSpinner( "Constant value 2"          , VRBoostValue + 35 , 0.01);







	m = menu.root.addSubmenu( "Tracker Configuration" );
	

	m->addCheckbox( "Tracker rotation" , &config.trackerRotationEnable );
	m->addSpinner ( "Yaw   multiplier" , &config.trackerYawMultiplier   , 0.05 );
	m->addSpinner ( "Pitch multiplier" , &config.trackerPitchMultiplier , 0.05 );
	m->addSpinner ( "Roll  multiplier" , &config.trackerRollMultiplier  , 0.05 );
	m->addCheckbox( "Roll  thing..."   , &config.rollEnabled  );


	i = m->addCheckbox( "Tracker movement" , &config.trackerPositionEnable );
	i->callbackValueChanged = [this](){
		if( !config.trackerPositionEnable ){
			m_spShaderViewAdjustment->UpdatePosition(0.0f, 0.0f, 0.0f);
		}
	};

	i = m->addSpinner( "X multiplier" , &config.trackerXMultiplier , 0.00001 );
	i->callbackValueChanged = [this](){
		BRASSA_UpdateConfigSettings();
	};

	i = m->addSpinner( "Y multiplier" , &config.trackerYMultiplier , 0.00001 );
	i->callbackValueChanged = [this](){
		BRASSA_UpdateConfigSettings();
	};

	i = m->addSpinner( "Z multiplier" , &config.trackerZMultiplier , 0.00001 );
	i->callbackValueChanged = [this](){
		BRASSA_UpdateConfigSettings();
	};


	m->addCheckbox( "Mouse emulation"        , &config.trackerMouseEmulation );
	m->addSpinner ( "Mouse yaw   multiplier" , &config.trackerMouseYawMultiplier   , 0.05 );
	m->addSpinner ( "Mouse pitch multiplier" , &config.trackerMousePitchMultiplier , 0.05 );


	i = m->addAction( "Reset view"   );
	i->callbackValueChanged = [this](){
		if( tracker ){
			tracker->reset();
		}
	};





	i= menu.root.addAction ( "Restore configuration" );
	i->callbackValueChanged = [this](){
		config = configBackup;
	};



	i= menu.root.addAction ( "Save configuration" );
	i->callbackValueChanged = [this](){
		SaveConfiguration();
	};


	if( config.shaderAnalyzer ){

		m = menu.root.addSubmenu( "Shader analyzer" );

		i = m->addCheckbox( "Use transposed rules"          , &config.shaderAnalyzerTranspose       );

		i = m->addCheckbox( "Detect use of transposed rules" , &config.shaderAnalyzerDetectTranspose );

		shadersMenu = m->addSubmenu( "Shader list" );

		shadersMenu->addCheckbox( "Show unused shaders" , &showUnusedShaders );
		shadersMenu->addCheckbox( "Show pixel  shaders" , &showPixelShaders );
	}
	
	
	/*
	i = m->addAction( "Create new shader rule" );
	i->callbackOpenSubmenu = [this](){
		GetCurrentShaderRules(true);
		menu.show = false;
	};

	rulesMenu = m->addSubmenu( "Change current shader rules" );
	rulesMenu->callbackOpenSubmenu = [this](){
		GetCurrentShaderRules(false);
	};



	i = m->addSubmenu( "Save rules shaders" );
	i->callbackOpenSubmenu = [this](){
		saveShaderRules();
		menu.show = false;
	};*/


}











bool D3DProxyDevice::isDrawHide( ){
	return config.shaderAnalyzer && ((currentVS && currentVS->hide) || (currentPS && currentPS->hide));
}





/*
void DataGatherer::UpdateRuleDisplay( ShaderConstant* c ){
	static int dummy;
	
	if( !c->nodeCreated ){
	
		std::string s;

		c->applyRule = m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule( c->name.toStdString() , s , c->ruleOperation , c->ruleTranspose );

		if( c->applyRule ){
			c->ruleName = QString::fromStdString( s );

			c->item->addText    ( "Rule name"      , &c->ruleName )->readOnly = true;;
			
			c->item->addCheckbox( "Rule transpose" , &c->ruleTranspose )->callbackValueChanged = [=](){
				if( c->applyRule ){
				}
			};

			c->item->addSelect  ( "Rule operation" , &dummy , QStringList()<<"!"<<"$#$" );

			c->nodeCreated = true;
		}

		

		//c->item->addSelect( "Rule" , (int*)&c->ruleOperation 


	}

				/*mi = n->item->addCheckbox( "Apply rule" , &n->applyRule );
				mi->callbackValueChanged = [=](){
					if( n->applyRule ){
						bool transpose = config.shaderAnalyzerTranspose;
						if( n->desc.Class == D3DXPARAMETER_CLASS::D3DXPC_VECTOR ){
							transpose = false;
						}
						addRule( n->name.toStdString() , true , n->desc.RegisterIndex , n->desc.Class , 1 , transpose );
					}else{
						deleteRule( n->name.toStdString() );
					}
				};*


				//n->item->addSelect  ( "Rule "      , &n->haveRule );

}
*/




/*


void DataGatherer::BRASSA_ChangeRules()
{
	menuHelperRect.left = 0;
	menuHelperRect.top = 0;

	UINT menuEntryCount = 2;
	UINT constantIndex = 0;
	std::vector<std::string> menuEntries;
	std::vector<bool> menuColor;
	std::vector<DWORD> menuID;
	// loop through relevant vertex shader constants
	auto itShaderConstants = m_relevantVSConstantNames.begin();
	while (itShaderConstants != m_relevantVSConstantNames.end())
	{
		menuColor.push_back(itShaderConstants->hasRule);
		menuID.push_back(constantIndex);
		menuEntries.push_back(itShaderConstants->name);
		if (itShaderConstants->nodeOpen)
		{
			// output the class
			menuColor.push_back(itShaderConstants->hasRule);
			menuID.push_back(constantIndex+(1<<31));
			// output shader constant + index 
			switch(itShaderConstants->desc.Class)
			{
			case D3DXPC_VECTOR:
				menuEntries.push_back("  D3DXPC_VECTOR");
				break;
			case D3DXPC_MATRIX_ROWS:
				menuEntries.push_back("  D3DXPC_MATRIX_ROWS");
				break;
			case D3DXPC_MATRIX_COLUMNS:
				menuEntries.push_back("  D3DXPC_MATRIX_COLUMNS");
				break;
			}
			menuEntryCount++;

			// output the class
			menuColor.push_back(itShaderConstants->hasRule);
			menuID.push_back(constantIndex+(1<<30));
			if (itShaderConstants->hasRule)
				menuEntries.push_back("  "+itShaderConstants->ruleName);
			else
				menuEntries.push_back("  No Rule assigned");
			menuEntryCount++;

			// output wether transposed or not
			if ((itShaderConstants->hasRule) && (itShaderConstants->desc.Class != D3DXPC_VECTOR))
			{
				menuColor.push_back(itShaderConstants->hasRule);
				menuID.push_back(constantIndex+(1<<29));
				if (itShaderConstants->isTransposed)
					menuEntries.push_back("  Transposed");
				else
					menuEntries.push_back("  Non-Transposed");
				menuEntryCount++;
			}
		}

		constantIndex++;
		menuEntryCount++;
		++itShaderConstants;
	}


	if ((controls.Key_Down(VK_RETURN) || controls.Key_Down(VK_RSHIFT) || (controls.xButtonsStatus[0x0c])) && (menuVelocity == D3DXVECTOR2(0.0f, 0.0f)))
	{
		// switch shader rule node
		if ((entryID >= 0) && (entryID < menuEntryCount-2) && (menuEntryCount>2))
		{
			// constant node entry ?
			if ((menuID[entryID] & (1<<31)) == (1<<31))
			{
				// no influence on class node entry
			}
			else if ((menuID[entryID] & (1<<30)) == (1<<30)) // add/delete rule
			{
				// no rule present, so add
				if (!m_relevantVSConstantNames[menuID[entryID]].hasRule)
				{
					auto itShaderConstants = m_relevantVSConstants.begin();
					while (itShaderConstants != m_relevantVSConstants.end())
					{
						// constant name in menu entries already present
						if (itShaderConstants->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
						{
							// never assign "transposed" to vector
							if (itShaderConstants->desc.Class == D3DXPARAMETER_CLASS::D3DXPC_VECTOR)
								
							else
								addRule(itShaderConstants->name, true, itShaderConstants->desc.RegisterIndex, itShaderConstants->desc.Class, 1, m_bTransposedRules);
							itShaderConstants->hasRule = true;

							// set the menu output accordingly
							auto itShaderConstants1 = m_relevantVSConstantNames.begin();
							while (itShaderConstants1 != m_relevantVSConstantNames.end())
							{
								// set rule bool for all relevant constant names
								if (itShaderConstants1->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
								{
									UINT operation;
									itShaderConstants1->hasRule = m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule(itShaderConstants1->name, itShaderConstants1->ruleName, operation, itShaderConstants1->isTransposed);
								}
								++itShaderConstants1;
							}
						}

						++itShaderConstants;
					}
				}
				else // rule present, so delete
				{
					deleteRule(m_relevantVSConstantNames[menuID[entryID]].name);

					// set the menu output accordingly
					auto itShaderConstants1 = m_relevantVSConstantNames.begin();
					while (itShaderConstants1 != m_relevantVSConstantNames.end())
					{
						// set rule bool for all relevant constant names
						if (itShaderConstants1->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
						{
							UINT operation;
							itShaderConstants1->hasRule = m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule(itShaderConstants1->name, itShaderConstants1->ruleName, operation, itShaderConstants1->isTransposed);
						}
						++itShaderConstants1;
					}
				}
			}
			else if ((menuID[entryID] & (1<<29)) == (1<<29))
			{
				bool newTrans = !m_relevantVSConstantNames[menuID[entryID]].isTransposed;
				// transposed or non-transposed
				auto itShaderConstants = m_relevantVSConstants.begin();
				while (itShaderConstants != m_relevantVSConstants.end())
				{
					// constant name in menu entries already present
					if (itShaderConstants->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
					{
						// get the operation id
						UINT operation;
						m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule(itShaderConstants->name, itShaderConstants->ruleName, operation, itShaderConstants->isTransposed);
						modifyRule(itShaderConstants->name, operation, newTrans);

						// set the menu output accordingly
						auto itShaderConstants1 = m_relevantVSConstantNames.begin();
						while (itShaderConstants1 != m_relevantVSConstantNames.end())
						{
							// set rule bool for all relevant constant names
							if (itShaderConstants1->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
							{
								itShaderConstants1->hasRule = m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule(itShaderConstants1->name, itShaderConstants1->ruleName, operation, itShaderConstants1->isTransposed);
							}
							++itShaderConstants1;
						}
					}

					itShaderConstants++;
				}
			}
			else
			{
				// open or close node
				m_relevantVSConstantNames[menuID[entryID]].nodeOpen = !m_relevantVSConstantNames[menuID[entryID]].nodeOpen;

				auto itShaderConstants = m_relevantVSConstants.begin();
				while (itShaderConstants != m_relevantVSConstants.end())
				{
					// constant name in menu entries already present
					if (itShaderConstants->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
					{
						// show blinking if shader is drawn
						if (m_relevantVSConstantNames[menuID[entryID]].nodeOpen)
						{
							if (std::find(m_excludedVShaders.begin(), m_excludedVShaders.end(), itShaderConstants->hash) == m_excludedVShaders.end()) {
								m_excludedVShaders.push_back(itShaderConstants->hash);
							}
						}
						else
						{
							// erase all entries for that hash
							m_excludedVShaders.erase(std::remove(m_excludedVShaders.begin(), m_excludedVShaders.end(), itShaderConstants->hash), m_excludedVShaders.end()); 
						}
					}
					++itShaderConstants;
				}
			}

			menuVelocity.x+=2.0f;
		}
		// back to main menu
		if (entryID == menuEntryCount-2)
		{
			BRASSA_mode = BRASSA_Modes::MAINMENU;
			menuVelocity.x+=2.0f;
		}
		// back to game
		if (entryID == menuEntryCount-1)
		{
			BRASSA_mode = BRASSA_Modes::INACTIVE;
		}
	}

	if ((controls.Key_Down(VK_LEFT) || controls.Key_Down(0x4A) || (controls.xInputState.Gamepad.sThumbLX<-8192)) && (menuVelocity == D3DXVECTOR2(0.0f, 0.0f)))
	{
		// switch shader rule node
		if ((entryID >= 0) && (entryID < menuEntryCount-2) && (menuEntryCount>2))
		{
			if ((menuID[entryID] & (1<<30)) == (1<<30)) // rule node entry
			{
				// rule present, so modify
				if (m_relevantVSConstantNames[menuID[entryID]].hasRule)
				{
					// get the operation id
					UINT operation;
					m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule(m_relevantVSConstantNames[menuID[entryID]].name, m_relevantVSConstantNames[menuID[entryID]].ruleName, operation, m_relevantVSConstantNames[menuID[entryID]].isTransposed);
					if (operation > 0)
						operation--;

					auto itShaderConstants = m_relevantVSConstants.begin();
					while (itShaderConstants != m_relevantVSConstants.end())
					{
						// constant name in menu entries already present
						if (itShaderConstants->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
						{
							modifyRule(itShaderConstants->name, operation, itShaderConstants->isTransposed);

							// set the menu output accordingly
							auto itShaderConstants1 = m_relevantVSConstantNames.begin();
							while (itShaderConstants1 != m_relevantVSConstantNames.end())
							{
								// set rule bool for all relevant constant names
								if (itShaderConstants1->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
								{
									itShaderConstants1->hasRule = m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule(itShaderConstants1->name, itShaderConstants1->ruleName, operation, itShaderConstants1->isTransposed);
								}
								++itShaderConstants1;
							}
						}

						itShaderConstants++;
					}
				}
			}
		}
		menuVelocity.x+=2.0f;
	}

	if ((controls.Key_Down(VK_RIGHT) || controls.Key_Down(0x4C) || (controls.xInputState.Gamepad.sThumbLX>8192)) && (menuVelocity == D3DXVECTOR2(0.0f, 0.0f)))
	{
		// switch shader rule node
		if ((entryID >= 0) && (entryID < menuEntryCount-2) && (menuEntryCount>2))
		{
			if ((menuID[entryID] & (1<<30)) == (1<<30)) // rule node entry
			{
				// rule present, so modify
				if (m_relevantVSConstantNames[menuID[entryID]].hasRule)
				{
					// get the operation id
					UINT operation;
					m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule(m_relevantVSConstantNames[menuID[entryID]].name, m_relevantVSConstantNames[menuID[entryID]].ruleName, operation, m_relevantVSConstantNames[menuID[entryID]].isTransposed);
					if (m_relevantVSConstantNames[menuID[entryID]].desc.Class == D3DXPARAMETER_CLASS::D3DXPC_VECTOR)
					{
						if (operation < (UINT)ShaderConstantModificationFactory::Vec4EyeShiftUnity)
							operation++;
					}
					else
					{
						if (operation < (UINT)ShaderConstantModificationFactory::MatConvergenceOffset)
							operation++;
					}

					auto itShaderConstants = m_relevantVSConstants.begin();
					while (itShaderConstants != m_relevantVSConstants.end())
					{
						// constant name in menu entries already present
						if (itShaderConstants->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
						{
							modifyRule(itShaderConstants->name, operation, itShaderConstants->isTransposed);

							// set the menu output accordingly
							auto itShaderConstants1 = m_relevantVSConstantNames.begin();
							while (itShaderConstants1 != m_relevantVSConstantNames.end())
							{
								// set rule bool for all relevant constant names
								if (itShaderConstants1->name.compare(m_relevantVSConstantNames[menuID[entryID]].name) == 0)
								{
									itShaderConstants1->hasRule = m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule(itShaderConstants1->name, itShaderConstants1->ruleName, operation, itShaderConstants1->isTransposed);
								}
								++itShaderConstants1;
							}
						}

						itShaderConstants++;
					}
				}
			}
		}
		menuVelocity.x+=2.0f;
	}

}



void DataGatherer::Analyze()
{

	// loop through relevant vertex shader constants
	auto itShaderConstants = m_relevantVSConstantNames.begin();
	while (itShaderConstants != m_relevantVSConstantNames.end())
	{
		// loop through matrix constant name assumptions
		for (int i = 0; i < MATRIX_NAMES; i++)
		{
			// test if assumption is found in constant name
			if (strstr(itShaderConstants->name.c_str(), m_wvpMatrixConstantNames[i].c_str()) != 0)
			{
				// test for "to-be-avoided" assumptions
				for (int j = 0; j < AVOID_SUBSTRINGS; j++)
				{
					if (strstr(itShaderConstants->name.c_str(), m_wvpMatrixAvoidedSubstrings[j].c_str()) != 0)
					{
						// break loop
						i = MATRIX_NAMES;
						break;
					}
				}

				// still in loop ?
				if (i < MATRIX_NAMES)
				{
					// add this rule !!!!
					if (addRule(itShaderConstants->name, true, itShaderConstants->desc.RegisterIndex, itShaderConstants->desc.Class, 2, m_bTransposedRules))
						m_addedVSConstants.push_back(*itShaderConstants);

					// output debug data
					OutputDebugStringA("---Shader Rule");
					// output constant name
					OutputDebugStringA(itShaderConstants->desc.Name);
					// output shader constant + index 
					switch(itShaderConstants->desc.Class)
					{
					case D3DXPC_VECTOR:
						OutputDebugStringA("D3DXPC_VECTOR");
						break;
					case D3DXPC_MATRIX_ROWS:
						OutputDebugStringA("D3DXPC_MATRIX_ROWS");
						break;
					case D3DXPC_MATRIX_COLUMNS:
						OutputDebugStringA("D3DXPC_MATRIX_COLUMNS");
						break;
					}
					char buf[32];
					sprintf_s(buf,"Register Index: %d", itShaderConstants->desc.RegisterIndex);
					OutputDebugStringA(buf);
					sprintf_s(buf,"Shader Hash: %u", itShaderConstants->hash);
					OutputDebugStringA(buf);
					sprintf_s(buf,"Transposed: %d", m_bTransposedRules);
					OutputDebugStringA(buf);

					// end loop
					i = MATRIX_NAMES;
				}
			}
			else
				if (itShaderConstants->desc.RegisterIndex == 128)
				{
					OutputDebugStringA(itShaderConstants->name.c_str());
					OutputDebugStringA(m_wvpMatrixConstantNames[i].c_str());
				}
		}

		++itShaderConstants;
	}

	// save data
	saveShaderRules();
	
}


void DataGatherer::GetCurrentShaderRules(bool allStartRegisters)
{
	ShaderModificationRepository* pModRep = m_pGameHandler->GetShaderModificationRepository();

	// clear name vector, loop through constants
	for( ShaderConstant& c : m_relevantVSConstantNames ){
		delete c.item;
	}
	m_relevantVSConstantNames.clear();


	for( ShaderConstant& constant : m_relevantVSConstants ){

		bool namePresent     = false;
		bool registerPresent = !allStartRegisters;

		for( ShaderConstant& c : m_relevantVSConstantNames ){
			if( constant.name.compare(c.name) == 0 ){
				namePresent = true;
				if( constant.desc.RegisterIndex == c.desc.RegisterIndex ){
					registerPresent = true;
				}
			}
		}

		if( !namePresent || !registerPresent ){
			UINT operation = 0;

			if( pModRep ){
				constant.hasRule = pModRep->ConstantHasRule( constant.name , constant.ruleName , operation , constant.isTransposed );
			}else{
				constant.hasRule = false;
			}

			QString name = QString::fromStdString(constant.name);

			switch( constant.desc.Class ){
			case D3DXPC_VECTOR:
				name += " (D3DXPC_VECTOR)";
				break;

			case D3DXPC_MATRIX_ROWS:
				name += " (D3DXPC_MATRIX_ROWS)";
				break;

			case D3DXPC_MATRIX_COLUMNS:
				name += " (D3DXPC_MATRIX_COLUMNS)";
				break;
			}

			constant.applyRule    = false;
			constant.isTransposed = false;
			constant.item         = rulesMenu->addSubmenu( name );
			

			cMenuItem* i;

			if( constant.hasRule && constant.desc.Class != D3DXPARAMETER_CLASS::D3DXPC_VECTOR ){
				i = constant.item->addCheckbox( "Transposed" , &constant.isTransposed );
				i->callbackValueChanged = [&](){
					//UINT operation;
					//m_pGameHandler->GetShaderModificationRepository()->ConstantHasRule( constant.name, constant.ruleName , operation , constant.isTransposed );
					//modifyRule( constant.name, operation, constant.isTransposed );
				};
			}

			i = constant.item->addCheckbox( "Apply rule" , &constant.applyRule );
			i->callbackValueChanged = [&](){
				if( constant.applyRule ){
					addRule( constant.name , true , constant.desc.RegisterIndex , constant.desc.Class , 1 , constant.isTransposed );
				}else{
					deleteRule( constant.name );
				}
			};

			m_relevantVSConstantNames.push_back( constant );
		}

	}
}

*/






































void D3DProxyDevice::SaveConfiguration(){
	static char exe_path[MAX_PATH];
	GetModuleFileNameA( 0   , exe_path , MAX_PATH );
	menu.saveHotkeys( );
	config.save( config.getGameConfigFile(exe_path) , QList<int>() << cConfig::SAVE_GAME << cConfig::SAVE_PROFILE << cConfig::SAVE_USER );
}

/**
* Destructor : calls ReleaseEverything() and releases swap chains.
* @see ReleaseEverything()
***/
D3DProxyDevice::~D3DProxyDevice()
{

	#ifdef SHOW_CALLS
		OutputDebugStringA("called ~D3DProxyDevice");
	#endif
	ReleaseEverything();

	m_spShaderViewAdjustment.reset();

	delete m_pGameHandler;
	m_spManagedShaderRegisters.reset();

	FreeLibrary(hmVRboost);

	// always do this last
	auto it = m_activeSwapChains.begin();
	while (it != m_activeSwapChains.end()) {

		if ((*it) != NULL) {
			(*it)->Release();
			delete (*it);
		}

		it = m_activeSwapChains.erase(it);
	}

#ifdef _EXPORT_LOGFILE
	m_logFile.close();
#endif

	SAFE_RELEASE(actual)
}

#define IF_GUID(riid,a,b,c,d,e,f,g,h,i,j,k) if ((riid.Data1==a)&&(riid.Data2==b)&&(riid.Data3==c)&&(riid.Data4[0]==d)&&(riid.Data4[1]==e)&&(riid.Data4[2]==f)&&(riid.Data4[3]==g)&&(riid.Data4[4]==h)&&(riid.Data4[5]==i)&&(riid.Data4[6]==j)&&(riid.Data4[7]==k))
/**
* Catch QueryInterface calls and increment the reference counter if necesarry. 
***/

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , QueryInterface , REFIID , riid , LPVOID* , ppv )

	//DEFINE_GUID(IID_IDirect3DDevice9Ex, 0xb18b10ce, 0x2649, 0x405a, 0x87, 0xf, 0x95, 0xf7, 0x77, 0xd4, 0x31, 0x3a);
	IF_GUID(riid,0xb18b10ce,0x2649,0x405a,0x87,0xf,0x95,0xf7,0x77,0xd4,0x31,0x3a)
	{
		if (ppv==NULL)
			return E_POINTER;

		this->AddRef();
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	return actual->QueryInterface(riid,ppv);
}



METHOD_IMPL( ULONG , WINAPI , D3DProxyDevice , AddRef )	 
	return ++m_nRefCount;
}


METHOD_IMPL( ULONG , WINAPI , D3DProxyDevice , Release )
	if(--m_nRefCount == 0){
		delete this;
		return 0;
	}	
	return m_nRefCount;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetDirect3D , IDirect3D9** , ppD3D9 )
	if (!m_pCreatedBy){
		return D3DERR_INVALIDCALL;
	}

	*ppD3D9 = m_pCreatedBy;
	m_pCreatedBy->AddRef();
	return D3D_OK;
}


// Calls SetCursorProperties() using the actual left surface from the proxy of pCursorBitmap.
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetCursorProperties , UINT , XHotSpot , UINT , YHotSpot , IDirect3DSurface9* , pCursorBitmap )
	if (!pCursorBitmap){
		return actual->SetCursorProperties(XHotSpot, YHotSpot, NULL);
	}
	return actual->SetCursorProperties(XHotSpot, YHotSpot, static_cast<D3D9ProxySurface*>(pCursorBitmap)->actual );
}


/**
* Creates a proxy (or wrapped) swap chain.
* @param pSwapChain [in, out] Proxy (wrapped) swap chain to be returned.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateAdditionalSwapChain , D3DPRESENT_PARAMETERS* , pPresentationParameters , IDirect3DSwapChain9** , pSwapChain )
	IDirect3DSwapChain9* pActualSwapChain;
	HRESULT result = actual->CreateAdditionalSwapChain(pPresentationParameters, &pActualSwapChain);

	if (SUCCEEDED(result)) {
		D3D9ProxySwapChain* wrappedSwapChain = new D3D9ProxySwapChain(pActualSwapChain, this, true);
		*pSwapChain = wrappedSwapChain;
		m_activeSwapChains.push_back(wrappedSwapChain);
	}

	return result;
}

/**
* Provides the swap chain from the intern vector of active proxy (wrapped) swap chains.
* @param pSwapChain [in, out] The proxy (wrapped) swap chain to be returned.
* @see D3D9ProxySwapChain
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetSwapChain , UINT , iSwapChain , IDirect3DSwapChain9** , pSwapChain )

	try {
		*pSwapChain = m_activeSwapChains.at(iSwapChain); 
		//Device->GetSwapChain increases ref count on the chain (docs don't say this)
		(*pSwapChain)->AddRef();
	}
	catch (std::out_of_range) {
		OutputDebugStringA("GetSwapChain: out of range fetching swap chain");
		return D3DERR_INVALIDCALL;
	}

	return D3D_OK;
}






/**
* Calls the backbuffer using the stored active proxy (wrapped) swap chain.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetBackBuffer , UINT , iSwapChain, UINT , iBackBuffer , D3DBACKBUFFER_TYPE , Type , IDirect3DSurface9** , ppBackBuffer )
	HRESULT result;
	try {
		result = m_activeSwapChains.at(iSwapChain)->GetBackBuffer(iBackBuffer, Type, ppBackBuffer);
		// ref count increase happens in the swapchain GetBackBuffer so we don't add another ref here as we are just passing the value through
	}
	catch (std::out_of_range) {
		OutputDebugStringA("GetBackBuffer: out of range getting swap chain");
		result = D3DERR_INVALIDCALL;
	}

	return result;
}


/**
* Creates a proxy (or wrapped) texture (D3DProxyTexture).
* Texture to be created only gets both stereo textures if game handler agrees.
* @see D3DProxyTexture
* @see GameHandler::ShouldDuplicateTexture()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateTexture , UINT , Width , UINT , Height , UINT , Levels , DWORD , Usage , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DTexture9** , ppTexture , HANDLE* , pSharedHandle )
	HRESULT creationResult;
	IDirect3DTexture9* pLeftTexture = NULL;
	IDirect3DTexture9* pRightTexture = NULL;	

	// try and create left
	if (SUCCEEDED(creationResult = actual->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &pLeftTexture, pSharedHandle))) {

		// Does this Texture need duplicating?
		if (m_pGameHandler->ShouldDuplicateTexture(Width, Height, Levels, Usage, Format, Pool)) {

			if (FAILED(actual->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &pRightTexture, pSharedHandle))) {
				OutputDebugStringA("Failed to create right eye texture while attempting to create stereo pair, falling back to mono\n");
				pRightTexture = NULL;
			}
		}
	}
	else {
		OutputDebugStringA("Failed to create texture\n"); 
	}

	if (SUCCEEDED(creationResult))
		*ppTexture = new D3D9ProxyTexture(pLeftTexture, pRightTexture, this);

	return creationResult;
}

/**
* Creates a a proxy (or wrapped) volume texture (D3D9ProxyVolumeTexture).
* Volumes can't be used as render targets and therefore don't need to be stereo (in DX9).
* @see D3D9ProxyVolumeTexture
***/	
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVolumeTexture , UINT , Width , UINT , Height , UINT , Depth , UINT , Levels , DWORD , Usage , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DVolumeTexture9** , ppVolumeTexture , HANDLE* , pSharedHandle )
	IDirect3DVolumeTexture9* pActualTexture = NULL;
	HRESULT creationResult = actual->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, &pActualTexture, pSharedHandle);

	if (SUCCEEDED(creationResult))
		*ppVolumeTexture = new D3D9ProxyVolumeTexture(pActualTexture, this);

	return creationResult;
}

/**
* Creates a proxy (or wrapped) cube texture (D3D9ProxyCubeTexture).
* Texture to be created only gets both stereo textures if game handler agrees.
* @see D3D9ProxyCubeTexture
* @see GameHandler::ShouldDuplicateCubeTexture() 
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateCubeTexture , UINT , EdgeLength , UINT , Levels , DWORD , Usage , D3DFORMAT , Format, D3DPOOL , Pool , IDirect3DCubeTexture9** , ppCubeTexture , HANDLE* , pSharedHandle )
	HRESULT creationResult;
	IDirect3DCubeTexture9* pLeftCubeTexture = NULL;
	IDirect3DCubeTexture9* pRightCubeTexture = NULL;	

	// try and create left
	if (SUCCEEDED(creationResult = actual->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, &pLeftCubeTexture, pSharedHandle))) {

		// Does this Texture need duplicating?
		if (m_pGameHandler->ShouldDuplicateCubeTexture(EdgeLength, Levels, Usage, Format, Pool)) {

			if (FAILED(actual->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, &pRightCubeTexture, pSharedHandle))) {
				OutputDebugStringA("Failed to create right eye texture while attempting to create stereo pair, falling back to mono\n");
				pRightCubeTexture = NULL;
			}
		}
	}
	else {
		OutputDebugStringA("Failed to create texture\n"); 
	}

	if (SUCCEEDED(creationResult))
		*ppCubeTexture = new D3D9ProxyCubeTexture(pLeftCubeTexture, pRightCubeTexture, this);

	return creationResult;
}

/**
* Creates base vertex buffer pointer (D3D9ProxyVertexBuffer).
* @see D3D9ProxyVertexBuffer
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVertexBuffer , UINT , Length , DWORD , Usage , DWORD , FVF , D3DPOOL , Pool, IDirect3DVertexBuffer9** , ppVertexBuffer , HANDLE* , pSharedHandle )
	IDirect3DVertexBuffer9* pActualBuffer = NULL;
	HRESULT creationResult = actual->CreateVertexBuffer(Length, Usage, FVF, Pool, &pActualBuffer, pSharedHandle);

	if (SUCCEEDED(creationResult))
		*ppVertexBuffer = new D3D9ProxyVertexBuffer(pActualBuffer, this);

	return creationResult;
}

/**
* * Creates base index buffer pointer (D3D9ProxyIndexBuffer).
* @see D3D9ProxyIndexBuffer
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateIndexBuffer , UINT , Length , DWORD , Usage , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DIndexBuffer9** , ppIndexBuffer , HANDLE* , pSharedHandle )
	IDirect3DIndexBuffer9* pActualBuffer = NULL;
	HRESULT creationResult = actual->CreateIndexBuffer(Length, Usage, Format, Pool, &pActualBuffer, pSharedHandle);

	if (SUCCEEDED(creationResult))
		*ppIndexBuffer = new D3D9ProxyIndexBuffer(pActualBuffer, this);

	return creationResult;
}


/**
* Copies rectangular subsets of pixels from one proxy (wrapped) surface to another.
* @see D3D9ProxySurface
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , UpdateSurface , IDirect3DSurface9* , pSourceSurface , CONST RECT* , pSourceRect , IDirect3DSurface9* , pDestinationSurface , CONST POINT* , pDestPoint )
	if (!pSourceSurface || !pDestinationSurface)
		return D3DERR_INVALIDCALL;

	IDirect3DSurface9* pSourceSurfaceLeft = static_cast<D3D9ProxySurface*>(pSourceSurface)->actual;
	IDirect3DSurface9* pSourceSurfaceRight = static_cast<D3D9ProxySurface*>(pSourceSurface)->right;
	IDirect3DSurface9* pDestSurfaceLeft = static_cast<D3D9ProxySurface*>(pDestinationSurface)->actual;
	IDirect3DSurface9* pDestSurfaceRight = static_cast<D3D9ProxySurface*>(pDestinationSurface)->right;

	HRESULT result = actual->UpdateSurface(pSourceSurfaceLeft, pSourceRect, pDestSurfaceLeft, pDestPoint);

	if (SUCCEEDED(result)) {
		if (!pSourceSurfaceRight && pDestSurfaceRight) {
			//OutputDebugStringA("INFO: UpdateSurface - Source is not stereo, destination is stereo. Copying source to both sides of destination.\n");

			if (FAILED(actual->UpdateSurface(pSourceSurfaceLeft, pSourceRect, pDestSurfaceRight, pDestPoint))) {
				OutputDebugStringA("ERROR: UpdateSurface - Failed to copy source left to destination right.\n");
			}
		} 
		else if (pSourceSurfaceRight && !pDestSurfaceRight) {
			//OutputDebugStringA("INFO: UpdateSurface - Source is stereo, destination is not stereo. Copied Left side only.\n");
		}
		else if (pSourceSurfaceRight && pDestSurfaceRight)	{
			if (FAILED(actual->UpdateSurface(pSourceSurfaceRight, pSourceRect, pDestSurfaceRight, pDestPoint))) {
				OutputDebugStringA("ERROR: UpdateSurface - Failed to copy source right to destination right.\n");
			}
		}
	}

	return result;
}

/**
* Calls a helper function to unwrap the textures and calls the super method for both sides.
* The super method updates the dirty portions of a texture.
* @see vireio::UnWrapTexture()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , UpdateTexture , IDirect3DBaseTexture9* , pSourceTexture , IDirect3DBaseTexture9* , pDestinationTexture )
	if (!pSourceTexture || !pDestinationTexture)
		return D3DERR_INVALIDCALL;


	IDirect3DBaseTexture9* pSourceTextureLeft = NULL;
	IDirect3DBaseTexture9* pSourceTextureRight = NULL;
	IDirect3DBaseTexture9* pDestTextureLeft = NULL;
	IDirect3DBaseTexture9* pDestTextureRight = NULL;

	vireio::UnWrapTexture(pSourceTexture, &pSourceTextureLeft, &pSourceTextureRight);
	vireio::UnWrapTexture(pDestinationTexture, &pDestTextureLeft, &pDestTextureRight);

	HRESULT result = actual->UpdateTexture(pSourceTextureLeft, pDestTextureLeft);

	if (SUCCEEDED(result)) {
		if (!pSourceTextureRight && pDestTextureRight) {
			//OutputDebugStringA("INFO: UpdateTexture - Source is not stereo, destination is stereo. Copying source to both sides of destination.\n");

			if (FAILED(actual->UpdateTexture(pSourceTextureLeft, pDestTextureRight))) {
				OutputDebugStringA("ERROR: UpdateTexture - Failed to copy source left to destination right.\n");
			}
		} 
		else if (pSourceTextureRight && !pDestTextureRight) {
			//OutputDebugStringA("INFO: UpdateTexture - Source is stereo, destination is not stereo. Copied Left side only.\n");
		}
		else if (pSourceTextureRight && pDestTextureRight)	{
			if (FAILED(actual->UpdateTexture(pSourceTextureRight, pDestTextureRight))) {
				OutputDebugStringA("ERROR: UpdateTexture - Failed to copy source right to destination right.\n");
			}
		}
	}

	return result;
}

/**
* Copies the render-target data from proxy (wrapped) source surface to proxy (wrapped) destination surface.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetRenderTargetData , IDirect3DSurface9* , pRenderTarget , IDirect3DSurface9* , pDestSurface )
	if ((pDestSurface == NULL) || (pRenderTarget == NULL))
		return D3DERR_INVALIDCALL;

	D3D9ProxySurface* pWrappedRenderTarget = static_cast<D3D9ProxySurface*>(pRenderTarget);
	D3D9ProxySurface* pWrappedDest = static_cast<D3D9ProxySurface*>(pDestSurface);

	IDirect3DSurface9* pRenderTargetLeft = pWrappedRenderTarget->actual;
	IDirect3DSurface9* pRenderTargetRight = pWrappedRenderTarget->right;
	IDirect3DSurface9* pDestSurfaceLeft = pWrappedDest->actual;
	IDirect3DSurface9* pDestSurfaceRight = pWrappedDest->right;

	HRESULT result = actual->GetRenderTargetData(pRenderTargetLeft, pDestSurfaceLeft);

	if (SUCCEEDED(result)) {
		if (!pRenderTargetRight && pDestSurfaceRight) {
			//OutputDebugStringA("INFO: GetRenderTargetData - Source is not stereo, destination is stereo. Copying source to both sides of destination.\n");

			if (FAILED(actual->GetRenderTargetData(pRenderTargetLeft, pDestSurfaceRight))) {
				OutputDebugStringA("ERROR: GetRenderTargetData - Failed to copy source left to destination right.\n");
			}
		} 
		else if (pRenderTargetRight && !pDestSurfaceRight) {
			//OutputDebugStringA("INFO: GetRenderTargetData - Source is stereo, destination is not stereo. Copied Left side only.\n");
		}
		else if (pRenderTargetRight && pDestSurfaceRight)	{
			if (FAILED(actual->GetRenderTargetData(pRenderTargetRight, pDestSurfaceRight))) {
				OutputDebugStringA("ERROR: GetRenderTargetData - Failed to copy source right to destination right.\n");
			}
		}
	}

	return result;
}

/**
* Gets the front buffer data from the internal stored active proxy (or wrapped) swap chain.
* @see D3D9ProxySwapChain
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetFrontBufferData , UINT , iSwapChain , IDirect3DSurface9* , pDestSurface )
	HRESULT result;
	try {
		result = m_activeSwapChains.at(iSwapChain)->GetFrontBufferData(pDestSurface);
	}
	catch (std::out_of_range) {
		OutputDebugStringA("GetFrontBufferData: out of range fetching swap chain");
		result = D3DERR_INVALIDCALL;
	}

	return result;
}

/**
* Copy the contents of the source proxy (wrapped) surface rectangles to the destination proxy (wrapped) surface rectangles.
* @see D3D9ProxySurface
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , StretchRect , IDirect3DSurface9* , pSourceSurface , CONST RECT* , pSourceRect , IDirect3DSurface9* , pDestSurface , CONST RECT* , pDestRect , D3DTEXTUREFILTERTYPE , Filter )
	if (!pSourceSurface || !pDestSurface)
		return D3DERR_INVALIDCALL;

	D3D9ProxySurface* pWrappedSource = static_cast<D3D9ProxySurface*>(pSourceSurface);
	D3D9ProxySurface* pWrappedDest = static_cast<D3D9ProxySurface*>(pDestSurface);

	IDirect3DSurface9* pSourceSurfaceLeft = pWrappedSource->actual;
	IDirect3DSurface9* pSourceSurfaceRight = pWrappedSource->right;
	IDirect3DSurface9* pDestSurfaceLeft = pWrappedDest->actual;
	IDirect3DSurface9* pDestSurfaceRight = pWrappedDest->right;

	HRESULT result = actual->StretchRect(pSourceSurfaceLeft, pSourceRect, pDestSurfaceLeft, pDestRect, Filter);

	if (SUCCEEDED(result)) {
		if (!pSourceSurfaceRight && pDestSurfaceRight) {
			//OutputDebugStringA("INFO: StretchRect - Source is not stereo, destination is stereo. Copying source to both sides of destination.\n");

			if (FAILED(actual->StretchRect(pSourceSurfaceLeft, pSourceRect, pDestSurfaceRight, pDestRect, Filter))) {
				OutputDebugStringA("ERROR: StretchRect - Failed to copy source left to destination right.\n");
			}
		} 
		else if (pSourceSurfaceRight && !pDestSurfaceRight) {
			//OutputDebugStringA("INFO: StretchRect - Source is stereo, destination is not stereo. Copied Left side only.\n");
		}
		else if (pSourceSurfaceRight && pDestSurfaceRight)	{
			if (FAILED(actual->StretchRect(pSourceSurfaceRight, pSourceRect, pDestSurfaceRight, pDestRect, Filter))) {
				OutputDebugStringA("ERROR: StretchRect - Failed to copy source right to destination right.\n");
			}
		}
	}

	return result;
}

/**
* Fills the rectangle for both stereo sides if switchDrawingSide() agrees and sets the render target accordingly.
* @see switchDrawingSide()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , ColorFill, IDirect3DSurface9* , pSurface , CONST RECT* , pRect , D3DCOLOR , color )
	HRESULT result;

	D3D9ProxySurface* pDerivedSurface = static_cast<D3D9ProxySurface*> (pSurface);
	if (SUCCEEDED(result = actual->ColorFill(pDerivedSurface->actual, pRect, color)))
	{
		if (pDerivedSurface->right){
			actual->ColorFill(pDerivedSurface->right, pRect, color);
		}
	}

	return result;
}

/**
* Updates render target accordingly to current render side.
* Updates proxy collection of stereo render targets to reflect new actual render target.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetRenderTarget , DWORD , RenderTargetIndex , IDirect3DSurface9* , pRenderTarget )
	D3D9ProxySurface* newRenderTarget = static_cast<D3D9ProxySurface*>(pRenderTarget);

#ifdef _DEBUG
	if (newRenderTarget && !newRenderTarget->actual && !newRenderTarget->right) {
		OutputDebugStringA("RenderTarget is not a valid (D3D9ProxySurface) stereo capable surface\n"); 
	}
#endif

	//// Update actual render target ////
	HRESULT result;

	// Removing a render target
	if (newRenderTarget == NULL) {
		if (RenderTargetIndex == 0) {
			// main render target should never be set to NULL
			result = D3DERR_INVALIDCALL; 
		}		
		else {
			result = actual->SetRenderTarget(RenderTargetIndex, NULL);
		}
	}
	// Setting a render target
	else {
		if (m_currentRenderingSide == vireio::Left) {
			result = actual->SetRenderTarget(RenderTargetIndex, newRenderTarget->actual);
		}
		else {
			result = actual->SetRenderTarget(RenderTargetIndex, newRenderTarget->right);
		}
	}

	//// update proxy collection of stereo render targets to reflect new actual render target ////
	if (result == D3D_OK) {		
		// changing rendertarget resets viewport to fullsurface
		m_bActiveViewportIsDefault = true;

		// release old render target
		if (m_activeRenderTargets[RenderTargetIndex] != NULL)
			m_activeRenderTargets[RenderTargetIndex]->Release();

		// replace with new render target (may be NULL)
		m_activeRenderTargets[RenderTargetIndex] = newRenderTarget;
		if (m_activeRenderTargets[RenderTargetIndex] != NULL)
			m_activeRenderTargets[RenderTargetIndex]->AddRef();
	}

	return result;
}

/**
* Provides render target from the internally stored active proxy (wrapped) render targets.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetRenderTarget , DWORD , RenderTargetIndex , IDirect3DSurface9** , ppRenderTarget )
	if ((RenderTargetIndex >= m_activeRenderTargets.capacity()) || (RenderTargetIndex < 0)) {
		return D3DERR_INVALIDCALL;
	}

	IDirect3DSurface9* targetToReturn = m_activeRenderTargets[RenderTargetIndex];
	if (!targetToReturn)
		return D3DERR_NOTFOUND;
	else {
		*ppRenderTarget = targetToReturn;
		targetToReturn->AddRef();
		return D3D_OK;
	}
}

/**
* Updates depth stencil accordingly to current render side.
* Updates stored proxy (or wrapped) depth stencil.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetDepthStencilSurface , IDirect3DSurface9* , pNewZStencil )
	D3D9ProxySurface* pNewDepthStencil = static_cast<D3D9ProxySurface*>(pNewZStencil);

	IDirect3DSurface9* pActualStencilForCurrentSide = NULL;
	if (pNewDepthStencil) {
		if (m_currentRenderingSide == vireio::Left)
			pActualStencilForCurrentSide = pNewDepthStencil->actual;
		else
			pActualStencilForCurrentSide = pNewDepthStencil->right;
	}

	// Update actual depth stencil
	HRESULT result = actual->SetDepthStencilSurface(pActualStencilForCurrentSide);

	// Update stored proxy depth stencil
	if (SUCCEEDED(result)) {
		if (m_pActiveStereoDepthStencil) {
			m_pActiveStereoDepthStencil->Release();
		}

		m_pActiveStereoDepthStencil = pNewDepthStencil;
		if (m_pActiveStereoDepthStencil) {
			m_pActiveStereoDepthStencil->AddRef();
		}
	}

	return result;
}

/**
* Provides the active proxy (wrapped) depth stencil.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetDepthStencilSurface , IDirect3DSurface9** , ppZStencilSurface )
	if (!m_pActiveStereoDepthStencil)
		return D3DERR_NOTFOUND;

	*ppZStencilSurface = m_pActiveStereoDepthStencil;
	(*ppZStencilSurface)->AddRef();

	return D3D_OK;
}

/**
* Updates tracker if device says it should.  Handles controls if this is the first scene of the frame.
* Because input for this frame would already have been handled here so injection of any mouse 
* manipulation ?
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , BeginScene )
	if (tracker){
		tracker->beginFrame();
	}

	if (m_isFirstBeginSceneOfFrame) {

		// save screenshot before first clear() is called
		if (screenshot>0)
		{
			if (screenshot==1)
				stereoView->SaveScreen();
			screenshot--;
		}

		// set last frame vertex shader count
		m_VertexShaderCountLastFrame = m_VertexShaderCount;

		// avoid squished viewport in case of brassa menu being drawn
		if( m_bViewportIsSquished && menu.show )
		{
			if (m_bViewportIsSquished)
				actual->SetViewport(&m_LastViewportSet);
			m_bViewportIsSquished = false;
		}

		// handle controls 
		if ((m_deviceBehavior.whenToHandleHeadTracking == DeviceBehavior::WhenToDo::BEGIN_SCENE) || (m_deviceBehavior.whenToHandleHeadTracking == DeviceBehavior::WhenToDo::FIRST_BEGIN_SCENE))
			HandleTracking();

		// draw BRASSA
		if ((m_deviceBehavior.whenToRenderBRASSA == DeviceBehavior::WhenToDo::BEGIN_SCENE) || (m_deviceBehavior.whenToRenderBRASSA == DeviceBehavior::WhenToDo::FIRST_BEGIN_SCENE))
		{
			menu.render();
		}

		// handle controls
		HandleControls();

		// set vertex shader call count to zero
		m_VertexShaderCount = 0;

		if( config.shaderAnalyzer ){
			for( cShader* s : shaders ){
				s->item->visible = (s->used || showUnusedShaders) && (s->vs || showPixelShaders);
				s->used          = false;
			}
		}
	}
	else
	{
		// handle controls 
		if (m_deviceBehavior.whenToHandleHeadTracking == DeviceBehavior::WhenToDo::BEGIN_SCENE)
			HandleTracking();

		// draw BRASSA
		if (m_deviceBehavior.whenToRenderBRASSA == DeviceBehavior::WhenToDo::BEGIN_SCENE)
		{
			menu.render();
		}
	}

	

	return actual->BeginScene();
}

/**
* BRASSA called here for source engine games.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , EndScene )
	// handle controls 
	if (m_deviceBehavior.whenToHandleHeadTracking == DeviceBehavior::WhenToDo::END_SCENE) 
		HandleTracking();

	// draw BRASSA
	if (m_deviceBehavior.whenToRenderBRASSA == DeviceBehavior::WhenToDo::END_SCENE) 
	{
		menu.render();
	}

	if( tracker ){
		tracker->endFrame();
	}

	return actual->EndScene();
}

/**
* Clears both stereo sides if switchDrawingSide() agrees.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , Clear , DWORD , Count , CONST D3DRECT* , pRects , DWORD , Flags , D3DCOLOR , Color , float , Z , DWORD , Stencil )
	HRESULT result;
	if (SUCCEEDED(result = actual->Clear(Count, pRects, Flags, Color, Z, Stencil))) {
		if (switchDrawingSide()) {

			HRESULT hr;
			if (FAILED(hr = actual->Clear(Count, pRects, Flags, Color, Z, Stencil))) {

#ifdef _DEBUG
				char buf[256];
				sprintf_s(buf, "Error: %s error description: %s\n",
				DXGetErrorString(hr), DXGetErrorDescription(hr));

				OutputDebugStringA(buf);
				OutputDebugStringA("Clear failed\n");

#endif

			}
		}
	}

	return result;
}

/**
* Catches transform for stored proxy state block accordingly or updates proxy device.
* @see D3D9ProxyStateBlock
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetTransform , D3DTRANSFORMSTATETYPE , State , CONST D3DMATRIX* , pMatrix )
	if(State == D3DTS_VIEW)
	{
		D3DXMATRIX tempLeft;
		D3DXMATRIX tempRight;
		D3DXMATRIX* pViewToSet = NULL;
		bool tempIsTransformSet = false;

		if (!pMatrix) {
			D3DXMatrixIdentity(&tempLeft);
			D3DXMatrixIdentity(&tempRight);
		}
		else {

			D3DXMATRIX sourceMatrix(*pMatrix);

			// If the view is set to the identity then we don't need to perform any adjustments
			if (D3DXMatrixIsIdentity(&sourceMatrix)) {

				D3DXMatrixIdentity(&tempLeft);
				D3DXMatrixIdentity(&tempRight);
			}
			else {
				// If the view matrix is modified we need to apply left/right adjustments (for stereo rendering)
				tempLeft = sourceMatrix * m_spShaderViewAdjustment->LeftViewTransform();
				tempRight = sourceMatrix * m_spShaderViewAdjustment->RightViewTransform();

				tempIsTransformSet = true;
			}
		}


		// If capturing state block capture without updating proxy device
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureViewTransform(tempLeft, tempRight);
			if (m_currentRenderingSide == vireio::Left) {
				pViewToSet = &tempLeft;
			}
			else {
				pViewToSet = &tempRight;
			}
		}
		else { // otherwise update proxy device

			m_bViewTransformSet = tempIsTransformSet;
			m_leftView = tempLeft;
			m_rightView = tempRight;

			if (m_currentRenderingSide == vireio::Left) {
				m_pCurrentView = &m_leftView;
			}
			else {
				m_pCurrentView = &m_rightView;
			}

			pViewToSet = m_pCurrentView;
		}

		return actual->SetTransform(State, pViewToSet);

	}
	else if(State == D3DTS_PROJECTION)
	{

		D3DXMATRIX tempLeft;
		D3DXMATRIX tempRight;
		D3DXMATRIX* pProjectionToSet = NULL;
		bool tempIsTransformSet = false;

		if (!pMatrix) {

			D3DXMatrixIdentity(&tempLeft);
			D3DXMatrixIdentity(&tempRight);
		}
		else {
			D3DXMATRIX sourceMatrix(*pMatrix);

			// If the view is set to the identity then we don't need to perform any adjustments
			if (D3DXMatrixIsIdentity(&sourceMatrix)) {

				D3DXMatrixIdentity(&tempLeft);
				D3DXMatrixIdentity(&tempRight);
			}
			else {


				tempLeft = sourceMatrix;
				tempRight = sourceMatrix;

				tempIsTransformSet = true;
			}			
		}

		// If capturing state block capture without updating proxy device
		if (m_pCapturingStateTo) {

			m_pCapturingStateTo->SelectAndCaptureProjectionTransform(tempLeft, tempRight);
			if (m_currentRenderingSide == vireio::Left) {
				pProjectionToSet = &tempLeft;
			}
			else {
				pProjectionToSet = &tempRight;
			}
		}
		else { // otherwise update proxy device

			m_bProjectionTransformSet = tempIsTransformSet;
			m_leftProjection = tempLeft;
			m_rightProjection = tempRight;

			if (m_currentRenderingSide == vireio::Left) {
				m_pCurrentProjection = &m_leftProjection;
			}
			else {
				m_pCurrentProjection = &m_rightProjection;
			}

			pProjectionToSet = m_pCurrentProjection;
		}

		return actual->SetTransform(State, pProjectionToSet);
	}

	return actual->SetTransform(State, pMatrix);
}





/**
* Try and set, if success save viewport.
* Also, it captures the viewport state in stored proxy state block.
* If viewport width and height match primary render target size and zmin is 0 and zmax 1 set 
* m_bActiveViewportIsDefault flag true.
* @see D3D9ProxyStateBlock::SelectAndCaptureState()
* @see m_bActiveViewportIsDefault
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetViewport , CONST D3DVIEWPORT9* , pViewport )	
	HRESULT result = actual->SetViewport(pViewport);

	if (SUCCEEDED(result)) {

		// If in a Begin-End StateBlock pair update the block state rather than the current proxy device state
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureState(*pViewport);
		}
		else {
			m_bActiveViewportIsDefault = isViewportDefaultForMainRT(pViewport);
			m_LastViewportSet = *pViewport;
		}
	}

	
	if (m_bViewportIsSquished)
		SetGUIViewport();
	
	return result;
}





/**
* Creates proxy state block.
* Also, selects capture type option according to state block type.
* @param ppSB [in, out] The proxy (or wrapped) state block returned.
* @see D3DProxyStateBlock
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateStateBlock , D3DSTATEBLOCKTYPE , Type , IDirect3DStateBlock9** , ppSB )
	IDirect3DStateBlock9* pActualStateBlock = NULL;
	HRESULT creationResult = actual->CreateStateBlock(Type, &pActualStateBlock);

	if (SUCCEEDED(creationResult)) {

		D3D9ProxyStateBlock::CaptureType capType;

		switch (Type) {
		case D3DSBT_ALL: 
			{
				capType = D3D9ProxyStateBlock::Cap_Type_Full;
				break;
			}

		case D3DSBT_PIXELSTATE: 
			{
				capType = D3D9ProxyStateBlock::Cap_Type_Pixel;
				break;
			}

		case D3DSBT_VERTEXSTATE: 
			{
				capType = D3D9ProxyStateBlock::Cap_Type_Vertex;
				break;
			}

		default:
			{
				capType = D3D9ProxyStateBlock::Cap_Type_Full;
				break;
			}    
		}

		*ppSB = new D3D9ProxyStateBlock(pActualStateBlock, this, capType, m_currentRenderingSide == vireio::Left);
	}

	return creationResult;
}

/**
* Creates and stores proxy state block.
* @see D3DProxyStateBlock
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , BeginStateBlock )
	HRESULT result;
	if (SUCCEEDED(result = actual->BeginStateBlock())) {
		m_bInBeginEndStateBlock = true;
		m_pCapturingStateTo = new D3D9ProxyStateBlock(NULL, this, D3D9ProxyStateBlock::Cap_Type_Selected, m_currentRenderingSide == vireio::Left);
	}

	return result;
}

/**
* Calls both super method and method from stored proxy state block.
* @param [in, out] The returned proxy (or wrapped) state block.
* @see D3D9ProxyStateBlock::EndStateBlock()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , EndStateBlock , IDirect3DStateBlock9** , ppSB )
	IDirect3DStateBlock9* pActualStateBlock = NULL;
	HRESULT creationResult = actual->EndStateBlock(&pActualStateBlock);

	if (SUCCEEDED(creationResult)) {
		m_pCapturingStateTo->EndStateBlock(pActualStateBlock);
		*ppSB = m_pCapturingStateTo;
	}
	else {
		m_pCapturingStateTo->Release();
		if (m_pCapturingStateTo) delete m_pCapturingStateTo;
	}

	m_pCapturingStateTo = NULL;
	m_bInBeginEndStateBlock = false;

	return creationResult;
}



/**
* Provides texture from stored active (mono) texture stages.
* @see D3D9ProxyTexture
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetTexture , DWORD , Stage , IDirect3DBaseTexture9** , ppTexture )
	if (m_activeTextureStages.count(Stage) != 1)
		return D3DERR_INVALIDCALL;
	else {
		*ppTexture = m_activeTextureStages[Stage];
		if ((*ppTexture))
			(*ppTexture)->AddRef();
		return D3D_OK;
	}
}

/**
* Calls a helper function to unwrap the textures and calls the super method for both sides.
* Update stored active (mono) texture stages if new texture was successfully set.
*
* @see vireio::UnWrapTexture() 
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetTexture , DWORD , Stage , IDirect3DBaseTexture9* , pTexture )
	HRESULT result;
	if (pTexture) {

		IDirect3DBaseTexture9* pActualLeftTexture = NULL;
		IDirect3DBaseTexture9* pActualRightTexture = NULL;

		vireio::UnWrapTexture(pTexture, &pActualLeftTexture, &pActualRightTexture);

		// Try and Update the actual devices textures
		if ((pActualRightTexture == NULL) || (m_currentRenderingSide == vireio::Left)) // use left (mono) if not stereo or one left side
			result = actual->SetTexture(Stage, pActualLeftTexture);
		else
			result = actual->SetTexture(Stage, pActualRightTexture);

	}
	else {
		result = actual->SetTexture(Stage, NULL);
	}

	// Update m_activeTextureStages if new texture was successfully set
	if (SUCCEEDED(result)) {

		// If in a Begin-End StateBlock pair update the block state rather than the current proxy device state
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureState(Stage, pTexture);
		}
		else {

			// remove existing texture that was active at Stage if there is one
			if (m_activeTextureStages.count(Stage) == 1) { 

				IDirect3DBaseTexture9* pOldTexture = m_activeTextureStages.at(Stage);
				if (pOldTexture)
					pOldTexture->Release();

				m_activeTextureStages.erase(Stage);
			}

			// insert new texture (can be a NULL pointer, this is important for StateBlock tracking)
			if(m_activeTextureStages.insert(std::pair<DWORD, IDirect3DBaseTexture9*>(Stage, pTexture)).second) {
				//success
				if (pTexture)
					pTexture->AddRef();
			}
			else {
				OutputDebugStringA(__FUNCTION__);
				OutputDebugStringA("\n");
				OutputDebugStringA("Unable to store active Texture Stage.\n");
				assert(false);

				//If we get here the state of the texture tracking is fubared and an implosion is imminent.

				result = D3DERR_INVALIDCALL;
			}
		}
	}

	return result;
}

/**
* Base GetTextureStageState functionality.
***/



/**
* Applies all dirty shader registers, draws both stereo sides if switchDrawingSide() agrees.
* @see switchDrawingSide()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawPrimitive , D3DPRIMITIVETYPE , PrimitiveType , UINT , StartVertex , UINT , PrimitiveCount )
	if( isDrawHide() ){
		return S_OK;
	}
	
	m_spManagedShaderRegisters->ApplyAllDirty(m_currentRenderingSide);

	HRESULT result;
	if (SUCCEEDED(result = actual->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount))) {
		if (switchDrawingSide())
			actual->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
	}

	return result;
}

/**
* Applies all dirty shader registers, draws both stereo sides if switchDrawingSide() agrees.
* @see switchDrawingSide()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawIndexedPrimitive , D3DPRIMITIVETYPE , PrimitiveType , INT , BaseVertexIndex , UINT , MinVertexIndex , UINT , NumVertices , UINT , startIndex , UINT , primCount )
	if( isDrawHide() ){
		return S_OK;
	}

	m_spManagedShaderRegisters->ApplyAllDirty(m_currentRenderingSide);

	HRESULT result;
	if (SUCCEEDED(result = actual->DrawIndexedPrimitive(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount))) {
		if (switchDrawingSide()) {
			HRESULT result2 = actual->DrawIndexedPrimitive(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
			if (result != result2)
				OutputDebugStringA("moop\n");
		}
	}

	return result;
}

/**
* Applies all dirty shader registers, draws both stereo sides if switchDrawingSide() agrees.
* @see switchDrawingSide()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawPrimitiveUP , D3DPRIMITIVETYPE , PrimitiveType , UINT , PrimitiveCount , CONST void* , pVertexStreamZeroData , UINT , VertexStreamZeroStride )
	if( isDrawHide() ){
		return S_OK;
	}

	m_spManagedShaderRegisters->ApplyAllDirty(m_currentRenderingSide);

	HRESULT result;
	if (SUCCEEDED(result = actual->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride))) {
		if (switchDrawingSide())
			actual->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
	}

	return result;
}

/**
* Applies all dirty shader registers, draws both stereo sides if switchDrawingSide() agrees.
* @see switchDrawingSide()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawIndexedPrimitiveUP , D3DPRIMITIVETYPE , PrimitiveType , UINT , MinVertexIndex , UINT , NumVertices , UINT , PrimitiveCount , CONST void* , pIndexData , D3DFORMAT , IndexDataFormat , CONST void* , pVertexStreamZeroData , UINT , VertexStreamZeroStride )
	if( isDrawHide() ){
		return S_OK;
	}

	m_spManagedShaderRegisters->ApplyAllDirty(m_currentRenderingSide);

	HRESULT result;
	if (SUCCEEDED(result = actual->DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride))) {
		if (switchDrawingSide())
			actual->DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
	}

	return result;
}

/**
* Applies all dirty shader registers, processes vertices.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , ProcessVertices , UINT , SrcStartIndex , UINT , DestIndex , UINT , VertexCount , IDirect3DVertexBuffer9* , pDestBuffer , IDirect3DVertexDeclaration9* , pVertexDecl , DWORD , Flags )
	if (!pDestBuffer)
		return D3DERR_INVALIDCALL;

	m_spManagedShaderRegisters->ApplyAllDirty(m_currentRenderingSide);

	D3D9ProxyVertexBuffer* pCastDestBuffer = static_cast<D3D9ProxyVertexBuffer*>(pDestBuffer);
	D3D9ProxyVertexDeclaration* pCastVertexDeclaration = NULL;

	HRESULT result;
	if (pVertexDecl) {
		pCastVertexDeclaration = static_cast<D3D9ProxyVertexDeclaration*>(pVertexDecl);
		result = actual->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pCastDestBuffer->actual, pCastVertexDeclaration->actual, Flags);
	}
	else {
		result = actual->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pCastDestBuffer->actual, NULL, Flags);
	}

	return result;
}

/**
* Creates base vertex declaration (D3D9ProxyVertexDeclaration).
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVertexDeclaration , CONST D3DVERTEXELEMENT9* , pVertexElements , IDirect3DVertexDeclaration9** , ppDecl )
	IDirect3DVertexDeclaration9* pActualVertexDeclaration = NULL;
	HRESULT creationResult = actual->CreateVertexDeclaration(pVertexElements, &pActualVertexDeclaration );

	if (SUCCEEDED(creationResult))
		*ppDecl = new D3D9ProxyVertexDeclaration(pActualVertexDeclaration, this);

	return creationResult;
}

/**
* Catches vertex declaration in stored proxy state block.
* First, set vertex declaration by base function.
* @see D3D9ProxyStateBlock
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetVertexDeclaration , IDirect3DVertexDeclaration9* , pDecl )
	D3D9ProxyVertexDeclaration* pWrappedVDeclarationData = static_cast<D3D9ProxyVertexDeclaration*>(pDecl);

	// Update actual Vertex Declaration
	HRESULT result;
	if (pWrappedVDeclarationData)
		result = actual->SetVertexDeclaration(pWrappedVDeclarationData->actual);
	else
		result = actual->SetVertexDeclaration(NULL);

	// Update stored proxy Vertex Declaration
	if (SUCCEEDED(result)) {

		// If in a Begin-End StateBlock pair update the block state rather than the current proxy device state
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureState(pWrappedVDeclarationData);
		}
		else {

			if (m_pActiveVertexDeclaration) {
				m_pActiveVertexDeclaration->Release();
			}

			m_pActiveVertexDeclaration = pWrappedVDeclarationData;
			if (m_pActiveVertexDeclaration) {
				m_pActiveVertexDeclaration->AddRef();
			}
		}
	}

	return result;
}

/**
* Provides currently stored vertex declaration.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetVertexDeclaration , IDirect3DVertexDeclaration9** , ppDecl )
	if (!m_pActiveVertexDeclaration) 
		// TODO check this is the response if no declaration set
		//In Response to TODO:  JB, Jan 12. I believe it crashes most times this happens, tested by simply nulling out the ppDecl pointer and passing it into the base d3d method
		return D3DERR_INVALIDCALL; 

	*ppDecl = m_pActiveVertexDeclaration;

	return D3D_OK;
}

/**
* Creates proxy (wrapped) vertex shader.
* @param ppShader [in, out] The created proxy vertex shader.
* @see D3D9ProxyVertexShader
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateVertexShader , CONST DWORD* , pFunction , IDirect3DVertexShader9** , ppShader )
	IDirect3DVertexShader9* pActualVShader = NULL;
	HRESULT creationResult = actual->CreateVertexShader(pFunction, &pActualVShader);

	if (SUCCEEDED(creationResult)) {
		D3D9ProxyVertexShader* shader = new D3D9ProxyVertexShader(pActualVShader, this, m_pGameHandler->GetShaderModificationRepository());
		*ppShader = shader;
		shaders += shader;
	}

	return creationResult;
}

/**
* Sets and updates stored proxy vertex shader.
* @see D3D9ProxyVertexShader
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetVertexShader , IDirect3DVertexShader9* , pShader )
	D3D9ProxyVertexShader* shader = static_cast<D3D9ProxyVertexShader*>(pShader);

	HRESULT result;

	if( shader ){
		result = actual->SetVertexShader( shader->actual );
	}else{
		result = actual->SetVertexShader(NULL);
	}

	// Update stored proxy Vertex shader
	if( SUCCEEDED(result) ){

		// If in a Begin-End StateBlock pair update the block state rather than the current proxy device state
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureState(shader);
		}else{
			if (currentVS) {
				currentVS->Release();
			}

			currentVS = shader;

			if (currentVS) {
				currentVS->AddRef();
			}

			m_spManagedShaderRegisters->ActiveVertexShaderChanged( currentVS );
		}
	}

	if( shader ){
		if( shader->m_bSquishViewport ){
			SetGUIViewport();
		}else{
			if( m_bViewportIsSquished ){
				actual->SetViewport(&m_LastViewportSet);
			}
			m_bViewportIsSquished = false;
		}

		if( config.shaderAnalyzer ){
			shader->used = true;

			if( shader->blink ){
				shader->hide = ((GetTickCount()%300)>150);
			}
		}
	}

	// increase vertex shader call count
	++m_VertexShaderCount;
	return result;
}

/**
* Returns the stored and active proxy vertex shader.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetVertexShader , IDirect3DVertexShader9** , ppShader )
	if (!currentVS)
		return D3DERR_INVALIDCALL;

	*ppShader = currentVS;

	return D3D_OK;
}

/**
* Sets shader constants either at stored proxy state block or in managed shader register class.
* @see D3D9ProxyStateBlock
* @see ShaderRegisters
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetVertexShaderConstantF , UINT , StartRegister , CONST float* , pConstantData , UINT , Vector4fCount )
	HRESULT result = D3DERR_INVALIDCALL;

	
	// Tests if the set constant is a transposed matrix and sets the relevant bool.
	// Is Matrix transposed ?
	// Affine transformation matrices have in the last row (0,0,0,1). World and view matrices are 
	// usually affine, since they are a combination of affine transformations (rotation, scale, 
	// translation ...).
	// Perspective projection matrices have in the last column (0,0,1,0) if left-handed and 
	// (0,0,-1,0) if right-handed.
	// Orthographic projection matrices have in the last column (0,0,0,1).
	// If those are transposed you find the entries in the last column/row.
	if( config.shaderAnalyzer && config.shaderAnalyzerDetectTranspose ){
		for( cShader* s : shaders ){
			for( cShaderConstant& c : s->constants ){
				if( s == currentVS ){
					if( c.RegisterIndex >= StartRegister  &&
						c.RegisterIndex < StartRegister + Vector4fCount
					){

						int i = 0;

						if( c.Class == D3DXPARAMETER_CLASS::D3DXPC_MATRIX_ROWS ){
							i = 14;
						}

						if( c.Class == D3DXPARAMETER_CLASS::D3DXPC_MATRIX_COLUMNS ){
							i = 12;
						}

						if( i ){
							D3DXMATRIX matrix = D3DXMATRIX( pConstantData + (c.RegisterIndex-StartRegister)*4*sizeof(float) );
					
							if( fabs( fabs(matrix[i]) - 1.0 ) > 0.00001 ){
								config.shaderAnalyzerTranspose = true;
							}
						}
					}
				}
			}
		}
	}


	if (m_pCapturingStateTo) {
		result = m_pCapturingStateTo->SelectAndCaptureStateVSConst(StartRegister, pConstantData, Vector4fCount);
	}
	else { 
		result = m_spManagedShaderRegisters->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	}

	return result;
}

/**
* Provides constant registers from managed shader register class.
* @see ShaderRegisters
* @see ShaderRegisters::GetVertexShaderConstantF()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetVertexShaderConstantF , UINT , StartRegister , float* , pData , UINT , Vector4fCount )
	return m_spManagedShaderRegisters->GetVertexShaderConstantF(StartRegister, pData, Vector4fCount);
}






/**
* Sets stream source and updates stored vertex buffers.
* Also, it calls proxy state block to capture states.
* @see D3D9ProxyStateBlock::SelectAndCaptureState()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetStreamSource , UINT , StreamNumber , IDirect3DVertexBuffer9* , pStreamData , UINT , OffsetInBytes , UINT , Stride )	
	D3D9ProxyVertexBuffer* pCastStreamData = static_cast<D3D9ProxyVertexBuffer*>(pStreamData);
	HRESULT result;
	if (pStreamData) {		
		result = actual->SetStreamSource(StreamNumber, pCastStreamData->actual, OffsetInBytes, Stride);
	}
	else {
		result = actual->SetStreamSource(StreamNumber, NULL, OffsetInBytes, Stride);
	}


	// Update m_activeVertexBuffers if new vertex buffer was successfully set
	if (SUCCEEDED(result)) {

		// If in a Begin-End StateBlock pair update the block state rather than the current proxy device state
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureState(StreamNumber, pCastStreamData);
		}
		else {
			// remove existing vertex buffer that was active at StreamNumber if there is one
			if (m_activeVertexBuffers.count(StreamNumber) == 1) { 

				IDirect3DVertexBuffer9* pOldBuffer = m_activeVertexBuffers.at(StreamNumber);
				if (pOldBuffer == pStreamData)
					return result;

				if (pOldBuffer)
					pOldBuffer->Release();

				m_activeVertexBuffers.erase(StreamNumber);
			}

			// insert new vertex buffer
			if(m_activeVertexBuffers.insert(std::pair<UINT, D3D9ProxyVertexBuffer*>(StreamNumber, pCastStreamData)).second) {
				//success
				if (pStreamData)
					pStreamData->AddRef();
			}
			else {
				OutputDebugStringA(__FUNCTION__);
				OutputDebugStringA("\n");
				OutputDebugStringA("Unable to store active Texture Stage.\n");
				assert(false);

				//If we get here the state of the texture tracking is fubared and an implosion is imminent.

				result = D3DERR_INVALIDCALL;
			}
		}
	}

	return result;
}

/**
* Provides stream data from stored vertex buffers.
* TODO ppStreamData is marked in and out in docs. Potentially it can be a get when the stream hasn't been set before???
* Category of probleme: Worry about it if it breaks.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetStreamSource , UINT , StreamNumber , IDirect3DVertexBuffer9** , ppStreamData , UINT* , pOffsetInBytes , UINT* , pStride )
	// This whole methods implementation is highly questionable. Not sure exactly how GetStreamSource works
	HRESULT result = D3DERR_INVALIDCALL;

	if (m_activeVertexBuffers.count(StreamNumber) == 1) {

		//IDirect3DVertexBuffer9* pCurrentActual = m_activeVertexBuffers[StreamNumber]->actual;

		//IDirect3DVertexBuffer9* pActualResultBuffer = NULL;
		//HRESULT result = actual->GetStreamSource(StreamNumber, &pCurrentActual, pOffsetInBytes, pStride);

		*ppStreamData = m_activeVertexBuffers[StreamNumber];
		if ((*ppStreamData))
			(*ppStreamData)->AddRef();

		result = D3D_OK;
	}
	return result;
}




/**
* Sets indices and calls proxy state block to capture states.
* @see D3D9ProxyStateBlock::SelectAndCaptureState()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetIndices , IDirect3DIndexBuffer9* , pIndexData )
	D3D9ProxyIndexBuffer* pWrappedNewIndexData = static_cast<D3D9ProxyIndexBuffer*>(pIndexData);

	// Update actual index buffer
	HRESULT result;
	if (pWrappedNewIndexData)
		result = actual->SetIndices(pWrappedNewIndexData->actual);
	else
		result = actual->SetIndices(NULL);

	if (SUCCEEDED(result)) {

		// If in a Begin-End StateBlock pair update the block state rather than the current proxy device state
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureState(pWrappedNewIndexData);
		}
		else {
			// Update stored proxy index buffer
			if (m_pActiveIndicies) {
				m_pActiveIndicies->Release();
			}

			m_pActiveIndicies = pWrappedNewIndexData;
			if (m_pActiveIndicies) {
				m_pActiveIndicies->AddRef();
			}
		}
	}

	return result;
}

/**
* Provides stored indices.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetIndices , IDirect3DIndexBuffer9** , ppIndexData )
	if (!m_pActiveIndicies)
		return D3DERR_INVALIDCALL;

	*ppIndexData = m_pActiveIndicies;
	m_pActiveIndicies->AddRef();

	return D3D_OK;
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreatePixelShader , CONST DWORD* , pFunction , IDirect3DPixelShader9** , ppShader )
	IDirect3DPixelShader9* pActualPShader = NULL;
	HRESULT creationResult = actual->CreatePixelShader(pFunction, &pActualPShader);

	if (SUCCEEDED(creationResult)) {
		D3D9ProxyPixelShader* shader = new D3D9ProxyPixelShader(pActualPShader, this, m_pGameHandler->GetShaderModificationRepository());
		*ppShader = shader;
		shaders += shader;
	}

	return creationResult;
}

/**
* Sets pixel shader and calls proxy state block to capture states.
* @see D3D9ProxyStateBlock::SelectAndCaptureState()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetPixelShader , IDirect3DPixelShader9* , pShader )
	D3D9ProxyPixelShader* shader = static_cast<D3D9ProxyPixelShader*>(pShader);

	// Update actual pixel shader
	HRESULT result;
	if( shader ){
		result = actual->SetPixelShader( shader->actual );
	}else{
		result = actual->SetPixelShader(NULL);
		return result;
	}

	// Update stored proxy pixel shader
	if( SUCCEEDED(result) ){

		// If in a Begin-End StateBlock pair update the block state rather than the current proxy device state
		if (m_pCapturingStateTo) {
			m_pCapturingStateTo->SelectAndCaptureState( shader );
		}else{

			if( currentPS ){
				currentPS->Release();
			}

			currentPS = shader;

			if( currentPS ) {
				currentPS->AddRef();
			}

			m_spManagedShaderRegisters->ActivePixelShaderChanged( shader );
		}
	}

	if( config.shaderAnalyzer && shader ){
		shader->used = true;

		if( shader->blink ){
			shader->hide = ((GetTickCount()%300)>150);
		}
	}

	return result;
}

/**
* Provides stored pixel shader.
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetPixelShader , IDirect3DPixelShader9** , ppShader )
	if (!currentPS)
		return D3DERR_INVALIDCALL;

	*ppShader = currentPS;

	return D3D_OK;
}

/**
* Sets shader constants either at stored proxy state block or in managed shader register class.
* @see D3D9ProxyStateBlock
* @see ShaderRegisters
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , SetPixelShaderConstantF , UINT , StartRegister , CONST float* , pConstantData , UINT , Vector4fCount )
	HRESULT result = D3DERR_INVALIDCALL;

	if (m_pCapturingStateTo) {
		result = m_pCapturingStateTo->SelectAndCaptureStatePSConst(StartRegister, pConstantData, Vector4fCount);
	}
	else { 
		result = m_spManagedShaderRegisters->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	}

	return result;
}

/**
* Provides constant registers from managed shader register class.
* @see ShaderRegisters
* @see ShaderRegisters::GetPixelShaderConstantF()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , GetPixelShaderConstantF , UINT , StartRegister , float* , pData , UINT , Vector4fCount )
	return m_spManagedShaderRegisters->GetPixelShaderConstantF(StartRegister, pData, Vector4fCount);
}




/**
* Applies all dirty registers, draws both stereo sides if switchDrawingSide() agrees.
* @see switchDrawingSide()
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawRectPatch , UINT , Handle , CONST float* , pNumSegs , CONST D3DRECTPATCH_INFO* , pRectPatchInfo )
	m_spManagedShaderRegisters->ApplyAllDirty(m_currentRenderingSide);

	HRESULT result;
	if (SUCCEEDED(result = actual->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo))) {
		if (switchDrawingSide())
			actual->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
	}

	return result;
}

/**
* Applies all dirty registers, draws both stereo sides if switchDrawingSide() agrees.
* @see switchDrawingSide() 
***/
METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , DrawTriPatch , UINT , Handle , CONST float* , pNumSegs , CONST D3DTRIPATCH_INFO* , pTriPatchInfo )
	m_spManagedShaderRegisters->ApplyAllDirty(m_currentRenderingSide);

	HRESULT result;
	if (SUCCEEDED(result = actual->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo))) {
		if (switchDrawingSide())
			actual->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
	}

	return result;
}




METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateQuery , D3DQUERYTYPE , Type , IDirect3DQuery9** , ppQuery )
	// this seems a weird response to me but it's what the actual device does.
	if (!ppQuery)
		return D3D_OK;

	IDirect3DQuery9* pActualQuery = NULL;
	HRESULT creationResult = actual->CreateQuery(Type, &pActualQuery);

	if (SUCCEEDED(creationResult)) {
		*ppQuery = new D3D9ProxyQuery(pActualQuery, this);
	}

	return creationResult;
}


/**
* Returns the actual embedded device pointer.
***/
IDirect3DDevice9* D3DProxyDevice::getActual()
{
	return actual;
}


METHOD_IMPL( void , , D3DProxyDevice, HandleControls )
	controls.UpdateXInputs();
}


/**
* Updates selected motion tracker orientation.
***/
METHOD_IMPL( void , , D3DProxyDevice , HandleTracking )
	if( !tracker ){
 		// VRboost rules present ?
 		m_VRboostRulesPresent = !config.VRboostRule.isEmpty();


		switch(config.trackerMode){
		case 10:
			tracker = Vireio_Create_Tracker_FreeSpace();
			break;

		case 20:
			tracker = Vireio_Create_Tracker_FreeTrack();
			break;

		case 30:
			tracker = Vireio_Create_Tracker_SharedMemory();
			break;

		case 40:
			tracker = Vireio_Create_Tracker_Oculus();
			break;

		}


		if( tracker ){
			if( !tracker->open() ){
				menu.showMessage("Tracker error: unable to open device");
				printf( "tracker: open failed\n" );
				tracker->close();
				delete tracker;
				tracker =0 ;
			}
		}

		if( tracker ){
			//Only advise calibration for positional tracking on DK2
			//if (tracker->SupportsPositionTracking()){
			//	calibrate_tracker = true;
				//TODO: add new tracker interface
			//}
 		}
	}

	if( !tracker ){
		return;
	}


	long prevYaw   = RADIANS_TO_DEGREES( tracker->currentYaw   ) * config.trackerMouseYawMultiplier;
	long prevPitch = RADIANS_TO_DEGREES( tracker->currentPitch ) * config.trackerMousePitchMultiplier;

	if( tracker->update() ){
		tracker->currentYaw   *= config.trackerYawMultiplier;
		tracker->currentPitch *= config.trackerPitchMultiplier;
		tracker->currentRoll  *= config.trackerRollMultiplier;

		//multiplied in m_spShaderViewAdjustment->UpdatePosition
		//tracker->currentX     *= config.trackerPositionMultiplier;
		//tracker->currentY     *= config.trackerPositionMultiplier;
		//tracker->currentZ     *= config.trackerPositionMultiplier;

		if( config.trackerMouseEmulation ){
			long currentYaw   = RADIANS_TO_DEGREES(tracker->currentYaw  ) * config.trackerMouseYawMultiplier;
			long currentPitch = RADIANS_TO_DEGREES(tracker->currentPitch) * config.trackerMousePitchMultiplier;

			long deltaYaw   = currentYaw   - prevYaw;
			long deltaPitch = currentPitch - prevPitch;

			deltaYaw   = ((deltaYaw   + 180) % 360) - 180;
			deltaPitch = ((deltaPitch + 180) % 360) - 180;

			if( abs(deltaYaw) > 100 ){
				deltaYaw = 0;
			}

			if( abs(deltaPitch) > 100 ){
				deltaPitch = 0;
			}
			
			INPUT i;
			i.type           = INPUT_MOUSE;
			i.mi.dx          = deltaYaw;
			i.mi.dy          = deltaPitch;
			i.mi.mouseData   = 0;
			i.mi.dwFlags     = MOUSEEVENTF_MOVE;
			i.mi.time        = 0;
			i.mi.dwExtraInfo = 0;

			SendInput( 1 , &i , sizeof(i) );
		}
	}
	
	if( calibrate_tracker ){
		menu.showMessage(
			"Please Calibrate HMD/Tracker:\n"
			"     -  Sit comfortably with your head facing forwards\n"
			"     -  Go to tracker configuration in main menu\n"
			"     -  Reset view \n"
			"     -  Adjust multipliers to comfortable values\n"
		);
	}

	if ( config.rollEnabled ) {
		m_spShaderViewAdjustment->UpdateRoll(tracker->currentRoll);
	}

	if( config.trackerMouseEmulation ){
		return;
	}

	if( config.trackerRotationEnable ){
		m_spShaderViewAdjustment->UpdatePitchYaw( tracker->currentPitch , tracker->currentYaw );
	}else{
		m_spShaderViewAdjustment->UpdatePitchYaw( 0 , 0 );
	}

	if( config.trackerPositionEnable ){
		m_spShaderViewAdjustment->UpdatePosition(
			tracker->currentYaw,
			tracker->currentPitch,
			tracker->currentRoll,
			(VRBoostValue[VRboostAxis::CameraTranslateX] / 20.0f) + tracker->currentX * config.trackerXMultiplier * config.stereoScale, 
			(VRBoostValue[VRboostAxis::CameraTranslateY] / 20.0f) + tracker->currentY * config.trackerYMultiplier * config.stereoScale,
			(VRBoostValue[VRboostAxis::CameraTranslateZ] / 20.0f) + tracker->currentZ * config.trackerZMultiplier * config.stereoScale
		);
	}
		
	m_spShaderViewAdjustment->ComputeViewTransforms();

	m_isFirstBeginSceneOfFrame = false;

	// update vrboost, if present, tracker available and shader count higher than the minimum
	if( hmVRboost && m_VRboostRulesPresent && m_bVRBoostToggle
		&& (m_VertexShaderCountLastFrame>(UINT)config.VRboostMinShaderCount)
		&& (m_VertexShaderCountLastFrame<(UINT)config.VRboostMaxShaderCount) )
	{
		VRBoostStatus.VRBoost_Active = true;
		// development bool
		bool createNSave = false;

		// apply VRboost memory rules if present
		VRBoostValue[VRboostAxis::TrackerYaw]   = tracker->currentYaw;
		VRBoostValue[VRboostAxis::TrackerPitch] = tracker->currentPitch;
		VRBoostValue[VRboostAxis::TrackerRoll]  = tracker->currentRoll;

		if (m_pVRboost_ApplyMemoryRules(MAX_VRBOOST_VALUES, (float**)&VRBoostValue) != S_OK)
		{
			VRBoostStatus.VRBoost_ApplyRules = false;
			if (!createNSave)
			{
				// load VRboost rules
				if (config.VRboostRule != "")
				{
					//OutputDebugStringA(std::string("config.VRboostPath: " + config.VRboostPath).c_str());
					if (m_pVRboost_LoadMemoryRules(config.exeName.toStdString() , config.getVRBoostRuleFilePath().toStdString() ) != S_OK)
						VRBoostStatus.VRBoost_LoadRules = false;
					else
						VRBoostStatus.VRBoost_LoadRules = true;
				}
			}
		}
		else
		{
			VRBoostStatus.VRBoost_ApplyRules = true;
		}
	}
	else
	{
		VRBoostStatus.VRBoost_Active = false;
	}

	if( VRBoostStatus.VRBoost_Active ){
		if( !VRBoostStatus.VRBoost_LoadRules ){
			menu.showMessage(
				"VRBoost LoadRules Failed!\n"
				"To Enable head tracking, turn on tracker mouse emulation\n"
				"in tracker configuration menu"
			);
		}else
		if( !VRBoostStatus.VRBoost_ApplyRules ){
			menu.showMessage(
				"VRBoost rules loaded but could not be applied\n"
				"To Enable head tracking, turn on tracker mouse emulation\n"
				"in tracker configuration menu"
			);
		}
	}
}

/**
* Handles all updates if Present() is called in an extern swap chain.
***/
METHOD_IMPL( void , , D3DProxyDevice , HandleUpdateExtern )
	#ifdef SHOW_CALLS
		OutputDebugStringA("called HandleUpdateExtern");
	#endif
	m_isFirstBeginSceneOfFrame = true;

	BRASSA_UpdateBorder();
}

/**
* Creates or restores class setup.
* Subclasses which override this method must call through to super method.
* Do not directly call this method in subclasses.
* This method should be used to re/create any resources that are held by the device proxy and deleted by Reset.
* 
* The only resources used like this are going to be extra resources that are used by the proxy and are not
* part of the actual calling application. 
* 
* Examples in D3DProxyDevice: The Font used in the BRASSA overlay and the stereo buffer.
* 
* Example of something you wouldn't create here:
* Render targets in the m_activeRenderTargets collection. They need to be released to successfully Reset
* the device, but they just wrap IDirect3DSurface9 objects from the underlying application and will be
* re/created by the underlying application.
* 
* This method will be called when the proxy device is initialised with Init (happens before device is
* passed back to actual application by CreateDevice) and after a successful device Reset.
***/
METHOD_IMPL( void , , D3DProxyDevice , OnCreateOrRestore )	
	m_currentRenderingSide = vireio::Left;
	m_pCurrentMatViewTransform = &m_spShaderViewAdjustment->LeftAdjustmentMatrix();
	m_pCurrentView = &m_leftView;
	m_pCurrentProjection = &m_leftProjection;

	// Wrap the swap chain
	IDirect3DSwapChain9* pActualPrimarySwapChain;
	if (FAILED(actual->GetSwapChain(0, &pActualPrimarySwapChain))) {
		OutputDebugStringA("Failed to fetch swapchain.\n");
		exit(1); 
	}

	assert (m_activeSwapChains.size() == 0);
	m_activeSwapChains.push_back(new D3D9ProxySwapChain(pActualPrimarySwapChain, this, false));
	assert (m_activeSwapChains.size() == 1);

	// Set the primary rendertarget to the first stereo backbuffer
	IDirect3DSurface9* pWrappedBackBuffer;
	m_activeSwapChains[0]->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pWrappedBackBuffer);
	SetRenderTarget(0, pWrappedBackBuffer);
	pWrappedBackBuffer->Release();
	pWrappedBackBuffer = NULL;


	actual->GetViewport(&m_LastViewportSet);

	// If there is an initial depth stencil
	IDirect3DSurface9* pDepthStencil;
	if (SUCCEEDED(actual->GetDepthStencilSurface(&pDepthStencil))) { 

		D3DSURFACE_DESC stencilDesc;
		pDepthStencil->GetDesc(&stencilDesc);
		pDepthStencil->Release();

		IDirect3DSurface9* pTemp = NULL;
		CreateDepthStencilSurface(stencilDesc.Width, stencilDesc.Height, stencilDesc.Format, stencilDesc.MultiSampleType, stencilDesc.MultiSampleQuality, false, &pTemp, NULL);
		SetDepthStencilSurface(pTemp);
		pTemp->Release();	
	}

	menu.createResources();

	stereoView->Init(getActual());

	m_spShaderViewAdjustment->UpdateProjectionMatrices((float)stereoView->viewport.Width/(float)stereoView->viewport.Height);
	m_spShaderViewAdjustment->ComputeViewTransforms();

	// set BRASSA main values
	menu.viewportWidth  = stereoView->viewport.Width;
	menu.viewportHeight = stereoView->viewport.Height;
}

/**
* Switches rendering to which ever side is specified by side.
* Use to specify the side that you want to draw to.
* Overriding classes should call the base implementation first and then makes any extra needed changes
* based on the result of the base implementation (if the base class doesn't change side then derived shouldn't 
* change either)
* 
* @return True if change succeeded, false if it fails. The switch will fail if you attempt to setDrawingSide(Right)
* when the current primary active render target (target 0  in m_activeRenderTargets) is not stereo.
* Attempting to switch to a side when that side is already the active side will return true without making any changes.
***/
METHOD_IMPL( bool , , D3DProxyDevice , setDrawingSide , vireio::RenderPosition , side )
	// Already on the correct eye
	if (side == m_currentRenderingSide) {
		return true;
	}

	// should never try and render for the right eye if there is no render target for the main render targets right side
	if (!m_activeRenderTargets[0]->right && (side == vireio::Right)) {
		return false;
	}

	// Everything hasn't changed yet but we set this first so we don't accidentally use the member instead of the local and break
	// things, as I have already managed twice.
	m_currentRenderingSide = side;

	// switch render targets to new side
	bool renderTargetChanged = false;
	HRESULT result;
	D3D9ProxySurface* pCurrentRT;
	for(std::vector<D3D9ProxySurface*>::size_type i = 0; i != m_activeRenderTargets.size(); i++) 
	{
		if ((pCurrentRT = m_activeRenderTargets[i]) != NULL) {

			if (side == vireio::Left) 
				result = actual->SetRenderTarget(i, pCurrentRT->actual); 
			else 
				result = actual->SetRenderTarget(i, pCurrentRT->right);

			if (result != D3D_OK) {
				OutputDebugStringA("Error trying to set one of the Render Targets while switching between active eyes for drawing.\n");
			}
			else {
				renderTargetChanged = true;
			}
		}
	}

	// if a non-fullsurface viewport is active and a rendertarget changed we need to reapply the viewport
	if (renderTargetChanged && !m_bActiveViewportIsDefault) {
		actual->SetViewport(&m_LastViewportSet);
	}

	if (m_bViewportIsSquished)
		SetGUIViewport();

	// switch depth stencil to new side
	if (m_pActiveStereoDepthStencil != NULL) { 
		if (side == vireio::Left) 
			result = actual->SetDepthStencilSurface(m_pActiveStereoDepthStencil->actual); 
		else 
			result = actual->SetDepthStencilSurface(m_pActiveStereoDepthStencil->right);
	}

	// switch textures to new side
	IDirect3DBaseTexture9* pActualLeftTexture = NULL;
	IDirect3DBaseTexture9* pActualRightTexture = NULL;

	for(auto it = m_activeTextureStages.begin(); it != m_activeTextureStages.end(); ++it )
	{
		if (it->second) {
			pActualLeftTexture = NULL;
			pActualRightTexture = NULL;
			vireio::UnWrapTexture(it->second, &pActualLeftTexture, &pActualRightTexture);

			// if stereo texture
			if (pActualRightTexture != NULL) { 
				if (side == vireio::Left) 
					result = actual->SetTexture(it->first, pActualLeftTexture); 
				else 
					result = actual->SetTexture(it->first, pActualRightTexture);
			}
			// else the texture is mono and doesn't need changing. It will always be set initially and then won't need changing

			if (result != D3D_OK)
				OutputDebugStringA("Error trying to set one of the textures while switching between active eyes for drawing.\n");
		}
	}

	// update view transform for new side 
	if (m_bViewTransformSet) {

		if (side == vireio::Left) {
			m_pCurrentView = &m_leftView;
		}
		else {
			m_pCurrentView = &m_rightView;
		}

		actual->SetTransform(D3DTS_VIEW, m_pCurrentView);
	}

	// update projection transform for new side 
	if (m_bProjectionTransformSet) {

		if (side == vireio::Left) {
			m_pCurrentProjection = &m_leftProjection;
		}
		else {
			m_pCurrentProjection = &m_rightProjection;
		}

		actual->SetTransform(D3DTS_PROJECTION, m_pCurrentProjection);
	}

	// Updated computed view translation (used by several derived proxies - see: ComputeViewTranslation)
	if (side == vireio::Left) {
		m_pCurrentMatViewTransform = &m_spShaderViewAdjustment->LeftAdjustmentMatrix();
	}
	else {
		m_pCurrentMatViewTransform = &m_spShaderViewAdjustment->RightAdjustmentMatrix();
	}

	// Apply active stereo shader constants
	m_spManagedShaderRegisters->ApplyAllStereoConstants(side);

	return true;
}

/**
* Try and toggle to other drawing side. 
* @return False if changes fails due to the current render target being mono.
***/
METHOD_IMPL( bool , , D3DProxyDevice , switchDrawingSide )
	bool switched = false;

	if (m_currentRenderingSide == vireio::Left) {
		switched = setDrawingSide(vireio::Right);
	}
	else if (m_currentRenderingSide == vireio::Right) {
		switched = setDrawingSide(vireio::Left);
	}
	else {
		DebugBreak();
	}

	return switched;
}

/**
* Adds a default shader rule to the game configuration.
* @return True if rule was added, false if rule already present.
***/
METHOD_IMPL( bool , , D3DProxyDevice , addRule , std::string , constantName , bool , allowPartialNameMatch , UINT , startRegIndex , D3DXPARAMETER_CLASS , constantType , UINT , operationToApply , bool , transpose )
	return m_pGameHandler->AddRule(m_spShaderViewAdjustment, constantName, allowPartialNameMatch, startRegIndex, constantType, operationToApply, transpose);
}

/**
* Adds a default shader rule to the game configuration.
* @return True if rule was added, false if rule already present.
***/
METHOD_IMPL( bool , , D3DProxyDevice , modifyRule , std::string , constantName , UINT , operationToApply , bool , transpose )
	return m_pGameHandler->ModifyRule(m_spShaderViewAdjustment, constantName, operationToApply, transpose);
}

/**
* Delete rule.
* @return True if rule was deleted, false if rule not present.
***/
METHOD_IMPL( bool , , D3DProxyDevice , deleteRule , std::string , constantName )
	return m_pGameHandler->DeleteRule(m_spShaderViewAdjustment, constantName);
}

/**
* Saves current game shader rules (and game configuration).
***/
METHOD_IMPL( void , , D3DProxyDevice , saveShaderRules ) 
	m_pGameHandler->Save(config, m_spShaderViewAdjustment);

	SaveConfiguration();
}

/**
* Simple helper to clear a rectangle using the specified color.
* @param renderPosition Left or Right render target to be used.
* @param rect The rectangle in pixel space to be cleared.
* @param color The direct 3d color to be used.
***/
METHOD_IMPL( void , , D3DProxyDevice , ClearRect , vireio::RenderPosition , renderPosition , D3DRECT , rect , D3DCOLOR , color )
	setDrawingSide(renderPosition);
	actual->Clear(1, &rect, D3DCLEAR_TARGET, color, 0, 0);
}

/**
* Simple helper to clear an empty rectangle or border using the specified color.
* @param renderPosition Left or Right render target to be used.
* @param rect The rectangle in pixel space to be cleared.
* @param color The direct 3d color to be used.
* @param bw The border width.
***/
METHOD_IMPL( void , , D3DProxyDevice , ClearEmptyRect , vireio::RenderPosition , renderPosition , D3DRECT , rect , D3DCOLOR , color, int , bw )
	// helper rectangle
	D3DRECT rect0 = D3DRECT(rect);

	setDrawingSide(renderPosition);

	rect0.y2 = rect.y1 + bw;
	actual->Clear(1, &rect0, D3DCLEAR_TARGET, color, 0, 0);

	rect0.y1 = rect.y2 - bw;
	rect0.y2 = rect.y2;
	actual->Clear(1, &rect0, D3DCLEAR_TARGET, color, 0, 0);

	rect0.y1 = rect.y1;
	rect0.x2 = rect.x1 + bw;
	actual->Clear(1, &rect0, D3DCLEAR_TARGET, color, 0, 0);

	rect0.x1 = rect.x2 - bw;
	rect0.x2 = rect.x2;
	actual->Clear(1, &rect0, D3DCLEAR_TARGET, color, 0, 0);
}

/**
* Draws a simple selection control.
* @param renderPosition Left or Right render target to be used.
* @param rect The rectangle in pixel space to be cleared.
* @param color The direct 3d color to be used.
* @param selectionIndex The index of the currently chosen selection.
* @param selectionRange The range of the selection.
***/
METHOD_IMPL( void , , D3DProxyDevice , DrawSelection , vireio::RenderPosition , renderPosition , D3DRECT , rect , D3DCOLOR , color , int , selectionIndex , int , selectionRange )	
	// get width of each selection
	float selectionWidth = (rect.x2-rect.x1) / (float)selectionRange;

	// get secondary color
	D3DXCOLOR color2 = D3DXCOLOR(color);
	FLOAT red = color2.r;
	color2.r = color2.g * 0.7f;
	color2.g = red;

	for (int i = 0; i < selectionRange; i++)
	{
		rect.x2 = rect.x1+(int)selectionWidth;
		if (i==selectionIndex)
			ClearRect(renderPosition, rect, color);
		else
			ClearRect(renderPosition, rect, color2);
		rect.x1+=(int)selectionWidth;
	}
}

/**
* Draws a simple selection control.
* @param renderPosition Left or Right render target to be used.
* @param rect The rectangle in pixel space to be cleared.
* @param color The direct 3d color to be used.
* @param selectionIndex The index of the currently chosen selection.
* @param selectionRange The range of the selection.
***/
METHOD_IMPL( void , , D3DProxyDevice , DrawScrollbar , vireio::RenderPosition , renderPosition , D3DRECT , rect , D3DCOLOR , color , float , scroll , int , scrollbarSize )	
	if (scroll<0.0f) scroll=0.0f;
	if (scroll>1.0f) scroll=1.0f;

	// get width of each selection
	int scrollHeight = rect.y2-rect.y1-scrollbarSize;
	scrollHeight = (int)(scrollHeight*scroll);

	// get secondary color
	D3DXCOLOR color2 = D3DXCOLOR(color);
	FLOAT red = color2.r;
	color2.r = color2.g * 0.7f;
	color2.g = red;

	ClearRect(renderPosition, rect, color2);
	rect.y1 += scrollHeight;
	rect.y2 = rect.y1+scrollbarSize;
	ClearRect(renderPosition, rect, color);
}

/**
* Draws a text with a dark shadow.
* @see DrawText()
***/
METHOD_IMPL( void , , D3DProxyDevice , DrawTextShadowed , ID3DXFont* , font , LPD3DXSPRITE , sprite , LPCSTR , lpchText , int , cchText , LPRECT , lprc , UINT , format, D3DCOLOR , color )
	lprc->left+=2; lprc->right+=2; lprc->top+=2; lprc->bottom+=2;
	font->DrawTextA(sprite, lpchText, -1, lprc, format, D3DCOLOR_ARGB(255, 64, 64, 64));
	lprc->left-=2; lprc->right-=2; lprc->top-=2; lprc->bottom-=2;
	font->DrawTextA(sprite, lpchText, -1, lprc, format, color);
}





/**
* BRASSA menu border velocity updated here
* Arrow up/down need to be done via call from Present().
***/
METHOD_IMPL( void , , D3DProxyDevice , BRASSA_UpdateBorder )
	// handle controls 
	if (m_deviceBehavior.whenToHandleHeadTracking == DeviceBehavior::PRESENT)
		HandleTracking();

	// draw BRASSA
	if (m_deviceBehavior.whenToRenderBRASSA == DeviceBehavior::PRESENT)
	{
		menu.render();
	}
}

/**
* Updates the current config based on the current device settings.
***/
METHOD_IMPL( void , , D3DProxyDevice , BRASSA_UpdateConfigSettings )
	config.WorldFOV = VRBoostValue[VRboostAxis::WorldFOV];
	config.PlayerFOV = VRBoostValue[VRboostAxis::PlayerFOV];
	config.FarPlaneFOV = VRBoostValue[VRboostAxis::FarPlaneFOV];
	config.CameraTranslateX = VRBoostValue[VRboostAxis::CameraTranslateX];
	config.CameraTranslateY = VRBoostValue[VRboostAxis::CameraTranslateY];
	config.CameraTranslateZ = VRBoostValue[VRboostAxis::CameraTranslateZ];
	config.CameraDistance = VRBoostValue[VRboostAxis::CameraDistance];
	config.CameraZoom = VRBoostValue[VRboostAxis::CameraZoom];
	config.CameraHorizonAdjustment = VRBoostValue[VRboostAxis::CameraHorizonAdjustment];
	config.ConstantValue1 = VRBoostValue[VRboostAxis::ConstantValue1];
	config.ConstantValue2 = VRBoostValue[VRboostAxis::ConstantValue2];
	config.ConstantValue3 = VRBoostValue[VRboostAxis::ConstantValue3];

}

/**
* Updates all device settings read from the current config.
***/
METHOD_IMPL( void , , D3DProxyDevice , BRASSA_UpdateDeviceSettings )

	m_spShaderViewAdjustment->ComputeViewTransforms();

	// VRBoost
	VRBoostValue[VRboostAxis::WorldFOV] = config.WorldFOV;
	VRBoostValue[VRboostAxis::PlayerFOV] = config.PlayerFOV;
	VRBoostValue[VRboostAxis::FarPlaneFOV] = config.FarPlaneFOV;
	VRBoostValue[VRboostAxis::CameraTranslateX] = config.CameraTranslateX;
	VRBoostValue[VRboostAxis::CameraTranslateY] = config.CameraTranslateY;
	VRBoostValue[VRboostAxis::CameraTranslateZ] = config.CameraTranslateZ;
	VRBoostValue[VRboostAxis::CameraDistance] = config.CameraDistance;
	VRBoostValue[VRboostAxis::CameraZoom] = config.CameraZoom;
	VRBoostValue[VRboostAxis::CameraHorizonAdjustment] = config.CameraHorizonAdjustment;
	VRBoostValue[VRboostAxis::ConstantValue1] = config.ConstantValue1;
	VRBoostValue[VRboostAxis::ConstantValue2] = config.ConstantValue2;
	VRBoostValue[VRboostAxis::ConstantValue3] = config.ConstantValue3;

	// set behavior accordingly to game type
	switch(config.game_type)
	{
	case D3DProxyDevice::FIXED:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::BEGIN_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::PRESENT;
		break;
	case D3DProxyDevice::SOURCE:
	case D3DProxyDevice::SOURCE_L4D:
	case D3DProxyDevice::SOURCE_ESTER:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::END_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::END_SCENE;
		break;
	case D3DProxyDevice::UNREAL:
	case D3DProxyDevice::UNREAL_MIRROR:
	case D3DProxyDevice::UNREAL_UT3:
	case D3DProxyDevice::UNREAL_BIOSHOCK:
	case D3DProxyDevice::UNREAL_BORDERLANDS:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::END_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::END_SCENE;
		break;
	case D3DProxyDevice::EGO:
	case D3DProxyDevice::EGO_DIRT:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::END_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::END_SCENE;
		break;
	case D3DProxyDevice::REALV:
	case D3DProxyDevice::REALV_ARMA:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::BEGIN_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::END_SCENE;
		break;
	case D3DProxyDevice::UNITY:
	case D3DProxyDevice::UNITY_SLENDER:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::BEGIN_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::END_SCENE;
		break;
	case D3DProxyDevice::GAMEBRYO:
	case D3DProxyDevice::GAMEBRYO_SKYRIM:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::BEGIN_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::END_SCENE;
		break;
	case D3DProxyDevice::LFS:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::BEGIN_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::END_SCENE;
		break;
	case D3DProxyDevice::CDC:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::END_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::END_SCENE;
		break;
	default:
		m_deviceBehavior.whenToHandleHeadTracking = DeviceBehavior::WhenToDo::BEGIN_SCENE;
		m_deviceBehavior.whenToRenderBRASSA = DeviceBehavior::WhenToDo::PRESENT;
		break;
	}
}


//FPS Calculator

#define MAXSAMPLES 100

float D3DProxyDevice::CalcFPS(){
	static bool init=false;
	static int tickindex=0;
	static LONGLONG ticksum=0;
	static LONGLONG ticklist[MAXSAMPLES];
	static LONGLONG prevTick;
	static LARGE_INTEGER perffreq;
	if (!init)
	{
		//Initialise - should only ever happen once
		memset(ticklist, 0, sizeof(LONGLONG) * MAXSAMPLES);
		QueryPerformanceFrequency(&perffreq);
		init=true;
	}

	//Get the new tick
	LARGE_INTEGER newtick;
	QueryPerformanceCounter(&newtick);
	
	ticksum -= ticklist[tickindex];
    ticksum += newtick.QuadPart - prevTick;
    ticklist[tickindex] = newtick.QuadPart - prevTick;
    tickindex = ++tickindex % MAXSAMPLES;
	prevTick = newtick.QuadPart;

	float FPS = (float)((double)MAXSAMPLES / ((double)ticksum / (double)perffreq.QuadPart));

	//char buffer[256];
	//sprintf(buffer, "FPS: %.1f\n", FPS);
	//OutputDebugStringA(buffer);

    return FPS;
}



/**
* Releases HUD font, shader registers, render targets, texture stages, vertex buffers, depth stencils, indices, shaders, declarations.
***/
METHOD_IMPL( void , , D3DProxyDevice , ReleaseEverything )
	// Fonts and any other D3DX interfaces should be released first.
	// They frequently hold stateblocks which are holding further references to other resources.
	menu.freeResources();


	m_spManagedShaderRegisters->ReleaseResources();

	if (m_pCapturingStateTo) {
		m_pCapturingStateTo->Release();
		m_pCapturingStateTo = NULL;
	}

	// one of these will still have a count of 1 until the backbuffer is released
	for(std::vector<D3D9ProxySurface*>::size_type i = 0; i != m_activeRenderTargets.size(); i++) 
	{
		if (m_activeRenderTargets[i] != NULL) {
			m_activeRenderTargets[i]->Release();
			m_activeRenderTargets[i] = NULL;
		}
	} 


	auto it = m_activeTextureStages.begin();
	while (it != m_activeTextureStages.end()) {
		if (it->second)
			it->second->Release();

		it = m_activeTextureStages.erase(it);
	}


	auto itVB = m_activeVertexBuffers.begin();
	while (itVB != m_activeVertexBuffers.end()) {
		if (itVB->second)
			itVB->second->Release();

		itVB = m_activeVertexBuffers.erase(itVB);
	}


	SAFE_RELEASE( m_pActiveStereoDepthStencil )
	SAFE_RELEASE( m_pActiveIndicies )
	SAFE_RELEASE( currentPS )
	SAFE_RELEASE( currentVS )
	SAFE_RELEASE( m_pActiveVertexDeclaration )
}
/**
* Comparison made against active primary render target.
*
***/
METHOD_IMPL( bool , , D3DProxyDevice , isViewportDefaultForMainRT , CONST D3DVIEWPORT9* , pViewport )
	D3D9ProxySurface* pPrimaryRenderTarget = m_activeRenderTargets[0];
	D3DSURFACE_DESC pRTDesc;
	pPrimaryRenderTarget->GetDesc(&pRTDesc);

	return  ((pViewport->Height == pRTDesc.Height) && (pViewport->Width == pRTDesc.Width) &&
		(pViewport->MinZ <= SMALL_FLOAT) && (pViewport->MaxZ >= SLIGHTLY_LESS_THAN_ONE));
}

/**
* Stores and sets view transform calling SetTransform() accordingly to current render side.
* @param pLeftMatrix The left view matrix.
* @param pRightMatrix The right view matrix.
* @param apply True to apply calling SetTransform()
* @see actual->SetTransform()
***/
METHOD_IMPL( HRESULT , , D3DProxyDevice , SetStereoViewTransform , D3DXMATRIX , pLeftMatrix , D3DXMATRIX , pRightMatrix , bool , apply )
	if (D3DXMatrixIsIdentity(&pLeftMatrix) && D3DXMatrixIsIdentity(&pRightMatrix)) {
		m_bViewTransformSet = false;
	}
	else {
		m_bViewTransformSet = true;
	}

	m_leftView = pLeftMatrix;
	m_rightView = pRightMatrix;

	if (m_currentRenderingSide == vireio::Left) {
		m_pCurrentView = &m_leftView;
	}
	else {
		m_pCurrentView = &m_rightView;
	}

	if (apply)
		return actual->SetTransform(D3DTS_VIEW, m_pCurrentView);
	else
		return D3D_OK;
}

/**
* Stores and sets projection transform calling SetTransform() accordingly to current render side.
* @param pLeftMatrix The left view matrix.
* @param pRightMatrix The right view matrix.
* @param apply True to apply calling SetTransform()
* @see actual->SetTransform()
***/
METHOD_IMPL( HRESULT , , D3DProxyDevice , SetStereoProjectionTransform , D3DXMATRIX , pLeftMatrix , D3DXMATRIX , pRightMatrix , bool , apply )
	if (D3DXMatrixIsIdentity(&pLeftMatrix) && D3DXMatrixIsIdentity(&pRightMatrix)) {
		m_bProjectionTransformSet = false;
	}
	else {
		m_bProjectionTransformSet = true;
	}

	m_leftProjection = pLeftMatrix;
	m_rightProjection = pRightMatrix;

	if (m_currentRenderingSide == vireio::Left) {
		m_pCurrentProjection = &m_leftProjection;
	}
	else {
		m_pCurrentProjection = &m_rightProjection;
	}

	if (apply)
		return actual->SetTransform(D3DTS_PROJECTION, m_pCurrentProjection);
	else
		return D3D_OK;
}

/**
* Sets the viewport to squish the GUI accordingly.
***/
METHOD_IMPL( void , , D3DProxyDevice , SetGUIViewport )
	// do not squish the viewport in case brassa menu is open - GBCODE Why?
	//if ((BRASSA_mode>=BRASSA_Modes::MAINMENU) && (BRASSA_mode<BRASSA_Modes::BRASSA_ENUM_RANGE))
	//	return;

	D3DXMATRIX mLeftShift;
	D3DXMATRIX mRightShift;

	// set shift by current gui 3d depth
	float shiftInPixels = config.guiDepth;
	D3DXMatrixTranslation(&mLeftShift, -shiftInPixels, 0, 0);
	D3DXMatrixTranslation(&mRightShift, shiftInPixels, 0, 0);

	// get matrix
	D3DXMATRIX mVPSquash = mLeftShift * m_spShaderViewAdjustment->Squash();
	if (m_currentRenderingSide != vireio::Left)
		mVPSquash = mRightShift * m_spShaderViewAdjustment->Squash();

	// get viewport
	actual->GetViewport(&m_ViewportIfSquished);

	// get screen center and translate it
	float centerX = (((FLOAT)stereoView->viewport.Width-(FLOAT)stereoView->viewport.X)/2.0f);
	float centerY = (((FLOAT)stereoView->viewport.Height-(FLOAT)stereoView->viewport.Y)/2.0f);

	// get left/top viewport sides
	D3DXVECTOR3 vIn = D3DXVECTOR3((FLOAT)stereoView->viewport.X-centerX, (FLOAT)stereoView->viewport.Y-centerY,1);
	D3DXVECTOR4 vOut = D3DXVECTOR4();
	D3DXVec3Transform(&vOut,&vIn, &mVPSquash);
	float floatMultiplier = 4;
	int originalX = (int)(vOut.x+centerX);
	int originalY = (int)(vOut.y+centerY);
	if(m_bfloatingMenu && tracker )
	{
		/*char buf[64];
		LPCSTR psz = NULL;
		sprintf_s(buf, "yaw: %f, pitch: %f\n", tracker->primaryYaw, tracker->primaryPitch);
		psz = buf;*/		
		m_ViewportIfSquished.X = (int)(vOut.x+centerX-(((m_fFloatingYaw - tracker->currentYaw) * floatMultiplier) * (180 / M_PI)));
		m_ViewportIfSquished.Y = (int)(vOut.y+centerY-(((m_fFloatingPitch - tracker->currentPitch) * floatMultiplier) * (180 / M_PI)));
	}
	else
	{
		m_ViewportIfSquished.X = (int)(vOut.x+centerX);
		m_ViewportIfSquished.Y = (int)(vOut.y+centerY);
	}

	// get right/bottom viewport sides
	vIn = D3DXVECTOR3((FLOAT)(stereoView->viewport.Width+stereoView->viewport.X)-centerX, (FLOAT)(stereoView->viewport.Height+stereoView->viewport.Y)-centerY,1);
	vOut = D3DXVECTOR4();
	D3DXVec3Transform(&vOut,&vIn, &mVPSquash);
	m_ViewportIfSquished.Width = (int)(vOut.x+centerX) - originalX;
	m_ViewportIfSquished.Height = (int)(vOut.y+centerY) - originalY;

	// set viewport
	m_bViewportIsSquished = true;
	actual->SetViewport(&m_ViewportIfSquished);
}

/**
* Rounds the floats to make them more display friendly
**/
METHOD_IMPL( float , , D3DProxyDevice , RoundBrassaValue , float , val )
	return (float)floor(val * 1000.0f + 0.5f) / 1000.0f;
}


METHOD_IMPL( bool , , D3DProxyDevice , InitVRBoost )
	bool initSuccess = false;

	// explicit VRboost dll import
	hmVRboost = LoadLibraryA("VRboost.dll");

	VRBoostStatus.VRBoost_Active = false;
	VRBoostStatus.VRBoost_LoadRules = false;
	VRBoostStatus.VRBoost_ApplyRules = false;

	// get VRboost methods
	if (hmVRboost != NULL)
	{
		OutputDebugStringA("VR Boost Loaded\n");
		// get methods explicit
		m_pVRboost_LoadMemoryRules = (LPVRBOOST_LoadMemoryRules)GetProcAddress(hmVRboost, "VRboost_LoadMemoryRules");
		m_pVRboost_SaveMemoryRules = (LPVRBOOST_SaveMemoryRules)GetProcAddress(hmVRboost, "VRboost_SaveMemoryRules");
		m_pVRboost_CreateFloatMemoryRule = (LPVRBOOST_CreateFloatMemoryRule)GetProcAddress(hmVRboost, "VRboost_CreateFloatMemoryRule");
		m_pVRboost_SetProcess = (LPVRBOOST_SetProcess)GetProcAddress(hmVRboost, "VRboost_SetProcess");
		m_pVRboost_ReleaseAllMemoryRules = (LPVRBOOST_ReleaseAllMemoryRules)GetProcAddress(hmVRboost, "VRboost_ReleaseAllMemoryRules");
		m_pVRboost_ApplyMemoryRules = (LPVRBOOST_ApplyMemoryRules)GetProcAddress(hmVRboost, "VRboost_ApplyMemoryRules");
		if ((!m_pVRboost_LoadMemoryRules) || 
			(!m_pVRboost_SaveMemoryRules) || 
			(!m_pVRboost_CreateFloatMemoryRule) || 
			(!m_pVRboost_SetProcess) || 
			(!m_pVRboost_ReleaseAllMemoryRules) || 
			(!m_pVRboost_ApplyMemoryRules))
		{
			hmVRboost = NULL;
			FreeLibrary(hmVRboost);
		}
		else
		{
			initSuccess = true;
			VRBoostStatus.VRBoost_Active = true;
			OutputDebugStringA("Success loading VRboost methods.");
		}

		m_VRboostRulesPresent = false;
		m_VertexShaderCount = 0;
		m_VertexShaderCountLastFrame = 0;

		// set common default VRBoost values
		ZeroMemory(&VRBoostValue[0], MAX_VRBOOST_VALUES*sizeof(float));
		VRBoostValue[VRboostAxis::Zero] = 0.0f;
		VRBoostValue[VRboostAxis::One] = 1.0f;
		VRBoostValue[VRboostAxis::WorldFOV] = 95.0f;
		VRBoostValue[VRboostAxis::PlayerFOV] = 125.0f;
		VRBoostValue[VRboostAxis::FarPlaneFOV] = 95.0f;
	}
	else
	{
		initSuccess = false;
	}

	return initSuccess;
}

METHOD_IMPL( bool , , D3DProxyDevice , InitBrassa )
	screenshot = (int)false;
	m_bVRBoostToggle = true;
	m_fVRBoostIndicator = 0.0f;

	m_spShaderViewAdjustment->ComputeViewTransforms();

	for (int i = 0; i < 16; i++)
		controls.xButtonsStatus[i] = false;
	
	
	return true;
}












  /**
* Here the chosen stereoviews draw function is called to render to wrapped back buffer.
* All other final screen output is also done here.
***/
void D3DProxyDevice::ProxyPresent(){
 	IDirect3DSurface9* pWrappedBackBuffer;

	try {
		m_activeSwapChains.at(0)->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pWrappedBackBuffer);

		if (stereoView->initialized)
			stereoView->Draw(static_cast<D3D9ProxySurface*>(pWrappedBackBuffer));

		pWrappedBackBuffer->Release();
	}
	catch (std::out_of_range) {
		OutputDebugStringA("Present: No primary swap chain found. (Present probably called before device has been reset)");
	}

	// did set this now also in proxy swap chain ? solved ?
	// (this can break if device present is followed by present on another swap chain... or not work well anyway)
	m_isFirstBeginSceneOfFrame = true; 

	BRASSA_UpdateBorder();

	//Now calculate frames per second
	fps = CalcFPS();
 }


 






/**
* Creates a proxy (or wrapped) depth stencil surface (D3D9ProxySurface).
* Surface to be created only gets both stereo surfaces if game handler agrees.
* @see D3D9ProxySurface
* @see GameHandler::ShouldDuplicateDepthStencilSurface() 
***/
METHOD_IMPL( HRESULT , , D3DProxyDevice , ProxyCreateDepthStencilSurface , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Discard , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage , bool , useEx )
	IDirect3DSurface9* left  = 0;
	IDirect3DSurface9* right = 0;
	HRESULT            resultLeft;
	HRESULT            resultRight;

	if( useEx ){
		resultLeft = actualEx->CreateDepthStencilSurfaceEx( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &left, pSharedHandle , Usage );
	}else{
		resultLeft = actual  ->CreateDepthStencilSurface  ( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &left, pSharedHandle);
	}
	
	if( SUCCEEDED(resultLeft) ){

		// TODO Should we always duplicated Depth stencils? I think yes, but there may be exceptions
		if (m_pGameHandler->ShouldDuplicateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard)) 
		{
			if( useEx ){
				resultRight = actualEx->CreateDepthStencilSurfaceEx( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &right, pSharedHandle , Usage );
			}else{
				resultRight = actual  ->CreateDepthStencilSurface  ( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &right, pSharedHandle);
			}

			if( FAILED(resultRight) ){
				OutputDebugStringA("Failed to create right eye Depth Stencil Surface while attempting to create stereo pair, falling back to mono\n");
				right = 0;
			}
		}

		*ppSurface = new D3D9ProxySurface(left, right, this, NULL);
	} else {
		OutputDebugStringA("Failed to create Depth Stencil Surface\n"); 
	}

	return resultLeft;
}



/**
* Creates proxy (wrapped) render target, if swapchain buffer returns StereoBackBuffer, otherwise D3D9ProxySurface.
* Duplicates render target if game handler agrees.
* @see GameHandler::ShouldDuplicateRenderTarget()
* @see StereoBackBuffer
* @see D3D9ProxySurface
***/
METHOD_IMPL( HRESULT , , D3DProxyDevice , ProxyCreateRenderTarget , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample ,
												  DWORD , MultisampleQuality , BOOL , Lockable , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage , bool , isSwapChainBackBuffer , bool , useEx )
	IDirect3DSurface9* left  = 0;
	IDirect3DSurface9* right = 0;
	HRESULT resultLeft;
	HRESULT resultRight;

	// create left/mono

	if( useEx ){
		resultLeft = actualEx->CreateRenderTargetEx(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &left, pSharedHandle , Usage );
	}else{
		resultLeft = actual->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &left, pSharedHandle);
	}

	if( SUCCEEDED(resultLeft) ){

		/* "If Needed" heuristic is the complicated part here.
		Fixed heuristics (based on type, format, size, etc) + game specific overrides + isForcedMono + magic? */
		// TODO Should we duplicate this Render Target? Replace "true" with heuristic
		if (m_pGameHandler->ShouldDuplicateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, isSwapChainBackBuffer))
		{
			if( useEx ){
				resultRight = actualEx->CreateRenderTargetEx(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &right, pSharedHandle , Usage );
			}else{
				resultRight = actual->CreateRenderTarget  (Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &right, pSharedHandle);
			}

			if( FAILED(resultRight) ){
				OutputDebugStringA("Failed to create right eye render target while attempting to create stereo pair, falling back to mono\n");
				right = NULL;
			}
		}

		if (!isSwapChainBackBuffer){
			*ppSurface = new D3D9ProxySurface(left, right, this, NULL);
		}else{
			*ppSurface = new StereoBackBuffer(left, right, this);
		}
	}
	else {
		OutputDebugStringA("Failed to create render target\n"); 
	}


	return resultLeft;
}



//Creates a wrapped mono surface with only one (left) side.
//OffscreenPlainSurfaces doesn't need to be Stereo. 
//They can't be used as render targets and they can't have rendertargets copied to them with stretch
//rect, so don't need to be stereo capable.
//See table at bottom of 
//<http://msdn.microsoft.com/en-us/library/windows/desktop/bb174471%28v=vs.85%29.aspx> 
//for stretch rect restrictions.
METHOD_IMPL( HRESULT , , D3DProxyDevice , ProxyCreateOffscreenPlainSurface , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage , bool , useEx )
	IDirect3DSurface9* surface = 0;
	HRESULT result;
	
	if( useEx ){
		result = actualEx->CreateOffscreenPlainSurfaceEx(Width, Height, Format, Pool, &surface, pSharedHandle , Usage );
	}else{
		result = actual->CreateOffscreenPlainSurface(Width, Height, Format, Pool, &surface, pSharedHandle );
	}

	if (SUCCEEDED(result)){
		*ppSurface = new D3D9ProxySurface(surface, NULL, this, NULL);
	}

	return result;
}


/**
* Calls release functions here and in stereo view class, releases swap chains and restores everything.
* Subclasses which override this method must call through to super method at the end of the subclasses
* implementation.
* @see ReleaseEverything()
* @see StereoView::ReleaseEverything()
* @see OnCreateOrRestore()
***/
METHOD_IMPL( HRESULT , , D3DProxyDevice , ProxyReset , D3DPRESENT_PARAMETERS* , pPresentationParameters , D3DDISPLAYMODEEX* , pFullscreenDisplayMode , bool , useEx )
	if(stereoView)
		stereoView->ReleaseEverything();

	ReleaseEverything();

	m_bInBeginEndStateBlock = false;

	auto it = m_activeSwapChains.begin();
	while (it != m_activeSwapChains.end()) {

		if ((*it) != NULL)
			(*it)->Release();

		delete (*it);

		it = m_activeSwapChains.erase(it);
	}

	HRESULT hr;
	if( useEx ){
		hr = actualEx->ResetEx(pPresentationParameters,pFullscreenDisplayMode);
	}else{
		hr = actual->Reset(pPresentationParameters);
	}

	// if the device has been successfully reset we need to recreate any resources we created
	if (hr == D3D_OK)  {
		OnCreateOrRestore();
		stereoView->PostReset();
	}
	else {
#ifdef _DEBUG
		char buf[256];
		sprintf_s(buf, "Error: %s error description: %s\n",
			DXGetErrorString(hr), DXGetErrorDescription(hr));

		OutputDebugStringA(buf);				
#endif
		OutputDebugStringA("Device reset failed");
	}

	return hr;
}







// IDirect3DDevice9 / IDirect3DDevice9Ex similar methods

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , Present , CONST RECT* , pSourceRect , CONST RECT* , pDestRect , HWND , hDestWindowOverride , CONST RGNDATA* , pDirtyRegion )
	ProxyPresent();
	return actual->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , PresentEx , CONST RECT* , pSourceRect , CONST RECT* , pDestRect , HWND , hDestWindowOverride , CONST RGNDATA* , pDirtyRegion , DWORD , dwFlags )
	ProxyPresent();
	return actualEx->PresentEx(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion , dwFlags );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateOffscreenPlainSurface , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle )	
	return ProxyCreateOffscreenPlainSurface( Width, Height, Format, Pool, ppSurface , pSharedHandle , 0 , false );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateOffscreenPlainSurfaceEx , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DPOOL , Pool , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage )	
	return ProxyCreateOffscreenPlainSurface( Width, Height, Format, Pool, ppSurface , pSharedHandle , Usage , true );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateRenderTarget , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Lockable , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle )
	//The IDirect3DSurface9** ppSurface returned should always be a D3D9ProxySurface. Any class overloading
	//this method should ensure that this remains true.
	return ProxyCreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle, 0 , false , false );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateRenderTargetEx , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Lockable , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage )
	return ProxyCreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle, 0 , false , true );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateDepthStencilSurface , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Discard , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle )
	return ProxyCreateDepthStencilSurface( Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface , pSharedHandle , 0  , false );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , CreateDepthStencilSurfaceEx , UINT , Width , UINT , Height , D3DFORMAT , Format , D3DMULTISAMPLE_TYPE , MultiSample , DWORD , MultisampleQuality , BOOL , Discard , IDirect3DSurface9** , ppSurface , HANDLE* , pSharedHandle , DWORD , Usage )
	return ProxyCreateDepthStencilSurface( Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface , pSharedHandle , Usage , true );
}


METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , Reset , D3DPRESENT_PARAMETERS* , pPresentationParameters )
	return ProxyReset( pPresentationParameters , 0 , false );
}

METHOD_IMPL( HRESULT , WINAPI , D3DProxyDevice , ResetEx , D3DPRESENT_PARAMETERS* , pPresentationParameters , D3DDISPLAYMODEEX* , pFullscreenDisplayMode )
	return ProxyReset( pPresentationParameters , pFullscreenDisplayMode , true );
}




// IDirect3DDevice9Ex base methods. are there any proxy required for those?
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , GetDisplayModeEx , UINT , iSwapChain , D3DDISPLAYMODEEX* , pMode , D3DDISPLAYROTATION* , pRotation )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , SetConvolutionMonoKernel , UINT , width , UINT , height , float* , rows , float* , columns )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , ComposeRects , IDirect3DSurface9* , pSrc , IDirect3DSurface9* , pDst , IDirect3DVertexBuffer9* , pSrcRectDescs , UINT , NumRects , IDirect3DVertexBuffer9* , pDstRectDescs , D3DCOMPOSERECTSOP , Operation , int , Xoffset , int , Yoffset )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , GetGPUThreadPriority , INT* , pPriority )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , SetGPUThreadPriority , INT , Priority )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , WaitForVBlank , UINT , iSwapChain )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , CheckResourceResidency , IDirect3DResource9** , pResourceArray , UINT32 , NumResources )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , SetMaximumFrameLatency , UINT , MaxLatency )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , GetMaximumFrameLatency , UINT* , pMaxLatency )
METHOD_THRU_EX( HRESULT , WINAPI , D3DProxyDevice , CheckDeviceState , HWND , hDestinationWindow )



// IDirect3DDevice9 base methods
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , TestCooperativeLevel ) // The calling application will start releasing resources after TestCooperativeLevel returns D3DERR_DEVICENOTRESET.
METHOD_THRU( UINT    , WINAPI , D3DProxyDevice , GetAvailableTextureMem )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , EvictManagedResources )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetDeviceCaps , D3DCAPS9* , pCaps )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetDisplayMode , UINT , iSwapChain , D3DDISPLAYMODE* , pMode )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetCreationParameters , D3DDEVICE_CREATION_PARAMETERS* , pParameters )
METHOD_THRU( void    , WINAPI , D3DProxyDevice , SetCursorPosition , int , X , int , Y , DWORD , Flags )
METHOD_THRU( BOOL    , WINAPI , D3DProxyDevice , ShowCursor , BOOL , bShow )
METHOD_THRU( UINT    , WINAPI , D3DProxyDevice , GetNumberOfSwapChains )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetRasterStatus , UINT , iSwapChain , D3DRASTER_STATUS* , pRasterStatus )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetDialogBoxMode , BOOL , bEnableDialogs )
METHOD_THRU( void    , WINAPI , D3DProxyDevice , SetGammaRamp     , UINT , iSwapChain , DWORD , Flags , CONST D3DGAMMARAMP* , pRamp )
METHOD_THRU( void    , WINAPI , D3DProxyDevice , GetGammaRamp     , UINT , iSwapChain , D3DGAMMARAMP* , pRamp )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetTransform , D3DTRANSFORMSTATETYPE , State , D3DMATRIX* , pMatrix )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , MultiplyTransform , D3DTRANSFORMSTATETYPE , State , CONST D3DMATRIX* , pMatrix );
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetViewport , D3DVIEWPORT9* , pViewport )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetMaterial , CONST D3DMATERIAL9* , pMaterial )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetMaterial , D3DMATERIAL9* , pMaterial )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetLight , DWORD , Index , CONST D3DLIGHT9* , pLight )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetLight , DWORD , Index , D3DLIGHT9* , pLight )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , LightEnable , DWORD , Index , BOOL , Enable )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetLightEnable , DWORD , Index , BOOL* , pEnable )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetClipPlane , DWORD , Index , CONST float* , pPlane )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetClipPlane , DWORD , Index , float* , pPlane )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetRenderState , D3DRENDERSTATETYPE , State , DWORD , Value )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetRenderState , D3DRENDERSTATETYPE , State , DWORD* , pValue )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetClipStatus , CONST D3DCLIPSTATUS9* , pClipStatus )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetClipStatus , D3DCLIPSTATUS9* , pClipStatus )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetTextureStageState , DWORD , Stage , D3DTEXTURESTAGESTATETYPE , Type , DWORD* , pValue )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetTextureStageState , DWORD , Stage , D3DTEXTURESTAGESTATETYPE , Type , DWORD  , Value )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetSamplerState , DWORD , Sampler , D3DSAMPLERSTATETYPE , Type , DWORD* , pValue)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetSamplerState,DWORD ,Sampler,D3DSAMPLERSTATETYPE ,Type,DWORD ,Value)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , ValidateDevice,DWORD* ,pNumPasses)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetPaletteEntries,UINT ,PaletteNumber,CONST PALETTEENTRY*, pEntries)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetPaletteEntries,UINT ,PaletteNumber,PALETTEENTRY* ,pEntries)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetCurrentTexturePalette,UINT, PaletteNumber)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetCurrentTexturePalette,UINT*,PaletteNumber)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetScissorRect,CONST RECT*, pRect)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetScissorRect,RECT*, pRect)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetSoftwareVertexProcessing,BOOL, bSoftware)
METHOD_THRU( BOOL    , WINAPI , D3DProxyDevice , GetSoftwareVertexProcessing)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetNPatchMode,float, nSegments)
METHOD_THRU( float   , WINAPI , D3DProxyDevice , GetNPatchMode)
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetFVF , DWORD , FVF )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetFVF , DWORD* , pFVF )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetVertexShaderConstantI , UINT , StartRegister , CONST int*  , pConstantData , UINT , Vector4iCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetVertexShaderConstantI , UINT , StartRegister , int*        , pConstantData , UINT , Vector4iCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetVertexShaderConstantB , UINT , StartRegister , CONST BOOL* , pConstantData , UINT , BoolCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetVertexShaderConstantB , UINT , StartRegister , BOOL*       , pConstantData , UINT , BoolCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetStreamSourceFreq , UINT , StreamNumber , UINT  , Setting )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetStreamSourceFreq , UINT , StreamNumber , UINT* , pSetting )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetPixelShaderConstantI , UINT , StartRegister , CONST int* , pConstantData , UINT , Vector4iCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetPixelShaderConstantI , UINT , StartRegister , int*       , pConstantData , UINT , Vector4iCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , SetPixelShaderConstantB , UINT , StartRegister , CONST BOOL*, pConstantData , UINT , BoolCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , GetPixelShaderConstantB , UINT , StartRegister , BOOL*      , pConstantData , UINT , BoolCount )
METHOD_THRU( HRESULT , WINAPI , D3DProxyDevice , DeletePatch , UINT , Handle )

