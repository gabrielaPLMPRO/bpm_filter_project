#include "bmp_utils.h"
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_ppm;
    int32_t y_ppm;
    uint32_t clr_used;
    uint32_t clr_important;
} DIBHeader;
#pragma pack(pop)

BMPImage* read_bmp(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;

    BMPHeader bmpHeader;
    DIBHeader dibHeader;

    fread(&bmpHeader, sizeof(BMPHeader), 1, f);
    fread(&dibHeader, sizeof(DIBHeader), 1, f);

    if (bmpHeader.type != 0x4D42 || dibHeader.bpp != 24) {
        fclose(f);
        return NULL;
    }

    BMPImage* img = malloc(sizeof(BMPImage));
    img->width = dibHeader.width;
    img->height = dibHeader.height;

    int padding = (4 - (img->width * 3) % 4) % 4;

    img->pixels = malloc(sizeof(RGB*) * img->height);
    for (int i = 0; i < img->height; i++) {
        img->pixels[i] = malloc(sizeof(RGB) * img->width);
        fread(img->pixels[i], sizeof(RGB), img->width, f);
        fseek(f, padding, SEEK_CUR);
    }

    fclose(f);
    return img;
}

uint8_t** convert_to_grayscale(BMPImage* img) {
    uint8_t** gray = malloc(sizeof(uint8_t*) * img->height);
    for (int i = 0; i < img->height; i++) {
        gray[i] = malloc(sizeof(uint8_t) * img->width);
        for (int j = 0; j < img->width; j++) {
            RGB p = img->pixels[i][j];
            gray[i][j] = (uint8_t)(0.299 * p.r + 0.587 * p.g + 0.114 * p.b);
        }
    }
    return gray;
}

void write_bmp_gray(const char* filename, uint8_t** gray, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Erro ao abrir o arquivo de saída.\n");
        return;
    }

    int padding = (4 - (width * 3) % 4) % 4;
    int row_size = (width * 3) + padding;
    int img_size = row_size * height;

    BMPHeader bmpHeader = { 0x4D42, 54 + img_size, 0, 0, 54 };
    DIBHeader dibHeader = {
        40, width, height, 1, 24, 0, img_size, 0, 0, 0, 0
    };

    fwrite(&bmpHeader, sizeof(BMPHeader), 1, f);
    fwrite(&dibHeader, sizeof(DIBHeader), 1, f);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            uint8_t value = gray[i][j];
            uint8_t pixel[3] = { value, value, value }; // B, G, R
            fwrite(pixel, 3, 1, f);
        }
        for (int p = 0; p < padding; p++)
            fputc(0, f);
    }

    fclose(f);
}

void free_bmp(BMPImage* img) {
    for (int i = 0; i < img->height; i++)
        free(img->pixels[i]);
    free(img->pixels);
    free(img);
}

void free_grayscale(uint8_t** gray, int height) {
    for (int i = 0; i < height; i++)
        free(gray[i]);
    free(gray);
}
