#pragma once
#include "linker.h"
#include <QFile>

class ElfFixer
{
private:

	//节头, 下面的这三个变量对应好
	enum ShIdx
	{
		SI_NULL = 0,
		SI_DYNSYM,
		SI_DYNSTR,
		SI_HASH,
		SI_RELDYN,
		SI_RELPLT,
		SI_PLT,
		SI_TEXT,
		SI_ARMEXIDX,
		SI_RODATA,
		SI_FINI_ARRAY,
		SI_INIT_ARRAY,
		SI_DATA_REL_RO,
		SI_DYNAMIC,
		SI_GOT,
		SI_DATA,
		SI_BSS,
		SI_SHSTRTAB,
		SI_MAX
	};

	Elf32_Shdr shdrs_[SI_MAX];
	static const char strtab[];

	static Elf32_Word GetShdrName(int idx);

	char *name_;	//修复后文件名称
	soinfo *si_;	//待修复dump so或由ElfReader加载正常so文件得到的
	QFile file_;

	Elf32_Ehdr ehdr_;	//通过正常的so文件获取

	//通过soinfo获取
	const Elf32_Phdr *phdr_;
	size_t phnum_;

public:
	ElfFixer(soinfo *si, Elf32_Ehdr ehdr, const char *name);
	~ElfFixer();
	bool fix();
	bool write();

private:
	//修复Ehdr
	bool fixEhdr();

	//修复Phdr
	bool fixPhdr();

	//修复整个Shdr, 名称, 文件偏移
	bool fixShdr();

	//从Phdr中修复部分Shdr: .dynamic, .arm.exidx
	bool fixShdrFromPhdr();

	//从.dynamic中修复部分Shdr:  .hash, .dynsym, .dynstr, .rel.dyn, .rel.plt, .init_array, fini_array
	bool fixShdrFromDynamic();

	//根据.dynamic和.dynsym引用的字符串来确定.dynstr节的大小
	bool fixDynstr();

	//根据.hash,.rel.plt,.rel.dyn引用的符号信息来确定.dynsym, .dynstr节的大小
	bool fixDynsym();

	//根据Shdr的关系修复 .plt, .text, .got, .data, .bss
	bool fixShdrFromShdr();

	//将文件中记录的内存地址转为文件偏移, -1表示失败
	Elf32_Off addrToOff(Elf32_Addr addr);

	Elf32_Addr offToAddr(Elf32_Off off);

	//返回文件中记录的内存地址所在的节, -1表示没找到
	int findShIdx(Elf32_Addr addr);
};

