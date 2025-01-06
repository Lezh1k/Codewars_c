#ifndef COMMONS_H
#define COMMONS_H

#include <stdbool.h>
#include <stdint.h>

int32_t trailing_zeros_32_bi(uint32_t v);
int32_t trailing_zeros_64(uint64_t v);

int32_t leading_zeros_32_bi(uint32_t v);

int32_t leading_zeros_64(uint64_t v);
int32_t leading_zeros_32(uint32_t v);

uint32_t nearest_power_of_2_u32(uint32_t v);
uint64_t nearest_power_of_2_u64(uint64_t v);
uint32_t log_of_power_2_u32(uint32_t v);
uint64_t log_of_power_2_u64(uint64_t v);

void array_shift(uint8_t *arr, int N, bool to_left);

///
/// \brief bit_comb - generates all bit combinations
/// \param pool - all bits count
/// \param need - how many bits in combination should be set
/// \param chosen - current combination
/// \param at - bit number to test
/// \param cb - callback for chosen combination
///
void bit_comb(uint32_t pool, uint32_t need, uint32_t chosen, uint32_t at,
              void (*cb)(uint32_t));
//////////////////////////////////////////////////////////////////////////

#endif // COMMONS_H
