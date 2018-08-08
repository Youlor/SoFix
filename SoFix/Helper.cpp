#include "Helper.h"
#include "ElfReader.h"
#include "QTextStream"
#include "ElfFixer.h"

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

	elfFixSo(sopath.toLocal8Bit(), nullptr);
}

void Helper::elfFixDumpSo()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString sopath;
	QString dumppath;

	qout << QSTR8BIT("请输入正常so文件路径:") << endl;
	qin >> sopath;

	qout << QSTR8BIT("请输入dump so文件路径:") << endl;
	qin >> dumppath;

	elfFixSo(sopath.toLocal8Bit(), dumppath.toLocal8Bit());
}

//如果dumppath为空则直接修复正常so文件, 否则根据根据正常so文件修复dump文件
bool Helper::elfFixSo(const char *sopath, const char *dumppath)
{
	const char *name = dumppath ? dumppath : sopath;
	ElfReader elf_reader(sopath, dumppath);
	QTextStream qout(stdout);

	if (elf_reader.Load())
	{
		QString loadedpath = QSTR8BIT(name) + ".loaded";
		QFile loadedFile(loadedpath);
		if (loadedFile.open(QIODevice::ReadWrite))
		{
			loadedFile.write((char *)elf_reader.load_start(), elf_reader.load_size());
			qout << QSTR8BIT("加载成功!加载后文件路径: ") + loadedpath << endl;

			const char* bname = strrchr(name, '\\');//返回最后一次出现"/"之后的字符串, 即获取不含路径的文件名
			soinfo* si = soinfo_alloc(bname ? bname + 1 : name);//分配soinfo内存, 将内存初始化为0, name拷贝
			if (si == NULL)
			{
				qout << QSTR8BIT("抱歉, 文件名不能超过128个字符!") << endl;
				return NULL;
			}

			//初始化soinfo的其他字段
			si->base = elf_reader.load_start();
			si->size = elf_reader.load_size();
			si->load_bias = elf_reader.load_bias();
			si->flags = 0;
			si->entry = 0;
			si->dynamic = NULL;
			si->phnum = elf_reader.phdr_count();
			si->phdr = elf_reader.loaded_phdr();

			QString fixedpath = QSTR8BIT(name) + ".fixed";
			
			ElfFixer elf_fixer(si, sopath, fixedpath.toLocal8Bit());
			if (elf_fixer.fix() && elf_fixer.write())
			{
				qout << QSTR8BIT("修复成功!修复后文件路径: ") + fixedpath << endl;
			}
		}
	}
	else
	{
		qout << QSTR8BIT("so加载失败, 可能不是有效的so文件") << endl;
	}
	return true;
}