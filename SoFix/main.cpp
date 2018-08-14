#include <QTextStream>
#include <Helper.h>
#include "ElfBuilder.h"

#define QSTR8BIT(s) (QString::fromLocal8Bit(s))

int main(int argc, char *argv[])
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	int input = 0;

	while (true)
	{
		qout << Helper::cmdSo.description << endl;
		qin.flush();
		qin >> input;
		if (input > Helper::cmdSo.cmdCount || input < 1)
		{
			qout << QSTR8BIT("ÎÞÐ§ÊäÈë...") << endl;
			system("pause");
			continue;
		}

		Helper::cmdSo.pfnCmds[input - 1]();
		system("pause");
	}
	
	return 0;
}
