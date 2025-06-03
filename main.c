#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdint.h>
#include "bmp_utils.h"
#include "filtros.h"
#include "utils.h"

void median_filter_parallel_worker(uint8_t** gray, int width, int height, int mask_size,
                                   uint8_t* shared_result, int start_line, int end_line) {
    int offset = mask_size / 2;
    for (int i = start_line; i < end_line; i++) {
        for (int j = 0; j < width; j++) {

            int count = 0;
            int max_vals = mask_size * mask_size;
            uint8_t neighbors[max_vals];

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
            shared_result[i * width + j] = neighbors[count / 2];
        }
    }
}

void laplacian_filter_parallel_worker(uint8_t** input, int width, int height, int mask_size,
                                      uint8_t* shared_result, int start_line, int end_line) {
    int offset = mask_size / 2;

    for (int i = start_line; i < end_line; i++) {
        for (int j = 0; j < width; j++) {
            int sum = 0;
            for (int mi = -offset; mi <= offset; mi++) {
                for (int mj = -offset; mj <= offset; mj++) {
                    int ni = i + mi;
                    int nj = j + mj;

                    if (ni < 0) ni = 0;
                    if (nj < 0) nj = 0;
                    if (ni >= height) ni = height - 1;
                    if (nj >= width) nj = width - 1;

                    int weight = 0;
                    if (mi == 0 && mj == 0) {
                        weight = 8;
                    } else {
                        weight = -1;
                    }
                    sum += input[ni][nj] * weight;
                }
            }
            if (sum < 0) sum = 0;
            if (sum > 255) sum = 255;

            shared_result[i * width + j] = (uint8_t)sum;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <imagem.bmp> <tamanho_mascara> <num_processos>\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    int mask_size = atoi(argv[2]);
    int num_processos = atoi(argv[3]);

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

    size_t img_size = image->width * image->height * sizeof(uint8_t);

    uint8_t* shared_filtered = mmap(NULL, img_size, PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_filtered == MAP_FAILED) {
        perror("mmap");
        free_grayscale(gray, image->height);
        free_bmp(image);
        return 1;
    }

    int linhas_por_proc = image->height / num_processos;
    int linhas_resto = image->height % num_processos;

    pid_t* pids = malloc(num_processos * sizeof(pid_t));
    if (!pids) {
        fprintf(stderr, "Erro ao alocar memória para pids\n");
        munmap(shared_filtered, img_size);
        free_grayscale(gray, image->height);
        free_bmp(image);
        return 1;
    }

    int linha_inicial = 0;
    for (int p = 0; p < num_processos; p++) {
        int linhas_este_proc = linhas_por_proc + (p == num_processos - 1 ? linhas_resto : 0);
        int linha_final = linha_inicial + linhas_este_proc;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            for (int k = 0; k < p; k++) wait(NULL);
            munmap(shared_filtered, img_size);
            free_grayscale(gray, image->height);
            free_bmp(image);
            free(pids);
            return 1;
        } else if (pid == 0) {
            printf("Processo filho PID %d processando linhas de %d a %d (mediana)\n",
                   getpid(), linha_inicial, linha_final);
            median_filter_parallel_worker(gray, image->width, image->height, mask_size,
                                         shared_filtered, linha_inicial, linha_final);
            _exit(0);
        } else {
            pids[p] = pid;
        }
        linha_inicial = linha_final;
    }

    for (int p = 0; p < num_processos; p++) {
        waitpid(pids[p], NULL, 0);
    }
    free(pids);

    uint8_t** filtered_2d = malloc(image->height * sizeof(uint8_t*));
    if (!filtered_2d) {
        fprintf(stderr, "Erro ao alocar memória para filtered_2d\n");
        munmap(shared_filtered, img_size);
        free_grayscale(gray, image->height);
        free_bmp(image);
        return 1;
    }
    for (int i = 0; i < image->height; i++) {
        filtered_2d[i] = &shared_filtered[i * image->width];
    }

    char nome_mediana[64];
    gerar_nome_com_timestamp("mediana", "bmp", nome_mediana, sizeof(nome_mediana));
    write_bmp_gray(nome_mediana, filtered_2d, image->width, image->height);
    printf("Filtro de mediana aplicado e salvo em %s (máscara %dx%d) com %d processos\n",
           nome_mediana, mask_size, mask_size, num_processos);

    uint8_t* shared_laplacian = mmap(NULL, img_size, PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_laplacian == MAP_FAILED) {
        perror("mmap");
        free(filtered_2d);
        munmap(shared_filtered, img_size);
        free_grayscale(gray, image->height);
        free_bmp(image);
        return 1;
    }

    linha_inicial = 0;
    pids = malloc(num_processos * sizeof(pid_t));
    if (!pids) {
        fprintf(stderr, "Erro ao alocar memória para pids\n");
        munmap(shared_laplacian, img_size);
        free(filtered_2d);
        munmap(shared_filtered, img_size);
        free_grayscale(gray, image->height);
        free_bmp(image);
        return 1;
    }

    for (int p = 0; p < num_processos; p++) {
        int linhas_este_proc = linhas_por_proc + (p == num_processos - 1 ? linhas_resto : 0);
        int linha_final = linha_inicial + linhas_este_proc;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            for (int k = 0; k < p; k++) wait(NULL);
            munmap(shared_laplacian, img_size);
            free(filtered_2d);
            munmap(shared_filtered, img_size);
            free_grayscale(gray, image->height);
            free_bmp(image);
            free(pids);
            return 1;
        } else if (pid == 0) {
            printf("Processo filho PID %d processando linhas de %d a %d (laplaciano)\n",
                   getpid(), linha_inicial, linha_final);
            laplacian_filter_parallel_worker(filtered_2d, image->width, image->height,
                                mask_size, shared_laplacian, linha_inicial, linha_final);
            _exit(0);
        } else {
            pids[p] = pid;
        }
        linha_inicial = linha_final;
    }

    for (int p = 0; p < num_processos; p++) {
        waitpid(pids[p], NULL, 0);
    }
    free(pids);

    uint8_t** laplaciano_2d = malloc(image->height * sizeof(uint8_t*));
    if (!laplaciano_2d) {
        fprintf(stderr, "Erro ao alocar memória para laplaciano_2d\n");
        munmap(shared_laplacian, img_size);
        free(filtered_2d);
        munmap(shared_filtered, img_size);
        free_grayscale(gray, image->height);
        free_bmp(image);
        return 1;
    }
    for (int i = 0; i < image->height; i++) {
        laplaciano_2d[i] = &shared_laplacian[i * image->width];
    }

    char nome_laplaciano[64];
    gerar_nome_com_timestamp("laplaciano", "bmp", nome_laplaciano, sizeof(nome_laplaciano));
    write_bmp_gray(nome_laplaciano, laplaciano_2d, image->width, image->height);
    printf("Filtro laplaciano paralelo aplicado e salvo em %s com %d processos\n",
           nome_laplaciano, num_processos);

    // Limpeza
    free_grayscale(gray, image->height);
    free(filtered_2d);
    free(laplaciano_2d);
    munmap(shared_filtered, img_size);
    munmap(shared_laplacian, img_size);
    free_bmp(image);

    return 0;
}
