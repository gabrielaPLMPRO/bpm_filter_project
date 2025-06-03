#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "filtros.h"

int compare_uint8(const void* a, const void* b) {
    return (*(uint8_t*)a - *(uint8_t*)b);
}

uint8_t** median_filter(uint8_t** gray, int width, int height, int mask_size) {
    int offset = mask_size / 2;
    uint8_t** result = malloc(height * sizeof(uint8_t*));
    for (int i = 0; i < height; i++)
        result[i] = malloc(width * sizeof(uint8_t));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            int count = 0;
            int max_vals = mask_size * mask_size;
            uint8_t* neighbors = malloc(max_vals * sizeof(uint8_t));

            for (int mi = -offset; mi <= offset; mi++) {
                for (int mj = -offset; mj <= offset; mj++) {
                    int ni = i + mi;
                    int nj = j + mj;

                    if (ni < 0) ni = 0;
                    if (nj < 0) nj = 0;
                    if (ni >= height) ni = height - 1;
                    if (nj >= width) nj = width - 1;

                    neighbors[count++] = gray[ni][nj];
                }
            }

            qsort(neighbors, count, sizeof(uint8_t), compare_uint8);
            result[i][j] = neighbors[count / 2];
            free(neighbors);
        }
    }

    return result;
}

uint8_t** laplacian_filter(uint8_t** gray, int width, int height, int mask_size) {
    if (mask_size != 3) {
        fprintf(stderr, "Por enquanto, apenas máscara 3x3 é suportada para o Laplaciano.\n");
        return NULL;
    }

    int kernel[3][3] = {
        { -1, -1, -1 },
        { -1,  8, -1 },
        { -1, -1, -1 }
    };

    uint8_t** result = malloc(height * sizeof(uint8_t*));
    for (int i = 0; i < height; i++)
        result[i] = calloc(width, sizeof(uint8_t));

    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int sum = 0;

            for (int ki = -1; ki <= 1; ki++) {
                for (int kj = -1; kj <= 1; kj++) {
                    sum += gray[i + ki][j + kj] * kernel[ki + 1][kj + 1];
                }
            }

            int val = abs(sum);
            if (val > 255) val = 255;
            result[i][j] = (uint8_t)val;
        }
    }

    return result;
}
