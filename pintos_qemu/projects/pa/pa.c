#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "projects/pa/pa.h"

void run_patest(char **argv)
{   
    /// TODO: make your own test
    
    // printf("base\n");
    // palloc_get_status(0);
    
    // printf("allocate 64 page\n");
    // void *a = palloc_get_multiple(0, 64);
    // palloc_get_status(0);
    
    // printf("allocate 63 page\n");
    // void *b = palloc_get_multiple(0, 63);
    // palloc_get_status(0);
    
    // printf("allocate 9 page\n");
    // void *c = palloc_get_multiple(0,9);
    // palloc_get_status(0);

    // printf("free 64 page\n");
    // palloc_free_multiple(a, 64);
    // palloc_get_status(0);
    
    // printf("free 9 page\n");
    // palloc_free_multiple(c, 9);
    // palloc_get_status(0);

    // printf("free 63 page\n");
    // palloc_free_multiple(b,63);
    // palloc_get_status(0);
    
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
}
