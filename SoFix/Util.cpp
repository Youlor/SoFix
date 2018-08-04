#include "Util.h"
#include <windows.h>

void * Util::mmap(void *addr, uint32_t size, QFile &file, uint32_t offset)
{
	void *ret = nullptr;
	size = PAGE_END(size);
	addr = (void *)PAGE_END((size_t)addr);
	if (file.seek(offset))
	{
		ret = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
		assert(((uint32_t)ret % 0x1000 == 0));
		qint64 rc = file.read((char *)ret, size);
		if (rc != size)
		{
			VirtualFree(ret, size, MEM_DECOMMIT);
			return nullptr;
		}

		if (addr)
		{
			memcpy(addr, ret, size);
			VirtualFree(ret, size, MEM_DECOMMIT);
			ret = addr;
		}
	}

	return ret;
}

void * Util::mmap(void *addr, uint32_t size)
{
	size = PAGE_END(size);
	addr = (void *)PAGE_END((size_t)addr);
	if (addr)
	{
		memset(addr, 0, size);
		return addr;
	}
	else
	{
		return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
	}
}

int Util::munmap(void *addr, uint32_t size)
{
	size = PAGE_END(size);
	addr = (void *)PAGE_END((size_t)addr);
	if (addr)
	{
		VirtualFree(addr, size, MEM_DECOMMIT);
	}
	
	return 0;
}
