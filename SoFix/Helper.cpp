#include "Helper.h"
#include "ElfReader.h"
#include "QTextStream"

#define QSTR8BIT(s) (QString::fromLocal8Bit(s))

const Helper::Command Helper::cmdSo =
{
	QSTR8BIT("请选择修复类型:\n1.正常So文件\n2.Dump So文件"),
	2,
	{ elfFixNormalSo , elfFixDumpSo }
};

void Helper::elfFixNormalSo()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString sopath;

	qout << QSTR8BIT("请输入正常so文件路径:") << endl;
	qin >> sopath;

	elfFixSo(sopath, false);
}

void Helper::elfFixDumpSo()
{
	
}

void Helper::elfFixSo(QString &sopath, bool dump)
{
	ElfReader elfReader(sopath.toStdString().c_str(), dump);
	QTextStream qout(stdout);

	if (elfReader.Load() && !dump)
	{
		QString loadedpath = sopath + ".loaded";
		QFile loadedFile(loadedpath);
		if (loadedFile.open(QIODevice::ReadWrite))
		{
			loadedFile.write((char *)elfReader.load_start(), elfReader.load_size());
			qout << QSTR8BIT("加载成功!加载后文件路径: ") + loadedpath << endl;
		}
	}
}