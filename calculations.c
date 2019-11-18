//
// Created by pstadler on 18.11.19.
//
#include "calculations.h"
#include <stdlib.h>

static int compare_func( const void* a, const void* b);

/* https://stackoverflow.com/a/3893967/1794026 */
static int
compare_func( const void* a, const void* b) {
    /* Compare function for qsort (used for median sorting) */
    int int_a = *((int*) a);
    int int_b = *((int*) b);
    return (int_a > int_b) - (int_a < int_b);
}

uint32_t
median_u32(uint32_t* buf, uint32_t blen) {
    /* Returns median of given (u16) buffer */
    qsort(buf, blen, sizeof(uint32_t), compare_func);
    return (blen % 2 == 0) ?  ((buf[(blen-1)/2] + buf[blen/2])/2) : buf[blen/2];
}
