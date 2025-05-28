#include <stdio.h>
#include <stdlib.h>
#include "bmp_utils.h"
#include "filtros.h"

int main(int argc, char* argv[]) {
    const char* input_file = "borboleta.bmp";

    BMPImage* image = read_bmp(input_file);
    if (!image) {
        fprintf(stderr, "Erro ao ler a imagem BMP.\n");
        return 1;
    }

    uint8_t** gray = convert_to_grayscale(image);
    if (!gray) {
        fprintf(stderr, "Erro ao converter para tons de cinza.\n");
        free_bmp(image);
        return 1;
    }

    write_bmp_gray("cinza.bmp", gray, image->width, image->height);
    printf("Imagem convertida para tons de cinza e salva em cinza.bmp\n");

    int mask_size = 3;
    uint8_t** filtered = median_filter(gray, image->width, image->height, mask_size);
    write_bmp_gray("mediana.bmp", filtered, image->width, image->height);
    printf("Filtro de mediana aplicado e salvo em mediana.bmp (máscara %dx%d)\n", mask_size, mask_size);

    uint8_t** laplaciano = laplacian_filter(filtered, image->width, image->height, 3);
    write_bmp_gray("laplaciano.bmp", laplaciano, image->width, image->height);
    printf("Filtro laplaciano aplicado e salvo em laplaciano.bmp\n");

    free_grayscale(gray, image->height);
    free_grayscale(filtered, image->height);
    free_grayscale(laplaciano, image->height);
    free_bmp(image);

    return 0;
}
