# Analisador Sintático MIPS

Projeto acadêmico desenvolvido em linguagem C para realizar a análise sintática de programas escritos em Assembly MIPS.

## Objetivo

Implementar um analisador sintático capaz de verificar se as instruções de um programa MIPS seguem as regras gramaticais definidas pelo projeto.

## Funcionalidades

- Pré-processamento do código-fonte.
- Análise léxica e identificação dos tokens.
- Análise sintática por descida recursiva.
- Identificação e apresentação de erros sintáticos.
- Recuperação de erros para permitir a identificação de múltiplos problemas.
- Geração de arquivos com os resultados da análise.

## Tecnologias utilizadas

- Linguagem C
- Compilador GCC
- Visual Studio Code
- Graphviz para representação de árvores sintáticas

## Como executar

Para compilar o projeto, abra um terminal na pasta dos arquivos e execute:

```bash
gcc -std=c11 -Wall -Wextra -pedantic main.c preprocessador.c lexico.c sintatico.c -o main.exe
```

Para analisar um arquivo MIPS:

```bash
./main.exe sint_ok_03.asm resultado
```

O programa informa se a entrada é sintaticamente válida ou inválida e gera os arquivos de saída correspondentes.

## Testes

O projeto inclui arquivos de exemplo para testar programas sintaticamente válidos e programas com erros.

## Contexto acadêmico

Projeto desenvolvido para a disciplina de Linguagens Formais, Autômatos e Compiladores, sob orientação do professor Marcelo Eustáquio.

**Instituição:** Universidade Católica de Brasília (UCB).
