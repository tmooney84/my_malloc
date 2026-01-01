#ifndef BITWISE_HELPERS_H 
#define BITWISE_HELPERS_H

#include <stdint.h>
#include <stdio.h>

#define MIN_TBL_PWR 5
#define MAX_TBL_PWR 11

int32_t next_pow2(uint32_t v);
int32_t chunk_size_index(uint32_t v);

#endif