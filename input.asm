.data
# This is just a label pointing to the start of .data
# We will store data starting at 0x10000000
data_start:
    .word 0 # Placeholder

.text
start:
    # Load base address of data segment
    lui   x5, 0x10000       # x5 = 0x10000000

    # Load test values into registers
    addi  x6, x0, 0xAA      # Value for sb (1-byte store)
    addi  x7, x0, 0xBBBB    # Value for sh (2-byte store)
    addi  x8, x0, -1        # Value for sw (4-byte store)
    addi  x9, x0, 0x1234    # Value for sd (8-byte store)
    
    # --- S-Format Test Instructions ---
    # Syntax: instruction rs2, imm(rs1)
    
    # Store 0x1234 (8 bytes) at address 0x10000000
    sd x9, 0(x5)            
    
    # Store -1 (4 bytes) at address 0x10000008
    sw x8, 8(x5)            
    
    # Store 0xBBBB (2 bytes) at address 0x1000000C
    sh x7, 12(x5)           
    
    # Store 0xAA (1 byte) at address 0x1000000F (offset 15)
    sb x6, 15(x5)

    # Test negative offset
    # Store 0xAA (1 byte) at address 0x10000000 - 4 = 0x0FFFFFFF_C
    sb x6, -4(x5)