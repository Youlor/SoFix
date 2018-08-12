#include "ElfReader.h"
#include "Util.h"

#define QSTR8BIT(s) (QString::fromLocal8Bit(s))

#define DL_ERR qDebug

ElfReader::ElfReader(const char* sopath, const char* dumppath)
	: sopath_(nullptr), dumppath_(nullptr), phdr_num_(0), phdr_mmap_(NULL),
	phdr_table_(NULL), phdr_size_(0), load_start_(NULL),
	load_size_(0), load_bias_(0), loaded_phdr_(NULL)
{
	if (sopath)
	{
		sopath_ = strdup(sopath);
		sofile_.setFileName(QSTR8BIT(sopath));
	}

	if (dumppath)
	{
		dumppath_ = strdup(dumppath);
		dumpfile_.setFileName(QSTR8BIT(dumppath));
	}
}

ElfReader::~ElfReader()
{
	if (sopath_)
	{
		free(sopath_);
	}
	sofile_.close();

	if (dumppath_)
	{
		free(dumppath_);
	}
	dumpfile_.close();

	if (phdr_mmap_ != NULL)
	{
		Util::munmap(phdr_mmap_, phdr_size_);
	}
}

//完成elf的加载
bool ElfReader::Load()
{
	bool loaded = false;
	if (sopath_)
	{
		//如果是正常的so文件, 则参考linker源码进行加载
		loaded = OpenElf() &&
			ReadElfHeader() &&
			VerifyElfHeader() &&
			ReadProgramHeader() &&
			ReserveAddressSpace() &&
			LoadSegments() &&
			FindPhdr();

		if (loaded && dumppath_) //如果根据正常的so, 修复dump so, 直接偷梁换柱
		{
			if (dumpfile_.open(QIODevice::ReadOnly | QIODevice::ExistingOnly))
			{
				dumpfile_.read((char *)load_start_, load_size_);
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	else if(dumppath_)
	{
		//如果仅仅修复dump so, 读取dump文件获取Elf头部信息
		//This is Ugly...
		sopath_ = strdup(dumppath_);
		sofile_.setFileName(QSTR8BIT(dumppath_));

		if (OpenElf() && ReadElfHeader() && VerifyElfHeader() && ReadProgramHeader())
		{
			Elf32_Addr min_vaddr;
			load_size_ = phdr_table_get_load_size(phdr_table_, phdr_num_, &min_vaddr); //获取可以存放所有的可加载的节的页大小
			if (load_size_ == 0)
			{
				DL_ERR("\"%s\" has no loadable segments", sopath_);
				return false;
			}

			uint8_t* addr = reinterpret_cast<uint8_t*>(min_vaddr);
			void* start = Util::mmap(NULL, sofile_.size(), sofile_, 0);
			if (start == nullptr)
			{
				DL_ERR("couldn't reserve %d bytes of address space for \"%s\"", load_size_, sopath_);
				return false;
			}

			load_start_ = start;
			load_bias_ = reinterpret_cast<uint8_t*>(start) - addr;

			loaded = FindPhdr();
		}
	}
	

	return loaded;
}

bool ElfReader::OpenElf()
{
	return sofile_.open(QIODevice::ReadOnly | QIODevice::ExistingOnly);
}

bool ElfReader::ReadElfHeader()
{
	//成功返回读取的字节数, 出错返回-1并设置errno, 如果在调read之前已到达文件末尾, 则这次read返回0
	sofile_.seek(0);
	qint64 rc = sofile_.read((char *)&header_, sizeof(header_));
	if (rc < 0)
	{
		DL_ERR("can't read file \"%s\"", sopath_);
		return false;
	}
	if (rc != sizeof(header_))
	{
		DL_ERR("\"%s\" is too small to be an ELF executable", sopath_);
		return false;
	}

	return true;
}

bool ElfReader::VerifyElfHeader()
{
	if (header_.e_ident[EI_MAG0] != ELFMAG0 ||
		header_.e_ident[EI_MAG1] != ELFMAG1 ||
		header_.e_ident[EI_MAG2] != ELFMAG2 ||
		header_.e_ident[EI_MAG3] != ELFMAG3)
	{
		DL_ERR("\"%s\" has bad ELF magic", sopath_);
		return false;
	}

	if (header_.e_ident[EI_CLASS] != ELFCLASS32)
	{
		DL_ERR("\"%s\" not 32-bit: %d", sopath_, header_.e_ident[EI_CLASS]);
		return false;
	}
	if (header_.e_ident[EI_DATA] != ELFDATA2LSB)
	{
		DL_ERR("\"%s\" not little-endian: %d", sopath_, header_.e_ident[EI_DATA]);
		return false;
	}

	if (header_.e_type != ET_DYN)
	{
		DL_ERR("\"%s\" has unexpected e_type: %d", sopath_, header_.e_type);
		return false;
	}

	if (header_.e_version != EV_CURRENT)
	{
		DL_ERR("\"%s\" has unexpected e_version: %d", sopath_, header_.e_version);
		return false;
	}

	if (header_.e_machine != EM_ARM)
	{
		DL_ERR("\"%s\" has unexpected e_machine: %d", sopath_, header_.e_machine);
		return false;
	}

	return true;
}

bool ElfReader::ReadProgramHeader()
{
	phdr_num_ = header_.e_phnum;

	// Like the kernel, we only accept program header tables that
	// are smaller than 64KiB.
	if (phdr_num_ < 1 || phdr_num_ > 65536 / sizeof(Elf32_Phdr))
	{
		DL_ERR("\"%s\" has invalid e_phnum: %d", sopath_, phdr_num_);
		return false;
	}

	Elf32_Addr page_min = PAGE_START(header_.e_phoff); //ph所在的页的首地址
	Elf32_Addr page_max = PAGE_END(header_.e_phoff + (phdr_num_ * sizeof(Elf32_Phdr))); //ph的尾部所在的页的下一个页的首地址
	Elf32_Addr page_offset = PAGE_OFFSET(header_.e_phoff);	//ph在页(4K大小)中的偏移

	phdr_size_ = page_max - page_min;//ph所占的页大小

	void* mmap_result = Util::mmap(NULL, phdr_size_, sofile_, page_min);//将ph映射到内存中
	if (mmap_result == nullptr)
	{
		DL_ERR("\"%s\" phdr mmap failed", sopath_);
		return false;
	}

	phdr_mmap_ = mmap_result;
	phdr_table_ = reinterpret_cast<Elf32_Phdr*>(reinterpret_cast<char*>(mmap_result) + page_offset);
	return true;
}

size_t ElfReader::phdr_table_get_load_size(const Elf32_Phdr* phdr_table,
	size_t phdr_count,
	Elf32_Addr* out_min_vaddr,
	Elf32_Addr* out_max_vaddr)
{
	Elf32_Addr min_vaddr = 0xFFFFFFFFU;
	Elf32_Addr max_vaddr = 0x00000000U;

	bool found_pt_load = false;
	for (size_t i = 0; i < phdr_count; ++i)
	{
		const Elf32_Phdr* phdr = &phdr_table[i];

		if (phdr->p_type != PT_LOAD)
		{
			continue;
		}
		found_pt_load = true;

		if (phdr->p_vaddr < min_vaddr)
		{
			min_vaddr = phdr->p_vaddr;
		}

		if (phdr->p_vaddr + phdr->p_memsz > max_vaddr)
		{
			max_vaddr = phdr->p_vaddr + phdr->p_memsz;
		}
	}
	if (!found_pt_load)
	{
		min_vaddr = 0x00000000U;
	}

	min_vaddr = PAGE_START(min_vaddr);
	max_vaddr = PAGE_END(max_vaddr);

	if (out_min_vaddr != NULL)
	{
		*out_min_vaddr = min_vaddr;
	}
	if (out_max_vaddr != NULL)
	{
		*out_max_vaddr = max_vaddr;
	}
	return max_vaddr - min_vaddr;
}

bool ElfReader::ReserveAddressSpace()
{
	Elf32_Addr min_vaddr;
	load_size_ = phdr_table_get_load_size(phdr_table_, phdr_num_, &min_vaddr); //获取可以存放所有的可加载的节的页大小
	if (load_size_ == 0)
	{
		DL_ERR("\"%s\" has no loadable segments", sopath_);
		return false;
	}

	uint8_t* addr = reinterpret_cast<uint8_t*>(min_vaddr);
	void* start = Util::mmap(nullptr, load_size_);
	if (start == nullptr)
	{
		DL_ERR("couldn't reserve %d bytes of address space for \"%s\"", load_size_, sopath_);
		return false;
	}

	load_start_ = start;
	load_bias_ = reinterpret_cast<uint8_t*>(start) - addr;
	return true;
}

bool ElfReader::LoadSegments()
{
	for (size_t i = 0; i < phdr_num_; ++i)
	{
		const Elf32_Phdr* phdr = &phdr_table_[i];

		if (phdr->p_type != PT_LOAD)
		{
			continue;
		}

		// Segment addresses in memory.
		Elf32_Addr seg_start = phdr->p_vaddr + load_bias_;	//内存映射实际起始地址
		Elf32_Addr seg_end = seg_start + phdr->p_memsz;	//内存映射实际终止地址

		Elf32_Addr seg_page_start = PAGE_START(seg_start); //内存映射页首地址
		Elf32_Addr seg_page_end = PAGE_END(seg_end);	   //内存映射终止页

		Elf32_Addr seg_file_end = seg_start + phdr->p_filesz; //文件映射终止页

		// File offsets.
		Elf32_Addr file_start = phdr->p_offset;
		Elf32_Addr file_end = file_start + phdr->p_filesz;

		Elf32_Addr file_page_start = PAGE_START(file_start); //文件映射页首地址
		Elf32_Addr file_length = file_end - file_page_start; //文件映射大小

		if (file_length != 0) //进行文件内存映射
		{
			void* seg_addr = Util::mmap((void*)seg_page_start,
				file_length,
				sofile_,
				file_page_start);
			if (seg_addr == nullptr)
			{
				DL_ERR("couldn't map \"%s\" segment %d", sopath_, i);
				return false;
			}
		}

		// if the segment is writable, and does not end on a page boundary,
		// zero-fill it until the page limit. 从文件映射边界填充0到页边界
		if ((phdr->p_flags & PF_W) != 0 && PAGE_OFFSET(seg_file_end) > 0)
		{
			memset((void*)seg_file_end, 0, PAGE_SIZE - PAGE_OFFSET(seg_file_end));
		}

		seg_file_end = PAGE_END(seg_file_end);

		// seg_file_end is now the first page address after the file
		// content. If seg_end is larger, we need to zero anything
		// between them. This is done by using a private anonymous
		// map for all extra pages.
		// 如果内存映射>文件映射则会额外映射并初始化为0
		if (seg_page_end > seg_file_end)
		{
			void* zeromap = Util::mmap((void*)seg_file_end, seg_page_end - seg_file_end);
			if (zeromap == nullptr)
			{
				DL_ERR("couldn't zero fill \"%s\"", sopath_);
				return false;
			}
		}
	}
	return true;
}

bool ElfReader::FindPhdr()
{
	const Elf32_Phdr* phdr_limit = phdr_table_ + phdr_num_;

	// If there is a PT_PHDR, use it directly. 如果Ph表中存在 PT_PHDR 即自身, 则检查它
	for (const Elf32_Phdr* phdr = phdr_table_; phdr < phdr_limit; ++phdr)
	{
		if (phdr->p_type == PT_PHDR)
		{
			return CheckPhdr(load_bias_ + phdr->p_vaddr);
		}
	}

	// Otherwise, check the first loadable segment. If its file offset
	// is 0, it starts with the ELF header, and we can trivially find the
	// loaded program header from it. 
	//否则, 检查第一个可加载段且文件偏移为0, 起始于Elf Header, 则检查它
	for (const Elf32_Phdr* phdr = phdr_table_; phdr < phdr_limit; ++phdr)
	{
		if (phdr->p_type == PT_LOAD)
		{
			if (phdr->p_offset == 0)
			{
				Elf32_Addr  elf_addr = load_bias_ + phdr->p_vaddr;
				const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)(void*)elf_addr;
				Elf32_Addr  offset = ehdr->e_phoff;
				return CheckPhdr((Elf32_Addr)ehdr + offset);
			}
			break;
		}
	}

	DL_ERR("can't find loaded phdr for \"%s\"", sopath_);
	return false;
}

bool ElfReader::CheckPhdr(Elf32_Addr loaded)
{
	const Elf32_Phdr* phdr_limit = phdr_table_ + phdr_num_;
	Elf32_Addr loaded_end = loaded + (phdr_num_ * sizeof(Elf32_Phdr));
	for (Elf32_Phdr* phdr = phdr_table_; phdr < phdr_limit; ++phdr)
	{
		if (phdr->p_type != PT_LOAD)
		{
			continue;
		}

		//表示了有效的文件映射
		Elf32_Addr seg_start = phdr->p_vaddr + load_bias_;	//内存基址
		Elf32_Addr seg_end = phdr->p_filesz + seg_start;	//内存基址 + 文件大小
		if (seg_start <= loaded && loaded_end <= seg_end)
		{
			loaded_phdr_ = reinterpret_cast<const Elf32_Phdr*>(loaded);
			return true;
		}
	}
	DL_ERR("\"%s\" loaded phdr %x not in loadable segment", sopath_, loaded);
	return false;
}
