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

	elfFixSo(sopath.toLocal8Bit().toStdString().c_str(), sopath.toLocal8Bit().toStdString().c_str());
}

void Helper::elfFixDumpSo()
{
	
}

bool Helper::elfFixSo(const char *sopath, const char *fixpath)
{
	ElfReader elf_reader(sopath);
	QTextStream qout(stdout);

	if (elf_reader.Load())
	{
		QString loadedpath = QSTR8BIT(sopath) + ".loaded";
		QFile loadedFile(loadedpath);
		if (loadedFile.open(QIODevice::ReadWrite))
		{
			loadedFile.write((char *)elf_reader.load_start(), elf_reader.load_size());
			qout << QSTR8BIT("加载成功!加载后文件路径: ") + loadedpath << endl;

			const char* bname = strrchr(sopath, '\\');//返回最后一次出现"/"之后的字符串, 即获取不含路径的文件名
			soinfo* si = soinfo_alloc(bname ? bname + 1 : sopath);//分配soinfo内存, 将内存初始化为0, name拷贝
			if (si == NULL) 
			{
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

			ElfFixer elf_fixer(si, elf_reader.ehdr(), fixpath);
			elf_fixer.fix();
		}
	}

	return true;
}