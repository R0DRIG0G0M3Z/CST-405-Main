.data
newline: .asciiz "\n"

.text
.globl main

main:
    li $v0, 1
    li $a0, 12
    syscall

    li $v0, 4
    la $a0, newline
    syscall

    li $v0, 10
    syscall