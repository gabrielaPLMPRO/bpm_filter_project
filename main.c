#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bmp_utils.h"
#include "filtros.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <imagem.bmp> <tamanho_mascara> <num_processos>\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    int mask_size = atoi(argv[2]);
    int num_processos = atoi(argv[3]); // Ainda não usado, mas validado

    if (mask_size < 3 || mask_size % 2 == 0) {
        fprintf(stderr, "Erro: tamanho da máscara deve ser um número ímpar ≥ 3.\n");
        return 1;
    }

    if (num_processos < 1) {
        fprintf(stderr, "Erro: número de processos deve ser ≥ 1.\n");
        return 1;
    }

    BMPImage* image = read_bmp(input_file);
    if (!image) {
        fprintf(stderr, "Erro ao ler a imagem BMP: %s\n", input_file);
        return 1;
    }

    uint8_t** gray = convert_to_grayscale(image);
    if (!gray) {
        fprintf(stderr, "Erro ao converter para tons de cinza.\n");
        free_bmp(image);
        return 1;
    }

    char nome_cinza[64];
    gerar_nome_com_timestamp("cinza", "bmp", nome_cinza, sizeof(nome_cinza));
    write_bmp_gray(nome_cinza, gray, image->width, image->height);
    printf("Imagem convertida para tons de cinza e salva em %s\n", nome_cinza);

    uint8_t** filtered = median_filter(gray, image->width, image->height, mask_size);
    if (!filtered) {
        fprintf(stderr, "Erro ao aplicar filtro de mediana.\n");
        free_grayscale(gray, image->height);
        free_bmp(image);
        return 1;
    }

    char nome_mediana[64];
    gerar_nome_com_timestamp("mediana", "bmp", nome_mediana, sizeof(nome_mediana));
    write_bmp_gray(nome_mediana, filtered, image->width, image->height);
    printf("Filtro de mediana aplicado e salvo em %s (máscara %dx%d)\n", nome_mediana, mask_size, mask_size);

    uint8_t** laplaciano = laplacian_filter(filtered, image->width, image->height, 3);
    if (laplaciano) {
        char nome_laplaciano[64];
        gerar_nome_com_timestamp("laplaciano", "bmp", nome_laplaciano, sizeof(nome_laplaciano));
        write_bmp_gray(nome_laplaciano, laplaciano, image->width, image->height);
        printf("Filtro laplaciano aplicado e salvo em %s\n", nome_laplaciano);
    } else {
        printf("Erro ao aplicar o filtro laplaciano.\n");
    }

    free_grayscale(gray, image->height);
    free_grayscale(filtered, image->height);
    if (laplaciano)
        free_grayscale(laplaciano, image->height);
    free_bmp(image);

    return 0;
}

