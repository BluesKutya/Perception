#include "DataGatherer.h"
#include "D3D9ProxyVertexShader.h"
#include <qfile.h>
#include <qtextstream.h>
#include <qcryptographichash.h>



DataGatherer::DataGatherer(IDirect3DDevice9* pDevice, IDirect3DDevice9Ex* pDeviceEx,D3D9ProxyDirect3D* pCreatedBy ) : D3DProxyDevice(pDevice,pDeviceEx ,pCreatedBy){
	currentVS         = 0;
	currentPS         = 0;
	showUnusedShaders = false;
	showPixelShaders  = false;

	cMenuItem* m;
	cMenuItem* i;

	m = menu.root.addSubmenu( "Shader analyzer" );

	i = m->addCheckbox( "Use transposed rules"          , &config.shaderAnalyzerTranspose       );

	i = m->addCheckbox( "Detect use of transposed rules" , &config.shaderAnalyzerDetectTranspose );

	shadersMenu = m->addSubmenu( "Shader list" );

	shadersMenu->addCheckbox( "Show unused shaders" , &showUnusedShaders );
	shadersMenu->addCheckbox( "Show pixel  shaders" , &showPixelShaders );
	
	
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



DataGatherer::~DataGatherer( ){
}



bool DataGatherer::isDrawHide( ){
	return (currentVS && currentVS->hide) || (currentPS && currentPS->hide);
}


void DataGatherer::ShaderCreate( IUnknown* ptr , bool isVertex ){
	ptr->AddRef();

	printf("create %d\n",ptr);

	Shader* shader = new Shader;
	shader->ptr      = ptr;
	shader->isVertex = isVertex;
	shader->used     = true;
	shader->hide     = false;
	shader->blink    = false;

	shaders += shader;

	
	D3D9ProxyVertexShader* vs = static_cast<D3D9ProxyVertexShader*>( isVertex ? ptr : 0   );
	D3D9ProxyPixelShader*  ps = static_cast<D3D9ProxyPixelShader* >( isVertex ? 0   : ptr );

	ID3DXConstantTable*     table = 0;

	{ //Get shader hash and table
		UINT len;

		if( vs ){
			vs->actual->GetFunction( 0 , &len );
		}else{
			ps->actual->GetFunction( 0 , &len );			
		}

		shader->code.resize( len );

		if( vs ){
			vs->actual->GetFunction( shader->code.data() , &len );
		}else{
			ps->actual->GetFunction( shader->code.data() , &len );			
		}
	
		shader->hash = QCryptographicHash::hash( shader->code , QCryptographicHash::Md5 ).toHex().toUpper();
	
		D3DXGetShaderConstantTable( reinterpret_cast<DWORD*>(shader->code.data()) , &table );
	}


	cMenuItem* mi;

	shader->item = shadersMenu->addSubmenu( QString(vs?"Vertex":"Pixel") + " shader " + shader->hash );

	shader->item->addCheckbox( "Do not draw" , &shader->hide );
	
	shader->item->addCheckbox( "Blink" , &shader->blink ); 

	/*
	cMenuItem* i = shader->item->addAction( "Save shader to \"" + shader->name + ".vs\"" ); 
	i->callbackValueChanged = [=](){
		QFile file( shader->name + ".vs" );
		if( file.open( QFile::WriteOnly ) ){
			QTextStream text( &file );

			ID3DXBuffer* buf; 
			D3DXDisassembleShader( reinterpret_cast<DWORD*>(shader->code.data()) , false , 0 , &buf ); 

			text << QByteArray( (char*)buf->GetBufferPointer() , buf->GetBufferSize() );
			text << "\r\n";
			text << "Shader Creator: " << shader->desc.Creator << "\r\n";
			text << "Shader Version: " << shader->desc.Version << "\r\n";
			text << "Shader Hash   : " << shader->name         << "\r\n";

			buf->Release();
		}
	};
	*/




	if( table ){
		D3DXCONSTANTTABLE_DESC tableDesc;
		table->GetDesc(&tableDesc);

		for( int c=0 ; c<tableDesc.Constants ; c++ ){
			D3DXHANDLE handle = table->GetConstant( 0 , c );
			if( !handle ){
				continue;
			}

			D3DXCONSTANT_DESC constantDesc[512];
			UINT              constantCount = 512;

			table->GetConstantDesc( handle , constantDesc , &constantCount );
			
			if( constantCount >= 512 ){
				printf("Need larger constant description buffer");
			}

			// loop through constants, output relevant data
			for( int i=0 ; i<constantCount ; i++ ){
				D3DXCONSTANT_DESC& desc = constantDesc[i];

				if( desc.RegisterSet != D3DXRS_FLOAT4 ){
					continue;
				}
				
				QString typeName;

				switch( desc.Class ){
				case D3DXPC_VECTOR:
					typeName = "D3DXPC_VECTOR";
					break;

				case D3DXPC_MATRIX_ROWS:
					typeName = "D3DXPC_MATRIX_ROWS";
					break;

				case D3DXPC_MATRIX_COLUMNS:
					typeName = "D3DXPC_MATRIX_COLUMNS";
					break;
				}

				if( typeName.isEmpty() ){
					continue;
				}

				ShaderConstant* n = new ShaderConstant;
				n->shader            = shader;
				n->desc              = desc;
				n->name              = desc.Name;

				n->item = shader->item->addSubmenu( "Constant \"" + n->name + "\"" );

				constants += n;
				
			}
		}
		
		SAFE_RELEASE(table)
	}
}




void DataGatherer::ShaderUse( IUnknown* ptr , Shader** current  ){
	*current = 0;

	for( Shader* s : shaders ){
		if( s->ptr == ptr ){
			*current = s;

			s->used = true;

			if( s->blink ){
				s->hide = ((GetTickCount()%300)>150);
			}

			break;
		}
	}
}




















HRESULT WINAPI DataGatherer::BeginScene( ){
	if( m_isFirstBeginSceneOfFrame ){
		for( Shader* s : shaders ){
			s->item->visible = (s->used || showUnusedShaders) && (s->isVertex || showPixelShaders);
			s->used          = false;
		}
	}

	return D3DProxyDevice::BeginScene();
}



HRESULT WINAPI DataGatherer::Present(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion){
	return D3DProxyDevice::Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}


HRESULT WINAPI DataGatherer::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount){
	if( isDrawHide() ){
		return S_OK;
	}
	return D3DProxyDevice::DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}



HRESULT WINAPI DataGatherer::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount){
	if( isDrawHide() ){
		return S_OK;
	}
	return D3DProxyDevice::DrawIndexedPrimitive(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}



HRESULT WINAPI DataGatherer::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride){
	if( isDrawHide() ){
		return S_OK;
	}
	return D3DProxyDevice::DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}



HRESULT WINAPI DataGatherer::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride){
	if( isDrawHide() ){
		return S_OK;
	}
	return D3DProxyDevice::DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}



HRESULT WINAPI DataGatherer::CreateVertexShader(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader){
	HRESULT ret = D3DProxyDevice::CreateVertexShader(pFunction, ppShader);
	if( SUCCEEDED(ret) ){
		ShaderCreate( *ppShader , true );
	}
	return ret;
}


HRESULT WINAPI DataGatherer::CreatePixelShader(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader){
	HRESULT ret = D3DProxyDevice::CreatePixelShader(pFunction, ppShader);
	if( SUCCEEDED(ret) ){
		ShaderCreate( *ppShader , false );
	}
	return ret;
}



HRESULT WINAPI DataGatherer::SetVertexShader( IDirect3DVertexShader9* pShader ){
	ShaderUse( pShader , &currentVS );
	return D3DProxyDevice::SetVertexShader(pShader);
}


HRESULT WINAPI DataGatherer::SetPixelShader( IDirect3DPixelShader9* pShader ){
	ShaderUse( pShader , &currentPS );
	return D3DProxyDevice::SetPixelShader(pShader);
}






// Tests if the set constant is a transposed matrix and sets the relevant bool.
// Is Matrix transposed ?
// Affine transformation matrices have in the last row (0,0,0,1). World and view matrices are 
// usually affine, since they are a combination of affine transformations (rotation, scale, 
// translation ...).
// Perspective projection matrices have in the last column (0,0,1,0) if left-handed and 
// (0,0,-1,0) if right-handed.
// Orthographic projection matrices have in the last column (0,0,0,1).
// If those are transposed you find the entries in the last column/row.
HRESULT WINAPI DataGatherer::SetVertexShaderConstantF( UINT StartRegister , CONST float* pConstantData , UINT Vector4fCount ){

	if( config.shaderAnalyzerDetectTranspose ){
		for( ShaderConstant* c : constants ){
			if( c->shader == currentVS ){
				if( c->desc.RegisterIndex >= StartRegister  &&
					c->desc.RegisterIndex < StartRegister + Vector4fCount
				){

					int i = 0;

					if( c->desc.Class == D3DXPARAMETER_CLASS::D3DXPC_MATRIX_ROWS ){
						i = 14;
					}

					if( c->desc.Class == D3DXPARAMETER_CLASS::D3DXPC_MATRIX_COLUMNS ){
						i = 12;
					}

					if( i ){
						D3DXMATRIX matrix = D3DXMATRIX( pConstantData + (c->desc.RegisterIndex-StartRegister)*4*sizeof(float) );
					
						if( fabs( fabs(matrix[i]) - 1.0 ) > 0.00001 ){
							config.shaderAnalyzerTranspose = true;
						}
					}
				}
			}
		}
	}

	return D3DProxyDevice::SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}







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
*/

/**
* Analyzes the game and outputs a shader rule xml file.
***/
void DataGatherer::Analyze()
{
/*
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
	*/
}

/**
* Fills the data structure for the shader rule menu nodes.
* @param allStartRegisters True if an existing constant name is added with all possible start registers.
***/
void DataGatherer::GetCurrentShaderRules(bool allStartRegisters)
{
	/*ShaderModificationRepository* pModRep = m_pGameHandler->GetShaderModificationRepository();

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

	}*/
}
