RISC-V Assembler (CS207 FCS Lab Project)

This is a two-pass assembler for a subset of the RISC-V 64-bit ISA, written in C++. It reads an assembly file (input.asm) and converts it into a machine code file (output.mc), following the memory layout and output format specified by the Venus assembler.

The assembler correctly handles labels, instruction encoding, and data directive.

Features:-
Two-Pass Design: Correctly resolves forward and backward label references for all branch and jump instructions.

C++ Implementation: A clean, single-file C++17 implementation(in main.cpp file)

Detailed Output: Generates machine code, a compressed assembly line, and a detailed 6 or 7-field debug string for each instruction, as specified in the project requirements.

Venus Memory Model: Assumes .text segment starts at 0x00000000 and .data segment starts at 0x10000000.

It supports 37 instructions-
• R format - add, addw, and, or, sll, slt, sra, srl, sub, subw, xor, mul,
mulw, div, divw, rem, remw
• I format - addi, addiw, andi, ori, lb, ld, lh, lw, jalr
• S format - sb, sw, sh, sd
• SB format - beq, bne, bge, blt
• U format - auipc, lui
• UJ format - jal

and 7 assembler directives-
.text, .data, .byte, .half, .word, .dword, .asciz.
