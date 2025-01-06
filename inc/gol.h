#ifndef GOL_H
#define GOL_H

#include <stdint.h>

int **get_generation(const int32_t **cells, int32_t generations, int32_t *ptr_r,
                     int32_t *ptr_c);

#endif // GOL_H
