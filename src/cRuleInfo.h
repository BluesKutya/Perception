#pragma once
#include <qstring.h>
#include <qstringlist.h>

struct cRuleInfo{
	QString         name;
	QStringList     constants;
	QStringList     shadersInclude;
	QStringList     shadersExclude;
	QString         operationName;
	bool            transpose;
};

