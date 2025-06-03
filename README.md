# BMP_FILTER_PROJECT

## Compilação da aplicação
> Irá gerar o executável `filtro`
```bash
gcc -o filtro main.c bmp_utils.c filtros.c utils.c
```

## Chamada da Aplicação:
```bash
./filtro <imagem.bmp> <tamanho_mascara> <num_processos>
```
