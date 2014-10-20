

C( CALCULATED )
P( QString , vireioDir              ,)
P( float   , screenAspectRatio      ,)
P( float   , scaleToFillHorizontal  ,)
P( float   , lensXCenterOffset      ,)

C( GENERAL )
P( QString , stereoDevice			, )
P( int     , trackerMode			, )
P( bool    , logToConsole			, )
P( bool    , logToFile				, )
P( bool    , logHijack				, )
P( bool    , logVerbose             , )
P( bool    , pauseOnLaunch			, )
P( bool    , streamingEnable		, )
P( QString , streamingAddress		, )
P( int     , streamingPort			, )
P( QString , streamingCodec			, )
P( int     , streamingBitrate		, )
P( bool    , showNotifications		, )
P( int     , displayAdapter         , )
P( int     , mirrorScreen           , )
P( int     , mirrorMode             , )
P( bool    , mirrorFullscreen		, )

P( bool    , shaderAnalyzer			         , )
P( bool    , shaderAnalyzerTranspose         , )
P( bool    , shaderAnalyzerDetectTranspose   , )
P( bool    , shaderAnalyzerShowUnusedShaders , )
P( bool    , shaderAnalyzerShowPixelShaders  , )



C( GAME )
P( QString , exePath					, )
P( QString , profileName				, )

C( PROFILE )
P( QString , exeName                    , )// Game executable name
P( QString , shaderRule                 , )// Name of shader rule file for this game
P( bool    , VRboostEnable              , )
P( int     , VRboostMinShaderCount      , )// Minimum Vertex Shader Count to apply VRboost (security)
P( int     , VRboostMaxShaderCount      , )// Maximum Vertex Shader Count to apply VRboost (security)
P( int     , game_type                  , )// Game type enumeration. Matching D3DProxyDevice::ProxyTypes
P( bool    , rollEnabled                , )// True if headtracking-roll is to be enabled
P( bool    , swap_eyes                  , )// Option to swap the eye output for different 3D systems
P( bool    , showVRMouse                , )

P( int     , duplicateRenderTarget , )
P( int     , duplicateDepthStencil , )
P( int     , duplicateTexture      , )
P( int     , duplicateCubeTexture  , )
P( int     , whenRenderMenu        , )
P( int     , whenUpdateTracker     , )
P( int     , saveStateMethod       , )

P( bool    , trackerTimewarp         , )

P( float   , trackerYawMultiplier		, )
P( float   , trackerPitchMultiplier	    , )
P( float   , trackerRollMultiplier		, )
P( float   , trackerXMultiplier	        , )
P( float   , trackerYMultiplier	        , )
P( float   , trackerZMultiplier	        , )
P( bool    , trackerPositionEnable      , )
P( bool    , trackerRotationEnable      , )
P( float   , trackerMouseYawMultiplier	, )
P( float   , trackerMousePitchMultiplier, )
P( bool    , trackerMouseEmulation		, )

P( float   , stereoScale       , )
P( float   , stereoConvergence , )


P( bool    , guiBulletLabyrinth , )
P( float   , guiSquash   , )
P( float   , guiDepth    , )
P( float   , hudDistance , )
P( float   , hudDepth    , )



P( float   , DistortionScale            , )// The scale to apply distortion
P( float   , YOffset                    , )// The Y offset from the centre of the screen on the Y-axis *
P( float   , IPDOffset                  , )// The IPD offset from the centre of the screen on the X-axis *
P( int     , VRBoostResetHotkey         , )// Hotkey option to reset VRBoost
P( float   , WorldFOV                   , )// Game-specific global FOV setting
P( float   , PlayerFOV                  , )// Game-specific player surroundings FOV setting
P( float   , FarPlaneFOV                , )// Game-specific FOV setting for the most distant objects
P( float   , CameraTranslateX           , )// Translates the camera along the X-Axis
P( float   , CameraTranslateY           , )// Translates the camera along the Y-Axis
P( float   , CameraTranslateZ           , )// Translates the camera along the Z-Axis
P( float   , CameraDistance             , )// Changes the distance of the camera
P( float   , CameraZoom                 , )// Changes the cameras zoom
P( float   , CameraHorizonAdjustment    , )// Adjusts the cameras X axis rotation to match the horizon
P( float   , ConstantValue1             , )// Game-specific constant value 1
P( float   , ConstantValue2             , )// Game-specific constant value 2
P( float   , ConstantValue3             , )// Game-specific constant value 3
P( QString , SteamAppId                 , )// Game Steam app id (number only). Create game desktop shortcut from steam, open properties and find the number.
P( QString , CommandLineArguments       , )// Arguments passed to executable

C( DEVICE )
P( QString , shader							,)
P( bool    , isHmd							,)
P( int     , resolutionWidth				,)
P( int     , resolutionHeight				,)
P( float   , physicalWidth					,)
P( float   , physicalHeight					,)
P( float   , distortionCoefficients         ,[4] )
P( float   , chromaCoefficients             ,[4] )
P( float   , eyeToScreenDistance			,)
P( float   , physicalLensSeparation			,)
P( float   , lensYCenterOffset				,)
P( float   , lensIPDCenterOffset			,)
P( float   , minDistortionScale				,)
P( bool    , chromaticAberrationCorrection	,)
P( bool    , useOvrDeviceSettings			,)

C( USER )
P( float       , PlayerIPD    ,)
P( QStringList , hotkey_uid   ,)
P( QStringList , hotkey_codes ,)
