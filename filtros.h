#ifndef FILTROS_H
#define FILTROS_H

#include <stdint.h>

int compare_uint8(const void* a, const void* b);  // <--- adicione esta linha

uint8_t** median_filter(uint8_t** gray, int width, int height, int mask_size);
uint8_t** laplacian_filter(uint8_t** gray, int width, int height, int mask_size);

#endif

