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
