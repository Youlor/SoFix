#pragma once
#include "linker.h"
#include <QFile>

class ElfFixer
{
private:
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
		SI_FINI_ARRAY,
		SI_INIT_ARRAY,
		SI_DYNAMIC,
		SI_GOT,
		SI_DATA,
		SI_BSS,
		SI_SHSTRTAB
	};

	static const char strtab[];
	static Elf32_Word GetShdrName(int idx);

	char *name_;	//待修复文件
	soinfo *si_;
	QFile file_;

	Elf32_Ehdr ehdr_;	//通过正常的so文件获取

	//通过soinfo获取
	const Elf32_Phdr *phdr_;
	size_t phnum_;

	//通过soinfo中的phdr_直接获取 PT_DYNAMIC, PT_ARM_EXIDX
	Elf32_Shdr dynamic_;
	Elf32_Shdr arm_exidex;

	//遍历dynamic_获取
	Elf32_Shdr hash_;
	Elf32_Shdr dynsym_;
	Elf32_Shdr dynstr_;
	Elf32_Shdr rel_dyn_;
	Elf32_Shdr rel_plt_;
	Elf32_Shdr init_array_;
	Elf32_Shdr fini_array_;

	//通过节的关系或特征获取
	Elf32_Shdr plt_;
	Elf32_Shdr got_;
	Elf32_Shdr data_;
	Elf32_Shdr bss_;
	Elf32_Shdr text_;

	//SHT_UNDEF
	Elf32_Shdr shnull_;

	//.shstrtab
	Elf32_Shdr shstrtab_;

public:
	ElfFixer(soinfo *si, Elf32_Ehdr ehdr, const char *name);
	~ElfFixer();
	bool fix();

private:
	//修复Ehdr
	bool fixEhdr();

	//修复Phdr
	bool fixPhdr();

	//修复整个Shdr, 名称, 文件偏移
	bool fixShdr();

	bool showAll();

	//从Phdr中修复部分Shdr: .dynamic, .arm.exidx
	bool fixShdrFromPhdr();

	//从.dynamic中修复部分Shdr
	bool fixShdrFromDynamic();

	//根据Shdr的关系修复
	bool fixShdrFromShdr();

	//将内存地址转为文件偏移, -1表示失败
	Elf32_Off addrToOff(Elf32_Addr addr);
};

