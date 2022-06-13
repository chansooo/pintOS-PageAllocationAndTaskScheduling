#include "threads/palloc.h"
#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "threads/loader.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.  See malloc.h for an allocator that
   hands out smaller chunks.

   System memory is divided into two "pools" called the kernel
   and user pools.  The user pool is for user (virtual) memory
   pages, the kernel pool for everything else.  The idea here is
   that the kernel needs to have memory for its own operations
   even if user processes are swapping like mad.

   By default, half of system RAM is given to the kernel pool and
   half to the user pool.  That should be huge overkill for the
   kernel pool, but that's just fine for demonstration purposes. */

/* A memory pool. */
struct pool
  {
    struct lock lock;                   /* Mutual exclusion. */
    struct bitmap *used_map;            /* Bitmap of free pages. */
    uint8_t *base;                      /* Base of pool. */
  };

struct block 
{
  size_t idx;
  struct list_elem elem;
}
//찬수가 한 거~
//list 8개로 만들자!
struct list[8] page_list;

for(int i=0; i<8; i++){
  list_init(page_list[i]);
}

list_push_back(list[8], );


/* Two pools: one for kernel data, one for user pages. */
static struct pool kernel_pool, user_pool;

static void init_pool (struct pool *, void *base, size_t page_cnt,
                       const char *name);
static bool page_from_pool (const struct pool *, void *page);

/* Initializes the page allocator.  At most USER_PAGE_LIMIT
   pages are put into the user pool. */
//페이지 할당자 초기화 함수. 커널과 유저 페이지 나눠서 각자 할당
void
palloc_init (size_t user_page_limit)
{
  /* Free memory starts at 1 MB and runs to the end of RAM. */
  uint8_t *free_start = ptov (1024 * 1024);
  uint8_t *free_end = ptov (init_ram_pages * PGSIZE);
  size_t free_pages = (free_end - free_start) / PGSIZE;
  size_t user_pages = free_pages / 2;
  size_t kernel_pages;
  if (user_pages > user_page_limit)
    user_pages = user_page_limit;
  kernel_pages = free_pages - user_pages;

  /* Give half of memory to kernel, half to user. */
  init_pool (&kernel_pool, free_start, kernel_pages, "kernel pool");
  init_pool (&user_pool, free_start + kernel_pages * PGSIZE,
             user_pages, "user pool");
}

/* Obtains and returns a group of PAGE_CNT contiguous free pages.
   If PAL_USER is set, the pages are obtained from the user pool,
   otherwise from the kernel pool.  If PAL_ZERO is set in FLAGS,
   then the pages are filled with zeros.  If too few pages are
   available, returns a null pointer, unless PAL_ASSERT is set in
   FLAGS, in which case the kernel panics. */
//2^n에서 buddy의 공간을 최적으로 사용하는 크기 return
size_t
find_power_of_two(size_t b){
  for(size_t i=0;;i++){
    //2의 i승이 temp보다 크다면 해당 i 반환
    if(b <= pow(2,i)){ 
size_t next_power_of_2(size_t size)
{
    /* depend on the fact that size < 2^32 */
  int i =1;
  while(true){
    if(size < i){
      return i;
    }
    i = i * 2;
  }
  return i
}

size_t pow(size_t num, size_t n){
  for(int i =0; i < n; i++){
    num = num * 2;
  }
  return num;
}

   //연속된 페이지 할당
void *
palloc_get_multiple (enum palloc_flags flags, size_t page_cnt)
{
  struct pool *pool = flags & PAL_USER ? &user_pool : &kernel_pool;
  void *pages;
  size_t page_idx;

  if (page_cnt == 0)
    return NULL;

  lock_acquire (&pool->lock);
  //여기를 바꾸자
  page_idx = bitmap_scan_and_flip (pool->used_map, 0, page_cnt, false);
  for(int i=0; i<9;i++){ //2^8 = 256까지만 고려?
    
    //쓸 수 있는 블록 있는데 
    if(!list_empty(block_list[i])){ //쓸 수 있는 block이 있으면
      if(find_power_of_two(page_cnt) <= pow(2,i)){ //그 block에 page가 들어갈 수 있으면      
        b = list_entry(list_pop_front(&block_list[i], struct block, elem));
        if(i == 1 || find_power_of_two(page_cnt) > pow(2,i-1)){ //딱 맞으면 그대로 할당

        } else{ // 들어가긴 하는데 너무 과분하면
          struct block *b2; //여기에 선언하면 사라지나?
          b2->idx = b->idx + pow(2,(i-1)); // +1해줘야하나?
          list_push_back(&block_list[i], &b2->elem);
        }
        page_idx = b->idx;
      }
    }
  }
  lock_release (&pool->lock);

  if (page_idx != BITMAP_ERROR)
    pages = pool->base + page_idx; //pool + block의 인덱스로 넘겨주기
  else
    pages = NULL;

  if (pages != NULL) 
    {
      if (flags & PAL_ZERO)
        memset (pages, 0, PGSIZE * page_cnt);
    }
  else 
    {
      if (flags & PAL_ASSERT)
        PANIC ("palloc_get: out of pages");
    }

  return pages;
}

/* Obtains a single free page and returns its kernel virtual
   address.
   If PAL_USER is set, the page is obtained from the user pool,
   otherwise from the kernel pool.  If PAL_ZERO is set in FLAGS,
   then the page is filled with zeros.  If no pages are
   available, returns a null pointer, unless PAL_ASSERT is set in
   FLAGS, in which case the kernel panics. */
   //단일 페이지 할당
void *
palloc_get_page (enum palloc_flags flags) 
{
  return palloc_get_multiple (flags, 1);
}

/* Frees the PAGE_CNT pages starting at PAGES. */
void
palloc_free_multiple (void *pages, size_t page_cnt) 
{
  struct pool *pool;
  size_t page_idx;

  ASSERT (pg_ofs (pages) == 0);
  if (pages == NULL || page_cnt == 0)
    return;

  if (page_from_pool (&kernel_pool, pages))
    pool = &kernel_pool;
  else if (page_from_pool (&user_pool, pages))
    pool = &user_pool;
  else
    NOT_REACHED ();

  page_idx = pg_no (pages) - pg_no (pool->base);

#ifndef NDEBUG
  memset (pages, 0xcc, PGSIZE * page_cnt);
#endif

  ASSERT (bitmap_all (pool->used_map, page_idx, page_cnt));
  bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
      //합칠 수 있는지 확인
      //꺼낸 게 buddy라면 합칠 게 사용 중이니까 그대로 두기
     
      size_t size = list_size(&block_list[i]);
      struct block *c;


      
      //b가 buddy 중 앞에 있는 아이라면
      if((b->idx / pow(2,i)) % 2 == 0 ){
        while(size){ //blocklist에 버디 있는지 확인
          c = list_entry(list_pop_front(&block_list[i], struct block, elem)); 
          if(c->idx == (page_idx + pow(2,i))){
            //버디 있으면 합치기
            c->idx = page_idx;
            list_push_back(&block_list[i+1], &c)
            break;
          }
          list_push_back(&block_list[i], &c);
          size--;
        }
      } else{
        //b가 buddy 중 뒤에 있는 아이라면
        while(size){ //blocklist에 버디 있는지 확인
          c = list_entry(list_pop_front(&block_list[i], struct block, elem)); 
          if(c->idx == (page_idx - pow(2,i))){
            //버디 있으면 합치기
            c->idx = page_idx - pow(2,i);
            list_push_back(&block_list[i+1], &c)
            break;
          }
          list_push_back(&block_list[i], &c);
          size--;
        }
      }
    }
  }
}

/* Frees the page at PAGE. */
void
palloc_free_page (void *page) 
{
  palloc_free_multiple (page, 1);
}

/* Initializes pool P as starting at START and ending at END,
   naming it NAME for debugging purposes. */
static void
init_pool (struct pool *p, void *base, size_t page_cnt, const char *name) 
{
  /* We'll put the pool's used_map at its base.
     Calculate the space needed for the bitmap
     and subtract it from the pool's size. */
  size_t bm_pages = DIV_ROUND_UP (bitmap_buf_size (page_cnt), PGSIZE);
  if (bm_pages > page_cnt)
    PANIC ("Not enough memory in %s for bitmap.", name);
  page_cnt -= bm_pages;

  printf ("%zu pages available in %s.\n", page_cnt, name);

  /* Initialize the pool. */
  lock_init (&p->lock);
  p->used_map = bitmap_create_in_buf (page_cnt, base, bm_pages * PGSIZE);
  p->base = base + bm_pages * PGSIZE;
}

/* Returns true if PAGE was allocated from POOL,
   false otherwise. */
static bool
page_from_pool (const struct pool *pool, void *page) 
{
  size_t page_no = pg_no (page);
  size_t start_page = pg_no (pool->base);
  size_t end_page = start_page + bitmap_size (pool->used_map);

  return page_no >= start_page && page_no < end_page;
}

/* Obtains a status of the page pool */
void
palloc_get_status (enum palloc_flags flags)
{
  //IMPLEMENT THIS
  //PAGE STATUS 0 if FREE, 1 if USED
  //32 PAGE STATUS PER LINE
}

//-----------------------------------------------
typedef unsigned long elem_type;

struct bitmap
  {
    size_t bit_cnt;     /* Number of bits. */
    elem_type *bits;    /* Elements that represent bits. */
  };

//2^n에서 buddy의 공간을 최적으로 사용하는 크기 return
size_t
find_power_of_two(struct bitmap *b){
  size_t temp = bitmap_size(b);
  for(size_t i=0;;i++){
    //2의 i승이 temp보다 크다면 해당 i 반환
    if(temp <= pow(2,i)){
      return i;
    }
  }
}


// size_t
// bitmap_scan_and_flip (struct bitmap *b, size_t start, size_t cnt, bool value)
// {
//   size_t idx = bitmap_scan (b, start, cnt, value);
//   if (idx != BITMAP_ERROR) 
//     bitmap_set_multiple (b, idx, cnt, !value);
//   return idx;
// }

/* Finds and returns the starting index of the first group of CNT
   consecutive bits in B at or after START that are all set to
   VALUE.
   If there is no such group, returns BITMAP_ERROR. */
   //들어갈 곳 찾는 주요 로직
size_t
bitmap_scan (const struct bitmap *b, size_t start, size_t cnt, bool value) 
{
  ASSERT (b != NULL);
  ASSERT (start <= b->bit_cnt);

  if (cnt <= b->bit_cnt) 
    {
      size_t last = b->bit_cnt - cnt;
      size_t i;
      for (i = start; i <= last; i++)
      //여기서 가능한 곳이 나오면 바로 return해버리는 것임
      //그러면 우리는! 바로 return 때리는 것이 아니라 빈 공간의 크기를 측정해서 2^n에 가장 가까운 index를 return해주면 된다
      //그러면 우리는! i를 찾았을 때 크기를 확인하고 temp에 저장한 다음  
      //이게 아니야
      //쪼개진 아이들을 생각해야해
      //할당해줄 때 
        if (!bitmap_contains (b, i, cnt, !value)) 
          return i; 
    }
  return BITMAP_ERROR;
}

/* Returns true if any bits in B between START and START + CNT,
   exclusive, are set to VALUE, and false otherwise. */
   //start~start+cnt까지 가능한 지점 있는지 확인
bool
bitmap_contains (const struct bitmap *b, size_t start, size_t cnt, bool value) 
{
  size_t i;
  
  ASSERT (b != NULL);
  ASSERT (start <= b->bit_cnt);
  ASSERT (start + cnt <= b->bit_cnt);

  for (i = 0; i < cnt; i++)
    if (bitmap_test (b, start + i) == value)
      return true;
  return false;
}