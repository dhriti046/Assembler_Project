.data
 arr: .word 1 2 3
.word 4

 .text
_start:
    auipc x1,0
    addi  x1,x1,5
    auipc x2,0
    addi  x2,x2,8
    lw    x3,0(x2)
    auipc x10,0
    addi  x10,x10,9
    lw    x8,0(x10)
    addi  x4,x0,0

search_loop:
    beq   x4,x3,not_found
    addi  x5,x4,2
    add   x6,x1,x5
    lw    x7,0(x6)
    beq   x7,x8,found
    addi  x4,x4,1
    jal   x0,search_loop

found:
    addi  x9,x4,0
    beq   x0,x0,finish

not_found:
    addi  x9,x0,-1
    beq   x0,x0,finish

finish:
    beq   x0,x0,finish
