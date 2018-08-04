#pragma once
#include <QFile>
#include <stdint.h>

#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE-1))
#define PAGE_START(x)  ((x) & PAGE_MASK)
#define PAGE_END(x)    (PAGE_START((x) + (PAGE_SIZE-1)))
#define PAGE_OFFSET(x) ((x) & ~PAGE_MASK)

typedef struct 
{
	void *aligned;
} AlignedMemory;

class Util
{
public:
	Util() = delete;
	~Util() = delete;

	//读取文件内容到内存中, 若addr为NULL则新分配内存, 使用VirtualAlloc保证按页对齐, addr, size将被强制页对齐
	static void *mmap(void *addr, uint32_t size, QFile &file, uint32_t offset);

	//如果addr为NULL则新分配内存, 否则将内存置为0
	static void *mmap(void *addr, uint32_t size);

	//释放由mmap分配的内存
	static int munmap(void *addr, uint32_t size);
};

