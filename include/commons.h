#ifndef COMMONS_H
#define COMMONS_H

#include <stdint.h>
#include <stdbool.h>

int32_t trailing_zeros_32_bi(uint32_t v);
int32_t trailing_zeros_64(uint64_t v);

int32_t leading_zeros_32_bi(uint32_t v);

int32_t leading_zeros_64(uint64_t v);
int32_t leading_zeros_32(uint32_t v);

uint32_t nearest_power_of_2(uint32_t v);
uint32_t log_of_power_2(uint32_t v);

void array_shift(uint8_t *arr, int N, bool to_left);
//////////////////////////////////////////////////////////////////////////

#endif // COMMONS_H
