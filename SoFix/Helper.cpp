#include "Helper.h"
#include "ElfReader.h"
#include "QTextStream"
#include "ElfFixer.h"
#include <QSettings>
#include "Util.h"

#define QSTR8BIT(s) (QString::fromLocal8Bit(s))

const Helper::Command Helper::cmdSo =
{
	QSTR8BIT("------------Android Arm so Fix Tool, By Youlor, Reference ThomasKing------------\n"
	"请选择修复文件类型:\n1.正常So文件\n2.Dump So文件(需正常so文件辅助修复)"
	"\n3.Dump So文件(直接修复)"),
	3,
	{ elfFixNormalSo , elfFixDumpSoFromNormal, elfFixDumpSo }
};

void Helper::elfFixNormalSo()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString sopath;

	qout << QSTR8BIT("请输入待修复的正常so文件路径:") << endl;
	qin >> sopath;

	elfFixSo(sopath.toLocal8Bit(), nullptr);
}

void Helper::elfFixDumpSoFromNormal()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString sopath;
	QString dumppath;

	qout << QSTR8BIT("请输入用于辅助修复的正常so文件路径:") << endl;
	qin >> sopath;

	qout << QSTR8BIT("请输入待修复的dump so文件路径:") << endl;
	qin >> dumppath;

	elfFixSo(sopath.toLocal8Bit(), dumppath.toLocal8Bit());
}

bool Helper::elfDumpSoToNormal(QString &dumppath)
{
	QString normalpath;
	QTextStream qout(stdout);

	normalpath = dumppath + ".normal";
	QFile normalFile(normalpath);

	ElfReader elf_reader(nullptr, dumppath.toLocal8Bit());
	if (elf_reader.Load() && normalFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		const Elf32_Phdr* phdr = elf_reader.loaded_phdr();
		const Elf32_Phdr* phdr_limit = phdr + elf_reader.phdr_count();
		Elf32_Addr load_bias = (Elf32_Addr)elf_reader.load_bias();

		for (phdr = elf_reader.loaded_phdr(); phdr < phdr_limit; phdr++)
		{
			if (phdr->p_type != PT_LOAD)
			{
				continue;
			}

			Elf32_Addr seg_start = phdr->p_vaddr + load_bias;	//内存映射实际起始地址
			Elf32_Addr seg_end = seg_start + phdr->p_memsz;	//内存映射实际终止地址

			Elf32_Addr seg_page_start = PAGE_START(seg_start); //内存映射页首地址
			Elf32_Addr seg_page_end = PAGE_END(seg_end);	   //内存映射终止页

			Elf32_Addr seg_file_end = seg_start + phdr->p_filesz; //文件映射终止页

			// File offsets.
			Elf32_Addr file_start = phdr->p_offset;
			Elf32_Addr file_end = file_start + phdr->p_filesz;

			Elf32_Addr file_page_start = PAGE_START(file_start); //文件映射页首地址
			Elf32_Addr file_length = file_end - file_page_start; //文件映射大小

			//如果没有可写权限, 修正文件的实际映射大小
			if ((phdr->p_flags & PF_W) == 0)
			{
				seg_file_end = PAGE_END(seg_file_end);
				file_end = PAGE_END(file_end);
				file_length = PAGE_END(file_length);
			}

			if (file_length != 0) //从内存写回文件
			{
				normalFile.seek(file_page_start);
				normalFile.write((char *)seg_page_start, file_length);
			}
		}
	}

	normalFile.seek(0);
	normalFile.write((char *)&elf_reader.header(), sizeof(Elf32_Ehdr));
	normalFile.close();
	qout << QSTR8BIT("还原为文件so成功: ") + normalpath << endl;
	return true;
}

void Helper::elfFixDumpSo()
{
	QTextStream qout(stdout);
	QTextStream qin(stdin);
	QString dumppath;
	QString normalpath;

	qout << QSTR8BIT("请输入待修复的dump so文件路径:") << endl;
	qin >> dumppath;
	normalpath = dumppath + ".normal";

	//将dump so转为正常文件so, 再调用elfFixNormalSo修复节-_-
	//额...越修越坏 算了-_- 文件so还能看~_~
	if (elfDumpSoToNormal(dumppath))
	{
		//elfFixSo(normalpath.toLocal8Bit(), nullptr);
	}
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
		if (loadedFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
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
			else
			{
				qout << QSTR8BIT("so修复失败, 可能不是有效的so文件(不包含DT_DYNAMIC, DT_HASH, DT_STRTAB, DT_SYMTAB)") + fixedpath << endl;
			}
		}
	}
	else
	{
		qout << QSTR8BIT("so加载失败, 可能不是有效的so文件") << endl;
	}
	return true;
}
