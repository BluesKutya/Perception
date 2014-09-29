#include "cProfileSelectDialog.h"
#include <Vireio.h>


cProfileSelectDialog::cProfileSelectDialog( QWidget* parent ) : QDialog(parent) {
	ui.setupUi(this);

	for( QString& s : vireio_global_config.getAvailableProfiles() ){
		ui.profile->addItem( s );
	}
}



void cProfileSelectDialog::on_ok_clicked(){
	selectedProfileName = ui.profile->currentText();

	if( selectedProfileName.isEmpty() ){
		reject();
	}else{
		//TODO: add check if profile is compatible
		accept();
	}
}

