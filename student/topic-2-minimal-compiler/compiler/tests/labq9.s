.data
newline: .asciiz "\n"

.text
.globl main

main:
    li   $t0, 1
    li   $t1, 2
    add  $t2, $t0, $t1

    add  $t3, $t2, $t2

    li   $t4, 3
    add  $t5, $t2, $t3
    add  $t6, $t5, $t4

    move $a0, $t6
    li   $v0, 1
    syscall

    la   $a0, newline
    li   $v0, 4
    syscall

    li   $v0, 10
    syscall