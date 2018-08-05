#pragma once
#include <stdint.h>

#pragma pack(push, 1)

typedef uint8_t		Elf_Byte;

typedef uint32_t	Elf32_Addr;
#define ELF32_FSZ_ADDR	4
typedef uint32_t	Elf32_Off;
typedef int32_t		Elf32_SOff;
#define ELF32_FSZ_OFF	4
typedef int32_t		Elf32_Sword;
#define ELF32_FSZ_SWORD 4
typedef uint32_t	Elf32_Word;
#define ELF32_FSZ_WORD	4
typedef uint16_t	Elf32_Half;
#define ELF32_FSZ_HALF	2
typedef uint64_t	Elf32_Lword;
#define ELF32_FSZ_LWORD 8

typedef uint64_t	Elf64_Addr;
#define ELF64_FSZ_ADDR	8
typedef uint64_t	Elf64_Off;
typedef int64_t		Elf64_SOff;
#define ELF64_FSZ_OFF	8
typedef int32_t		Elf64_Shalf;
#define ELF64_FSZ_SHALF 4

typedef int32_t		Elf64_Sword;
#define ELF64_FSZ_SWORD 4
typedef uint32_t	Elf64_Word;
#define ELF64_FSZ_WORD	4

typedef int64_t		Elf64_Sxword;
#define ELF64_FSZ_SXWORD 8
typedef uint64_t	Elf64_Xword;
#define ELF64_FSZ_XWORD 8
typedef uint64_t	Elf64_Lword;
#define ELF64_FSZ_LWORD 8
typedef uint16_t	Elf64_Half;
#define ELF64_FSZ_HALF 2

/*
* ELF Header
*/
#define ELF_NIDENT	16

typedef struct {
	unsigned char	e_ident[ELF_NIDENT];	/* Id bytes 标识*/
	Elf32_Half	e_type;			/* file type 文件类型 32bit? */
	Elf32_Half	e_machine;		/* machine type 机器类型 需要和处理器匹配 */
	Elf32_Word	e_version;		/* version number 版本 VT_CURRENT? */
	Elf32_Addr	e_entry;		/* entry point exe的入口虚拟地址 */
	Elf32_Off	e_phoff;		/* Program hdr offset phdr_table的文件偏移(静态链接可选) */
	Elf32_Off	e_shoff;		/* Section hdr offset shdr_table的文件偏移(执行可选) */
	Elf32_Word	e_flags;		/* Processor flags 与处理器相关的标志 */
	Elf32_Half	e_ehsize;		/* sizeof ehdr ehdr的大小 */
	Elf32_Half	e_phentsize;		/* Program header entry size phdr的大小(静态链接可选) */
	Elf32_Half	e_phnum;		/* Number of program headers phdr的数量(静态链接可选) */
	Elf32_Half	e_shentsize;		/* Section header entry size shdr的大小(执行可选) */
	Elf32_Half	e_shnum;		/* Number of section headers shdr的数量(执行可选) */
	Elf32_Half	e_shstrndx;		/* String table index 字符串表strtab的所在节的索引(执行可选) */
} Elf32_Ehdr; //ELF文件头


			  /* e_ident offsets */
#define EI_MAG0		0	/* '\177' */
#define EI_MAG1		1	/* 'E'	  */
#define EI_MAG2		2	/* 'L'	  */
#define EI_MAG3		3	/* 'F'	  */
#define EI_CLASS	4	/* File class */
#define EI_DATA		5	/* Data encoding */
#define EI_VERSION	6	/* File version */
#define EI_OSABI	7	/* Operating system/ABI identification */
#define EI_ABIVERSION	8	/* ABI version */
#define EI_PAD		9	/* Start of padding bytes up to EI_NIDENT*/
#define EI_NIDENT	16	/* First non-ident header byte */

			  /* e_ident[EI_MAG0,EI_MAG3] */
#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'
#define ELFMAG		"\177ELF"
#define SELFMAG		4

			  /* e_ident[EI_CLASS] */
#define ELFCLASSNONE	0	/* Invalid class */
#define ELFCLASS32	1	/* 32-bit objects */
#define ELFCLASS64	2	/* 64-bit objects */
#define ELFCLASSNUM	3

			  /* e_ident[EI_DATA] */
#define ELFDATANONE	0	/* Invalid data encoding */
#define ELFDATA2LSB	1	/* 2's complement values, LSB first */
#define ELFDATA2MSB	2	/* 2's complement values, MSB first */

			  /* e_ident[EI_VERSION] */
#define EV_NONE		0	/* Invalid version */
#define EV_CURRENT	1	/* Current version */
#define EV_NUM		2

			  /* e_ident[EI_OSABI] */
#define ELFOSABI_SYSV		0	/* UNIX System V ABI */
#define ELFOSABI_HPUX		1	/* HP-UX operating system */
#define ELFOSABI_NETBSD		2	/* NetBSD */
#define ELFOSABI_LINUX		3	/* GNU/Linux */
#define ELFOSABI_HURD		4	/* GNU/Hurd */
#define ELFOSABI_86OPEN		5	/* 86Open */
#define ELFOSABI_SOLARIS	6	/* Solaris */
#define ELFOSABI_MONTEREY	7	/* Monterey */
#define ELFOSABI_IRIX		8	/* IRIX */
#define ELFOSABI_FREEBSD	9	/* FreeBSD */
#define ELFOSABI_TRU64		10	/* TRU64 UNIX */
#define ELFOSABI_MODESTO	11	/* Novell Modesto */
#define ELFOSABI_OPENBSD	12	/* OpenBSD */
#define ELFOSABI_OPENVMS	13	/* OpenVMS */
#define ELFOSABI_NSK		14	/* HP Non-Stop Kernel */
#define ELFOSABI_AROS		15	/* Amiga Research OS */
			  /* Unofficial OSABIs follow */
#define ELFOSABI_ARM		97	/* ARM */
#define ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */

#define ELFOSABI_NONE		ELFOSABI_SYSV
#define ELFOSABI_AIX		ELFOSABI_MONTEREY

			  /* e_type */
#define ET_NONE		0	/* No file type */
#define ET_REL		1	/* Relocatable file */
#define ET_EXEC		2	/* Executable file */
#define ET_DYN		3	/* Shared object file */
#define ET_CORE		4	/* Core file */
#define ET_NUM		5

#define ET_LOOS		0xfe00	/* Operating system specific range */
#define ET_HIOS		0xfeff
#define ET_LOPROC	0xff00	/* Processor-specific range */
#define ET_HIPROC	0xffff

			  /* e_machine */
#define EM_NONE		0	/* No machine */
#define EM_M32		1	/* AT&T WE 32100 */
#define EM_SPARC	2	/* SPARC */
#define EM_386		3	/* Intel 80386 */
#define EM_68K		4	/* Motorola 68000 */
#define EM_88K		5	/* Motorola 88000 */
#define EM_486		6	/* Intel 80486 */
#define EM_860		7	/* Intel 80860 */
#define EM_MIPS		8	/* MIPS I Architecture */
#define EM_S370		9	/* Amdahl UTS on System/370 */
#define EM_MIPS_RS3_LE	10	/* MIPS RS3000 Little-endian */
			  /* 11-14 - Reserved */
#define EM_RS6000	11	/* IBM RS/6000 XXX reserved */
#define EM_PARISC	15	/* Hewlett-Packard PA-RISC */
#define EM_NCUBE	16	/* NCube XXX reserved */
#define EM_VPP500	17	/* Fujitsu VPP500 */
#define EM_SPARC32PLUS	18	/* Enhanced instruction set SPARC */
#define EM_960		19	/* Intel 80960 */
#define EM_PPC		20	/* PowerPC */
#define EM_PPC64	21	/* 64-bit PowerPC */
			  /* 22-35 - Reserved */
#define EM_S390		22	/* System/390 XXX reserved */
#define EM_V800		36	/* NEC V800 */
#define EM_FR20		37	/* Fujitsu FR20 */
#define EM_RH32		38	/* TRW RH-32 */
#define EM_RCE		39	/* Motorola RCE */
#define EM_ARM		40	/* Advanced RISC Machines ARM */
#define EM_ALPHA	41	/* DIGITAL Alpha */
#define EM_SH		42	/* Hitachi Super-H */
#define EM_SPARCV9	43	/* SPARC Version 9 */
#define EM_TRICORE	44	/* Siemens Tricore */
#define EM_ARC		45	/* Argonaut RISC Core */
#define EM_H8_300	46	/* Hitachi H8/300 */
#define EM_H8_300H	47	/* Hitachi H8/300H */
#define EM_H8S		48	/* Hitachi H8S */
#define EM_H8_500	49	/* Hitachi H8/500 */
#define EM_IA_64	50	/* Intel Merced Processor */
#define EM_MIPS_X	51	/* Stanford MIPS-X */
#define EM_COLDFIRE	52	/* Motorola Coldfire */
#define EM_68HC12	53	/* Motorola MC68HC12 */
#define EM_MMA		54	/* Fujitsu MMA Multimedia Accelerator */
#define EM_PCP		55	/* Siemens PCP */
#define EM_NCPU		56	/* Sony nCPU embedded RISC processor */
#define EM_NDR1		57	/* Denso NDR1 microprocessor */
#define EM_STARCORE	58	/* Motorola Star*Core processor */
#define EM_ME16		59	/* Toyota ME16 processor */
#define EM_ST100	60	/* STMicroelectronics ST100 processor */
#define EM_TINYJ	61	/* Advanced Logic Corp. TinyJ embedded family processor */
#define EM_X86_64	62	/* AMD x86-64 architecture */
#define EM_PDSP		63	/* Sony DSP Processor */
#define EM_PDP10	64	/* Digital Equipment Corp. PDP-10 */
#define EM_PDP11	65	/* Digital Equipment Corp. PDP-11 */
#define EM_FX66		66	/* Siemens FX66 microcontroller */
#define EM_ST9PLUS	67	/* STMicroelectronics ST9+ 8/16 bit microcontroller */
#define EM_ST7		68	/* STMicroelectronics ST7 8-bit microcontroller */
#define EM_68HC16	69	/* Motorola MC68HC16 Microcontroller */
#define EM_68HC11	70	/* Motorola MC68HC11 Microcontroller */
#define EM_68HC08	71	/* Motorola MC68HC08 Microcontroller */
#define EM_68HC05	72	/* Motorola MC68HC05 Microcontroller */
#define EM_SVX		73	/* Silicon Graphics SVx */
#define EM_ST19		74	/* STMicroelectronics ST19 8-bit CPU */
#define EM_VAX		75	/* Digital VAX */
#define EM_CRIS		76	/* Axis Communications 32-bit embedded processor */
#define EM_JAVELIN	77	/* Infineon Technologies 32-bit embedded CPU */
#define EM_FIREPATH	78	/* Element 14 64-bit DSP processor */
#define EM_ZSP		79	/* LSI Logic's 16-bit DSP processor */
#define EM_MMIX		80	/* Donald Knuth's educational 64-bit processor */
#define EM_HUANY	81	/* Harvard's machine-independent format */
#define EM_PRISM	82	/* SiTera Prism */
#define EM_AVR		83	/* Atmel AVR 8-bit microcontroller */
#define EM_FR30		84	/* Fujitsu FR30 */
#define EM_D10V		85	/* Mitsubishi D10V */
#define EM_D30V		86	/* Mitsubishi D30V */
#define EM_V850		87	/* NEC v850 */
#define EM_M32R		88	/* Mitsubishi M32R */
#define EM_MN10300	89	/* Matsushita MN10300 */
#define EM_MN10200	90	/* Matsushita MN10200 */
#define EM_PJ		91	/* picoJava */
#define EM_OPENRISC	92	/* OpenRISC 32-bit embedded processor */
#define EM_ARC_A5	93	/* ARC Cores Tangent-A5 */
#define EM_XTENSA	94	/* Tensilica Xtensa Architecture */
#define EM_VIDEOCORE	95	/* Alphamosaic VideoCore processor */
#define EM_TMM_GPP	96	/* Thompson Multimedia General Purpose Processor */
#define EM_NS32K	97	/* National Semiconductor 32000 series */
#define EM_TPC		98	/* Tenor Network TPC processor */
#define EM_SNP1K	99	/* Trebia SNP 1000 processor */
#define EM_ST200	100	/* STMicroelectronics ST200 microcontroller */
#define EM_IP2K		101	/* Ubicom IP2xxx microcontroller family */
#define EM_MAX		102	/* MAX processor */
#define EM_CR		103	/* National Semiconductor CompactRISC micorprocessor */
#define EM_F2MC16	104	/* Fujitsu F2MC16 */
#define EM_MSP430	105	/* Texas Instruments MSP430 */
#define EM_BLACKFIN	106	/* Analog Devices Blackfin DSP */
#define EM_SE_C33	107	/* Seiko Epson S1C33 family */
#define EM_SEP		108	/* Sharp embedded microprocessor */
#define EM_ARCA		109	/* Arca RISC microprocessor */
#define EM_UNICORE	110	/* UNICORE from PKU-Unity Ltd. and MPRC Peking University */

			  /* Unofficial machine types follow */
#define EM_AVR32	6317	/* used by NetBSD/avr32 */
#define EM_ALPHA_EXP	36902	/* used by NetBSD/alpha; obsolete */
#define EM_NUM		36903

/*
* Program Header 程序头部: 描述段的信息
*/
typedef struct {
	Elf32_Word	p_type;		/* entry type 段的类型, 主要是PHDR段, LOAD段, DYNAMIC段 */
	Elf32_Off	p_offset;	/* offset 段的文件偏移 */
	Elf32_Addr	p_vaddr;	/* virtual address 段的虚拟地址 */
	Elf32_Addr	p_paddr;	/* physical address 物理地址(似乎没用) */
	Elf32_Word	p_filesz;	/* file size 文件大小 */
	Elf32_Word	p_memsz;	/* memory size 内存大小 */
	Elf32_Word	p_flags;	/* flags 标志,  给出了段的页属性 */
	Elf32_Word	p_align;	/* memory & file alignment 内存和文件对齐(似乎没用, 固定4K) */
} Elf32_Phdr;

/* p_type */
#define PT_NULL		0		/* Program header table entry unused 未使用 */
#define PT_LOAD		1		/* Loadable program segment LOAD段, 可加载段 */
#define PT_DYNAMIC	2		/* Dynamic linking information DYNAMIC节, 包含动态链接信息 */
#define PT_INTERP	3		/* Program interpreter */
#define PT_NOTE		4		/* Auxiliary information */
#define PT_SHLIB	5		/* Reserved, unspecified semantics */
#define PT_PHDR		6		/* Entry for header table itself 
* 表明程序头部表自身的大小和位置, 如果存在则必须在所有LOAD段之前, 且只包含一个
*/
#define PT_TLS		7		/* TLS initialisation image */
#define PT_NUM		8

#define PT_LOOS		0x60000000	/* OS-specific range */

/* GNU-specific */
#define PT_GNU_EH_FRAME 0x6474e550	/* EH frame segment */
#define PT_GNU_STACK	0x6474e551	/* Indicate executable stack */
#define PT_GNU_RELRO	0x6474e552	/* Make read-only after relocation */

#define PT_HIOS		0x6fffffff
#define PT_LOPROC	0x70000000	/* Processor-specific range */
#define PT_HIPROC	0x7fffffff

#define PT_MIPS_REGINFO 0x70000000

/* p_flags */
#define PF_R		0x4		/* Segment is readable */
#define PF_W		0x2		/* Segment is writable */
#define PF_X		0x1		/* Segment is executable */

#define PF_MASKOS	0x0ff00000	/* Operating system specific values */
#define PF_MASKPROC	0xf0000000	/* Processor-specific values */

/* Extended program header index. */
#define PN_XNUM		0xffff

/*
* Section Headers 节头: 描述了节的信息
*/
//1. 有节头部不一定有节
//2. 节区占用文件中的连续区域(长度可能为0)
//3. 节区不能重叠
//4. 文件中可能包含非活动空间, 不属于任何头部和节区

//sh_type		sh_link									sh_info
//SHT_DYNAMIC	该节所用到的字符串表的节区头部索引				0
//SHT_HASH		该表所用到的符号表的节区头部索引				0
//SHT_REL		相关符号表的节区头部索引				重定位所适用的节区头部索引
//SHT_SYMTAB	相关字符串表的节区头部索引			最后一个局部符号的符号表索引值加一
//SHT_DYNSYM	同上											同上

//字符串表(String Table): 连续的0结尾的字符串, 包含节区名称, 符号名称, char *
//符号表(Symbol Table): 符号信息, 包括符号定义和符号引用, Elf32_Sym
//重定位表: 包含将符号引用与符号定义进行链接的信息, Elf32_Rel

typedef struct {
	Elf32_Word	sh_name;	/* section name (.shstrtab index) 节名称 */
	Elf32_Word	sh_type;	/* section type 节类型 */
	Elf32_Word	sh_flags;	/* section flags 位标志 */
	Elf32_Addr	sh_addr;	/* virtual address 虚拟地址(若为0则不会映射到内存) */
	Elf32_Off	sh_offset;	/* file offset 文件偏移 */
	Elf32_Word	sh_size;	/* section size 节大小 */
	Elf32_Word	sh_link;	/* link to another 相关节区头部索引 */
	Elf32_Word	sh_info;	/* misc info 附加信息 */
	Elf32_Word	sh_addralign;	/* memory alignment 内存对齐,  sh_addr %   sh_addralign==0 */
	Elf32_Word	sh_entsize;	/* table entry size 若该节区是一个表, 则这里表示表项的大小, 如符号表, 否则为0 */
} Elf32_Shdr;

/* sh_type 节区类型 */
#define SHT_NULL	      0		/* Section header table entry unused 未使用 */
#define SHT_PROGBITS	  1		/* Program information 该节区包含程序定义的信息, 由程序本身进行解释
* 如: .comments版本控制信息, .data .data1已初始化数据, .debug调试信息,
* .fini程序退出函数代码, .got全局偏移表, .init初始化代码, .interp解释器路径,
* ,line调试行号信息, .plt过程链接表, .rodata .rodata1只读数据, .text程序的可执行指令
*/
#define SHT_SYMTAB	      2		/* Symbol table 符号表 , 目前只包含一个, 一般用于静态链接, 但也可用于动态链接
* 如: .symtab符号表
*/
#define SHT_STRTAB	      3		/* String table 字符串表, 可能有多个 
* .dynstr动态链接相关字符串, .shstrtab节区名称, .strtab与符号表项相关的名称即符号名
*/
#define SHT_RELA	      4		/* Relocation information w/ addend 重定位表项, 可能有对齐内容 */
#define SHT_HASH	      5		/* Symbol hash table 符号哈希表, 目前只包含一个 
* .hash符号哈希表
*/
#define SHT_DYNAMIC	      6		/* Dynamic linking information 动态链接信息, 目前只包含一个
* 如: .dynamic包含动态链接信息
*/
#define SHT_NOTE	      7		/* Auxiliary information 辅助信息 
* 如: .note包含注释信息
*/
#define SHT_NOBITS	      8		/* No space allocated in file image 没有文件映射的节 
* 如: .bss没有文件映射但有内存映射的节, 会被初始化为0
*/
#define SHT_REL		      9		/* Relocation information w/o addend 重定位表项 
* 如: .rel.xxx 包含了xxx节的重定位信息
*/
#define SHT_SHLIB	     10		/* Reserved, unspecified semantics 保留 */
#define SHT_DYNSYM	     11		/* Symbol table for dynamic linker 完整的符号表, 用于动态linker
* 如: .dynsym包含了动态链接符号表
*/
#define SHT_INIT_ARRAY	     14		/* Initialization function pointers 初始化函数指针数组 
* 如: .init_array
*/
#define SHT_FINI_ARRAY	     15		/* Termination function pointers 结束函数指针数组 
* 如：.fini_array
*/
#define SHT_PREINIT_ARRAY    16		/* Pre-initialization function ptrs 预初始化函数指针数组 */
#define SHT_GROUP	     	 17		/* Section group */
#define SHT_SYMTAB_SHNDX     18		/* Section indexes (see SHN_XINDEX) */
#define SHT_NUM		     	 19

#define SHT_LOOS	     0x60000000 /* Operating system specific range */
#define SHT_GNU_HASH	     0x6ffffff6 /* GNU style symbol hash table */
#define SHT_SUNW_move	     0x6ffffffa
#define SHT_SUNW_syminfo     0x6ffffffc
#define SHT_SUNW_verdef	     0x6ffffffd /* Versions defined by file */
#define SHT_GNU_verdef	     SHT_SUNW_verdef
#define SHT_SUNW_verneed     0x6ffffffe /* Versions needed by file */
#define SHT_GNU_verneed	     SHT_SUNW_verneed
#define SHT_SUNW_versym	     0x6fffffff /* Symbol versions */
#define SHT_GNU_versym	     SHT_SUNW_versym
#define SHT_HIOS	     0x6fffffff
#define SHT_LOPROC	     0x70000000 /* Processor-specific range */
#define SHT_AMD64_UNWIND 0x70000001 /* unwind information */
#define SHT_AMMEXIDX     0x70000001 /* unwind information */
#define SHT_HIPROC	     0x7fffffff
#define SHT_LOUSER	     0x80000000 /* Application-specific range */
#define SHT_HIUSER	     0xffffffff

/* sh_flags */
#define SHF_WRITE	     0x00000001 /* Contains writable data 包含可写数据 */
#define SHF_ALLOC	     0x00000002 /* Occupies memory 该节区占用内存 */
#define SHF_EXECINSTR	     0x00000004 /* Contains executable insns 包含可执行的机器指令  */
#define SHF_MERGE	     0x00000010 /* Might be merged 可能被合并 */
#define SHF_STRINGS	     0x00000020 /* Contains nul terminated strings 包含字符串 */
#define SHF_INFO_LINK	     0x00000040 /* "sh_info" contains SHT index sh_info中包含索引 */
#define SHF_LINK_ORDER	     0x00000080 /* Preserve order after combining 合并后保持顺序 */
#define SHF_OS_NONCONFORMING 0x00000100 /* OS specific handling required */
#define SHF_GROUP	     0x00000200 /* Is member of a group 是否是一组 */
#define SHF_TLS		     0x00000400 /* Holds thread-local data 线程局部存储 */
#define SHF_MASKOS	     0x0ff00000 /* Operating system specific values */
#define SHF_MASKPROC	     0xf0000000 /* Processor-specific values 处理器相关值 */
#define SHF_ORDERED	     0x40000000 /* Ordering requirement (Solaris) */
#define SHF_EXCLUDE	     0x80000000 /* Excluded unless unles ref/alloc
(Solaris).*/
/*
* Symbol Table
*/
typedef struct {
	Elf32_Word	st_name;	/* Symbol name (.strtab index) 符号名称, 实际上是在strtab节中的索引  */
	Elf32_Word	st_value;	/* value of symbol 一般为符号的虚拟地址 */
	Elf32_Word	st_size;	/* size of symbol 与符号相关的大小, 如对象大小等 */
	Elf_Byte	st_info;	/* type / binding attrs 符号的类型和绑定属性 */
	Elf_Byte	st_other;	/* unused 未使用 */
	Elf32_Half	st_shndx;	/* section index of symbol 符号所在节的索引(SHN_UNDEF表示未定义的符号) */
} Elf32_Sym;


/* Symbol Table index of the undefined symbol 未定义的符号 */
#define ELF_SYM_UNDEFINED	0

#define STN_UNDEF		0	/* undefined index */

/* st_info: Symbol Bindings 符号绑定 */
#define STB_LOCAL		0	/* local symbol 局部符号, 对所在文件外不可见, 因此查找符号时不会查找该类符号 */
#define STB_GLOBAL		1	/* global symbol 全局符号 */
#define STB_WEAK		2	/* weakly defined global symbol 弱全局符号(区别在于静态链接时) */
#define STB_NUM			3

#define STB_LOOS		10	/* Operating system specific range */
#define STB_HIOS		12
#define STB_LOPROC		13	/* Processor-specific range */
#define STB_HIPROC		15

/* st_info: Symbol Types 符号类型(执行时用不到) */
#define STT_NOTYPE		0	/* Type not specified 类型未指定 */
#define STT_OBJECT		1	/* Associated with a data object 与数据对象关联 */
#define STT_FUNC		2	/* Associated with a function 与函数或可执行代码关联 */
#define STT_SECTION		3	/* Associated with a section */
#define STT_FILE		4	/* Associated with a file name */
#define STT_COMMON		5	/* Uninitialised common block */
#define STT_TLS			6	/* Thread local data object */
#define STT_NUM			7

#define STT_LOOS		10	/* Operating system specific range */
#define STT_HIOS		12
#define STT_LOPROC		13	/* Processor-specific range */
#define STT_HIPROC		15

/* st_other: Visibility Types */
#define STV_DEFAULT		0	/* use binding type */
#define STV_INTERNAL		1	/* not referenced from outside */
#define STV_HIDDEN		2	/* not visible, may be used via ptr */
#define STV_PROTECTED		3	/* visible, not preemptible */
#define STV_EXPORTED		4
#define STV_SINGLETON		5
#define STV_ELIMINATE		6

/* st_info/st_other utility macros */
#define ELF_ST_BIND(info)		((uint32_t)(info) >> 4)
#define ELF_ST_TYPE(info)		((uint32_t)(info) & 0xf)
#define ELF_ST_INFO(bind,type)		((Elf_Byte)(((bind) << 4) | \
					 ((type) & 0xf)))
#define ELF_ST_VISIBILITY(other)	((uint32_t)(other) & 3)

/*
* Special section indexes
*/
#define SHN_UNDEF	0		/* Undefined section */

#define SHN_LORESERVE	0xff00		/* Reserved range */
#define SHN_ABS		0xfff1		/*  Absolute symbols */
#define SHN_COMMON	0xfff2		/*  Common symbols */
#define SHN_XINDEX	0xffff		/* Escape -- index stored elsewhere */
#define SHN_HIRESERVE	0xffff

#define SHN_LOPROC	0xff00		/* Processor-specific range */
#define SHN_HIPROC	0xff1f
#define SHN_LOOS	0xff20		/* Operating system specific range */
#define SHN_HIOS	0xff3f

#define SHN_MIPS_ACOMMON 0xff00
#define SHN_MIPS_TEXT	0xff01
#define SHN_MIPS_DATA	0xff02
#define SHN_MIPS_SCOMMON 0xff03

/*
* Relocation Entries
*/

//type:
//	S: 符号地址sym_addr=Elf32_Sym.st_value + sofino.load_bias
//	A: 待重定位地址处(r_offset + soinfo.load_bias)存放的相对地址即 *(Elf32_Addr *)(r_offset + soinfo.load_bias)
//	B: so的基址soinfo.base
//  P: 待重定位地址r_offset + soinfo.load_bias
//	O: r_offset

//	R_*_NONE: 无效项
//	R_ARM_JUMP_SLOT: PLT过程偏移表项, S
//	R_ARM_GLOB_DAT: GOT全局偏移表项, S
//	R_ARM_ABS32: A + S
//	R_ARM_REL32: A + S - O
//	R_ARM_RELATIVE: A + B
//

typedef struct {
	Elf32_Word	r_offset;	/* where to do it 一般是需要重定位的虚拟地址 */
	Elf32_Word	r_info;		/* index & type of relocation 低8位是type, 高24位是sym index, 为所需符号在符号表中的索引 */
} Elf32_Rel;

/* r_info utility macros */
#define ELF32_R_SYM(info)	((info) >> 8)
#define ELF32_R_TYPE(info)	((info) & 0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + (unsigned char)(type))

/*
* Dynamic Section structure array
* 有以下类型: 
* DT_NULL: 结束标志(必需)
* DT_NEEDED: 依赖库, d_val存放依赖库名称字符串在DT_STRTAB中的偏移(可选)
* DT_HASH: 符号哈希表, d_ptr存放虚拟地址(必需)
* DT_STRTAB: 字符串表, d_ptr存放虚拟地址(必需)
* DT_STRSZ: 字符串表的大小, d_val存放字节数(必需)
* DT_SYMTAB: 符号表, d_ptr存放虚拟地址(必需), 类型为Elf32_Sym
* DT_INIT: 初始化函数, d_ptr存放虚拟地址
* DT_SYMBOLIC: 影响函数soinfo_do_lookup查找符号的顺序
* DT_REL: 重定位表, dt_ptr存放重定位表的地址, 类型为Elf32_Rel
* DT_RELSZ: 重定位表的大小, d_val存放字节数
* DT_PLTREL: 重定位项的类型, d_val表示DT_REL或DT_RELA, 这里只支持DT_REL
* DT_TEXTREL: 是否需要重定位
* DT_JMPREL: 重定位表, d_ptr存放重定位表的地址, 类型为Elf32_Rel
* DT_PLTRELSZ: 重定位表的大小, d_val存放字节数
* DT_INIT_ARRAY: 初始化函数数组, d_ptr中存放虚拟地址
* DT_INIT_ARRAYSZ: 初始化函数数组的大小, d_val中存放字节数
* DT_PLTGOT: PLT过程偏移表(R_ARM_JMP_SLOT), GOT全局偏移表(R_ARM_GOT), d_ptr中存放虚拟地址
*/
typedef struct {
	Elf32_Word		d_tag;	/* entry tag value 表明哪一类的Dynamic节 */
	union {
		Elf32_Addr	d_ptr;	//虚拟地址
		Elf32_Word	d_val;	//整数值, 根据不同的tag有不同的解释
	} d_un;
} Elf32_Dyn;

typedef struct {
	Elf64_Xword		d_tag;	/* entry tag value */
	union {
		Elf64_Addr	d_ptr;
		Elf64_Xword	d_val;
	} d_un;
} Elf64_Dyn;

/* d_tag */
#define DT_NULL		0	/* Marks end of dynamic array */
#define DT_NEEDED	1	/* Name of needed library (DT_STRTAB offset)  */
#define DT_PLTRELSZ	2	/* Size, in bytes, of relocations in PLT */
#define DT_PLTGOT	3	/* Address of PLT and/or GOT ,R_ARM_GLOB_DAT */
#define DT_HASH		4	/* Address of symbol hash table */
#define DT_STRTAB	5	/* Address of string table */
#define DT_SYMTAB	6	/* Address of symbol table */
#define DT_RELA		7	/* Address of Rela relocation table */
#define DT_RELASZ	8	/* Size, in bytes, of DT_RELA table */
#define DT_RELAENT	9	/* Size, in bytes, of one DT_RELA entry */
#define DT_STRSZ	10	/* Size, in bytes, of DT_STRTAB table */
#define DT_SYMENT	11	/* Size, in bytes, of one DT_SYMTAB entry */
#define DT_INIT		12	/* Address of initialization function */
#define DT_FINI		13	/* Address of termination function */
#define DT_SONAME	14	/* Shared object name (DT_STRTAB offset) */
#define DT_RPATH	15	/* Library search path (DT_STRTAB offset) */
#define DT_SYMBOLIC	16	/* Start symbol search within local object */
#define DT_REL		17	/* Address of Rel relocation table */
#define DT_RELSZ	18	/* Size, in bytes, of DT_REL table */
#define DT_RELENT	19	/* Size, in bytes, of one DT_REL entry */
#define DT_PLTREL	20	/* Type of PLT relocation entries */
#define DT_DEBUG	21	/* Used for debugging; unspecified */
#define DT_TEXTREL	22	/* Relocations might modify non-writable seg */
#define DT_JMPREL	23	/* Address of relocations associated with PLT */
#define DT_BIND_NOW	24	/* Process all relocations at load-time */
#define DT_INIT_ARRAY	25	/* Address of initialization function array */
#define DT_FINI_ARRAY	26	/* Size, in bytes, of DT_INIT_ARRAY array */
#define DT_INIT_ARRAYSZ 27	/* Address of termination function array */
#define DT_FINI_ARRAYSZ 28	/* Size, in bytes, of DT_FINI_ARRAY array*/
#define DT_NUM		29

#define DT_LOOS		0x60000000	/* Operating system specific range */
#define DT_VERSYM	0x6ffffff0	/* Symbol versions */
#define DT_FLAGS_1	0x6ffffffb	/* ELF dynamic flags */
#define DT_VERDEF	0x6ffffffc	/* Versions defined by file */
#define DT_VERDEFNUM	0x6ffffffd	/* Number of versions defined by file */
#define DT_VERNEED	0x6ffffffe	/* Versions needed by file */
#define DT_VERNEEDNUM	0x6fffffff	/* Number of versions needed by file */
#define DT_HIOS		0x6fffffff
#define DT_LOPROC	0x70000000	/* Processor-specific range */
#define DT_HIPROC	0x7fffffff

/* Flag values for DT_FLAGS_1 (incomplete) */
#define DF_1_BIND_NOW	0x00000001	/* Same as DF_BIND_NOW */
#define DF_1_NODELETE	0x00000008	/* Set the RTLD_NODELETE for object */
#define DF_1_INITFIRST	0x00000020	/* Object's init/fini take priority */
#define DF_1_NOOPEN	0x00000040	/* Do not allow loading on dlopen() */

#pragma pack(pop)