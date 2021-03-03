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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8 //8배수

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7) //size보다 크고 가장 가까운 8의 배수로 반올림


#define SIZE_T_SIZE (ALIGN(sizeof(size_t))) //반올림된 사이즈 가져오기

#define WSIZE 4  //워드 크기
#define DSIZE 8 //더블 워드
#define CHUNKSIZE (1<<12) //힙 확장을 위한 기본 크기
#define REABUFSIZE (1<<13) //REALLOC에서 사용하기 위한 ADDITIONAL 버퍼 크기
#define MINBLOCK 24 //최소 블럭 크기

#define MAX(x,y) ((x)>(y)?(x):(y)) //max 함수

#define PACK(size, alloc) ((size) | (alloc)) //크기와 할당 비트 통합해서 헤더, 푸터에 저장할 값

#define GET(p) (*(unsigned int *)(p)) //p가 참조하는 워드 읽어 리턴
#define PUT(p,val) (*(unsigned int *)(p) = (val)) //p가 가리키는 워드에 val 저장

#define GET_SIZE(p) (GET(p) & ~0x7) //p의 size 리턴
#define GET_ALLOC(p) (GET(p) & 0x1) //p의 alloc 리턴

#define HDRP(bp) ((char *)(bp) - WSIZE) //헤더를 가리키는 포인터 리턴
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //풋터를 가리키는 포인터 리턴

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //다음 블록의 블록 포인터 리턴
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //이전 블록의 블록 포인터 리턴

#define NEXT_FRLP(ptr)  (*(char **)((char *)(ptr) + DSIZE)) //가용 블럭의 다음 블록 포인터 리턴
#define PREV_FRLP(ptr)  (*(char **)((char * )(ptr))) //가용 블럭의 이전 블록 포인터 리턴

static char *heap_listp = 0; //힙의 위치
static char *free_listp = 0; //리스트의 last in 가르킴
//static char *saved_ptr; //nextfit 용

static void *extend_heap(size_t words); //힙 증가
static void *coalesce(void *bp); //free 되었을때, 앞 뒤 확인하여 통합
static void place(void *bp, size_t asize); //공간 할당
static void *find_fit(size_t asize); //쓸 수 있는 공간이 있나?
static void removeblock(void *bp); //가용 리스트 관리 - 노드 지우기
static void insertblock(void *bp); //가용 리스트 관리 - 노드 넣기


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{   
    if((heap_listp = mem_sbrk(4*WSIZE*3)) == (void *)-1) //초기 힙 확장
    {
        return -1;
    }
    PUT(heap_listp, 0); //Alignment 패딩
    PUT(heap_listp + (1 * WSIZE), PACK(MINBLOCK, 1)); //프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), 0); //프롤로그 prev
    PUT(heap_listp + (3 * WSIZE), 0); //프롤로그 next
    PUT(heap_listp + MINBLOCK, PACK(MINBLOCK, 1)); //프롤로그 풋터
    PUT(heap_listp + MINBLOCK + (1 * WSIZE), PACK(0, 1)); //에필로그 헤더

    free_listp = 0; //list 시작 포인터 초기화

    //saved_ptr = heap_listp; //nextfit 초기화
    
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL) //빈 HEAP을 CHUNKSIZE byte만틈 확장
    {
        return -1; //NULL -> 초기화 실패
    }
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
    
    if(size <= 0) //0의 공간 요청
    {
        return NULL; //allocate 할 것 없음
    }
    asize = ALIGN(size) + 2*WSIZE; //실제로 할당할 공간은 8의 배수 + 헤더 + 풋터
    if(asize<MINBLOCK)
    {
        asize = MINBLOCK; //할당할 사이즈가 최소 블럭보다 작으면 최소 블럭이다
    }
    if((bp = find_fit(asize)) != NULL ) //fit 함수를 구현하여, fit 된 경우
    {   
        //saved_ptr = bp;
        place(bp, asize); //그 공간 주자
        return bp;
    }
    extendsize = MAX(asize,CHUNKSIZE); //fit 할곳이 없는 경우 (한번 확장할때 chunksize만큼은 확장-키울수록 util 낮아짐)
    if((bp = extend_heap(extendsize/WSIZE)) == NULL) //힙 확장
    {
        return NULL; //확장 실패->멀록실패
    }
    place(bp, asize);//확장한 공간에 배치
    return bp;
    /* skelton code
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
    */
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr)); //size 받아옴

    PUT(HDRP(ptr), PACK(size,0)); //헤더 alloc여부 변환
    PUT(FTRP(ptr), PACK(size,0)); //풋터 alloc여부 변환
    coalesce(ptr); //근처와 병합
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr; //이전 malloc
    void *newptr; //realloc 할 곳
    size_t copySize;
    size_t asize;
    
    if(size <= 0) //size가 0 이하 -> free()작동
    {
        mm_free(oldptr);
        return 0;
    }
    else if(oldptr == NULL) //ptr이 NULL -> malloc작동
    {
        return mm_malloc(size);
    }
    else
    {
        copySize = GET_SIZE(HDRP(oldptr)); //기존 사이즈 가져옴
        asize = ALIGN(size) + DSIZE; //실제로 할당할 사이즈(+헤더+풋터)
        if(asize<=copySize) //사이즈가 더 작다면 굳이 malloc을 할 필요가 없다 (공간활용도 떨어지겠지만, 속도 더 빨라짐)
        {
            return ptr;
        }
        newptr = mm_malloc(size+REABUFSIZE); //주어진 size를 만족하는 가용 블럭 탐색(여유롭게 malloc해서 다음 realloc시 가능하면 malloc, memcpy 안하도록)
        if (newptr == NULL) //malloc 실패
        {
            return NULL;
        }
        
        if (size < copySize) //요구하는 블럭의 크기가 옛날 크기보다 작은 경우
        {
            copySize = size; //덮어 씌우지 않게 size까지만 복사해옴
        }
        memcpy(newptr, oldptr, copySize); //복사
        mm_free(oldptr); //이전 malloc free해줌
        return newptr;
    }
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * DSIZE : words * DSIZE; //짝수화
    if(size < MINBLOCK)
    {
        size = MINBLOCK;
    }
    if((long)(bp = mem_sbrk(size)) == -1) //sbrk로 확장, bp는 새로운 heap의 블럭 가르킴
    {
        return NULL; //-1인경우 확장 실패
    }

    PUT(HDRP(bp), PACK(size,0));//헤더 만들어줌
    PUT(FTRP(bp), PACK(size,0));//풋터 만들어줌
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));//에필로그 헤더 만들어줌

    return coalesce(bp); //확장 이후가 연결 가능한지 확인한 후, 앞이 비었을 수 있으니 연결
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); //이전 블록의 풋터의 얼록 여부
    if (PREV_BLKP(bp) == bp) //힙의 첫 블럭에서는  ALLOC 못함
    {
       prev_alloc = 1;
    }
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); //다음 블록의 헤더의 얼록여부
    if (NEXT_BLKP(bp) == bp) //힙의 마지막 블럭에서는  ALLOC 못함
    {
       next_alloc = 1;
    }
    size_t size = GET_SIZE(HDRP(bp)); //사이즈

    if (prev_alloc && next_alloc) //case 1 : prev와 next가 모두 alloc 되어 있는 경우
    {
	    insertblock(bp); //bp는 새로운 가용
        return bp; //연결 x
    }

    else if (prev_alloc && !next_alloc) //case 2 : prev만 alloc 되어 있는 경우
    {
        size = size + GET_SIZE(HDRP(NEXT_BLKP(bp))); //다음 블럭 사이즈 더함
        removeblock(NEXT_BLKP(bp)); //다음 그냥 블럭은 가용 리스트에 있을 것인데, 이는 병합될 예정이므로 제거
        PUT(HDRP(bp),PACK(size,0)); //헤더의 변화
        PUT(FTRP(bp),PACK(size,0)); //푸터의 사이즈 변화(헤더에 size가 변했기 때문에 푸터의 위치 이후 블럭의 끝으로 변함)
        insertblock(bp); //bp는 새로운 가용
    }

    else if (!prev_alloc && next_alloc) //case 3 : next만 alloc 되어 있는 경우
    {
        size = size + GET_SIZE(HDRP(PREV_BLKP(bp))); //이전 블록 사이즈 더함
        removeblock(PREV_BLKP(bp)); //이전 그냥 블럭은 가용 리스트에 있는데, 현 블럭을 지우고 이전것을 확장시키는 것 보단, 일괄적으로 이걸 만들어줄려고 함(LIFO)
        PUT(FTRP(bp),PACK(size,0)); //푸터에 사이즈 저장
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0)); //이전 블록의 헤더에 사이즈 저장
        bp = PREV_BLKP(bp); //bp 위치를 전으로 옮김
        insertblock(bp); //bp는 새로운 가용
    }
    else //case 4 : prev와 next 모두 alloc 안된 경우
    {
        size = size + GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp))); //앞뒤 사이즈 더함
        removeblock(PREV_BLKP(bp));//앞 블럭은 통합될 예정
        removeblock(NEXT_BLKP(bp));//뒤 블럭은 통합될 예정
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0)); //이전 블록 헤더에 사이즈 저장 -> 풋터 위치 결정됨
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0)); //bp의 위치는 아직 중간이므로, 다음 블록의 포인터를 받아와 풋터를 결정해 사이즈 저장
        bp = PREV_BLKP(bp); //bp 위치 이전 블럭으로 옮김
        insertblock(bp); //bp는 새로운 가용
    }
    /*
    if((saved_ptr > (char *)bp) && (saved_ptr < NEXT_BLKP(bp))) //통합으로 인해 헤더를 가르키지 않는 경우
    {
        saved_ptr = bp; //헤더로 옮겨줌
    }
    */
    return bp;
}
static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp)); //블럭의 공간 받아옴
    int left = csize - asize; //request와 블럭의 공간의 차이 저장
    if(left >= MINBLOCK) //차이가 24byte보다 클 경우, 한 블럭 이상 만들 수 있음(내부 단편화 줄이기)
    {
        PUT(HDRP(bp), PACK(asize,1)); //헤더 사이즈 감소
        PUT(FTRP(bp), PACK(asize,1)); //풋터 사이즈 감소
        removeblock(bp); //bp는 더이상 가용이 아님
        bp = NEXT_BLKP(bp); //bp의 헤더를 바꾸었으므로 다음 블럭이 지목 됨
        PUT(HDRP(bp), PACK(left,0)); //다음 블럭 헤드 분리
        PUT(FTRP(bp), PACK(left,0)); //다음 블럭 풋터 분리
        coalesce(bp); //남은 블럭을 가용 리스트에 넣고, case 3일 수 있으므로 병합
    }
    else //작을 경우, 아무리 쪼개도 최소 블럭의 크기보다 작음
    {
        PUT(HDRP(bp), PACK(csize,1)); //헤더 변경
        PUT(FTRP(bp), PACK(csize,1)); //풋터 변경
        removeblock(bp); //bp는 더이상 가용이 아님
    }
    
}
static void *find_fit(size_t asize){

    void *bp;
    bp = free_listp; //가용 리스트 탐색의 시작
    while(bp != NULL) //가용 리스트의 끝까지 루프
    {
        if(asize<=GET_SIZE(HDRP(bp))) //alloc이 0이며, size가 asize보다 큰 경우 -> explict로 넘어오며 alloc 0인건 보장됨, first fit
        {
            return bp; //그곳에 fit
        }
        bp = NEXT_FRLP(bp); //다음 블럭
    }
    return NULL;

   /*
    char *bp = saved_ptr; //이전 bp 부터 탐색 시작
    while(GET_SIZE(HDRP(saved_ptr))>0) //save된 곳 부터 끝까지 
    {   
        if(!GET_ALLOC(HDRP(saved_ptr))&&(asize<=GET_SIZE(HDRP(saved_ptr)))) //alloc이 0이며, size가 asize보다 큰 경우
        {
            return saved_ptr; //그곳에 fit
        }
        saved_ptr = NEXT_BLKP(saved_ptr); //다음 블럭
    }
    saved_ptr = heap_listp; //처음부터 탐색 시작
    while(saved_ptr < bp) //처음부터 save 된 곳 전까지
    {   
        if(!GET_ALLOC(HDRP(saved_ptr))&&(asize<=GET_SIZE(HDRP(saved_ptr)))) //alloc이 0이며, size가 asize보다 큰 경우
        {
            return saved_ptr; //그곳에 fit
        }
        saved_ptr = NEXT_BLKP(saved_ptr); //다음 블럭
    }
    return NULL; //no fit
    */
    
}

static void removeblock(void *bp)
{
    if ((PREV_FRLP(bp) == NULL) && (NEXT_FRLP(bp) == NULL)) //bp의 이전, 이후 모두 NULL -> bp만 리스트에 있음
    { 
        free_listp = 0; //리스트 비우기
    }
    else if ((PREV_FRLP(bp) != NULL) && (NEXT_FRLP(bp) == NULL))  //bp의 이전이 있고, 이후가 널 -> 리스트의 첫번째
    {
        NEXT_FRLP(PREV_FRLP(bp)) = NULL; //이전의 다음을 NULL로 잡아줌
    }
    else if ((PREV_FRLP(bp) == NULL) && (NEXT_FRLP(bp) != NULL)) //bp 이전이 널, 이후가 있음 -> 리스트의 마지막
    {   
        PREV_FRLP(NEXT_FRLP(bp)) = NULL; //이후의 이전이 널
        free_listp = NEXT_FRLP(bp); //마지막 이후가 첫 요소
    }
    else if ((PREV_FRLP(bp) != NULL) && (NEXT_FRLP(bp) != NULL)) //중간에 있는 케이스
    {
        PREV_FRLP(NEXT_FRLP(bp)) = PREV_FRLP(bp); //이후의 이전은 지금의 이전
        NEXT_FRLP(PREV_FRLP(bp)) = NEXT_FRLP(bp); //이전의 이후는 지금의 이후
    }
}

static void insertblock(void *bp)
{
    if(free_listp != NULL) //리스트가 구성되어 있을때
    {
        NEXT_FRLP(bp) = free_listp; //기존 처음은 자신의 다음
        PREV_FRLP(bp) = NULL; //자신은 이전 없음
        PREV_FRLP(NEXT_FRLP(bp)) = bp; //다음의 이전은 자신
        free_listp = bp; //자신은 처음
    }
    else //리스트가 구성되어 있지 않음 -> 자신이 리스트의 첫 요소일때
    {
        free_listp = bp; //자신은 처음
        NEXT_FRLP(bp) = NULL; //다음은 없음
        PREV_FRLP(bp) = NULL; //이전도 없음
    }
}
