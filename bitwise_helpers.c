#include <stdint.h>
#include <stdio.h>

#define MIN_TBL_PWR 5
#define MAX_TBL_PWR 11

int32_t next_pow2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

int32_t chunk_size_index(uint32_t v) {
    int32_t p = next_pow2(v);  // e.g. 457 → 512
    int32_t idx = 31 - __builtin_clz(p) - MIN_TBL_PWR; // -5 because first bucket is 32
    if(idx > MAX_TBL_PWR - MIN_TBL_PWR){
       perror("Error... allocation size larger than largest bucket size\n");
    }
    return idx;
}

int main(void){
    int v = 33;
    int z = next_pow2(v);
    printf("Bucket is: %d\n", z);
    printf("2 to the power of n: %d\n", chunk_size_index(v));
    printf("2 to the power of n: %d\n", chunk_size_index(z));
    return 0;
}