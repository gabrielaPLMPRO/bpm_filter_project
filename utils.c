#include <time.h>
#include <stdio.h>
#include <string.h>

void gerar_nome_com_timestamp(const char* base, const char* extensao, char* destino, size_t tamanho) {
    time_t agora = time(NULL);
    struct tm* tm_info = localtime(&agora);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    snprintf(destino, tamanho, "%s_%s.%s", base, timestamp, extensao);
}

