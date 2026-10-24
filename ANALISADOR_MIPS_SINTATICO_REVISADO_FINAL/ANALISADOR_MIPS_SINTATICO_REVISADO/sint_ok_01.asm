.text
.globl main
main:
li $t0, 10
li $t1, 0xFF
add $t2, $t0, $t1
addu $t2, $t0, $t1
sub $t2, $t0, $t1
subu $t2, $t0, $t1
mul $t2, $t0, $t1
and $t2, $t0, $t1
or $t2, $t0, $t1
xor $t2, $t0, $t1
nor $t2, $t0, $t1
slt $t2, $t0, $t1
move $t2, $t0
div $t0, $t1
mflo $t0
mfhi $t0
jr $ra
addi $t0, $t1, -1
sll $t0, $t1, 2
srl $t0, $t1, 2
la $t0, destino
lw $t0, destino
sw $t0, 4($sp)
lb $t0, destino
sb $t0, 0($sp)
lh $t0, destino
sh $t0, 8($sp)
beq $t0, $t1, destino
bne $t0, $t1, destino
blt $t0, $t1, destino
ble $t0, $t1, destino
bgt $t0, $t1, destino
bge $t0, $t1, destino
j destino
jal destino
nop
destino: syscall
