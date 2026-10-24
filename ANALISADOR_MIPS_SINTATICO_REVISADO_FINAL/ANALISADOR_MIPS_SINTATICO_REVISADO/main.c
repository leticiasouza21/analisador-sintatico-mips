#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocessador.h"
#include "lexico.h"
#include "sintatico.h"
static int filename(char *dst,size_t size,const char *prefix,const char *ext){int n=snprintf(dst,size,"%s%s",prefix,ext);return n>=0&&(size_t)n<size;}
static int lex_has_errors(const char *path){FILE *f=fopen(path,"r");char s[2048];int any=0;if(!f)return -1;while(fgets(s,sizeof(s),f)){if(strstr(s,"Nenhum erro lexico encontrado.")==NULL){for(char *p=s;*p;p++)if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='\t'){any=1;break;}}}if(ferror(f))any=-1;fclose(f);return any;}
int main(int argc,char **argv){
 char pre[1024],lex[1024],ts[1024],le[1024],se[1024],dot[1024];FILE *in=NULL,*out=NULL,*tokens=NULL,*tree=NULL,*errors=NULL;int result=2;
 if(argc!=3){fprintf(stderr,"Uso: %s <entrada.asm> <prefixo_saida>\n",argv[0]);return 2;}
 if(!filename(pre,sizeof(pre),argv[2],".pre")||!filename(lex,sizeof(lex),argv[2],".lex")||!filename(ts,sizeof(ts),argv[2],".ts")||!filename(le,sizeof(le),argv[2],".lex.err")||!filename(se,sizeof(se),argv[2],".sin.err")||!filename(dot,sizeof(dot),argv[2],".dot")){fprintf(stderr,"Prefixo de saida muito longo.\n");return 2;}
 in=fopen(argv[1],"r");if(!in){perror("Abrir entrada");goto done;}
 out=fopen(pre,"w");if(!out){perror("Criar .pre");goto done;}
 if(!PreProcessamento(in,out)||fclose(out)!=0){out=NULL;fprintf(stderr,"Falha no pre-processamento.\n");goto done;}out=NULL;fclose(in);in=NULL;
 in=fopen(pre,"r");out=fopen(lex,"w");if(!in||!out){perror("Abrir arquivos lexicos");goto done;}
 ConfigurarSaidasLexicas(ts,le);AnaliseLexica(in,out);
 if(fclose(out)!=0){out=NULL;goto done;}out=NULL;fclose(in);in=NULL;
 int lexical=lex_has_errors(le);if(lexical<0){fprintf(stderr,"Falha ao ler erros lexicos.\n");goto done;}
 tree=fopen(dot,"w");errors=fopen(se,"w");if(!tree||!errors){perror("Criar arquivos sintaticos");goto done;}
 if(lexical){fputs("ANALISE SINTATICA NAO EXECUTADA: EXISTEM ERROS LEXICOS.\n",errors);puts("ANÁLISE SINTÁTICA NÃO EXECUTADA: EXISTEM ERROS LÉXICOS.");result=1;}
 else {tokens=fopen(lex,"r");if(!tokens){perror("Abrir .lex");goto done;}int s=AnaliseSintatica(tokens,tree,errors);if(s<0){fprintf(stderr,"Falha operacional na analise sintatica.\n");goto done;}result=s;puts(s?"PROGRAMA SINTATICAMENTE INVÁLIDO":"PROGRAMA SINTATICAMENTE VÁLIDO");}
 if(fclose(tree)!=0)result=2;
 tree=NULL;
 if(fclose(errors)!=0)result=2;
 errors=NULL;
 done:if(in)fclose(in);if(out)fclose(out);if(tokens)fclose(tokens);if(tree)fclose(tree);if(errors)fclose(errors);return result;
}
