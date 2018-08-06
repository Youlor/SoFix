#include "ElfFixer.h"
#include "linker.h"
#include <QDebug>
#include "Util.h"

#define QSTR8BIT(s) (QString::fromLocal8Bit(s))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define DEBUG qDebug
#define DL_ERR qDebug

const char ElfFixer::strtab[] =
"\0.dynsym\0.dynstr\0.hash\0.rel.dyn\0.rel.plt\0.plt\0.text\0.ARM.exidx\0"
".fini_array\0.init_array\0.dynamic\0.got\0.data\0.bss\0.shstrtab\0";

Elf32_Word ElfFixer::GetShdrName(int idx)
{
	Elf32_Word ret = 0;
	const char *str = strtab;
	for (int i = 0; i < idx; i++)
	{
		ret += strlen(str) + 1;
		str += strlen(str) + 1;
	}

	return ret;
}

ElfFixer::ElfFixer(soinfo *si, Elf32_Ehdr ehdr, const char *name)
	: si_(si), ehdr_(ehdr), phdr_(nullptr), phnum_(0)
{
	if (name)
	{
		name_ = strdup(name);
		file_.setFileName(QSTR8BIT(name));
	}

	memset(shdrs_, 0, sizeof(shdrs_));
}

ElfFixer::~ElfFixer()
{
	if (name_)
	{
		free(name_);
	}
	file_.close();
}

bool ElfFixer::fix()
{
	return fixPhdr() &&
		fixEhdr() &&
		fixShdr() &&
		showAll();
}

bool ElfFixer::write()
{
	if (!file_.open(QIODevice::ReadWrite))
	{
		return false;
	}

	const Elf32_Phdr* phdr = phdr_;
	const Elf32_Phdr* phdr_limit = phdr + phnum_;

	//考虑到段数据可能运行时被改变, 这里从dump后文件中读取
	for (phdr = phdr_; phdr < phdr_limit; phdr++)
	{
		file_.seek(PAGE_START(phdr->p_offset));
		file_.write((char *)(PAGE_START(si_->load_bias + phdr->p_vaddr)), PAGE_END(phdr->p_filesz));
	}

	//Elf头部应该在段数据写入之后写入, 避免被段数据覆盖
	file_.seek(0);
	file_.write((char *)&ehdr_, sizeof(Elf32_Ehdr));
	file_.seek(ehdr_.e_phoff);
	file_.write((char *)phdr_, ehdr_.e_phnum + sizeof(Elf32_Phdr));

	//写入节头
	file_.seek(ehdr_.e_shoff);
	file_.write((char *)&shdrs_[SI_NULL], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_DYNSYM], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_DYNSTR], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_HASH], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_RELDYN], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_RELPLT], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_PLT], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_TEXT], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_ARMEXIDX], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_INIT_ARRAY], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_FINI_ARRAY], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_DYNAMIC], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_GOT], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_DATA], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_BSS], sizeof(Elf32_Shdr));
	file_.write((char *)&shdrs_[SI_SHSTRTAB], sizeof(Elf32_Shdr));

	//写入节头名称
	file_.seek(shdrs_[SI_SHSTRTAB].sh_offset);
	file_.write(strtab, shdrs_[SI_SHSTRTAB].sh_size);

	return true;
}

bool ElfFixer::fixEhdr()
{
	//将shdr放在可加载段数据的尾部
	const Elf32_Phdr* phdr = phdr_;
	const Elf32_Phdr* phdr_limit = phdr + phnum_;

	Elf32_Off max_off = 0;

	for (phdr = phdr_; phdr < phdr_limit; phdr++)
	{
		if (PAGE_END(phdr->p_offset + phdr->p_filesz) > max_off)
		{
			max_off = PAGE_END(phdr->p_offset + phdr->p_filesz);
		}
	}

	ehdr_.e_shoff = max_off;
	ehdr_.e_shentsize = sizeof(Elf32_Shdr);
	ehdr_.e_shnum = SI_MAX;
	ehdr_.e_shstrndx = SI_SHSTRTAB;

	//在shdr尾部加个节名称表
	shdrs_[SI_SHSTRTAB].sh_name = GetShdrName(SI_SHSTRTAB);
	shdrs_[SI_SHSTRTAB].sh_type = SHT_STRTAB;
	shdrs_[SI_SHSTRTAB].sh_flags = 0;
	shdrs_[SI_SHSTRTAB].sh_addr = 0;
	shdrs_[SI_SHSTRTAB].sh_offset = ehdr_.e_shoff + ehdr_.e_shnum * sizeof(Elf32_Shdr);
	shdrs_[SI_SHSTRTAB].sh_size = sizeof(strtab);
	shdrs_[SI_SHSTRTAB].sh_link = 0;
	shdrs_[SI_SHSTRTAB].sh_info = 0;
	shdrs_[SI_SHSTRTAB].sh_addralign = 1;
	shdrs_[SI_SHSTRTAB].sh_entsize = 0;

	return true;
}

bool ElfFixer::fixPhdr()
{
	if (si_)
	{
		phdr_ = si_->phdr;
		phnum_ = si_->phnum;
		return true;
	}

	return false;
}

bool ElfFixer::fixShdr()
{
	return fixShdrFromPhdr() &&
		fixShdrFromDynamic() &&
		fixShdrFromShdr();
}

bool ElfFixer::showAll()
{
	DEBUG("Name\tAddr\tOff\tSize\tEs\n");
	for (int i = 0; i < SI_MAX; i++)
	{
		DEBUG("%s\t%08X\t%08X\t%08X\t%08X\n",
			strtab + shdrs_[i].sh_name, shdrs_[i].sh_addr,
			shdrs_[i].sh_offset, shdrs_[i].sh_size, shdrs_[i].sh_entsize);
	}

	return true;
}

bool ElfFixer::fixShdrFromPhdr()
{
	phdr_table_get_dynamic_section(phdr_, phnum_, si_->load_bias, &si_->dynamic, NULL, NULL);
	shdrs_[SI_DYNAMIC].sh_name = GetShdrName(SI_DYNAMIC);
	shdrs_[SI_DYNAMIC].sh_type = SHT_DYNAMIC;
	shdrs_[SI_DYNAMIC].sh_flags = SHF_WRITE | SHF_ALLOC;
	shdrs_[SI_DYNAMIC].sh_addr = (Elf32_Addr)si_->dynamic - si_->load_bias;
	shdrs_[SI_DYNAMIC].sh_offset = addrToOff(shdrs_[SI_DYNAMIC].sh_addr);
	//dynamic_.sh_size = 0;		//遍历到DT_NULL即可确定大小
	shdrs_[SI_DYNAMIC].sh_link = SI_DYNSTR;
	shdrs_[SI_DYNAMIC].sh_info = 0;
	shdrs_[SI_DYNAMIC].sh_addralign = 4;
	shdrs_[SI_DYNAMIC].sh_entsize = sizeof(Elf32_Dyn);

	(void)phdr_table_get_arm_exidx(phdr_, phnum_, si_->load_bias, &si_->ARM_exidx, &si_->ARM_exidx_count);
	if (si_->ARM_exidx)
	{
		shdrs_[SI_ARMEXIDX].sh_name = GetShdrName(SI_ARMEXIDX);
		shdrs_[SI_ARMEXIDX].sh_type = SHT_AMMEXIDX;
		shdrs_[SI_ARMEXIDX].sh_flags = SHF_ALLOC | SHF_LINK_ORDER;
		shdrs_[SI_ARMEXIDX].sh_addr = (Elf32_Addr)si_->ARM_exidx - si_->load_bias;
		shdrs_[SI_ARMEXIDX].sh_offset = addrToOff(shdrs_[SI_ARMEXIDX].sh_addr);
		shdrs_[SI_ARMEXIDX].sh_size = si_->ARM_exidx_count * 8;
		shdrs_[SI_ARMEXIDX].sh_link = SI_TEXT;
		shdrs_[SI_ARMEXIDX].sh_info = 0;
		shdrs_[SI_ARMEXIDX].sh_addralign = 4;
		shdrs_[SI_ARMEXIDX].sh_entsize = 8;
	}

	return true;
}

bool ElfFixer::fixShdrFromDynamic()
{
	/* "base" might wrap around UINT32_MAX. */
	Elf32_Addr base = si_->load_bias;
	shdrs_[SI_DYNAMIC].sh_size = 1;	//DT_NULL
	shdrs_[SI_DYNSTR].sh_size = 0;
	uint32_t needed_count = 0; //记录DT_NEEDED的数量
	// Extract useful information from dynamic section. 抽取动态节中的有用信息，各种DT_
	// 获取各个entry的数据(地址或值), DT_NULL为该节的结束标志

	for (Elf32_Dyn* d = si_->dynamic; d->d_tag != DT_NULL; ++d)
	{
		shdrs_[SI_DYNAMIC].sh_size += sizeof(Elf32_Dyn);	//遍历DT得到dynamic_节准确大小
		switch (d->d_tag)
		{
		case DT_HASH:
			si_->nbucket = ((unsigned *)(base + d->d_un.d_ptr))[0];
			si_->nchain = ((unsigned *)(base + d->d_un.d_ptr))[1];
			si_->bucket = (unsigned *)(base + d->d_un.d_ptr + 8);
			si_->chain = (unsigned *)(base + d->d_un.d_ptr + 8 + si_->nbucket * 4);

			shdrs_[SI_HASH].sh_name = GetShdrName(SI_HASH);
			shdrs_[SI_HASH].sh_type = SHT_HASH;
			shdrs_[SI_HASH].sh_flags = SHF_ALLOC;
			shdrs_[SI_HASH].sh_addr = d->d_un.d_ptr;
			shdrs_[SI_HASH].sh_offset = addrToOff(shdrs_[SI_HASH].sh_addr);
			shdrs_[SI_HASH].sh_size = (2 + si_->nbucket + si_->nchain) * sizeof(Elf32_Word);
			shdrs_[SI_HASH].sh_link = ShIdx::SI_DYNSYM;	//.dynsym的节索引
			shdrs_[SI_HASH].sh_info = 0;
			shdrs_[SI_HASH].sh_addralign = 4;
			shdrs_[SI_HASH].sh_entsize = sizeof(Elf32_Word);
			break;
		case DT_STRTAB:
			si_->strtab = (const char *)(base + d->d_un.d_ptr);

			shdrs_[SI_DYNSTR].sh_name = GetShdrName(SI_DYNSTR);
			shdrs_[SI_DYNSTR].sh_type = SHT_STRTAB;
			shdrs_[SI_DYNSTR].sh_flags = SHF_ALLOC;
			shdrs_[SI_DYNSTR].sh_addr = d->d_un.d_ptr;
			shdrs_[SI_DYNSTR].sh_offset = addrToOff(shdrs_[SI_DYNSTR].sh_addr);
			//shdrs_[SI_DYNSTR].sh_size = 0;	//由于这个字段不是必须的, 由DT_STRSZ获取大小可能不准确
			shdrs_[SI_DYNSTR].sh_link = 0;
			shdrs_[SI_DYNSTR].sh_info = 0;
			shdrs_[SI_DYNSTR].sh_addralign = 1;
			shdrs_[SI_DYNSTR].sh_entsize = 0;
			break;
		case DT_STRSZ:
			//shdrs_[SI_DYNSTR].sh_size = d->d_un.d_val;	//由DT_STRSZ获取大小可能不准确, 后续可以通过引用该节(.symtab, .dynamic)的范围来确定大小
			break;
		case DT_SYMTAB:
			si_->symtab = (Elf32_Sym *)(base + d->d_un.d_ptr);

			shdrs_[SI_DYNSYM].sh_name = GetShdrName(SI_DYNSYM);
			shdrs_[SI_DYNSYM].sh_type = SHT_DYNSYM;
			shdrs_[SI_DYNSYM].sh_flags = SHF_ALLOC;
			shdrs_[SI_DYNSYM].sh_addr = d->d_un.d_ptr;
			shdrs_[SI_DYNSYM].sh_offset = addrToOff(shdrs_[SI_DYNSYM].sh_addr);
			shdrs_[SI_DYNSYM].sh_size = 0;		//后续根据符号HASH表决定
			shdrs_[SI_DYNSYM].sh_link = ShIdx::SI_DYNSTR;	//.dynstr的节索引
			shdrs_[SI_DYNSYM].sh_info = 1;	//最后一个局部符号的符号表索引值加一, 暂时给1
			shdrs_[SI_DYNSYM].sh_addralign = 4;
			shdrs_[SI_DYNSYM].sh_entsize = sizeof(Elf32_Sym);
			break;
		case DT_JMPREL:
			si_->plt_rel = (Elf32_Rel*)(base + d->d_un.d_ptr);

			shdrs_[SI_RELPLT].sh_name = GetShdrName(SI_RELPLT);
			shdrs_[SI_RELPLT].sh_type = SHT_REL;
			shdrs_[SI_RELPLT].sh_flags = SHF_ALLOC | SHF_INFO_LINK;
			shdrs_[SI_RELPLT].sh_addr = d->d_un.d_ptr;
			shdrs_[SI_RELPLT].sh_offset = addrToOff(shdrs_[SI_RELPLT].sh_addr);
			//shdrs_[SI_RELPLT].sh_size = 0;		//根据DT_PLTRELSZ获取准确大小
			shdrs_[SI_RELPLT].sh_link = ShIdx::SI_DYNSYM;	//.dynsym的节索引
			shdrs_[SI_RELPLT].sh_info = ShIdx::SI_GOT;		//.got的节索引
			shdrs_[SI_RELPLT].sh_addralign = 4;
			shdrs_[SI_RELPLT].sh_entsize = sizeof(Elf32_Rel);
			break;
		case DT_PLTRELSZ:
			si_->plt_rel_count = d->d_un.d_val / sizeof(Elf32_Rel);

			shdrs_[SI_RELPLT].sh_size = d->d_un.d_val;	//根据DT_PLTRELSZ获取.rel.plt准确大小
			break;
		case DT_REL:
			si_->rel = (Elf32_Rel*)(base + d->d_un.d_ptr);

			shdrs_[SI_RELDYN].sh_name = GetShdrName(SI_RELDYN);
			shdrs_[SI_RELDYN].sh_type = SHT_REL;
			shdrs_[SI_RELDYN].sh_flags = SHF_ALLOC;
			shdrs_[SI_RELDYN].sh_addr = d->d_un.d_ptr;
			shdrs_[SI_RELDYN].sh_offset = addrToOff(shdrs_[SI_RELDYN].sh_addr);
			//shdrs_[SI_RELDYN].sh_size = 0;	//可以由DT_RELSZ确定准确大小
			shdrs_[SI_RELDYN].sh_link = ShIdx::SI_DYNSYM;	//.dynsym的节索引
			shdrs_[SI_RELDYN].sh_info = 0;
			shdrs_[SI_RELDYN].sh_addralign = 4;
			shdrs_[SI_RELDYN].sh_entsize = sizeof(Elf32_Rel);
			break;
		case DT_RELSZ:
			si_->rel_count = d->d_un.d_val / sizeof(Elf32_Rel);

			shdrs_[SI_RELDYN].sh_size = d->d_un.d_val;	//获取.rel.dyn准确大小
			break;
		case DT_PLTGOT:
			/* Save this in case we decide to do lazy binding. We don't yet. */
			si_->plt_got = (unsigned *)(base + d->d_un.d_ptr);	//_global_offset_table_的虚拟地址, 不是必需信息, 因此只能用作不准确修复
			break;
		case DT_INIT_ARRAY:
			si_->init_array = reinterpret_cast<linker_function_t*>(base + d->d_un.d_ptr);
			DEBUG("%s constructors (DT_INIT_ARRAY) found at %p", si_->name, si_->init_array);

			shdrs_[SI_INIT_ARRAY].sh_name = GetShdrName(SI_INIT_ARRAY);
			shdrs_[SI_INIT_ARRAY].sh_type = SHT_INIT_ARRAY;
			shdrs_[SI_INIT_ARRAY].sh_flags = SHF_WRITE | SHF_ALLOC;
			shdrs_[SI_INIT_ARRAY].sh_addr = d->d_un.d_ptr;
			shdrs_[SI_INIT_ARRAY].sh_offset = addrToOff(shdrs_[SI_INIT_ARRAY].sh_addr);
			//shdrs_[SI_INIT_ARRAY].sh_size = 0;	//可以根据DT_INIT_ARRAYSZ获取准确大小
			shdrs_[SI_INIT_ARRAY].sh_link = 0;
			shdrs_[SI_INIT_ARRAY].sh_info = 0;
			shdrs_[SI_INIT_ARRAY].sh_addralign = 4;
			shdrs_[SI_INIT_ARRAY].sh_entsize = 4;
			break;
		case DT_INIT_ARRAYSZ:
			si_->init_array_count = ((unsigned)d->d_un.d_val) / sizeof(Elf32_Addr);

			shdrs_[SI_INIT_ARRAY].sh_size = d->d_un.d_val;	//获取.init_array的准确大小
			break;
		case DT_FINI_ARRAY:
			si_->fini_array = reinterpret_cast<linker_function_t*>(base + d->d_un.d_ptr);
			DEBUG("%s destructors (DT_FINI_ARRAY) found at %p", si_->name, si_->fini_array);

			shdrs_[SI_FINI_ARRAY].sh_name = GetShdrName(SI_FINI_ARRAY);
			shdrs_[SI_FINI_ARRAY].sh_type = SHT_FINI_ARRAY;
			shdrs_[SI_FINI_ARRAY].sh_flags = SHF_WRITE | SHF_ALLOC;
			shdrs_[SI_FINI_ARRAY].sh_addr = d->d_un.d_ptr;
			shdrs_[SI_FINI_ARRAY].sh_offset = addrToOff(shdrs_[SI_FINI_ARRAY].sh_addr);
			//shdrs_[SI_FINI_ARRAY].sh_size = 0;	//可以根据DT_FINI_ARRAYSZ获取准确大小
			shdrs_[SI_FINI_ARRAY].sh_link = 0;
			shdrs_[SI_FINI_ARRAY].sh_info = 0;
			shdrs_[SI_FINI_ARRAY].sh_addralign = 4;
			shdrs_[SI_FINI_ARRAY].sh_entsize = 4;
			break;
		case DT_FINI_ARRAYSZ:
			si_->fini_array_count = ((unsigned)d->d_un.d_val) / sizeof(Elf32_Addr);

			shdrs_[SI_FINI_ARRAY].sh_size = d->d_un.d_val;	//获取.fini_array的准确大小
			break;

		case DT_NEEDED:
			++needed_count;
			break;
		}

	}

	fixDynsym();
	fixDynstr();

	return true;
}

void ElfFixer::fixDynstr()
{
	//根据.dynamic引用的字符串来确定.dynstr节的大小
	for (Elf32_Dyn* d = si_->dynamic; d->d_tag != DT_NULL; ++d)
	{
		if (d->d_tag == DT_NEEDED)
		{
			const char* library_name = si_->strtab + d->d_un.d_val; //so库名称
			shdrs_[SI_DYNSTR].sh_size = MAX(shdrs_[SI_DYNSTR].sh_size, d->d_un.d_val + strlen(library_name) + 1);
		}
	}
}

bool ElfFixer::fixDynsym()
{
	shdrs_[SI_DYNSYM].sh_size = sizeof(Elf32_Sym);
	shdrs_[SI_DYNSYM].sh_info = 1;
	//通过.rel.plt中引用的符号来确定dynsym, dynstr的节大小
	{
		Elf32_Rel* rel = si_->plt_rel;//si_->rel
		unsigned count = si_->plt_rel_count;

		Elf32_Sym* symtab = si_->symtab;
		const char* strtab = si_->strtab;
		Elf32_Sym* s;

		//遍历重定位节 DT_JMPREL/DT_REL
		for (size_t idx = 0; idx < count; ++idx, ++rel)
		{
			unsigned type = ELF32_R_TYPE(rel->r_info);
			unsigned sym = ELF32_R_SYM(rel->r_info);
			char* sym_name = NULL;
			if (type == 0) // R_*_NONE
			{
				continue;
			}
			if (sym != 0)
			{
				sym_name = (char *)(strtab + symtab[sym].st_name); //获取符号名
				shdrs_[SI_DYNSYM].sh_size = MAX(shdrs_[SI_DYNSYM].sh_size, (sym + 1) * sizeof(Elf32_Sym));
				shdrs_[SI_DYNSTR].sh_size = MAX(shdrs_[SI_DYNSTR].sh_size, symtab[sym].st_name + strlen(sym_name) + 1);
			}

		}
	}

	//通过.rel.dyn中引用的符号来确定dynsym, dynstr的节大小
	{
		Elf32_Rel* rel = si_->rel;//si_->rel
		unsigned count = si_->rel_count;

		Elf32_Sym* symtab = si_->symtab;
		const char* strtab = si_->strtab;
		Elf32_Sym* s;

		//遍历重定位节 DT_JMPREL/DT_REL
		for (size_t idx = 0; idx < count; ++idx, ++rel)
		{
			unsigned type = ELF32_R_TYPE(rel->r_info);
			unsigned sym = ELF32_R_SYM(rel->r_info);
			char* sym_name = NULL;
			if (type == 0) // R_*_NONE
			{
				continue;
			}
			if (sym != 0)
			{
				sym_name = (char *)(strtab + symtab[sym].st_name); //获取符号名
				shdrs_[SI_DYNSYM].sh_size = MAX(shdrs_[SI_DYNSYM].sh_size, (sym + 1) * sizeof(Elf32_Sym));
				shdrs_[SI_DYNSTR].sh_size = MAX(shdrs_[SI_DYNSTR].sh_size, symtab[sym].st_name + strlen(sym_name) + 1);
			}
		}
	}

	//通过.hash中引用的符号来确定dynsym, dynstr的节大小
	{
		Elf32_Sym* symtab = si_->symtab;
		const char* strtab = si_->strtab;
		char* sym_name = NULL;

		//遍历si->bucket, 查询sym
		for (unsigned hash = 0; hash < si_->nbucket; hash++)
		{
			for (unsigned n = si_->bucket[hash]; n != 0; n = si_->chain[n])
			{
				sym_name = (char *)(strtab + symtab[n].st_name); //获取符号名
				shdrs_[SI_DYNSYM].sh_size = MAX(shdrs_[SI_DYNSYM].sh_size, (n + 1) * sizeof(Elf32_Sym));
				shdrs_[SI_DYNSTR].sh_size = MAX(shdrs_[SI_DYNSTR].sh_size, symtab[n].st_name + strlen(sym_name) + 1);
			}
		}
	}

	//遍历dynsym_得到sh_info
	for (Elf32_Sym *sym = si_->symtab; sym < si_->symtab + (shdrs_[SI_DYNSYM].sh_size / sizeof(Elf32_Sym)); sym++)
	{
		if (ELF32_ST_BIND(sym->st_info) == STB_LOCAL && sym->st_shndx != SHN_UNDEF)
		{
			int local_idx = si_->symtab - sym;
			shdrs_[SI_DYNSYM].sh_info = local_idx + 1;

			//TODO: 判断虚拟地址所在的节, 最后修正st_shndx(原来为0的不处理), 便于010使用-_-
			if (sym->st_shndx != 0)
			{

			}
		}
	}

	return true;
}

bool ElfFixer::fixShdrFromShdr()
{
	//修复.plt
	//修复起始地址, 两种方法: 
	//	1. 通过16字节特征码(shellcode)且必须落在已知节之外
	//	2. 通过.rel.plt表中类型为R_ARM_JUMP_SLOT的范围来确定

	//04 E0 2D E5                 STR             LR, [SP, #  - 4]!
	//04 E0 9F E5                 LDR             LR, =(_GLOBAL_OFFSET_TABLE_ - 0x5EB0)
	//0E E0 8F E0                 ADD             LR, PC, LR; _GLOBAL_OFFSET_TABLE_
	//08 F0 BE E5                 LDR             PC, [LR, #8]!; dword_0

	//修复大小 = (20 + 12 * .rel.plt项目数);
	const char plt_code[] = {
		'\x4', '\xE0', '\x2D', '\xE5',
		'\x4', '\xE0', '\x9F', '\xE5',
		'\xE', '\xE0', '\x8F', '\xE0',
		'\x8', '\xF0', '\xBE', '\xE5'
	};

	//通过观察发现, .plt节, .got节必然存在, 因为需要调用libc的__cxa_atexit, __cxa_finalize

	int addr = Util::kmpSearch((char *)si_->base, si_->size, plt_code, 16);
	if (addr == -1)
	{
		DEBUG("kmpSearch .plt节没有找到\n");
	}
	else
	{
		shdrs_[SI_PLT].sh_name = GetShdrName(SI_PLT);
		shdrs_[SI_PLT].sh_type = SHT_PROGBITS;
		shdrs_[SI_PLT].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
		shdrs_[SI_PLT].sh_addr = addr;
		shdrs_[SI_PLT].sh_offset = addrToOff(shdrs_[SI_PLT].sh_addr);
		shdrs_[SI_PLT].sh_size = 20 + 12 * (shdrs_[SI_RELPLT].sh_size / sizeof(Elf32_Rel));
		shdrs_[SI_PLT].sh_link = 0;
		shdrs_[SI_PLT].sh_info = 0;
		shdrs_[SI_PLT].sh_addralign = 4;
		shdrs_[SI_PLT].sh_entsize = 0;
		DEBUG("kmpSearch 已经找到.plt节 @offset: %X\n", shdrs_[SI_PLT].sh_offset);
	}

	//修复.got


	return true;
}

Elf32_Off ElfFixer::addrToOff(Elf32_Addr addr)
{
	const Elf32_Phdr* phdr = si_->phdr;
	const Elf32_Phdr* phdr_limit = phdr + si_->phnum;
	Elf32_Off off = -1;

	for (phdr = si_->phdr; phdr < phdr_limit; phdr++)
	{
		if (phdr->p_type != PT_LOAD)
		{
			continue;
		}

		Elf32_Addr seg_start = phdr->p_vaddr;	//内存映射实际起始地址
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
		}

		//判断addr是否在LOAD段中
		if (addr >= seg_page_start && addr < seg_file_end)
		{
			off = file_page_start + addr - seg_page_start;
			break;
		}
	}

	return off;
}

int ElfFixer::findShIdx(Elf32_Off off)
{
	int idx = -1;
	for (int i = 0; i < SI_MAX; i++)
	{
		if (off >= shdrs_[i].sh_offset && off < shdrs_[i].sh_offset + shdrs_[i].sh_size)
		{
			idx = i;
			break;
		}
	}

	return idx;
}