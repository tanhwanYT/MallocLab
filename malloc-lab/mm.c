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
    "NS REDFORCE",
    /* First member's full name */
    "taehwanPark",
    /* First member's email address */
    "pkrxoghks1209@gmail.com",
    /* Second member's full name (leave blank if none) */
    "CFO WIN",
    /* Second member's email address (leave blank if none) */
    "KT WIN"};

/* single word (4) or double word (8) alignment */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12) 
#define ALIGNMENT 8

#define MAX(x, y) ((x) > (y) ? (x) : (y))

static char *heap_listp = 0;
static char *last_bp = 0;
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))- DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static void *find_fit(size_t asize);
static void *Next_fit(size_t asize);
static void place(char *bp, size_t asize);
static void *extend_heap(size_t word);
static void *coalesce(char *bp);
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    heap_listp = mem_sbrk(4*WSIZE);
    if(heap_listp == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);  

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0) return NULL;

    if(size <= DSIZE) asize = 2*DSIZE;
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    bp = find_fit(asize);
    if(bp != NULL)
    {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize,CHUNKSIZE);

    bp = extend_heap(extendsize / WSIZE);
    if(bp == NULL) return NULL;
    place(bp, asize);
    return bp;
}

static void *find_fit(size_t asize)
{
    char* next_block = heap_listp;
    
    while (GET_SIZE(HDRP(next_block)) > 0)
    {
        if(asize <= GET_SIZE(HDRP(next_block)) && !GET_ALLOC(HDRP(next_block)))
        {   
            return next_block;  
        }
        else
        {
            next_block = NEXT_BLKP(next_block);
        }
    }
    return NULL;
}

static void *Next_fit(size_t asize)
{
    char *bp = last_bp;
    
    while (GET_SIZE(HDRP(bp)) > 0)
    {
        if(asize <= GET_SIZE(HDRP(bp)) && !GET_ALLOC(HDRP(bp)))
        {   
            last_bp = bp;
            return bp; 
        }
        bp = NEXT_BLKP(bp);
    }

    bp = heap_listp;
    while (bp < last_bp)
    {
        if(asize <= GET_SIZE(HDRP(bp)) && !GET_ALLOC(HDRP(bp)))
        {   
            last_bp = bp;
            return bp;  
        }
        bp = NEXT_BLKP(bp);
    }
    return NULL;
}

static void place(char *bp, size_t asize) // 분할
{
    size_t size = GET_SIZE(HDRP(bp));

    if(size - asize >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size - asize, 0));
        PUT(FTRP(bp), PACK(size - asize, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));      
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

static void *coalesce(char *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc)
    {
        return bp;
    }
    else if(prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *extend_heap(size_t word)
{
    char *bp;
    size_t size;

    size = (word % 2) ? (word + 1) * WSIZE : word * WSIZE;
    bp = mem_sbrk(size);
    if(bp == (void*)-1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);

    if (newptr == NULL)
        return NULL;

    copySize = GET_SIZE(HDRP(ptr)) - DSIZE; 

    if (size < copySize)
        copySize = size;

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}