# teste de dados e alternancia
.data
.globl entrada
entrada: .word 10, 20, -30, 0xFF
v1: .half 1000
v2: .byte 65
msg: .asciiz "Ola, mundo"
msg2: .ascii "ABC"
bloco: .space 100
.align 2
.text
inicio:
li $t0, 10
.data
.text
fim:
nop
