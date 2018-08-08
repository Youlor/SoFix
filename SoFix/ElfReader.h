#pragma once
#include "exec_elf.h"
#include <QFile>


class ElfReader 
{
public:
	ElfReader(const char* sopath, const char *dumppath);
	~ElfReader();

	bool Load();

	size_t phdr_count() { return phdr_num_; }
	Elf32_Addr load_start() { return reinterpret_cast<Elf32_Addr>(load_start_); }
	Elf32_Addr load_size() { return load_size_; }
	Elf32_Addr load_bias() { return load_bias_; }
	const Elf32_Phdr* loaded_phdr() { return loaded_phdr_; }

private:
	bool OpenElf();
	bool ReadElfHeader();
	bool VerifyElfHeader();
	bool ReadProgramHeader();
	static size_t phdr_table_get_load_size(const Elf32_Phdr* phdr_table, 
		size_t phdr_count, Elf32_Addr* out_min_vaddr, Elf32_Addr* out_max_vaddr = 0);
	bool ReserveAddressSpace();
	bool LoadSegments();
	bool FindPhdr();
	bool CheckPhdr(Elf32_Addr);

	char* sopath_;		//正常so
	char* dumppath_;	//待修复dump so, 如果为空说明修复正常so

	QFile sofile_;
	QFile dumpfile_;

	Elf32_Ehdr header_;			//elf文件头部
	size_t phdr_num_;			//程序头表项数量

	void* phdr_mmap_;			//程序头表内存映射的页起始地址
	Elf32_Phdr* phdr_table_;	//程序头表的内存地址
	Elf32_Addr phdr_size_;		//程序头表所占页大小

	// First page of reserved address space. 保留的用于段加载的地址空间的起始页即内存映射起始页
	void* load_start_;
	// Size in bytes of reserved address space. 保留地址空间的大小即内存映射大小
	Elf32_Addr load_size_;
	// Load bias. 实际内存映射与期望内存映射的偏移
	Elf32_Addr load_bias_;

	// Loaded phdr. 已加载的PT_PHDR段, 或第一个可加载的段PT_LOAD并且文件偏移p_offset为0的段
	const Elf32_Phdr* loaded_phdr_;
};
