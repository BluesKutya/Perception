#include <qapplication.h>
#include "cMainWindow.h"
#include <Vireio.h>

int main(int argc, char **argv){
	QApplication a(argc, argv);

	vireio_global_config.vireioDir = a.applicationDirPath() + "/../";

	vireio_global_config.load( vireio_global_config.getMainConfigFile() );
	
	QCoreApplication::setOrganizationName( "Vireio" );
	QCoreApplication::setApplicationName ( "Perception" );

	cMainWindow w;
	w.show();

	return a.exec();
}
