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


/*explicit free list start*/
/*8个字节=1个字*/
#define WSIZE 8 
#define DSIZE 16
#define CHUNKSIZE (1 << 12)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
//size以字节为单位
#define PACK(size, prev_alloc, alloc) ((size) & ~(1<<1) | (prev_alloc << 1) & ~(1) | (alloc))
#define PACK_PREV_ALLOC(val, prev_alloc) ((val) & ~(1<<1) | (prev_alloc << 1))
#define PACK_ALLOC(val, alloc) ((val) | (alloc))

#define GET(p) (*(unsigned long *)(p))
#define PUT(p, val) (*(unsigned long *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) ((GET(p) & 0x2) >> 1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) /*only for free blk*/
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
/*only when prev_block is free, which can usd*/
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

#define GET_PRED(bp) (GET(bp))
#define SET_PRED(bp, val) (PUT(bp, val))

#define GET_SUCC(bp) (GET(bp + WSIZE))
#define SET_SUCC(bp, val) (PUT(bp + WSIZE, val))

#define MIN_BLK_SIZE (2 * DSIZE)
/*explicit free list end*/

/* single word (4) or double word (8) alignment */
#define ALIGNMENT DSIZE

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *heap_listp;
static char *free_listp;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
// static void *find_fit(size_t asize);
static void *find_fit_best(size_t asize);
static void *find_fit_first(size_t asize);
static void place(void *bp, size_t asize);
static void add_to_free_list(void *bp);
static void delete_from_free_list(void *bp);
double get_utilization();
void mm_check(const char *);

/*
    TODO:
        完成一个简单的分配器内存使用率统计
        user_malloc_size: 用户申请内存量
        heap_size: 分配器占用内存量
    HINTS:
        1. 在适当的地方修改上述两个变量，细节参考实验文档
        2. 在 get_utilization() 中计算使用率并返回
*/
size_t user_malloc_size = 0, heap_size = 0;

double get_utilization() {
    return (double) ((user_malloc_size * 1.0) / heap_size); 
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    free_listp = NULL;

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)//此时mem_brk在结束块的后面一个作为bp
        return -1;
    //TODO:heap size
    heap_size = 4*WSIZE;
    user_malloc_size = 0;

    PUT(heap_listp, 0);//start_mem_brk不指示内容，用来作为字的对齐
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1, 1));//序言块头
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1, 1));//序言块尾
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1, 1));//结束块
    heap_listp += (2 * WSIZE);//作为分配堆的头bp，序言头和尾

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)//分配4MB,以字为单位输入
        return -1;
    /* mm_check(__FUNCTION__);*/
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    //printf("\nin size to be malloced : size=%u, heap_size= %lu\n", size,heap_size);
    /*mm_check(__FUNCTION__);*/
    size_t newsize;
    size_t extend_size;
    char *bp;

    if (size == 0)
        return NULL;
    newsize = MAX(MIN_BLK_SIZE, ALIGN((size + WSIZE))); /*size+WSIZE(head_len)，待分配的块大小+头部大小（最少也要分配 4WORD, 32B， 256bits）*/
    /* newsize = MAX(MIN_BLK_SIZE, (ALIGN(size) + DSIZE));*/
    if ((bp = find_fit_best(newsize)) != NULL)
    {   
        place(bp, newsize);//分配堆与分隔空闲部分,从空闲表中取下合适的块，分配给需求的大小

        //TODO:user size
        user_malloc_size += GET_SIZE(HDRP(bp))-WSIZE;

        //printf("use_malloc_size: %lu\n",user_malloc_size);
        return bp;
    }
    /*no fit found.*/
    extend_size = MAX(newsize, CHUNKSIZE);//最少分配4KB
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL)//按字分配
    {
        return NULL;
    }
    place(bp, newsize);
    //TODO:user size
    user_malloc_size += GET_SIZE(HDRP(bp))-WSIZE;
    //printf("use_malloc_size: %lu\n",user_malloc_size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    //printf("\nfree ptr: %p\n", bp);
    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    //printf("free size: %lu\n", size);
    void *head_next_bp = NULL;
    //TODO:user size
    user_malloc_size -= (size-WSIZE);

    PUT(HDRP(bp), PACK(size, prev_alloc, 0));
    PUT(FTRP(bp), PACK(size, prev_alloc, 0));
    /*printf("%s, addr_start=%u, size_head=%u, size_foot=%u\n",*/
    /*    __FUNCTION__, HDRP(bp), (size_t)GET_SIZE(HDRP(bp)), (size_t)GET_SIZE(FTRP(bp)));*/

     /*notify next_block, i am free*/
    head_next_bp = HDRP(NEXT_BLKP(bp));
    PUT(head_next_bp, PACK_PREV_ALLOC(GET(head_next_bp), 0));
    //printf("free of colesce ");
    coalesce(bp);//处理空闲块，将其加入空闲链表中
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
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void *extend_heap(size_t words)
{
    //printf("\nstart extend\n");
    //printf("in extend_heap words = %u\n", words);

    /*get heap_brk*/
    char *old_heap_brk = mem_sbrk(0);
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(old_heap_brk));
    char *bp; 
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;//改正成字节为单位

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    //TODO:heap size
    heap_size = mem_heapsize();
    //printf("extend pass, now the heap_size is :%lu\n", heap_size);
    //设置好空闲块格式
    PUT(HDRP(bp), PACK(size, prev_alloc, 0)); /*last free block*/
    PUT(FTRP(bp), PACK(size, prev_alloc, 0));//新建最后一个空闲块，改变head and foot，把上一段的结束块头部作为新的空闲块的头部

    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1)); /*break block 将堆尾部空闲块下一个块的头设置为空，作为结束块的头部，作为结束快标记*/
    //printf("extend of coalesce ");
    return coalesce(bp);//如果bp前有空闲块，则要合并,处理空闲列表
}

static void *coalesce(void *bp)
{   
    //printf("\nstart coalesce\n");
    /*add_to_free_list(bp);*/
    int c;
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    /*
        TODO:
            将 bp 指向的空闲块 与 相邻块合并
            结合前一块及后一块的分配情况，共有 4 种可能性
            分别完成相应case下的 数据结构维护逻辑
    */
    char * prev_bp = NULL;
    char * next_bp = NULL;
    if (prev_alloc && next_alloc) /* 前后都是已分配的块 */
    {
        c=0;
       add_to_free_list(bp);
    }
    else if (prev_alloc && !next_alloc) /*前块已分配，后块空闲*/
    {
        c=1;
        next_bp = NEXT_BLKP(bp);

        size = size + GET_SIZE(HDRP(next_bp));
        delete_from_free_list(next_bp);
        PUT(FTRP(bp), 0);
        PUT(HDRP(next_bp), 0);
        PUT(HDRP(bp),PACK(size, prev_alloc, 0));
        PUT(FTRP(bp),PACK(size, prev_alloc, 0));
        add_to_free_list(bp);
    }
    else if (!prev_alloc && next_alloc) /*前块空闲，后块已分配,没有尾部*/
    {
        c=2;
        prev_bp = PREV_BLKP(bp);
        size_t prev_prev_alloc = GET_PREV_ALLOC(HDRP(prev_bp));

        size = size+ GET_SIZE(HDRP(prev_bp));
        PUT(FTRP(prev_bp),0);
        PUT(HDRP(prev_bp),PACK(size,prev_prev_alloc,0));
        PUT(FTRP(prev_bp),PACK(size,prev_prev_alloc,0));// FTRP(bp) = (char *)((bp) + GET_SIZE(HDRP(bp)) - DSIZE)
        PUT(HDRP(bp),0);
        bp=prev_bp;
    }
    else /*前后都是空闲块*/
    {
        c=3;
        next_bp = NEXT_BLKP(bp);
        prev_bp = PREV_BLKP(bp);
        size_t prev_prev_alloc = GET_PREV_ALLOC(HDRP(prev_bp));

        size = size + GET_SIZE(HDRP(next_bp)) + GET_SIZE(HDRP(prev_bp));
        delete_from_free_list(next_bp);

        PUT(HDRP(prev_bp),PACK(size,prev_prev_alloc,0));
        PUT(FTRP(prev_bp),PACK(size,prev_prev_alloc,0));
        bp=prev_bp;
    }
    //printf(" case%d: total size: %lu\n",c,size);
    //printf("\ncoalesce pass\n");
    return bp;
}

static void *find_fit_first(size_t asize)//以字节为单位的asize
{   
    int t=0;
    //printf("\nstart find\n");
    /* 
        首次匹配算法
        TODO:
            遍历 freelist， 找到第一个合适的空闲块后返回
        
        HINT: asize 已经计算了块头部的大小
    */
    char * ptr = free_listp;
    while (ptr != 0 && GET_SIZE(HDRP(ptr)) < asize   ){//必须先判断指针是否为0，否则会出现指针越界的情况
        t++;
        //printf("%d ptr:%p, next ptr:%p\n",t,ptr, GET_SUCC(ptr));
        ptr = GET_SUCC(ptr);
   }
    //printf("jump out of while\n");
    /*if(ptr == 0){ 
        printf("not found: prt = NULL\n");
    }
    else{
        //printf("find pass: bp = %p\n" ,ptr);
        printf("find size:%lu",GET_SIZE(HDRP(ptr)));
    }*/
   return ptr; // 换成实际返回值
}

static void* find_fit_best(size_t asize) {
    /* 
        最佳配算法
        TODO:
            遍历 freelist， 找到最合适的空闲块，返回
        
        HINT: asize 已经计算了块头部的大小
    */
    char * ptr = free_listp;
    char * fit_best_ptr=NULL;
    
    while (ptr != 0){
        if(asize <= GET_SIZE(HDRP(ptr))){
            if((fit_best_ptr == 0) || ( GET_SIZE(HDRP(ptr)) < GET_SIZE(HDRP(fit_best_ptr)) ) )
                fit_best_ptr = ptr;
        }
        if(fit_best_ptr != 0 && asize == GET_SIZE(HDRP(fit_best_ptr)))
            break;
        ptr = GET_SUCC(ptr);

    }   

    return fit_best_ptr; // 换成实际返回值
}

static void place(void *bp, size_t asize)//bp不变
{
    //printf("\nstart place\n");
    /* 
        TODO:
        将一个空闲块转变为已分配的块

        HINTS:
            1. 若空闲块在分离出一个 asize 大小的使用块后，剩余空间不足空闲块的最小大小，
                则原先整个空闲块应该都分配出去
            2. 若剩余空间仍可作为一个空闲块，则原空闲块被分割为一个已分配块+一个新的空闲块
            3. 空闲块的最小大小已经 #define，或者根据自己的理解计算该值
    */
    size_t bp_size=GET_SIZE(HDRP(bp));
    size_t remain_size = bp_size - asize;

    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));

    char * next_bp = NULL;

    if(remain_size < MIN_BLK_SIZE){
        delete_from_free_list(bp);
        //printf("\nplace case1: remain size < min blk size. free_bp_size: %lu, asize:%lu\n", bp_size, asize);
        PUT(HDRP(bp), PACK(bp_size, prev_alloc, 1));
        next_bp = NEXT_BLKP(bp);
        //printf("bp: %p, next_bp: %p\n",bp, next_bp);
        PUT(HDRP(next_bp), PACK_PREV_ALLOC(GET(HDRP(next_bp)),1));
        
    }else{
        delete_from_free_list(bp);
        //printf("\nplace case2:remain size >= min blk size. free_bp_size: %lu, asize:%lu\n", bp_size, asize);
        PUT(HDRP(bp), PACK(asize, prev_alloc, 1));
        next_bp = NEXT_BLKP(bp);
        //printf("bp: %p , next_bp: %p, Head_np: %p \n",bp, next_bp, HDRP(next_bp));
        PUT(HDRP(next_bp), PACK(remain_size, 1, 0));
        PUT(FTRP(next_bp), PACK(remain_size, 1, 0));
        add_to_free_list(next_bp);
        //printf("AFTER ADD\nnp_value:%lu, HEAD_np_value: %lu\n",GET(next_bp),GET(HDRP(next_bp)));

    }
    //printf("\npalce pass\n");
   
}

static void add_to_free_list(void *bp)
{
    //printf("\nstart add\n");
    /*set pred & succ*/
    if (free_listp == NULL) /*free_list empty*/
    {
        SET_PRED(bp, 0);
        SET_SUCC(bp, 0);
        free_listp = bp;
    }
    else
    {
        SET_PRED(bp, 0);
        SET_SUCC(bp, (size_t)free_listp); /*size_t ???*/
        SET_PRED(free_listp, (size_t)bp);//头插法
        free_listp = bp;//作为head
    }
    //printf("add pass\n");
}

static void delete_from_free_list(void *bp)//从空闲链表中取下一个空闲块
{
    //printf("\nstart delete:\n");
    size_t prev_free_bp=0;
    size_t next_free_bp=0;

    prev_free_bp = GET_PRED(bp);
    next_free_bp = GET_SUCC(bp);

    if (prev_free_bp == next_free_bp && prev_free_bp != 0)
    {
        /*mm_check(__FUNCTION__);*/
        /*printf("\nin delete from list: bp=%u, prev_free_bp=%u, next_free_bp=%u\n", (size_t)bp, prev_free_bp, next_free_bp);*/
    }
    if (prev_free_bp && next_free_bp) /*11*/
    {
        SET_SUCC(prev_free_bp, GET_SUCC(bp));
        SET_PRED(next_free_bp, GET_PRED(bp));
    }
    else if (prev_free_bp && !next_free_bp) /*10*/
    {
        SET_SUCC(prev_free_bp, 0);
    }
    else if (!prev_free_bp && next_free_bp) /*01*/
    {
        SET_PRED(next_free_bp, 0);
        free_listp = (void *)next_free_bp;
    }
    else /*00*/
    {
        free_listp = NULL;
    }
   // printf("delete pass\n");
}

void mm_check(const char *function)
{
    /* printf("\n---cur func: %s :\n", function);
     char *bp = free_listp;
     int count_empty_block = 0;
     while (bp != NULL) //not end block;
     {
         count_empty_block++;
         printf("addr_start：%u, addr_end：%u, size_head:%u, size_foot:%u, PRED=%u, SUCC=%u \n", (size_t)bp - WSIZE,
                (size_t)FTRP(bp), GET_SIZE(HDRP(bp)), GET_SIZE(FTRP(bp)), GET_PRED(bp), GET_SUCC(bp));
         ;
         bp = (char *)GET_SUCC(bp);
     }
     printf("empty_block num: %d\n\n", count_empty_block);*/
}