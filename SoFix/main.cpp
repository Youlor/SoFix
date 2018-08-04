#include <QTextStream>
#include <Helper.h>

int main(int argc, char *argv[])
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	int input = 0;

	qout << Helper::cmdSo.description << endl;
	qin >> input;

	if (input > Helper::cmdSo.cmdCount || input < 1)
	{
		return 0;
	}

	Helper::cmdSo.pfnCmds[input - 1]();
	system("pause");
	return 0;
}
