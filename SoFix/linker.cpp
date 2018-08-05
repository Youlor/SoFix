#include "linker.h"
#include <QDebug>
#include <malloc.h>

#define DEBUG qDebug
#define DL_ERR qDebug

/*
* Copy src to string dst of size siz.  At most siz-1 characters
* will be copied.  Always NUL terminates (unless siz == 0).
* Returns strlen(src); if retval >= siz, truncation occurred.
*/
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) 
	{
		while (--n != 0)
		{
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) 
	{
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

soinfo* soinfo_alloc(const char* name) 
{
	if (strlen(name) >= SOINFO_NAME_LEN)
	{
		DL_ERR("library name \"%s\" too long", name);
		return NULL;
	}

	soinfo* si = (soinfo *)malloc(sizeof(soinfo));
	memset(si, 0, sizeof(soinfo));
	strlcpy(si->name, name, sizeof(si->name));

	DEBUG("name %s: allocated soinfo @ %p", name, si);
	return si;
}

/* Return the address and size of the ELF file's .dynamic section in memory,
* or NULL if missing. 遍历phdr_table, 返回.dynamic节的内存地址与数量以及保护属性
*
* Input:
*   phdr_table  -> program header table
*   phdr_count  -> number of entries in tables
*   load_bias   -> load bias
* Output:
*   dynamic       -> address of table in memory (NULL on failure).
*   dynamic_count -> number of items in table (0 on failure).
*   dynamic_flags -> protection flags for section (unset on failure)
* Return:
*   void
*/
void
phdr_table_get_dynamic_section(const Elf32_Phdr* phdr_table,
	int               phdr_count,
	Elf32_Addr        load_bias,
	Elf32_Dyn**       dynamic,
	size_t*           dynamic_count,
	Elf32_Word*       dynamic_flags)
{
	const Elf32_Phdr* phdr = phdr_table;
	const Elf32_Phdr* phdr_limit = phdr + phdr_count;

	for (phdr = phdr_table; phdr < phdr_limit; phdr++)
	{
		if (phdr->p_type != PT_DYNAMIC) 
		{
			continue;
		}

		*dynamic = reinterpret_cast<Elf32_Dyn*>(load_bias + phdr->p_vaddr);
		if (dynamic_count) 
		{
			*dynamic_count = (unsigned)(phdr->p_memsz / 8);
		}
		if (dynamic_flags) 
		{
			*dynamic_flags = phdr->p_flags;
		}
		return;
	}
	*dynamic = NULL;
	if (dynamic_count)
	{
		*dynamic_count = 0;
	}
}



/* Return the address and size of the .ARM.exidx section in memory,
* if present. 遍历phdr_table, 如果.ARM.exidx节存在则返回地址和数量
*
* Input:
*   phdr_table  -> program header table
*   phdr_count  -> number of entries in tables
*   load_bias   -> load bias
* Output:
*   arm_exidx       -> address of table in memory (NULL on failure).
*   arm_exidx_count -> number of items in table (0 on failure).
* Return:
*   0 on error, -1 on failure (_no_ error code in errno)
*/

#define PT_ARM_EXIDX    0x70000001      /* .ARM.exidx segment */

int
phdr_table_get_arm_exidx(const Elf32_Phdr* phdr_table,
	int               phdr_count,
	Elf32_Addr        load_bias,
	Elf32_Addr**      arm_exidx,
	unsigned*         arm_exidx_count)
{
	const Elf32_Phdr* phdr = phdr_table;
	const Elf32_Phdr* phdr_limit = phdr + phdr_count;

	for (phdr = phdr_table; phdr < phdr_limit; phdr++) 
	{
		if (phdr->p_type != PT_ARM_EXIDX)
			continue;

		*arm_exidx = (Elf32_Addr*)(load_bias + phdr->p_vaddr);
		*arm_exidx_count = (unsigned)(phdr->p_memsz / 8);
		return 0;
	}
	*arm_exidx = NULL;
	*arm_exidx_count = 0;
	return -1;
}

/* TODO: don't use unsigned for addrs below. It works, but is not
* ideal. They should probably be either uint32_t, Elf32_Addr, or unsigned
* long.
*/

//so的重定位
int soinfo_relocate(soinfo* si, Elf32_Rel* rel, unsigned count,
	soinfo* needed[])
{
//	Elf32_Sym* symtab = si->symtab;
//	const char* strtab = si->strtab;
//	Elf32_Sym* s;
//	Elf32_Rel* start = rel;
//	soinfo* lsi;
//
//	//遍历重定位节 DT_JMPREL/DT_REL
//	for (size_t idx = 0; idx < count; ++idx, ++rel) 
//	{
//		unsigned type = ELF32_R_TYPE(rel->r_info);
//		unsigned sym = ELF32_R_SYM(rel->r_info);
//		Elf32_Addr reloc = static_cast<Elf32_Addr>(rel->r_offset + si->load_bias); //获取需要重定位的地址
//		Elf32_Addr sym_addr = 0;
//		char* sym_name = NULL;
//
//		DEBUG("Processing '%s' relocation at index %d", si->name, idx);
//		if (type == 0) // R_*_NONE
//		{ 
//			continue;
//		}
//		if (sym != 0) 
//		{
//			sym_name = (char *)(strtab + symtab[sym].st_name); //获取符号名
//			s = soinfo_do_lookup(si, sym_name, &lsi, needed); //根据符号名称获取符号
//			if (s == NULL)
//			{
//				/* We only allow an undefined symbol if this is a weak
//				reference..   允许存在未定义的弱引用符号STB_WEAK */
//				s = &symtab[sym];
//				if (ELF32_ST_BIND(s->st_info) != STB_WEAK) 
//				{
//					DL_ERR("cannot locate symbol \"%s\" referenced by \"%s\"...", sym_name, si->name);
//					return -1;
//				}
//
//				/* IHI0044C AAELF 4.5.1.1:
//
//				Libraries are not searched to resolve weak references.
//				It is not an error for a weak reference to remain
//				unsatisfied.
//
//				During linking, the value of an undefined weak reference is:
//				- Zero if the relocation type is absolute
//				- The address of the place if the relocation is pc-relative
//				- The address of nominal base address if the relocation
//				type is base-relative.
//
//				不能是下面的这些类型
//				*/
//
//				switch (type) {
//#if defined(ANDROID_ARM_LINKER)
//				case R_ARM_JUMP_SLOT:
//				case R_ARM_GLOB_DAT:
//				case R_ARM_ABS32:
//				case R_ARM_RELATIVE:    /* Don't care. */
//#elif defined(ANDROID_X86_LINKER)
//				case R_386_JMP_SLOT:
//				case R_386_GLOB_DAT:
//				case R_386_32:
//				case R_386_RELATIVE:    /* Dont' care. */
//#endif /* ANDROID_*_LINKER */
//										/* sym_addr was initialized to be zero above or relocation
//										code below does not care about value of sym_addr.
//										No need to do anything.  */
//					break;
//
//#if defined(ANDROID_X86_LINKER)
//				case R_386_PC32:
//					sym_addr = reloc;
//					break;
//#endif /* ANDROID_X86_LINKER */
//
//#if defined(ANDROID_ARM_LINKER)
//				case R_ARM_COPY:
//					/* Fall through.  Can't really copy if weak symbol is
//					not found in run-time.  */
//#endif /* ANDROID_ARM_LINKER */
//				default:
//					DL_ERR("unknown weak reloc type %d @ %p (%d)",
//						type, rel, (int)(rel - start));
//					return -1;
//				}
//			}
//			else {
//				/* We got a definition.  */
//#if 0
//				if ((base == 0) && (si->base != 0)) {
//					/* linking from libraries to main image is bad */
//					DL_ERR("cannot locate \"%s\"...",
//						strtab + symtab[sym].st_name);
//					return -1;
//				}
//#endif
//				sym_addr = static_cast<Elf32_Addr>(s->st_value + lsi->load_bias); //获取符号地址
//			}
//			count_relocation(kRelocSymbol);
//		}
//		else {
//			s = NULL;
//		}
//
//		/* TODO: This is ugly. Split up the relocations by arch into
//		* different files.
//		*/
//		//进行重定位: 符号地址 --- 重定位地址
//		switch (type) {
//#if defined(ANDROID_ARM_LINKER)
//		case R_ARM_JUMP_SLOT:
//			count_relocation(kRelocAbsolute); //记录linker状态的linker_stats
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO JMP_SLOT %08x <- %08x %s", reloc, sym_addr, sym_name);
//			*reinterpret_cast<Elf32_Addr*>(reloc) = sym_addr;
//			break;
//		case R_ARM_GLOB_DAT:
//			count_relocation(kRelocAbsolute);
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO GLOB_DAT %08x <- %08x %s", reloc, sym_addr, sym_name);
//			*reinterpret_cast<Elf32_Addr*>(reloc) = sym_addr;
//			break;
//		case R_ARM_ABS32:
//			count_relocation(kRelocAbsolute);
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO ABS %08x <- %08x %s", reloc, sym_addr, sym_name);
//			*reinterpret_cast<Elf32_Addr*>(reloc) += sym_addr;
//			break;
//		case R_ARM_REL32:
//			count_relocation(kRelocRelative);
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO REL32 %08x <- %08x - %08x %s",
//				reloc, sym_addr, rel->r_offset, sym_name);
//			*reinterpret_cast<Elf32_Addr*>(reloc) += sym_addr - rel->r_offset;
//			break;
//#elif defined(ANDROID_X86_LINKER)
//		case R_386_JMP_SLOT:
//			count_relocation(kRelocAbsolute);
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO JMP_SLOT %08x <- %08x %s", reloc, sym_addr, sym_name);
//			*reinterpret_cast<Elf32_Addr*>(reloc) = sym_addr;
//			break;
//		case R_386_GLOB_DAT:
//			count_relocation(kRelocAbsolute);
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO GLOB_DAT %08x <- %08x %s", reloc, sym_addr, sym_name);
//			*reinterpret_cast<Elf32_Addr*>(reloc) = sym_addr;
//			break;
//#elif defined(ANDROID_MIPS_LINKER)
//		case R_MIPS_REL32:
//			count_relocation(kRelocAbsolute);
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO REL32 %08x <- %08x %s",
//				reloc, sym_addr, (sym_name) ? sym_name : "*SECTIONHDR*");
//			if (s) {
//				*reinterpret_cast<Elf32_Addr*>(reloc) += sym_addr;
//			}
//			else {
//				*reinterpret_cast<Elf32_Addr*>(reloc) += si->base;
//			}
//			break;
//#endif /* ANDROID_*_LINKER */
//
//#if defined(ANDROID_ARM_LINKER)
//		case R_ARM_RELATIVE:
//#elif defined(ANDROID_X86_LINKER)
//		case R_386_RELATIVE:
//#endif /* ANDROID_*_LINKER */
//			count_relocation(kRelocRelative);
//			MARK(rel->r_offset);
//			if (sym) {
//				DL_ERR("odd RELATIVE form...");
//				return -1;
//			}
//			TRACE_TYPE(RELO, "RELO RELATIVE %08x <- +%08x", reloc, si->base);
//			*reinterpret_cast<Elf32_Addr*>(reloc) += si->base;
//			break;
//
//#if defined(ANDROID_X86_LINKER)
//		case R_386_32:
//			count_relocation(kRelocRelative);
//			MARK(rel->r_offset);
//
//			TRACE_TYPE(RELO, "RELO R_386_32 %08x <- +%08x %s", reloc, sym_addr, sym_name);
//			*reinterpret_cast<Elf32_Addr*>(reloc) += sym_addr;
//			break;
//
//		case R_386_PC32:
//			count_relocation(kRelocRelative);
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO R_386_PC32 %08x <- +%08x (%08x - %08x) %s",
//				reloc, (sym_addr - reloc), sym_addr, reloc, sym_name);
//			*reinterpret_cast<Elf32_Addr*>(reloc) += (sym_addr - reloc);
//			break;
//#endif /* ANDROID_X86_LINKER */
//
//#ifdef ANDROID_ARM_LINKER
//		case R_ARM_COPY:
//			if ((si->flags & FLAG_EXE) == 0) {
//				/*
//				* http://infocenter.arm.com/help/topic/com.arm.doc.ihi0044d/IHI0044D_aaelf.pdf
//				*
//				* Section 4.7.1.10 "Dynamic relocations"
//				* R_ARM_COPY may only appear in executable objects where e_type is
//				* set to ET_EXEC.
//				*
//				* TODO: FLAG_EXE is set for both ET_DYN and ET_EXEC executables.
//				* We should explicitly disallow ET_DYN executables from having
//				* R_ARM_COPY relocations.
//				*/
//				DL_ERR("%s R_ARM_COPY relocations only supported for ET_EXEC", si->name);
//				return -1;
//			}
//			count_relocation(kRelocCopy);
//			MARK(rel->r_offset);
//			TRACE_TYPE(RELO, "RELO %08x <- %d @ %08x %s", reloc, s->st_size, sym_addr, sym_name);
//			if (reloc == sym_addr) {
//				Elf32_Sym *src = soinfo_do_lookup(NULL, sym_name, &lsi, needed);
//
//				if (src == NULL) {
//					DL_ERR("%s R_ARM_COPY relocation source cannot be resolved", si->name);
//					return -1;
//				}
//				if (lsi->has_DT_SYMBOLIC) {
//					DL_ERR("%s invalid R_ARM_COPY relocation against DT_SYMBOLIC shared "
//						"library %s (built with -Bsymbolic?)", si->name, lsi->name);
//					return -1;
//				}
//				if (s->st_size < src->st_size) {
//					DL_ERR("%s R_ARM_COPY relocation size mismatch (%d < %d)",
//						si->name, s->st_size, src->st_size);
//					return -1;
//				}
//				memcpy((void*)reloc, (void*)(src->st_value + lsi->load_bias), src->st_size);
//			}
//			else {
//				DL_ERR("%s R_ARM_COPY relocation target cannot be resolved", si->name);
//				return -1;
//			}
//			break;
//#endif /* ANDROID_ARM_LINKER */
//
//		default:
//			DL_ERR("unknown reloc type %d @ %p (%d)",
//				type, rel, (int)(rel - start));
//			return -1;
//		}
//	}
//	return 0;
	return 0;
}
