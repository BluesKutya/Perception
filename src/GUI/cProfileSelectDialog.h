#include <Ui_cProfileSelectDialog.h>
#include <QObject>

class cProfileSelectDialog : public QDialog {
Q_OBJECT
public:
	QString selectedProfileName;
	QString exeName;

	cProfileSelectDialog( QWidget* parent );

private:
	Ui_cProfileSelectDialog ui;

private slots:
	void on_ok_clicked();
};
