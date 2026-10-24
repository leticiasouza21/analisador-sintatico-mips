#include "sintatico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {char *cat,*lex; int line,col;} Tok;
typedef struct {char *name; int parent;} Node;
static Tok *ts; static size_t nt,cap,pos;
static Node *ns; static size_t nn,ncap;
static FILE *err; static int failures,iofail;
static int current_section; /* 1 data, 2 text */
static char *dupstr(const char *s){size_t n=strlen(s)+1; char *p=malloc(n); if(p)memcpy(p,s,n);return p;}
static int node(int parent,const char *name){
 if(nn==ncap){size_t c=ncap?ncap*2:256;Node *p=realloc(ns,c*sizeof(*p));if(!p){iofail=1;return -1;}ns=p;ncap=c;}
 ns[nn].name=dupstr(name);if(!ns[nn].name){iofail=1;return -1;}ns[nn].parent=parent;return (int)nn++;
}
static Tok *peek(void){return &ts[pos<nt?pos:nt-1];}
static int is(const char *cat){return strcmp(peek()->cat,cat)==0;}
static int issection(void){return is("DIR_DATA")||is("DIR_TEXT");}
static void diagnostic(const char *expected){Tok *t=peek();failures++;fprintf(err,"Linha %d, coluna %d: Encontrado %s(\"%s\"); esperado %s.\n",t->line,t->col,t->cat,t->lex,expected);}
static void leaf(int p){char *buf;size_t n=strlen(peek()->cat)+strlen(peek()->lex)+4;buf=malloc(n);if(!buf){iofail=1;return;}snprintf(buf,n,"%s\\n%s",peek()->cat,peek()->lex);node(p,buf);free(buf);if(pos+1<nt)pos++;}
static int take(int p,const char *cat){if(!is(cat)){diagnostic(cat);return 0;}leaf(p);return 1;}
static void recover(void){while(!is("EOL")&&!is("TK_EOF"))pos++;if(is("EOL"))pos++;}
/* Descendente recursivo: cada nao terminal tem sua propria rotina. */
static int in(const char *s,const char *set){size_t len=strlen(s);const char *p=set;while(*p){size_t i=0;while(p[i]&&p[i]!='|')i++;if(i==len&&strncmp(p,s,i)==0)return 1;p+=i;if(*p=='|')p++;}return 0;}
static int op_tres_registradores(int p){int n=node(p,"<op_tres_registradores>");if(!in(peek()->cat,"INS_ADD|INS_ADDU|INS_SUB|INS_SUBU|INS_MUL|INS_AND|INS_OR|INS_XOR|INS_NOR|INS_SLT")){diagnostic("operacao de tres registradores");return 0;}leaf(n);return 1;}
static int op_dois_registradores(int p){int n=node(p,"<op_dois_registradores>");if(!in(peek()->cat,"INS_MOVE|INS_DIV")){diagnostic("operacao de dois registradores");return 0;}leaf(n);return 1;}
static int op_um_registrador(int p){int n=node(p,"<op_um_registrador>");if(!in(peek()->cat,"INS_MFLO|INS_MFHI|INS_JR")){diagnostic("operacao de um registrador");return 0;}leaf(n);return 1;}
static int op_deslocamento(int p){int n=node(p,"<op_deslocamento>");if(!in(peek()->cat,"INS_SLL|INS_SRL")){diagnostic("operacao de deslocamento");return 0;}leaf(n);return 1;}
static int op_memoria(int p){int n=node(p,"<op_memoria>");if(!in(peek()->cat,"INS_LW|INS_SW|INS_LB|INS_SB|INS_LH|INS_SH")){diagnostic("operacao de memoria");return 0;}leaf(n);return 1;}
static int op_desvio(int p){int n=node(p,"<op_desvio>");if(!in(peek()->cat,"INS_BEQ|INS_BNE|INS_BLT|INS_BLE|INS_BGT|INS_BGE")){diagnostic("operacao de desvio");return 0;}leaf(n);return 1;}
static int op_salto(int p){int n=node(p,"<op_salto>");if(!in(peek()->cat,"INS_J|INS_JAL")){diagnostic("operacao de salto");return 0;}leaf(n);return 1;}
static int instrucao_tres_registradores(int p){int n=node(p,"<instrucao_tres_registradores>");return op_tres_registradores(n)&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"REG");}
static int instrucao_dois_registradores(int p){int n=node(p,"<instrucao_dois_registradores>");return op_dois_registradores(n)&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"REG");}
static int instrucao_um_registrador(int p){int n=node(p,"<instrucao_um_registrador>");return op_um_registrador(n)&&take(n,"REG");}
static int instrucao_imediato(int p){int n=node(p,"<instrucao_imediato>");return take(n,"INS_ADDI")&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"NUM_INT");}
static int instrucao_deslocamento(int p){int n=node(p,"<instrucao_deslocamento>");return op_deslocamento(n)&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"NUM_INT");}
static int instrucao_li(int p){int n=node(p,"<instrucao_li>");return take(n,"INS_LI")&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"NUM_INT");}
static int instrucao_la(int p){int n=node(p,"<instrucao_la>");return take(n,"INS_LA")&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"ID");}
static int endereco(int p){int n=node(p,"<endereco>");if(is("ID")){leaf(n);return 1;}return take(n,"NUM_INT")&&take(n,"SMB_OPA")&&take(n,"REG")&&take(n,"SMB_CPA");}
static int instrucao_memoria(int p){int n=node(p,"<instrucao_memoria>");return op_memoria(n)&&take(n,"REG")&&take(n,"SMB_COM")&&endereco(n);}
static int instrucao_desvio(int p){int n=node(p,"<instrucao_desvio>");return op_desvio(n)&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"REG")&&take(n,"SMB_COM")&&take(n,"ID");}
static int instrucao_salto(int p){int n=node(p,"<instrucao_salto>");return op_salto(n)&&take(n,"ID");}
static int instrucao_sem_operandos(int p){int n=node(p,"<instrucao_sem_operandos>");if(!in(peek()->cat,"INS_SYSCALL|INS_NOP")){diagnostic("syscall ou nop");return 0;}leaf(n);return 1;}
static int instrucao(int p){int n=node(p,"<instrucao>");const char *s=peek()->cat;
 if(in(s,"INS_ADD|INS_ADDU|INS_SUB|INS_SUBU|INS_MUL|INS_AND|INS_OR|INS_XOR|INS_NOR|INS_SLT"))return instrucao_tres_registradores(n);
 if(in(s,"INS_MOVE|INS_DIV"))return instrucao_dois_registradores(n);
 if(in(s,"INS_MFLO|INS_MFHI|INS_JR"))return instrucao_um_registrador(n);
 if(in(s,"INS_ADDI"))return instrucao_imediato(n);
 if(in(s,"INS_SLL|INS_SRL"))return instrucao_deslocamento(n);
 if(in(s,"INS_LI"))return instrucao_li(n);
 if(in(s,"INS_LA"))return instrucao_la(n);
 if(in(s,"INS_LW|INS_SW|INS_LB|INS_SB|INS_LH|INS_SH"))return instrucao_memoria(n);
 if(in(s,"INS_BEQ|INS_BNE|INS_BLT|INS_BLE|INS_BGT|INS_BGE"))return instrucao_desvio(n);
 if(in(s,"INS_J|INS_JAL"))return instrucao_salto(n);
 if(in(s,"INS_SYSCALL|INS_NOP"))return instrucao_sem_operandos(n);
 diagnostic("instrucao MIPS");return 0;
}
static int rotulo(int p){int n=node(p,"<rotulo>");return take(n,"ID")&&take(n,"SMB_COL");}
static int diretiva_global(int p){int n=node(p,"<diretiva_global>");return take(n,"DIR_GLOBL")&&take(n,"ID");}
static int diretiva_inteiros(int p){int n=node(p,"<diretiva_inteiros>");if(!in(peek()->cat,"DIR_WORD|DIR_HALF|DIR_BYTE")){diagnostic(".word, .half ou .byte");return 0;}leaf(n);return 1;}
static int lista_inteiros(int p){int n=node(p,"<lista_inteiros>");if(!take(n,"NUM_INT"))return 0;while(is("SMB_COM")){leaf(n);if(!take(n,"NUM_INT"))return 0;}return 1;}
static int diretiva_string(int p){int n=node(p,"<diretiva_string>");if(!in(peek()->cat,"DIR_ASCII|DIR_ASCIIZ")){diagnostic(".ascii ou .asciiz");return 0;}leaf(n);return take(p,"STRING");}
static int diretiva_dados(int p){int n=node(p,"<diretiva_dados>");const char *s=peek()->cat;
 if(in(s,"DIR_WORD|DIR_HALF|DIR_BYTE"))return diretiva_inteiros(n)&&lista_inteiros(n);
 if(in(s,"DIR_ASCII|DIR_ASCIIZ"))return diretiva_string(n);
 if(in(s,"DIR_SPACE|DIR_ALIGN")){leaf(n);return take(n,"NUM_INT");}
 diagnostic("diretiva de dados");return 0;
}
static int linha_dados(int p){int n=node(p,"<linha_dados>");int ok;
 if(is("ID")){ok=rotulo(n);if(ok&&!is("EOL"))ok=diretiva_dados(n);}
 else if(is("DIR_GLOBL"))ok=diretiva_global(n);
 else ok=diretiva_dados(n);
 return ok&&take(n,"EOL");
}
static int linha_codigo(int p){int n=node(p,"<linha_codigo>");int ok;
 if(is("ID")){ok=rotulo(n);if(ok&&!is("EOL"))ok=instrucao(n);}
 else if(is("DIR_GLOBL"))ok=diretiva_global(n);
 else ok=instrucao(n);
 return ok&&take(n,"EOL");
}
static int secao_dados(int p){int n=node(p,"<secao_dados>");int ok=take(n,"DIR_DATA")&&take(n,"EOL");current_section=1;
 if(!ok){recover();return 0;}
 while(!is("TK_EOF")&&!issection()){size_t before=pos;if(!linha_dados(n))recover();if(pos==before&&!is("TK_EOF"))pos++;}return 1;
}
static int secao_codigo(int p){int n=node(p,"<secao_codigo>");int ok=take(n,"DIR_TEXT")&&take(n,"EOL");current_section=2;
 if(!ok){recover();return 0;}
 while(!is("TK_EOF")&&!issection()){size_t before=pos;if(!linha_codigo(n))recover();if(pos==before&&!is("TK_EOF"))pos++;}return 1;
}
static int secao(int p){int n=node(p,"<secao>");if(is("DIR_DATA"))return secao_dados(n);if(is("DIR_TEXT"))return secao_codigo(n);diagnostic("DIR_DATA ou DIR_TEXT");return 0;}
static int programa(int p){int n=node(p,"<programa>");if(!issection()){diagnostic("DIR_DATA ou DIR_TEXT");recover();}
 while(!is("TK_EOF")){if(issection())secao(n);else{diagnostic("inicio de secao .data ou .text");recover();}}
 return take(n,"TK_EOF");
}
static int append(const char *cat,const char *lex,int l,int c){
 if(nt==cap){size_t v=cap?cap*2:256;Tok *p=realloc(ts,v*sizeof(*p));if(!p)return 0;ts=p;cap=v;}
 ts[nt].cat=dupstr(cat);ts[nt].lex=dupstr(lex);if(!ts[nt].cat||!ts[nt].lex)return 0;
 ts[nt].line=l;ts[nt].col=c;nt++;return 1;
}
static int load(FILE *f){
 char *line=NULL;size_t z=0,n=0;int ch,lastline=-1,lastcol=1;
 while(1){
  ch=fgetc(f);
  if(ch!=EOF&&ch!='\n'){
   if(n+2>z){size_t next=z?z*2:256;char *p=realloc(line,next);if(!p){free(line);return 0;}line=p;z=next;}
   line[n++]=(char)ch;continue;
  }
  if(n){
   line[n]='\0';char *p=line,*comma,*end;long l,c;
   if(*p=='<'){
    comma=strstr(p,", ");end=strrchr(p,'>');
    if(comma&&end&&comma<end&&end[1]==' '){
     *comma='\0';*end='\0';char *cat=p+1,*lex=comma+2;
     char *q=end+2;l=strtol(q,&q,10);c=strtol(q,&q,10);
     if(l<1||c<1){free(line);return 0;}
     if(lastline!=-1&&lastline!=-2&&l!=lastline){if(!append("EOL","EOL",lastline,lastcol)){free(line);return 0;}}
     if(strcmp(cat,"TK_EOF")==0){
     if(lastline>=0&&l==lastline&&!append("EOL","EOL",lastline,lastcol)){free(line);return 0;}
     if(!append("TK_EOF","EOF",(int)l,(int)c)){free(line);return 0;}
       lastline=-2;break;
}
     if(!append(cat,lex,(int)l,(int)c)){free(line);return 0;}
     lastline=(int)l;lastcol=(int)(c+strlen(lex));
    }
   }
  }
  n=0;
  if(ch==EOF)break;
 }
 free(line);if(ferror(f))return 0;
 if(lastline>=0&&!append("EOL","EOL",lastline,lastcol))return 0;
 if(nt==0||strcmp(ts[nt-1].cat,"TK_EOF")!=0){if(!append("TK_EOF","EOF",lastline>0?lastline+1:1,1))return 0;}
 return 1;
}
static void dot_quoted(FILE *f,const char *s){int separador=0;fputc('"',f);for(;*s;s++){unsigned char c=(unsigned char)*s;if(c=='\\'&&s[1]=='n'&&!separador){fputs("\\n",f);s++;separador=1;}else if(c=='"'||c=='\\'){fputc('\\',f);fputc(c,f);}else if(c=='\n')fputs("\\n",f);else if(c<32)fprintf(f,"\\x%02X",c);else fputc(c,f);}fputc('"',f);}
static void clearall(void){size_t i;for(i=0;i<nt;i++){free(ts[i].cat);free(ts[i].lex);}free(ts);ts=NULL;nt=cap=pos=0;for(i=0;i<nn;i++)free(ns[i].name);free(ns);ns=NULL;nn=ncap=0;failures=iofail=0;current_section=0;}
int AnaliseSintatica(FILE *tokens,FILE *arvore,FILE *erros){
 clearall();err=erros;if(!load(tokens)){clearall();return -1;}
 programa(-1);
 if(iofail){clearall();return -1;}
 if(failures==0){
  size_t i;fputs("digraph ArvoreSintatica {\n graph [ordering=out];\n node [shape=box,fontname=Arial];\n edge [arrowhead=none];\n",arvore);
  for(i=0;i<nn;i++){fprintf(arvore," n%zu [label=",i);dot_quoted(arvore,ns[i].name);fputs("];\n",arvore);}
  for(i=0;i<nn;i++)if(ns[i].parent>=0)fprintf(arvore," n%d -> n%zu;\n",ns[i].parent,i);
  fputs("}\n",arvore);fputs("Nenhum erro sintatico encontrado.\n",erros);
 }
 int result=failures?1:0;if(ferror(arvore)||ferror(erros))result=-1;clearall();return result;
}
