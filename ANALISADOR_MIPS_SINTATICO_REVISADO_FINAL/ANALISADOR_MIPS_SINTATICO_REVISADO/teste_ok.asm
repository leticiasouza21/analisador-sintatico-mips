.data
msg: .asciiz "Resultado # obtido"
valor: .word 10
.text
main:
li $t0, 5
li $t1, -10
li $s0, 0xFF
add $t2, $t0, $t1
lw $t3, 4($sp)
