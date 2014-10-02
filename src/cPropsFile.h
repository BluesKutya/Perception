#pragma once
#include "Vireio.h"

class cPropsFile{
public:
	cPropsFile();
	~cPropsFile();

	bool load( QString path );
	bool save( QString path );

	QString getString( const QString& name , bool* ok );
	void    setString( const QString& name , const QString& value );

	void get( bool&           , const QString& );
	void get( int&            , const QString& );
	void get( float&          , const QString& );
	void get( QString&        , const QString& );
	void get( QStringList&    , const QString& );

	void set( bool&          , const QString& );
	void set( int&           , const QString& );
	void set( float&         , const QString& );
	void set( QString&       , const QString& );
	void set( QStringList&   , const QString& );



	template<class T,int S>
	void get( T(&v)[S] , const QString& name ){
		for( int c=0 ; c<S ; c++ ){
			get(v[c] , QString("%1_%2").arg(name).arg(c));
		}
	}

	template<class T,int S>
	void set( T(&v)[S] , const QString& name ){
		for( int c=0 ; c<S ; c++ ){
			set(v[c] , QString("%1_%2").arg(name).arg(c));
		}
	}

private:
	struct Item{
		QString id;
		QString value;
		bool    used;
	};

	QList<Item> items;
};
