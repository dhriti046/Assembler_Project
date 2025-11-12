# This test file covers all 37 instructions and 7 directives.

.data
    # 1. Test .dword, .word, .half, .byte (with negative numbers) 
    val_d:  .dword  -5000       # Addr: 0x10000000
    val_w:  .word   1000        # Addr: 0x10000008
    val_h:  .half   -2          # Addr: 0x1000000C
    val_b:  .byte   -1          # Addr: 0x1000000E
    # 2. Test .asciz 
    str:    .asciz  "Hello"     # Addr: 0x1000000F

.text
start:
    # 3. Test U-Format (lui, auipc) [cite: 27]
    lui   x5, 0x10000       # x5 = 0x10000000 (data base address)
    auipc x6, 0             # x6 = PC (0x4)

    # 4. Test I-Format (Loads: ld, lw, lh, lb) [cite: 24]
    ld    x7, 0(x5)         # x7 = -5000 (from val_d)
    lw    x8, 8(x5)         # x8 = 1000 (from val_w)
    lh    x9, 12(x5)        # x9 = -2 (from val_h)
    lb    x10, 14(x5)       # x10 = -1 (from val_b)

    # 5. Test I-Format (Arithmetic: addi, addiw, andi, ori) [cite: 24]
    addi  x11, x8, 500      # x11 = 1000 + 500 = 1500
    addiw x12, x8, -1       # x12 = 1000 - 1 = 999 (32-bit)
    andi  x13, x8, 0xFF     # x13 = 1000 & 255 = 232
    ori   x14, x10, 0x1     # x14 = -1 | 1 = -1

    # 6. Test R-Format (All 17 instructions) [cite: 23]
    #    Inputs: x7 (-5000), x8 (1000), x9 (-2), x10 (-1)
    add   x15, x7, x8       # -5000 + 1000 = -4000
    addw  x16, x7, x8       # 32-bit add
    sub   x17, x8, x7       # 1000 - (-5000) = 6000
    subw  x18, x8, x7       # 32-bit sub
    mul   x19, x8, x9       # 1000 * -2 = -2000
    mulw  x20, x8, x9       # 32-bit mul
    div   x21, x7, x8       # -5000 / 1000 = -5
    divw  x22, x7, x8       # 32-bit div
    rem   x23, x8, x10      # 1000 % -1 = 0
    remw  x24, x8, x10      # 32-bit rem
    and   x25, x7, x8       # -5000 & 1000
    or    x26, x7, x8       # -5000 | 1000
    xor   x27, x7, x8       # -5000 ^ 1000
    slt   x28, x7, x8       # (-5000 < 1000) = 1
    sll   x29, x8, x10      # 1000 << -1 (uses last 6 bits, 1000 << 63)
    sra   x30, x7, x9       # -5000 >> -2 (uses last 6 bits, -5000 >> 62)
    srl   x31, x8, x10      # 1000 >> -1 (logical)

    # 7. Test S-Format (sd, sw, sh, sb) [cite: 25]
    sd    x0, 0(x5)         # Store 0 at val_d
    sw    x0, 8(x5)         # Store 0 at val_w
    sh    x0, 12(x5)        # Store 0 at val_h
    sb    x0, 14(x5)        # Store 0 at val_b

    # 8. Test SB-Format (blt, bge, beq, bne) [cite: 26]
    addi  x2, x0, 5
    addi  x3, x0, 0
loop:
    # Test backward branch (blt)
    addi  x3, x3, 1
    blt   x3, x2, loop      # Branch to loop if x3 < 5
    
    # Test forward branch (bge)
    bge   x3, x2, fwd_label # Should branch (5 >= 5)
    add   x0, x0, x0        # Should be skipped
fwd_label:
    beq   x3, x2, do_jal    # Should branch (5 == 5)
    add   x0, x0, x0        # Should be skipped
do_jal:
    bne   x0, x0, end       # Should NOT branch (0 == 0)

    # 9. Test UJ-Format (jal) [cite: 28]
    jal   x1, function      # x1 = PC+4 (0x9C), jump to function
    add   x0, x0, x0        # Should be skipped

    # 10. Test I-Format (jalr) [cite: 24]
function:
    addi  x1, x1, 4         # Adjust return address to skip
    jalr  x0, 0(x1)         # Return
    add   x0, x0, x0        # Should be skipped by jalr
end:
    add   x0, x0, x0        # End of text segment