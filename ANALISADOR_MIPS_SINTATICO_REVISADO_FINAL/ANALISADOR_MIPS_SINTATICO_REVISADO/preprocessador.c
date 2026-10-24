#include "preprocessador.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TAM_LINHA 4096

/*
 * Remove comentarios iniciados por # fora de strings,
 * normaliza sequencias de espacos e preserva strings.
 * Mantem uma quebra de linha para cada linha da entrada,
 * preservando a numeracao das linhas.
 */
static void processar_linha(const char *linha, char *resultado)
{
    int i = 0;
    int j = 0;
    int dentro_string = 0;
    int escape = 0;
    int espaco_pendente = 0;

    while (linha[i] != '\0' && linha[i] != '\n' && linha[i] != '\r')
    {
        char c = linha[i];

        if (dentro_string)
        {
            resultado[j++] = c;

            if (escape)
                escape = 0;
            else if (c == '\\')
                escape = 1;
            else if (c == '"')
                dentro_string = 0;

            i++;
            continue;
        }

        if (c == '#')
            break;

        if (c == '"')
        {
            if (espaco_pendente && j > 0)
                resultado[j++] = ' ';

            espaco_pendente = 0;
            dentro_string = 1;
            resultado[j++] = c;
            i++;
            continue;
        }

        if (c == ' ' || c == '\t')
        {
            if (j > 0)
                espaco_pendente = 1;
            i++;
            continue;
        }

        if (espaco_pendente)
        {
            resultado[j++] = ' ';
            espaco_pendente = 0;
        }

        resultado[j++] = c;
        i++;
    }

    while (j > 0 && resultado[j - 1] == ' ')
        j--;

    resultado[j] = '\0';
}

int PreProcessamento(FILE *in, FILE *out)
{
    size_t capacidade = 256, tamanho = 0;
    char *linha = malloc(capacidade), *resultado = NULL;
    int c, ok = 1;
    if (!linha) return 0;
    while ((c = fgetc(in)) != EOF)
    {
        if (tamanho + 2 > capacidade)
        {
            char *novo;
            if (capacidade > ((size_t)-1)/2) { ok = 0; break; }
            capacidade *= 2;
            novo = realloc(linha, capacidade);
            if (!novo) { ok = 0; break; }
            linha = novo;
        }
        linha[tamanho++] = (char)c;
        if (c == '\n')
        {
            linha[tamanho] = '\0';
            resultado = malloc(tamanho + 1);
            if (!resultado) { ok = 0; break; }
            processar_linha(linha, resultado);
            if (fprintf(out, "%s\n", resultado) < 0) ok = 0;
            free(resultado); resultado = NULL;
            tamanho = 0;
            if (!ok) break;
        }
    }
    if (ok && tamanho > 0)
    {
        linha[tamanho] = '\0';
        resultado = malloc(tamanho + 1);
        if (!resultado) ok = 0;
        else
        {
            processar_linha(linha, resultado);
            if (fprintf(out, "%s\n", resultado) < 0) ok = 0;
        }
    }
    free(resultado);
    free(linha);
    return ok && !ferror(in) && !ferror(out);
}
