#ifndef BMP_UTILS_H
#define BMP_UTILS_H

#include <stdint.h>

typedef struct {
    uint8_t b, g, r;
} RGB;

typedef struct {
    int width;
    int height;
    RGB** pixels;
} BMPImage;

BMPImage* read_bmp(const char* filename);
void write_bmp_gray(const char* filename, uint8_t** gray_pixels, int width, int height);
uint8_t** convert_to_grayscale(BMPImage* image);
void free_bmp(BMPImage* image);
void free_grayscale(uint8_t** gray, int height);

#endif