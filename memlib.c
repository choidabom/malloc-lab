/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"
#include "config.h"

/* private variables */
static char *mem_start_brk;  /* points to first byte of heap, 힙의 첫 바이트를 가리키는 변수 */
static char *mem_brk;        /* points to last byte of heap, 힙의 마지막 바이트를 가리키는 변수. 여기에다가 sbrk 함수로 새로운 사이지의 힙을 할당받는다.  */
static char *mem_max_addr;   /* largest legal heap address, 힙의 최대 크기(20MB)의 주소를 가리키는 변수 */ 

/* 
 * mem_init - initialize the memory system model
   최대 힙 메모리 공간을 할당받고 초기화해준다.
 */
void mem_init(void)
{
    /* allocate the storage we will use to model the available VM */
    // config.h에 정의되어 있음, #define MAX_HEAP (20*(1<<20)) : 20971520bytes == 20 MB
    // 먼저 20MB만큼의 MAX_HEAP을 malloc으로 동적할당해온다. 만약 메모리를 불러오는데 실패했다면 에러 메세지를 띄우고 프로그램을 종료한다.
    // 그 시작점을 mem_start_brk라 한다.
    // 아직 힙이 비어 있으므로 mem_brk도 mem_start_brk와 같다.
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
	fprintf(stderr, "mem_init_vm: malloc error\n");
	exit(1);
    }
    mem_max_addr = mem_start_brk + MAX_HEAP;  /* max legal heap address */
    mem_brk = mem_start_brk;                  /* heap is empty initially */
}

/* 
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit(void)
{
    free(mem_start_brk);
}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 * by incr bytes and returns the start address of the new area. In
 * this model, the heap cannot be shrunk.
 * byte 단위로 필요 메모리 크기를 입력받아 그 크기만큼 힙을 늘려주고, 새 메모리의 시작 지점을 리턴한다.
 * 힙을 줄이려 하거나 최대 힙 크기를 넘어버리면 리턴한다.
 * old_brk의 주소를 리턴하는 이유: 새로 늘어난 힙의 첫번째 주소이기 때문
 */
void *mem_sbrk(int incr) // incr => 바이트 형태로 입력받음
{   
    char *old_brk = mem_brk; // 힙 늘리기 전의 끝 포인터를 저장한다. 

    // 힙이 줄어들거나 최대 힙 사이즈를 벗어난다면
    // 메모리 부족으로 에러처리하고 -1을 리턴한다.
    if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
	errno = ENOMEM;
	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
	return (void *)-1; // 리턴값이 void*여야해서 형변환
    }
    mem_brk += incr;    // incr(요청된 용량)을 mem_brk에 더해준다. 
    return (void *)old_brk; // old_brk를 리턴하는 이유는 새로 늘어난 힙의 첫번째 주소이기 때문이다.
}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - returns the page size of the system
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}
