#include "cMenu.h"
#include "D3DProxyDevice.h"


cHotkey::cHotkey(){
	input = 0;
}

bool cHotkey::active( const QVector<int>& down ){
	if( codes.isEmpty() ){
		return false;
	}

	for( int c : codes ){
		if( !down.contains(c) ){
			return false;
		}
	}

	return true;
}


QString cHotkey::toString(){
	QString s;
	for( int c : codes ){
		if( !s.isEmpty() ){
			s += " + ";
		}
		s += QString::fromStdString( input->GetKeyName(c) );
	}
	return s;
}


QString cHotkey::toCodeString(){
	QString s;
	for( int c : codes ){
		if( !s.isEmpty() ){
			s += "+";
		}
		s += QString::number(c);
	}
	return s;
}


void cHotkey::fromCodeString( const QString& string ){
	codes.clear();
	for( QString& s : string.split("+") ){
		int key = s.toInt();
		if( key ){
			codes += key;
		}
	}
}


void cHotkey::clear(){
	codes.clear();
}


bool cHotkey::valid(){
	return !codes.isEmpty();
}


bool cHotkey::listen(){
	bool ret = false;

	if( input ){
		for( int c=0 ; c<256 ; c++ ){
			if( input->Key_Down(c) ){
				ret = true;
				if( c!=	0x10 && c!= 0x11 ){ //exclude Ctrl and Shift buttons (LCtrl and LShift is used)
					if( !codes.contains(c) ){
						codes += c;
					}
				}
			}
		}
	}

	return ret;
}









cMenuItem::cMenuItem(){
	parent         = 0;
	selected       = 0;
	showCalibrator = false;
	visible        = true;
	hotkey.input   = 0;
	readOnly       = false;
}


cMenuItem::~cMenuItem(){
	while( !children.isEmpty() ){
		delete children.first();
	}

	if( parent ){
		if( parent->selected == this ){
			parent->selected = 0;
		}
		parent->children.removeAll( this );
	}
}


cMenuItem* cMenuItem::add( const QString& n , TYPE t ){
	cMenuItem* i = new cMenuItem;
	i->parent       = this;
	i->name         = n;
	i->type         = t;
	i->hotkey.input = hotkey.input;

	int index = config.hotkey_uid.indexOf( i->path() );
	if( index >= 0 ){
		i->hotkey.fromCodeString( config.hotkey_codes[index] );
	}

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
	i->checkVar = variable ? variable : &i->internalBool;
	i->checkOn  = on_text;
	i->checkOff = off_text;
	return i;

}

cMenuItem* cMenuItem::addSelect  ( const QString& n , int* v , const QStringList& l ){
	cMenuItem* i = add( n , SELECT );
	i->selectVar      = v ? v : &i->internalInt;
	i->selectVariants = l;
	return i;
}

cMenuItem* cMenuItem::addText( const QString& n , QString* v ){
	cMenuItem* i = add( n , TEXT );
	i->textVar      = v;
	return i;
}



QString cMenuItem::path( ){
	if( parent ){
		return parent->path() + "/" + name;
	}
	return name;
}


void cMenuItem::trigger( float k ){
	switch( type ){
	case SUBMENU:
		break;

	case ACTION:
		break;

	case CHECKBOX:
		if( !readOnly ){
			*checkVar = !*checkVar;
		}
		break;

	case SPINNER:
		if( !readOnly ){
			*spinVar += spinStep * k;

			if( spinLimit ){
				if( *spinVar > spinMax ){
					*spinVar = spinMax;
				}

				if( *spinVar < spinMin ){
					*spinVar = spinMin;
				}
			}
		}
		break;

	case SELECT:
		if( !readOnly ){
			(*selectVar) += (int)k;

			if( (*selectVar) < 0 ){
				(*selectVar) = selectVariants.count()-1;
			}

			if( (*selectVar) >= selectVariants.count() ){
				(*selectVar) = 0;
			}
		}
		break;
	}

	if( callback ){
		auto copy = callback;
		copy();
	}
}






void cMenuItem::removeChildren( ){
	selected = 0;
	QList<cMenuItem*> del = children;
	for( cMenuItem* i : del ){
		delete i;
	}
	children.clear();
}





void cMenu::init( D3DProxyDevice* d ){
	root.name         = "Vireio Main Menu";
	root.type         = cMenuItem::SUBMENU;
	root.parent       = 0;
	root.hotkey.input = &d->controls;

	hotkeyNew.input   = &d->controls;
	
	device            = d;
	prevKeyDown       = false;
	hotkeyState       = 0;
	menu              = &root;
	show              = false;

	font   = 0;
	sprite = 0;
	scrollY = 0;
	keyDelay       = 0;
	
}


void cMenu::showMessage    ( const QString& text ){
	messageText = text;
	messageTimeout.start();
}


void cMenu::goToMenu( cMenuItem* item ){
	menu = item;
	updateSelected();
}



void cMenu::render( ){
	if( !font || !sprite ){
		return;
	}

	if( config.showVRMouse ){
		POINT pt;   
		GetCursorPos(&pt);
		
		ScreenToClient( device->windowHandle , &pt );

		D3DRECT rec2;	
		rec2.x1 = pt.x - 5;
		rec2.x2 = rec2.x1 + 10; 
		rec2.y1 = pt.y - 5;
		rec2.y2 = rec2.y1 + 10; 	
		
		device->Clear( 1 , &rec2 , D3DCLEAR_TARGET , D3DCOLOR_ARGB(255,255,255,255) , 0 , 0 );

		rec2.x1 += 2;
		rec2.x2 -= 2;
		rec2.y1 += 2;
		rec2.y2 -= 2;

		device->Clear( 1 , &rec2 , D3DCLEAR_TARGET , D3DCOLOR_ARGB(255,0,0,0) , 0 , 0 );
	}


	newKeyDown = false;
	
	updateSelected();


	if( show && hotkeyState ){
		if( hotkeyState==1 ){
			if( !hotkeyNew.listen() ){
				hotkeyNew.clear();
				hotkeyState = 2;
			}
		}else
		if( hotkeyState==2 ){
			if( !hotkeyNew.listen() && (hotkeyNew.valid() || hotkeyTimeout.elapsed()>2000) ){
				menu->selected->hotkey = hotkeyNew;
				hotkeyState = 0;
			}
		}
	}


	if( !hotkeyState && device->controls.Key_Down( VK_ADD ) ){
		newKeyDown = true;
		if( !prevKeyDown ){
			show = !show;
		}
	}


	if( !show && !hotkeyState ){
		QVector<int> down;
		for( int c=0 ; c<256 ; c++ ){
			if( device->controls.Key_Down(c) ){
				down += c;
			}
		}
		checkHotkeys( &root , down );
	}


	if( show && !hotkeyState && device->controls.Key_Down( VK_SUBTRACT ) ){
		newKeyDown = true;
		if( !prevKeyDown ){
			//if( menu->callbackCloseSubmenu ){
			//	menu->callbackCloseSubmenu();
			//}

			if( menu->parent ){
				menu = menu->parent;
			}else{
				show = false;
				return;
			}
		}
	}

	
	if( show && !hotkeyState && menu->selected ){
		int        move_y = 0;
		int        move_x = 0;
		cMenuItem* sel    = menu->selected;


		if( device->controls.Key_Down( VK_NUMPAD2 ) ){
			newKeyDown = true;
			if( !prevKeyDown || keyTime.elapsed()>500 ){
				move_y = -1;
			}
		}


		if( device->controls.Key_Down( VK_NUMPAD8 ) ){
			newKeyDown = true;
			if( !prevKeyDown || keyTime.elapsed()>500 ){
				move_y = +1;
			}
		}


		if( device->controls.Key_Down( VK_NUMPAD5 ) ){
			newKeyDown = true;
			if( !prevKeyDown ){
				if( sel->type == cMenuItem::SUBMENU ){
					menu = sel;
				}
				move_x = +1;
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


		if( device->controls.Key_Down( VK_MULTIPLY ) ){
			newKeyDown = true;
			if( !prevKeyDown ){
				hotkeyState = 1;
				hotkeyTimeout.start();
			}
		}

		if( move_y ){
			int i = menu->children.indexOf( sel );

			if( i < 0 ){
				menu->selected = 0;
			}else{
				for(;;){
					i -= move_y;
					if( i < 0 || i >= menu->children.count() ){
						break;
					}

					if( menu->children[i]->visible ){
						menu->selected = menu->children[i];
						break;
					}
				}
			}
		}


		if( move_x ){
			if( sel->type == cMenuItem::SPINNER ){
				float k = move_x;
				
				if( device->controls.Key_Down(VK_LCONTROL) ){
					k *= 0.1;
				}

				if( device->controls.Key_Down(VK_LSHIFT) ){
					k *= 10;
				}

				sel->trigger( k );

			}else{
				if( !prevKeyDown ){
					sel->trigger( move_x );
				}
			}

			updateSelected();
		}
	}

	if( newKeyDown && !prevKeyDown ){
		keyTime.start();
	}
	prevKeyDown = newKeyDown;

	/*if( show ){
		if( menu->callbackRender ){
			menu->callbackRender();
		}
	}*/

	if( messageText.isEmpty() && !show ){
		return;
	}


	sprite->Begin( D3DXSPRITE_ALPHABLEND );

	D3DXMATRIX matScale;
	D3DXMatrixScaling( &matScale , viewportWidth / 1920.0 , viewportHeight / 1080.0 , 1.0 );
	sprite->SetTransform( &matScale );

	if( show ){
		if( menu->children.isEmpty() ){
			scrollY = 0;
		}

		posY = 300 - scrollY;

		if( hotkeyState ){
			drawText( menu->selected->name , ALIGN_CENTER );
		}else{
			drawText( menu->name , ALIGN_CENTER );
		}
	
		posY += 40;
		drawRect( 0 , posY , 1920 , posY+3 , D3DCOLOR_ARGB(255,255,128,128)  );
	}

	if( show && hotkeyState ){
		QString s = "Key combination: ";
		if( hotkeyNew.valid() ){
			s += hotkeyNew.toString() ;
		}else{
			s += "<wait to clear hotkey>";
		}
		drawText( s , ALIGN_LEFT_COLUMN );
	}


	if( show && !hotkeyState ){

	
		for( cMenuItem* i : menu->children ){
			if( !i->visible ){
				continue;
			}

			if( i == menu->selected ){
				drawRect( 0 , posY+2 , 1920 , posY + 4 , D3DCOLOR_ARGB(255,128,255,128)  );

				if( posY > 800 ){
					scrollY += posY - 800;
				}

				if( posY < 300 ){
					scrollY += posY - 300;
				}
			}


			if( i->hotkey.valid() ){
				drawText( i->hotkey.toString() , ALIGN_HOTKEY_COLUMN );
			}

			if( posY > -64 && posY < 1144 ){
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
				}else
				if( i->type == cMenuItem::SELECT ){
					drawText( i->selectVariants[ *i->selectVar ] , ALIGN_RIGHT_COLUMN );
				}else
				if( i->type == cMenuItem::TEXT ){
					drawText( *i->textVar , ALIGN_RIGHT_COLUMN );
				}
			}

			posY += 40;

			if( i == menu->selected ){
				drawRect( 0 , posY-2 , 1920 , posY + 2 , D3DCOLOR_ARGB(255,128,255,128)  );
			}
		}
	}

	if( !hotkeyState && menu->showCalibrator ){
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

	if( !messageText.isEmpty() ){
		if( messageTimeout.elapsed() > 5000 ){
			messageText.clear();
		}else{
			posY = 600;
			drawText( messageText , ALIGN_CENTER );
		}
	}


	sprite->End( );
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

	
	if( align == ALIGN_HOTKEY_COLUMN ){
		rect.left += 300;
	}

	if( align == ALIGN_LEFT_COLUMN ){
		rect.left += 500;
	}

	if( align == ALIGN_RIGHT_COLUMN ){
		rect.left += 1200;
	}

	rect.left   += 2;
	rect.right  += 2;
	rect.top    += 2;
	rect.bottom += 2;
	font->DrawTextA( sprite , text.toLocal8Bit() , -1 , &rect , flags , D3DCOLOR_ARGB(255, 64, 64, 64) );

	rect.left   -= 2;
	rect.right  -= 2;
	rect.top    -= 2;
	rect.bottom -= 2;
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



void cMenu::checkHotkeys( cMenuItem* i , const QVector<int>& down ){
	for( cMenuItem* c : i->children ){
		checkHotkeys( c , down );
	}

	if( i->hotkey.active( down ) ){
		newKeyDown = true;

		if( !prevKeyDown ){
			printf("HOTKEY %s\n" , i->name.toLocal8Bit().data() );
			i->trigger( +1 );
		}
	}
}


void cMenu::updateSelected( ){
	if( !menu->selected && !menu->children.isEmpty() ){
		menu->selected = menu->children.first();
	}

	if( menu->selected && !menu->selected->visible ){
		menu->selected = 0;
		for( cMenuItem* i : menu->children ){
			if( i->visible ){
				menu->selected = i;
				break;
			}
		}
	}

}

void cMenu::createResources(){
	freeResources();
	D3DXCreateFontA(  device, 32, 0, FW_BOLD, 4, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &font );
	D3DXCreateSprite( device, &sprite);
}


void cMenu::freeResources(){
	if( font ) {
		font->Release();
		font = 0;
	}

	if( sprite ) {
		sprite->Release();
		sprite = 0;
	}
}




void cMenu::saveHotkeys( cMenuItem* item ){
	if( !item ){
		config.hotkey_uid  .clear();
		config.hotkey_codes.clear();
		item = &root;
	}

	if( item->hotkey.valid() ){
		config.hotkey_uid   += item->path();
		config.hotkey_codes += item->hotkey.toCodeString();
	}

	for( cMenuItem* c : item->children ){
		saveHotkeys( c );
	}
}