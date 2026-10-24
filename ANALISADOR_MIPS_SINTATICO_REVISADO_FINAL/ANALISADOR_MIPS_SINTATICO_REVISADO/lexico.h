#ifndef LEXICO_H
#define LEXICO_H

#include <stdio.h>

typedef struct {
    char nome[50];
    char lexema[8192];
    int linha;
    int coluna;
} Token;

typedef struct {
    char lexema[8192];
    char categoria[30];
    int primeiraLinha;
    int primeiraColuna;
} Simbolo;

/* Funcao obrigatoria do enunciado. */
void AnaliseLexica(FILE *in, FILE *out);

/* Define os nomes dos arquivos .ts e .err gerados pela analise. */
void ConfigurarSaidasLexicas(const char *arquivoTS, const char *arquivoERR);

#endif
