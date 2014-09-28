#include "cMenu.h"
#include "D3DProxyDevice.h"

cMenuItem::cMenuItem(){
	parent         = 0;
	selected       = 0;
	showCalibrator = false;
}

cMenuItem* cMenuItem::add( const QString& n , TYPE t ){
	cMenuItem* i = new cMenuItem;
	i->parent = this;
	i->name   = n;
	i->type   = t;
	children += i;
	return i;
}

cMenuItem* cMenuItem::addSubmenu ( const QString& n ){
	cMenuItem* i = add( n , SUBMENU );
	return i;
}


cMenuItem* cMenuItem::addAction  ( const QString& n ){
	cMenuItem* i = add( n , ACTION );
	return i;

}


cMenuItem* cMenuItem::addSpinner ( const QString& n , float* variable , float min , float max , float step ){
	cMenuItem* i = add( n , SPINNER );
	i->spinLimit = true;
	i->spinMin   = min;
	i->spinMax   = max;
	i->spinStep  = step;
	i->spinVar   = variable;
	return i;

}

cMenuItem* cMenuItem::addSpinner ( const QString& n , float* variable , float step ){
	cMenuItem* i = add( n , SPINNER );
	i->spinLimit = false;
	i->spinStep  = step;
	i->spinVar   = variable;
	return i;

}


cMenuItem* cMenuItem::addCheckbox( const QString& n , bool*  variable , const QString& on_text , const QString& off_text ){
	cMenuItem* i = add( n , CHECKBOX );
	i->checkVar = variable;
	i->checkOn  = on_text;
	i->checkOff = off_text;
	return i;

}



cMenu::cMenu( ){
	root.name   = "Vireio Main Menu";
	root.type   = cMenuItem::SUBMENU;
	root.parent = 0;
	prevKeyDown = false;
	menu        = &root;
}



void cMenu::render( ){
	if( !font || !sprite ){
		return;
	}

	bool newKeyDown = false;
	
	if( device->controls.Key_Down( VK_ADD ) ){
		newKeyDown = true;
		if( !prevKeyDown ){
			show = !show;
		}
	}

	if( !menu->selected && !menu->children.isEmpty() ){
		menu->selected = menu->children.first();
	}
	
	if( menu->selected ){
		int        move_y = 0;
		int        move_x = 0;
		cMenuItem* sel    = menu->selected;


		if( device->controls.Key_Down( VK_NUMPAD2 ) ){
			newKeyDown = true;
			if( !prevKeyDown ){
				move_y = -1;
			}
		}


		if( device->controls.Key_Down( VK_NUMPAD8 ) ){
			newKeyDown = true;
			if( !prevKeyDown ){
				move_y = +1;
			}
		}


		if( device->controls.Key_Down( VK_NUMPAD5 ) ){
			newKeyDown = true;
			if( !prevKeyDown ){
				switch( sel->type ){
				case cMenuItem::SUBMENU:
					menu = sel;
					if( sel->callbackOpenSubmenu ){
						sel->callbackOpenSubmenu();
					}
					break;

				case cMenuItem::CHECKBOX:
					*sel->checkVar = !*sel->checkVar;
					if( sel->callbackValueChanged ){
						sel->callbackValueChanged();
					}
					break;
				}
			}
		}

		
		//BUG: somehow it's sticked if shift is pressed
		if( device->controls.Key_Down( VK_NUMPAD4 ) ){
			newKeyDown = true;
			move_x = -1;
		}

		if( device->controls.Key_Down( VK_NUMPAD6 ) ){
			newKeyDown = true;
			move_x = +1;
		}

		if( move_y ){
			int i = menu->children.indexOf( sel );
			if( i >=0 ){
				i -= move_y;

				if( i >= 0 && i < menu->children.count() ){
					menu->selected = menu->children[i];
				}
			}
		}

		if( move_x ){
			float k = move_x;
			if( device->controls.Key_Down(VK_LCONTROL) ){
				k *= 0.1;
			}

			if( device->controls.Key_Down(VK_LSHIFT) ){
				k *= 10;
			}

			if( sel->type == cMenuItem::SPINNER ){
				(*sel->spinVar) += k * sel->spinStep;

				if( sel->spinLimit ){
					if( *sel->spinVar > sel->spinMax ){
						*sel->spinVar = sel->spinMax;
					}

					if( *sel->spinVar < sel->spinMin ){
						*sel->spinVar = sel->spinMin;
					}
				}

				if( sel->callbackValueChanged ){
					sel->callbackValueChanged();
				}
			}
		}
	}


	if( device->controls.Key_Down( VK_SUBTRACT ) ){
		newKeyDown = true;
		if( !prevKeyDown ){
			if( menu->callbackCloseSubmenu ){
				menu->callbackCloseSubmenu();
			}
			menu = menu->parent;
		}
	}

	prevKeyDown = newKeyDown;

	if( !show ){
		return;
	}

	if( menu->callbackRender ){
		menu->callbackRender();
	}

	sprite->Begin( D3DXSPRITE_ALPHABLEND );

	D3DXMATRIX matScale;
	D3DXMatrixScaling( &matScale , viewportWidth / 1920.0 , viewportHeight / 1080.0 , 1.0 );
	sprite->SetTransform( &matScale );

	posY = 300;

	drawText( menu->name , ALIGN_CENTER );
	posY += 40;

	drawRect( 0 , posY , 1920 , posY+3 , D3DCOLOR_ARGB(255,255,128,128)  );

	for( cMenuItem* i : menu->children ){
		if( i == menu->selected ){
			drawRect( 0 , posY+2 , 1920 , posY + 4 , D3DCOLOR_ARGB(255,128,255,128)  );
		}
			
		drawText( i->name , ALIGN_LEFT_COLUMN );

		if( i->type == cMenuItem::CHECKBOX ){
			drawText( (*i->checkVar) ? i->checkOn : i->checkOff , ALIGN_RIGHT_COLUMN );
		}else
		if( i->type == cMenuItem::SPINNER ){
			char s[256];

			int decimals = -floor(log10(i->spinStep));
			if( decimals < 0){
				decimals = 0;
			}

			sprintf_s( s , "%.*f" , decimals , *i->spinVar );
				
			drawText( s , ALIGN_RIGHT_COLUMN );
		}

		posY += 40;

		if( i == menu->selected ){
			drawRect( 0 , posY-2 , 1920 , posY + 2 , D3DCOLOR_ARGB(255,128,255,128)  );
		}
	}


	if( menu->showCalibrator ){
		D3DRECT rect;
		rect.x1 = viewportWidth / 2 - 1;
		rect.x2 = viewportWidth / 2 + 1;
		rect.y1 = 0;
		rect.y2 = viewportHeight;
		device->Clear( 1 , &rect , D3DCLEAR_TARGET , D3DCOLOR_ARGB(255,255,255,255) , 0 , 0 );

		rect.x1 = viewportWidth  / 2 - 0.06*viewportWidth;
		rect.x2 = viewportWidth  / 2 + 0.06*viewportWidth;
		rect.y1 = viewportHeight / 2 - 1;
		rect.y2 = viewportHeight / 2 + 1;
		device->Clear( 1 , &rect , D3DCLEAR_TARGET , D3DCOLOR_ARGB(255,255,255,255) , 0 , 0 );

		rect.x1 = viewportWidth  / 2 - 0.06*viewportWidth - 1;
		rect.x2 = viewportWidth  / 2 - 0.06*viewportWidth + 1;
		rect.y1 = viewportHeight / 2 - 32;
		rect.y2 = viewportHeight / 2 + 32;
		device->Clear( 1 , &rect , D3DCLEAR_TARGET , D3DCOLOR_ARGB(255,255,255,255) , 0 , 0 );

		rect.x1 = viewportWidth  / 2 + 0.06*viewportWidth - 1;
		rect.x2 = viewportWidth  / 2 + 0.06*viewportWidth + 1;
		rect.y1 = viewportHeight / 2 - 32;
		rect.y2 = viewportHeight / 2 + 32;
		device->Clear( 1 , &rect , D3DCLEAR_TARGET , D3DCOLOR_ARGB(255,255,255,255) , 0 , 0 );


		drawText(
			"1) Walk up as close as possible\n"
			"   to a 90 degree vertical object\n"
			"   (wall cornet, table, square post)\n"
			"2) Close left eye\n"
			"3) Open  right eye\n"
			"4) Align long vertical line with edge\n"
			"   with mouse or head tracker\n"
			"5) Close right eye\n"
			"6) Open  left  eye\n"
			"7) Adjust \"Separation\" until the same\n"
			"   edge is aligned with small vertical\n"
			"   line in left eye view\n"
			"8) Repeat 2..7 until edge is aligned on both\n"
			"   long line in right eye view and short right\n"
			"   line in left eye view. Separation now configured.\n"
			"9) Open both eyes\n"
			"10) Walk somewhere where there are objects nearby\n"
			"    and far away on screen on same time.\n"
			"11) Adjust \"Covergence\" to a comfort level.\n"
			"    (currently no precise method available)"
			, ALIGN_RIGHT_COLUMN
		);
	}


	sprite->End( );


		/*
	ClearEmptyRect(vireio::RenderPosition::Left, rect, D3DCOLOR_ARGB(255,255,128,128), 2);
	ClearEmptyRect(vireio::RenderPosition::Right, rect, D3DCOLOR_ARGB(255,255,128,128), 2);


	// helper rectangle
	D3DRECT rect0 = D3DRECT(rect);

	setDrawingSide(renderPosition);

	rect0.y2 = rect.y1 + bw;
	actual->Clear(1, &rect0, D3DCLEAR_TARGET, color, 0, 0);

	rect0.y1 = rect.y2 - bw;
	rect0.y2 = rect.y2;

	rect0.y1 = rect.y1;
	rect0.x2 = rect.x1 + bw;
	actual->Clear(1, &rect0, D3DCLEAR_TARGET, color, 0, 0);

	rect0.x1 = rect.x2 - bw;
	rect0.x2 = rect.x2;
	actual->Clear(1, &rect0, D3DCLEAR_TARGET, color, 0, 0);
	*/

}

void cMenu::drawText( const QString& text , int align ){
	RECT rect;
	rect.left   = 0;
	rect.right  = 1920;
	rect.top    = posY;
	rect.bottom = 1080;

	int flags = 0;

	if( align == ALIGN_CENTER ){
		flags |= DT_CENTER;
	}

	if( align == ALIGN_LEFT_COLUMN ){
		rect.left += 500;
	}

	if( align == ALIGN_RIGHT_COLUMN ){
		rect.left += 1200;
	}

	font->DrawTextA( sprite , text.toLocal8Bit() , -1 , &rect , flags , D3DCOLOR_ARGB(255, 64, 64, 64) );
	font->DrawTextA( sprite , text.toLocal8Bit() , -1 , &rect , flags , D3DCOLOR_ARGB(255, 255, 255, 255) );
}


void cMenu::drawRect( int x1 , int y1 , int x2 , int y2 , int color ){
	D3DRECT rect;
	rect.x1 = x1 * viewportWidth  / 1920;
	rect.x2 = x2 * viewportWidth  / 1920;
	rect.y1 = y1 * viewportHeight / 1080;
	rect.y2 = y2 * viewportHeight / 1080;
	device->Clear( 1 , &rect , D3DCLEAR_TARGET , color , 0 , 0 );

}

/*
METHOD_IMPL( void , , D3DProxyDevice , DrawTextShadowed , ID3DXFont* , font , LPD3DXSPRITE , sprite , LPCSTR , lpchText , int , cchText , LPRECT , lprc , UINT , format, D3DCOLOR , color )

}


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
}*/