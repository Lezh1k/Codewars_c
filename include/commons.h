#ifndef COMMONS_H
#define COMMONS_H

#include <stdint.h>

int32_t trailing_zeros_32_bi(uint32_t v);
int32_t trailing_zeros_64(uint64_t v);

int32_t leading_zeros_32_bi(uint32_t v);

int32_t leading_zeros_64(uint64_t v);
int32_t leading_zeros_32(uint32_t v);

#endif // COMMONS_H
