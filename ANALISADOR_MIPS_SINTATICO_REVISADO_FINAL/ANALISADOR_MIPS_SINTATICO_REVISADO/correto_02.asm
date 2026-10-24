# Teste de comentarios
.data
texto: .ascii "O # dentro da string nao e comentario"
.text
inicio:
li $v0, 0x10 # comentario removido
syscall
