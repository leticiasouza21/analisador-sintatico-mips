ANALISADOR MIPS - ETAPA 04 - ANALISE SINTATICA

Projeto baseado na versao corrigida da etapa de analise lexica.
Mantidos: preprocessador.c/.h, lexico.c/.h e testes anteriores.
Alterados: main.c e README.txt.
Novos: sintatico.c/.h e testes sint_ok_*.asm e sint_erro_*.asm.

COMPILAR (GCC):
gcc -std=c11 -Wall -Wextra -pedantic main.c preprocessador.c lexico.c sintatico.c -o main.exe

EXECUTAR:
./main.exe sint_ok_03.asm resultado

SAIDAS:
resultado.pre      codigo pre-processado
resultado.lex      tokens do lexico
resultado.ts       tabela de simbolos
resultado.lex.err  erros lexicos
resultado.sin.err  erros sintaticos
resultado.dot      arvore concreta para entrada valida

VISUALIZAR ARVORE (Graphviz):
dot -Tpng resultado.dot -o resultado.png

CODIGOS DE RETORNO:
0 valido, 1 erro lexico/sintatico, 2 falha operacional.

TESTES DA ETAPA 04:
sint_ok_01.asm  - instrucoes e grupos de operandos
sint_ok_02.asm  - diretivas de dados, alternancia, secao vazia
sint_ok_03.asm  - rotulos, salto, sem main obrigatorio
sint_ok_sem_quebra.asm - sem quebra final
sint_erro_01.asm - operandos ausentes, pontuacao, excesso
sint_erro_02.asm - lista com virgula final, secoes, parenteses
sint_erro_03.asm - sem secao, operandos incorretos, parenteses
sint_erro_vazio.asm - entrada vazia
sint_erro_lexico.asm - erro lexico bloqueia sintatico

IMPORTANTE: Este analisador verifica apenas sintaxe, nao semantica.
O relatorio tecnico e a arvore PNG sao entregues separadamente.

REVISAO FINAL: as funcoes do analisador sintatico foram separadas por nao terminal da gramatica. Testes de regressao: 4 validos, 5 invalidos (incluindo erro lexico). Arvore DOT renderizada com Graphviz.
