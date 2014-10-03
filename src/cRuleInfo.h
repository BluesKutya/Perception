#pragma once
#include <qstring.h>
#include <qstringlist.h>

struct cRuleInfo{
	QString         name;
	QStringList     constantsInclude;
	QStringList     shadersInclude;
	QStringList     shadersExclude;
	QString         operationName;
	bool            isMatrixRule;
	bool            transpose;
	bool            squishViewport;
};

