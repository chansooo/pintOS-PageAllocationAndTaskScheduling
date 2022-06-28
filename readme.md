# Page Allocation and Task Scheduling

# Page Allocation

best-fit, first-fit, next-fit, worst-fir, buddy system등 다양한 방법 중에서  pintOS의 경우 디폴트로 First-Fit의 방식으로 Page Allocation이 구현되어있다.

First-Fit으로 구현된 Page Allocation을 Buddy System 방식으로 새로 구현을 하였다.

`threads → palloc.c` 에서 코드를 확인할 수 있다.

## Buddy System

구현하려고 하는 Buddy System은 같은 크기의 한 쌍의 block으로 쪼개면서 할당을 하는 기법이다.

1M의 block에서 100k 할당을 요청한다면 

1M → 512k, 512k → 256k, 256k, 512k → 128k, 128k, 256k, 512k

다음과 같은 step으로 128k를 할당해주게 된다.

만약 할당을 해제할 때 buddy에 해당하는 메모리 공간이 idle하다면 한 쌍을 합쳐서 2배 큰 공간으로 만들어 주게 된다.

## 구현

```c
void *
palloc_get_multiple (enum palloc_flags flags, size_t page_cnt)
{
  struct pool *pool = flags & PAL_USER ? &user_pool : &kernel_pool;
  void *pages;
  size_t page_idx;

  if (page_cnt == 0)
    return NULL;

  lock_acquire (&pool->lock);
  page_idx = get_page_idx (pool->used_map, 0, page_cnt, false);

  lock_release (&pool->lock);

  if (page_idx != BITMAP_ERROR)
    pages = pool->base + PGSIZE * page_idx;

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
```

`page_cnt` 에 해당하는 만큼 할당을 해주는 함수이다.

`get_page_idx` 메소드를 통해서 할당을 할 index를 반환받아서 해당하는 index에서 page_cnt만큼 할당을 한다.

```c
size_t get_page_idx(struct bitmap *b, size_t start, size_t cnt, bool value){
    int size = 1;
    int idx = 0;
    while(cnt > size){  //맞는 size찾도록.
      size = size * 2;
    }
    while(idx <= bitmap_size(b)){
      if(!bitmap_contains(b, idx, size, !value)){
        bitmap_set_multiple(b,idx,size,!value);
        return idx;
      }
      
      idx += size;
      
    }
    
    return BITMAP_ERROR;
}
```

`get_page_idx` 에서는 할당이 가능한 index를 반환해준다.

요청받은 count만큼이 할당될 수 있는 2^n에 가까운 size를 찾아서  해당 크기의 메모리 공간을 사용한다.

만약 해당 size가 없다면 그것보다 2배 큰 공간을 절반으로 쪼개서 사용을 한다.

```c
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
  
  //buddy에 맞는 사이즈에 넣어주기
  int size = 1;
  while(size < page_cnt){
    size = size * 2;
  }
  page_cnt = size;

#ifndef NDEBUG
  memset (pages, 0xcc, PGSIZE * page_cnt);
#endif

  ASSERT (bitmap_all (pool->used_map, page_idx, page_cnt));
  bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
}
```

할당을 해제하는 메소드이다.

할당을 해줄 때 요청한 size만큼 딱 맞게 할당을 해주는 것이 아닌 2^n만큼 할당을 해주므로 해당 사이즈에 맞게 다시 해제해준다.

## Test

```c
printf("base\n");
    palloc_get_status(0);
    
    printf("allocate 64 page\n");
    void *a = palloc_get_multiple(0, 64);
    palloc_get_status(0);
    
    printf("allocate 6 page\n");
    void *b = palloc_get_multiple(0, 6);
    palloc_get_status(0);

    printf("free 64 page\n");
    palloc_free_multiple(a,64);
    palloc_get_status(0);

    printf("allocate 28 page\n");
    void *c = palloc_get_multiple(0, 28);
    palloc_get_status(0);
    

    printf("free 28 page\n");
    palloc_free_multiple(c,28);
    palloc_get_status(0);

    printf("free 6 page\n");
    palloc_free_multiple(b,6);
    palloc_get_status(0);

    while (1) {
        timer_msleep(1000);
    }
```

다음과 같이 test를 진행했을 때

![Untitled 1](https://user-images.githubusercontent.com/89574881/176100351-8738f75e-c186-4fe5-b72f-483247341cd3.png)

아무것도 할당하지 않은 상태.

초기화할 때 할당된 처음 3 bit를 제외하고 모두 0으로 잘 나오는 모습을 볼 수 있다.

![Untitled 1](https://user-images.githubusercontent.com/89574881/176099971-b48c85fe-a5a4-4008-bf26-870ed20cc3de.png)

64을 할당했을 때.
큰 block을 쪼개지 않아도 되면 쪼개지 않고 64비트를 적재할 수 있는 최적의 자리에 들어간 것을 볼 수 있다.

![Untitled 2](https://user-images.githubusercontent.com/89574881/176100208-add549ee-5c2f-499c-9b95-165c0deb7528.png)

6을 할당했을 때.
6을 할당하면 8만큼 할당이 되어야하는데 큰 block을 쪼개지 않아도 되면 쪼개지 않고 8을 적재할 수 있는 최적의 자리에 들어간 것을 볼 수 있다.

![Untitled 3](https://user-images.githubusercontent.com/89574881/176100232-7855c2d9-e49b-4f65-a0d3-249cc0239ae6.png)

64를 할당 해제하였을 때.
정상적으로 64bit가 해제되었음을 볼 수 있다.

![Untitled 4](https://user-images.githubusercontent.com/89574881/176100257-7b763e57-77aa-4f83-b450-62f8538395d4.png)

28을 할당했을 때.
정상적으로 32비트가 바뀐 것을 볼 수 있다.

![Untitled 5](https://user-images.githubusercontent.com/89574881/176100291-da9fb90a-07ad-456d-ab45-6110baad5802.png)

28비트를 해제했을 때

![Untitled 6](https://user-images.githubusercontent.com/89574881/176100313-f7293309-8b1c-4bd0-bcef-efc20c8f3e52.png)

마지막으로 남은 6비트를 해제하면 8비트가 0으로 바뀌고 다시 모든 bitmap이 0으로 바뀐 것을 볼 수 있다.

# Task Scheduling

pintos는 기본적으로 round-robin기법으로 task scheduling되어있다.

round-robin기법으로 구현된 것을 multi-level feedback queue 기법으로 바꿔서 구현을 해볼 것이다.

`threads → thread.c` 에서 코드를 확인할 수 있다.

## 구현

```c
static struct list ready_list0;
static struct list ready_list1;
static struct list ready_list2;
static struct list ready_list3;
int cur_queue;

#define TIME_SLICE_0 4
#define TIME_SLICE_1 5
#define TIME_SLICE_2 6
#define TIME_SLICE_3 7
```

큐는 4개가 존재하도록 구현하였고, 각각의 큐는 다른 priority를 가지며, 다른 timeslice를 가진다.

```c
if(cur_queue == 0){ //thread_ticks가 timeslice보다 크면 intr_yield_on_return
    if(++thread_ticks >= TIME_SLICE_0){
      intr_yield_on_return();
    }
  }else if(cur_queue == 1){
    if(++thread_ticks >= TIME_SLICE_1){
      intr_yield_on_return();
    }
  }else if(cur_queue == 2){
    if(++thread_ticks >= TIME_SLICE_2){
      intr_yield_on_return();
    }
  }else if(cur_queue == 3){
    if(++thread_ticks >= TIME_SLICE_3){
      intr_yield_on_return();
    }
  }
```

한 틱지 지날 때마다 실행되는 thread_tick 메소드에서 현재 큐의 time slice를 만족하게 되면 스케줄링이 진행되도록 하였다.

```c
if(cur_queue == 0){
    if(++thread_ticks >= TIME_SLICE_0){ //timeslice 충족
    //TODO: 한 단계 낮은 큐 뿐만 아니라 낮은 모든 큐에 대해서 aging검사
    //readylist1에서 0으로 올라올 수 있는 스레드 올려주기
      //readylist1->readylist0 체크
      temp_elem = list_begin(&ready_list1);
      while(temp_elem != list_end(&ready_list1)||list_empty(&ready_list1)){
        temp_thread = list_entry(temp_elem, struct thread, elem);
        temp_thread->age++;
        if(t->age >= 20){
          list_pop_front(&ready_list1); //queue1에 있는 거 꺼내고
          list_push_back(&ready_list0, &temp_thread->elem); //queue0에 넣어줌
          temp_thread->age = 0;
          temp_thread->priority = 0;
          temp_elem = list_begin(&ready_list1);
        }else{
          temp_elem = list_next(temp_elem);
        }
      }
      //readylist2->readylist1 체크
      temp_elem = list_begin(&ready_list2);
      while(temp_elem != list_end(&ready_list2)||list_empty(&ready_list2)){
        temp_thread = list_entry(temp_elem, struct thread, elem);
        temp_thread->age++;
        if(t->age >= 20){
          list_pop_front(&ready_list2); //queue2에 있는 거 꺼내고
          list_push_back(&ready_list1, &temp_thread->elem); //queue1에 넣어줌
          temp_thread->age = 0;
          temp_thread->priority = 1;
          temp_elem = list_begin(&ready_list2);
        }else{
          temp_elem = list_next(temp_elem);
        }
      }
      //readylist3->readylist2 체크
      temp_elem = list_begin(&ready_list3);
      while(temp_elem != list_end(&ready_list3)||list_empty(&ready_list3)){
        temp_thread = list_entry(temp_elem, struct thread, elem);
        temp_thread->age++;
        if(t->age >= 20){
          list_pop_front(&ready_list3); //queue1에 있는 거 꺼내고
          list_push_back(&ready_list2, &temp_thread->elem); //queue0에 넣어줌
          temp_thread->age = 0;
          temp_thread->priority = 2;
          temp_elem = list_begin(&ready_list3);
        }else{
          temp_elem = list_next(temp_elem);
        }
      }
    }
  }else if(cur_queue == 1){
    if(++thread_ticks >= TIME_SLICE_1){
            //readylist2->readylist1 체크
      temp_elem = list_begin(&ready_list2);
      while(temp_elem != list_end(&ready_list2)||list_empty(&ready_list2)){
        temp_thread = list_entry(temp_elem, struct thread, elem);
        temp_thread->age++;
        if(t->age >= 20){
          list_pop_front(&ready_list2); //queue2에 있는 거 꺼내고
          list_push_back(&ready_list1, &temp_thread->elem); //queue1에 넣어줌
          temp_thread->age = 0;
          temp_thread->priority = 0;
          temp_elem = list_begin(&ready_list2);
        }else{
          temp_elem = list_next(temp_elem);
        }
      }
      //readylist3->readylist2 체크
      temp_elem = list_begin(&ready_list3);
      while(temp_elem != list_end(&ready_list3)||list_empty(&ready_list3)){
        temp_thread = list_entry(temp_elem, struct thread, elem);
        temp_thread->age++;
        if(t->age >= 20){
          list_pop_front(&ready_list3); //queue1에 있는 거 꺼내고
          list_push_back(&ready_list2, &temp_thread->elem); //queue0에 넣어줌
          temp_thread->age = 0;
          temp_thread->priority = 2;
          temp_elem = list_begin(&ready_list3);
        }else{
          temp_elem = list_next(temp_elem);
        }
      }
    }
  }else if(cur_queue == 2){
    if(++thread_ticks >= TIME_SLICE_2){
      //readylist3->readylist2 체크
      temp_elem = list_begin(&ready_list3);
      while(temp_elem != list_end(&ready_list3)||list_empty(&ready_list3)){
        temp_thread = list_entry(temp_elem, struct thread, elem);
        temp_thread->age++;
        if(t->age >= 20){
          list_pop_front(&ready_list3); //queue1에 있는 거 꺼내고
          list_push_back(&ready_list2, &temp_thread->elem); //queue0에 넣어줌
          temp_thread->age = 0;
          temp_thread->priority = 2;
          temp_elem = list_begin(&ready_list3);
        }else{
          temp_elem = list_next(temp_elem);
        }
      }
    }
  }
```

또한 낮은 priority의 큐에 있는 thread들이 stavation에 빠지지 않도록 `aging` 기법을 통해 일정 tick이 지난 후에는 다음 priority가 높은 큐로 삽입을 해주었다.
