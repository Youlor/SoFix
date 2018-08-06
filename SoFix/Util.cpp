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
		if (rc < 0)
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

void Util::kmpGetNext(const char *p, int pSize, int next[])
{
	int pLen = pSize;
	next[0] = -1;
	int k = -1;
	int j = 0;
	while (j < pLen - 1)
	{
		//p[k]表示前缀，p[j]表示后缀    
		if (k == -1 || p[j] == p[k])
		{
			++j;
			++k;
			//较之前next数组求法，改动在下面4行  
			if (p[j] != p[k])
				next[j] = k;   //之前只有这一行  
			else
				//因为不能出现p[j] = p[ next[j ]]，所以当出现时需要继续递归，k = next[k] = next[next[k]]  
				next[j] = next[k];
		}
		else
		{
			k = next[k];
		}
	}
}

int Util::kmpSearch(const char *s, int sSize,const char *p, int pSize)
{
	int i = 0;
	int j = 0;
	int sLen = sSize;
	int pLen = pSize;
	int *next = new int[pSize];

	memset(next, 0, pSize * sizeof(int));
	kmpGetNext(p, pSize, next);

	while (i < sLen && j < pLen)
	{
		//1. 如果j = -1, 或者当前字符匹配成功（即S[i] == P[j], 都令i++, j++      
		if (j == -1 || s[i] == p[j])
		{
			i++;
			j++;
		}
		else
		{
			//2. 如果j != -1, 且当前字符匹配失败（即S[i] != P[j], 则令 i 不变，j = next[j]      
			//next[j]即为j所对应的next值        
			j = next[j];
		}
	}

	delete next;
	if (j == pLen)
	{
		return i - j;
	}
	else
	{
		return -1;
	}
}
