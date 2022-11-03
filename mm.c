/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Team 8",
    /* First member's full name */
    "Choi Dabom",
    /* First member's email address */
    "dabomi0305@naver.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE               4
#define DSIZE               8
#define MINIMUM             16
#define CHUNKSIZE           (1 << 12)

#define MAX(x, y)           ((x) > (y) ? (x) : (y))
#define PACK(size, alloc)   ((size) | (alloc))      

#define GET(p)              (*(unsigned int *)(p))
#define PUT(p, val)         (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)         (GET(p) & ~0x7)
#define GET_ALLOC(p)        (GET(p) & 0x1)

#define HDRP(bp)            ((char *)(bp) - WSIZE)
#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 

// Free list 상에서의 이전, 이후 블록의 포인터를 리턴한다. 
#define PREC_FREEP(bp)      (*(void **)(bp))             // 이전 블록의 bp
#define SUCC_FREEP(bp)      (*(void **)(bp + WSIZE))     // 이후 블록의 bp

static void *heap_listp = NULL;    // 항상 prologue block을 가리키는 정적 전역 변수 설정
static void *free_listp = NULL;    // free list의 맨 첫 블록을 가리키는 포인터

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

// mm_init: 최초의 가용블록(6워드)를 가져오고 힙을 생성하고, 할당기를 초기화한다.
int mm_init(void)
{   
    /* 메모리에서 6 words를 가져오고 이걸로 빈 가용 리스트 초기화 */
    if ((heap_listp = mem_sbrk(6*WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);                                 /* Alignment padding: 더블 워드 경계로 정렬된 미사용 패딩 */
    PUT(heap_listp + (1 * WSIZE), PACK(MINIMUM, 1));    /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), NULL);                /* Prologue block 안의 PREC 포인터 NULL로 초기화*/
    PUT(heap_listp + (3 * WSIZE), NULL);                /* Prologue block 안의 SUCC 포인터 NULL로 초기화*/
    PUT(heap_listp + (4 * WSIZE), PACK(MINIMUM, 1));    /* Prologue footer */
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));          /* Epilogue header */

    free_listp = heap_listp + (2 * WSIZE);              /* free_listp를 탐색의 시작점으로 둔다.*/

    // 그 후 CHUNKSIZE만큼 힙을 확장해 초기 가용 블록을 생성한다.
    // CHUNKSIZE는 바이트이기에 WSIZE로 나눠줌으로써 워드 단위로 만듬 
    // 즉, extend_heap은 워드 단위로 받음
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
 
    return 0;
}

// extend_heap: 워드 단위 메모리를 인자로 받아 힙을 늘려준다. 
static void *extend_heap(size_t words)
{
    char *bp;       // 블록 포인터 선언
    size_t size;    // 힙 영역의 크기를 담을 변수 선언

    // 더블 워드 정렬에 따라 메모리를 mem_sbrk 함수를 이용해 할당받는다.
    // Double Word Alignment: 늘 짝수 개수의 워드를 할당해주어야 한다.
    size = (words % 2) ? (words + 1) * WSIZE : (words) * WSIZE; // words가 홀수라면 1을 더한 후 4바이트를 곱하고, 짝수라면 그대로 4바이트를 곱해서 size에 저장 (즉, size는 힙의 총 byte 수)
    if ((long)(bp = mem_sbrk(size)) == -1)                    // 새 메모리의 첫 부분을 bp로 둔다. 주소값은 int로는 못 받아서 long으로 casting
        return NULL;

    // 새 가용 블록의 header와 footer를 정해주고 epilogue block을 가용 블록 맨 끝으로 옮긴다.
    PUT(HDRP(bp), PACK(size, 0));         // free block header
    PUT(FTRP(bp), PACK(size, 0));         // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epliogue header: 항상 size 0, alloc 1

    // 만약 이전 블록이 가용 블록이라면 연결, 통합된 블록의 블록 포인터를 리턴
    return coalesce(bp);
}

// coalesce: 해당 가용 블록을 앞 뒤 가용 블록과 연결하고 가용 블록의 주소를 리턴한다. 
static void *coalesce(void *bp){   
    // bp: free 상태의 블록의 payload를 가리키고 있는 포인터 

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 직전 블록의 헤더에서 가용 여부
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 직후 블록의 푸터에서 가용 여부
    size_t size = GET_SIZE(HDRP(bp));

    // case 1 : 직전, 직후 블록이 모두 할당 -> 해당 블록만 free list에 넣어주면 된다.

    // case 2 : 직전 블록 할당, 직후 블록 가용
    if(prev_alloc && !next_alloc){
        removeBlock(NEXT_BLKP(bp));    // free 상태였던 직후 블록을 free list에서 제거한다.
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // case 3 : 직전 블록 가용, 직후 블록 할당
    else if(!prev_alloc && next_alloc){
        removeBlock(PREV_BLKP(bp));    // 직전 블록을 free list에서 제거한다.
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp); 
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));  
    }

    // case 4 : 직전, 직후 블록 모두 가용
    else if (!prev_alloc && !next_alloc) {
        removeBlock(PREV_BLKP(bp));
        removeBlock(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));  
        PUT(FTRP(bp), PACK(size, 0));  
    }

    // 연결된 새 가용 블록을 free list에 추가한다.
    putFreeBlock(bp);

    return bp;
}


// mm_malloc: brk 포인터를 증가시켜 블록을 할당한다.
void *mm_malloc(size_t size)
{
    size_t asize;           // 정렬 조건과 헤더 & 푸터 용량을 고려하여 조정된 블록 사이즈
    size_t extendsize;      // 메모리를 할당할 자리가 없을 때 (no fit) 힙을 연장할 크기
    char *bp;

    // 가짜 요청 spurious request 무시
    if (size == 0)
        return NULL;

    if (size <= DSIZE)      // 정렬 조건 및 오버헤드(= 헤더 & 푸터 크기)를 고려하여 블록 사이즈 조정
        asize = MINIMUM;  // 요청받은 크기가 8바이트보다 작으면 aszie를 16바이트로 만든다. 
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
        // 요청받은 크기가 8바이트 보다 크다면, 사이에 8바이트를 더하고 (오버헤드) 다시 7을 더해서(올림 효과를 주기 위함) 8로 나눈 몫에 8을 곲한다. 

    // 가용 리스트에서 적합한 자리를 찾는다.
    if ((bp = find_fit(asize)) != NULL)
    {                       
        place(bp, asize);   
        return bp;
    }

    // 만약 맞는 크기의 가용 블록이 없다면 새로 힙을 늘려서
    extendsize = MAX(asize, CHUNKSIZE); // 둘 중 더 큰 값으로 사이즈를 정한다.
    // extend_heap()은 word 단위로 인자를 받으므로 WSIZE로 나눠준다.
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    // 새 힙에 메모리를 할당한다.
    place(bp, asize);
    return bp;
}

// mm_free: 블록 반환
void mm_free(void *bp)
{
    // 해당 블록의 size를 알아내 header와 footer의 정보를 수정한다.
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    // 만약 앞뒤의 블록이 가용 상태라면 연결한다.
    coalesce(bp);
}

// first_fit: free_list의 맨 처음부터 탐색하여 요구하는 메모리 공간보다 큰 가용 블록의 주소를 반환한다.
static void *find_fit(size_t asize)
{
    void* bp;
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) != 1; bp = SUCC_FREEP(bp)){
        if(asize <= GET_SIZE(HDRP(bp))){
            return bp;
        }
    }
    // 못 찾으면 NULL을 리턴한다.
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));

    // 할당될 블록이므로 free list에서 없애준다.
    removeBlock(bp);
    
    // 분할이 가능한 경우
    // => 남은 메모리가 최소한의 가용 블록을 만들 수 있는 4word(16byte)가 되느냐
    // => why? 적어도 16바이트 이상이어야 이용할 가능성이 있음 
    // header & footer: 1word씩, payload: 1word, 정렬 위한 padding: 1word = 4words
    if ((csize - asize) >= MINIMUM)
    {
        // 앞의 블록은 할당 블록으로
        PUT(HDRP(bp), PACK(asize, 1));          // 헤더값 갱신
        PUT(FTRP(bp), PACK(asize, 1));          // 푸터값 갱신
        // 뒤의 블록은 가용 블록으로 분할한다.
        bp = NEXT_BLKP(bp);                     // 다음블록 위치로 bp 이동
        PUT(HDRP(bp), PACK(csize - asize, 0));  // 남은 가용 블록의 헤더 갱신
        PUT(FTRP(bp), PACK(csize - asize, 0));  // 남은 가용 블록의 푸터 갱신

        // free list 첫번째에 분할된 블럭을 넣는다.
        putFreeBlock(bp);
    }
    // 분할할 수 없다면 남은 부분은 padding한다.
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

// putFreeBlock(bp): 새로 반환되거나 생성된 가용 블록을 free list의 첫 부분에 넣는다. 
void putFreeBlock(void *bp){
    SUCC_FREEP(bp) = free_listp;
    PREC_FREEP(bp) = NULL;
    PREC_FREEP(free_listp) = bp;
    free_listp = bp;    
}

// removeBlock: 할당되거나 연결되는 가용 블록을 free list에서 없앤다.
void removeBlock(void *bp){
    // free list의 첫 번째 블록을 없앨 때
    if (bp == free_listp){
        PREC_FREEP(SUCC_FREEP(bp)) = NULL;
        free_listp = SUCC_FREEP(bp);
    }
    else {
        SUCC_FREEP(PREC_FREEP(bp)) = SUCC_FREEP(bp);
        PREC_FREEP(SUCC_FREEP(bp)) = PREC_FREEP(bp);
    }
}

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;  // 크기를 조절하고 싶은 힙의 시작 포인터
    void *newptr;        // 크기 조절 뒤의 새 힙의 시작 포인터
    size_t copySize;     // 복사할 힙의 크기
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    copySize = GET_SIZE(HDRP(oldptr));

    // 원래 메모리 크기보다 적은 크기를 realloc하면 크기에 맞는 메모리만 할당되고 나머지는 안 된다. 
    if (size < copySize)
      copySize = size;

    memcpy(newptr, oldptr, copySize);  // newptr에 oldptr를 시작으로 copySize만큼의 메모리 값을 복사한다.
    mm_free(oldptr);  // 기존의 힙을 반환한다.
    return newptr;
}

