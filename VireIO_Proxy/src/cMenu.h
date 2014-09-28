#pragma once
#include <Vireio.h>
#include <qstring.h>
#include <d3dx9.h>
#include <functional>

class D3DProxyDevice;

class cMenuItem{
public:
	bool                  showCalibrator;
	std::function<void()> callbackOpenSubmenu;
	std::function<void()> callbackCloseSubmenu;
	std::function<void()> callbackValueChanged;
	std::function<void()> callbackRender;

	cMenuItem* addSubmenu ( const QString& name );
	cMenuItem* addAction  ( const QString& name );
	cMenuItem* addSpinner ( const QString& name , float* variable , float min , float max , float step );
	cMenuItem* addSpinner ( const QString& name , float* variable , float step );
	cMenuItem* addCheckbox( const QString& name , bool*  variable , const QString& on_text="true" , const QString& off_text="false" );

	void clear();

private:
	enum TYPE{
		SUBMENU,
		ACTION,
		SPINNER,
		CHECKBOX
	};
	
	QString               name;
	QList<cMenuItem*>     children;
	cMenuItem*            parent;
	cMenuItem*            selected;
	TYPE                  type;
	bool                  spinLimit;
	float                 spinMin;
	float                 spinMax;
	float                 spinStep;
	float*                spinVar;
	bool*                 checkVar;
	QString               checkOn;
	QString               checkOff;

	cMenuItem();

	cMenuItem* add( const QString& name , TYPE type );

	friend class cMenu;
};



class cMenu {
public:
	D3DProxyDevice* device;
	ID3DXSprite*    sprite;
	ID3DXFont*      font;
	int             viewportWidth;
	int             viewportHeight;
	cMenuItem       root;
	bool            prevKeyDown;
	bool            showOldMenu;
	bool            show;
	
	
	cMenu();
	void render ();

private:
	enum{
		ALIGN_CENTER,
		ALIGN_LEFT_COLUMN,
		ALIGN_RIGHT_COLUMN
	};

	int        posY;
	cMenuItem* menu;
	void drawText( const QString& text , int align );
	void drawRect( int x1 , int y1 , int x2 , int y2 , int color );
};




