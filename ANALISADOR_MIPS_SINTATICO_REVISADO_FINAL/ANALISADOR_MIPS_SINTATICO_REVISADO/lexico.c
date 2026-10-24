#include "lexico.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_SIMBOLOS 300
#define MAX_LEXEMA 8192

typedef enum {
    ESTADO_INICIAL,
    ESTADO_IDENTIFICADOR,
    ESTADO_DIRETIVA,
    ESTADO_REGISTRADOR,
    ESTADO_NUMERO_DECIMAL,
    ESTADO_NUMERO_NEGATIVO,
    ESTADO_NUMERO_HEXADECIMAL,
    ESTADO_STRING,
    ESTADO_ESCAPE,
    ESTADO_COMENTARIO,
    ESTADO_SIMBOLO,
    ESTADO_ERRO
} Estado;

typedef struct {
    const char *lexema;
    const char *token;
} Reservado;

static const Reservado instrucoes[] = {
    {"li","INS_LI"}, {"la","INS_LA"}, {"move","INS_MOVE"},
    {"lw","INS_LW"}, {"sw","INS_SW"}, {"lb","INS_LB"},
    {"sb","INS_SB"}, {"lh","INS_LH"}, {"sh","INS_SH"},
    {"add","INS_ADD"}, {"addi","INS_ADDI"}, {"addu","INS_ADDU"},
    {"sub","INS_SUB"}, {"subu","INS_SUBU"}, {"mul","INS_MUL"},
    {"div","INS_DIV"}, {"mflo","INS_MFLO"}, {"mfhi","INS_MFHI"},
    {"and","INS_AND"}, {"or","INS_OR"}, {"xor","INS_XOR"},
    {"nor","INS_NOR"}, {"sll","INS_SLL"}, {"srl","INS_SRL"},
    {"slt","INS_SLT"}, {"beq","INS_BEQ"}, {"bne","INS_BNE"},
    {"blt","INS_BLT"}, {"ble","INS_BLE"}, {"bgt","INS_BGT"},
    {"bge","INS_BGE"}, {"j","INS_J"}, {"jal","INS_JAL"},
    {"jr","INS_JR"}, {"syscall","INS_SYSCALL"}, {"nop","INS_NOP"}
};

static const Reservado diretivas[] = {
    {".data","DIR_DATA"}, {".text","DIR_TEXT"}, {".word","DIR_WORD"},
    {".half","DIR_HALF"}, {".byte","DIR_BYTE"}, {".space","DIR_SPACE"},
    {".ascii","DIR_ASCII"}, {".asciiz","DIR_ASCIIZ"},
    {".globl","DIR_GLOBL"}, {".align","DIR_ALIGN"}
};

static Simbolo tabela[MAX_SIMBOLOS];
static int qtdSimbolos = 0;
static char nomeTS[512] = "saida.ts";
static char nomeERR[512] = "saida.err";
static FILE *arquivoErr = NULL;
static int qtdErros = 0;

static int letra(int c)
{
    return isalpha((unsigned char)c);
}

static int digito(int c)
{
    return isdigit((unsigned char)c);
}

static int alnum_(int c)
{
    return isalnum((unsigned char)c) || c == '_';
}

static int hexa(int c)
{
    return isxdigit((unsigned char)c);
}

static void minusculas(const char *origem, char *destino, size_t tam)
{
    size_t i;
    for (i = 0; origem[i] != '\0' && i + 1 < tam; i++)
        destino[i] = (char)tolower((unsigned char)origem[i]);
    destino[i] = '\0';
}

static const char *buscar_reservado(const Reservado *lista, int n,
                                    const char *lexema)
{
    int i;
    char baixo[MAX_LEXEMA];
    minusculas(lexema, baixo, sizeof(baixo));

    for (i = 0; i < n; i++)
        if (strcmp(baixo, lista[i].lexema) == 0)
            return lista[i].token;

    return NULL;
}

static const char *classificar_instrucao(const char *lexema)
{
    return buscar_reservado(instrucoes,
        (int)(sizeof(instrucoes)/sizeof(instrucoes[0])), lexema);
}

static const char *classificar_diretiva(const char *lexema)
{
    return buscar_reservado(diretivas,
        (int)(sizeof(diretivas)/sizeof(diretivas[0])), lexema);
}

static int registrador_valido(const char *lexema)
{
    char s[MAX_LEXEMA];
    int i, n;

    minusculas(lexema, s, sizeof(s));

    if (strcmp(s, "$zero") == 0) return 1;
    if (strcmp(s, "$gp") == 0 || strcmp(s, "$sp") == 0 ||
        strcmp(s, "$fp") == 0 || strcmp(s, "$ra") == 0) return 1;
    if (strcmp(s, "$v0") == 0 || strcmp(s, "$v1") == 0) return 1;
    if (strcmp(s, "$k0") == 0 || strcmp(s, "$k1") == 0) return 1;

    if (s[0] == '$' && s[1] == 'a' && s[2] >= '0' && s[2] <= '3' &&
        s[3] == '\0') return 1;

    if (s[0] == '$' && s[1] == 't' && s[2] >= '0' && s[2] <= '9' &&
        s[3] == '\0') return 1;

    if (s[0] == '$' && s[1] == 's' && s[2] >= '0' && s[2] <= '7' &&
        s[3] == '\0') return 1;

    if (s[0] == '$' && s[1] != '\0')
    {
        for (i = 1; s[i] != '\0'; i++)
            if (!digito(s[i]))
                return 0;

        n = atoi(s + 1);
        return n >= 0 && n <= 31;
    }

    return 0;
}

static int procurar_simbolo(const char *lexema)
{
    int i;
    for (i = 0; i < qtdSimbolos; i++)
        if (strcmp(tabela[i].lexema, lexema) == 0)
            return i;
    return -1;
}

static void inserir_simbolo(const char *lexema, const char *categoria,
                            int linha, int coluna)
{
    if (qtdSimbolos >= MAX_SIMBOLOS || procurar_simbolo(lexema) >= 0)
        return;

    snprintf(tabela[qtdSimbolos].lexema,
             sizeof(tabela[qtdSimbolos].lexema), "%s", lexema);
    snprintf(tabela[qtdSimbolos].categoria,
             sizeof(tabela[qtdSimbolos].categoria), "%s", categoria);
    tabela[qtdSimbolos].primeiraLinha = linha;
    tabela[qtdSimbolos].primeiraColuna = coluna;
    qtdSimbolos++;
}

static void atualizar_primeira_ocorrencia(const char *lexema,
                                           int linha, int coluna)
{
    int p = procurar_simbolo(lexema);
    if (p >= 0 && tabela[p].primeiraLinha == 0)
    {
        tabela[p].primeiraLinha = linha;
        tabela[p].primeiraColuna = coluna;
    }
}

static void inicializar_tabela(void)
{
    int i;
    char reg[20];

    qtdSimbolos = 0;

    for (i = 0; i < (int)(sizeof(instrucoes)/sizeof(instrucoes[0])); i++)
        inserir_simbolo(instrucoes[i].lexema, "INSTRUCAO", 0, 0);

    for (i = 0; i < (int)(sizeof(diretivas)/sizeof(diretivas[0])); i++)
        inserir_simbolo(diretivas[i].lexema, "DIRETIVA", 0, 0);

    inserir_simbolo("$zero", "REGISTRADOR", 0, 0);
    inserir_simbolo("$v0", "REGISTRADOR", 0, 0);
    inserir_simbolo("$v1", "REGISTRADOR", 0, 0);

    for (i = 0; i <= 3; i++) {
        snprintf(reg, sizeof(reg), "$a%d", i);
        inserir_simbolo(reg, "REGISTRADOR", 0, 0);
    }
    for (i = 0; i <= 9; i++) {
        snprintf(reg, sizeof(reg), "$t%d", i);
        inserir_simbolo(reg, "REGISTRADOR", 0, 0);
    }
    for (i = 0; i <= 7; i++) {
        snprintf(reg, sizeof(reg), "$s%d", i);
        inserir_simbolo(reg, "REGISTRADOR", 0, 0);
    }

    inserir_simbolo("$k0", "REGISTRADOR", 0, 0);
    inserir_simbolo("$k1", "REGISTRADOR", 0, 0);
    inserir_simbolo("$gp", "REGISTRADOR", 0, 0);
    inserir_simbolo("$sp", "REGISTRADOR", 0, 0);
    inserir_simbolo("$fp", "REGISTRADOR", 0, 0);
    inserir_simbolo("$ra", "REGISTRADOR", 0, 0);

    for (i = 0; i <= 31; i++) {
        snprintf(reg, sizeof(reg), "$%d", i);
        inserir_simbolo(reg, "REGISTRADOR", 0, 0);
    }
}

static void registrar_erro(const char *tipo, const char *lexema,
                           int linha, int coluna)
{
    if (arquivoErr != NULL)
        fprintf(arquivoErr, "<%s, %s> %d %d\n",
                tipo, lexema, linha, coluna);
    qtdErros++;
}

static void emitir_token(FILE *out, const char *nome, const char *lexema,
                         int linha, int coluna)
{
    fprintf(out, "<%s, %s> %d %d\n", nome, lexema, linha, coluna);
}

static void salvar_tabela(void)
{
    FILE *f = fopen(nomeTS, "w");
    int i;

    if (f == NULL)
        return;

    fprintf(f, "LEXEMA,CATEGORIA,LINHA,COLUNA\n");
    for (i = 0; i < qtdSimbolos; i++)
        fprintf(f, "%s,%s,%d,%d\n",
                tabela[i].lexema,
                tabela[i].categoria,
                tabela[i].primeiraLinha,
                tabela[i].primeiraColuna);

    fclose(f);
}

void ConfigurarSaidasLexicas(const char *arquivoTSNome,
                             const char *arquivoERRNome)
{
    if (arquivoTSNome != NULL)
        snprintf(nomeTS, sizeof(nomeTS), "%s", arquivoTSNome);
    if (arquivoERRNome != NULL)
        snprintf(nomeERR, sizeof(nomeERR), "%s", arquivoERRNome);
}

/*
 * Implementacao do AFD.
 * Cada bloco abaixo corresponde a uma transicao a partir do
 * ESTADO_INICIAL para um estado especializado do automato.
 */
void AnaliseLexica(FILE *in, FILE *out)
{
    int c;
    int linha = 1;
    int coluna = 1;
    Estado estado = ESTADO_INICIAL;

    inicializar_tabela();
    qtdErros = 0;
    arquivoErr = fopen(nomeERR, "w");

    if (arquivoErr == NULL)
    {
        fprintf(stderr, "Erro: nao foi possivel criar %s\n", nomeERR);
        return;
    }

    while ((c = fgetc(in)) != EOF)
    {
        estado = ESTADO_INICIAL;

        if (c == ' ' || c == '\t' || c == '\r')
        {
            coluna++;
            continue;
        }

        if (c == '\n')
        {
            linha++;
            coluna = 1;
            continue;
        }

        /*
         * Comentario. Normalmente ja foi removido pelo pre-processador,
         * mas o estado existe para representar o AFD completo.
         */
        if (c == '#')
        {
            estado = ESTADO_COMENTARIO;
            coluna++;

            while ((c = fgetc(in)) != EOF && c != '\n')
                coluna++;

            if (c == '\n')
            {
                linha++;
                coluna = 1;
            }
            continue;
        }

        /* DIRETIVA */
        if (c == '.')
        {
            char lexema[MAX_LEXEMA];
            int i = 0;
            int excedeu = 0;
            int li = linha, co = coluna;
            const char *tipo;
            char baixo[MAX_LEXEMA];

            estado = ESTADO_DIRETIVA;
            lexema[i++] = (char)c;
            coluna++;

            while ((c = fgetc(in)) != EOF && alnum_(c))
            {
                if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c; else excedeu = 1;
                coluna++;
            }
            lexema[i] = '\0';

            if (c != EOF) ungetc(c, in);

            if (excedeu) { registrar_erro("ERRO_LEXEMA_MUITO_LONGO", lexema, li, co); continue; }
            tipo = classificar_diretiva(lexema);
            if (tipo == NULL)
            {
                estado = ESTADO_ERRO;
                registrar_erro("ERRO_DIRETIVA_INVALIDA", lexema, li, co);
            }
            else
            {
                emitir_token(out, tipo, lexema, li, co);
                minusculas(lexema, baixo, sizeof(baixo));
                atualizar_primeira_ocorrencia(baixo, li, co);
            }
            continue;
        }

        /* REGISTRADOR */
        if (c == '$')
        {
            char lexema[MAX_LEXEMA];
            char baixo[MAX_LEXEMA];
            int i = 0;
            int excedeu = 0;
            int li = linha, co = coluna;

            estado = ESTADO_REGISTRADOR;
            lexema[i++] = '$';
            coluna++;

            while ((c = fgetc(in)) != EOF && alnum_(c))
            {
                if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c; else excedeu = 1;
                coluna++;
            }
            lexema[i] = '\0';

            if (c != EOF) ungetc(c, in);

            if (excedeu) { registrar_erro("ERRO_LEXEMA_MUITO_LONGO", lexema, li, co); continue; }
            if (!registrador_valido(lexema))
            {
                estado = ESTADO_ERRO;
                registrar_erro("ERRO_REGISTRADOR_INVALIDO", lexema, li, co);
            }
            else
            {
                emitir_token(out, "REG", lexema, li, co);
                minusculas(lexema, baixo, sizeof(baixo));
                atualizar_primeira_ocorrencia(baixo, li, co);
            }
            continue;
        }

        /* STRING */
        if (c == '"')
        {
            char lexema[MAX_LEXEMA];
            int i = 0;
            int excedeu = 0;
            int li = linha, co = coluna;
            int fechou = 0;
            int escapeInvalido = 0;
            int terminouEmEOF = 0;

            estado = ESTADO_STRING;
            lexema[i++] = '"';
            coluna++;

            while ((c = fgetc(in)) != EOF)
            {
                if (c == '\n')
                {
                    linha++;
                    coluna = 1;
                    break;
                }

                if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c; else excedeu = 1;
                coluna++;

                if (c == '\\')
                {
                    int p;
                    estado = ESTADO_ESCAPE;
                    p = fgetc(in);

                    if (p == EOF)
                    {
                        terminouEmEOF = 1;
                        break;
                    }

                    if (p == '\n')
                    {
                        linha++;
                        coluna = 1;
                        break;
                    }

                    if (i < MAX_LEXEMA - 1) lexema[i++] = (char)p; else excedeu = 1;
                    coluna++;

                    if (p != 'n' && p != 't' && p != '"' &&
                        p != '\\' && p != '0')
                        escapeInvalido = 1;

                    estado = ESTADO_STRING;
                    continue;
                }

                if (c == '"')
                {
                    fechou = 1;
                    break;
                }
            }

            if (c == EOF && !fechou)
                terminouEmEOF = 1;

            lexema[i] = '\0';

            if (excedeu) { registrar_erro("ERRO_LEXEMA_MUITO_LONGO", lexema, li, co); continue; }
            if (!fechou)
            {
                estado = ESTADO_ERRO;
                if (terminouEmEOF)
                    registrar_erro("ERRO_STRING_NAO_FECHADA_EOF",
                                   lexema, li, co);
                else
                    registrar_erro("ERRO_STRING_NAO_FECHADA",
                                   lexema, li, co);
            }
            else if (escapeInvalido)
            {
                estado = ESTADO_ERRO;
                registrar_erro("ERRO_ESCAPE_INVALIDO", lexema, li, co);
            }
            else
            {
                emitir_token(out, "STRING", lexema, li, co);
            }
            continue;
        }

        /* NUMERO: decimal, negativo, hexadecimal e hexadecimal negativo */
        if (digito(c) || c == '-')
        {
            char lexema[MAX_LEXEMA];
            int i = 0;
            int excedeu = 0;
            int li = linha, co = coluna;
            int negativo = 0;
            int malformado = 0;
            int p;

            estado = ESTADO_NUMERO_DECIMAL;

            if (c == '-')
            {
                estado = ESTADO_NUMERO_NEGATIVO;
                p = fgetc(in);

                if (p == EOF || !digito(p))
                {
                    if (p != EOF) ungetc(p, in);
                    registrar_erro("ERRO_CARACTERE_INVALIDO", "-", li, co);
                    coluna++;
                    continue;
                }

                negativo = 1;
                lexema[i++] = '-';
                coluna++;
                c = p;
            }

            lexema[i++] = (char)c;
            coluna++;

            /* Prefixo hexadecimal 0x / -0x */
            if (c == '0')
            {
                p = fgetc(in);
                if (p == 'x' || p == 'X')
                {
                    int temHexa = 0;
                    estado = ESTADO_NUMERO_HEXADECIMAL;
                    if (i < MAX_LEXEMA - 1) lexema[i++] = (char)p; else excedeu = 1;
                    coluna++;

                    while ((c = fgetc(in)) != EOF && alnum_(c))
                    {
                        if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c; else excedeu = 1;
                        coluna++;
                        if (hexa(c))
                            temHexa = 1;
                        else
                            malformado = 1;
                    }

                    lexema[i] = '\0';
                    if (c != EOF) ungetc(c, in);

                    if (excedeu) { registrar_erro("ERRO_LEXEMA_MUITO_LONGO", lexema, li, co); continue; }
                    if (!temHexa || malformado)
                        registrar_erro("ERRO_NUMERO_MALFORMADO",
                                       lexema, li, co);
                    else
                        emitir_token(out, "NUM_INT", lexema, li, co);
                    continue;
                }
                else if (p != EOF)
                {
                    ungetc(p, in);
                }
            }

            /* Decimal. Se letras/_ vierem coladas, o lexema e malformado. */
            while ((c = fgetc(in)) != EOF && alnum_(c))
            {
                if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c; else excedeu = 1;
                coluna++;
                if (!digito(c))
                    malformado = 1;
            }

            lexema[i] = '\0';
            if (c != EOF) ungetc(c, in);

            if (excedeu) { registrar_erro("ERRO_LEXEMA_MUITO_LONGO", lexema, li, co); continue; }
            if (malformado)
            {
                estado = ESTADO_ERRO;
                if (!negativo && digito((unsigned char)lexema[0]))
                    registrar_erro("ERRO_IDENTIFICADOR_MALFORMADO",
                                   lexema, li, co);
                else
                    registrar_erro("ERRO_NUMERO_MALFORMADO",
                                   lexema, li, co);
            }
            else
            {
                emitir_token(out, "NUM_INT", lexema, li, co);
            }
            continue;
        }

        /* IDENTIFICADOR OU INSTRUCAO */
        if (letra(c) || c == '_')
        {
            char lexema[MAX_LEXEMA];
            char baixo[MAX_LEXEMA];
            int i = 0;
            int excedeu = 0;
            int li = linha, co = coluna;
            int p;
            const char *tipo;

            estado = ESTADO_IDENTIFICADOR;
            lexema[i++] = (char)c;
            coluna++;

            while ((c = fgetc(in)) != EOF && alnum_(c))
            {
                if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c; else excedeu = 1;
                coluna++;
            }

            /*
             * Se apos a parte valida vier um caractere colado que nao e
             * delimitador, todo o trecho e tratado como identificador
             * malformado. Exemplos: soma-total e inicio@.
             */
            if (c != EOF &&
                c != ' ' && c != '\t' && c != '\r' && c != '\n' &&
                c != ',' && c != ':' && c != '(' && c != ')' && c != '#')
            {
                do
                {
                    if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c; else excedeu = 1;
                    coluna++;
                    c = fgetc(in);
                }
                while (c != EOF &&
                       c != ' ' && c != '\t' && c != '\r' && c != '\n' &&
                       c != ',' && c != ':' && c != '(' && c != ')' && c != '#');

                lexema[i] = '\0';
                if (c != EOF) ungetc(c, in);

                estado = ESTADO_ERRO;
                registrar_erro(excedeu ? "ERRO_LEXEMA_MUITO_LONGO" : "ERRO_IDENTIFICADOR_MALFORMADO",
                               lexema, li, co);
                continue;
            }

            lexema[i] = '\0';
            if (c != EOF) ungetc(c, in);

            if (excedeu) { registrar_erro("ERRO_LEXEMA_MUITO_LONGO", lexema, li, co); continue; }
            tipo = classificar_instrucao(lexema);

            p = fgetc(in);
            if (p != EOF) ungetc(p, in);
            if (tipo != NULL && p == ':')
            {
                registrar_erro("ERRO_ROTULO_RESERVADO", lexema, li, co);
            }
            else if (tipo != NULL)
            {
                emitir_token(out, tipo, lexema, li, co);
                minusculas(lexema, baixo, sizeof(baixo));
                atualizar_primeira_ocorrencia(baixo, li, co);
            }
            else
            {
                emitir_token(out, "ID", lexema, li, co);

                /*
                 * Rotulo = identificador imediatamente seguido por ':'.
                 * O ':' continua sendo token separado.
                 */
                p = fgetc(in);
                if (p != EOF)
                {
                    ungetc(p, in);
                    if (p == ':')
                        inserir_simbolo(lexema, "ROTULO", li, co);
                }
            }
            continue;
        }

        /* SIMBOLOS */
        estado = ESTADO_SIMBOLO;
        if (c == ',')
        {
            emitir_token(out, "SMB_COM", ",", linha, coluna);
            coluna++;
            continue;
        }
        if (c == ':')
        {
            emitir_token(out, "SMB_COL", ":", linha, coluna);
            coluna++;
            continue;
        }
        if (c == '(')
        {
            emitir_token(out, "SMB_OPA", "(", linha, coluna);
            coluna++;
            continue;
        }
        if (c == ')')
        {
            emitir_token(out, "SMB_CPA", ")", linha, coluna);
            coluna++;
            continue;
        }

        /* Qualquer outro caractere e invalido. */
        {
            char invalido[2];
            estado = ESTADO_ERRO;
            invalido[0] = (char)c;
            invalido[1] = '\0';
            registrar_erro("ERRO_CARACTERE_INVALIDO",
                           invalido, linha, coluna);
            coluna++;
        }
    }

    /* Token especial de fim de arquivo. */
    emitir_token(out, "TK_EOF", "EOF", linha, coluna);

    if (qtdErros == 0)
        fprintf(arquivoErr, "Nenhum erro lexico encontrado.\n");

    fclose(arquivoErr);
    arquivoErr = NULL;

    salvar_tabela();

    /* Evita warning em compiladores muito rigorosos. */
    (void)estado;
}
