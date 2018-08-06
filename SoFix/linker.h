#pragma once

#include "exec_elf.h"

#define SOINFO_NAME_LEN 128
typedef void(*linker_function_t)();

struct link_map_t
{
	uintptr_t l_addr;
	char*  l_name;
	uintptr_t l_ld;
	link_map_t* l_next;
	link_map_t* l_prev;
};

struct soinfo
{
public:
	char name[SOINFO_NAME_LEN];
	const Elf32_Phdr* phdr; //已加载的PT_PHDR段, 或第一个可加载的段PT_LOAD并且文件偏移p_offset为0的段
	size_t phnum;
	Elf32_Addr entry;
	Elf32_Addr base;	//可加载的段的基址
	unsigned size;		//可加载的段的大小

	uint32_t unused1;  // DO NOT USE, maintained for compatibility.

	Elf32_Dyn* dynamic; //PT_DYNAMIC

	uint32_t unused2; // DO NOT USE, maintained for compatibility
	uint32_t unused3; // DO NOT USE, maintained for compatibility

	soinfo* next;    //链表的下一个so
	unsigned flags;

	//DT_STRTAB
	const char* strtab;

	//DT_SYMTAB
	Elf32_Sym* symtab;

	//DT_HASH (下面四个字段地址是连续的), 符号哈希表
	//先根据 bucket[hash % nbucket]得到符号表索引或chain表索引, 再根据chain表查询
	size_t nbucket;
	size_t nchain;
	unsigned* bucket;	//这个表给出了符号表的索引或chain表的索引
	unsigned* chain;	//每个表项为符号表的索引或下一个chain项

						//DT_PLTGOT
	unsigned* plt_got;

	//DT_JMPREL, DT_PLTRELSZ, 重定位
	Elf32_Rel* plt_rel;
	size_t plt_rel_count;

	//DT_REL, DT_RELSZ, 重定位
	Elf32_Rel* rel;
	size_t rel_count;

	//DT_PREINIT_ARRAY, DT_PREINIT_ARRAYSZ
	linker_function_t* preinit_array;
	size_t preinit_array_count;

	//DT_INIT_ARRAY, DT_INIT_ARRAYSZ 链接完成后会调用该函数数组
	linker_function_t* init_array;
	size_t init_array_count;

	//DT_FINI_ARRAY, DT_FINI_ARRAYSZ
	linker_function_t* fini_array;
	size_t fini_array_count;

	//DT_INIT 链接完成后会调用该函数
	linker_function_t init_func;

	//DT_FINI
	linker_function_t fini_func;

	//ARM EABI section used for stack unwinding. PT_ARM_EXIDX用于栈展开
	unsigned* ARM_exidx;
	size_t ARM_exidx_count;

	size_t ref_count;		//引用计数
	link_map_t link_map;

	bool constructors_called; //构造器是否已调用, 确保只调用一次

							  // When you read a virtual address from the ELF file, add this
							  // value to get the corresponding address in the process' address space.
	Elf32_Addr load_bias; //实际段加载基址与期望段加载基址的偏移

						  //是否有DT_TEXTREL, DT_SYMBOLIC
	bool has_text_relocations;
	bool has_DT_SYMBOLIC;
};

size_t strlcpy(char *dst, const char *src, size_t siz);
soinfo* soinfo_alloc(const char* name);
void phdr_table_get_dynamic_section(const Elf32_Phdr* phdr_table,
	int               phdr_count,
	Elf32_Addr        load_bias,
	Elf32_Dyn**       dynamic,
	size_t*           dynamic_count,
	Elf32_Word*       dynamic_flags);

int phdr_table_get_arm_exidx(const Elf32_Phdr* phdr_table,
	int               phdr_count,
	Elf32_Addr        load_bias,
	Elf32_Addr**      arm_exidx,
	unsigned*         arm_exidx_count);
