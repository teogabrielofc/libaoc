.text
.abicalls
.set noreorder
.globl __start
.globl _start
.ent __start
__start:
_start:
    move    $zero, $ra
    bal     1f
    nop
1:
    .cpload $ra
    move    $ra, $zero

    la      $a0, main
    lw      $a1, 0($sp)
    addiu   $a2, $sp, 4

    li      $t0, -8
    and     $sp, $sp, $t0
    addiu   $sp, $sp, -32

    lw      $a3, %got(_init)($gp)
    lw      $t0, %got(_fini)($gp)
    sw      $t0, 16($sp)
    sw      $v0, 20($sp)
    sw      $sp, 24($sp)

    lw      $t9, %call16(__uClibc_main)($gp)
    jalr    $t9
    nop

2:
    b       2b
    nop
.end __start
