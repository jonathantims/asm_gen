/*
 * asm.h
 * Exhaustive MASM x86/x64 register, instruction, directive, and keyword string table.
 * Each symbol exposes the lowercase MASM spelling for a documented token.
 * The instruction groups run through AVX2 and include the LOCK prefix,
 * the interlocked compare-exchange instructions, x87, and SSE through SSE4.2.
 */

/*
 * ============================================================================
 * 1. General Purpose Registers
 * ============================================================================
 */

/*
 * AL - accumulator family
 * AL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the AX/EAX/RAX family for integer return values, arithmetic
 * accumulators, and implicit multiply/divide operands
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov al, 0
 */
static const char* const ASM_AL = "al";

/*
 * AH - accumulator family
 * AH is a 8-bit register operand recognized by MASM.
 * Typical use: the high byte of the low 16 bits of AX/EAX/RAX
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: AH, BH, CH, and DH cannot be encoded in instructions that require a REX prefix in
 * 64-bit mode.
 *
 * Example:
 *   mov ah, 0
 */
static const char* const ASM_AH = "ah";

/*
 * AX - accumulator family
 * AX is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit arithmetic, addressing, or ABI-specific temporary storage related to
 * integer return values, arithmetic accumulators, and implicit multiply/divide operands
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ax, 0
 */
static const char* const ASM_AX = "ax";

/*
 * EAX - accumulator family
 * EAX is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit arithmetic, argument passing, and zero-extending writes into the parent
 * 64-bit register in x64 mode
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov eax, 0
 */
static const char* const ASM_EAX = "eax";

/*
 * RAX - accumulator family
 * RAX is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer operations and address calculations for integer return
 * values, arithmetic accumulators, and implicit multiply/divide operands
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rax, 0
 */
static const char* const ASM_RAX = "rax";

/*
 * BL - base register family
 * BL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the BX/EBX/RBX family for base-address calculations and callee-
 * saved temporaries in common calling conventions
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov bl, 0
 */
static const char* const ASM_BL = "bl";

/*
 * BH - base register family
 * BH is a 8-bit register operand recognized by MASM.
 * Typical use: the high byte of the low 16 bits of BX/EBX/RBX
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: AH, BH, CH, and DH cannot be encoded in instructions that require a REX prefix in
 * 64-bit mode.
 *
 * Example:
 *   mov bh, 0
 */
static const char* const ASM_BH = "bh";

/*
 * BX - base register family
 * BX is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit arithmetic, addressing, or ABI-specific temporary storage related to base-
 * address calculations and callee-saved temporaries in common calling conventions
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov bx, 0
 */
static const char* const ASM_BX = "bx";

/*
 * EBX - base register family
 * EBX is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit arithmetic, argument passing, and zero-extending writes into the parent
 * 64-bit register in x64 mode
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ebx, 0
 */
static const char* const ASM_EBX = "ebx";

/*
 * RBX - base register family
 * RBX is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer operations and address calculations for base-address
 * calculations and callee-saved temporaries in common calling conventions
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rbx, 0
 */
static const char* const ASM_RBX = "rbx";

/*
 * CL - count register family
 * CL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the CX/ECX/RCX family for loop counts, REP-prefixed string
 * counts, shifts that use CL, and volatile argument passing on many ABIs
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov cl, 0
 */
static const char* const ASM_CL = "cl";

/*
 * CH - count register family
 * CH is a 8-bit register operand recognized by MASM.
 * Typical use: the high byte of the low 16 bits of CX/ECX/RCX
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: AH, BH, CH, and DH cannot be encoded in instructions that require a REX prefix in
 * 64-bit mode.
 *
 * Example:
 *   mov ch, 0
 */
static const char* const ASM_CH = "ch";

/*
 * CX - count register family
 * CX is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit arithmetic, addressing, or ABI-specific temporary storage related to loop
 * counts, REP-prefixed string counts, shifts that use CL, and volatile argument passing on many
 * ABIs
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov cx, 0
 */
static const char* const ASM_CX = "cx";

/*
 * ECX - count register family
 * ECX is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit arithmetic, argument passing, and zero-extending writes into the parent
 * 64-bit register in x64 mode
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ecx, 0
 */
static const char* const ASM_ECX = "ecx";

/*
 * RCX - count register family
 * RCX is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer operations and address calculations for loop counts,
 * REP-prefixed string counts, shifts that use CL, and volatile argument passing on many ABIs
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rcx, 0
 */
static const char* const ASM_RCX = "rcx";

/*
 * DL - data register family
 * DL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the DX/EDX/RDX family for high halves of multiply/divide results,
 * I/O port addressing, and volatile argument passing
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov dl, 0
 */
static const char* const ASM_DL = "dl";

/*
 * DH - data register family
 * DH is a 8-bit register operand recognized by MASM.
 * Typical use: the high byte of the low 16 bits of DX/EDX/RDX
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: AH, BH, CH, and DH cannot be encoded in instructions that require a REX prefix in
 * 64-bit mode.
 *
 * Example:
 *   mov dh, 0
 */
static const char* const ASM_DH = "dh";

/*
 * DX - data register family
 * DX is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit arithmetic, addressing, or ABI-specific temporary storage related to high
 * halves of multiply/divide results, I/O port addressing, and volatile argument passing
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov dx, 0
 */
static const char* const ASM_DX = "dx";

/*
 * EDX - data register family
 * EDX is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit arithmetic, argument passing, and zero-extending writes into the parent
 * 64-bit register in x64 mode
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov edx, 0
 */
static const char* const ASM_EDX = "edx";

/*
 * RDX - data register family
 * RDX is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer operations and address calculations for high halves of
 * multiply/divide results, I/O port addressing, and volatile argument passing
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rdx, 0
 */
static const char* const ASM_RDX = "rdx";

/*
 * SPL - stack-pointer family
 * SPL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the SP/ESP/RSP family for stack growth, PUSH/POP/CALL/RET, local
 * storage, and stack-based addressing
 * Relationship: RSP contains ESP, SP, and SPL. Writing ESP in 64-bit mode zero-extends into RSP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: SPL requires a REX prefix in 64-bit mode.
 *
 * Example:
 *   mov spl, 0
 */
static const char* const ASM_SPL = "spl";

/*
 * SP - stack-pointer family
 * SP is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit forms of stack growth, PUSH/POP/CALL/RET, local storage, and stack-based
 * addressing
 * Relationship: RSP contains ESP, SP, and SPL. Writing ESP in 64-bit mode zero-extends into RSP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov sp, 0
 */
static const char* const ASM_SP = "sp";

/*
 * ESP - stack-pointer family
 * ESP is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit forms of stack growth, PUSH/POP/CALL/RET, local storage, and stack-based
 * addressing; writes zero-extend into the 64-bit parent in long mode
 * Relationship: RSP contains ESP, SP, and SPL. Writing ESP in 64-bit mode zero-extends into RSP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov esp, 0
 */
static const char* const ASM_ESP = "esp";

/*
 * RSP - stack-pointer family
 * RSP is a 64-bit register operand recognized by MASM.
 * Typical use: 64-bit forms of stack growth, PUSH/POP/CALL/RET, local storage, and stack-based
 * addressing
 * Relationship: RSP contains ESP, SP, and SPL. Writing ESP in 64-bit mode zero-extends into RSP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rsp, 0
 */
static const char* const ASM_RSP = "rsp";

/*
 * BPL - frame-pointer family
 * BPL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the BP/EBP/RBP family for stack-frame anchoring, local variable
 * addressing, and nonvolatile storage in many calling conventions
 * Relationship: RBP contains EBP, BP, and BPL. Writing EBP in 64-bit mode zero-extends into RBP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: BPL requires a REX prefix in 64-bit mode.
 *
 * Example:
 *   mov bpl, 0
 */
static const char* const ASM_BPL = "bpl";

/*
 * BP - frame-pointer family
 * BP is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit forms of stack-frame anchoring, local variable addressing, and nonvolatile
 * storage in many calling conventions
 * Relationship: RBP contains EBP, BP, and BPL. Writing EBP in 64-bit mode zero-extends into RBP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov bp, 0
 */
static const char* const ASM_BP = "bp";

/*
 * EBP - frame-pointer family
 * EBP is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit forms of stack-frame anchoring, local variable addressing, and nonvolatile
 * storage in many calling conventions; writes zero-extend into the 64-bit parent in long mode
 * Relationship: RBP contains EBP, BP, and BPL. Writing EBP in 64-bit mode zero-extends into RBP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ebp, 0
 */
static const char* const ASM_EBP = "ebp";

/*
 * RBP - frame-pointer family
 * RBP is a 64-bit register operand recognized by MASM.
 * Typical use: 64-bit forms of stack-frame anchoring, local variable addressing, and nonvolatile
 * storage in many calling conventions
 * Relationship: RBP contains EBP, BP, and BPL. Writing EBP in 64-bit mode zero-extends into RBP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rbp, 0
 */
static const char* const ASM_RBP = "rbp";

/*
 * SIL - source-index family
 * SIL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the SI/ESI/RSI family for source addresses for string
 * instructions and general pointer arithmetic
 * Relationship: RSI contains ESI, SI, and SIL. Writing ESI in 64-bit mode zero-extends into RSI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: SIL requires a REX prefix in 64-bit mode.
 *
 * Example:
 *   mov sil, 0
 */
static const char* const ASM_SIL = "sil";

/*
 * SI - source-index family
 * SI is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit forms of source addresses for string instructions and general pointer
 * arithmetic
 * Relationship: RSI contains ESI, SI, and SIL. Writing ESI in 64-bit mode zero-extends into RSI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov si, 0
 */
static const char* const ASM_SI = "si";

/*
 * ESI - source-index family
 * ESI is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit forms of source addresses for string instructions and general pointer
 * arithmetic; writes zero-extend into the 64-bit parent in long mode
 * Relationship: RSI contains ESI, SI, and SIL. Writing ESI in 64-bit mode zero-extends into RSI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov esi, 0
 */
static const char* const ASM_ESI = "esi";

/*
 * RSI - source-index family
 * RSI is a 64-bit register operand recognized by MASM.
 * Typical use: 64-bit forms of source addresses for string instructions and general pointer
 * arithmetic
 * Relationship: RSI contains ESI, SI, and SIL. Writing ESI in 64-bit mode zero-extends into RSI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rsi, 0
 */
static const char* const ASM_RSI = "rsi";

/*
 * DIL - destination-index family
 * DIL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the DI/EDI/RDI family for destination addresses for string
 * instructions and general pointer arithmetic
 * Relationship: RDI contains EDI, DI, and DIL. Writing EDI in 64-bit mode zero-extends into RDI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DIL requires a REX prefix in 64-bit mode.
 *
 * Example:
 *   mov dil, 0
 */
static const char* const ASM_DIL = "dil";

/*
 * DI - destination-index family
 * DI is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit forms of destination addresses for string instructions and general pointer
 * arithmetic
 * Relationship: RDI contains EDI, DI, and DIL. Writing EDI in 64-bit mode zero-extends into RDI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov di, 0
 */
static const char* const ASM_DI = "di";

/*
 * EDI - destination-index family
 * EDI is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit forms of destination addresses for string instructions and general pointer
 * arithmetic; writes zero-extend into the 64-bit parent in long mode
 * Relationship: RDI contains EDI, DI, and DIL. Writing EDI in 64-bit mode zero-extends into RDI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov edi, 0
 */
static const char* const ASM_EDI = "edi";

/*
 * RDI - destination-index family
 * RDI is a 64-bit register operand recognized by MASM.
 * Typical use: 64-bit forms of destination addresses for string instructions and general pointer
 * arithmetic
 * Relationship: RDI contains EDI, DI, and DIL. Writing EDI in 64-bit mode zero-extends into RDI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rdi, 0
 */
static const char* const ASM_RDI = "rdi";

/*
 * R8B - extended general-purpose register family R8
 * R8B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R8 register for general integer temporaries; on
 * Win64, R8 and R9 also carry the third and fourth integer or pointer arguments
 * Relationship: R8 contains R8D, R8W, and R8B. Writing R8D in 64-bit mode zero-extends into R8.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r8b, 0
 */
static const char* const ASM_R8B = "r8b";

/*
 * R8W - extended general-purpose register family R8
 * R8W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R8 for address and integer work
 * Relationship: R8 contains R8D, R8W, and R8B. Writing R8D in 64-bit mode zero-extends into R8.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r8w, 0
 */
static const char* const ASM_R8W = "r8w";

/*
 * R8D - extended general-purpose register family R8
 * R8D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R8; writes zero-extend into the parent register
 * Relationship: R8 contains R8D, R8W, and R8B. Writing R8D in 64-bit mode zero-extends into R8.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r8d, 0
 */
static const char* const ASM_R8D = "r8d";

/*
 * R8 - extended general-purpose register family R8
 * R8 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R8
 * Relationship: R8 contains R8D, R8W, and R8B. Writing R8D in 64-bit mode zero-extends into R8.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r8, 0
 */
static const char* const ASM_R8 = "r8";

/*
 * R9B - extended general-purpose register family R9
 * R9B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R9 register for general integer temporaries; on
 * Win64, R8 and R9 also carry the third and fourth integer or pointer arguments
 * Relationship: R9 contains R9D, R9W, and R9B. Writing R9D in 64-bit mode zero-extends into R9.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r9b, 0
 */
static const char* const ASM_R9B = "r9b";

/*
 * R9W - extended general-purpose register family R9
 * R9W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R9 for address and integer work
 * Relationship: R9 contains R9D, R9W, and R9B. Writing R9D in 64-bit mode zero-extends into R9.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r9w, 0
 */
static const char* const ASM_R9W = "r9w";

/*
 * R9D - extended general-purpose register family R9
 * R9D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R9; writes zero-extend into the parent register
 * Relationship: R9 contains R9D, R9W, and R9B. Writing R9D in 64-bit mode zero-extends into R9.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r9d, 0
 */
static const char* const ASM_R9D = "r9d";

/*
 * R9 - extended general-purpose register family R9
 * R9 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R9
 * Relationship: R9 contains R9D, R9W, and R9B. Writing R9D in 64-bit mode zero-extends into R9.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r9, 0
 */
static const char* const ASM_R9 = "r9";

/*
 * R10B - extended general-purpose register family R10
 * R10B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R10 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R10 contains R10D, R10W, and R10B. Writing R10D in 64-bit mode zero-extends into
 * R10.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r10b, 0
 */
static const char* const ASM_R10B = "r10b";

/*
 * R10W - extended general-purpose register family R10
 * R10W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R10 for address and integer work
 * Relationship: R10 contains R10D, R10W, and R10B. Writing R10D in 64-bit mode zero-extends into
 * R10.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r10w, 0
 */
static const char* const ASM_R10W = "r10w";

/*
 * R10D - extended general-purpose register family R10
 * R10D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R10; writes zero-extend into the parent register
 * Relationship: R10 contains R10D, R10W, and R10B. Writing R10D in 64-bit mode zero-extends into
 * R10.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r10d, 0
 */
static const char* const ASM_R10D = "r10d";

/*
 * R10 - extended general-purpose register family R10
 * R10 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R10
 * Relationship: R10 contains R10D, R10W, and R10B. Writing R10D in 64-bit mode zero-extends into
 * R10.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r10, 0
 */
static const char* const ASM_R10 = "r10";

/*
 * R11B - extended general-purpose register family R11
 * R11B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R11 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R11 contains R11D, R11W, and R11B. Writing R11D in 64-bit mode zero-extends into
 * R11.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r11b, 0
 */
static const char* const ASM_R11B = "r11b";

/*
 * R11W - extended general-purpose register family R11
 * R11W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R11 for address and integer work
 * Relationship: R11 contains R11D, R11W, and R11B. Writing R11D in 64-bit mode zero-extends into
 * R11.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r11w, 0
 */
static const char* const ASM_R11W = "r11w";

/*
 * R11D - extended general-purpose register family R11
 * R11D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R11; writes zero-extend into the parent register
 * Relationship: R11 contains R11D, R11W, and R11B. Writing R11D in 64-bit mode zero-extends into
 * R11.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r11d, 0
 */
static const char* const ASM_R11D = "r11d";

/*
 * R11 - extended general-purpose register family R11
 * R11 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R11
 * Relationship: R11 contains R11D, R11W, and R11B. Writing R11D in 64-bit mode zero-extends into
 * R11.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r11, 0
 */
static const char* const ASM_R11 = "r11";

/*
 * R12B - extended general-purpose register family R12
 * R12B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R12 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R12 contains R12D, R12W, and R12B. Writing R12D in 64-bit mode zero-extends into
 * R12.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r12b, 0
 */
static const char* const ASM_R12B = "r12b";

/*
 * R12W - extended general-purpose register family R12
 * R12W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R12 for address and integer work
 * Relationship: R12 contains R12D, R12W, and R12B. Writing R12D in 64-bit mode zero-extends into
 * R12.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r12w, 0
 */
static const char* const ASM_R12W = "r12w";

/*
 * R12D - extended general-purpose register family R12
 * R12D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R12; writes zero-extend into the parent register
 * Relationship: R12 contains R12D, R12W, and R12B. Writing R12D in 64-bit mode zero-extends into
 * R12.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r12d, 0
 */
static const char* const ASM_R12D = "r12d";

/*
 * R12 - extended general-purpose register family R12
 * R12 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R12
 * Relationship: R12 contains R12D, R12W, and R12B. Writing R12D in 64-bit mode zero-extends into
 * R12.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r12, 0
 */
static const char* const ASM_R12 = "r12";

/*
 * R13B - extended general-purpose register family R13
 * R13B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R13 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R13 contains R13D, R13W, and R13B. Writing R13D in 64-bit mode zero-extends into
 * R13.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r13b, 0
 */
static const char* const ASM_R13B = "r13b";

/*
 * R13W - extended general-purpose register family R13
 * R13W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R13 for address and integer work
 * Relationship: R13 contains R13D, R13W, and R13B. Writing R13D in 64-bit mode zero-extends into
 * R13.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r13w, 0
 */
static const char* const ASM_R13W = "r13w";

/*
 * R13D - extended general-purpose register family R13
 * R13D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R13; writes zero-extend into the parent register
 * Relationship: R13 contains R13D, R13W, and R13B. Writing R13D in 64-bit mode zero-extends into
 * R13.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r13d, 0
 */
static const char* const ASM_R13D = "r13d";

/*
 * R13 - extended general-purpose register family R13
 * R13 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R13
 * Relationship: R13 contains R13D, R13W, and R13B. Writing R13D in 64-bit mode zero-extends into
 * R13.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r13, 0
 */
static const char* const ASM_R13 = "r13";

/*
 * R14B - extended general-purpose register family R14
 * R14B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R14 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R14 contains R14D, R14W, and R14B. Writing R14D in 64-bit mode zero-extends into
 * R14.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r14b, 0
 */
static const char* const ASM_R14B = "r14b";

/*
 * R14W - extended general-purpose register family R14
 * R14W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R14 for address and integer work
 * Relationship: R14 contains R14D, R14W, and R14B. Writing R14D in 64-bit mode zero-extends into
 * R14.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r14w, 0
 */
static const char* const ASM_R14W = "r14w";

/*
 * R14D - extended general-purpose register family R14
 * R14D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R14; writes zero-extend into the parent register
 * Relationship: R14 contains R14D, R14W, and R14B. Writing R14D in 64-bit mode zero-extends into
 * R14.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r14d, 0
 */
static const char* const ASM_R14D = "r14d";

/*
 * R14 - extended general-purpose register family R14
 * R14 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R14
 * Relationship: R14 contains R14D, R14W, and R14B. Writing R14D in 64-bit mode zero-extends into
 * R14.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r14, 0
 */
static const char* const ASM_R14 = "r14";

/*
 * R15B - extended general-purpose register family R15
 * R15B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R15 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R15 contains R15D, R15W, and R15B. Writing R15D in 64-bit mode zero-extends into
 * R15.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r15b, 0
 */
static const char* const ASM_R15B = "r15b";

/*
 * R15W - extended general-purpose register family R15
 * R15W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R15 for address and integer work
 * Relationship: R15 contains R15D, R15W, and R15B. Writing R15D in 64-bit mode zero-extends into
 * R15.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r15w, 0
 */
static const char* const ASM_R15W = "r15w";

/*
 * R15D - extended general-purpose register family R15
 * R15D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R15; writes zero-extend into the parent register
 * Relationship: R15 contains R15D, R15W, and R15B. Writing R15D in 64-bit mode zero-extends into
 * R15.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r15d, 0
 */
static const char* const ASM_R15D = "r15d";

/*
 * R15 - extended general-purpose register family R15
 * R15 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R15
 * Relationship: R15 contains R15D, R15W, and R15B. Writing R15D in 64-bit mode zero-extends into
 * R15.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r15, 0
 */
static const char* const ASM_R15 = "r15";

/*
 * ============================================================================
 * 2. Segment Registers
 * ============================================================================
 */

/*
 * CS - Code Segment
 * CS is a 16-bit register operand recognized by MASM.
 * Typical use: selects the current code segment in segmented modes and pairs with RIP/EIP/IP for
 * instruction fetch
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CS is generally changed by far control transfers, privilege transitions, or system
 * mechanisms rather than by ordinary arithmetic code. There is no `mov cs, imm`. Read the
 * selector with `mov ax, cs`. The same rule applies to the other segment registers below:
 * copy through a general register (`mov ax, cs` / `mov ds, ax`), never an immediate store.
 *
 * Example:
 *   mov ax, cs
 */
static const char* const ASM_CS = "cs";

/*
 * DS - Data Segment
 * DS is a 16-bit register operand recognized by MASM.
 * Typical use: provides the default data segment for many legacy memory references
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: In flat 32-bit or 64-bit models, DS usually holds a flat selector and is less visible
 * in source code.
 *
 * Example:
 *   mov ax, cs
 *   mov ds, ax
 */
static const char* const ASM_DS = "ds";

/*
 * ES - Extra Segment
 * ES is a 16-bit register operand recognized by MASM.
 * Typical use: historically supplies the destination segment for string operations such as MOVS,
 * STOS, SCAS, and CMPS
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Many string forms implicitly use ES for the destination in legacy modes.
 *
 * Example:
 *   mov ax, ds
 *   mov es, ax
 */
static const char* const ASM_ES = "es";

/*
 * FS - FS Segment
 * FS is a 16-bit register operand recognized by MASM.
 * Typical use: often points at thread-local or processor-local data structures
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: On Windows, FS is commonly used for the TEB in 32-bit mode.
 *
 * Example:
 *   mov eax, fs:[18h]
 */
static const char* const ASM_FS = "fs";

/*
 * GS - GS Segment
 * GS is a 16-bit register operand recognized by MASM.
 * Typical use: often points at thread-local or processor-local data structures in long mode
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: On Windows x64, GS commonly references the TEB/PEB chain and kernel swapgs state.
 *
 * Example:
 *   mov rax, gs:[60h]
 */
static const char* const ASM_GS = "gs";

/*
 * SS - Stack Segment
 * SS is a 16-bit register operand recognized by MASM.
 * Typical use: defines the stack segment used by stack references in segmented execution modes
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Changing SS has ordering and interrupt-shadow rules on x86 and is generally privileged
 * or carefully controlled.
 *
 * Example:
 *   mov ax, ss
 */
static const char* const ASM_SS = "ss";

/*
 * ============================================================================
 * 3. Special Registers
 * ============================================================================
 */

/*
 * RIP - Instruction Pointer
 * RIP is a 64-bit register operand recognized by MASM.
 * Typical use: holds the address of the next instruction in 64-bit mode and supports RIP-
 * relative addressing for position-independent code
 * Relationship: RIP is updated implicitly by sequential execution, branches, calls, returns,
 * interrupts, and exceptions.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: You cannot encode RIP as a general destination register; it is referenced implicitly or
 * through RIP-relative memory operands.
 *
 * Example:
 *   lea rax, [rip+label]
 */
static const char* const ASM_RIP = "rip";

/*
 * EIP - Instruction Pointer
 * EIP is a 32-bit register operand recognized by MASM.
 * Typical use: holds the next instruction address in 32-bit modes
 * Relationship: EIP is modified by control-transfer instructions rather than ordinary MOV
 * instructions.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   call next
 * next:
 *   pop eax
 */
static const char* const ASM_EIP = "eip";

/*
 * IP - Instruction Pointer
 * IP is a 16-bit register operand recognized by MASM.
 * Typical use: holds the next instruction offset in 16-bit modes
 * Relationship: IP pairs with CS in segmented addressing.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   call next
 * next:
 *   pop ax
 */
static const char* const ASM_IP = "ip";

/*
 * RFLAGS - Flags Register
 * RFLAGS is a 64-bit register operand recognized by MASM.
 * Typical use: contains status flags, control flags, and system flags for 64-bit execution
 * Relationship: RFLAGS extends EFLAGS/FLAGS; arithmetic instructions update status bits such as
 * CF, ZF, SF, OF, AF, and PF.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Access is usually via PUSHFQ/POPFQ, LAHF/SAHF, or privileged mechanisms rather than
 * direct MOV.
 *
 * Example:
 *   pushfq
 *   pop rax
 */
static const char* const ASM_RFLAGS = "rflags";

/*
 * EFLAGS - Flags Register
 * EFLAGS is a 32-bit register operand recognized by MASM.
 * Typical use: contains status and control flags in 32-bit modes
 * Relationship: EFLAGS is the low 32 bits of RFLAGS.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   pushfd
 *   pop eax
 */
static const char* const ASM_EFLAGS = "eflags";

/*
 * FLAGS - Flags Register
 * FLAGS is a 16-bit register operand recognized by MASM.
 * Typical use: contains the architectural low 16 bits of the flags register in 16-bit code
 * Relationship: FLAGS is the low 16-bit subset of EFLAGS/RFLAGS.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   pushf
 *   pop ax
 */
static const char* const ASM_FLAGS = "flags";

/*
 * CR0 - Control Register
 * CR0 is a 64-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR0 enables and reports major processor modes such as protected mode, paging, task
 * switching state, and cache behavior.
 *
 * Example:
 *   mov rax, cr0
 */
static const char* const ASM_CR0 = "cr0";

/*
 * CR1 - Control Register
 * CR1 is a 32-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR1 is reserved on modern x86 CPUs and is not used as a general architectural control
 * register.
 *
 * Example:
 *   mov rax, cr0
 */
static const char* const ASM_CR1 = "cr1";

/*
 * CR2 - Control Register
 * CR2 is a 64-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR2 receives the linear address that caused the most recent page fault.
 *
 * Example:
 *   mov rax, cr2
 */
static const char* const ASM_CR2 = "cr2";

/*
 * CR3 - Control Register
 * CR3 is a 64-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR3 holds the page-table root for paging hardware; on PCID-capable CPUs it also carries
 * process-context identifiers.
 *
 * Example:
 *   mov rax, cr3
 */
static const char* const ASM_CR3 = "cr3";

/*
 * CR4 - Control Register
 * CR4 is a 64-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR4 enables optional architectural features such as PAE, PGE, OSFXSR, SMEP, SMAP, and
 * more.
 *
 * Example:
 *   mov rax, cr4
 */
static const char* const ASM_CR4 = "cr4";

/*
 * DR0 - Debug Register
 * DR0 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR0 holds a linear address for one of the hardware breakpoint slots.
 *
 * Example:
 *   mov dr0, rax
 */
static const char* const ASM_DR0 = "dr0";

/*
 * DR1 - Debug Register
 * DR1 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR1 holds a linear address for one of the hardware breakpoint slots.
 *
 * Example:
 *   mov dr1, rax
 */
static const char* const ASM_DR1 = "dr1";

/*
 * DR2 - Debug Register
 * DR2 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR2 holds a linear address for one of the hardware breakpoint slots.
 *
 * Example:
 *   mov dr2, rax
 */
static const char* const ASM_DR2 = "dr2";

/*
 * DR3 - Debug Register
 * DR3 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR3 holds a linear address for one of the hardware breakpoint slots.
 *
 * Example:
 *   mov dr3, rax
 */
static const char* const ASM_DR3 = "dr3";

/*
 * DR4 - Debug Register
 * DR4 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR4 is obsolete and may alias DR6 on old processors when debug extensions are disabled;
 * modern software should avoid it.
 *
 * Example:
 *   mov rax, dr6
 */
static const char* const ASM_DR4 = "dr4";

/*
 * DR5 - Debug Register
 * DR5 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR5 is obsolete and may alias DR7 on old processors when debug extensions are disabled;
 * modern software should avoid it.
 *
 * Example:
 *   mov rax, dr6
 */
static const char* const ASM_DR5 = "dr5";

/*
 * DR6 - Debug Register
 * DR6 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR6 reports debug-status results such as which breakpoint fired.
 *
 * Example:
 *   mov rax, dr6
 */
static const char* const ASM_DR6 = "dr6";

/*
 * DR7 - Debug Register
 * DR7 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR7 controls breakpoint enables, trigger types, and sizes for the hardware debug
 * registers.
 *
 * Example:
 *   mov dr7, rax
 */
static const char* const ASM_DR7 = "dr7";

/*
 * ============================================================================
 * 4. FPU Registers
 * ============================================================================
 *
 * x87 is the original Intel floating-point unit: the 8087 coprocessor and its
 * descendants. The name is the instruction family's name, not a separate spec from
 * IEEE 754. Modern x87 implements IEEE 754, but it keeps its own 80-bit registers.
 *
 * The register file is a stack of eight registers, ST(0) through ST(7), not a
 * general-purpose file and not the call stack. ST(0) is the top. A load pushes and
 * makes the new value ST(0); a store-and-pop consumes ST(0). Most instructions
 * operate on ST(0) and optionally one other slot. FXCH swaps ST(0) with another
 * slot so an instruction that only sees the top can reach a buried value. FINIT
 * and FNINIT empty the stack and restore the default control word.
 *
 * The status word holds the stack top (TOP, a 3-bit index), the condition codes
 * C0-C3, and the exception flags. The control word holds the rounding mode, the
 * precision (24, 53, or 64-bit significand), and the exception masks. Tag bits say
 * whether each slot is valid, zero, special, or empty.
 *
 * Free-standing x87 code is uncommon in new Windows x64 programs. The Windows x64
 * ABI passes and returns float and double in XMM0-XMM5 (SSE2), not on this stack.
 * x87 is still present, still used by some 32-bit code, by 80-bit long double, and
 * by old binaries. The instructions remain valid in 64-bit mode. See section 18
 * for how the instructions use this stack, including NaN, remainder, and #NM.
 *
 * There is no `mov st(i), imm`. Load with FLD or FILD. Swap with FXCH.
 */
/*
 * ST0 - x87 floating-point stack register
 * ST0 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(0) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   fld qword ptr [rsi]
 */
static const char* const ASM_ST0 = "st(0)";

/*
 * ST1 - x87 floating-point stack register
 * ST1 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(1) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   fxch st(1)
 */
static const char* const ASM_ST1 = "st(1)";

/*
 * ST2 - x87 floating-point stack register
 * ST2 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(2) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   fxch st(2)
 */
static const char* const ASM_ST2 = "st(2)";

/*
 * ST3 - x87 floating-point stack register
 * ST3 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(3) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   fxch st(3)
 */
static const char* const ASM_ST3 = "st(3)";

/*
 * ST4 - x87 floating-point stack register
 * ST4 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(4) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   fxch st(4)
 */
static const char* const ASM_ST4 = "st(4)";

/*
 * ST5 - x87 floating-point stack register
 * ST5 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(5) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   fxch st(5)
 */
static const char* const ASM_ST5 = "st(5)";

/*
 * ST6 - x87 floating-point stack register
 * ST6 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(6) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   fxch st(6)
 */
static const char* const ASM_ST6 = "st(6)";

/*
 * ST7 - x87 floating-point stack register
 * ST7 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(7) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   fxch st(7)
 */
static const char* const ASM_ST7 = "st(7)";

/*
 * ============================================================================
 * 5. SIMD Registers
 * ============================================================================
 *
 * XMM registers are 16 bytes. YMM registers are 32 bytes and share their low 16
 * bytes with the matching XMM register. There is no `mov xmm0, 0`. Zero with
 * `pxor xmm0, xmm0` or `vxorps`, and load with MOVAPS, MOVUPS, or VMOVAPS.
 * Windows x64 uses XMM0-XMM5 as volatile for float and double arguments.
 * MM0-MM7 follow the YMM block. They are the 64-bit MMX registers, aliased
 * onto the x87 file, not onto XMM. SSE3 through SSE4.2 use these same XMM
 * registers; those instructions follow the SSE2 entries in section 19.
 */

/*
 * XMM0 - SIMD vector register
 * XMM0 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM0 is the low 128-bit lane of YMM0. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm0, xmmword ptr [rsi]
 */
static const char* const ASM_XMM0 = "xmm0";

/*
 * XMM1 - SIMD vector register
 * XMM1 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM1 is the low 128-bit lane of YMM1. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm1, xmmword ptr [rsi]
 */
static const char* const ASM_XMM1 = "xmm1";

/*
 * XMM2 - SIMD vector register
 * XMM2 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM2 is the low 128-bit lane of YMM2. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm2, xmmword ptr [rsi]
 */
static const char* const ASM_XMM2 = "xmm2";

/*
 * XMM3 - SIMD vector register
 * XMM3 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM3 is the low 128-bit lane of YMM3. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm3, xmmword ptr [rsi]
 */
static const char* const ASM_XMM3 = "xmm3";

/*
 * XMM4 - SIMD vector register
 * XMM4 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM4 is the low 128-bit lane of YMM4. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm4, xmmword ptr [rsi]
 */
static const char* const ASM_XMM4 = "xmm4";

/*
 * XMM5 - SIMD vector register
 * XMM5 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM5 is the low 128-bit lane of YMM5. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm5, xmmword ptr [rsi]
 */
static const char* const ASM_XMM5 = "xmm5";

/*
 * XMM6 - SIMD vector register
 * XMM6 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM6 is the low 128-bit lane of YMM6. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm6, xmmword ptr [rsi]
 */
static const char* const ASM_XMM6 = "xmm6";

/*
 * XMM7 - SIMD vector register
 * XMM7 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM7 is the low 128-bit lane of YMM7. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm7, xmmword ptr [rsi]
 */
static const char* const ASM_XMM7 = "xmm7";

/*
 * XMM8 - SIMD vector register
 * XMM8 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM8 is the low 128-bit lane of YMM8. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm8, xmmword ptr [rsi]
 */
static const char* const ASM_XMM8 = "xmm8";

/*
 * XMM9 - SIMD vector register
 * XMM9 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM9 is the low 128-bit lane of YMM9. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm9, xmmword ptr [rsi]
 */
static const char* const ASM_XMM9 = "xmm9";

/*
 * XMM10 - SIMD vector register
 * XMM10 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM10 is the low 128-bit lane of YMM10. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm10, xmmword ptr [rsi]
 */
static const char* const ASM_XMM10 = "xmm10";

/*
 * XMM11 - SIMD vector register
 * XMM11 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM11 is the low 128-bit lane of YMM11. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm11, xmmword ptr [rsi]
 */
static const char* const ASM_XMM11 = "xmm11";

/*
 * XMM12 - SIMD vector register
 * XMM12 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM12 is the low 128-bit lane of YMM12. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm12, xmmword ptr [rsi]
 */
static const char* const ASM_XMM12 = "xmm12";

/*
 * XMM13 - SIMD vector register
 * XMM13 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM13 is the low 128-bit lane of YMM13. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm13, xmmword ptr [rsi]
 */
static const char* const ASM_XMM13 = "xmm13";

/*
 * XMM14 - SIMD vector register
 * XMM14 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM14 is the low 128-bit lane of YMM14. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm14, xmmword ptr [rsi]
 */
static const char* const ASM_XMM14 = "xmm14";

/*
 * XMM15 - SIMD vector register
 * XMM15 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM15 is the low 128-bit lane of YMM15. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   movaps xmm15, xmmword ptr [rsi]
 */
static const char* const ASM_XMM15 = "xmm15";

/*
 * YMM0 - AVX vector register
 * YMM0 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM0 extends XMM0; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm0, ymmword ptr [rsi]
 */
static const char* const ASM_YMM0 = "ymm0";

/*
 * YMM1 - AVX vector register
 * YMM1 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM1 extends XMM1; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm1, ymmword ptr [rsi]
 */
static const char* const ASM_YMM1 = "ymm1";

/*
 * YMM2 - AVX vector register
 * YMM2 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM2 extends XMM2; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm2, ymmword ptr [rsi]
 */
static const char* const ASM_YMM2 = "ymm2";

/*
 * YMM3 - AVX vector register
 * YMM3 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM3 extends XMM3; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm3, ymmword ptr [rsi]
 */
static const char* const ASM_YMM3 = "ymm3";

/*
 * YMM4 - AVX vector register
 * YMM4 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM4 extends XMM4; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm4, ymmword ptr [rsi]
 */
static const char* const ASM_YMM4 = "ymm4";

/*
 * YMM5 - AVX vector register
 * YMM5 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM5 extends XMM5; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm5, ymmword ptr [rsi]
 */
static const char* const ASM_YMM5 = "ymm5";

/*
 * YMM6 - AVX vector register
 * YMM6 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM6 extends XMM6; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm6, ymmword ptr [rsi]
 */
static const char* const ASM_YMM6 = "ymm6";

/*
 * YMM7 - AVX vector register
 * YMM7 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM7 extends XMM7; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm7, ymmword ptr [rsi]
 */
static const char* const ASM_YMM7 = "ymm7";

/*
 * YMM8 - AVX vector register
 * YMM8 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM8 extends XMM8; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm8, ymmword ptr [rsi]
 */
static const char* const ASM_YMM8 = "ymm8";

/*
 * YMM9 - AVX vector register
 * YMM9 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM9 extends XMM9; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm9, ymmword ptr [rsi]
 */
static const char* const ASM_YMM9 = "ymm9";

/*
 * YMM10 - AVX vector register
 * YMM10 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM10 extends XMM10; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm10, ymmword ptr [rsi]
 */
static const char* const ASM_YMM10 = "ymm10";

/*
 * YMM11 - AVX vector register
 * YMM11 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM11 extends XMM11; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm11, ymmword ptr [rsi]
 */
static const char* const ASM_YMM11 = "ymm11";

/*
 * YMM12 - AVX vector register
 * YMM12 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM12 extends XMM12; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm12, ymmword ptr [rsi]
 */
static const char* const ASM_YMM12 = "ymm12";

/*
 * YMM13 - AVX vector register
 * YMM13 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM13 extends XMM13; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm13, ymmword ptr [rsi]
 */
static const char* const ASM_YMM13 = "ymm13";

/*
 * YMM14 - AVX vector register
 * YMM14 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM14 extends XMM14; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm14, ymmword ptr [rsi]
 */
static const char* const ASM_YMM14 = "ymm14";

/*
 * YMM15 - AVX vector register
 * YMM15 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM15 extends XMM15; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   vmovaps ymm15, ymmword ptr [rsi]
 */
static const char* const ASM_YMM15 = "ymm15";

/*
 * MM0 through MM7 are the MMX registers. Each is 64 bits and is aliased onto
 * one x87 data register, not onto XMM. An MMX write marks the corresponding
 * x87 tag valid, so x87 instructions after MMX see a dirty stack until EMMS.
 * New Windows x64 code uses XMM. These names are here because MASM still
 * accepts them and the .MMX directive still selects them.
 */

/*
 * MM0 - MMX register
 * MM0 is a 64-bit MMX register operand recognized by MASM.
 *
 * Syntax: mm0
 * Operands: Use the register name directly as an operand.
 * Flags affected: None directly. Any flag changes come from the instruction that reads or
 * writes the register.
 * Notes: MM0 aliases an x87 data register. After an MMX instruction, EMMS (or an x87 FINIT) is
 * required before further x87 use. XMM0 is a different register.
 *
 * Example:
 *   movq mm0, qword ptr [rsi]
 */
static const char* const ASM_MM0 = "mm0";

/*
 * MM1 - MMX register
 * MM1 is a 64-bit MMX register operand recognized by MASM.
 *
 * Syntax: mm1
 * Operands: Use the register name directly as an operand.
 * Flags affected: None directly. Any flag changes come from the instruction that reads or
 * writes the register.
 * Notes: MM1 aliases an x87 data register. After an MMX instruction, EMMS (or an x87 FINIT) is
 * required before further x87 use. XMM1 is a different register.
 *
 * Example:
 *   movq mm1, qword ptr [rsi]
 */
static const char* const ASM_MM1 = "mm1";

/*
 * MM2 - MMX register
 * MM2 is a 64-bit MMX register operand recognized by MASM.
 *
 * Syntax: mm2
 * Operands: Use the register name directly as an operand.
 * Flags affected: None directly. Any flag changes come from the instruction that reads or
 * writes the register.
 * Notes: MM2 aliases an x87 data register. After an MMX instruction, EMMS (or an x87 FINIT) is
 * required before further x87 use. XMM2 is a different register.
 *
 * Example:
 *   movq mm2, qword ptr [rsi]
 */
static const char* const ASM_MM2 = "mm2";

/*
 * MM3 - MMX register
 * MM3 is a 64-bit MMX register operand recognized by MASM.
 *
 * Syntax: mm3
 * Operands: Use the register name directly as an operand.
 * Flags affected: None directly. Any flag changes come from the instruction that reads or
 * writes the register.
 * Notes: MM3 aliases an x87 data register. After an MMX instruction, EMMS (or an x87 FINIT) is
 * required before further x87 use. XMM3 is a different register.
 *
 * Example:
 *   movq mm3, qword ptr [rsi]
 */
static const char* const ASM_MM3 = "mm3";

/*
 * MM4 - MMX register
 * MM4 is a 64-bit MMX register operand recognized by MASM.
 *
 * Syntax: mm4
 * Operands: Use the register name directly as an operand.
 * Flags affected: None directly. Any flag changes come from the instruction that reads or
 * writes the register.
 * Notes: MM4 aliases an x87 data register. After an MMX instruction, EMMS (or an x87 FINIT) is
 * required before further x87 use. XMM4 is a different register.
 *
 * Example:
 *   movq mm4, qword ptr [rsi]
 */
static const char* const ASM_MM4 = "mm4";

/*
 * MM5 - MMX register
 * MM5 is a 64-bit MMX register operand recognized by MASM.
 *
 * Syntax: mm5
 * Operands: Use the register name directly as an operand.
 * Flags affected: None directly. Any flag changes come from the instruction that reads or
 * writes the register.
 * Notes: MM5 aliases an x87 data register. After an MMX instruction, EMMS (or an x87 FINIT) is
 * required before further x87 use. XMM5 is a different register.
 *
 * Example:
 *   movq mm5, qword ptr [rsi]
 */
static const char* const ASM_MM5 = "mm5";

/*
 * MM6 - MMX register
 * MM6 is a 64-bit MMX register operand recognized by MASM.
 *
 * Syntax: mm6
 * Operands: Use the register name directly as an operand.
 * Flags affected: None directly. Any flag changes come from the instruction that reads or
 * writes the register.
 * Notes: MM6 aliases an x87 data register. After an MMX instruction, EMMS (or an x87 FINIT) is
 * required before further x87 use. XMM6 is a different register.
 *
 * Example:
 *   movq mm6, qword ptr [rsi]
 */
static const char* const ASM_MM6 = "mm6";

/*
 * MM7 - MMX register
 * MM7 is a 64-bit MMX register operand recognized by MASM.
 *
 * Syntax: mm7
 * Operands: Use the register name directly as an operand.
 * Flags affected: None directly. Any flag changes come from the instruction that reads or
 * writes the register.
 * Notes: MM7 aliases an x87 data register. After an MMX instruction, EMMS (or an x87 FINIT) is
 * required before further x87 use. XMM7 is a different register.
 *
 * Example:
 *   movq mm7, qword ptr [rsi]
 */
static const char* const ASM_MM7 = "mm7";

/*
 * ============================================================================
 * 6. Data Movement Instructions
 * ============================================================================
 */

/*
 * MOV - Move Data
 * Copies the source operand into the destination operand without memory-to-memory transfer and
 * without altering status flags.
 *
 * Syntax: MOV dest, src
 * Operands: destination register or memory, source register, memory, immediate, or certain
 * special-register forms
 * Flags affected: None
 * Notes: Operands must have compatible sizes. Segment, control, and debug-register forms have
 * architectural restrictions. MASM selects the proper encoding from operand types.
 *
 * Example:
 *   mov eax, 42
 *   mov [rbp-4], eax
 */
static const char* const ASM_MOV = "mov";

/*
 * MOVSX - Move with Sign Extension
 * Copies a smaller signed source into a larger destination and sign-extends the value to fill
 * the destination width.
 *
 * Syntax: MOVSX reg, r/m8|r/m16
 * Operands: destination register larger than the source; source register or memory
 * Flags affected: None
 * Notes: Useful when promoting signed byte or word values before arithmetic or comparisons.
 *
 * Example:
 *   movsx eax, byte ptr [rsi]
 *   movsx rdx, word ptr [rbx]
 */
static const char* const ASM_MOVSX = "movsx";

/*
 * MOVZX - Move with Zero Extension
 * Copies a smaller unsigned source into a larger destination and fills the new high bits with
 * zero.
 *
 * Syntax: MOVZX reg, r/m8|r/m16
 * Operands: destination register larger than the source; source register or memory
 * Flags affected: None
 * Notes: Use MOVZX when the source should be treated as an unsigned byte or word.
 *
 * Example:
 *   movzx eax, byte ptr [rsi]
 *   movzx r8d, word ptr [rbx]
 */
static const char* const ASM_MOVZX = "movzx";

/*
 * MOVSXD - Move with Sign Extension to 64 Bits
 * Sign-extends a 32-bit source into a 64-bit destination register.
 *
 * Syntax: MOVSXD reg64, r/m32
 * Operands: 64-bit destination register; 32-bit register or memory source
 * Flags affected: None
 * Notes: Primarily used in 64-bit code when converting signed 32-bit integers to 64-bit values.
 *
 * Example:
 *   movsxd rax, dword ptr [rcx]
 *   movsxd r10, eax
 */
static const char* const ASM_MOVSXD = "movsxd";

/*
 * LEA - Load Effective Address
 * Computes the address implied by a memory operand and writes that address into a register
 * without reading memory.
 *
 * Syntax: LEA reg, mem
 * Operands: destination general-purpose register; source memory-address expression
 * Flags affected: None
 * Notes: LEA is widely used for pointer arithmetic, indexing, and strength-reduced arithmetic
 * because only address calculation occurs. Strength reduction replaces a multiply or divide by
 * a constant with a cheaper equivalent: a shift, an LEA, or a multiply by a reciprocal.
 * Compilers do this. `lea edx, [eax+eax*2]` is the reduced form of multiplying by 3. It is
 * not a special CPU mode; it is just the instruction the compiler chose. The same idea shows
 * up as SHL/SHR for powers of two and as a magic-number multiply for a fixed divisor. See
 * also the note at the start of the arithmetic section.
 *
 * Example:
 *   lea rax, [rbx+rcx*4+16]
 *   lea edx, [eax+eax*2]
 */
static const char* const ASM_LEA = "lea";

/*
 * XCHG - Exchange Operands
 * Atomically exchanges the values of two operands. When one operand is memory, the processor
 * locks the exchange with respect to other agents.
 *
 * Syntax: XCHG op1, op2
 * Operands: register/register or register/memory operands of the same size
 * Flags affected: None
 * Notes: There is no memory-to-memory form. XCHG with AX/EAX/RAX and another register has
 * special short encodings. A memory destination is locked with or without an explicit
 * LOCK prefix. That form is InterlockedExchange.
 *
 * Example:
 *   xchg eax, edx
 *   xchg rax, [rsp]
 */
static const char* const ASM_XCHG = "xchg";

/*
 * BSWAP - Byte Swap
 * Reverses the byte order of a 32-bit or 64-bit register, converting between little-endian and
 * big-endian byte layouts.
 *
 * Syntax: BSWAP reg32|reg64
 * Operands: single 32-bit or 64-bit general-purpose register
 * Flags affected: None
 * Notes: There is no 8-bit or 16-bit form. Often used in serialization, checksums, and protocol
 * parsing.
 *
 * Example:
 *   bswap eax
 *   bswap rdx
 */
static const char* const ASM_BSWAP = "bswap";

/*
 * MOVBE - Move and Byte-Swap
 * Loads or stores a word, dword, or qword and reverses its byte order in the same instruction.
 *
 * Syntax: MOVBE reg, m | MOVBE m, reg
 * Operands: A general-purpose register and a memory operand of the same size. There is no
 * register-to-register form.
 * Flags affected: None.
 * Notes: MOVBE to a register swaps while loading. MOVBE to memory swaps while storing. 16-bit,
 * 32-bit, and 64-bit forms exist; there is no 8-bit form. Introduced for Atom and later
 * indicated by CPUID.01H:ECX.MOVBE[bit 22]. BSWAP remains the register-only swap.
 *
 * Example:
 *   movbe eax, dword ptr [rcx]
 *   movbe dword ptr [rcx], eax
 */
static const char* const ASM_MOVBE = "movbe";

/*
 * CMOVA - Conditional Move if Above / unsigned greater
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 and ZF=0 is true in the current flags state.
 *
 * Syntax: CMOVA reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmova eax, ecx
 */
static const char* const ASM_CMOVA = "cmova";

/*
 * CMOVAE - Conditional Move if Above or Equal / unsigned >=
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 is true in the current flags state.
 *
 * Syntax: CMOVAE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovae eax, ecx
 */
static const char* const ASM_CMOVAE = "cmovae";

/*
 * CMOVB - Conditional Move if Below / unsigned <
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 is true in the current flags state.
 *
 * Syntax: CMOVB reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovb eax, ecx
 */
static const char* const ASM_CMOVB = "cmovb";

/*
 * CMOVBE - Conditional Move if Below or Equal / unsigned <=
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 or ZF=1 is true in the current flags state.
 *
 * Syntax: CMOVBE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovbe eax, ecx
 */
static const char* const ASM_CMOVBE = "cmovbe";

/*
 * CMOVC - Conditional Move if Carry set
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 is true in the current flags state.
 *
 * Syntax: CMOVC reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovc eax, ecx
 */
static const char* const ASM_CMOVC = "cmovc";

/*
 * CMOVE - Conditional Move if Equal / zero
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=1 is true in the current flags state.
 *
 * Syntax: CMOVE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmove eax, ecx
 */
static const char* const ASM_CMOVE = "cmove";

/*
 * CMOVG - Conditional Move if Greater / signed >
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=0 and SF=OF is true in the current flags state.
 *
 * Syntax: CMOVG reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovg eax, ecx
 */
static const char* const ASM_CMOVG = "cmovg";

/*
 * CMOVGE - Conditional Move if Greater or Equal / signed >=
 * Reads the source operand and copies it into the destination register only if the condition
 * SF=OF is true in the current flags state.
 *
 * Syntax: CMOVGE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovge eax, ecx
 */
static const char* const ASM_CMOVGE = "cmovge";

/*
 * CMOVL - Conditional Move if Less / signed <
 * Reads the source operand and copies it into the destination register only if the condition
 * SF!=OF is true in the current flags state.
 *
 * Syntax: CMOVL reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovl eax, ecx
 */
static const char* const ASM_CMOVL = "cmovl";

/*
 * CMOVLE - Conditional Move if Less or Equal / signed <=
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=1 or SF!=OF is true in the current flags state.
 *
 * Syntax: CMOVLE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovle eax, ecx
 */
static const char* const ASM_CMOVLE = "cmovle";

/*
 * CMOVNA - Conditional Move if Not Above / unsigned <=
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 or ZF=1 is true in the current flags state.
 *
 * Syntax: CMOVNA reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovna eax, ecx
 */
static const char* const ASM_CMOVNA = "cmovna";

/*
 * CMOVNAE - Conditional Move if Not Above or Equal / unsigned <
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 is true in the current flags state.
 *
 * Syntax: CMOVNAE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnae eax, ecx
 */
static const char* const ASM_CMOVNAE = "cmovnae";

/*
 * CMOVNB - Conditional Move if Not Below / unsigned >=
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 is true in the current flags state.
 *
 * Syntax: CMOVNB reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnb eax, ecx
 */
static const char* const ASM_CMOVNB = "cmovnb";

/*
 * CMOVNBE - Conditional Move if Not Below or Equal / unsigned >
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 and ZF=0 is true in the current flags state.
 *
 * Syntax: CMOVNBE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnbe eax, ecx
 */
static const char* const ASM_CMOVNBE = "cmovnbe";

/*
 * CMOVNC - Conditional Move if No carry
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 is true in the current flags state.
 *
 * Syntax: CMOVNC reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnc eax, ecx
 */
static const char* const ASM_CMOVNC = "cmovnc";

/*
 * CMOVNE - Conditional Move if Not Equal / nonzero
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=0 is true in the current flags state.
 *
 * Syntax: CMOVNE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovne eax, ecx
 */
static const char* const ASM_CMOVNE = "cmovne";

/*
 * CMOVNG - Conditional Move if Not Greater / signed <=
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=1 or SF!=OF is true in the current flags state.
 *
 * Syntax: CMOVNG reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovng eax, ecx
 */
static const char* const ASM_CMOVNG = "cmovng";

/*
 * CMOVNGE - Conditional Move if Not Greater or Equal / signed <
 * Reads the source operand and copies it into the destination register only if the condition
 * SF!=OF is true in the current flags state.
 *
 * Syntax: CMOVNGE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnge eax, ecx
 */
static const char* const ASM_CMOVNGE = "cmovnge";

/*
 * CMOVNL - Conditional Move if Not Less / signed >=
 * Reads the source operand and copies it into the destination register only if the condition
 * SF=OF is true in the current flags state.
 *
 * Syntax: CMOVNL reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnl eax, ecx
 */
static const char* const ASM_CMOVNL = "cmovnl";

/*
 * CMOVNLE - Conditional Move if Not Less or Equal / signed >
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=0 and SF=OF is true in the current flags state.
 *
 * Syntax: CMOVNLE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnle eax, ecx
 */
static const char* const ASM_CMOVNLE = "cmovnle";

/*
 * CMOVNO - Conditional Move if No overflow
 * Reads the source operand and copies it into the destination register only if the condition
 * OF=0 is true in the current flags state.
 *
 * Syntax: CMOVNO reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovno eax, ecx
 */
static const char* const ASM_CMOVNO = "cmovno";

/*
 * CMOVNP - Conditional Move if Not parity / parity odd
 * Reads the source operand and copies it into the destination register only if the condition
 * PF=0 is true in the current flags state.
 *
 * Syntax: CMOVNP reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnp eax, ecx
 */
static const char* const ASM_CMOVNP = "cmovnp";

/*
 * CMOVNS - Conditional Move if Not sign
 * Reads the source operand and copies it into the destination register only if the condition
 * SF=0 is true in the current flags state.
 *
 * Syntax: CMOVNS reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovns eax, ecx
 */
static const char* const ASM_CMOVNS = "cmovns";

/*
 * CMOVNZ - Conditional Move if Not zero
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=0 is true in the current flags state.
 *
 * Syntax: CMOVNZ reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnz eax, ecx
 */
static const char* const ASM_CMOVNZ = "cmovnz";

/*
 * CMOVO - Conditional Move if Overflow
 * Reads the source operand and copies it into the destination register only if the condition
 * OF=1 is true in the current flags state.
 *
 * Syntax: CMOVO reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovo eax, ecx
 */
static const char* const ASM_CMOVO = "cmovo";

/*
 * CMOVP - Conditional Move if Parity / parity even
 * Reads the source operand and copies it into the destination register only if the condition
 * PF=1 is true in the current flags state.
 *
 * Syntax: CMOVP reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovp eax, ecx
 */
static const char* const ASM_CMOVP = "cmovp";

/*
 * CMOVPE - Conditional Move if Parity even
 * Reads the source operand and copies it into the destination register only if the condition
 * PF=1 is true in the current flags state.
 *
 * Syntax: CMOVPE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovpe eax, ecx
 */
static const char* const ASM_CMOVPE = "cmovpe";

/*
 * CMOVPO - Conditional Move if Parity odd
 * Reads the source operand and copies it into the destination register only if the condition
 * PF=0 is true in the current flags state.
 *
 * Syntax: CMOVPO reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovpo eax, ecx
 */
static const char* const ASM_CMOVPO = "cmovpo";

/*
 * CMOVS - Conditional Move if Sign
 * Reads the source operand and copies it into the destination register only if the condition
 * SF=1 is true in the current flags state.
 *
 * Syntax: CMOVS reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovs eax, ecx
 */
static const char* const ASM_CMOVS = "cmovs";

/*
 * CMOVZ - Conditional Move if Zero
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=1 is true in the current flags state.
 *
 * Syntax: CMOVZ reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovz eax, ecx
 */
static const char* const ASM_CMOVZ = "cmovz";

/*
 * PUSH - Push onto Stack
 * Decrements the stack pointer and stores the operand value on the stack.
 *
 * Syntax: PUSH src
 * Operands: register, memory, immediate, or segment register depending on mode
 * Flags affected: None
 * Notes: Operand size controls the amount pushed. In 64-bit mode, most pushes are 8 bytes and
 * adjust RSP accordingly.
 *
 * Example:
 *   push rbp
 *   push 16
 */
static const char* const ASM_PUSH = "push";

/*
 * POP - Pop from Stack
 * Loads the value at the top of the stack into the destination operand and then increments the
 * stack pointer.
 *
 * Syntax: POP dest
 * Operands: register, memory, or certain segment registers depending on mode
 * Flags affected: None
 * Notes: The destination size controls how much stack space is removed. POP to CS is not allowed
 * with ordinary POP.
 *
 * Example:
 *   pop rbp
 *   pop qword ptr [rsp]
 */
static const char* const ASM_POP = "pop";

/*
 * PUSHA - Push All General Registers
 * Pushes the legacy 16-bit general-purpose registers in a fixed order beginning with AX and
 * including the original SP value.
 *
 * Syntax: PUSHA
 * Operands: implicit 16-bit register set
 * Flags affected: None
 * Notes: PUSHA is available only in 16-bit operand-size form and is invalid in 64-bit mode. The
 * 32-bit alias is PUSHAD, which MASM may also accept separately.
 *
 * Example:
 *   pusha
 */
static const char* const ASM_PUSHA = "pusha";

/*
 * POPA - Pop All General Registers
 * Restores the legacy 16-bit general-purpose registers from the stack in the reverse order of
 * PUSHA while discarding the saved SP slot.
 *
 * Syntax: POPA
 * Operands: implicit 16-bit register set
 * Flags affected: None
 * Notes: POPA is invalid in 64-bit mode. The 32-bit alias is POPAD in assemblers that expose it.
 *
 * Example:
 *   popa
 */
static const char* const ASM_POPA = "popa";

/*
 * PUSHF - Push Flags
 * Pushes the current FLAGS register image onto the stack using the active operand size.
 *
 * Syntax: PUSHF
 * Operands: implicit flags register
 * Flags affected: None
 * Notes: Use PUSHFD in 32-bit code or PUSHFQ in 64-bit code when you want an explicit width
 * mnemonic.
 *
 * Example:
 *   pushf
 */
static const char* const ASM_PUSHF = "pushf";

/*
 * POPF - Pop Flags
 * Restores flags bits from the top of the stack into FLAGS/EFLAGS/RFLAGS according to the active
 * operand size and privilege rules.
 *
 * Syntax: POPF
 * Operands: implicit flags register
 * Flags affected: Restores architectural flags bits from memory on the stack
 * Notes: Not all bits are writable at all privilege levels. Interrupt enable and IOPL changes
 * are restricted.
 *
 * Example:
 *   popf
 */
static const char* const ASM_POPF = "popf";

/*
 * PUSHFD - Push EFLAGS
 * Pushes the 32-bit EFLAGS image onto the stack.
 *
 * Syntax: PUSHFD
 * Operands: implicit EFLAGS register
 * Flags affected: None
 * Notes: Primarily used in 32-bit code to save status and control flags explicitly as a dword.
 *
 * Example:
 *   pushfd
 */
static const char* const ASM_PUSHFD = "pushfd";

/*
 * POPFD - Pop EFLAGS
 * Restores the 32-bit EFLAGS image from the stack subject to privilege restrictions.
 *
 * Syntax: POPFD
 * Operands: implicit EFLAGS register
 * Flags affected: Restores writable EFLAGS bits from the stack
 * Notes: Some flag bits are ignored or protected depending on CPL and mode.
 *
 * Example:
 *   popfd
 */
static const char* const ASM_POPFD = "popfd";

/*
 * PUSHFQ - Push RFLAGS
 * Pushes the 64-bit RFLAGS image onto the stack in long mode.
 *
 * Syntax: PUSHFQ
 * Operands: implicit RFLAGS register
 * Flags affected: None
 * Notes: Used by low-level 64-bit code to preserve the full visible flags state.
 *
 * Example:
 *   pushfq
 */
static const char* const ASM_PUSHFQ = "pushfq";

/*
 * POPFQ - Pop RFLAGS
 * Restores writable bits of RFLAGS from the stack in long mode.
 *
 * Syntax: POPFQ
 * Operands: implicit RFLAGS register
 * Flags affected: Restores writable RFLAGS bits from the stack
 * Notes: Privilege restrictions still apply to IF, IOPL, and system-control bits.
 *
 * Example:
 *   popfq
 */
static const char* const ASM_POPFQ = "popfq";

/*
 * CBW - Convert Byte to Word
 * Sign-extends AL into AX.
 *
 * Syntax: CBW
 * Operands: implicit AL -> AX
 * Flags affected: None
 * Notes: This instruction prepares a signed 8-bit value in AL for 16-bit arithmetic or division.
 *
 * Example:
 *   cbw
 */
static const char* const ASM_CBW = "cbw";

/*
 * CWD - Convert Word to Doubleword
 * Sign-extends AX into DX:AX.
 *
 * Syntax: CWD
 * Operands: implicit AX -> DX:AX
 * Flags affected: None
 * Notes: Commonly used before signed 16-bit IDIV.
 *
 * Example:
 *   cwd
 */
static const char* const ASM_CWD = "cwd";

/*
 * CWDE - Convert Word to Doubleword Extended
 * Sign-extends AX into EAX.
 *
 * Syntax: CWDE
 * Operands: implicit AX -> EAX
 * Flags affected: None
 * Notes: Promotes a signed 16-bit value to 32 bits without using a separate source operand.
 *
 * Example:
 *   cwde
 */
static const char* const ASM_CWDE = "cwde";

/*
 * CDQ - Convert Doubleword to Quadword
 * Sign-extends EAX into EDX:EAX.
 *
 * Syntax: CDQ
 * Operands: implicit EAX -> EDX:EAX
 * Flags affected: None
 * Notes: Typically used before signed 32-bit IDIV.
 *
 * Example:
 *   cdq
 */
static const char* const ASM_CDQ = "cdq";

/*
 * CDQE - Convert Doubleword to Quadword Extended
 * Sign-extends EAX into RAX in 64-bit mode.
 *
 * Syntax: CDQE
 * Operands: implicit EAX -> RAX
 * Flags affected: None
 * Notes: Useful when a 32-bit signed result must be promoted to a 64-bit pointer or arithmetic
 * value.
 *
 * Example:
 *   cdqe
 */
static const char* const ASM_CDQE = "cdqe";

/*
 * CQO - Convert Quadword to Octoword
 * Sign-extends RAX into RDX:RAX.
 *
 * Syntax: CQO
 * Operands: implicit RAX -> RDX:RAX
 * Flags affected: None
 * Notes: Typically used before signed 64-bit IDIV.
 *
 * Example:
 *   cqo
 */
static const char* const ASM_CQO = "cqo";

/*
 * MOVS - Move String Data
 * Move String Data copies an element from [DS:(E|R)SI] to [ES:(E|R)DI] and then advances or
 * decrements the pointers by the element size.
 *
 * Syntax: MOVS
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   rep movsb
 */
static const char* const ASM_MOVS = "movs";

/*
 * MOVSB - Move Byte String Data
 * Move Byte String Data copies one byte from [DS:(E|R)SI] to [ES:(E|R)DI] and updates SI/DI,
 * ESI/EDI, or RSI/RDI according to DF.
 *
 * Syntax: MOVSB
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   rep movsb
 */
static const char* const ASM_MOVSB = "movsb";

/*
 * MOVSW - Move Word String Data
 * Move Word String Data copies one 16-bit element between the implicit string pointers and
 * updates them by 2.
 *
 * Syntax: MOVSW
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   rep movsw
 */
static const char* const ASM_MOVSW = "movsw";

/*
 * MOVSD - Move Doubleword String Data
 * Move Doubleword String Data copies one 32-bit element between the implicit string pointers and
 * updates them by 4.
 *
 * Syntax: MOVSD
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   rep movsd
 */
static const char* const ASM_MOVSD = "movsd";

/*
 * MOVSQ - Move Quadword String Data
 * Move Quadword String Data copies one 64-bit element between RSI and RDI in 64-bit mode and
 * updates them by 8.
 *
 * Syntax: MOVSQ
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   rep movsq
 */
static const char* const ASM_MOVSQ = "movsq";

/*
 * LODS - Load String Data
 * Load String Data loads an element from [DS:(E|R)SI] into AL/AX/EAX/RAX and then updates the
 * source pointer according to DF.
 *
 * Syntax: LODS
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   cld
 *   lodsd
 */
static const char* const ASM_LODS = "lods";

/*
 * LODSB - Load Byte String Data
 * Load Byte String Data loads one byte into AL from [DS:(E|R)SI] and adjusts the source pointer
 * by 1.
 *
 * Syntax: LODSB
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   cld
 *   lodsb
 */
static const char* const ASM_LODSB = "lodsb";

/*
 * LODSW - Load Word String Data
 * Load Word String Data loads one 16-bit element into AX and adjusts the source pointer by 2.
 *
 * Syntax: LODSW
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   cld
 *   lodsw
 */
static const char* const ASM_LODSW = "lodsw";

/*
 * LODSD - Load Doubleword String Data
 * Load Doubleword String Data loads one 32-bit element into EAX and adjusts the source pointer
 * by 4.
 *
 * Syntax: LODSD
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   cld
 *   lodsd
 */
static const char* const ASM_LODSD = "lodsd";

/*
 * LODSQ - Load Quadword String Data
 * Load Quadword String Data loads one 64-bit element into RAX and adjusts RSI by 8.
 *
 * Syntax: LODSQ
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rsi, src
 *   cld
 *   lodsq
 */
static const char* const ASM_LODSQ = "lodsq";

/*
 * STOS - Store String Data
 * Store String Data stores AL/AX/EAX/RAX to [ES:(E|R)DI] and then updates the destination
 * pointer according to DF.
 *
 * Syntax: STOS
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rdi, dst
 *   mov eax, 0
 *   mov rcx, n
 *   cld
 *   rep stosd
 */
static const char* const ASM_STOS = "stos";

/*
 * STOSB - Store Byte String Data
 * Store Byte String Data stores AL to [ES:(E|R)DI] and adjusts the destination pointer by 1.
 *
 * Syntax: STOSB
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rdi, dst
 *   mov al, 0
 *   mov rcx, n
 *   cld
 *   rep stosb
 */
static const char* const ASM_STOSB = "stosb";

/*
 * STOSW - Store Word String Data
 * Store Word String Data stores AX and adjusts the destination pointer by 2.
 *
 * Syntax: STOSW
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rdi, dst
 *   mov ax, 0
 *   mov rcx, n
 *   cld
 *   rep stosw
 */
static const char* const ASM_STOSW = "stosw";

/*
 * STOSD - Store Doubleword String Data
 * Store Doubleword String Data stores EAX and adjusts the destination pointer by 4.
 *
 * Syntax: STOSD
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rdi, dst
 *   mov eax, 0
 *   mov rcx, n
 *   cld
 *   rep stosd
 */
static const char* const ASM_STOSD = "stosd";

/*
 * STOSQ - Store Quadword String Data
 * Store Quadword String Data stores RAX and adjusts RDI by 8.
 *
 * Syntax: STOSQ
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: RSI (or SI/ESI) is the source, RDI (or DI/EDI) is the destination, and RCX is the
 * count under REP. DF clear (CLD) means the pointers increment; DF set means they decrement.
 * Segment defaults are built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lea rdi, dst
 *   xor eax, eax
 *   mov rcx, n
 *   cld
 *   rep stosq
 */
static const char* const ASM_STOSQ = "stosq";

/*
 * ============================================================================
 * 7. Arithmetic Instructions
 * ============================================================================
 *
 * Strength reduction: when a multiply or divide has a constant operand, a compiler
 * often replaces it with a shift, an LEA, or a multiply by a modular reciprocal.
 * The instruction you see (SHL, LEA, or a fixed IMUL) is that reduced form. It is
 * not a special CPU mode. LEA in the data-movement section is the usual
 * multiply-by-3/5/9 spelling (`lea edx, [eax+eax*2]`).
 */

/*
 * ADD - Add
 * Adds the source operand to the destination operand and stores the sum in the destination.
 *
 * Syntax: ADD dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the result
 * Notes: There is no memory-to-memory form. ADD is the fundamental integer addition primitive.
 *
 * Example:
 *   add eax, 1
 */
static const char* const ASM_ADD = "add";

/*
 * ADC - Add with Carry
 * Adds the source operand and the current carry flag to the destination operand.
 *
 * Syntax: ADC dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the result
 * Notes: ADC is used for multi-precision addition because it incorporates CF from a previous
 * lower-word add.
 *
 * Example:
 *   adc eax, 1
 */
static const char* const ASM_ADC = "adc";

/*
 * ADCX - Add with Carry Flag
 * Adds the source and the carry flag to the destination, and writes the new carry back to CF
 * only.
 *
 * Syntax: ADCX reg, r/m
 * Operands: Register destination; register or memory source of the same size.
 * Flags affected: CF is set from the add. OF, SF, ZF, AF, and PF are left unchanged.
 * Notes: ADX feature, CPUID.07H:EBX[19]. Unlike ADC, the other status flags survive, so a
 * carry chain on CF can run beside a second chain on OF (ADOX) or beside flag-setting
 * arithmetic. Used for wide unsigned adds.
 *
 * Example:
 *   adcx rax, rcx
 */
static const char* const ASM_ADCX = "adcx";

/*
 * ADOX - Add with Overflow Flag
 * Adds the source and the overflow flag to the destination, and writes the new carry back to
 * OF only.
 *
 * Syntax: ADOX reg, r/m
 * Operands: Register destination; register or memory source of the same size.
 * Flags affected: OF is set from the add. CF, SF, ZF, AF, and PF are left unchanged.
 * Notes: ADX feature, CPUID.07H:EBX[19]. The carry input and the carry output are OF, not CF.
 * Paired with ADCX so two independent big-integer chains can advance without saving flags.
 *
 * Example:
 *   adox rax, rcx
 */
static const char* const ASM_ADOX = "adox";

/*
 * SUB - Subtract
 * Subtracts the source operand from the destination operand and stores the difference in the
 * destination.
 *
 * Syntax: SUB dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the result
 * Notes: SUB performs unsigned borrow reporting through CF and signed overflow reporting through
 * OF.
 *
 * Example:
 *   sub eax, 1
 */
static const char* const ASM_SUB = "sub";

/*
 * SBB - Subtract with Borrow
 * Subtracts the source operand and the current carry flag from the destination operand.
 *
 * Syntax: SBB dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the result
 * Notes: SBB is the subtraction counterpart to ADC for multi-precision arithmetic.
 *
 * Example:
 *   sbb eax, 1
 */
static const char* const ASM_SBB = "sbb";

/*
 * INC - Increment
 * Adds 1 to the destination operand.
 *
 * Syntax: INC dest
 * Operands: register or memory operand
 * Flags affected: OF, SF, ZF, AF, and PF are updated; CF is not changed
 * Notes: INC is convenient when carry preservation matters, but ADD reg, 1 is sometimes
 * preferred for uniform flag behavior.
 *
 * Example:
 *   inc eax, 1
 */
static const char* const ASM_INC = "inc";

/*
 * DEC - Decrement
 * Subtracts 1 from the destination operand.
 *
 * Syntax: DEC dest
 * Operands: register or memory operand
 * Flags affected: OF, SF, ZF, AF, and PF are updated; CF is not changed
 * Notes: DEC preserves CF, which distinguishes it from SUB dest, 1.
 *
 * Example:
 *   dec eax, 1
 */
static const char* const ASM_DEC = "dec";

/*
 * NEG - Two's-Complement Negate
 * Replaces the destination operand with its arithmetic negation (0 - dest).
 *
 * Syntax: NEG dest
 * Operands: register or memory operand
 * Flags affected: OF, SF, ZF, AF, PF, and CF are updated; CF becomes 0 only when the original
 * operand was zero
 * Notes: NEG is useful for sign inversion and absolute-value sequences.
 *
 * Example:
 *   neg eax, 1
 */
static const char* const ASM_NEG = "neg";

/*
 * CMP - Compare
 * Subtracts the source operand from the destination operand for flags only and discards the
 * arithmetic result.
 *
 * Syntax: CMP left, right
 * Operands: register or memory left operand; register, memory, or immediate right operand
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set as if SUB had executed
 * Notes: CMP feeds Jcc, SETcc, CMOVcc, LOOP conditions, and other flag-dependent logic.
 *
 * Example:
 *   cmp eax, 1
 */
static const char* const ASM_CMP = "cmp";

/*
 * XADD - Exchange and Add
 * Exchanges the source and destination operands, then stores the sum into the destination.
 *
 * Syntax: XADD dest, src
 * Operands: register/register or register/memory operands of the same size
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the addition result
 * Notes: With a LOCK prefix and a memory destination, XADD is useful for atomic counters and
 * fetch-add patterns.
 *
 * Example:
 *   mov eax, 1
 *   lock xadd dword ptr [rcx], eax
 */
static const char* const ASM_XADD = "xadd";

/*
 * LOCK prefix and the interlocked instructions
 *
 * Windows InterlockedExchange, InterlockedExchangeAdd, InterlockedIncrement,
 * InterlockedCompareExchange, InterlockedCompareExchange128, InterlockedAnd,
 * InterlockedOr, InterlockedXor, and InterlockedBitTestAndSet are not extra
 * opcodes. They are these instructions with a memory destination:
 *
 *   InterlockedExchange              xchg with memory (lock is implicit)
 *   InterlockedExchangeAdd           lock xadd
 *   InterlockedIncrement             lock xadd of 1, or lock inc
 *   InterlockedDecrement             lock xadd of -1, or lock dec
 *   InterlockedCompareExchange       lock cmpxchg
 *   InterlockedCompareExchange64     lock cmpxchg of a qword on x64;
 *                                    lock cmpxchg8b on x86
 *   InterlockedCompareExchange128    lock cmpxchg16b
 *   InterlockedAnd / Or / Xor        lock and / lock or / lock xor
 *   InterlockedBitTestAndSet         lock bts
 *   InterlockedBitTestAndReset       lock btr
 *   InterlockedBitTestAndComplement  lock btc
 *
 * The ordering rule is the note at the start of section 8. LOCK does not
 * make a 16-byte SSE store atomic. CMPXCHG16B is the 16-byte interlocked
 * operation, and it has its own alignment rule below.
 */

/*
 * LOCK - Atomic Read-Modify-Write Prefix
 * Prefix that makes the following read-modify-write instruction atomic with respect to other
 * agents and totally ordered with other locked operations.
 *
 * Syntax: LOCK instruction
 * Operands: A prefix. The next instruction must have a memory destination.
 * Flags affected: None from the prefix. The prefixed instruction updates flags as it otherwise
 * would.
 * Notes: Legal only before ADD, ADC, AND, BTC, BTR, BTS, CMPXCHG, CMPXCHG8B, CMPXCHG16B, DEC,
 * INC, NEG, NOT, OR, SBB, SUB, XOR, and XADD. A register destination is invalid (#UD). XCHG
 * with a memory operand is locked whether or not LOCK is written; that form is
 * InterlockedExchange. Align the destination to its natural size. A lock that crosses a cache
 * line is still specified as atomic and is the slow split-lock case. The memory-ordering rule
 * lives at the start of section 8.
 *
 * Example:
 *   mov eax, 1
 *   lock xadd dword ptr [rcx], eax
 */
static const char* const ASM_LOCK = "lock";

/*
 * CMPXCHG - Compare and Exchange
 * Compares the accumulator with the destination. On equality, stores the source register into
 * the destination. On inequality, loads the destination into the accumulator.
 *
 * Syntax: CMPXCHG r/m, reg
 * Operands: Memory or register destination; register source. The accumulator (AL, AX, EAX, or
 * RAX, matching the operand size) is implicit.
 * Flags affected: ZF=1 if the accumulator matched the destination, otherwise ZF=0. CF, PF, AF,
 * SF, and OF are unaffected.
 * Notes: LOCK CMPXCHG with a memory destination is InterlockedCompareExchange. A failed
 * compare still writes the observed value into the accumulator, so a retry loop uses that
 * value as the next expected input. The source register is not modified. 8-bit uses AL, 16-bit
 * AX, 32-bit EAX, and 64-bit RAX. There is no immediate form.
 *
 * Example:
 *   mov eax, edx
 *   lock cmpxchg dword ptr [rcx], r8d
 */
static const char* const ASM_CMPXCHG = "cmpxchg";

/*
 * CMPXCHG8B - Compare and Exchange 8 Bytes
 * Compares EDX:EAX with a memory qword. On equality, stores ECX:EBX there. On inequality,
 * loads the qword into EDX:EAX.
 *
 * Syntax: CMPXCHG8B m64
 * Operands: One memory qword. EDX:EAX is the expected value and ECX:EBX is the new value. Both
 * pairs are implicit.
 * Flags affected: ZF=1 on equality, otherwise ZF=0. CF, PF, AF, SF, and OF are unaffected.
 * Notes: This is the x86 InterlockedCompareExchange64. In 64-bit mode it still uses EDX:EAX
 * and ECX:EBX, not RDX:RAX. LOCK CMPXCHG8B is the atomic form. It does not #GP on a misaligned
 * address; align it to 8 bytes anyway. On x64 a qword LOCK CMPXCHG is the ordinary 64-bit
 * compare-exchange, and CMPXCHG8B remains the explicit EDX:EAX form.
 *
 * Example:
 *   lock cmpxchg8b qword ptr [rcx]
 */
static const char* const ASM_CMPXCHG8B = "cmpxchg8b";

/*
 * CMPXCHG16B - Compare and Exchange 16 Bytes
 * Compares RDX:RAX with a 16-byte memory operand. On equality, stores RCX:RBX there. On
 * inequality, loads the memory into RDX:RAX.
 *
 * Syntax: CMPXCHG16B m128
 * Operands: One 16-byte memory operand. RDX:RAX is the expected pair (RDX is the high half)
 * and RCX:RBX is the new pair.
 * Flags affected: ZF=1 on equality, otherwise ZF=0. CF, PF, AF, SF, and OF are unaffected.
 * Notes: InterlockedCompareExchange128. Requires the CX16 feature (CPUID.01H:ECX[13]). #GP if
 * the address is not 16-byte aligned, or if CX16 is not set. LOCK is required for the atomic
 * form used by interlocked operations. RDX is the high 64 bits of the compared value and RAX
 * is the low 64 bits.
 *
 * Example:
 *   lock cmpxchg16b xmmword ptr [rcx]
 */
static const char* const ASM_CMPXCHG16B = "cmpxchg16b";

/*
 * MUL - Unsigned Multiply
 * Multiplies the implicit accumulator by the source operand and stores the full-width product in
 * a register pair or in the wider accumulator register.
 *
 * Syntax: MUL src
 * Operands: source register or memory; implicit AL/AX/EAX/RAX accumulator
 * Flags affected: CF and OF are set when the upper half of the product is nonzero; other status
 * flags are undefined
 * Notes: Result destinations are AX for byte multiply, DX:AX for word, EDX:EAX for dword, and
 * RDX:RAX for qword.
 *
 * Example:
 *   mul ecx
 *   mul qword ptr [rsi]
 */
static const char* const ASM_MUL = "mul";

/*
 * IMUL - Signed Multiply
 * Performs signed multiplication in one-, two-, or three-operand forms.
 *
 * Syntax: IMUL src | IMUL reg, r/m | IMUL reg, r/m, imm
 * Operands: one-operand implicit-accumulator form or explicit register destination forms
 * Flags affected: CF and OF indicate whether the signed product was truncated in the explicit
 * forms; other flags are undefined or architecturally insignificant
 * Notes: The one-operand form writes a double-width product like MUL. Two- and three-operand
 * forms produce a truncated product in the destination register.
 *
 * Example:
 *   imul ecx
 *   imul eax, edx, 10
 */
static const char* const ASM_IMUL = "imul";

/*
 * DIV - Unsigned Divide
 * Divides the implicit dividend in AX, DX:AX, EDX:EAX, or RDX:RAX by the source operand.
 *
 * Syntax: DIV src
 * Operands: source register or memory; implicit dividend and quotient/remainder registers
 * Flags affected: Undefined
 * Notes: Quotient and remainder widths depend on operand size. Division by zero or quotient
 * overflow raises a divide error exception.
 *
 * Example:
 *   xor edx, edx
 *   div ecx
 */
static const char* const ASM_DIV = "div";

/*
 * IDIV - Signed Divide
 * Performs signed division of the implicit dividend by the source operand.
 *
 * Syntax: IDIV src
 * Operands: source register or memory; implicit signed dividend in AX, DX:AX, EDX:EAX, or
 * RDX:RAX
 * Flags affected: Undefined
 * Notes: Use CWD, CDQ, or CQO beforehand to sign-extend the dividend. Divide error is raised on
 * division by zero or quotient overflow.
 *
 * Example:
 *   cdq
 *   idiv ecx
 */
static const char* const ASM_IDIV = "idiv";

/*
 * ============================================================================
 * 8. Logical Instructions
 * ============================================================================
 *
 * Concurrency, for the integer operations in this section and for ordinary
 * integer moves and read-modify-writes elsewhere in this file:
 * A plain MOV of a naturally aligned 1-, 2-, 4-, or 8-byte value is atomic on
 * x64, but it is not ordered. Another thread can observe stores out of order.
 * A LOCK-prefixed read-modify-write (LOCK XADD, LOCK CMPXCHG, LOCK BTS, and the
 * implicit lock on XCHG with memory) is atomic and totally ordered with other
 * LOCKed operations. LOCK, CMPXCHG, CMPXCHG8B, and CMPXCHG16B are defined in
 * section 7. Without LOCK, two threads can tear a misaligned write or
 * any write larger than 8 bytes, and stores can become visible out of sequence.
 * Later notes that mention LOCK point back here; they do not describe a second
 * memory model. SSE vector loads are a different case and are described at the
 * start of section 19.
 */

/*
 * AND - Bitwise AND
 * Computes the bitwise logical AND of the operands and writes the result to the destination.
 *
 * Syntax: AND dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: ZF, SF, and PF reflect the result; CF and OF are cleared; AF is undefined
 * Notes: AND is often used for masking and alignment.
 *
 * Example:
 *   and eax, 0FFh
 */
static const char* const ASM_AND = "and";

/*
 * OR - Bitwise OR
 * Computes the bitwise logical inclusive OR of the operands and writes the result to the
 * destination.
 *
 * Syntax: OR dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: ZF, SF, and PF reflect the result; CF and OF are cleared; AF is undefined
 * Notes: OR is commonly used to combine bitfields or test for nonzero values.
 *
 * Example:
 *   or eax, 0FFh
 */
static const char* const ASM_OR = "or";

/*
 * XOR - Bitwise Exclusive OR
 * Computes the bitwise XOR of the operands and writes the result to the destination.
 *
 * Syntax: XOR dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: ZF, SF, and PF reflect the result; CF and OF are cleared; AF is undefined
 * Notes: XOR reg, reg is a classic zeroing idiom.
 *
 * Example:
 *   xor eax, 0FFh
 */
static const char* const ASM_XOR = "xor";

/*
 * NOT - Bitwise NOT
 * Inverts every bit in the destination operand.
 *
 * Syntax: NOT dest
 * Operands: register or memory operand
 * Flags affected: None
 * Notes: NOT is a pure bitwise complement operation and does not interact with flags.
 *
 * Example:
 *   not eax, 0FFh
 */
static const char* const ASM_NOT = "not";

/*
 * TEST - Logical Compare
 * Computes a bitwise AND for flags only and discards the result.
 *
 * Syntax: TEST left, right
 * Operands: register or memory left operand; register, memory, or immediate right operand
 * Flags affected: ZF, SF, and PF reflect the temporary result; CF and OF are cleared; AF is
 * undefined
 * Notes: TEST is commonly used to probe bits, check sign, or check for zero without modifying
 * the original operand.
 *
 * Example:
 *   test eax, 0FFh
 */
static const char* const ASM_TEST = "test";

/*
 * ============================================================================
 * 9. Shift and Rotate Instructions
 * ============================================================================
 */

/*
 * SHL - Shift Logical Left
 * Shifts the destination left, filling low bits with zero.
 *
 * Syntax: SHL dest, count
 * Operands: register or memory destination; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is defined for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: SHL is synonymous with SAL for integer operands and is commonly used for multiplication
 * by powers of two.
 *
 * Example:
 *   shl eax, 1
 */
static const char* const ASM_SHL = "shl";

/*
 * SHR - Shift Logical Right
 * Shifts the destination right, filling high bits with zero.
 *
 * Syntax: SHR dest, count
 * Operands: register or memory destination; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is defined for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: Use SHR for unsigned division by powers of two.
 *
 * Example:
 *   shr eax, 1
 */
static const char* const ASM_SHR = "shr";

/*
 * SAL - Shift Arithmetic Left
 * Alias of SHL for left shifts on integer operands.
 *
 * Syntax: SAL dest, count
 * Operands: register or memory destination; immediate count or CL
 * Flags affected: Same as SHL
 * Notes: MASM accepts SAL and SHL interchangeably for left shifts.
 *
 * Example:
 *   sal eax, 1
 */
static const char* const ASM_SAL = "sal";

/*
 * SAR - Shift Arithmetic Right
 * Shifts the destination right while replicating the original sign bit into the vacated high
 * bits.
 *
 * Syntax: SAR dest, count
 * Operands: register or memory destination; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is cleared for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: SAR preserves signedness and is commonly used for signed division by powers of two with
 * rounding toward negative infinity.
 *
 * Example:
 *   sar eax, 1
 */
static const char* const ASM_SAR = "sar";

/*
 * ROL - Rotate Left
 * Rotates bits left through the operand without involving the carry flag as part of the data
 * path.
 *
 * Syntax: ROL dest, count
 * Operands: register or memory operand; immediate count or CL
 * Flags affected: CF receives the last bit rotated out; OF is defined only for count 1
 * Notes: Rotates are useful for hashes, checksums, and bitfield permutations.
 *
 * Example:
 *   rol eax, 1
 */
static const char* const ASM_ROL = "rol";

/*
 * ROR - Rotate Right
 * Rotates bits right through the operand without involving the carry flag as part of the data
 * path.
 *
 * Syntax: ROR dest, count
 * Operands: register or memory operand; immediate count or CL
 * Flags affected: CF receives the last bit rotated out; OF is defined only for count 1
 * Notes: Common in crypto, hashing, and bitfield manipulation.
 *
 * Example:
 *   ror eax, 1
 */
static const char* const ASM_ROR = "ror";

/*
 * RCL - Rotate through Carry Left
 * Rotates the destination left through CF, treating carry as an extra bit.
 *
 * Syntax: RCL dest, count
 * Operands: register or memory operand; immediate count or CL
 * Flags affected: CF is part of the rotation path; OF is defined only for count 1
 * Notes: RCL is useful for multi-precision shifts and rotate operations across multiple words.
 *
 * Example:
 *   rcl eax, 1
 */
static const char* const ASM_RCL = "rcl";

/*
 * RCR - Rotate through Carry Right
 * Rotates the destination right through CF, treating carry as an extra bit.
 *
 * Syntax: RCR dest, count
 * Operands: register or memory operand; immediate count or CL
 * Flags affected: CF is part of the rotation path; OF is defined only for count 1
 * Notes: RCR complements RCL for multi-word arithmetic.
 *
 * Example:
 *   rcr eax, 1
 */
static const char* const ASM_RCR = "rcr";

/*
 * SHLD - Double-Precision Shift Left
 * Shifts the destination left while shifting bits in from a second source operand.
 *
 * Syntax: SHLD dest, src, count
 * Operands: destination register or memory; source register; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is defined for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: SHLD is a double-precision shift across two registers. The destination shifts left
 * and the vacated low bits come from the high bits of the source. The source itself is not
 * modified, so a 128-bit shift takes a second instruction on the low half. Count is an
 * immediate or CL.
 *
 * Example:
 *   shld rdx, rax, 1
 *   shl rax, 1
 */
static const char* const ASM_SHLD = "shld";

/*
 * SHRD - Double-Precision Shift Right
 * Shifts the destination right while shifting bits in from a second source operand.
 *
 * Syntax: SHRD dest, src, count
 * Operands: destination register or memory; source register; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is defined for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: SHRD is the rightward double-precision shift. The destination shifts right and the
 * vacated high bits come from the low bits of the source. The source is not modified. To shift
 * a 128-bit value in rdx:rax right by 1, shift the low half first (so it consumes rdx's old
 * low bit), then shift the high half.
 *
 * Example:
 *   shrd rax, rdx, 1
 *   shr rdx, 1
 */
static const char* const ASM_SHRD = "shrd";

/*
 * ============================================================================
 * 10. Bit Manipulation
 * ============================================================================
 *
 * BT, BSF, BSR, POPCNT, LZCNT, TZCNT, and the BMI forms are real instructions, and
 * production compilers emit them when the target allows it. If this repository's
 * generated output prefers TEST, SHR, or a short loop instead, that is because
 * those sequences are easier to validate and do not depend on POPCNT or BMI CPUID
 * bits. The instructions are not obsolete. A LOCK prefix on a memory BT/BTS/BTR/BTC
 * makes the bit update atomic; see the concurrency note at the start of section 8.
 */

/*
 * BT - Bit Test
 * Copies the selected bit from the source operand into CF.
 *
 * Syntax: BT base, index
 * Operands: register or memory bit base; register or immediate bit index
 * Flags affected: CF receives the selected bit; other flags are unchanged
 * Notes: BT is commonly used to probe bitmaps and capability masks.
 *
 * Example:
 *   bt eax, ebx
 */
static const char* const ASM_BT = "bt";

/*
 * BTS - Bit Test and Set
 * Copies the selected bit into CF and then sets that bit in the destination.
 *
 * Syntax: BTS base, index
 * Operands: register or memory bit base; register or immediate bit index
 * Flags affected: CF receives the original bit value; other flags are unchanged
 * Notes: With LOCK on memory, BTS can atomically claim a bit.
 *
 * Example:
 *   bts eax, ebx
 */
static const char* const ASM_BTS = "bts";

/*
 * BTR - Bit Test and Reset
 * Copies the selected bit into CF and then clears that bit in the destination.
 *
 * Syntax: BTR base, index
 * Operands: register or memory bit base; register or immediate bit index
 * Flags affected: CF receives the original bit value; other flags are unchanged
 * Notes: Useful for bitmap allocators and flag words.
 *
 * Example:
 *   btr eax, ebx
 */
static const char* const ASM_BTR = "btr";

/*
 * BTC - Bit Test and Complement
 * Copies the selected bit into CF and then toggles that bit in the destination.
 *
 * Syntax: BTC base, index
 * Operands: register or memory bit base; register or immediate bit index
 * Flags affected: CF receives the original bit value; other flags are unchanged
 * Notes: With LOCK on memory, BTC can atomically flip a shared bit.
 *
 * Example:
 *   btc eax, ebx
 */
static const char* const ASM_BTC = "btc";

/*
 * BSF - Bit Scan Forward
 * Searches for the least-significant set bit and returns its index.
 *
 * Syntax: BSF reg, src
 * Operands: destination register; source register or memory
 * Flags affected: ZF=1 if the source is zero, otherwise ZF=0 and destination receives the bit
 * index; other flags are undefined
 * Notes: The destination is undefined when the source is zero.
 *
 * Example:
 *   bsf eax, ebx
 */
static const char* const ASM_BSF = "bsf";

/*
 * BSR - Bit Scan Reverse
 * Searches for the most-significant set bit and returns its index.
 *
 * Syntax: BSR reg, src
 * Operands: destination register; source register or memory
 * Flags affected: ZF=1 if the source is zero, otherwise ZF=0 and destination receives the bit
 * index; other flags are undefined
 * Notes: The destination is undefined when the source is zero.
 *
 * Example:
 *   bsr eax, ebx
 */
static const char* const ASM_BSR = "bsr";

/*
 * POPCNT - Population Count
 * Counts the number of set bits in the source operand and writes that count to the destination
 * register.
 *
 * Syntax: POPCNT reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: ZF=1 if the source is zero; CF, OF, SF, AF, and PF are cleared
 * Notes: Requires POPCNT support and is useful for bitset cardinality and rank operations.
 *
 * Example:
 *   popcnt eax, ebx
 */
static const char* const ASM_POPCNT = "popcnt";

/*
 * LZCNT - Leading Zero Count
 * Counts the number of leading zero bits in the source operand.
 *
 * Syntax: LZCNT reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: ZF is set if the result is zero; CF is set if the source is zero; other
 * arithmetic flags are cleared or undefined by the architecture
 * Notes: Requires ABM/LZCNT support and differs from BSR on zero inputs.
 *
 * Example:
 *   lzcnt eax, ebx
 */
static const char* const ASM_LZCNT = "lzcnt";

/*
 * TZCNT - Trailing Zero Count
 * Counts the number of trailing zero bits in the source operand.
 *
 * Syntax: TZCNT reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: ZF is set if the result is zero; CF is set if the source is zero; other
 * arithmetic flags are cleared or undefined by the architecture
 * Notes: Requires BMI1 support and differs from BSF on zero inputs.
 *
 * Example:
 *   tzcnt eax, ebx
 */
static const char* const ASM_TZCNT = "tzcnt";

/*
 * ANDN - Bitwise AND NOT
 * Computes the bitwise AND of the inverted first source and the second source and writes the
 * result to the destination.
 *
 * Syntax: ANDN reg, src1, src2
 * Operands: destination register; two register or register/memory sources
 * Flags affected: ZF and SF reflect the result; CF and OF are cleared; AF and PF are undefined
 * Notes: BMI1 instruction useful for mask clearing without an explicit NOT.
 *
 * Example:
 *   andn eax, ebx
 */
static const char* const ASM_ANDN = "andn";

/*
 * BEXTR - Bit Field Extract
 * Extracts a contiguous bit field from the source operand based on a start and length control
 * value.
 *
 * Syntax: BEXTR reg, src, control
 * Operands: destination register; source register or memory; control register or immediate form
 * depending on encoding
 * Flags affected: ZF=1 when the extracted result is zero; CF and OF are cleared; other flags are
 * undefined
 * Notes: BMI1 and TBM variants exist; MASM usually follows the Intel BMI1 form. The control
 * operand is not a mystery third data value: its low byte is the starting bit and the next
 * byte is the length. `mov ecx, 0804h` means start at bit 4 and take 8 bits, so the
 * instruction copies ebx[11:4] into eax and zero-extends. Bits above the extracted field are
 * cleared. A length of zero yields zero.
 *
 * Example:
 *   mov ecx, 0804h
 *   bextr eax, ebx, ecx
 */
static const char* const ASM_BEXTR = "bextr";

/*
 * BLSI - Extract Lowest Set Bit
 * Copies only the least-significant set bit of the source to the destination and clears all
 * other bits.
 *
 * Syntax: BLSI reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: CF=1 if the source was nonzero; ZF reflects whether the result is zero; OF is
 * cleared; SF reflects the result
 * Notes: BMI1 instruction useful for low-bit isolation.
 *
 * Example:
 *   blsi eax, ebx
 */
static const char* const ASM_BLSI = "blsi";

/*
 * BLSMSK - Mask from Lowest Set Bit
 * Generates a mask of ones from bit 0 through the least-significant set bit of the source.
 *
 * Syntax: BLSMSK reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: CF=1 if the source was nonzero; ZF reflects whether the result is zero; OF is
 * cleared; SF reflects the result
 * Notes: BMI1 instruction often paired with bit scanning and sparse-set algorithms.
 *
 * Example:
 *   blsmsk eax, ebx
 */
static const char* const ASM_BLSMSK = "blsmsk";

/*
 * BLSR - Reset Lowest Set Bit
 * Copies the source to the destination while clearing only the least-significant set bit.
 *
 * Syntax: BLSR reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: CF=1 if the source was zero; ZF reflects whether the result is zero; OF is
 * cleared; SF reflects the result
 * Notes: BMI1 instruction useful for iterating through set bits.
 *
 * Example:
 *   blsr eax, ebx
 */
static const char* const ASM_BLSR = "blsr";

/*
 * BZHI - Zero High Bits
 * Copies the low part of the source operand up to the index count and clears all higher bits.
 *
 * Syntax: BZHI reg, src, index
 * Operands: destination register; source register or memory; index register
 * Flags affected: ZF and SF reflect the result; CF reports whether the index exceeded the
 * operand width; OF is cleared
 * Notes: BMI2 instruction useful for mask generation without an explicit shift and subtract.
 *
 * Example:
 *   bzhi eax, ebx
 */
static const char* const ASM_BZHI = "bzhi";

/*
 * PDEP - Parallel Bit Deposit
 * Deposits contiguous low bits from one source into positions selected by a mask operand.
 *
 * Syntax: PDEP reg, src, mask
 * Operands: destination register; source register; mask register or memory
 * Flags affected: None
 * Notes: BMI2 instruction used for Morton codes, sparse bit packing, and table-free
 * permutations.
 *
 * Example:
 *   pdep eax, ebx
 */
static const char* const ASM_PDEP = "pdep";

/*
 * PEXT - Parallel Bit Extract
 * Extracts bits selected by a mask operand and packs them contiguously into the destination.
 *
 * Syntax: PEXT reg, src, mask
 * Operands: destination register; source register; mask register or memory
 * Flags affected: None
 * Notes: BMI2 instruction used for dense extraction from sparse bitfields.
 *
 * Example:
 *   pext eax, ebx, ecx
 */
static const char* const ASM_PEXT = "pext";

/*
 * MULX - Unsigned Multiply Without Flags
 * Multiplies RDX or EDX by the explicit source and writes the high half and the low half to
 * two separate registers.
 *
 * Syntax: MULX high, low, src
 * Operands: Two register destinations and one register or memory source. The other factor is
 * implicit in RDX (64-bit) or EDX (32-bit).
 * Flags affected: None. MULX does not modify RFLAGS.
 * Notes: BMI2, CPUID.07H:EBX[8]. The first destination is the high half and the second is the
 * low half. `mulx rax, rbx, rcx` sets rax:rbx to rdx * rcx, high half in rax. The implicit
 * factor is not overwritten unless it is also a destination.
 *
 * Example:
 *   mulx rax, rbx, rcx
 */
static const char* const ASM_MULX = "mulx";

/*
 * RORX - Rotate Right Without Flags
 * Rotates the source right by an immediate count and writes the result to a separate
 * destination.
 *
 * Syntax: RORX reg, r/m, imm8
 * Operands: Register destination, register or memory source, and an 8-bit immediate count.
 * Flags affected: None. RORX does not modify RFLAGS.
 * Notes: BMI2. The count is masked to 6 bits for a 64-bit operand and 5 bits for a 32-bit
 * operand, the same mask ROR uses. The source is not overwritten unless it is also the
 * destination.
 *
 * Example:
 *   rorx rax, rcx, 13
 */
static const char* const ASM_RORX = "rorx";

/*
 * SHLX - Logical Shift Left Without Flags
 * Shifts the source left by a count in a register and writes the result to a separate
 * destination.
 *
 * Syntax: SHLX reg, r/m, count
 * Operands: Register destination, register or memory source, and a register count.
 * Flags affected: None. SHLX does not modify RFLAGS.
 * Notes: BMI2. The count is masked to the operand width, as SHL would mask it. Bits shifted
 * out are discarded and zeros enter at the bottom. The source register is preserved when it is
 * not the destination.
 *
 * Example:
 *   shlx rax, rcx, rdx
 */
static const char* const ASM_SHLX = "shlx";

/*
 * SHRX - Logical Shift Right Without Flags
 * Shifts the source right by a count in a register, filling with zeros, and writes a separate
 * destination.
 *
 * Syntax: SHRX reg, r/m, count
 * Operands: Register destination, register or memory source, and a register count.
 * Flags affected: None. SHRX does not modify RFLAGS.
 * Notes: BMI2. Logical, not arithmetic: the high bits fill with zero. The count is masked to
 * the operand width. This is the three-operand form of SHR.
 *
 * Example:
 *   shrx rax, rcx, rdx
 */
static const char* const ASM_SHRX = "shrx";

/*
 * SARX - Arithmetic Shift Right Without Flags
 * Shifts the source right by a count in a register, filling with the sign bit, and writes a
 * separate destination.
 *
 * Syntax: SARX reg, r/m, count
 * Operands: Register destination, register or memory source, and a register count.
 * Flags affected: None. SARX does not modify RFLAGS.
 * Notes: BMI2. Arithmetic, like SAR: the original sign bit is copied into the vacated high
 * bits. The count is masked to the operand width.
 *
 * Example:
 *   sarx rax, rcx, rdx
 */
static const char* const ASM_SARX = "sarx";

/*
 * ============================================================================
 * 11. Control Flow
 * ============================================================================
 */

/*
 * JMP - Unconditional Jump
 * Transfers control to the target address without saving a return address.
 *
 * Syntax: JMP target
 * Operands: relative label, register, or memory indirect target depending on form
 * Flags affected: None
 * Notes: Short, near, and far forms exist. MASM chooses the shortest legal encoding for label-
 * based relative jumps unless directed otherwise.
 *
 * Example:
 *   jmp done
 *   jmp qword ptr [rax]
 */
static const char* const ASM_JMP = "jmp";

/*
 * CALL - Call Procedure
 * Transfers control to the target and pushes a return address on the stack for RET to use later.
 *
 * Syntax: CALL target
 * Operands: relative label, register, or memory indirect target; far forms are also available in
 * appropriate modes
 * Flags affected: None
 * Notes: CALL is the standard procedure-invocation instruction. Indirect calls are used for
 * function pointers, imports, and vtables.
 *
 * Example:
 *   call printf
 *   call qword ptr [rax]
 */
static const char* const ASM_CALL = "call";

/*
 * RET - Return from Procedure
 * Pops the saved return address from the stack and transfers control back to the caller.
 *
 * Syntax: RET [imm16]
 * Operands: implicit stack top with optional immediate stack adjustment
 * Flags affected: None
 * Notes: RET and RETN are near returns: they pop RIP (or EIP/IP) and leave CS alone. RETF is
 * the far return: it pops the offset and then CS. Use RETF only for a far call, an interrupt
 * return that is not IRET, or a legacy segment change. RET n and RETF n also pop n extra
 * bytes, which is how a callee-pops convention (stdcall) removes arguments. The Windows x64
 * ABI is caller-cleanup, so that immediate form is not used there. In flat 64-bit user code
 * the ordinary near RET is the one you want.
 *
 * Example:
 *   ret
 *   ret 16
 */
static const char* const ASM_RET = "ret";

/*
 * RETN - Near Return
 * Returns to a near caller within the current code segment.
 *
 * Syntax: RETN [imm16]
 * Operands: implicit stack top with optional immediate stack adjustment
 * Flags affected: None
 * Notes: RETN is the explicit near-return spelling. It pops only the return offset, same as
 * RET. Write RETN when a listing must show that this return is near, for example next to a
 * RETF in 16-bit code that mixes near and far procedures. `retn 8` pops the return address
 * and then discards 8 bytes of callee-owned arguments. Windows x64 does not use that form.
 *
 * Example:
 *   retn
 *   retn 8
 */
static const char* const ASM_RETN = "retn";

/*
 * RETF - Far Return
 * Returns to a far caller by popping IP/EIP/RIP and CS, optionally with stack adjustment.
 *
 * Syntax: RETF [imm16]
 * Operands: implicit far return frame on the stack
 * Flags affected: None
 * Notes: RETF pops the offset and CS. Use it only to return from a far CALL, from a call
 * gate, or from a legacy segment change. It is not the return a Windows x64 function uses;
 * that ABI is near and caller-cleanup. `retf 4` also discards 4 extra bytes after the far
 * frame. A hardware interrupt return is IRET/IRETQ, which restores flags as well as CS.
 *
 * Example:
 *   retf
 *   retf 4
 */
static const char* const ASM_RETF = "retf";

/*
 * JA - Jump if Above / unsigned greater
 * Transfers control to the target label only when the condition CF=0 and ZF=0 is true in the
 * current status flags.
 *
 * Syntax: JA label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   ja target
 */
static const char* const ASM_JA = "ja";

/*
 * JAE - Jump if Above or Equal / unsigned >=
 * Transfers control to the target label only when the condition CF=0 is true in the current
 * status flags.
 *
 * Syntax: JAE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jae target
 */
static const char* const ASM_JAE = "jae";

/*
 * JB - Jump if Below / unsigned <
 * Transfers control to the target label only when the condition CF=1 is true in the current
 * status flags.
 *
 * Syntax: JB label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jb target
 */
static const char* const ASM_JB = "jb";

/*
 * JBE - Jump if Below or Equal / unsigned <=
 * Transfers control to the target label only when the condition CF=1 or ZF=1 is true in the
 * current status flags.
 *
 * Syntax: JBE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jbe target
 */
static const char* const ASM_JBE = "jbe";

/*
 * JC - Jump if Carry set
 * Transfers control to the target label only when the condition CF=1 is true in the current
 * status flags.
 *
 * Syntax: JC label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jc target
 */
static const char* const ASM_JC = "jc";

/*
 * JE - Jump if Equal / zero
 * Transfers control to the target label only when the condition ZF=1 is true in the current
 * status flags.
 *
 * Syntax: JE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   je target
 */
static const char* const ASM_JE = "je";

/*
 * JG - Jump if Greater / signed >
 * Transfers control to the target label only when the condition ZF=0 and SF=OF is true in the
 * current status flags.
 *
 * Syntax: JG label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jg target
 */
static const char* const ASM_JG = "jg";

/*
 * JGE - Jump if Greater or Equal / signed >=
 * Transfers control to the target label only when the condition SF=OF is true in the current
 * status flags.
 *
 * Syntax: JGE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jge target
 */
static const char* const ASM_JGE = "jge";

/*
 * JL - Jump if Less / signed <
 * Transfers control to the target label only when the condition SF!=OF is true in the current
 * status flags.
 *
 * Syntax: JL label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jl target
 */
static const char* const ASM_JL = "jl";

/*
 * JLE - Jump if Less or Equal / signed <=
 * Transfers control to the target label only when the condition ZF=1 or SF!=OF is true in the
 * current status flags.
 *
 * Syntax: JLE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jle target
 */
static const char* const ASM_JLE = "jle";

/*
 * JNA - Jump if Not Above / unsigned <=
 * Transfers control to the target label only when the condition CF=1 or ZF=1 is true in the
 * current status flags.
 *
 * Syntax: JNA label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jna target
 */
static const char* const ASM_JNA = "jna";

/*
 * JNAE - Jump if Not Above or Equal / unsigned <
 * Transfers control to the target label only when the condition CF=1 is true in the current
 * status flags.
 *
 * Syntax: JNAE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnae target
 */
static const char* const ASM_JNAE = "jnae";

/*
 * JNB - Jump if Not Below / unsigned >=
 * Transfers control to the target label only when the condition CF=0 is true in the current
 * status flags.
 *
 * Syntax: JNB label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnb target
 */
static const char* const ASM_JNB = "jnb";

/*
 * JNBE - Jump if Not Below or Equal / unsigned >
 * Transfers control to the target label only when the condition CF=0 and ZF=0 is true in the
 * current status flags.
 *
 * Syntax: JNBE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnbe target
 */
static const char* const ASM_JNBE = "jnbe";

/*
 * JNC - Jump if No carry
 * Transfers control to the target label only when the condition CF=0 is true in the current
 * status flags.
 *
 * Syntax: JNC label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnc target
 */
static const char* const ASM_JNC = "jnc";

/*
 * JNE - Jump if Not Equal / nonzero
 * Transfers control to the target label only when the condition ZF=0 is true in the current
 * status flags.
 *
 * Syntax: JNE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jne target
 */
static const char* const ASM_JNE = "jne";

/*
 * JNG - Jump if Not Greater / signed <=
 * Transfers control to the target label only when the condition ZF=1 or SF!=OF is true in the
 * current status flags.
 *
 * Syntax: JNG label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jng target
 */
static const char* const ASM_JNG = "jng";

/*
 * JNGE - Jump if Not Greater or Equal / signed <
 * Transfers control to the target label only when the condition SF!=OF is true in the current
 * status flags.
 *
 * Syntax: JNGE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnge target
 */
static const char* const ASM_JNGE = "jnge";

/*
 * JNL - Jump if Not Less / signed >=
 * Transfers control to the target label only when the condition SF=OF is true in the current
 * status flags.
 *
 * Syntax: JNL label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnl target
 */
static const char* const ASM_JNL = "jnl";

/*
 * JNLE - Jump if Not Less or Equal / signed >
 * Transfers control to the target label only when the condition ZF=0 and SF=OF is true in the
 * current status flags.
 *
 * Syntax: JNLE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnle target
 */
static const char* const ASM_JNLE = "jnle";

/*
 * JNO - Jump if No overflow
 * Transfers control to the target label only when the condition OF=0 is true in the current
 * status flags.
 *
 * Syntax: JNO label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jno target
 */
static const char* const ASM_JNO = "jno";

/*
 * JNP - Jump if Not parity / parity odd
 * Transfers control to the target label only when the condition PF=0 is true in the current
 * status flags.
 *
 * Syntax: JNP label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnp target
 */
static const char* const ASM_JNP = "jnp";

/*
 * JNS - Jump if Not sign
 * Transfers control to the target label only when the condition SF=0 is true in the current
 * status flags.
 *
 * Syntax: JNS label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jns target
 */
static const char* const ASM_JNS = "jns";

/*
 * JNZ - Jump if Not zero
 * Transfers control to the target label only when the condition ZF=0 is true in the current
 * status flags.
 *
 * Syntax: JNZ label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnz target
 */
static const char* const ASM_JNZ = "jnz";

/*
 * JO - Jump if Overflow
 * Transfers control to the target label only when the condition OF=1 is true in the current
 * status flags.
 *
 * Syntax: JO label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jo target
 */
static const char* const ASM_JO = "jo";

/*
 * JP - Jump if Parity / parity even
 * Transfers control to the target label only when the condition PF=1 is true in the current
 * status flags.
 *
 * Syntax: JP label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jp target
 */
static const char* const ASM_JP = "jp";

/*
 * JPE - Jump if Parity even
 * Transfers control to the target label only when the condition PF=1 is true in the current
 * status flags.
 *
 * Syntax: JPE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jpe target
 */
static const char* const ASM_JPE = "jpe";

/*
 * JPO - Jump if Parity odd
 * Transfers control to the target label only when the condition PF=0 is true in the current
 * status flags.
 *
 * Syntax: JPO label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jpo target
 */
static const char* const ASM_JPO = "jpo";

/*
 * JS - Jump if Sign
 * Transfers control to the target label only when the condition SF=1 is true in the current
 * status flags.
 *
 * Syntax: JS label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   js target
 */
static const char* const ASM_JS = "js";

/*
 * JZ - Jump if Zero
 * Transfers control to the target label only when the condition ZF=1 is true in the current
 * status flags.
 *
 * Syntax: JZ label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jz target
 */
static const char* const ASM_JZ = "jz";

/*
 * JECXZ - Jump if ECX is Zero
 * Transfers control to the target if ECX equals zero.
 *
 * Syntax: JECXZ label
 * Operands: short relative label
 * Flags affected: None
 * Notes: Useful for loop setup in 32-bit code because it tests the count register directly
 * rather than the flags register. Compilers and this repository's generators usually write
 * `test ecx, ecx` / `jz` instead. JECXZ only reaches a short (8-bit) displacement, and the
 * TEST/Jcc form is not slower and is easier to validate.
 *
 * Example:
 *   jecxz empty
 */
static const char* const ASM_JECXZ = "jecxz";

/*
 * JRCXZ - Jump if RCX is Zero
 * Transfers control to the target if RCX equals zero.
 *
 * Syntax: JRCXZ label
 * Operands: short relative label
 * Flags affected: None
 * Notes: 64-bit counterpart to JECXZ. It tests RCX without writing flags, but only with a
 * short displacement. Generated code in this repository uses `test rcx, rcx` / `jz` or
 * `cmp rcx, imm` / `jnz` because that is easier to validate and can reach a near target.
 * JRCXZ is not obsolete; it is just a poor fit for those generators.
 *
 * Example:
 *   jrcxz empty
 */
static const char* const ASM_JRCXZ = "jrcxz";

/*
 * LOOP - Counted Loop
 * Counted Loop decrements CX, ECX, or RCX according to address size and jumps when the count is
 * not zero.
 *
 * Syntax: LOOP label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP decrements RCX (or ECX/CX, by address size) and branches if the count is not
 * zero. It does not alter status flags. Modern compilers avoid it: on current Intel and AMD
 * it is slower than `dec rcx` / `jnz`. That is why generated code in this repository uses
 * CMP or DEC plus JNZ instead. LOOPE, LOOPNE, LOOPZ, and LOOPNZ are the same instruction
 * family with an extra ZF test; they are avoided for the same reason.
 *
 * Example:
 *   loop next_item
 */
static const char* const ASM_LOOP = "loop";

/*
 * LOOPE - Loop While Equal
 * Loop While Equal decrements the count register and jumps while the count is not zero and ZF=1.
 *
 * Syntax: LOOPE label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loope next_item
 */
static const char* const ASM_LOOPE = "loope";

/*
 * LOOPNE - Loop While Not Equal
 * Loop While Not Equal decrements the count register and jumps while the count is not zero and
 * ZF=0.
 *
 * Syntax: LOOPNE label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loopne next_item
 */
static const char* const ASM_LOOPNE = "loopne";

/*
 * LOOPZ - Loop While Zero
 * Loop While Zero alias of LOOPE that decrements the count and jumps while ZF=1 and the count is
 * nonzero.
 *
 * Syntax: LOOPZ label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loopz next_item
 */
static const char* const ASM_LOOPZ = "loopz";

/*
 * LOOPNZ - Loop While Not Zero
 * Loop While Not Zero alias of LOOPNE that decrements the count and jumps while ZF=0 and the
 * count is nonzero.
 *
 * Syntax: LOOPNZ label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loopnz next_item
 */
static const char* const ASM_LOOPNZ = "loopnz";

/*
 * ============================================================================
 * 12. String Instructions
 * ============================================================================
 *
 * RSI is the source, RDI is the destination, and RCX is the count under REP.
 * DF clear (CLD) means the pointers increment by the element size; DF set (STD)
 * means they decrement. The MOVS/LODS/STOS forms earlier in the file use the same
 * registers. Each example below is a complete setup, not a bare mnemonic.
 */

/*
 * CMPS - Compare String Data
 * Compare String Data compares memory elements at the implicit source and destination pointers,
 * sets flags as if a subtraction occurred, and advances or decrements the pointers according to
 * DF.
 *
 * Syntax: CMPS
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   repe cmpsb
 */
static const char* const ASM_CMPS = "cmps";

/*
 * CMPSB - Compare Byte String Data
 * Compare Byte String Data compares bytes at [DS:(E|R)SI] and [ES:(E|R)DI].
 *
 * Syntax: CMPSB
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   repe cmpsb
 */
static const char* const ASM_CMPSB = "cmpsb";

/*
 * CMPSW - Compare Word String Data
 * Compare Word String Data compares 16-bit elements and updates the string pointers by 2.
 *
 * Syntax: CMPSW
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   repe cmpsw
 */
static const char* const ASM_CMPSW = "cmpsw";

/*
 * CMPSD - Compare Doubleword String Data
 * Compare Doubleword String Data compares 32-bit elements and updates the string pointers by 4.
 *
 * Syntax: CMPSD
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   repe cmpsd
 */
static const char* const ASM_CMPSD = "cmpsd";

/*
 * CMPSQ - Compare Quadword String Data
 * Compare Quadword String Data compares 64-bit elements and updates RSI/RDI by 8.
 *
 * Syntax: CMPSQ
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rsi, src
 *   lea rdi, dst
 *   mov rcx, n
 *   cld
 *   repe cmpsq
 */
static const char* const ASM_CMPSQ = "cmpsq";

/*
 * SCAS - Scan String Data
 * Scan String Data compares AL/AX/EAX/RAX against the memory element at [ES:(E|R)DI], sets
 * flags, and updates the destination pointer according to DF.
 *
 * Syntax: SCAS
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rdi, dst
 *   mov al, 0
 *   mov rcx, n
 *   cld
 *   repne scasb
 */
static const char* const ASM_SCAS = "scas";

/*
 * SCASB - Scan Byte String Data
 * Scan Byte String Data compares AL against the destination byte at [ES:(E|R)DI].
 *
 * Syntax: SCASB
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rdi, dst
 *   mov al, 0
 *   mov rcx, n
 *   cld
 *   repne scasb
 */
static const char* const ASM_SCASB = "scasb";

/*
 * SCASW - Scan Word String Data
 * Scan Word String Data compares AX against the destination word and updates the pointer by 2.
 *
 * Syntax: SCASW
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rdi, dst
 *   mov ax, 0
 *   mov rcx, n
 *   cld
 *   repne scasw
 */
static const char* const ASM_SCASW = "scasw";

/*
 * SCASD - Scan Doubleword String Data
 * Scan Doubleword String Data compares EAX against the destination dword and updates the pointer
 * by 4.
 *
 * Syntax: SCASD
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rdi, dst
 *   mov eax, 0
 *   mov rcx, n
 *   cld
 *   repne scasd
 */
static const char* const ASM_SCASD = "scasd";

/*
 * SCASQ - Scan Quadword String Data
 * Scan Quadword String Data compares RAX against the destination qword and updates RDI by 8.
 *
 * Syntax: SCASQ
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: RSI is the source, RDI is the destination, and RCX is the count under REPE/REPNE.
 * DF clear (CLD) means the pointers increment. SCAS compares AL/AX/EAX/RAX with [RDI] and does
 * not use RSI. REP, REPE/REPZ, and REPNE/REPNZ prefix these instructions to walk a buffer.
 *
 * Example:
 *   lea rdi, dst
 *   xor eax, eax
 *   mov rcx, n
 *   cld
 *   repne scasq
 */
static const char* const ASM_SCASQ = "scasq";

/*
 * REP - Repeat Prefix
 * Repeat Prefix repeats the following string instruction while RCX/ECX/CX is nonzero.
 *
 * Syntax: REP string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   rep movsb
 */
static const char* const ASM_REP = "rep";

/*
 * REPE - Repeat While Equal Prefix
 * Repeat While Equal Prefix repeats the following CMPS or SCAS instruction while the count is
 * nonzero and ZF=1.
 *
 * Syntax: REPE string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   repe movsb
 */
static const char* const ASM_REPE = "repe";

/*
 * REPNE - Repeat While Not Equal Prefix
 * Repeat While Not Equal Prefix repeats the following CMPS or SCAS instruction while the count
 * is nonzero and ZF=0.
 *
 * Syntax: REPNE string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   repne movsb
 */
static const char* const ASM_REPNE = "repne";

/*
 * REPZ - Repeat While Zero Prefix
 * Repeat While Zero Prefix alias of REPE.
 *
 * Syntax: REPZ string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   repz movsb
 */
static const char* const ASM_REPZ = "repz";

/*
 * REPNZ - Repeat While Not Zero Prefix
 * Repeat While Not Zero Prefix alias of REPNE.
 *
 * Syntax: REPNZ string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   repnz movsb
 */
static const char* const ASM_REPNZ = "repnz";

/*
 * ============================================================================
 * 13. Flag Instructions
 * ============================================================================
 *
 * Status and control flags these instructions read or write. Arithmetic and
 * compare instructions set the status flags; the instructions below mostly
 * set or clear a single flag, or copy a subset.
 *
 * CF  Carry. Set by unsigned overflow on ADD/SUB, by shifts (last bit out),
 *     by BT (the tested bit), and by comparisons (below). CLC/STC/CMC change
 *     it directly. ADC and SBB consume it.
 * ZF  Zero. Set when a result or a comparison difference is zero. JE/JZ and
 *     CMOVE/SETE test it. DEC and most arithmetic set it; LOOP does not.
 * SF  Sign. Copy of the result's top bit. JS and signed compares use it.
 * OF  Overflow. Set when a signed result does not fit. JO and signed compares
 *     combine it with SF. A 1-bit shift defines OF; a longer shift leaves it
 *     undefined on many forms.
 * PF  Parity. Set when the low 8 bits of the result contain an even number of
 *     1 bits. JP/JNP test it. Also set by unordered x87 compares (FCOMI) to
 *     mean "at least one operand was NaN".
 * AF  Auxiliary carry. Carry out of bit 3, for BCD. Most modern code ignores
 *     it. Arithmetic sets it; logical instructions leave it undefined.
 * DF  Direction. CLD clears it so string ops increment RSI/RDI. STD sets it
 *     so they decrement. The Windows ABI requires DF clear on entry and return.
 * IF  Interrupt enable. STI sets it, CLI clears it, subject to privilege.
 *     Maskable interrupts are delivered only while IF is 1.
 * TF  Trap. When set, the processor raises a debug exception after an
 *     instruction. Debuggers use it for single step. POPF can set it; there
 *     is no dedicated user mnemonic in this catalogue.
 *
 * SETcc writes a byte of 1 when its condition is true and 0 otherwise. It does
 * not itself change flags. Compilers do emit SETcc. If this repository's
 * generated output shows CMP+Jcc instead, that is because those branches are
 * easier to validate, not because SETcc is obsolete. See the CMOVcc note in
 * the data-movement section for the same generator choice.
 */

/*
 * STC - Set Carry Flag
 * Sets CF to 1.
 *
 * Syntax: STC
 * Operands: implicit flags register
 * Flags affected: CF is set; all other flags are unchanged
 * Notes: Used before ADC, RCL, multi-precision arithmetic, or software carry generation.
 *
 * Example:
 *   stc
 */
static const char* const ASM_STC = "stc";

/*
 * CLC - Clear Carry Flag
 * Clears CF to 0.
 *
 * Syntax: CLC
 * Operands: implicit flags register
 * Flags affected: CF is cleared; all other flags are unchanged
 * Notes: Often used before ADC or to normalize carry state.
 *
 * Example:
 *   clc
 */
static const char* const ASM_CLC = "clc";

/*
 * CMC - Complement Carry Flag
 * Inverts CF.
 *
 * Syntax: CMC
 * Operands: implicit flags register
 * Flags affected: CF is toggled; all other flags are unchanged
 * Notes: Useful in multi-precision arithmetic and bit-serial algorithms.
 *
 * Example:
 *   cmc
 */
static const char* const ASM_CMC = "cmc";

/*
 * STD - Set Direction Flag
 * Sets DF so string instructions auto-decrement their pointer registers.
 *
 * Syntax: STD
 * Operands: implicit flags register
 * Flags affected: DF is set; all other flags are unchanged
 * Notes: Low-level code must usually restore DF to the expected ABI state before returning.
 *
 * Example:
 *   std
 */
static const char* const ASM_STD = "std";

/*
 * CLD - Clear Direction Flag
 * Clears DF so string instructions auto-increment their pointer registers.
 *
 * Syntax: CLD
 * Operands: implicit flags register
 * Flags affected: DF is cleared; all other flags are unchanged
 * Notes: Most modern runtimes assume DF=0 during normal execution.
 *
 * Example:
 *   cld
 */
static const char* const ASM_CLD = "cld";

/*
 * STI - Set Interrupt Flag
 * Sets IF to enable maskable interrupts after the architecturally defined interrupt shadow.
 *
 * Syntax: STI
 * Operands: implicit flags register
 * Flags affected: IF is set subject to privilege checks
 * Notes: Typically privileged or used only in low-level system code.
 *
 * Example:
 *   sti
 */
static const char* const ASM_STI = "sti";

/*
 * CLI - Clear Interrupt Flag
 * Clears IF to disable maskable interrupts.
 *
 * Syntax: CLI
 * Operands: implicit flags register
 * Flags affected: IF is cleared subject to privilege checks
 * Notes: Typically privileged or used only in low-level system code.
 *
 * Example:
 *   cli
 */
static const char* const ASM_CLI = "cli";

/*
 * LAHF - Load AH from Flags
 * Copies selected status flags into AH.
 *
 * Syntax: LAHF
 * Operands: implicit FLAGS bits -> AH
 * Flags affected: None; AH receives SF:ZF:0:AF:0:PF:1:CF
 * Notes: Useful for compact flag saving in user mode when supported.
 *
 * Example:
 *   lahf
 */
static const char* const ASM_LAHF = "lahf";

/*
 * SAHF - Store AH into Flags
 * Loads selected status flags from AH.
 *
 * Syntax: SAHF
 * Operands: implicit AH -> FLAGS bits
 * Flags affected: SF, ZF, AF, PF, and CF are loaded from AH; other flags are unchanged
 * Notes: Useful for restoring a compact saved flag subset.
 *
 * Example:
 *   sahf
 */
static const char* const ASM_SAHF = "sahf";

/*
 * SETA - Set Byte if Above / unsigned greater
 * Writes 1 to the destination byte if the condition CF=0 and ZF=0 is true; otherwise writes 0.
 *
 * Syntax: SETA r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide: 1 if CF=0 and ZF=0, otherwise 0. The flag list at the start of
 * this section is what the condition names mean. This repository's generators usually
 * lower the same test to CMP+Jcc; compilers do emit SETcc.
 *
 * Example:
 *   cmp eax, ebx
 *   seta al
 */
static const char* const ASM_SETA = "seta";

/*
 * SETAE - Set Byte if Above or Equal / unsigned >=
 * Writes 1 to the destination byte if the condition CF=0 is true; otherwise writes 0.
 *
 * Syntax: SETAE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setae al
 */
static const char* const ASM_SETAE = "setae";

/*
 * SETB - Set Byte if Below / unsigned <
 * Writes 1 to the destination byte if the condition CF=1 is true; otherwise writes 0.
 *
 * Syntax: SETB r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setb al
 */
static const char* const ASM_SETB = "setb";

/*
 * SETBE - Set Byte if Below or Equal / unsigned <=
 * Writes 1 to the destination byte if the condition CF=1 or ZF=1 is true; otherwise writes 0.
 *
 * Syntax: SETBE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setbe al
 */
static const char* const ASM_SETBE = "setbe";

/*
 * SETC - Set Byte if Carry set
 * Writes 1 to the destination byte if the condition CF=1 is true; otherwise writes 0.
 *
 * Syntax: SETC r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setc al
 */
static const char* const ASM_SETC = "setc";

/*
 * SETE - Set Byte if Equal / zero
 * Writes 1 to the destination byte if the condition ZF=1 is true; otherwise writes 0.
 *
 * Syntax: SETE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   sete al
 */
static const char* const ASM_SETE = "sete";

/*
 * SETG - Set Byte if Greater / signed >
 * Writes 1 to the destination byte if the condition ZF=0 and SF=OF is true; otherwise writes 0.
 *
 * Syntax: SETG r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setg al
 */
static const char* const ASM_SETG = "setg";

/*
 * SETGE - Set Byte if Greater or Equal / signed >=
 * Writes 1 to the destination byte if the condition SF=OF is true; otherwise writes 0.
 *
 * Syntax: SETGE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setge al
 */
static const char* const ASM_SETGE = "setge";

/*
 * SETL - Set Byte if Less / signed <
 * Writes 1 to the destination byte if the condition SF!=OF is true; otherwise writes 0.
 *
 * Syntax: SETL r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setl al
 */
static const char* const ASM_SETL = "setl";

/*
 * SETLE - Set Byte if Less or Equal / signed <=
 * Writes 1 to the destination byte if the condition ZF=1 or SF!=OF is true; otherwise writes 0.
 *
 * Syntax: SETLE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setle al
 */
static const char* const ASM_SETLE = "setle";

/*
 * SETNA - Set Byte if Not Above / unsigned <=
 * Writes 1 to the destination byte if the condition CF=1 or ZF=1 is true; otherwise writes 0.
 *
 * Syntax: SETNA r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setna al
 */
static const char* const ASM_SETNA = "setna";

/*
 * SETNAE - Set Byte if Not Above or Equal / unsigned <
 * Writes 1 to the destination byte if the condition CF=1 is true; otherwise writes 0.
 *
 * Syntax: SETNAE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnae al
 */
static const char* const ASM_SETNAE = "setnae";

/*
 * SETNB - Set Byte if Not Below / unsigned >=
 * Writes 1 to the destination byte if the condition CF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNB r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnb al
 */
static const char* const ASM_SETNB = "setnb";

/*
 * SETNBE - Set Byte if Not Below or Equal / unsigned >
 * Writes 1 to the destination byte if the condition CF=0 and ZF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNBE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnbe al
 */
static const char* const ASM_SETNBE = "setnbe";

/*
 * SETNC - Set Byte if No carry
 * Writes 1 to the destination byte if the condition CF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNC r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnc al
 */
static const char* const ASM_SETNC = "setnc";

/*
 * SETNE - Set Byte if Not Equal / nonzero
 * Writes 1 to the destination byte if the condition ZF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setne al
 */
static const char* const ASM_SETNE = "setne";

/*
 * SETNG - Set Byte if Not Greater / signed <=
 * Writes 1 to the destination byte if the condition ZF=1 or SF!=OF is true; otherwise writes 0.
 *
 * Syntax: SETNG r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setng al
 */
static const char* const ASM_SETNG = "setng";

/*
 * SETNGE - Set Byte if Not Greater or Equal / signed <
 * Writes 1 to the destination byte if the condition SF!=OF is true; otherwise writes 0.
 *
 * Syntax: SETNGE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnge al
 */
static const char* const ASM_SETNGE = "setnge";

/*
 * SETNL - Set Byte if Not Less / signed >=
 * Writes 1 to the destination byte if the condition SF=OF is true; otherwise writes 0.
 *
 * Syntax: SETNL r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnl al
 */
static const char* const ASM_SETNL = "setnl";

/*
 * SETNLE - Set Byte if Not Less or Equal / signed >
 * Writes 1 to the destination byte if the condition ZF=0 and SF=OF is true; otherwise writes 0.
 *
 * Syntax: SETNLE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnle al
 */
static const char* const ASM_SETNLE = "setnle";

/*
 * SETNO - Set Byte if No overflow
 * Writes 1 to the destination byte if the condition OF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNO r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setno al
 */
static const char* const ASM_SETNO = "setno";

/*
 * SETNP - Set Byte if Not parity / parity odd
 * Writes 1 to the destination byte if the condition PF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNP r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnp al
 */
static const char* const ASM_SETNP = "setnp";

/*
 * SETNS - Set Byte if Not sign
 * Writes 1 to the destination byte if the condition SF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNS r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setns al
 */
static const char* const ASM_SETNS = "setns";

/*
 * SETNZ - Set Byte if Not zero
 * Writes 1 to the destination byte if the condition ZF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNZ r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnz al
 */
static const char* const ASM_SETNZ = "setnz";

/*
 * SETO - Set Byte if Overflow
 * Writes 1 to the destination byte if the condition OF=1 is true; otherwise writes 0.
 *
 * Syntax: SETO r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   seto al
 */
static const char* const ASM_SETO = "seto";

/*
 * SETP - Set Byte if Parity / parity even
 * Writes 1 to the destination byte if the condition PF=1 is true; otherwise writes 0.
 *
 * Syntax: SETP r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setp al
 */
static const char* const ASM_SETP = "setp";

/*
 * SETPE - Set Byte if Parity even
 * Writes 1 to the destination byte if the condition PF=1 is true; otherwise writes 0.
 *
 * Syntax: SETPE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setpe al
 */
static const char* const ASM_SETPE = "setpe";

/*
 * SETPO - Set Byte if Parity odd
 * Writes 1 to the destination byte if the condition PF=0 is true; otherwise writes 0.
 *
 * Syntax: SETPO r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setpo al
 */
static const char* const ASM_SETPO = "setpo";

/*
 * SETS - Set Byte if Sign
 * Writes 1 to the destination byte if the condition SF=1 is true; otherwise writes 0.
 *
 * Syntax: SETS r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   sets al
 */
static const char* const ASM_SETS = "sets";

/*
 * SETZ - Set Byte if Zero
 * Writes 1 to the destination byte if the condition ZF=1 is true; otherwise writes 0.
 *
 * Syntax: SETZ r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setz al
 */
static const char* const ASM_SETZ = "setz";

/*
 * ============================================================================
 * 14. Stack Frame
 * ============================================================================
 */

/*
 * ENTER - Build Stack Frame
 * Creates a procedure stack frame by pushing the old frame pointer, establishing a new frame
 * pointer, and optionally allocating stack space and nested frame links.
 *
 * Syntax: ENTER imm16, imm8
 * Operands: frame size immediate and nesting-level immediate
 * Flags affected: None
 * Notes: ENTER is semantically rich but slower and less common than the manual PUSH/MOV/SUB
 * prologue sequence in modern code.
 *
 * Example:
 *   enter 32, 0
 */
static const char* const ASM_ENTER = "enter";

/*
 * LEAVE - Destroy Stack Frame
 * Restores the previous frame pointer and stack pointer, effectively reversing a standard ENTER
 * or PUSH/MOV frame setup.
 *
 * Syntax: LEAVE
 * Operands: implicit RBP/EBP/BP and stack pointer
 * Flags affected: None
 * Notes: LEAVE is equivalent to MOV SP, BP followed by POP BP in the active operand size.
 *
 * Example:
 *   leave
 */
static const char* const ASM_LEAVE = "leave";

/*
 * ============================================================================
 * 15. I/O Instructions
 * ============================================================================
 */

/*
 * IN - Input from Port
 * Reads data from an I/O port into AL, AX, or EAX.
 *
 * Syntax: IN accumulator, imm8|DX
 * Operands: implicit accumulator and immediate port or DX port register
 * Flags affected: None
 * Notes: Port I/O is privileged or platform-specific on many systems.
 *
 * Example:
 *   in al, dx
 */
static const char* const ASM_IN = "in";

/*
 * OUT - Output to Port
 * Writes AL, AX, or EAX to an I/O port.
 *
 * Syntax: OUT imm8|DX, accumulator
 * Operands: implicit accumulator and immediate port or DX port register
 * Flags affected: None
 * Notes: Port I/O is privileged or platform-specific on many systems.
 *
 * Example:
 *   out dx, al
 */
static const char* const ASM_OUT = "out";

/*
 * INS - Input String from Port
 * Reads from the port in DX to the memory location at ES:(E|R)DI and updates the pointer
 * according to DF.
 *
 * Syntax: INS
 * Operands: implicit DX port and ES:(E|R)DI destination
 * Flags affected: None
 * Notes: String-port input may be repeated with REP.
 *
 * Example:
 *   rep insd
 */
static const char* const ASM_INS = "ins";

/*
 * INSB - Input Byte String from Port
 * Reads one byte from the DX port into ES:(E|R)DI.
 *
 * Syntax: INSB
 * Operands: implicit DX port and destination pointer
 * Flags affected: None
 * Notes: Often used with REP for device buffers.
 *
 * Example:
 *   insb
 */
static const char* const ASM_INSB = "insb";

/*
 * INSW - Input Word String from Port
 * Reads one word from the DX port into ES:(E|R)DI.
 *
 * Syntax: INSW
 * Operands: implicit DX port and destination pointer
 * Flags affected: None
 * Notes: Pointer movement is 2 bytes per iteration.
 *
 * Example:
 *   insw
 */
static const char* const ASM_INSW = "insw";

/*
 * INSD - Input Doubleword String from Port
 * Reads one dword from the DX port into ES:(E|R)DI.
 *
 * Syntax: INSD
 * Operands: implicit DX port and destination pointer
 * Flags affected: None
 * Notes: Pointer movement is 4 bytes per iteration.
 *
 * Example:
 *   insd
 */
static const char* const ASM_INSD = "insd";

/*
 * OUTS - Output String to Port
 * Writes the memory element at DS:(E|R)SI to the port in DX and updates the source pointer
 * according to DF.
 *
 * Syntax: OUTS
 * Operands: implicit source pointer and DX port
 * Flags affected: None
 * Notes: May be repeated with REP for block output.
 *
 * Example:
 *   rep outsw
 */
static const char* const ASM_OUTS = "outs";

/*
 * OUTSB - Output Byte String to Port
 * Writes one byte from DS:(E|R)SI to the DX port.
 *
 * Syntax: OUTSB
 * Operands: implicit source pointer and DX port
 * Flags affected: None
 * Notes: Pointer movement is 1 byte per iteration.
 *
 * Example:
 *   outsb
 */
static const char* const ASM_OUTSB = "outsb";

/*
 * OUTSW - Output Word String to Port
 * Writes one word from DS:(E|R)SI to the DX port.
 *
 * Syntax: OUTSW
 * Operands: implicit source pointer and DX port
 * Flags affected: None
 * Notes: Pointer movement is 2 bytes per iteration.
 *
 * Example:
 *   outsw
 */
static const char* const ASM_OUTSW = "outsw";

/*
 * OUTSD - Output Doubleword String to Port
 * Writes one dword from DS:(E|R)SI to the DX port.
 *
 * Syntax: OUTSD
 * Operands: implicit source pointer and DX port
 * Flags affected: None
 * Notes: Pointer movement is 4 bytes per iteration.
 *
 * Example:
 *   outsd
 */
static const char* const ASM_OUTSD = "outsd";

/*
 * ============================================================================
 * 16. System Instructions
 * ============================================================================
 */

/*
 * SYSCALL - Fast System Call
 * Enters the operating system using the 64-bit fast system-call mechanism.
 *
 * Syntax: SYSCALL
 * Operands: implicit model-specific registers and calling-convention registers
 * Flags affected: RFLAGS is masked by IA32_FMASK; RCX receives the return RIP and R11 receives
 * the original RFLAGS image
 * Notes: Available only when enabled by the OS and CPU. Register conventions are ABI-specific.
 *
 * Example:
 *   syscall
 */
static const char* const ASM_SYSCALL = "syscall";

/*
 * SYSRET - Return from Fast System Call
 * Returns from a SYSCALL transition back to user mode using state prepared by the kernel.
 *
 * Syntax: SYSRET
 * Operands: implicit RCX return address and R11 flags image
 * Flags affected: Restores selected flags and transfers control
 * Notes: Typically privileged and tightly coupled to the operating system's syscall ABI.
 *
 * Example:
 *   sysret
 */
static const char* const ASM_SYSRET = "sysret";

/*
 * SYSENTER - Fast System Enter
 * Performs a fast transition to system mode using SYSENTER MSR configuration.
 *
 * Syntax: SYSENTER
 * Operands: implicit CS/EIP/ESP-style machine state via MSRs
 * Flags affected: None architecturally visible in the usual programming model
 * Notes: Common in older 32-bit kernels; ABI conventions are OS-defined.
 *
 * Example:
 *   sysenter
 */
static const char* const ASM_SYSENTER = "sysenter";

/*
 * SYSEXIT - Fast System Exit
 * Returns from a SYSENTER-style system call to less-privileged code.
 *
 * Syntax: SYSEXIT
 * Operands: implicit return state from registers and MSRs
 * Flags affected: None architecturally visible in the usual programming model
 * Notes: Primarily used by kernels that implement the SYSENTER/SYSEXIT ABI.
 *
 * Example:
 *   sysexit
 */
static const char* const ASM_SYSEXIT = "sysexit";

/*
 * INT - Software Interrupt
 * Invokes an interrupt or trap handler through the IDT using the supplied vector number.
 *
 * Syntax: INT imm8
 * Operands: 8-bit interrupt vector immediate
 * Flags affected: None directly; the processor pushes flags and state as part of exception
 * delivery
 * Notes: Used for BIOS, DOS, debugging, or software interrupt APIs in legacy environments.
 *
 * Example:
 *   int
 */
static const char* const ASM_INT = "int";

/*
 * INT3 - Breakpoint Interrupt
 * Generates breakpoint interrupt vector 3 with a dedicated one-byte encoding favored by
 * debuggers.
 *
 * Syntax: INT3
 * Operands: implicit breakpoint vector
 * Flags affected: None directly
 * Notes: Commonly patched into code for breakpoints.
 *
 * Example:
 *   int3
 */
static const char* const ASM_INT3 = "int3";

/*
 * INTO - Interrupt on Overflow
 * Triggers interrupt vector 4 if OF is set.
 *
 * Syntax: INTO
 * Operands: implicit OF test
 * Flags affected: None
 * Notes: Legacy instruction not available in 64-bit mode.
 *
 * Example:
 *   into
 */
static const char* const ASM_INTO = "into";

/*
 * IRET - Interrupt Return
 * Returns from an interrupt or exception handler by restoring IP/EIP/RIP, CS, and FLAGS state
 * from the stack.
 *
 * Syntax: IRET
 * Operands: implicit interrupt frame on the stack
 * Flags affected: Restores flags from the interrupt frame
 * Notes: Used in 16-bit and size-dependent contexts; IRETD and IRETQ make width explicit.
 *
 * Example:
 *   iret
 */
static const char* const ASM_IRET = "iret";

/*
 * IRETD - Interrupt Return Doubleword
 * Returns from a 32-bit interrupt frame.
 *
 * Syntax: IRETD
 * Operands: implicit 32-bit interrupt frame
 * Flags affected: Restores EFLAGS from the stack frame
 * Notes: Used by 32-bit handlers and system code.
 *
 * Example:
 *   iretd
 */
static const char* const ASM_IRETD = "iretd";

/*
 * IRETQ - Interrupt Return Quadword
 * Returns from a 64-bit interrupt or exception frame.
 *
 * Syntax: IRETQ
 * Operands: implicit 64-bit interrupt frame
 * Flags affected: Restores RFLAGS from the stack frame
 * Notes: Used by 64-bit kernels, hypervisors, and low-level handlers.
 *
 * Example:
 *   iretq
 */
static const char* const ASM_IRETQ = "iretq";

/*
 * HLT - Halt Processor
 * Stops instruction execution until an interrupt, NMI, SMI, INIT, or reset event occurs.
 *
 * Syntax: HLT
 * Operands: implicit processor state
 * Flags affected: None
 * Notes: Privileged on most systems and typically used only by kernels or firmware idle loops.
 *
 * Example:
 *   hlt
 */
static const char* const ASM_HLT = "hlt";

/*
 * NOP - No Operation
 * Consumes instruction bytes and execution resources without changing the architectural state
 * used by general code.
 *
 * Syntax: NOP
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Used for alignment, patch space, timing, and placeholder instructions.
 *
 * Example:
 *   nop
 */
static const char* const ASM_NOP = "nop";

/*
 * PAUSE - Spin-Wait Hint
 * Provides a hint to the processor that the code is in a spin-wait loop.
 *
 * Syntax: PAUSE
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Modern code uses PAUSE inside user-mode and kernel spin loops. It tells the CPU
 * this thread is waiting, which reduces power and stops the pipeline from speculating a
 * load that will miss again (the "memory-order machine clear" penalty on a tight lock
 * poll). It is not a fence and it does not sleep the thread. Put it in the body of a
 * loop that is re-reading a flag or a lock word.
 *
 * Example:
 * spin:
 *   pause
 *   cmp dword ptr [lock], 0
 *   jne spin
 */
static const char* const ASM_PAUSE = "pause";

/*
 * MONITOR and MWAIT are the SSE3 privileged wait pair (CPUID.01H:ECX[3]).
 * UMONITOR, UMWAIT, and TPAUSE are the user-mode WAITPKG forms
 * (CPUID.07H:ECX[5]). None of them is a locked read-modify-write. PAUSE,
 * above, is the spin-loop hint that does not sleep.
 */

/*
 * MONITOR - Set Up Monitor Address
 * Arms the monitor hardware to watch a linear address range for stores from other agents.
 *
 * Syntax: MONITOR rax, rcx, rdx
 * Operands: RAX holds the linear address. The low 32 bits of RCX are the extensions and the
 * low 32 bits of RDX are the hints. ML64 requires these 64-bit names; the encoding has no
 * register field.
 * Flags affected: None.
 * Notes: SSE3. Privileged (CPL 0). A store to the armed range is what completes a following
 * MWAIT. The address is linear. User-mode code uses UMONITOR instead.
 *
 * Example:
 *   mov rax, rbx
 *   xor rcx, rcx
 *   xor rdx, rdx
 *   monitor rax, rcx, rdx
 */
static const char* const ASM_MONITOR = "monitor";

/*
 * MWAIT - Monitor Wait
 * Stops the processor until a store hits the address armed by MONITOR, or until an interrupt
 * or timeout policy wakes it.
 *
 * Syntax: MWAIT rax, rcx
 * Operands: The low 32 bits of RAX are the C-state hint. The low 32 bits of RCX are the
 * extensions. Bit 0 of RCX, when set, treats masked interrupts as break events. ML64 requires
 * the 64-bit names; the encoding has no register field.
 * Flags affected: None.
 * Notes: SSE3. Privileged (CPL 0). MONITOR must have armed the address first. This is not
 * PAUSE and it is not a lock. User-mode code uses UMWAIT.
 *
 * Example:
 *   xor rax, rax
 *   xor rcx, rcx
 *   mwait rax, rcx
 */
static const char* const ASM_MWAIT = "mwait";

/*
 * UMONITOR - User-Mode Monitor Address
 * Arms the user-mode monitor to watch the linear address in the source register.
 *
 * Syntax: UMONITOR r32|r64
 * Operands: One general-purpose register holding the address.
 * Flags affected: None.
 * Notes: WAITPKG. Legal at user privilege, unlike MONITOR. A later UMWAIT wakes when another
 * agent stores to the armed range. The address should be naturally aligned for the width you
 * are publishing.
 *
 * Example:
 *   umonitor rax
 */
static const char* const ASM_UMONITOR = "umonitor";

/*
 * UMWAIT - User-Mode Monitor Wait
 * Waits until a store hits the UMONITOR address, or until the Time-Stamp Counter reaches the
 * deadline in EDX:EAX.
 *
 * Syntax: UMWAIT r32
 * Operands: The register operand is a hint and is ECX in the documented form. EDX:EAX holds
 * the TSC deadline.
 * Flags affected: CF=1 if the deadline was reached. CF=0 if a store or another break ended the
 * wait. The other arithmetic flags are cleared.
 * Notes: WAITPKG. UMONITOR arms the address. Bit 0 of ECX selects between the two user-mode
 * wait implementations. The deadline is EDX:EAX even in 64-bit mode, not RDX. This does not
 * lock the watched memory.
 *
 * Example:
 *   umwait ecx
 */
static const char* const ASM_UMWAIT = "umwait";

/*
 * TPAUSE - Timed Pause
 * Waits until the Time-Stamp Counter reaches the deadline in EDX:EAX, without an armed monitor
 * address.
 *
 * Syntax: TPAUSE r32
 * Operands: The register operand is a hint and is ECX in the documented form. EDX:EAX holds
 * the TSC deadline.
 * Flags affected: CF=1 if the deadline was reached. CF=0 if the wait ended early. The other
 * arithmetic flags are cleared.
 * Notes: WAITPKG. Unlike UMWAIT, no UMONITOR is required. Bit 0 of ECX selects the wait
 * implementation. Use it for a short timed backoff. It is not a fence and it does not observe
 * a lock word by itself.
 *
 * Example:
 *   tpause ecx
 */
static const char* const ASM_TPAUSE = "tpause";

/*
 * UD2 - Undefined Instruction Trap
 * Intentionally triggers an invalid-opcode exception.
 *
 * Syntax: UD2
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Useful for guaranteed traps, unreachable markers, and sanitizer/debug instrumentation.
 *
 * Example:
 *   ud2
 */
static const char* const ASM_UD2 = "ud2";

/*
 * CPUID - Query CPU Identification
 * Returns processor identification and feature information based on the requested leaf values in
 * EAX/ECX.
 *
 * Syntax: CPUID
 * Operands: implicit EAX/ECX inputs and EAX/EBX/ECX/EDX outputs
 * Flags affected: None
 * Notes: Serializing instruction often used for feature detection and vendor strings.
 *
 * Example:
 *   cpuid
 */
static const char* const ASM_CPUID = "cpuid";

/*
 * RDTSC - Read Time-Stamp Counter
 * Reads the time-stamp counter into EDX:EAX or RDX:RAX.
 *
 * Syntax: RDTSC
 * Operands: implicit counter destination registers
 * Flags affected: None
 * Notes: Not fully serializing by itself; LFENCE or CPUID may be used around it depending on the
 * required ordering.
 *
 * Example:
 *   rdtsc
 */
static const char* const ASM_RDTSC = "rdtsc";

/*
 * RDTSCP - Read Time-Stamp Counter and Processor ID
 * Reads the time-stamp counter and auxiliary processor ID in a partially serializing manner.
 *
 * Syntax: RDTSCP
 * Operands: implicit EDX:EAX/RDX:RAX result and ECX auxiliary output
 * Flags affected: None
 * Notes: Useful for higher-quality timing and core identification than bare RDTSC.
 *
 * Example:
 *   rdtscp
 */
static const char* const ASM_RDTSCP = "rdtscp";

/*
 * RDRAND - Read Random Number
 * Writes a hardware random number into the destination register when the DRNG can supply one.
 *
 * Syntax: RDRAND r16|r32|r64
 * Operands: One general-purpose register destination.
 * Flags affected: CF=1 if the register received a random value. CF=0 if it did not; the
 * destination is unchanged. OF, SF, ZF, AF, and PF are undefined.
 * Notes: CPUID.01H:ECX.RDRAND[bit 30]. A 0 in CF means retry. This is a
 * deterministic-random-number-generator result, not a fresh entropy sample. RDSEED is the
 * instruction that draws seed material.
 *
 * Example:
 *   rdrand rax
 */
static const char* const ASM_RDRAND = "rdrand";

/*
 * RDSEED - Read Random Seed
 * Writes a conditioned entropy sample into the destination register when the entropy source
 * can supply one.
 *
 * Syntax: RDSEED r16|r32|r64
 * Operands: One general-purpose register destination.
 * Flags affected: CF=1 if the register received a seed. CF=0 if it did not; the destination is
 * unchanged. OF, SF, ZF, AF, and PF are undefined.
 * Notes: CPUID.07H:EBX.RDSEED[bit 18]. Use it to seed a software generator. A tight retry loop
 * is the wrong consumer: a failure means the entropy source is not ready yet. RDRAND is the
 * cheaper ongoing random stream.
 *
 * Example:
 *   rdseed rax
 */
static const char* const ASM_RDSEED = "rdseed";

/*
 * RDMSR - Read Model-Specific Register
 * Reads the MSR selected by ECX into EDX:EAX.
 *
 * Syntax: RDMSR
 * Operands: implicit ECX selector and EDX:EAX result
 * Flags affected: None
 * Notes: Privileged instruction used by kernels and hypervisors.
 *
 * Example:
 *   rdmsr
 */
static const char* const ASM_RDMSR = "rdmsr";

/*
 * WRMSR - Write Model-Specific Register
 * Writes EDX:EAX to the MSR selected by ECX.
 *
 * Syntax: WRMSR
 * Operands: implicit ECX selector and EDX:EAX value
 * Flags affected: None
 * Notes: Privileged and potentially dangerous because MSRs control key processor behavior.
 *
 * Example:
 *   wrmsr
 */
static const char* const ASM_WRMSR = "wrmsr";

/*
 * RDPMC - Read Performance-Monitor Counter
 * Reads a performance-monitoring counter selected by ECX.
 *
 * Syntax: RDPMC
 * Operands: implicit ECX selector and EDX:EAX result
 * Flags affected: None
 * Notes: Availability in user mode depends on CR4.PCE and platform policy.
 *
 * Example:
 *   rdpmc
 */
static const char* const ASM_RDPMC = "rdpmc";

/*
 * XGETBV - Read Extended Control Register
 * Reads XCR state selected by ECX into EDX:EAX.
 *
 * Syntax: XGETBV
 * Operands: implicit ECX selector and EDX:EAX result
 * Flags affected: None
 * Notes: Used to query OS-enabled extended state such as SSE and AVX support.
 *
 * Example:
 *   xgetbv
 */
static const char* const ASM_XGETBV = "xgetbv";

/*
 * XSETBV - Write Extended Control Register
 * Writes EDX:EAX into the XCR selected by ECX.
 *
 * Syntax: XSETBV
 * Operands: implicit ECX selector and EDX:EAX value
 * Flags affected: None
 * Notes: Privileged or otherwise restricted; used by kernels to enable XSAVE-managed state.
 *
 * Example:
 *   xsetbv
 */
static const char* const ASM_XSETBV = "xsetbv";

/*
 * INVD - Invalidate Caches without Writeback
 * Invalidates internal caches without first writing back modified lines.
 *
 * Syntax: INVD
 * Operands: implicit cache hierarchy state
 * Flags affected: None
 * Notes: Privileged. It drops every dirty cache line instead of writing it back to memory,
 * so data that existed only in the cache is lost. It comes up in firmware reset paths and
 * in a few platform sequences where RAM is about to be discarded or is known to be
 * incoherent. Normal kernels use WBINVD when they must flush, and ordinary programs use
 * neither. Executing INVD from user mode raises a general-protection fault.
 *
 * Example:
 *   invd
 */
static const char* const ASM_INVD = "invd";

/*
 * WBINVD - Write Back and Invalidate Cache
 * Writes back modified cache lines and invalidates internal caches.
 *
 * Syntax: WBINVD
 * Operands: implicit cache hierarchy state
 * Flags affected: None
 * Notes: Privileged and used for cache management in firmware, kernels, or platform code.
 *
 * Example:
 *   wbinvd
 */
static const char* const ASM_WBINVD = "wbinvd";

/*
 * INVLPG - Invalidate TLB Entry
 * Invalidates the translation lookaside buffer entry for the specified linear address.
 *
 * Syntax: INVLPG mem
 * Operands: memory operand supplying the linear address
 * Flags affected: None
 * Notes: Privileged instruction commonly used after page-table updates.
 *
 * Example:
 *   invlpg
 */
static const char* const ASM_INVLPG = "invlpg";

/*
 * CLFLUSH - Cache Line Flush
 * Writes back and invalidates the cache line containing the specified memory operand from all
 * cache levels coherent with the current core.
 *
 * Syntax: CLFLUSH mem
 * Operands: memory operand naming the cache line
 * Flags affected: None
 * Notes: Used in persistence, cache-control, and some side-channel research code.
 *
 * Example:
 *   clflush
 */
static const char* const ASM_CLFLUSH = "clflush";

/*
 * CLFLUSHOPT - Optimized Cache Line Flush
 * Like CLFLUSH but permits more implementation flexibility and batching.
 *
 * Syntax: CLFLUSHOPT mem
 * Operands: memory operand naming the cache line
 * Flags affected: None
 * Notes: Often paired with SFENCE for ordering when used for persistent memory.
 *
 * Example:
 *   clflushopt
 */
static const char* const ASM_CLFLUSHOPT = "clflushopt";

/*
 * LFENCE - Load Fence
 * Prevents later loads from being reordered before earlier loads.
 *
 * Syntax: LFENCE
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: LFENCE orders loads: a load after the fence cannot pass a load before it. On Intel
 * it also serializes the instruction stream, which is why timing code puts it around RDTSC.
 * It does not order stores. Integer atomicity is the separate note at the start of section 8;
 * a plain aligned MOV is atomic but not ordered, and LFENCE is one way to order the loads.
 *
 * Example:
 *   lfence
 *   rdtsc
 *   lfence
 *   mov eax, dword ptr [rsi]
 *   lfence
 *   rdtsc
 */
static const char* const ASM_LFENCE = "lfence";

/*
 * SFENCE - Store Fence
 * Prevents later stores from being reordered before earlier stores.
 *
 * Syntax: SFENCE
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: SFENCE orders stores. It matters after a non-temporal store (MOVNTDQ, MOVNTI),
 * which is allowed to become visible later than an ordinary store. The flag store below is
 * not visible to another thread before the streaming store that precedes the fence. SFENCE
 * does not order loads.
 *
 * Example:
 *   movntdq xmmword ptr [rdi], xmm0
 *   sfence
 *   mov byte ptr [flag], 1
 */
static const char* const ASM_SFENCE = "sfence";

/*
 * MFENCE - Memory Fence
 * Prevents later memory operations from being reordered before earlier ones.
 *
 * Syntax: MFENCE
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: MFENCE is a full barrier: every load and store before it is globally visible before
 * any load or store after it. Use it when a plain store (including a non-temporal one) must
 * be ordered both ways. A LOCK-prefixed read-modify-write is already totally ordered with
 * other LOCKed operations; see section 8. MFENCE is the explicit fence when you did not use
 * LOCK. SSE vector stores are not made atomic by MFENCE; the fence only orders them.
 *
 * Example:
 *   mov dword ptr [shared], eax
 *   mfence
 *   mov dword ptr [flag], 1
 */
static const char* const ASM_MFENCE = "mfence";

/*
 * PREFETCH - Prefetch Data
 * Hints that the addressed memory should be fetched into cache before it is needed.
 *
 * Syntax: PREFETCH mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: The argument is a normal memory address, a hint, not a cache-line number. The CPU
 * rounds that address down to the line that contains it and may start filling the line.
 * Hardware prefetchers already do this for sequential streams. Software prefetch helps when
 * the next address is data-dependent (a pointer chase, a hash probe) and you can name it
 * many instructions before the real load. The CPU is free to ignore the hint. This generic
 * spelling is the older 3DNow form; Intel's locality hints are PREFETCHT0, PREFETCHT1,
 * PREFETCHT2, and PREFETCHNTA. Modern compilers still emit those when a loop's next
 * iteration is far enough ahead to hide latency.
 *
 * Example:
 *   prefetch byte ptr [rsi+64]
 */
static const char* const ASM_PREFETCH = "prefetch";

/*
 * PREFETCHW - Prefetch for Write
 * Hints that the addressed memory will be written soon.
 *
 * Syntax: PREFETCHW mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Same address-hint rule as PREFETCH. The extra request is to bring the line in a
 * state that allows a store (exclusive or modified) so the later write does not wait on a
 * read-for-ownership. That helps before updating a hot shared counter or filling a node you
 * are about to mutate. It is still a hint.
 *
 * Example:
 *   prefetchw byte ptr [rdi]
 *   add dword ptr [rdi], 1
 */
static const char* const ASM_PREFETCHW = "prefetchw";

/*
 * PREFETCHNTA - Non-Temporal Prefetch
 * Prefetches data with a non-temporal locality hint.
 *
 * Syntax: PREFETCHNTA mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: "Non-temporal" here means "do not keep this"; "prefetch" means "start the fill
 * early". Both are true at once: the line is brought in, but it is marked as the first
 * victim so a streaming buffer that will be used once does not push useful data out of the
 * cache. The argument is a memory address, not a line number. The fill may use only one
 * way of a set. It is not the same as a non-temporal store, which avoids allocating a line
 * for data you are writing.
 *
 * Example:
 *   prefetchnta byte ptr [rsi]
 *   movdqa xmm0, xmmword ptr [rsi]
 */
static const char* const ASM_PREFETCHNTA = "prefetchnta";

/*
 * PREFETCHT0 - Temporal Prefetch T0
 * Prefetches data with high temporal locality, favoring close cache levels.
 *
 * Syntax: PREFETCHT0 mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: The argument is the address of the data, not a cache-line index. T0, T1, and T2
 * are hints about which level to bring that line into: approximately L1, L2, and L3. The
 * exact mapping is CPU-specific, not a guarantee. T0 asks for every level, including L1,
 * because the data will be reused soon. Ordinary loads also allocate in the cache; the
 * point of the prefetch is to start that fill before the load that actually needs the
 * bytes, so the load does not stall.
 *
 * Example:
 *   prefetcht0 byte ptr [rsi+128]
 *   mov eax, dword ptr [rsi]
 */
static const char* const ASM_PREFETCHT0 = "prefetcht0";

/*
 * PREFETCHT1 - Temporal Prefetch T1
 * Prefetches data with a moderate temporal-locality hint.
 *
 * Syntax: PREFETCHT1 mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Address hint, same as PREFETCHT0. T1 asks for a middle level, approximately L2,
 * and not necessarily L1. The mapping is CPU-specific. Use it for data you will touch, but
 * not on the very next instruction, so it should not displace the L1 working set.
 *
 * Example:
 *   prefetcht1 byte ptr [rbx]
 */
static const char* const ASM_PREFETCHT1 = "prefetcht1";

/*
 * PREFETCHT2 - Temporal Prefetch T2
 * Prefetches data with a more distant temporal-locality hint.
 *
 * Syntax: PREFETCHT2 mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Address hint, same as PREFETCHT0. T2 is the furthest of the three, approximately
 * L3, and the processor may keep the line out of L1 and L2. The mapping is CPU-specific,
 * not a promise that level 2 of the hint equals cache level 2 on every chip.
 *
 * Example:
 *   prefetcht2 byte ptr [rbx+4096]
 */
static const char* const ASM_PREFETCHT2 = "prefetcht2";

/*
 * CLTS - Clear Task-Switched Flag
 * Clears CR0.TS so x87/SSE instructions can execute without a device-not-available fault.
 *
 * Syntax: CLTS
 * Operands: implicit CR0 state
 * Flags affected: None
 * Notes: Device-not-available (#NM) is raised when CR0.TS is set and software executes an
 * x87 or SSE instruction. The kernel sets TS so it can lazy-switch FPU/SSE state: a thread
 * that never touches those registers costs nothing at context-switch time. The first
 * x87/SSE instruction in the thread faults, the kernel restores that thread's FP context,
 * CLTS clears TS, and the instruction is restarted. That is why the fault exists. CLTS is
 * privileged. User code never executes it; it only causes the fault the kernel handles.
 *
 * Example:
 *   clts
 */
static const char* const ASM_CLTS = "clts";

/*
 * LGDT - Load Global Descriptor Table Register
 * Loads the GDTR from a memory operand.
 *
 * Syntax: LGDT mem
 * Operands: memory operand containing a descriptor-table pseudo-descriptor
 * Flags affected: None
 * Notes: The memory operand is a 6-byte (or 10-byte in long mode) pseudo-descriptor: a 16-bit
 * limit and the linear base of the GDT. Boot code and hypervisors execute LGDT once to
 * install that table. Application code does not. In 64-bit mode segmentation is mostly flat,
 * but the GDT still holds the kernel and user code and data selectors and the TSS descriptor.
 *
 * Example:
 *   lgdt fword ptr [gdtr]
 */
static const char* const ASM_LGDT = "lgdt";

/*
 * SGDT - Store Global Descriptor Table Register
 * Stores the current GDTR to memory.
 *
 * Syntax: SGDT mem
 * Operands: memory destination operand
 * Flags affected: None
 * Notes: Writes the current limit and base back to a pseudo-descriptor in memory. A
 * hypervisor or a debugger uses it to see which GDT is installed. It does not copy the
 * descriptor table itself, only the register that points at it.
 *
 * Example:
 *   sgdt fword ptr [buf]
 */
static const char* const ASM_SGDT = "sgdt";

/*
 * LIDT - Load Interrupt Descriptor Table Register
 * Loads the IDTR from a memory operand.
 *
 * Syntax: LIDT mem
 * Operands: memory operand containing an IDT pseudo-descriptor
 * Flags affected: None
 * Notes: Same pseudo-descriptor shape as LGDT, but the table is the interrupt descriptor
 * table. The kernel loads it so exceptions and hardware interrupts vector to the right
 * gates. User code does not execute LIDT. SIDT stores the current IDTR for diagnostics.
 *
 * Example:
 *   lidt fword ptr [idtr]
 */
static const char* const ASM_LIDT = "lidt";

/*
 * SIDT - Store Interrupt Descriptor Table Register
 * Stores the current IDTR to memory.
 *
 * Syntax: SIDT mem
 * Operands: memory destination operand
 * Flags affected: None
 * Notes: Used for diagnostics and virtualization-related code.
 *
 * Example:
 *   sidt
 */
static const char* const ASM_SIDT = "sidt";

/*
 * LLDT - Load Local Descriptor Table Register
 * Loads the LDTR selector from a register or memory operand.
 *
 * Syntax: LLDT r/m16
 * Operands: 16-bit register or memory selector
 * Flags affected: None
 * Notes: Privileged instruction for segmented protected-mode task environments.
 *
 * Example:
 *   lldt
 */
static const char* const ASM_LLDT = "lldt";

/*
 * SLDT - Store Local Descriptor Table Register
 * Stores the LDTR selector into a register or memory operand.
 *
 * Syntax: SLDT r/m16
 * Operands: 16-bit register or memory destination
 * Flags affected: None
 * Notes: Mostly relevant to system software or emulators.
 *
 * Example:
 *   sldt
 */
static const char* const ASM_SLDT = "sldt";

/*
 * LTR - Load Task Register
 * Loads the task register with a TSS selector.
 *
 * Syntax: LTR r/m16
 * Operands: 16-bit register or memory selector
 * Flags affected: None
 * Notes: The operand is a selector in the GDT that names a task-state segment. In 64-bit
 * mode the CPU does not hardware-switch tasks, but the TSS is still required: privilege
 * changes and interrupts load RSP from it. The kernel executes LTR once per CPU at startup
 * after that descriptor is in the GDT. It is privileged.
 *
 * Example:
 *   ltr ax
 */
static const char* const ASM_LTR = "ltr";

/*
 * STR - Store Task Register
 * Stores the task-register selector to a register or memory operand.
 *
 * Syntax: STR r/m16
 * Operands: 16-bit register or memory destination
 * Flags affected: None
 * Notes: Mostly relevant to low-level system software.
 *
 * Example:
 *   str
 */
static const char* const ASM_STR = "str";

/*
 * LMSW - Load Machine Status Word
 * Loads selected low CR0 bits from the source operand.
 *
 * Syntax: LMSW r/m16
 * Operands: 16-bit register or memory source
 * Flags affected: None
 * Notes: 80286-era instruction. It writes the low four bits of CR0 (the machine status word)
 * and it cannot clear PE once protected mode is on. Modern code uses `mov rax, cr0` and
 * `mov cr0, rax` instead, which can see the whole register. You meet LMSW only in ancient
 * real-to-protected transition code. It is privileged.
 *
 * Example:
 *   lmsw ax
 */
static const char* const ASM_LMSW = "lmsw";

/*
 * SMSW - Store Machine Status Word
 * Stores the machine-status word derived from CR0 into the destination operand.
 *
 * Syntax: SMSW r/m16|r32|r64
 * Operands: register or memory destination
 * Flags affected: None
 * Notes: Useful mainly to low-level software investigating control state.
 *
 * Example:
 *   smsw
 */
static const char* const ASM_SMSW = "smsw";

/*
 * SWAPGS - Swap Kernel and User GS Base
 * Exchanges the current GS base with the kernel GS-base MSR.
 *
 * Syntax: SWAPGS
 * Operands: implicit GS base state
 * Flags affected: None
 * Notes: SYSCALL does not switch GS. On kernel entry the 64-bit kernel executes SWAPGS so
 * that `gs:` addresses the per-CPU kernel block instead of the user TEB; on the way back it
 * executes SWAPGS again. The two bases live in MSRs (GS_BASE and KERNEL_GS_BASE). User code
 * must not execute it; it is privileged and will fault.
 *
 * Example:
 *   swapgs
 *   mov rsp, gs:[8]
 */
static const char* const ASM_SWAPGS = "swapgs";

/*
 * XSAVE - Save Extended Processor State
 * Stores x87, SSE, AVX, and other enabled extended state components to an XSAVE area according
 * to XCR0 and EDX:EAX masks.
 *
 * Syntax: XSAVE mem
 * Operands: memory destination and implicit EDX:EAX component mask
 * Flags affected: May update modified/compacted tracking state; does not use normal arithmetic
 * flags
 * Notes: The operand is a 64-byte-aligned memory buffer, not a register. EDX:EAX is a mask of
 * which state components to write (x87, SSE, AVX, and later extensions the OS enabled in
 * XCR0). The kernel uses XSAVE, or the optimized variants, on a context switch when it must
 * keep AVX state as well as x87 and XMM. A bit of 0 in the mask means "leave that component
 * out". Application code normally lets the OS do this; see also the #NM note on CLTS.
 *
 * Example:
 *   mov eax, 7
 *   xor edx, edx
 *   xsave xmmword ptr [rbx]
 */
static const char* const ASM_XSAVE = "xsave";

/*
 * XRSTOR - Restore Extended Processor State
 * Restores enabled processor state components from an XSAVE area.
 *
 * Syntax: XRSTOR mem
 * Operands: memory source and implicit EDX:EAX component mask
 * Flags affected: May update internal state-tracking metadata; no ordinary arithmetic flags
 * semantics
 * Notes: Used with XSAVE-family save areas.
 *
 * Example:
 *   xrstor
 */
static const char* const ASM_XRSTOR = "xrstor";

/*
 * XSAVEOPT - Optimized Save Extended State
 * Like XSAVE but may skip components that are already in memory and unchanged.
 *
 * Syntax: XSAVEOPT mem
 * Operands: memory destination and implicit EDX:EAX component mask
 * Flags affected: No ordinary arithmetic flags semantics
 * Notes: Same buffer and EDX:EAX mask as XSAVE. The processor tracks which components were
 * modified since the last restore and may skip writing a component that still matches
 * memory. Kernels use it to make a context switch cheaper when a thread did not touch AVX.
 * The buffer must still be 64-byte aligned, and the instruction is only available when
 * CPUID says so. It is an OS primitive, not something a normal function emits.
 *
 * Example:
 *   mov eax, 7
 *   xor edx, edx
 *   xsaveopt xmmword ptr [rbx]
 */
static const char* const ASM_XSAVEOPT = "xsaveopt";

/*
 * ============================================================================
 * 17. CRC and AES Instructions
 * ============================================================================
 *
 * These are optional. Check CPUID (SSE4.2 for CRC32, PCLMULQDQ for the carry-less
 * multiply, AES-NI for the AES round instructions, SHA for the SHA-1 and
 * SHA-256 rounds) before using them. They are
 * worth using when the CPU has the feature and you are hashing or encrypting bulk
 * data: one instruction replaces a byte-wise table lookup, and the difference is
 * large. They do not replace a complete AES or CRC implementation by themselves.
 * AESENC and AESDEC are one round; the key schedule is AESKEYGENASSIST plus
 * shifts and xors; PCLMULQDQ and CRC32 are the CRC and GHASH side.
 */
/*
 * CRC32 - CRC-32 Accumulate
 * Computes a CRC-32C update from the destination accumulator and the source operand.
 *
 * Syntax: CRC32 reg, r/m8|r/m16|r/m32|r/m64
 * Operands: destination register accumulator and source register or memory operand
 * Flags affected: OF, SF, ZF, AF, CF, and PF are undefined
 * Notes: One step of CRC-32C (the Castagnoli polynomial, not the Ethernet/ZIP CRC-32).
 * The destination is the running remainder; the source is the next byte, word, dword, or
 * qword to fold in. A buffer loop is `crc32 eax, dword ptr [rsi]` plus an advance of the
 * pointer. Much faster than a byte table when SSE4.2 is present.
 *
 * Example:
 *   crc32 eax, byte ptr [rsi]
 */
static const char* const ASM_CRC32 = "crc32";

/*
 * AESENC - AES Round Encrypt
 * Performs one standard AES encryption round on the state using the round key operand.
 *
 * Syntax: AESENC xmm1, xmm2/m128
 * Operands: destination/source XMM state and round-key operand
 * Flags affected: None
 * Notes: One middle AES encrypt round: ShiftRows, SubBytes, MixColumns, then AddRoundKey
 * with the second operand. AES-128 uses nine of these between the initial AddRoundKey and
 * AESENCLAST. The XMM register holds one 128-bit block. AES-NI only; check CPUID.
 *
 * Example:
 *   aesenc xmm0, xmm1
 */
static const char* const ASM_AESENC = "aesenc";

/*
 * AESENCLAST - AES Final Encrypt Round
 * Performs the final AES encryption round without MixColumns.
 *
 * Syntax: AESENCLAST xmm1, xmm2/m128
 * Operands: destination/source XMM state and round-key operand
 * Flags affected: None
 * Notes: The last encrypt round: ShiftRows, SubBytes, and AddRoundKey, with no MixColumns.
 * It follows the AESENC rounds and uses the last round key. After it, xmm0 holds the
 * ciphertext block.
 *
 * Example:
 *   aesenclast xmm0, xmm1
 */
static const char* const ASM_AESENCLAST = "aesenclast";

/*
 * AESDEC - AES Round Decrypt
 * Performs one standard AES decryption round on the state using the round key operand.
 *
 * Syntax: AESDEC xmm1, xmm2/m128
 * Operands: destination/source XMM state and round-key operand
 * Flags affected: None
 * Notes: One middle AES decrypt round: InvShiftRows, InvSubBytes, InvMixColumns, and
 * AddRoundKey with the second operand. It is the inverse of AESENC and is used the same
 * way, once per middle round, after the equivalent inverse cipher key schedule.
 *
 * Example:
 *   aesdec xmm0, xmm1
 */
static const char* const ASM_AESDEC = "aesdec";

/*
 * AESDECLAST - AES Final Decrypt Round
 * Performs the final AES decryption round without InvMixColumns.
 *
 * Syntax: AESDECLAST xmm1, xmm2/m128
 * Operands: destination/source XMM state and round-key operand
 * Flags affected: None
 * Notes: The last decrypt round: InvShiftRows, InvSubBytes, and AddRoundKey, with no
 * InvMixColumns. It follows the AESDEC rounds. The block in the destination is then plaintext.
 *
 * Example:
 *   aesdeclast xmm0, xmm1
 */
static const char* const ASM_AESDECLAST = "aesdeclast";

/*
 * AESIMC - AES Inverse MixColumns
 * Applies the inverse MixColumns transform used to prepare decryption round keys.
 *
 * Syntax: AESIMC xmm1, xmm2/m128
 * Operands: destination XMM register and source XMM/memory operand
 * Flags affected: None
 * Notes: Applies Inverse MixColumns to one round key. The decrypt key schedule is the encrypt
 * round keys with this transform on every key except the first and the last. It is a setup
 * step, not a data round.
 *
 * Example:
 *   aesimc xmm2, xmm1
 */
static const char* const ASM_AESIMC = "aesimc";

/*
 * AESKEYGENASSIST - AES Key Generation Assist
 * Generates round-key material to assist software AES key expansion.
 *
 * Syntax: AESKEYGENASSIST xmm1, xmm2/m128, imm8
 * Operands: destination XMM register, source XMM/memory operand, and round constant immediate
 * Flags affected: None
 * Notes: One assist step of the AES key expansion: RotWord, SubWord, and the round constant
 * in the imm8. Software still shifts and xors the result with the previous key word to
 * finish the next round key. The imm8 is the Rcon for this round, not a data byte.
 *
 * Example:
 *   aeskeygenassist xmm1, xmm0, 1
 */
static const char* const ASM_AESKEYGENASSIST = "aeskeygenassist";

/*
 * PCLMULQDQ - Carry-Less Multiply
 * Performs polynomial carry-less multiplication on selected 64-bit lanes from two 128-bit
 * operands.
 *
 * Syntax: PCLMULQDQ xmm1, xmm2/m128, imm8
 * Operands: destination/source XMM register, source XMM/memory operand, and lane-select
 * immediate
 * Flags affected: None
 * Notes: Carry-less (polynomial) multiply of one 64-bit half of each operand. The imm8
 * selects the halves: 0 is low times low. That product is the folding step inside CRC
 * computation and inside AES-GCM's GHASH. Together with CRC32 it is the CRC side of this
 * section. Optional; check the PCLMULQDQ CPUID bit. Much faster than a bit-by-bit polynomial
 * multiply.
 *
 * Example:
 *   pclmulqdq xmm0, xmm1, 0
 */
static const char* const ASM_PCLMULQDQ = "pclmulqdq";

/*
 * SHA extensions. CPUID.07H:EBX.SHA[bit 29]. These update one round of the
 * SHA-1 or SHA-256 compression or message schedule. They do not hash a
 * buffer by themselves. SHA-256 rounds read the message words from implicit
 * XMM0.
 */

/*
 * SHA1RNDS4 - SHA-1 Four Rounds
 * Performs four SHA-1 rounds on the state in the destination using the message and a
 * round-group constant.
 *
 * Syntax: SHA1RNDS4 xmm, xmm/m128, imm8
 * Operands: XMM state destination/source, XMM or memory message, and an immediate round group.
 * Flags affected: None.
 * Notes: SHA-NI. The low 2 bits of imm8 select round group 0, 1, 2, or 3, which picks the
 * SHA-1 logic function and constant. The destination holds the SHA-1 ABCD state.
 *
 * Example:
 *   sha1rnds4 xmm0, xmm1, 0
 */
static const char* const ASM_SHA1RNDS4 = "sha1rnds4";

/*
 * SHA1NEXTE - SHA-1 Next E State
 * Adds the next message-schedule word into the SHA-1 E state.
 *
 * Syntax: SHA1NEXTE xmm, xmm/m128
 * Operands: XMM destination/source and XMM or memory message word.
 * Flags affected: None.
 * Notes: SHA-NI. Used between SHA1RNDS4 steps to rotate the working E value and add the
 * incoming schedule word.
 *
 * Example:
 *   sha1nexte xmm0, xmm1
 */
static const char* const ASM_SHA1NEXTE = "sha1nexte";

/*
 * SHA1MSG1 - SHA-1 Message Schedule, First Step
 * Performs the first part of a SHA-1 message-schedule expansion.
 *
 * Syntax: SHA1MSG1 xmm, xmm/m128
 * Operands: XMM destination/source and XMM or memory source.
 * Flags affected: None.
 * Notes: SHA-NI. Combines schedule words with the rotate-and-xor that SHA-1 uses to derive
 * later W values. SHA1MSG2 finishes the expansion.
 *
 * Example:
 *   sha1msg1 xmm0, xmm1
 */
static const char* const ASM_SHA1MSG1 = "sha1msg1";

/*
 * SHA1MSG2 - SHA-1 Message Schedule, Second Step
 * Finishes a SHA-1 message-schedule expansion step.
 *
 * Syntax: SHA1MSG2 xmm, xmm/m128
 * Operands: XMM destination/source and XMM or memory source.
 * Flags affected: None.
 * Notes: SHA-NI. The pair SHA1MSG1 then SHA1MSG2 replaces the scalar rotate/xor sequence that
 * builds W[t] from earlier words.
 *
 * Example:
 *   sha1msg2 xmm0, xmm1
 */
static const char* const ASM_SHA1MSG2 = "sha1msg2";

/*
 * SHA256MSG1 - SHA-256 Message Schedule, First Step
 * Performs the first part of a SHA-256 message-schedule expansion.
 *
 * Syntax: SHA256MSG1 xmm, xmm/m128
 * Operands: XMM destination/source and XMM or memory source.
 * Flags affected: None.
 * Notes: SHA-NI. Applies the SHA-256 sigma0 schedule function across the dwords. SHA256MSG2
 * completes the word.
 *
 * Example:
 *   sha256msg1 xmm0, xmm1
 */
static const char* const ASM_SHA256MSG1 = "sha256msg1";

/*
 * SHA256MSG2 - SHA-256 Message Schedule, Second Step
 * Finishes a SHA-256 message-schedule expansion step.
 *
 * Syntax: SHA256MSG2 xmm, xmm/m128
 * Operands: XMM destination/source and XMM or memory source.
 * Flags affected: None.
 * Notes: SHA-NI. Paired with SHA256MSG1. The result dwords are what a following SHA256RNDS2
 * consumes from XMM0.
 *
 * Example:
 *   sha256msg2 xmm0, xmm1
 */
static const char* const ASM_SHA256MSG2 = "sha256msg2";

/*
 * SHA256RNDS2 - SHA-256 Two Rounds
 * Performs two SHA-256 rounds. The message words come from implicit XMM0.
 *
 * Syntax: SHA256RNDS2 xmm, xmm/m128, xmm0
 * Operands: XMM destination state, XMM or memory state source, and XMM0 holding two message
 * dwords. ML64 requires the third operand, and it must be XMM0.
 * Flags affected: None.
 * Notes: SHA-NI. Load the two schedule words into the low dwords of XMM0 first. The
 * instruction reads both state operands and writes the updated state half to the destination.
 * The machine encoding has no field for the message register; XMM0 is fixed.
 *
 * Example:
 *   sha256rnds2 xmm1, xmm2, xmm0
 */
static const char* const ASM_SHA256RNDS2 = "sha256rnds2";

/*
 * ============================================================================
 * 18. FPU Instructions
 * ============================================================================
 *
 * x87 is the original Intel floating-point unit (the 8087 and its descendants).
 * The name is the instruction family, not a separate numeric spec. Modern x87
 * implements IEEE 754 using 80-bit registers. The register file is the stack
 * described in section 4: eight slots, ST(0) at the top. These instructions
 * push, pop, or compute on that stack. They do not use the call stack, and
 * they do not use XMM. FXCH swaps ST(0) with another slot. FINIT and FNINIT
 * empty the stack. The status word's TOP field is the stack pointer; its
 * condition codes report compares; its exception flags stick until cleared.
 * The control word selects rounding and precision.
 *
 * How they fit together: FLD or FILD pushes a value. Arithmetic (FADD, FMUL,
 * FDIV, FSQRT, and the transcendentals) replaces ST(0), sometimes popping a
 * second operand. FST stores without popping; FSTP stores and pops. FCOM and
 * FUCOM write condition codes in the status word; FSTSW AX copies that word
 * so integer code can branch. FCOMI writes EFLAGS directly. A pending unmasked
 * exception is delivered by FWAIT, or by any "wait" form (FCLEX, FSTSW, FINIT,
 * FSAVE), which is the no-wait opcode plus an implicit FWAIT.
 *
 * NaN, unless a comment below says otherwise: arithmetic propagates a quiet
 * NaN. A signaling NaN raises the invalid-operation exception. Loads and
 * stores copy the bits and do not apply that rule. Comparisons are exact, not
 * within an epsilon. An ordered compare (FCOM, FCOMI, FTST) raises invalid if
 * either operand is a NaN. An unordered compare (FUCOM, FUCOMI) sets the
 * unordered condition and does not raise invalid for a quiet NaN; a signaling
 * NaN still raises invalid.
 *
 * Free-standing x87 is uncommon in new Windows x64 programs. That ABI uses
 * SSE2 and XMM0-XMM5 for float and double arguments and return values. x87 is
 * still in the CPU, still used by some 32-bit code, by 80-bit long double, and
 * by old binaries, and these opcodes remain valid in 64-bit mode. The first
 * x87 or SSE instruction of a thread can raise device-not-available (#NM) when
 * CR0.TS is set; the kernel uses that fault to lazy-restore this stack. See
 * CLTS.
 */

/*
 * FLD - Load Floating-Point Value
 * Pushes a floating-point value onto the x87 register stack.
 *
 * Syntax: FLD m32fp|m64fp|m80fp|ST(i)
 * Operands: memory source or another x87 register
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: The stack top is decremented and the new value becomes ST(0).
 *
 * Example:
 *   fld qword ptr [value]
 */
static const char* const ASM_FLD = "fld";

/*
 * FST - Store Floating-Point Value
 * Stores ST(0) to the destination without popping the x87 stack.
 *
 * Syntax: FST m32fp|m64fp|ST(i)
 * Operands: memory or x87 register destination
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Useful when the top-of-stack value must remain available for later x87 operations.
 *
 * Example:
 *   fst qword ptr [result]
 */
static const char* const ASM_FST = "fst";

/*
 * FSTP - Store Floating-Point Value and Pop
 * Stores ST(0) to the destination and then pops the x87 stack.
 *
 * Syntax: FSTP m32fp|m64fp|m80fp|ST(i)
 * Operands: memory or x87 register destination
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: After the store, the x87 stack pointer advances and the next register becomes ST(0).
 *
 * Example:
 *   fstp qword ptr [result]
 */
static const char* const ASM_FSTP = "fstp";

/*
 * FILD - Load Integer to x87
 * Pushes a signed integer from memory onto the x87 stack after converting it to extended
 * precision. x87 is the instruction family (the 8087 and later), not a separate floating-point
 * spec; see the introduction at the start of this section.
 *
 * Syntax: FILD m16int|m32int|m64int
 * Operands: integer memory source
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Useful for integer-to-floating conversion into x87 code paths.
 *
 * Example:
 *   fild dword ptr [count]
 */
static const char* const ASM_FILD = "fild";

/*
 * FIST - Store Integer from x87
 * Converts ST(0) to an integer and stores it without popping the x87 stack.
 *
 * Syntax: FIST m16int|m32int
 * Operands: integer memory destination
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Rounding mode comes from the x87 control word.
 *
 * Example:
 *   fist dword ptr [count]
 */
static const char* const ASM_FIST = "fist";

/*
 * FISTP - Store Integer from x87 and Pop
 * Converts ST(0) to an integer, stores it, and pops the x87 stack.
 *
 * Syntax: FISTP m16int|m32int|m64int
 * Operands: integer memory destination
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Use when the converted value is final and the stack element should be consumed.
 *
 * Example:
 *   fistp qword ptr [count]
 */
static const char* const ASM_FISTP = "fistp";

/*
 * FISTTP - Store Integer with Truncation and Pop
 * Converts ST(0) to an integer using truncation toward zero, stores it, and pops the x87
 * stack.
 *
 * Syntax: FISTTP m16int|m32int|m64int
 * Operands: An integer memory destination. ST(0) is implicit and is popped.
 * Flags affected: EFLAGS are unaffected. The x87 status word records an invalid operation if
 * the value does not fit the destination integer.
 * Notes: SSE3, but it is an x87 instruction, not an XMM instruction. The rounding mode in the
 * x87 control word is ignored: the conversion chops toward zero. That is the difference from
 * FISTP.
 *
 * Example:
 *   fisttp qword ptr [rax]
 */
static const char* const ASM_FISTTP = "fisttp";

/*
 * FXCH - Exchange x87 Registers
 * Exchanges ST(0) with another x87 stack register.
 *
 * Syntax: FXCH [ST(i)]
 * Operands: optional x87 register operand, default ST(1)
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Reorders the logical x87 stack without moving actual data in memory.
 *
 * Example:
 *   fxch st(1)
 */
static const char* const ASM_FXCH = "fxch";

/*
 * FADD - Floating-Point Add
 * Adds the source operand to ST(0) or adds ST(0) to an x87 destination.
 *
 * Syntax: FADD m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fadd st(1)
 */
static const char* const ASM_FADD = "fadd";

/*
 * FADDP - Floating-Point Add and Pop
 * Adds ST(0) to the destination x87 register and then pops the stack.
 *
 * Syntax: FADDP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   faddp st(1), st(0)
 */
static const char* const ASM_FADDP = "faddp";

/*
 * FSUB - Floating-Point Subtract
 * Subtracts the source operand from the destination x87 operand.
 *
 * Syntax: FSUB m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fsub st(1)
 */
static const char* const ASM_FSUB = "fsub";

/*
 * FSUBP - Floating-Point Subtract and Pop
 * Subtracts ST(0) from the destination x87 register and pops the stack.
 *
 * Syntax: FSUBP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fsubp st(1), st(0)
 */
static const char* const ASM_FSUBP = "fsubp";

/*
 * FSUBR - Floating-Point Reverse Subtract
 * Subtracts the destination from the source order opposite to FSUB.
 *
 * Syntax: FSUBR m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fsubr st(1)
 */
static const char* const ASM_FSUBR = "fsubr";

/*
 * FSUBRP - Floating-Point Reverse Subtract and Pop
 * Performs reverse subtraction and pops the x87 stack.
 *
 * Syntax: FSUBRP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fsubrp st(1), st(0)
 */
static const char* const ASM_FSUBRP = "fsubrp";

/*
 * FMUL - Floating-Point Multiply
 * Multiplies x87 operands and stores the product in the destination.
 *
 * Syntax: FMUL m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fmul st(1)
 */
static const char* const ASM_FMUL = "fmul";

/*
 * FMULP - Floating-Point Multiply and Pop
 * Multiplies the destination x87 register by ST(0) and then pops the stack.
 *
 * Syntax: FMULP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fmulp st(1), st(0)
 */
static const char* const ASM_FMULP = "fmulp";

/*
 * FDIV - Floating-Point Divide
 * Divides the destination by the source in the default x87 operand order.
 *
 * Syntax: FDIV m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fdiv st(1)
 */
static const char* const ASM_FDIV = "fdiv";

/*
 * FDIVP - Floating-Point Divide and Pop
 * Divides the destination x87 register by ST(0) and pops the stack.
 *
 * Syntax: FDIVP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fdivp st(1), st(0)
 */
static const char* const ASM_FDIVP = "fdivp";

/*
 * FDIVR - Floating-Point Reverse Divide
 * Divides in the reverse operand order relative to FDIV.
 *
 * Syntax: FDIVR m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fdivr st(1)
 */
static const char* const ASM_FDIVR = "fdivr";

/*
 * FDIVRP - Floating-Point Reverse Divide and Pop
 * Performs reverse division and pops the x87 stack.
 *
 * Syntax: FDIVRP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fdivrp st(1), st(0)
 */
static const char* const ASM_FDIVRP = "fdivrp";

/*
 * FCHS - Change Sign
 * Negates the sign of ST(0) without changing its magnitude.
 *
 * Syntax: FCHS
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fchs
 */
static const char* const ASM_FCHS = "fchs";

/*
 * FABS - Absolute Value
 * Clears the sign bit of ST(0).
 *
 * Syntax: FABS
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fabs
 */
static const char* const ASM_FABS = "fabs";

/*
 * FSQRT - Square Root
 * Replaces ST(0) with its square root.
 *
 * Syntax: FSQRT
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Still used when the value is already on the x87 stack or when the type is 80-bit
 * long double. New Windows x64 float and double code uses SQRTSS or SQRTSD instead. A quiet
 * NaN propagates; a negative operand or a signaling NaN raises invalid. The result is rounded
 * according to the control word.
 *
 * Example:
 *   fsqrt
 */
static const char* const ASM_FSQRT = "fsqrt";

/*
 * FPREM - Partial Remainder
 * Computes a partial remainder useful for argument reduction.
 *
 * Syntax: FPREM
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Replaces ST(0) with a partial remainder of ST(0) divided by ST(1). "Partial" means
 * the 8087 could not always finish the reduction in one instruction: if condition code C2 is
 * set, execute FPREM again until C2 clears. The quotient is truncated toward zero. This is
 * the older form. FPREM1 is the IEEE remainder, which rounds the quotient to nearest. A quiet
 * NaN propagates; a signaling NaN raises invalid.
 *
 * Example:
 *   fprem
 */
static const char* const ASM_FPREM = "fprem";

/*
 * FPREM1 - IEEE Partial Remainder
 * Computes an IEEE-style partial remainder.
 *
 * Syntax: FPREM1
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Same partial-reduction protocol as FPREM, including the repeat-until-C2-clears loop.
 * The difference is the quotient: FPREM1 rounds it to nearest even, which is the IEEE 754
 * remainder. FPREM rounds it toward zero. Both exist because the original 8087 reduction was
 * the toward-zero form and IEEE remainder was added later. NaN handling matches FPREM.
 *
 * Example:
 *   fprem1
 */
static const char* const ASM_FPREM1 = "fprem1";

/*
 * FRNDINT - Round to Integer
 * Rounds ST(0) to an integral value using the current rounding mode.
 *
 * Syntax: FRNDINT
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   frndint
 */
static const char* const ASM_FRNDINT = "frndint";

/*
 * FSIN - Sine
 * Replaces ST(0) with its sine. New Windows x64 code does not use this for float or double;
 * libm uses SSE. It is still the hardware sine for an 80-bit value already on the x87 stack,
 * and it remains valid in 64-bit mode. If the argument is too large for the on-chip
 * reduction, C2 is set and ST(0) is left unchanged. A quiet NaN propagates; a signaling NaN
 * raises invalid.
 *
 * Syntax: FSIN
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fsin
 */
static const char* const ASM_FSIN = "fsin";

/*
 * FCOS - Cosine
 * Replaces ST(0) with its cosine.
 *
 * Syntax: FCOS
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fcos
 */
static const char* const ASM_FCOS = "fcos";

/*
 * FSINCOS - Sine and Cosine
 * Computes sine and cosine, leaving one result in ST(0) and pushing the other.
 *
 * Syntax: FSINCOS
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fsincos
 */
static const char* const ASM_FSINCOS = "fsincos";

/*
 * FPTAN - Partial Tangent
 * Computes tangent and pushes 1.0 as defined by the x87 architecture.
 *
 * Syntax: FPTAN
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fptan
 */
static const char* const ASM_FPTAN = "fptan";

/*
 * FPATAN - Arctangent
 * Computes arctangent from ST(1) and ST(0), stores the result, and pops the stack.
 *
 * Syntax: FPATAN
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fpatan
 */
static const char* const ASM_FPATAN = "fpatan";

/*
 * F2XM1 - 2^x Minus 1
 * Computes 2 raised to ST(0) minus 1 for a restricted input range.
 *
 * Syntax: F2XM1
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   f2xm1
 */
static const char* const ASM_F2XM1 = "f2xm1";

/*
 * FYL2X - y * log2(x)
 * Computes ST(1) * log2(ST(0)), stores the result, and pops the stack. This is how 80-bit
 * software builds log, exp, and pow (together with FLDLN2 and F2XM1). Modern float and double
 * libm uses SSE instead. It is still the right instruction when the values are already on the
 * x87 stack. Invalid if x is negative, or if either operand is a signaling NaN; a quiet NaN
 * propagates when the operation is defined.
 *
 * Syntax: FYL2X
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fyl2x
 */
static const char* const ASM_FYL2X = "fyl2x";

/*
 * FYL2XP1 - y * log2(x+1)
 * Computes ST(1) * log2(ST(0)+1) and pops the stack.
 *
 * Syntax: FYL2XP1
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fyl2xp1
 */
static const char* const ASM_FYL2XP1 = "fyl2xp1";

/*
 * FSCALE - Scale by Power of Two
 * Scales ST(0) by 2 raised to the integer part of ST(1). This is the x87 form of ldexp: it
 * adds an integer to the exponent without a full multiply. Still useful inside x87 argument
 * reduction. New scalar code uses SSE. A quiet NaN propagates; a signaling NaN raises invalid.
 *
 * Syntax: FSCALE
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fscale
 */
static const char* const ASM_FSCALE = "fscale";

/*
 * FXTRACT - Extract Exponent and Significand
 * Splits ST(0) into significand and exponent values on the x87 stack.
 *
 * Syntax: FXTRACT
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fxtract
 */
static const char* const ASM_FXTRACT = "fxtract";

/*
 * FCOM - Floating-Point Compare
 * Compares ST(0) with the source operand and updates x87 condition codes.
 *
 * Syntax: FCOM m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS. The comparison is exact, not within an epsilon. FCOM is ordered: a NaN
 * (quiet or signaling) raises invalid. Unordered compares are the FUCOM forms.
 *
 * Example:
 *   fcom st(1)
 */
static const char* const ASM_FCOM = "fcom";

/*
 * FCOMP - Floating-Point Compare and Pop
 * Compares and then pops one x87 stack element.
 *
 * Syntax: FCOMP m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS. Exact compare, same NaN rule as FCOM: any NaN raises invalid, then one
 * register is popped.
 *
 * Example:
 *   fcomp st(1)
 */
static const char* const ASM_FCOMP = "fcomp";

/*
 * FCOMPP - Floating-Point Compare and Pop Twice
 * Compares ST(0) and ST(1) and pops both stack elements.
 *
 * Syntax: FCOMPP m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS. Exact compare of ST(0) with ST(1), same ordered NaN rule as FCOM, then
 * both registers are popped. The general NaN rule for arithmetic is in the section introduction.
 *
 * Example:
 *   fcompp
 */
static const char* const ASM_FCOMPP = "fcompp";

/*
 * FUCOM - Floating-Point Unordered Compare
 * Performs an unordered compare that treats NaNs according to the FUCOM rules. The compare is
 * exact, not within an epsilon. If either operand is a NaN, C3, C2, and C0 are all set
 * (unordered) and a quiet NaN does not raise invalid. A signaling NaN still raises invalid.
 *
 * Syntax: FUCOM m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS. Exact unordered compare: quiet NaN sets the unordered condition and does
 * not raise invalid; a signaling NaN raises invalid.
 *
 * Example:
 *   fucom st(1)
 */
static const char* const ASM_FUCOM = "fucom";

/*
 * FUCOMP - Floating-Point Unordered Compare and Pop
 * Unordered compare followed by one pop.
 *
 * Syntax: FUCOMP m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS. Same exact unordered NaN rule as FUCOM, then one pop.
 *
 * Example:
 *   fucomp st(1)
 */
static const char* const ASM_FUCOMP = "fucomp";

/*
 * FUCOMPP - Floating-Point Unordered Compare and Pop Twice
 * Unordered compare followed by popping both operands.
 *
 * Syntax: FUCOMPP m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS. Same exact unordered NaN rule as FUCOM, then both operands are popped.
 *
 * Example:
 *   fucompp
 */
static const char* const ASM_FUCOMPP = "fucompp";

/*
 * FCOMI - Compare and Set EFLAGS
 * Compares ST(0) with ST(i) and writes the result into ZF, PF, and CF in EFLAGS.
 *
 * Syntax: FCOMI ST(i)
 * Operands: ST(0) and another x87 register
 * Flags affected: ZF, PF, and CF are updated to encode the compare result; OF, SF, and AF are
 * cleared
 * Notes: These are the x87 compare instructions that directly update integer condition codes for
 * easy Jcc/SETcc use. The compare is exact. Ordered: a NaN sets PF (and ZF and CF) and raises
 * invalid. PF=1 means unordered; ZF and CF encode less or greater when PF=0.
 *
 * Example:
 *   fcomi st(1)
 */
static const char* const ASM_FCOMI = "fcomi";

/*
 * FCOMIP - Compare, Set EFLAGS, and Pop
 * Like FCOMI but pops the x87 stack once after the compare.
 *
 * Syntax: FCOMIP ST(i)
 * Operands: ST(0) and another x87 register
 * Flags affected: ZF, PF, and CF are updated to encode the compare result; OF, SF, and AF are
 * cleared
 * Notes: These are the x87 compare instructions that directly update integer condition codes for
 * easy Jcc/SETcc use. Same exact ordered NaN rule as FCOMI (invalid on any NaN, PF=1 if
 * unordered), then one pop.
 *
 * Example:
 *   fcomip st(1)
 */
static const char* const ASM_FCOMIP = "fcomip";

/*
 * FUCOMI - Unordered Compare and Set EFLAGS
 * Performs an unordered compare and writes ZF, PF, and CF in EFLAGS.
 *
 * Syntax: FUCOMI ST(i)
 * Operands: ST(0) and another x87 register
 * Flags affected: ZF, PF, and CF are updated to encode the compare result; OF, SF, and AF are
 * cleared
 * Notes: These are the x87 compare instructions that directly update integer condition codes for
 * easy Jcc/SETcc use. Exact unordered compare: a quiet NaN sets PF, ZF, and CF and does not
 * raise invalid. A signaling NaN raises invalid. PF=1 is the unordered case.
 *
 * Example:
 *   fucomi st(1)
 */
static const char* const ASM_FUCOMI = "fucomi";

/*
 * FUCOMIP - Unordered Compare, Set EFLAGS, and Pop
 * Unordered compare with EFLAGS result and one stack pop.
 *
 * Syntax: FUCOMIP ST(i)
 * Operands: ST(0) and another x87 register
 * Flags affected: ZF, PF, and CF are updated to encode the compare result; OF, SF, and AF are
 * cleared
 * Notes: These are the x87 compare instructions that directly update integer condition codes for
 * easy Jcc/SETcc use. Same exact unordered NaN rule as FUCOMI, then one pop.
 *
 * Example:
 *   fucomip st(1)
 */
static const char* const ASM_FUCOMIP = "fucomip";

/*
 * FTST - Test against Zero
 * Compares ST(0) with +0.0 exactly, not within an epsilon, and updates x87 condition codes.
 * Same ordered NaN rule as FCOM: a NaN raises invalid. It does not pop.
 *
 * Syntax: FTST ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   ftst
 */
static const char* const ASM_FTST = "ftst";

/*
 * FXAM - Examine Value
 * Classifies the value in ST(0) and updates x87 condition codes. "Classifies" means it
 * inspects the bit pattern and reports one class through the condition codes: unsupported,
 * NaN, normal, infinity, zero, empty, or denormal. C1 reports the sign. It does not compare
 * against a tolerance and it does not change ST(0). Read the class with FSTSW.
 *
 * Syntax: FXAM ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fxam
 */
static const char* const ASM_FXAM = "fxam";

/*
 * FLDZ - Load +0.0
 * Pushes the constant 0.0 onto the x87 stack.
 *
 * Syntax: FLDZ ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldz
 */
static const char* const ASM_FLDZ = "fldz";

/*
 * FLD1 - Load +1.0
 * Pushes the constant 1.0 onto the x87 stack.
 *
 * Syntax: FLD1 ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fld1
 */
static const char* const ASM_FLD1 = "fld1";

/*
 * FLDPI - Load pi
 * Pushes an 80-bit approximation of pi onto the x87 stack. The significand is 66 bits, about
 * 19 decimal digits, not an infinite value and not a correctly rounded 80-bit pi in every
 * last bit beyond that. Still useful when you are already on the x87 stack. New code usually
 * loads a memory constant with SSE.
 *
 * Syntax: FLDPI ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldpi
 */
static const char* const ASM_FLDPI = "fldpi";

/*
 * FLDL2E - Load log2(e)
 * Pushes the constant log2(e) onto the x87 stack.
 *
 * Syntax: FLDL2E ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldl2e
 */
static const char* const ASM_FLDL2E = "fldl2e";

/*
 * FLDL2T - Load log2(10)
 * Pushes the constant log2(10) onto the x87 stack.
 *
 * Syntax: FLDL2T ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldl2t
 */
static const char* const ASM_FLDL2T = "fldl2t";

/*
 * FLDLG2 - Load log10(2)
 * Pushes the constant log10(2) onto the x87 stack.
 *
 * Syntax: FLDLG2 ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldlg2
 */
static const char* const ASM_FLDLG2 = "fldlg2";

/*
 * FLDLN2 - Load ln(2)
 * Pushes the constant ln(2) onto the x87 stack.
 *
 * Syntax: FLDLN2 ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldln2
 */
static const char* const ASM_FLDLN2 = "fldln2";

/*
 * FINIT - Initialize x87 Unit
 * Waits for pending exceptions and resets the x87 FPU environment to its default state.
 * The control word goes back to round-to-nearest, 64-bit precision, exceptions masked; the
 * status word is cleared; every tag is marked empty and TOP is 0. Use it at startup, or when
 * the stack state is unknown, before you rely on an empty stack. The wait means a pending
 * unmasked exception is delivered before the reset. FNINIT skips that wait.
 *
 * Syntax: FINIT ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   finit
 */
static const char* const ASM_FINIT = "finit";

/*
 * FNINIT - Initialize x87 Unit without Wait
 * Resets the x87 environment without the implicit FWAIT step. Same empty stack and default
 * control word as FINIT, but a pending unmasked exception is discarded instead of delivered.
 * Use it when you know you want a clean unit and you do not want the old exception. The
 * no-wait forms exist because the 8087 was asynchronous; see FWAIT.
 *
 * Syntax: FNINIT ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fninit
 */
static const char* const ASM_FNINIT = "fninit";

/*
 * FLDCW - Load x87 Control Word
 * Loads the x87 control word from memory. That word holds the rounding mode (nearest, down,
 * up, or toward zero), the precision (24, 53, or 64-bit significand), and the exception
 * masks. Load a new one when you need a specific rounding mode, for example toward zero
 * before an integer store, or when you want an exception unmasked. Windows x64 scalar code
 * more often changes MXCSR for SSE; FLDCW is the x87 equivalent. FSTCW reads the current word.
 *
 * Syntax: FLDCW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldcw word ptr [cw]
 */
static const char* const ASM_FLDCW = "fldcw";

/*
 * FSTCW - Store x87 Control Word
 * Stores the x87 control word to memory with the wait form.
 *
 * Syntax: FSTCW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fstcw word ptr [cw]
 */
static const char* const ASM_FSTCW = "fstcw";

/*
 * FNSTCW - Store x87 Control Word without Wait
 * Stores the x87 control word to memory without an implicit FWAIT.
 *
 * Syntax: FNSTCW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnstcw word ptr [cw]
 */
static const char* const ASM_FNSTCW = "fnstcw";

/*
 * FSTSW - Store x87 Status Word
 * Stores the x87 status word with wait semantics. The word holds TOP, the condition codes,
 * and the exception flags. After FCOM, `fstsw ax` then SAHF or a test of AH turns those codes
 * into a branch. FCOMI writes EFLAGS directly and is what compilers prefer. The wait delivers
 * a pending unmasked exception before the store. FNSTSW skips the wait. Use this when you
 * need to read the stack top or the sticky exceptions, not to compute a value.
 *
 * Syntax: FSTSW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fstsw ax
 */
static const char* const ASM_FSTSW = "fstsw";

/*
 * FNSTSW - Store x87 Status Word without Wait
 * Stores the x87 status word without an implicit FWAIT.
 *
 * Syntax: FNSTSW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnstsw ax
 */
static const char* const ASM_FNSTSW = "fnstsw";

/*
 * FCLEX - Clear x87 Exceptions
 * Clears pending x87 exception flags with wait semantics. The sticky flags (invalid, denormal,
 * zero-divide, overflow, underflow, precision, and the stack fault / error summary) stay set
 * until something clears them, and an unmasked one will be raised again. FCLEX waits, so a
 * pending unmasked exception is delivered first, then clears the flags so later instructions
 * can continue. Use it after you have inspected the status and decided to go on.
 *
 * Syntax: FCLEX ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fclex
 */
static const char* const ASM_FCLEX = "fclex";

/*
 * FNCLEX - Clear x87 Exceptions without Wait
 * Clears the same exception flags as FCLEX but does not wait, so it does not itself deliver a
 * pending unmasked exception. Use it when you want the flags cleared and you are intentionally
 * discarding the pending exception rather than handling it.
 *
 * Syntax: FNCLEX ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnclex
 */
static const char* const ASM_FNCLEX = "fnclex";

/*
 * FWAIT - Wait for x87
 * Waits until the x87 unit is ready and checks for pending unmasked x87 exceptions. The
 * original 8087 was a separate asynchronous coprocessor, so the CPU could run ahead and FWAIT
 * was how you caught up and delivered a pending exception. On modern CPUs x87 exceptions are
 * synchronous, but FWAIT still delivers a pending unmasked exception. The "wait" mnemonics
 * (FCLEX, FSTSW, FINIT, FSAVE) are the no-wait opcode plus this wait. WAIT is the same
 * instruction.
 *
 * Syntax: FWAIT ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fwait
 */
static const char* const ASM_FWAIT = "fwait";

/*
 * WAIT - Wait Alias
 * Alias for FWAIT.
 *
 * Syntax: WAIT ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   wait
 */
static const char* const ASM_WAIT = "wait";

/*
 * FNOP - x87 No Operation
 * Consumes an x87 instruction slot without changing meaningful x87 data state. It is not
 * interchangeable with the integer NOP when you specifically need an x87 opcode: for example
 * to deliver a pending x87 exception the way FWAIT does, or as a recognized x87 no-op in a
 * prefix sequence. An integer NOP does not touch the x87 unit.
 *
 * Syntax: FNOP ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnop
 */
static const char* const ASM_FNOP = "fnop";

/*
 * FDECSTP - Decrement x87 Stack Pointer
 * Adjusts the x87 TOP field in the status word, which is the floating-point stack pointer,
 * not the call stack. The eight registers are a circular stack; see the introduction at the
 * start of this section. FDECSTP rotates that pointer without computing and without marking
 * the old top empty the way a pop does. The previous ST(7) becomes ST(0). Use it to rotate
 * the register file or to undo a pointer move. FINCSTP rotates the other way.
 *
 * Syntax: FDECSTP ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fdecstp
 */
static const char* const ASM_FDECSTP = "fdecstp";

/*
 * FINCSTP - Increment x87 Stack Pointer
 * Rotates the x87 TOP pointer the other way from FDECSTP, again without computing. The old
 * ST(0) becomes ST(7) and is not marked empty. This moves the stack pointer only; it is how
 * you rotate the eight slots when you do not want a value pushed or popped.
 *
 * Syntax: FINCSTP ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fincstp
 */
static const char* const ASM_FINCSTP = "fincstp";

/*
 * FFREE - Free x87 Register
 * Marks the specified x87 register stack entry as empty in the tag word. It does not change
 * TOP and it does not clear the bits, only the tag. Use it to drop a slot that is not at the
 * top, or to avoid a stack overflow when a later push would otherwise see a full register.
 * A normal pop (FSTP) is the way to drop ST(0). FFREE is for a buried slot.
 *
 * Syntax: FFREE ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   ffree st(1)
 */
static const char* const ASM_FFREE = "ffree";

/*
 * FSAVE - Save x87 Environment and Registers
 * Saves the x87 environment and register stack to memory, then initializes the FPU. The image
 * is the control word, status word, tag word, instruction and data pointers, and all eight
 * 80-bit registers. After the store the unit is reset as by FINIT, so the caller can use a
 * clean stack and later FRSTOR the snapshot. Old context switches and 80-bit setjmp-style
 * code do this. The wait form delivers a pending exception first. Modern OS switches prefer
 * FXSAVE or XSAVE because those also save XMM and YMM. The operand is a memory area, not a
 * register.
 *
 * Syntax: FSAVE ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fsave [mem]
 */
static const char* const ASM_FSAVE = "fsave";

/*
 * FNSAVE - Save x87 State without Wait
 * Same save-and-init as FSAVE, without the implicit FWAIT, so a pending unmasked exception is
 * not delivered before the snapshot. Use it when you must capture the state including a
 * pending exception, or when you already know no exception is waiting. FRSTOR puts the image
 * back. Prefer XSAVE on a modern kernel path that also has AVX state.
 *
 * Syntax: FNSAVE ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnsave [mem]
 */
static const char* const ASM_FNSAVE = "fnsave";

/*
 * FRSTOR - Restore x87 Environment and Registers
 * Restores the x87 environment and register stack from memory.
 *
 * Syntax: FRSTOR ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   frstor [mem]
 */
static const char* const ASM_FRSTOR = "frstor";

/*
 * FXSAVE - Save FPU/MMX/XMM State
 * Stores the architectural floating-point, MMX, and XMM state to an FXSAVE area.
 *
 * Syntax: FXSAVE ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fxsave [mem]
 */
static const char* const ASM_FXSAVE = "fxsave";

/*
 * FXRSTOR - Restore FPU/MMX/XMM State
 * Restores architectural floating-point, MMX, and XMM state from an FXRSTOR area.
 *
 * Syntax: FXRSTOR ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fxrstor [mem]
 */
static const char* const ASM_FXRSTOR = "fxrstor";

/*
 * ============================================================================
 * 19. SSE through SSE4.2 Instructions
 * ============================================================================
 *
 * SSE and SSE2 operate on XMM registers, 16 bytes each. "Lane-wise" means the
 * same operation is applied independently to each packed element: 4 single-
 * precision floats, 2 doubles, 16 bytes, 8 words, or 4 dwords, depending on
 * the mnemonic. A lane does not carry into its neighbor (PADDB can overflow a
 * byte without touching the next byte). Scalar forms (MOVSS, ADDSS, ...) use
 * only the low lane and leave the others alone.
 *
 * Alignment: MOVAPS and MOVAPD require a 16-byte-aligned address and fault
 * otherwise. MOVUPS and MOVUPD do not. On modern Intel and AMD the unaligned
 * forms are fast when the address happens to be aligned, which is why compilers
 * often emit MOVUPS even for aligned data.
 *
 * Memory model: a naturally aligned 16-byte SSE load or store is not guaranteed
 * atomic as a whole. Another thread can observe a tear, a mix of old and new
 * bytes. SSE does not become atomic because the address is aligned. When order
 * matters, use a LOCK-prefixed integer operation (see the note at the start of
 * section 8) or an explicit fence (MFENCE, LFENCE, SFENCE) together with
 * aligned accesses. A fence orders visibility; it does not glue 16 bytes into
 * one atomic word.
 *
 * SSE does not replace x87. It is a separate register file. The Windows x64 ABI
 * uses XMM0-XMM5 as volatile for float and double arguments and return values.
 * The upper XMM registers and YMM state are a different saving problem; see
 * the AVX introduction and XSAVE.
 *
 * The entries run from the original SSE and SSE2 set through the rest of
 * SSE2, then SSE3, SSSE3, SSE4.1, and SSE4.2. CRC32 is in section 17 and
 * POPCNT is in section 10; both are SSE4.2. PMULLD is listed with the packed
 * multiplies and is SSE4.1, not SSE2. Each later group names its CPUID bit.
 */
/*
 * MOVAPS - Move Aligned Packed Single
 * Moves a 128-bit packed single-precision vector between XMM registers or aligned memory.
 *
 * Syntax: MOVAPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movaps xmm0, xmmword ptr [rax]
 */
static const char* const ASM_MOVAPS = "movaps";

/*
 * MOVUPS - Move Unaligned Packed Single
 * Moves a 128-bit packed single-precision vector without requiring aligned memory.
 *
 * Syntax: MOVUPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movups xmm0, xmmword ptr [rax]
 */
static const char* const ASM_MOVUPS = "movups";

/*
 * MOVSS - Move Scalar Single
 * Moves the low scalar single-precision element between registers or memory.
 *
 * Syntax: MOVSS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movss xmm0, dword ptr [rax]
 */
static const char* const ASM_MOVSS = "movss";

/*
 * MOVAPD - Move Aligned Packed Double
 * Moves a 128-bit packed double-precision vector between XMM registers or aligned memory.
 *
 * Syntax: MOVAPD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movapd xmm0, xmmword ptr [rax]
 */
static const char* const ASM_MOVAPD = "movapd";

/*
 * MOVUPD - Move Unaligned Packed Double
 * Moves a 128-bit packed double-precision vector without requiring aligned memory.
 *
 * Syntax: MOVUPD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movupd xmm0, xmmword ptr [rax]
 */
static const char* const ASM_MOVUPD = "movupd";

/*
 * MOVSD_SSE - Move Scalar Double (SSE2)
 * Moves the low scalar double-precision element between XMM registers or memory; this is the
 * SSE2 scalar move, not the string instruction of the same mnemonic.
 *
 * Syntax: MOVSD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movsd xmm0, qword ptr [rax]
 */
static const char* const ASM_MOVSD_SSE = "movsd";

/*
 * MOVDQA - Move Aligned Integer Vector
 * Moves 128-bit integer data between XMM registers or aligned memory.
 *
 * Syntax: MOVDQA ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movdqa xmm0, xmmword ptr [rax]
 */
static const char* const ASM_MOVDQA = "movdqa";

/*
 * MOVDQU - Move Unaligned Integer Vector
 * Moves 128-bit integer data between XMM registers or unaligned memory.
 *
 * Syntax: MOVDQU ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movdqu xmm0, xmmword ptr [rax]
 */
static const char* const ASM_MOVDQU = "movdqu";

/*
 * MOVLPS - Move Low Packed Singles
 * Moves the low 64 bits of packed single data between an XMM register and memory.
 *
 * Syntax: MOVLPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movlps xmm0, qword ptr [rax]
 */
static const char* const ASM_MOVLPS = "movlps";

/*
 * MOVHPS - Move High Packed Singles
 * Moves the high 64 bits of packed single data between an XMM register and memory.
 *
 * Syntax: MOVHPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movhps xmm0, qword ptr [rax]
 */
static const char* const ASM_MOVHPS = "movhps";

/*
 * MOVLPD - Move Low Packed Double
 * Moves the low 64 bits of a packed double vector between an XMM register and memory.
 *
 * Syntax: MOVLPD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movlpd xmm0, qword ptr [rax]
 */
static const char* const ASM_MOVLPD = "movlpd";

/*
 * MOVHPD - Move High Packed Double
 * Moves the high 64 bits of a packed double vector between an XMM register and memory.
 *
 * Syntax: MOVHPD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movhpd xmm0, qword ptr [rax]
 */
static const char* const ASM_MOVHPD = "movhpd";

/*
 * MOVLHPS - Move Low to High Packed Singles
 * Copies the low 64 bits of one XMM register into the high 64 bits of another.
 *
 * Syntax: MOVLHPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movlhps xmm0, xmm1
 */
static const char* const ASM_MOVLHPS = "movlhps";

/*
 * MOVHLPS - Move High to Low Packed Singles
 * Copies the high 64 bits of one XMM register into the low 64 bits of another.
 *
 * Syntax: MOVHLPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movhlps xmm0, xmm1
 */
static const char* const ASM_MOVHLPS = "movhlps";

/*
 * ADDPS - Add Packed Singles
 * Adds four packed single-precision elements.
 *
 * Syntax: addps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   addps xmm0, xmm1
 */
static const char* const ASM_ADDPS = "addps";

/*
 * ADDPD - Add Packed Doubles
 * Adds two packed double-precision elements.
 *
 * Syntax: addpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   addpd xmm0, xmm1
 */
static const char* const ASM_ADDPD = "addpd";

/*
 * ADDSS - Add Scalar Single
 * Adds the low scalar single-precision elements and leaves upper lanes unchanged in legacy SSE
 * encoding.
 *
 * Syntax: addss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   addss xmm0, xmm1
 */
static const char* const ASM_ADDSS = "addss";

/*
 * ADDSD - Add Scalar Double
 * Adds the low scalar double-precision elements and leaves upper lanes unchanged in legacy SSE2
 * encoding.
 *
 * Syntax: addsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   addsd xmm0, xmm1
 */
static const char* const ASM_ADDSD = "addsd";

/*
 * SUBPS - Subtract Packed Singles
 * Subtracts packed single-precision elements.
 *
 * Syntax: subps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   subps xmm0, xmm1
 */
static const char* const ASM_SUBPS = "subps";

/*
 * SUBPD - Subtract Packed Doubles
 * Subtracts packed double-precision elements.
 *
 * Syntax: subpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   subpd xmm0, xmm1
 */
static const char* const ASM_SUBPD = "subpd";

/*
 * SUBSS - Subtract Scalar Single
 * Subtracts the low scalar single-precision elements.
 *
 * Syntax: subss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   subss xmm0, xmm1
 */
static const char* const ASM_SUBSS = "subss";

/*
 * SUBSD - Subtract Scalar Double
 * Subtracts the low scalar double-precision elements.
 *
 * Syntax: subsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   subsd xmm0, xmm1
 */
static const char* const ASM_SUBSD = "subsd";

/*
 * MULPS - Multiply Packed Singles
 * Multiplies packed single-precision elements.
 *
 * Syntax: mulps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   mulps xmm0, xmm1
 */
static const char* const ASM_MULPS = "mulps";

/*
 * MULPD - Multiply Packed Doubles
 * Multiplies packed double-precision elements.
 *
 * Syntax: mulpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   mulpd xmm0, xmm1
 */
static const char* const ASM_MULPD = "mulpd";

/*
 * MULSS - Multiply Scalar Single
 * Multiplies the low scalar single-precision elements.
 *
 * Syntax: mulss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   mulss xmm0, xmm1
 */
static const char* const ASM_MULSS = "mulss";

/*
 * MULSD - Multiply Scalar Double
 * Multiplies the low scalar double-precision elements.
 *
 * Syntax: mulsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   mulsd xmm0, xmm1
 */
static const char* const ASM_MULSD = "mulsd";

/*
 * DIVPS - Divide Packed Singles
 * Divides packed single-precision elements.
 *
 * Syntax: divps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   divps xmm0, xmm1
 */
static const char* const ASM_DIVPS = "divps";

/*
 * DIVPD - Divide Packed Doubles
 * Divides packed double-precision elements.
 *
 * Syntax: divpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   divpd xmm0, xmm1
 */
static const char* const ASM_DIVPD = "divpd";

/*
 * DIVSS - Divide Scalar Single
 * Divides the low scalar single-precision elements.
 *
 * Syntax: divss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   divss xmm0, xmm1
 */
static const char* const ASM_DIVSS = "divss";

/*
 * DIVSD - Divide Scalar Double
 * Divides the low scalar double-precision elements.
 *
 * Syntax: divsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   divsd xmm0, xmm1
 */
static const char* const ASM_DIVSD = "divsd";

/*
 * SQRTPS - Square Root Packed Singles
 * Computes packed single-precision square roots.
 *
 * Syntax: sqrtps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   sqrtps xmm0, xmm1
 */
static const char* const ASM_SQRTPS = "sqrtps";

/*
 * SQRTPD - Square Root Packed Doubles
 * Computes packed double-precision square roots.
 *
 * Syntax: sqrtpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   sqrtpd xmm0, xmm1
 */
static const char* const ASM_SQRTPD = "sqrtpd";

/*
 * SQRTSS - Square Root Scalar Single
 * Computes the square root of the low scalar single-precision element.
 *
 * Syntax: sqrtss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   sqrtss xmm0, xmm1
 */
static const char* const ASM_SQRTSS = "sqrtss";

/*
 * SQRTSD - Square Root Scalar Double
 * Computes the square root of the low scalar double-precision element.
 *
 * Syntax: sqrtsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   sqrtsd xmm0, xmm1
 */
static const char* const ASM_SQRTSD = "sqrtsd";

/*
 * MAXPS - Packed Maximum Singles
 * Selects the greater packed single-precision element from each lane.
 *
 * Syntax: maxps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   maxps xmm0, xmm1
 */
static const char* const ASM_MAXPS = "maxps";

/*
 * MAXPD - Packed Maximum Doubles
 * Selects the greater packed double-precision element from each lane.
 *
 * Syntax: maxpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   maxpd xmm0, xmm1
 */
static const char* const ASM_MAXPD = "maxpd";

/*
 * MAXSS - Scalar Maximum Single
 * Selects the greater low scalar single-precision element.
 *
 * Syntax: maxss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   maxss xmm0, xmm1
 */
static const char* const ASM_MAXSS = "maxss";

/*
 * MAXSD - Scalar Maximum Double
 * Selects the greater low scalar double-precision element.
 *
 * Syntax: maxsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   maxsd xmm0, xmm1
 */
static const char* const ASM_MAXSD = "maxsd";

/*
 * MINPS - Packed Minimum Singles
 * Selects the smaller packed single-precision element from each lane.
 *
 * Syntax: minps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   minps xmm0, xmm1
 */
static const char* const ASM_MINPS = "minps";

/*
 * MINPD - Packed Minimum Doubles
 * Selects the smaller packed double-precision element from each lane.
 *
 * Syntax: minpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   minpd xmm0, xmm1
 */
static const char* const ASM_MINPD = "minpd";

/*
 * MINSS - Scalar Minimum Single
 * Selects the smaller low scalar single-precision element.
 *
 * Syntax: minss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   minss xmm0, xmm1
 */
static const char* const ASM_MINSS = "minss";

/*
 * MINSD - Scalar Minimum Double
 * Selects the smaller low scalar double-precision element.
 *
 * Syntax: minsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   minsd xmm0, xmm1
 */
static const char* const ASM_MINSD = "minsd";

/*
 * CMPPS - Compare Packed Singles
 * Compares packed single-precision elements and writes an all-ones/all-zeros mask result to each
 * lane.
 *
 * Syntax: cmpps xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and comparison-immediate selector
 * Flags affected: None
 * Notes: The immediate selects the comparison predicate; the result is a vector mask rather than
 * EFLAGS bits.
 *
 * Example:
 *   cmpps xmm0, xmm1, 0
 */
static const char* const ASM_CMPPS = "cmpps";

/*
 * CMPPD - Compare Packed Doubles
 * Compares packed double-precision elements and writes mask results per lane.
 *
 * Syntax: cmppd xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and comparison-immediate selector
 * Flags affected: None
 * Notes: The immediate selects the comparison predicate; the result is a vector mask rather than
 * EFLAGS bits.
 *
 * Example:
 *   cmppd xmm0, xmm1, 0
 */
static const char* const ASM_CMPPD = "cmppd";

/*
 * CMPSS - Compare Scalar Single
 * Compares the low scalar single-precision elements and writes a mask result to the low lane.
 *
 * Syntax: cmpss xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and comparison-immediate selector
 * Flags affected: None
 * Notes: The immediate selects the comparison predicate; the result is a vector mask rather than
 * EFLAGS bits.
 *
 * Example:
 *   cmpss xmm0, xmm1, 0
 */
static const char* const ASM_CMPSS = "cmpss";

/*
 * CMPSD_SSE - Compare Scalar Double (SSE2)
 * Compares the low scalar double-precision elements and writes a mask result to the low lane;
 * this is the SSE compare, not the string CMPSD instruction.
 *
 * Syntax: cmpsd xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and comparison-immediate selector
 * Flags affected: None
 * Notes: The immediate selects the comparison predicate; the result is a vector mask rather than
 * EFLAGS bits.
 *
 * Example:
 *   cmpsd xmm0, xmm1, 0
 */
static const char* const ASM_CMPSD_SSE = "cmpsd";

/*
 * COMISS - Compare Ordered Scalar Single and Set EFLAGS
 * Performs an ordered compare of low scalar single-precision values and writes ZF, PF, and CF in
 * EFLAGS.
 *
 * Syntax: comiss xmm1, xmm2/m64
 * Operands: XMM register operands or one XMM plus memory source
 * Flags affected: ZF, PF, and CF are updated; OF, SF, and AF are cleared
 * Notes: These compare instructions are the SIMD equivalents that feed ordinary Jcc/SETcc
 * integer control flow.
 *
 * Example:
 *   comiss xmm0, xmm1
 */
static const char* const ASM_COMISS = "comiss";

/*
 * COMISD - Compare Ordered Scalar Double and Set EFLAGS
 * Performs an ordered compare of low scalar double-precision values and writes ZF, PF, and CF in
 * EFLAGS.
 *
 * Syntax: comisd xmm1, xmm2/m64
 * Operands: XMM register operands or one XMM plus memory source
 * Flags affected: ZF, PF, and CF are updated; OF, SF, and AF are cleared
 * Notes: These compare instructions are the SIMD equivalents that feed ordinary Jcc/SETcc
 * integer control flow.
 *
 * Example:
 *   comisd xmm0, xmm1
 */
static const char* const ASM_COMISD = "comisd";

/*
 * UCOMISS - Compare Unordered Scalar Single and Set EFLAGS
 * Performs an unordered compare of low scalar single-precision values and writes ZF, PF, and CF
 * in EFLAGS.
 *
 * Syntax: ucomiss xmm1, xmm2/m64
 * Operands: XMM register operands or one XMM plus memory source
 * Flags affected: ZF, PF, and CF are updated; OF, SF, and AF are cleared
 * Notes: These compare instructions are the SIMD equivalents that feed ordinary Jcc/SETcc
 * integer control flow.
 *
 * Example:
 *   ucomiss xmm0, xmm1
 */
static const char* const ASM_UCOMISS = "ucomiss";

/*
 * UCOMISD - Compare Unordered Scalar Double and Set EFLAGS
 * Performs an unordered compare of low scalar double-precision values and writes ZF, PF, and CF
 * in EFLAGS.
 *
 * Syntax: ucomisd xmm1, xmm2/m64
 * Operands: XMM register operands or one XMM plus memory source
 * Flags affected: ZF, PF, and CF are updated; OF, SF, and AF are cleared
 * Notes: These compare instructions are the SIMD equivalents that feed ordinary Jcc/SETcc
 * integer control flow.
 *
 * Example:
 *   ucomisd xmm0, xmm1
 */
static const char* const ASM_UCOMISD = "ucomisd";

/*
 * ANDPS - Bitwise AND Packed Singles
 * Bitwise AND of packed single-precision bit patterns.
 *
 * Syntax: andps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   andps xmm0, xmm1
 */
static const char* const ASM_ANDPS = "andps";

/*
 * ANDPD - Bitwise AND Packed Doubles
 * Bitwise AND of packed double-precision bit patterns.
 *
 * Syntax: andpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   andpd xmm0, xmm1
 */
static const char* const ASM_ANDPD = "andpd";

/*
 * ORPS - Bitwise OR Packed Singles
 * Bitwise OR of packed single-precision bit patterns.
 *
 * Syntax: orps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   orps xmm0, xmm1
 */
static const char* const ASM_ORPS = "orps";

/*
 * ORPD - Bitwise OR Packed Doubles
 * Bitwise OR of packed double-precision bit patterns.
 *
 * Syntax: orpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   orpd xmm0, xmm1
 */
static const char* const ASM_ORPD = "orpd";

/*
 * XORPS - Bitwise XOR Packed Singles
 * Bitwise XOR of packed single-precision bit patterns.
 *
 * Syntax: xorps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   xorps xmm0, xmm1
 */
static const char* const ASM_XORPS = "xorps";

/*
 * XORPD - Bitwise XOR Packed Doubles
 * Bitwise XOR of packed double-precision bit patterns.
 *
 * Syntax: xorpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   xorpd xmm0, xmm1
 */
static const char* const ASM_XORPD = "xorpd";

/*
 * ANDNPS - Bitwise AND NOT Packed Singles
 * Bitwise AND of the inverted destination and the source for packed single data.
 *
 * Syntax: andnps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   andnps xmm0, xmm1
 */
static const char* const ASM_ANDNPS = "andnps";

/*
 * ANDNPD - Bitwise AND NOT Packed Doubles
 * Bitwise AND of the inverted destination and the source for packed double data.
 *
 * Syntax: andnpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   andnpd xmm0, xmm1
 */
static const char* const ASM_ANDNPD = "andnpd";

/*
 * SHUFPS - Shuffle Packed Singles
 * Reorders packed single elements selected by an immediate control byte.
 *
 * Syntax: shufps xmm1, xmm2/m128, imm8
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   shufps xmm0, xmm1, 00011011b
 */
static const char* const ASM_SHUFPS = "shufps";

/*
 * SHUFPD - Shuffle Packed Doubles
 * Reorders packed double elements selected by an immediate control byte.
 *
 * Syntax: shufpd xmm1, xmm2/m128, imm8
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   shufpd xmm0, xmm1, 01b
 */
static const char* const ASM_SHUFPD = "shufpd";

/*
 * UNPCKLPS - Unpack Low Packed Singles
 * Interleaves the low packed single elements from two operands.
 *
 * Syntax: unpcklps xmm1, xmm2/m128
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   unpcklps xmm0, xmm1
 */
static const char* const ASM_UNPCKLPS = "unpcklps";

/*
 * UNPCKHPS - Unpack High Packed Singles
 * Interleaves the high packed single elements from two operands.
 *
 * Syntax: unpckhps xmm1, xmm2/m128
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   unpckhps xmm0, xmm1
 */
static const char* const ASM_UNPCKHPS = "unpckhps";

/*
 * UNPCKLPD - Unpack Low Packed Doubles
 * Interleaves the low packed double elements from two operands.
 *
 * Syntax: unpcklpd xmm1, xmm2/m128
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   unpcklpd xmm0, xmm1
 */
static const char* const ASM_UNPCKLPD = "unpcklpd";

/*
 * UNPCKHPD - Unpack High Packed Doubles
 * Interleaves the high packed double elements from two operands.
 *
 * Syntax: unpckhpd xmm1, xmm2/m128
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   unpckhpd xmm0, xmm1
 */
static const char* const ASM_UNPCKHPD = "unpckhpd";

/*
 * CVTSI2SS - Convert Integer to Scalar Single
 * Converts a signed integer to the low scalar single-precision element.
 *
 * Syntax: cvtsi2ss dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtsi2ss xmm0, eax
 */
static const char* const ASM_CVTSI2SS = "cvtsi2ss";

/*
 * CVTSI2SD - Convert Integer to Scalar Double
 * Converts a signed integer to the low scalar double-precision element.
 *
 * Syntax: cvtsi2sd dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtsi2sd xmm0, eax
 */
static const char* const ASM_CVTSI2SD = "cvtsi2sd";

/*
 * CVTSS2SI - Convert Scalar Single to Integer
 * Converts the low scalar single-precision element to a signed integer using current rounding
 * mode.
 *
 * Syntax: cvtss2si dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtss2si eax, xmm0
 */
static const char* const ASM_CVTSS2SI = "cvtss2si";

/*
 * CVTSD2SI - Convert Scalar Double to Integer
 * Converts the low scalar double-precision element to a signed integer using current rounding
 * mode.
 *
 * Syntax: cvtsd2si dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtsd2si eax, xmm0
 */
static const char* const ASM_CVTSD2SI = "cvtsd2si";

/*
 * CVTTSS2SI - Convert Scalar Single to Integer with Truncation
 * Converts with truncation toward zero regardless of MXCSR rounding mode.
 *
 * Syntax: cvttss2si dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvttss2si eax, xmm0
 */
static const char* const ASM_CVTTSS2SI = "cvttss2si";

/*
 * CVTTSD2SI - Convert Scalar Double to Integer with Truncation
 * Converts with truncation toward zero regardless of MXCSR rounding mode.
 *
 * Syntax: cvttsd2si dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvttsd2si eax, xmm0
 */
static const char* const ASM_CVTTSD2SI = "cvttsd2si";

/*
 * CVTSS2SD - Convert Scalar Single to Scalar Double
 * Widens the low scalar single-precision element to scalar double-precision.
 *
 * Syntax: cvtss2sd dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtss2sd xmm0, xmm1
 */
static const char* const ASM_CVTSS2SD = "cvtss2sd";

/*
 * CVTSD2SS - Convert Scalar Double to Scalar Single
 * Narrows the low scalar double-precision element to scalar single-precision.
 *
 * Syntax: cvtsd2ss dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtsd2ss xmm0, xmm1
 */
static const char* const ASM_CVTSD2SS = "cvtsd2ss";

/*
 * CVTPS2PD - Convert Packed Singles to Packed Doubles
 * Widens packed single-precision values to packed doubles.
 *
 * Syntax: cvtps2pd dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtps2pd xmm0, xmm1
 */
static const char* const ASM_CVTPS2PD = "cvtps2pd";

/*
 * CVTPD2PS - Convert Packed Doubles to Packed Singles
 * Narrows packed double-precision values to packed singles.
 *
 * Syntax: cvtpd2ps dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtpd2ps xmm0, xmm1
 */
static const char* const ASM_CVTPD2PS = "cvtpd2ps";

/*
 * CVTPS2DQ - Convert Packed Singles to Packed Integers
 * Converts packed singles to signed dword integers using current rounding mode.
 *
 * Syntax: cvtps2dq dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtps2dq xmm0, xmm1
 */
static const char* const ASM_CVTPS2DQ = "cvtps2dq";

/*
 * CVTDQ2PS - Convert Packed Integers to Packed Singles
 * Converts packed signed dword integers to packed single-precision values.
 *
 * Syntax: cvtdq2ps dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtdq2ps xmm0, xmm1
 */
static const char* const ASM_CVTDQ2PS = "cvtdq2ps";

/*
 * CVTPD2DQ - Convert Packed Doubles to Packed Integers
 * Converts packed doubles to signed dword integers.
 *
 * Syntax: cvtpd2dq dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtpd2dq xmm0, xmm1
 */
static const char* const ASM_CVTPD2DQ = "cvtpd2dq";

/*
 * CVTDQ2PD - Convert Packed Integers to Packed Doubles
 * Converts packed signed dword integers to packed doubles.
 *
 * Syntax: cvtdq2pd dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtdq2pd xmm0, xmm1
 */
static const char* const ASM_CVTDQ2PD = "cvtdq2pd";

/*
 * CVTTPD2DQ - Convert Packed Doubles to Packed Integers with Truncation
 * Converts packed doubles to signed dword integers using truncation.
 *
 * Syntax: cvttpd2dq dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvttpd2dq xmm0, xmm1
 */
static const char* const ASM_CVTTPD2DQ = "cvttpd2dq";

/*
 * CVTTPS2DQ - Convert Packed Singles to Packed Integers with Truncation
 * Converts packed singles to signed dword integers using truncation.
 *
 * Syntax: cvttps2dq dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvttps2dq xmm0, xmm1
 */
static const char* const ASM_CVTTPS2DQ = "cvttps2dq";

/*
 * PAND - Packed Bitwise AND
 * Performs bitwise AND on packed integer data.
 *
 * Syntax: pand xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pand xmm0, xmm1
 */
static const char* const ASM_PAND = "pand";

/*
 * PANDN - Packed Bitwise AND NOT
 * Performs bitwise AND of the inverted destination and the source on packed integer data.
 *
 * Syntax: pandn xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pandn xmm0, xmm1
 */
static const char* const ASM_PANDN = "pandn";

/*
 * POR - Packed Bitwise OR
 * Performs bitwise OR on packed integer data.
 *
 * Syntax: por xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   por xmm0, xmm1
 */
static const char* const ASM_POR = "por";

/*
 * PXOR - Packed Bitwise XOR
 * Performs bitwise XOR on packed integer data.
 *
 * Syntax: pxor xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pxor xmm0, xmm1
 */
static const char* const ASM_PXOR = "pxor";

/*
 * PADDB - Packed Add Bytes
 * Lane-wise means each 8-bit element is added on its own; see the section 19 introduction.
 * Adds packed 8-bit integer lanes.
 *
 * Syntax: paddb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   paddb xmm0, xmm1
 */
static const char* const ASM_PADDB = "paddb";

/*
 * PADDW - Packed Add Words
 * Adds packed 16-bit integer lanes.
 *
 * Syntax: paddw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   paddw xmm0, xmm1
 */
static const char* const ASM_PADDW = "paddw";

/*
 * PADDD - Packed Add Doublewords
 * Adds packed 32-bit integer lanes.
 *
 * Syntax: paddd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   paddd xmm0, xmm1
 */
static const char* const ASM_PADDD = "paddd";

/*
 * PADDQ - Packed Add Quadwords
 * Adds packed 64-bit integer lanes.
 *
 * Syntax: paddq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   paddq xmm0, xmm1
 */
static const char* const ASM_PADDQ = "paddq";

/*
 * PSUBB - Packed Subtract Bytes
 * Subtracts packed 8-bit integer lanes.
 *
 * Syntax: psubb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psubb xmm0, xmm1
 */
static const char* const ASM_PSUBB = "psubb";

/*
 * PSUBW - Packed Subtract Words
 * Subtracts packed 16-bit integer lanes.
 *
 * Syntax: psubw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psubw xmm0, xmm1
 */
static const char* const ASM_PSUBW = "psubw";

/*
 * PSUBD - Packed Subtract Doublewords
 * Subtracts packed 32-bit integer lanes.
 *
 * Syntax: psubd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psubd xmm0, xmm1
 */
static const char* const ASM_PSUBD = "psubd";

/*
 * PSUBQ - Packed Subtract Quadwords
 * Subtracts packed 64-bit integer lanes.
 *
 * Syntax: psubq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psubq xmm0, xmm1
 */
static const char* const ASM_PSUBQ = "psubq";

/*
 * PMULLW - Packed Multiply Low Words
 * Multiplies packed signed 16-bit integers and keeps the low 16 bits of each product.
 *
 * Syntax: pmullw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pmullw xmm0, xmm1
 */
static const char* const ASM_PMULLW = "pmullw";

/*
 * PMULLD - Packed Multiply Doublewords
 * Multiplies packed signed 32-bit integers and keeps the low 32 bits of each product.
 *
 * Syntax: pmulld xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: SSE4.1, not SSE2. Four signed dword products, low 32 bits kept. PMULDQ, later
 * in this section, multiplies only the even dwords and keeps 64-bit products.
 *
 * Example:
 *   pmulld xmm0, xmm1
 */
static const char* const ASM_PMULLD = "pmulld";

/*
 * PMULHW - Packed Multiply High Words
 * Multiplies packed signed 16-bit integers and keeps the high 16 bits of each product.
 *
 * Syntax: pmulhw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pmulhw xmm0, xmm1
 */
static const char* const ASM_PMULHW = "pmulhw";

/*
 * PMULUDQ - Packed Multiply Unsigned Doubleword to Quadword
 * Multiplies selected unsigned 32-bit lanes producing 64-bit products.
 *
 * Syntax: pmuludq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pmuludq xmm0, xmm1
 */
static const char* const ASM_PMULUDQ = "pmuludq";

/*
 * PSLLW - Packed Shift Left Logical Words
 * Shifts packed 16-bit lanes left logically.
 *
 * Syntax: psllw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psllw xmm0, xmm1
 */
static const char* const ASM_PSLLW = "psllw";

/*
 * PSLLD - Packed Shift Left Logical Doublewords
 * Shifts packed 32-bit lanes left logically.
 *
 * Syntax: pslld xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pslld xmm0, xmm1
 */
static const char* const ASM_PSLLD = "pslld";

/*
 * PSLLQ - Packed Shift Left Logical Quadwords
 * Shifts packed 64-bit lanes left logically.
 *
 * Syntax: psllq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psllq xmm0, xmm1
 */
static const char* const ASM_PSLLQ = "psllq";

/*
 * PSRLW - Packed Shift Right Logical Words
 * Shifts packed 16-bit lanes right logically.
 *
 * Syntax: psrlw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psrlw xmm0, xmm1
 */
static const char* const ASM_PSRLW = "psrlw";

/*
 * PSRLD - Packed Shift Right Logical Doublewords
 * Shifts packed 32-bit lanes right logically.
 *
 * Syntax: psrld xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psrld xmm0, xmm1
 */
static const char* const ASM_PSRLD = "psrld";

/*
 * PSRLQ - Packed Shift Right Logical Quadwords
 * Shifts packed 64-bit lanes right logically.
 *
 * Syntax: psrlq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psrlq xmm0, xmm1
 */
static const char* const ASM_PSRLQ = "psrlq";

/*
 * PSRAW - Packed Shift Right Arithmetic Words
 * Shifts packed signed 16-bit lanes right arithmetically.
 *
 * Syntax: psraw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psraw xmm0, xmm1
 */
static const char* const ASM_PSRAW = "psraw";

/*
 * PSRAD - Packed Shift Right Arithmetic Doublewords
 * Shifts packed signed 32-bit lanes right arithmetically.
 *
 * Syntax: psrad xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psrad xmm0, xmm1
 */
static const char* const ASM_PSRAD = "psrad";

/*
 * PCMPEQB - Packed Compare Equal Bytes
 * Compares packed byte lanes for equality and produces all-ones or all-zeros masks.
 *
 * Syntax: pcmpeqb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpeqb xmm0, xmm1
 */
static const char* const ASM_PCMPEQB = "pcmpeqb";

/*
 * PCMPEQW - Packed Compare Equal Words
 * Compares packed word lanes for equality and produces masks.
 *
 * Syntax: pcmpeqw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpeqw xmm0, xmm1
 */
static const char* const ASM_PCMPEQW = "pcmpeqw";

/*
 * PCMPEQD - Packed Compare Equal Doublewords
 * Compares packed dword lanes for equality and produces masks.
 *
 * Syntax: pcmpeqd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpeqd xmm0, xmm1
 */
static const char* const ASM_PCMPEQD = "pcmpeqd";

/*
 * PCMPGTB - Packed Compare Greater Than Bytes
 * Performs signed packed byte greater-than comparisons.
 *
 * Syntax: pcmpgtb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpgtb xmm0, xmm1
 */
static const char* const ASM_PCMPGTB = "pcmpgtb";

/*
 * PCMPGTW - Packed Compare Greater Than Words
 * Performs signed packed word greater-than comparisons.
 *
 * Syntax: pcmpgtw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpgtw xmm0, xmm1
 */
static const char* const ASM_PCMPGTW = "pcmpgtw";

/*
 * PCMPGTD - Packed Compare Greater Than Doublewords
 * Performs signed packed dword greater-than comparisons.
 *
 * Syntax: pcmpgtd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpgtd xmm0, xmm1
 */
static const char* const ASM_PCMPGTD = "pcmpgtd";

/*
 * PACKSSWB - Pack with Signed Saturation Word to Byte
 * Packs signed word lanes to bytes using signed saturation.
 *
 * Syntax: packsswb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   packsswb xmm0, xmm1
 */
static const char* const ASM_PACKSSWB = "packsswb";

/*
 * PACKSSDW - Pack with Signed Saturation Doubleword to Word
 * Packs signed dword lanes to words using signed saturation.
 *
 * Syntax: packssdw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   packssdw xmm0, xmm1
 */
static const char* const ASM_PACKSSDW = "packssdw";

/*
 * PACKUSWB - Pack with Unsigned Saturation Word to Byte
 * Packs signed or unsigned word lanes to bytes using unsigned saturation.
 *
 * Syntax: packuswb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   packuswb xmm0, xmm1
 */
static const char* const ASM_PACKUSWB = "packuswb";

/*
 * PUNPCKLBW - Unpack Low Bytes
 * Interleaves the low packed byte lanes from two operands.
 *
 * Syntax: punpcklbw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpcklbw xmm0, xmm1
 */
static const char* const ASM_PUNPCKLBW = "punpcklbw";

/*
 * PUNPCKLWD - Unpack Low Words
 * Interleaves the low packed word lanes from two operands.
 *
 * Syntax: punpcklwd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpcklwd xmm0, xmm1
 */
static const char* const ASM_PUNPCKLWD = "punpcklwd";

/*
 * PUNPCKLDQ - Unpack Low Doublewords
 * Interleaves the low packed dword lanes from two operands.
 *
 * Syntax: punpckldq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckldq xmm0, xmm1
 */
static const char* const ASM_PUNPCKLDQ = "punpckldq";

/*
 * PUNPCKLQDQ - Unpack Low Quadwords
 * Interleaves the low qword lanes from two XMM operands.
 *
 * Syntax: punpcklqdq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpcklqdq xmm0, xmm1
 */
static const char* const ASM_PUNPCKLQDQ = "punpcklqdq";

/*
 * PUNPCKHBW - Unpack High Bytes
 * Interleaves the high packed byte lanes from two operands.
 *
 * Syntax: punpckhbw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckhbw xmm0, xmm1
 */
static const char* const ASM_PUNPCKHBW = "punpckhbw";

/*
 * PUNPCKHWD - Unpack High Words
 * Interleaves the high packed word lanes from two operands.
 *
 * Syntax: punpckhwd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckhwd xmm0, xmm1
 */
static const char* const ASM_PUNPCKHWD = "punpckhwd";

/*
 * PUNPCKHDQ - Unpack High Doublewords
 * Interleaves the high packed dword lanes from two operands.
 *
 * Syntax: punpckhdq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckhdq xmm0, xmm1
 */
static const char* const ASM_PUNPCKHDQ = "punpckhdq";

/*
 * PUNPCKHQDQ - Unpack High Quadwords
 * Interleaves the high qword lanes from two XMM operands.
 *
 * Syntax: punpckhqdq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckhqdq xmm0, xmm1
 */
static const char* const ASM_PUNPCKHQDQ = "punpckhqdq";

/*
 * PSHUFD - Shuffle Packed Doublewords
 * Reorders packed dword lanes based on an immediate control.
 *
 * Syntax: pshufd xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pshufd xmm0, xmm1
 */
static const char* const ASM_PSHUFD = "pshufd";

/*
 * PSHUFHW - Shuffle High Words
 * Reorders the high packed word lanes of an XMM register based on an immediate control.
 *
 * Syntax: pshufhw xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pshufhw xmm0, xmm1
 */
static const char* const ASM_PSHUFHW = "pshufhw";

/*
 * PSHUFLW - Shuffle Low Words
 * Reorders the low packed word lanes of an XMM register based on an immediate control.
 *
 * Syntax: pshuflw xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pshuflw xmm0, xmm1
 */
static const char* const ASM_PSHUFLW = "pshuflw";

/*
 * MOVQ_SSE - Move Quadword (SSE/MMX-style)
 * Moves a 64-bit quantity between XMM registers, MMX registers, and/or memory depending on the
 * encoding. This symbol names the SIMD move form rather than any unrelated mnemonic reuse.
 *
 * Syntax: movq xmm0, qword ptr [rax]
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   movq xmm0, qword ptr [rax]
 */
static const char* const ASM_MOVQ_SSE = "movq";

/*
 * MOVD_SSE - Move Doubleword (SSE/MMX-style)
 * Moves a 32-bit integer value between a general-purpose register, memory, and an XMM register.
 * This symbol names the SIMD move form rather than any unrelated mnemonic reuse.
 *
 * Syntax: movd xmm0, eax
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   movd xmm0, eax
 */
static const char* const ASM_MOVD_SSE = "movd";

/*
 * PEXTRW - Extract Word
 * Extracts a selected 16-bit word from an XMM/MMX register into a general-purpose register or
 * memory.
 *
 * Syntax: pextrw eax, xmm0, 2
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   pextrw eax, xmm0, 2
 */
static const char* const ASM_PEXTRW = "pextrw";

/*
 * PINSRW - Insert Word
 * Inserts a 16-bit word from a general-purpose register or memory into a selected lane of an
 * XMM/MMX register.
 *
 * Syntax: pinsrw xmm0, eax, 2
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   pinsrw xmm0, eax, 2
 */
static const char* const ASM_PINSRW = "pinsrw";

/*
 * PMOVMSKB - Move Byte Mask
 * Collects the most-significant bit of each byte lane into a general-purpose register bitmask.
 *
 * Syntax: pmovmskb eax, xmm0
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   pmovmskb eax, xmm0
 */
static const char* const ASM_PMOVMSKB = "pmovmskb";

/*
 * MASKMOVDQU - Masked Store Bytes
 * Stores selected bytes from an XMM register to memory using the sign bits of a mask XMM
 * register.
 *
 * Syntax: maskmovdqu xmm0, xmm1
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   maskmovdqu xmm0, xmm1
 */
static const char* const ASM_MASKMOVDQU = "maskmovdqu";

/*
 * LDMXCSR - Load MXCSR
 * Loads the SIMD floating-point control/status register from memory.
 *
 * Syntax: ldmxcsr dword ptr [mxcsr]
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   ldmxcsr dword ptr [mxcsr]
 */
static const char* const ASM_LDMXCSR = "ldmxcsr";

/*
 * STMXCSR - Store MXCSR
 * Stores the SIMD floating-point control/status register to memory.
 *
 * Syntax: stmxcsr dword ptr [mxcsr]
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   stmxcsr dword ptr [mxcsr]
 */
static const char* const ASM_STMXCSR = "stmxcsr";

/*
 * ----------------------------------------------------------------------------
 * MMX state
 * ----------------------------------------------------------------------------
 *
 * EMMS is the companion to MM0-MM7 in section 5. It is not an SSE
 * instruction. It is placed here, ahead of the later SSE groups, because
 * leaving MMX state live and then executing x87 is the bug it exists to close.
 */

/*
 * EMMS - Empty MMX State
 * Marks every x87 tag empty so the x87 stack is usable again after MMX instructions.
 *
 * Syntax: EMMS
 * Operands: No explicit operands.
 * Flags affected: None. The x87 tag word is set to empty.
 * Notes: Does not change XMM or YMM state and does not change the x87 control word. Call it
 * after an MMX sequence and before FLD, FADD, or any other x87 instruction. XMM code does not
 * need EMMS.
 *
 * Example:
 *   emms
 */
static const char* const ASM_EMMS = "emms";

/*
 * MOVQ2DQ - Move Quadword from MMX to XMM
 * Copies the 64-bit MMX source into the low qword of an XMM register and zeros the high qword.
 *
 * Syntax: MOVQ2DQ xmm, mm
 * Operands: XMM destination and MMX source.
 * Flags affected: None.
 * Notes: SSE2 transfer from the MMX file into XMM. The high 64 bits of the destination become
 * zero. EMMS is still required later if the program returns to x87.
 *
 * Example:
 *   movq2dq xmm0, mm0
 */
static const char* const ASM_MOVQ2DQ = "movq2dq";

/*
 * MOVDQ2Q - Move Quadword from XMM to MMX
 * Copies the low 64 bits of an XMM register into an MMX register.
 *
 * Syntax: MOVDQ2Q mm, xmm
 * Operands: MMX destination and XMM source.
 * Flags affected: None.
 * Notes: SSE2 transfer from XMM into the MMX file. The high 64 bits of the XMM source are
 * ignored. The write marks an x87 tag valid; follow the MMX sequence with EMMS before x87.
 *
 * Example:
 *   movdq2q mm0, xmm0
 */
static const char* const ASM_MOVDQ2Q = "movdq2q";

/*
 * ----------------------------------------------------------------------------
 * Further SSE2 integer, saturating, and non-temporal operations
 * ----------------------------------------------------------------------------
 *
 * The entries above are the SSE and SSE2 operations the original table
 * carried: moves, packed floating-point arithmetic, and the basic integer
 * pipeline. The block below is the rest of SSE2 that those entries skipped:
 * saturating arithmetic, byte and word min/max, the sum of absolute
 * differences, the multiply-add, the whole-register byte shifts, and the
 * non-temporal stores. They use the same XMM file and the same SSE2 CPUID
 * bit (CPUID.01H:EDX[26]) as PADDB and MOVAPS.
 */

/*
 * PADDSB - Add Packed Signed Bytes with Saturation
 * SSE2. Adds 16 packed signed bytes and saturates each result instead of wrapping.
 *
 * Syntax: PADDSB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: Lane-wise, like PADDB. A sum above 127 becomes 127, and a sum below -128 becomes
 * -128. The bit pattern stays in its own byte.
 *
 * Example:
 *   paddsb xmm0, xmm1
 */
static const char* const ASM_PADDSB = "paddsb";

/*
 * PADDSW - Add Packed Signed Words with Saturation
 * SSE2. Adds 8 packed signed words and saturates each result instead of wrapping.
 *
 * Syntax: PADDSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: Lane-wise, like PADDB. A sum above 32767 becomes 32767, and a sum below -32768
 * becomes -32768. The bit pattern stays in its own word.
 *
 * Example:
 *   paddsw xmm0, xmm1
 */
static const char* const ASM_PADDSW = "paddsw";

/*
 * PADDUSB - Add Packed Unsigned Bytes with Saturation
 * SSE2. Adds 16 packed unsigned bytes and saturates each result instead of wrapping.
 *
 * Syntax: PADDUSB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: Lane-wise, like PADDB. A sum above 255 becomes 255. The bit pattern stays in its own
 * byte.
 *
 * Example:
 *   paddusb xmm0, xmm1
 */
static const char* const ASM_PADDUSB = "paddusb";

/*
 * PADDUSW - Add Packed Unsigned Words with Saturation
 * SSE2. Adds 8 packed unsigned words and saturates each result instead of wrapping.
 *
 * Syntax: PADDUSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: Lane-wise, like PADDB. A sum above 65535 becomes 65535. The bit pattern stays in its
 * own word.
 *
 * Example:
 *   paddusw xmm0, xmm1
 */
static const char* const ASM_PADDUSW = "paddusw";

/*
 * PSUBSB - Subtract Packed Signed Bytes with Saturation
 * SSE2. Subtracts 16 packed signed bytes and saturates each result instead of wrapping.
 *
 * Syntax: PSUBSB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: Lane-wise, like PADDB. A difference above 127 becomes 127, and a difference below
 * -128 becomes -128. The bit pattern stays in its own byte.
 *
 * Example:
 *   psubsb xmm0, xmm1
 */
static const char* const ASM_PSUBSB = "psubsb";

/*
 * PSUBSW - Subtract Packed Signed Words with Saturation
 * SSE2. Subtracts 8 packed signed words and saturates each result instead of wrapping.
 *
 * Syntax: PSUBSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: Lane-wise, like PADDB. A difference above 32767 becomes 32767, and a difference below
 * -32768 becomes -32768. The bit pattern stays in its own word.
 *
 * Example:
 *   psubsw xmm0, xmm1
 */
static const char* const ASM_PSUBSW = "psubsw";

/*
 * PSUBUSB - Subtract Packed Unsigned Bytes with Saturation
 * SSE2. Subtracts 16 packed unsigned bytes and saturates each result instead of wrapping.
 *
 * Syntax: PSUBUSB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: Lane-wise, like PADDB. A difference below 0 becomes 0. The bit pattern stays in its
 * own byte.
 *
 * Example:
 *   psubusb xmm0, xmm1
 */
static const char* const ASM_PSUBUSB = "psubusb";

/*
 * PSUBUSW - Subtract Packed Unsigned Words with Saturation
 * SSE2. Subtracts 8 packed unsigned words and saturates each result instead of wrapping.
 *
 * Syntax: PSUBUSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: Lane-wise, like PADDB. A difference below 0 becomes 0. The bit pattern stays in its
 * own word.
 *
 * Example:
 *   psubusw xmm0, xmm1
 */
static const char* const ASM_PSUBUSW = "psubusw";

/*
 * PAVGB - Average Packed Unsigned Bytes
 * Averages each pair of unsigned bytes, rounding up.
 *
 * Syntax: PAVGB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2 (the XMM form). Each result is (a + b + 1) >> 1, so a tie rounds toward the
 * larger integer. Unsigned. The MMX form of the same mnemonic uses MM registers.
 *
 * Example:
 *   pavgb xmm0, xmm1
 */
static const char* const ASM_PAVGB = "pavgb";

/*
 * PAVGW - Average Packed Unsigned Words
 * Averages each pair of unsigned words, rounding up.
 *
 * Syntax: PAVGW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2 (the XMM form). Each result is (a + b + 1) >> 1. Unsigned 16-bit lanes.
 *
 * Example:
 *   pavgw xmm0, xmm1
 */
static const char* const ASM_PAVGW = "pavgw";

/*
 * PMAXUB - Maximum of Packed Unsigned Bytes
 * Writes the larger of each pair of unsigned bytes.
 *
 * Syntax: PMAXUB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2. Comparison is unsigned, so 0xFF is greater than 0x7F. PMAXSB, the signed byte
 * form, is SSE4.1.
 *
 * Example:
 *   pmaxub xmm0, xmm1
 */
static const char* const ASM_PMAXUB = "pmaxub";

/*
 * PMAXSW - Maximum of Packed Signed Words
 * Writes the larger of each pair of signed words.
 *
 * Syntax: PMAXSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2. Comparison is signed. PMAXUW, the unsigned word form, is SSE4.1.
 *
 * Example:
 *   pmaxsw xmm0, xmm1
 */
static const char* const ASM_PMAXSW = "pmaxsw";

/*
 * PMINUB - Minimum of Packed Unsigned Bytes
 * Writes the smaller of each pair of unsigned bytes.
 *
 * Syntax: PMINUB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2. Comparison is unsigned. PMINSB is the SSE4.1 signed byte form.
 *
 * Example:
 *   pminub xmm0, xmm1
 */
static const char* const ASM_PMINUB = "pminub";

/*
 * PMINSW - Minimum of Packed Signed Words
 * Writes the smaller of each pair of signed words.
 *
 * Syntax: PMINSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2. Comparison is signed. PMINUW is the SSE4.1 unsigned word form.
 *
 * Example:
 *   pminsw xmm0, xmm1
 */
static const char* const ASM_PMINSW = "pminsw";

/*
 * PSADBW - Sum of Absolute Differences of Bytes
 * Sums the absolute differences of unsigned bytes into two word results.
 *
 * Syntax: PSADBW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2. The low 8 bytes produce one sum in the low word of the low qword, and the high
 * 8 bytes produce one sum in the low word of the high qword. The other bytes of each qword are
 * cleared. Used by motion estimation and by some memcmp-style distances.
 *
 * Example:
 *   psadbw xmm0, xmm1
 */
static const char* const ASM_PSADBW = "psadbw";

/*
 * PMADDWD - Multiply and Add Packed Words to Doublewords
 * Multiplies signed words and adds adjacent products into packed dwords.
 *
 * Syntax: PMADDWD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2. Eight signed 16-bit products are formed, then each adjacent pair is added to a
 * 32-bit result, four dwords in all. The most-negative word times itself is defined to
 * saturate rather than overflow the product.
 *
 * Example:
 *   pmaddwd xmm0, xmm1
 */
static const char* const ASM_PMADDWD = "pmaddwd";

/*
 * PMULHUW - Multiply Packed Unsigned Words and Store High
 * Multiplies unsigned words and keeps the high 16 bits of each 32-bit product.
 *
 * Syntax: PMULHUW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE2 XMM form. PMULHW is the signed high half, PMULLW is the low half, and PMULHUW is
 * the unsigned high half.
 *
 * Example:
 *   pmulhuw xmm0, xmm1
 */
static const char* const ASM_PMULHUW = "pmulhuw";

/*
 * PSLLDQ - Shift Double Quadword Left Logical
 * Shifts the whole 128-bit register left by an immediate number of bytes.
 *
 * Syntax: PSLLDQ xmm, imm8
 * Operands: XMM register and an 8-bit immediate byte count.
 * Flags affected: None.
 * Notes: SSE2. This is not lane-wise: the 16 bytes move as one block and zeros enter at the
 * low end. A count of 16 or more clears the register. PSLLW, PSLLD, and PSLLQ shift inside
 * each element; PSLLDQ shifts the elements themselves.
 *
 * Example:
 *   pslldq xmm0, 1
 */
static const char* const ASM_PSLLDQ = "pslldq";

/*
 * PSRLDQ - Shift Double Quadword Right Logical
 * Shifts the whole 128-bit register right by an immediate number of bytes.
 *
 * Syntax: PSRLDQ xmm, imm8
 * Operands: XMM register and an 8-bit immediate byte count.
 * Flags affected: None.
 * Notes: SSE2. Zeros enter at the high end. A count of 16 or more clears the register. The
 * shift is of the whole XMM value, not of each qword.
 *
 * Example:
 *   psrldq xmm0, 1
 */
static const char* const ASM_PSRLDQ = "psrldq";

/*
 * MOVNTPS - Store Packed Singles Non-Temporal
 * Stores 16 bytes of packed singles to aligned memory, hinting that the line will not be
 * reused soon.
 *
 * Syntax: MOVNTPS m128, xmm
 * Operands: Aligned 128-bit memory destination and an XMM source.
 * Flags affected: None.
 * Notes: SSE2 streaming store. The address must be 16-byte aligned or the instruction faults.
 * The store is weakly ordered. SFENCE, in section 16, makes preceding streaming stores visible
 * before a later load or store. A streaming store is not an atomic 16-byte write; see the
 * memory note at the start of this section.
 *
 * Example:
 *   movntps xmmword ptr [rax], xmm0
 */
static const char* const ASM_MOVNTPS = "movntps";

/*
 * MOVNTPD - Store Packed Doubles Non-Temporal
 * Stores 16 bytes of packed doubles to aligned memory with a non-temporal hint.
 *
 * Syntax: MOVNTPD m128, xmm
 * Operands: Aligned 128-bit memory destination and an XMM source.
 * Flags affected: None.
 * Notes: SSE2. Same ordering and alignment rules as MOVNTPS. Use SFENCE before a consumer
 * reads the written bytes.
 *
 * Example:
 *   movntpd xmmword ptr [rax], xmm0
 */
static const char* const ASM_MOVNTPD = "movntpd";

/*
 * MOVNTDQ - Store Integer Double Quadword Non-Temporal
 * Stores 16 bytes of integer XMM data to aligned memory with a non-temporal hint.
 *
 * Syntax: MOVNTDQ m128, xmm
 * Operands: Aligned 128-bit memory destination and an XMM source.
 * Flags affected: None.
 * Notes: SSE2 integer streaming store. Alignment, weak ordering, and the SFENCE requirement
 * match MOVNTPS. MOVNTDQA, the streaming load, is SSE4.1.
 *
 * Example:
 *   movntdq xmmword ptr [rax], xmm0
 */
static const char* const ASM_MOVNTDQ = "movntdq";

/*
 * MOVNTI - Store Doubleword or Quadword Non-Temporal
 * Stores a general-purpose register to memory with a non-temporal hint.
 *
 * Syntax: MOVNTI m32|m64, reg
 * Operands: Memory destination and a 32-bit or 64-bit general-purpose source.
 * Flags affected: None.
 * Notes: SSE2. The integer streaming store, not an XMM instruction. The address should be
 * naturally aligned for the operand size. Weakly ordered, like MOVNTPS; SFENCE publishes it.
 * This is not LOCK and it is not atomic with respect to a torn wider write.
 *
 * Example:
 *   movnti qword ptr [rax], rcx
 */
static const char* const ASM_MOVNTI = "movnti";

/*
 * ----------------------------------------------------------------------------
 * SSE3
 * ----------------------------------------------------------------------------
 *
 * CPUID.01H:ECX.SSE3[bit 0]. Horizontal adds, the add/subtract lane split,
 * and the duplicate moves. FISTTP is the SSE3 integer store and it is listed
 * with the other x87 stores in section 18. MONITOR and MWAIT are the SSE3
 * wait pair and they are listed with PAUSE in section 16.
 */

/*
 * ADDSUBPS - Add and Subtract Packed Singles
 * Subtracts the even single lanes and adds the odd single lanes.
 *
 * Syntax: ADDSUBPS xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None. Numeric exceptions update MXCSR.
 * Notes: SSE3. Lane 0 and lane 2 are dest - src. Lane 1 and lane 3 are dest + src. Used to
 * build a complex multiply.
 *
 * Example:
 *   addsubps xmm0, xmm1
 */
static const char* const ASM_ADDSUBPS = "addsubps";

/*
 * ADDSUBPD - Add and Subtract Packed Doubles
 * Subtracts the low double and adds the high double.
 *
 * Syntax: ADDSUBPD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None. Numeric exceptions update MXCSR.
 * Notes: SSE3. Lane 0 is dest - src and lane 1 is dest + src.
 *
 * Example:
 *   addsubpd xmm0, xmm1
 */
static const char* const ASM_ADDSUBPD = "addsubpd";

/*
 * HADDPS - Horizontal Add Packed Singles
 * Adds adjacent singles inside the destination and inside the source.
 *
 * Syntax: HADDPS xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None. Numeric exceptions update MXCSR.
 * Notes: SSE3. The two low results are the adjacent sums of the original destination (lanes
 * 0+1, then 2+3). The two high results are the adjacent sums of the source. This crosses pairs
 * inside the register, which PADDB-style lane arithmetic does not.
 *
 * Example:
 *   haddps xmm0, xmm1
 */
static const char* const ASM_HADDPS = "haddps";

/*
 * HADDPD - Horizontal Add Packed Doubles
 * Adds the two doubles of the destination and the two doubles of the source.
 *
 * Syntax: HADDPD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None. Numeric exceptions update MXCSR.
 * Notes: SSE3. The low result is dest0 + dest1. The high result is src0 + src1.
 *
 * Example:
 *   haddpd xmm0, xmm1
 */
static const char* const ASM_HADDPD = "haddpd";

/*
 * HSUBPS - Horizontal Subtract Packed Singles
 * Subtracts adjacent singles inside the destination and inside the source.
 *
 * Syntax: HSUBPS xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None. Numeric exceptions update MXCSR.
 * Notes: SSE3. Low results are dest0 - dest1 and dest2 - dest3. High results are src0 - src1
 * and src2 - src3.
 *
 * Example:
 *   hsubps xmm0, xmm1
 */
static const char* const ASM_HSUBPS = "hsubps";

/*
 * HSUBPD - Horizontal Subtract Packed Doubles
 * Subtracts the two doubles of the destination and the two doubles of the source.
 *
 * Syntax: HSUBPD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None. Numeric exceptions update MXCSR.
 * Notes: SSE3. The low result is dest0 - dest1. The high result is src0 - src1.
 *
 * Example:
 *   hsubpd xmm0, xmm1
 */
static const char* const ASM_HSUBPD = "hsubpd";

/*
 * MOVSHDUP - Move and Duplicate Odd Singles
 * Duplicates the odd-indexed singles into both lanes of each pair.
 *
 * Syntax: MOVSHDUP xmm, xmm/m128
 * Operands: XMM destination and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE3. Result lanes are src1, src1, src3, src3. The source is not required to be the
 * destination. MOVSLDUP is the even-lane twin.
 *
 * Example:
 *   movshdup xmm0, xmm1
 */
static const char* const ASM_MOVSHDUP = "movshdup";

/*
 * MOVSLDUP - Move and Duplicate Even Singles
 * Duplicates the even-indexed singles into both lanes of each pair.
 *
 * Syntax: MOVSLDUP xmm, xmm/m128
 * Operands: XMM destination and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE3. Result lanes are src0, src0, src2, src2.
 *
 * Example:
 *   movsldup xmm0, xmm1
 */
static const char* const ASM_MOVSLDUP = "movsldup";

/*
 * MOVDDUP - Move and Duplicate One Double
 * Duplicates one double into both lanes of the destination.
 *
 * Syntax: MOVDDUP xmm, xmm/m64
 * Operands: XMM destination. The source is the low double of an XMM register, or a 64-bit
 * memory operand.
 * Flags affected: None.
 * Notes: SSE3. The memory form loads 8 bytes, not 16. The register form duplicates the low
 * double and ignores the high double.
 *
 * Example:
 *   movddup xmm0, qword ptr [rax]
 */
static const char* const ASM_MOVDDUP = "movddup";

/*
 * LDDQU - Load Unaligned Integer Double Quadword
 * Loads 16 unaligned bytes into an XMM register.
 *
 * Syntax: LDDQU xmm, m128
 * Operands: XMM destination and a 128-bit memory source that need not be aligned.
 * Flags affected: None.
 * Notes: SSE3. On current Intel and AMD this is not faster than MOVDQU. It was a hint, on
 * early cores, to load across a cache-line split as two aligned pieces. It does not fault on
 * an unaligned address.
 *
 * Example:
 *   lddqu xmm0, xmmword ptr [rax]
 */
static const char* const ASM_LDDQU = "lddqu";

/*
 * ----------------------------------------------------------------------------
 * SSSE3
 * ----------------------------------------------------------------------------
 *
 * CPUID.01H:ECX.SSSE3[bit 9]. Supplemental SSE3: the byte shuffle, the
 * horizontal integer adds, the absolute value and sign operations, and the
 * byte align. These are integer XMM instructions.
 */

/*
 * PSHUFB - Packed Shuffle Bytes
 * Permutes the destination bytes under the control of a second XMM value, or zeros a byte
 * whose control has the high bit set.
 *
 * Syntax: PSHUFB xmm, xmm/m128
 * Operands: XMM destination/source (the data) and XMM or memory control.
 * Flags affected: None.
 * Notes: SSSE3. Each control byte's low 4 bits select a source byte from the destination. If
 * the control byte's high bit is set, the result byte is zero. The control itself is not
 * written. This is how a byte permute or a nibble lookup is usually spelled.
 *
 * Example:
 *   pshufb xmm0, xmm1
 */
static const char* const ASM_PSHUFB = "pshufb";

/*
 * PHADDW - Packed Horizontal Add Words
 * Adds adjacent signed words in the destination and in the source.
 *
 * Syntax: PHADDW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. The low four results are the adjacent sums of the original destination words.
 * The high four results are the adjacent sums of the source. Addition wraps; PHADDSW
 * saturates.
 *
 * Example:
 *   phaddw xmm0, xmm1
 */
static const char* const ASM_PHADDW = "phaddw";

/*
 * PHADDSW - Packed Horizontal Add Words with Saturation
 * Adds adjacent signed words and saturates each sum to a signed word.
 *
 * Syntax: PHADDSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. Same pairing as PHADDW. A sum outside -32768..32767 saturates.
 *
 * Example:
 *   phaddsw xmm0, xmm1
 */
static const char* const ASM_PHADDSW = "phaddsw";

/*
 * PHADDD - Packed Horizontal Add Doublewords
 * Adds adjacent signed dwords in the destination and in the source.
 *
 * Syntax: PHADDD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. Two sums come from the destination and two from the source. The add wraps
 * modulo 2^32.
 *
 * Example:
 *   phaddd xmm0, xmm1
 */
static const char* const ASM_PHADDD = "phaddd";

/*
 * PHSUBW - Packed Horizontal Subtract Words
 * Subtracts adjacent signed words in the destination and in the source.
 *
 * Syntax: PHSUBW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. Results are a0-a1, a2-a3, and so on for the destination, then the same for the
 * source. The subtract wraps.
 *
 * Example:
 *   phsubw xmm0, xmm1
 */
static const char* const ASM_PHSUBW = "phsubw";

/*
 * PHSUBSW - Packed Horizontal Subtract Words with Saturation
 * Subtracts adjacent signed words and saturates each difference to a signed word.
 *
 * Syntax: PHSUBSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. Same pairing as PHSUBW, with signed saturation.
 *
 * Example:
 *   phsubsw xmm0, xmm1
 */
static const char* const ASM_PHSUBSW = "phsubsw";

/*
 * PHSUBD - Packed Horizontal Subtract Doublewords
 * Subtracts adjacent signed dwords in the destination and in the source.
 *
 * Syntax: PHSUBD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. Two differences come from the destination and two from the source. The
 * subtract wraps.
 *
 * Example:
 *   phsubd xmm0, xmm1
 */
static const char* const ASM_PHSUBD = "phsubd";

/*
 * PMADDUBSW - Multiply Unsigned Bytes by Signed Bytes and Add
 * Multiplies unsigned bytes by signed bytes and adds adjacent products, saturating to a signed
 * word.
 *
 * Syntax: PMADDUBSW xmm, xmm/m128
 * Operands: XMM destination/source (unsigned bytes) and XMM or memory source (signed bytes).
 * Flags affected: None.
 * Notes: SSSE3. Sixteen byte products are formed. Each adjacent pair is added with signed
 * saturation into a word, eight words in all. The destination bytes are the unsigned factors.
 *
 * Example:
 *   pmaddubsw xmm0, xmm1
 */
static const char* const ASM_PMADDUBSW = "pmaddubsw";

/*
 * PMULHRSW - Multiply Packed Signed Words, Round, and Store High
 * Multiplies signed words, rounds, and keeps the high half of the rounded product.
 *
 * Syntax: PMULHRSW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. The 32-bit product is increased by 0x4000 and then bits 30:15 are stored. That
 * is the fixed-point multiply used when a Q15 value needs a rounded high half. PMULHW
 * truncates instead of rounding.
 *
 * Example:
 *   pmulhrsw xmm0, xmm1
 */
static const char* const ASM_PMULHRSW = "pmulhrsw";

/*
 * PABSB - Packed Absolute Value of Bytes
 * Replaces each signed byte with its absolute value.
 *
 * Syntax: PABSB xmm, xmm/m128
 * Operands: XMM destination and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. The absolute value of -128 (0x80) does not fit in a signed byte and remains
 * 0x80.
 *
 * Example:
 *   pabsb xmm0, xmm1
 */
static const char* const ASM_PABSB = "pabsb";

/*
 * PABSW - Packed Absolute Value of Words
 * Replaces each signed word with its absolute value.
 *
 * Syntax: PABSW xmm, xmm/m128
 * Operands: XMM destination and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. The absolute value of -32768 remains 0x8000.
 *
 * Example:
 *   pabsw xmm0, xmm1
 */
static const char* const ASM_PABSW = "pabsw";

/*
 * PABSD - Packed Absolute Value of Doublewords
 * Replaces each signed dword with its absolute value.
 *
 * Syntax: PABSD xmm, xmm/m128
 * Operands: XMM destination and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. The absolute value of 0x80000000 remains 0x80000000.
 *
 * Example:
 *   pabsd xmm0, xmm1
 */
static const char* const ASM_PABSD = "pabsd";

/*
 * PSIGNB - Packed Sign of Bytes
 * Negates, clears, or keeps each destination byte according to the sign of the source byte.
 *
 * Syntax: PSIGNB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. A negative source byte negates the destination byte, a zero source clears it,
 * and a positive source leaves it. Negating -128 saturates to -128.
 *
 * Example:
 *   psignb xmm0, xmm1
 */
static const char* const ASM_PSIGNB = "psignb";

/*
 * PSIGNW - Packed Sign of Words
 * Negates, clears, or keeps each destination word according to the sign of the source word.
 *
 * Syntax: PSIGNW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. Same sign rule as PSIGNB, on words. Negating -32768 saturates to -32768.
 *
 * Example:
 *   psignw xmm0, xmm1
 */
static const char* const ASM_PSIGNW = "psignw";

/*
 * PSIGND - Packed Sign of Doublewords
 * Negates, clears, or keeps each destination dword according to the sign of the source dword.
 *
 * Syntax: PSIGND xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSSE3. Same sign rule as PSIGNB, on dwords. Negating 0x80000000 saturates to itself.
 *
 * Example:
 *   psignd xmm0, xmm1
 */
static const char* const ASM_PSIGND = "psignd";

/*
 * PALIGNR - Packed Align Right
 * Concatenates the destination and the source and shifts the 32-byte result right by an
 * immediate byte count.
 *
 * Syntax: PALIGNR xmm, xmm/m128, imm8
 * Operands: XMM destination, XMM or memory source, and an 8-bit immediate byte count.
 * Flags affected: None.
 * Notes: SSSE3. The source is the low half of the concatenation and the destination is the
 * high half. The low 16 bytes after the shift are written back. A count of 0 yields the
 * source, a count of 16 yields the destination, and a count above 32 yields zero.
 *
 * Example:
 *   palignr xmm0, xmm1, 1
 */
static const char* const ASM_PALIGNR = "palignr";

/*
 * ----------------------------------------------------------------------------
 * SSE4.1
 * ----------------------------------------------------------------------------
 *
 * CPUID.01H:ECX.SSE4_1[bit 19]. Dword multiply, the remaining min/max
 * sizes, blends, rounding, insert and extract, sign and zero extension,
 * and the qword compare. PMULLD is documented with the other packed
 * multiplies earlier in this section; it is an SSE4.1 instruction, not SSE2.
 */

/*
 * PMULDQ - Multiply Packed Signed Doublewords to Quadwords
 * Multiplies the even signed dwords and writes two 64-bit products.
 *
 * Syntax: PMULDQ xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. Dwords 0 and 2 of each operand are multiplied. Dwords 1 and 3 are ignored.
 * PMULLD multiplies all four dwords and keeps the low 32 bits; it is listed with PMULLW above.
 *
 * Example:
 *   pmuldq xmm0, xmm1
 */
static const char* const ASM_PMULDQ = "pmuldq";

/*
 * PTEST - Logical Compare of Packed Test
 * Sets ZF and CF from a bitwise test of the two operands, without writing either operand.
 *
 * Syntax: PTEST xmm, xmm/m128
 * Operands: XMM source and XMM or 128-bit memory source. Neither operand is written.
 * Flags affected: ZF=1 when xmm1 AND xmm2 is zero. CF=1 when xmm1 AND NOT xmm2 is zero. AF,
 * OF, PF, and SF are cleared.
 * Notes: SSE4.1. ZF set means the source has no bit set in the positions the first operand has
 * set. CF set means every bit set in the first operand is also set in the source. This is the
 * packed form of TEST used on a mask of all-ones lanes.
 *
 * Example:
 *   ptest xmm0, xmm1
 */
static const char* const ASM_PTEST = "ptest";

/*
 * PCMPEQQ - Compare Packed Quadwords for Equality
 * Compares two qwords for equality and writes all-ones or all-zeros to each lane.
 *
 * Syntax: PCMPEQQ xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. Each matching qword becomes 0xFFFFFFFFFFFFFFFF and each mismatch becomes 0.
 * PCMPGTQ, the signed greater-than form, is SSE4.2.
 *
 * Example:
 *   pcmpeqq xmm0, xmm1
 */
static const char* const ASM_PCMPEQQ = "pcmpeqq";

/*
 * PACKUSDW - Pack Signed Doublewords to Unsigned Words with Saturation
 * Packs signed dwords from both operands into unsigned words, saturating.
 *
 * Syntax: PACKUSDW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. Values below 0 become 0 and values above 65535 become 65535. The low four
 * words of the result come from the destination and the high four from the source. PACKSSDW
 * saturates to signed words; PACKUSWB saturates signed words to unsigned bytes.
 *
 * Example:
 *   packusdw xmm0, xmm1
 */
static const char* const ASM_PACKUSDW = "packusdw";

/*
 * PMOVSXBW - Sign-Extend 8 Bytes to 8 Words
 * Sign-extends 8 bytes from the source into 8 words in the destination.
 *
 * Syntax: PMOVSXBW xmm, xmm/m64
 * Operands: XMM destination. The source is the low 8 bytes of an XMM register, or a qword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m64, not m128. The register form reads only the low 8
 * bytes. Each byte becomes one element of the result and the high bits of that element are
 * the sign extension.
 *
 * Example:
 *   pmovsxbw xmm0, qword ptr [rax]
 */
static const char* const ASM_PMOVSXBW = "pmovsxbw";

/*
 * PMOVSXBD - Sign-Extend 4 Bytes to 4 Dwords
 * Sign-extends 4 bytes from the source into 4 dwords in the destination.
 *
 * Syntax: PMOVSXBD xmm, xmm/m32
 * Operands: XMM destination. The source is the low 4 bytes of an XMM register, or a dword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m32, not m128. The register form reads only the low 4
 * bytes. Each byte becomes one element of the result and the high bits of that element are
 * the sign extension.
 *
 * Example:
 *   pmovsxbd xmm0, dword ptr [rax]
 */
static const char* const ASM_PMOVSXBD = "pmovsxbd";

/*
 * PMOVSXBQ - Sign-Extend 2 Bytes to 2 Qwords
 * Sign-extends 2 bytes from the source into 2 qwords in the destination.
 *
 * Syntax: PMOVSXBQ xmm, xmm/m16
 * Operands: XMM destination. The source is the low 2 bytes of an XMM register, or a word
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m16, not m128. The register form reads only the low 2
 * bytes. Each byte becomes one element of the result and the high bits of that element are
 * the sign extension.
 *
 * Example:
 *   pmovsxbq xmm0, word ptr [rax]
 */
static const char* const ASM_PMOVSXBQ = "pmovsxbq";

/*
 * PMOVSXWD - Sign-Extend 4 Words to 4 Dwords
 * Sign-extends 4 words from the source into 4 dwords in the destination.
 *
 * Syntax: PMOVSXWD xmm, xmm/m64
 * Operands: XMM destination. The source is the low 4 words of an XMM register, or a qword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m64, not m128. The register form reads only the low 4
 * words. Each word becomes one element of the result and the high bits of that element are
 * the sign extension.
 *
 * Example:
 *   pmovsxwd xmm0, qword ptr [rax]
 */
static const char* const ASM_PMOVSXWD = "pmovsxwd";

/*
 * PMOVSXWQ - Sign-Extend 2 Words to 2 Qwords
 * Sign-extends 2 words from the source into 2 qwords in the destination.
 *
 * Syntax: PMOVSXWQ xmm, xmm/m32
 * Operands: XMM destination. The source is the low 2 words of an XMM register, or a dword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m32, not m128. The register form reads only the low 2
 * words. Each word becomes one element of the result and the high bits of that element are
 * the sign extension.
 *
 * Example:
 *   pmovsxwq xmm0, dword ptr [rax]
 */
static const char* const ASM_PMOVSXWQ = "pmovsxwq";

/*
 * PMOVSXDQ - Sign-Extend 2 Dwords to 2 Qwords
 * Sign-extends 2 dwords from the source into 2 qwords in the destination.
 *
 * Syntax: PMOVSXDQ xmm, xmm/m64
 * Operands: XMM destination. The source is the low 2 dwords of an XMM register, or a qword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m64, not m128. The register form reads only the low 2
 * dwords. Each dword becomes one element of the result and the high bits of that element are
 * the sign extension.
 *
 * Example:
 *   pmovsxdq xmm0, qword ptr [rax]
 */
static const char* const ASM_PMOVSXDQ = "pmovsxdq";

/*
 * PMOVZXBW - Zero-Extend 8 Bytes to 8 Words
 * Zero-extends 8 bytes from the source into 8 words in the destination.
 *
 * Syntax: PMOVZXBW xmm, xmm/m64
 * Operands: XMM destination. The source is the low 8 bytes of an XMM register, or a qword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m64, not m128. The register form reads only the low 8
 * bytes. Each byte becomes one element of the result and the high bits of that element are
 * the zero extension.
 *
 * Example:
 *   pmovzxbw xmm0, qword ptr [rax]
 */
static const char* const ASM_PMOVZXBW = "pmovzxbw";

/*
 * PMOVZXBD - Zero-Extend 4 Bytes to 4 Dwords
 * Zero-extends 4 bytes from the source into 4 dwords in the destination.
 *
 * Syntax: PMOVZXBD xmm, xmm/m32
 * Operands: XMM destination. The source is the low 4 bytes of an XMM register, or a dword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m32, not m128. The register form reads only the low 4
 * bytes. Each byte becomes one element of the result and the high bits of that element are
 * the zero extension.
 *
 * Example:
 *   pmovzxbd xmm0, dword ptr [rax]
 */
static const char* const ASM_PMOVZXBD = "pmovzxbd";

/*
 * PMOVZXBQ - Zero-Extend 2 Bytes to 2 Qwords
 * Zero-extends 2 bytes from the source into 2 qwords in the destination.
 *
 * Syntax: PMOVZXBQ xmm, xmm/m16
 * Operands: XMM destination. The source is the low 2 bytes of an XMM register, or a word
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m16, not m128. The register form reads only the low 2
 * bytes. Each byte becomes one element of the result and the high bits of that element are
 * the zero extension.
 *
 * Example:
 *   pmovzxbq xmm0, word ptr [rax]
 */
static const char* const ASM_PMOVZXBQ = "pmovzxbq";

/*
 * PMOVZXWD - Zero-Extend 4 Words to 4 Dwords
 * Zero-extends 4 words from the source into 4 dwords in the destination.
 *
 * Syntax: PMOVZXWD xmm, xmm/m64
 * Operands: XMM destination. The source is the low 4 words of an XMM register, or a qword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m64, not m128. The register form reads only the low 4
 * words. Each word becomes one element of the result and the high bits of that element are
 * the zero extension.
 *
 * Example:
 *   pmovzxwd xmm0, qword ptr [rax]
 */
static const char* const ASM_PMOVZXWD = "pmovzxwd";

/*
 * PMOVZXWQ - Zero-Extend 2 Words to 2 Qwords
 * Zero-extends 2 words from the source into 2 qwords in the destination.
 *
 * Syntax: PMOVZXWQ xmm, xmm/m32
 * Operands: XMM destination. The source is the low 2 words of an XMM register, or a dword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m32, not m128. The register form reads only the low 2
 * words. Each word becomes one element of the result and the high bits of that element are
 * the zero extension.
 *
 * Example:
 *   pmovzxwq xmm0, dword ptr [rax]
 */
static const char* const ASM_PMOVZXWQ = "pmovzxwq";

/*
 * PMOVZXDQ - Zero-Extend 2 Dwords to 2 Qwords
 * Zero-extends 2 dwords from the source into 2 qwords in the destination.
 *
 * Syntax: PMOVZXDQ xmm, xmm/m64
 * Operands: XMM destination. The source is the low 2 dwords of an XMM register, or a qword
 * memory operand.
 * Flags affected: None.
 * Notes: SSE4.1. The memory form is m64, not m128. The register form reads only the low 2
 * dwords. Each dword becomes one element of the result and the high bits of that element are
 * the zero extension.
 *
 * Example:
 *   pmovzxdq xmm0, qword ptr [rax]
 */
static const char* const ASM_PMOVZXDQ = "pmovzxdq";

/*
 * PMINSB - Minimum of Packed Signed Bytes
 * Writes the smaller of each pair of signed bytes.
 *
 * Syntax: PMINSB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The compare is signed. 0x7F is greater than 0xFF. SSE2 already has PMINUB,
 * PMAXUB, PMINSW, and PMAXSW; this is one of the widths those four do not cover.
 *
 * Example:
 *   pminsb xmm0, xmm1
 */
static const char* const ASM_PMINSB = "pminsb";

/*
 * PMAXSB - Maximum of Packed Signed Bytes
 * Writes the larger of each pair of signed bytes.
 *
 * Syntax: PMAXSB xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The compare is signed. 0x7F is greater than 0xFF. SSE2 already has PMINUB,
 * PMAXUB, PMINSW, and PMAXSW; this is one of the widths those four do not cover.
 *
 * Example:
 *   pmaxsb xmm0, xmm1
 */
static const char* const ASM_PMAXSB = "pmaxsb";

/*
 * PMINUW - Minimum of Packed Unsigned Words
 * Writes the smaller of each pair of unsigned words.
 *
 * Syntax: PMINUW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The compare is unsigned. 0xFFFF is greater than 0x8000. SSE2 already has
 * PMINUB, PMAXUB, PMINSW, and PMAXSW; this is one of the widths those four do not cover.
 *
 * Example:
 *   pminuw xmm0, xmm1
 */
static const char* const ASM_PMINUW = "pminuw";

/*
 * PMAXUW - Maximum of Packed Unsigned Words
 * Writes the larger of each pair of unsigned words.
 *
 * Syntax: PMAXUW xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The compare is unsigned. 0xFFFF is greater than 0x8000. SSE2 already has
 * PMINUB, PMAXUB, PMINSW, and PMAXSW; this is one of the widths those four do not cover.
 *
 * Example:
 *   pmaxuw xmm0, xmm1
 */
static const char* const ASM_PMAXUW = "pmaxuw";

/*
 * PMINSD - Minimum of Packed Signed Doublewords
 * Writes the smaller of each pair of signed dwords.
 *
 * Syntax: PMINSD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The compare is signed. 0x7FFFFFFF is greater than 0xFFFFFFFF. SSE2 already
 * has PMINUB, PMAXUB, PMINSW, and PMAXSW; this is one of the widths those four do not cover.
 *
 * Example:
 *   pminsd xmm0, xmm1
 */
static const char* const ASM_PMINSD = "pminsd";

/*
 * PMAXSD - Maximum of Packed Signed Doublewords
 * Writes the larger of each pair of signed dwords.
 *
 * Syntax: PMAXSD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The compare is signed. 0x7FFFFFFF is greater than 0xFFFFFFFF. SSE2 already
 * has PMINUB, PMAXUB, PMINSW, and PMAXSW; this is one of the widths those four do not cover.
 *
 * Example:
 *   pmaxsd xmm0, xmm1
 */
static const char* const ASM_PMAXSD = "pmaxsd";

/*
 * PMINUD - Minimum of Packed Unsigned Doublewords
 * Writes the smaller of each pair of unsigned dwords.
 *
 * Syntax: PMINUD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The compare is unsigned. 0xFFFFFFFF is greater than 0x7FFFFFFF. SSE2 already
 * has PMINUB, PMAXUB, PMINSW, and PMAXSW; this is one of the widths those four do not cover.
 *
 * Example:
 *   pminud xmm0, xmm1
 */
static const char* const ASM_PMINUD = "pminud";

/*
 * PMAXUD - Maximum of Packed Unsigned Doublewords
 * Writes the larger of each pair of unsigned dwords.
 *
 * Syntax: PMAXUD xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The compare is unsigned. 0xFFFFFFFF is greater than 0x7FFFFFFF. SSE2 already
 * has PMINUB, PMAXUB, PMINSW, and PMAXSW; this is one of the widths those four do not cover.
 *
 * Example:
 *   pmaxud xmm0, xmm1
 */
static const char* const ASM_PMAXUD = "pmaxud";

/*
 * PBLENDVB - Variable Blend Packed Bytes
 * Selects each result byte from the destination or the source according to the sign bit of the
 * matching xmm0 byte.
 *
 * Syntax: PBLENDVB xmm, xmm/m128, xmm0
 * Operands: XMM destination/source, XMM or memory source, and the mask. ML64 requires the mask
 * to be written, and it must be XMM0.
 * Flags affected: None.
 * Notes: SSE4.1. A set sign bit in an xmm0 byte selects the source byte. A clear sign bit
 * keeps the destination byte. The machine encoding has no mask field. The VEX form VPBLENDVB
 * takes a real mask operand and can use any YMM register.
 *
 * Example:
 *   pblendvb xmm1, xmm2, xmm0
 */
static const char* const ASM_PBLENDVB = "pblendvb";

/*
 * PBLENDW - Blend Packed Words
 * Selects each result word from the destination or the source according to an immediate bit.
 *
 * Syntax: PBLENDW xmm, xmm/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and an 8-bit immediate.
 * Flags affected: None.
 * Notes: SSE4.1. Bit i of the immediate selects source word i when set and destination word i
 * when clear.
 *
 * Example:
 *   pblendw xmm0, xmm1, 0F0h
 */
static const char* const ASM_PBLENDW = "pblendw";

/*
 * BLENDPS - Blend Packed Singles
 * Selects each result single from the destination or the source according to an immediate bit.
 *
 * Syntax: BLENDPS xmm, xmm/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and an 8-bit immediate.
 * Flags affected: None.
 * Notes: SSE4.1. The low 4 bits of the immediate select the four lanes. A set bit takes the
 * source.
 *
 * Example:
 *   blendps xmm0, xmm1, 0Ah
 */
static const char* const ASM_BLENDPS = "blendps";

/*
 * BLENDPD - Blend Packed Doubles
 * Selects each result double from the destination or the source according to an immediate bit.
 *
 * Syntax: BLENDPD xmm, xmm/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and an 8-bit immediate.
 * Flags affected: None.
 * Notes: SSE4.1. Bit 0 selects the low double and bit 1 selects the high double. A set bit
 * takes the source.
 *
 * Example:
 *   blendpd xmm0, xmm1, 1
 */
static const char* const ASM_BLENDPD = "blendpd";

/*
 * BLENDVPS - Variable Blend Packed Singles
 * Selects each result single from the destination or the source according to the sign bit of
 * the matching xmm0 single.
 *
 * Syntax: BLENDVPS xmm, xmm/m128, xmm0
 * Operands: XMM destination/source, XMM or memory source, and the mask. ML64 requires the mask
 * to be written, and it must be XMM0.
 * Flags affected: None.
 * Notes: SSE4.1. The mask is xmm0, as with PBLENDVB, but the selector is the sign bit of each
 * single rather than of each byte. The encoding does not name the mask register.
 *
 * Example:
 *   blendvps xmm1, xmm2, xmm0
 */
static const char* const ASM_BLENDVPS = "blendvps";

/*
 * BLENDVPD - Variable Blend Packed Doubles
 * Selects each result double from the destination or the source according to the sign bit of
 * the matching xmm0 double.
 *
 * Syntax: BLENDVPD xmm, xmm/m128, xmm0
 * Operands: XMM destination/source, XMM or memory source, and the mask. ML64 requires the mask
 * to be written, and it must be XMM0.
 * Flags affected: None.
 * Notes: SSE4.1. The mask is xmm0. Each of the two sign bits selects a double. The encoding
 * does not name the mask register.
 *
 * Example:
 *   blendvpd xmm1, xmm2, xmm0
 */
static const char* const ASM_BLENDVPD = "blendvpd";

/*
 * DPPS - Dot Product of Packed Singles
 * Multiplies selected singles, sums those products, and broadcasts the sum into the lanes
 * selected by the immediate.
 *
 * Syntax: DPPS xmm, xmm/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and an 8-bit immediate.
 * Flags affected: None. Numeric exceptions update MXCSR.
 * Notes: SSE4.1. The high nibble of imm8 chooses which of the four products enter the sum. The
 * low nibble chooses which destination lanes receive that sum; the other lanes are cleared.
 *
 * Example:
 *   dpps xmm0, xmm1, 0F1h
 */
static const char* const ASM_DPPS = "dpps";

/*
 * DPPD - Dot Product of Packed Doubles
 * Multiplies the selected doubles, sums those products, and writes the sum into the lanes
 * selected by the immediate.
 *
 * Syntax: DPPD xmm, xmm/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and an 8-bit immediate.
 * Flags affected: None. Numeric exceptions update MXCSR.
 * Notes: SSE4.1. Bits 5:4 choose which of the two products enter the sum. Bits 1:0 choose
 * which destination lanes receive it.
 *
 * Example:
 *   dppd xmm0, xmm1, 31h
 */
static const char* const ASM_DPPD = "dppd";

/*
 * ROUNDPS - Round Packed Singles
 * Rounds each single in the source and writes four results.
 *
 * Syntax: ROUNDPS xmm, xmm/m128, imm8
 * Operands: XMM destination, XMM or memory source, and an 8-bit immediate rounding control.
 * Flags affected: None. MXCSR records an inexact result unless imm8 suppresses it.
 * Notes: SSE4.1. The low 2 bits select nearest-even (00), down (01), up (10), or toward zero
 * (11). Bit 2, when set, ignores those bits and uses the MXCSR rounding mode. Bit 3 suppresses
 * the precision exception.
 *
 * Example:
 *   roundps xmm0, xmm1, 3
 */
static const char* const ASM_ROUNDPS = "roundps";

/*
 * ROUNDPD - Round Packed Doubles
 * Rounds each double in the source and writes two results.
 *
 * Syntax: ROUNDPD xmm, xmm/m128, imm8
 * Operands: XMM destination, XMM or memory source, and an 8-bit immediate rounding control.
 * Flags affected: None. MXCSR records an inexact result unless imm8 suppresses it.
 * Notes: SSE4.1. The immediate fields mean the same thing as in ROUNDPS.
 *
 * Example:
 *   roundpd xmm0, xmm1, 3
 */
static const char* const ASM_ROUNDPD = "roundpd";

/*
 * ROUNDSS - Round Scalar Single
 * Rounds the low single and leaves the other three lanes of the destination unchanged.
 *
 * Syntax: ROUNDSS xmm, xmm/m128, imm8
 * Operands: XMM destination/source, XMM or 32-bit memory source, and an 8-bit immediate
 * rounding control.
 * Flags affected: None. MXCSR records an inexact result unless imm8 suppresses it.
 * Notes: SSE4.1. Only lane 0 is rounded. The immediate fields mean the same thing as in
 * ROUNDPS. A memory source is 32 bits.
 *
 * Example:
 *   roundss xmm0, xmm1, 3
 */
static const char* const ASM_ROUNDSS = "roundss";

/*
 * ROUNDSD - Round Scalar Double
 * Rounds the low double and leaves the high double of the destination unchanged.
 *
 * Syntax: ROUNDSD xmm, xmm/m128, imm8
 * Operands: XMM destination/source, XMM or 64-bit memory source, and an 8-bit immediate
 * rounding control.
 * Flags affected: None. MXCSR records an inexact result unless imm8 suppresses it.
 * Notes: SSE4.1. Only the low double is rounded. A memory source is 64 bits.
 *
 * Example:
 *   roundsd xmm0, xmm1, 3
 */
static const char* const ASM_ROUNDSD = "roundsd";

/*
 * INSERTPS - Insert Scalar Single
 * Inserts one single into a selected lane and can zero any of the four lanes.
 *
 * Syntax: INSERTPS xmm, xmm/m32, imm8
 * Operands: XMM destination/source, XMM or 32-bit memory source, and an 8-bit immediate.
 * Flags affected: None.
 * Notes: SSE4.1. Bits 7:6 select the source lane when the source is a register; a memory
 * source is already one single and those bits are ignored. Bits 5:4 select the destination
 * lane. Bits 3:0 zero the corresponding destination lanes after the insert.
 *
 * Example:
 *   insertps xmm0, xmm1, 10h
 */
static const char* const ASM_INSERTPS = "insertps";

/*
 * EXTRACTPS - Extract Scalar Single
 * Copies one single from an XMM register into a general-purpose register or memory.
 *
 * Syntax: EXTRACTPS reg/m32, xmm, imm8
 * Operands: 32-bit register or memory destination, XMM source, and an 8-bit lane select.
 * Flags affected: None.
 * Notes: SSE4.1. Bits 1:0 of the immediate select the lane. A register destination is
 * zero-extended to 64 bits in 64-bit mode.
 *
 * Example:
 *   extractps eax, xmm0, 1
 */
static const char* const ASM_EXTRACTPS = "extractps";

/*
 * PINSRB - Insert Byte
 * Inserts a byte from a general-purpose register or memory into a selected XMM byte.
 *
 * Syntax: PINSRB xmm, r32/m8, imm8
 * Operands: XMM destination/source, a 32-bit register or a byte of memory, and a byte index.
 * Flags affected: None.
 * Notes: SSE4.1. The immediate selects the byte lane, 0 through 15. Only the low 8 bits of a
 * register source are inserted. The other lanes stay as they were.
 *
 * Example:
 *   pinsrb xmm0, eax, 1
 */
static const char* const ASM_PINSRB = "pinsrb";

/*
 * PINSRD - Insert Doubleword
 * Inserts a dword from a general-purpose register or memory into a selected XMM dword.
 *
 * Syntax: PINSRD xmm, r32/m32, imm8
 * Operands: XMM destination/source, a 32-bit register or memory dword, and a dword index.
 * Flags affected: None.
 * Notes: SSE4.1. The immediate selects the dword lane, 0 through 3. PINSRW, the word form, is
 * the older SSE instruction.
 *
 * Example:
 *   pinsrd xmm0, eax, 1
 */
static const char* const ASM_PINSRD = "pinsrd";

/*
 * PINSRQ - Insert Quadword
 * Inserts a qword from a general-purpose register or memory into a selected XMM qword.
 *
 * Syntax: PINSRQ xmm, r64/m64, imm8
 * Operands: XMM destination/source, a 64-bit register or memory qword, and a qword index.
 * Flags affected: None.
 * Notes: SSE4.1. 64-bit mode. Bit 0 of the immediate selects the low or high qword. The other
 * qword stays as it was.
 *
 * Example:
 *   pinsrq xmm0, rax, 1
 */
static const char* const ASM_PINSRQ = "pinsrq";

/*
 * PEXTRB - Extract Byte
 * Copies one byte from an XMM register into a general-purpose register or memory.
 *
 * Syntax: PEXTRB reg/m8, xmm, imm8
 * Operands: Register or byte-memory destination, XMM source, and a byte index.
 * Flags affected: None.
 * Notes: SSE4.1. The immediate selects the byte. A register destination receives the byte
 * zero-extended.
 *
 * Example:
 *   pextrb eax, xmm0, 1
 */
static const char* const ASM_PEXTRB = "pextrb";

/*
 * PEXTRD - Extract Doubleword
 * Copies one dword from an XMM register into a general-purpose register or memory.
 *
 * Syntax: PEXTRD reg/m32, xmm, imm8
 * Operands: 32-bit register or memory destination, XMM source, and a dword index.
 * Flags affected: None.
 * Notes: SSE4.1. The immediate selects the dword, 0 through 3. A 64-bit register destination
 * is zero-extended.
 *
 * Example:
 *   pextrd eax, xmm0, 1
 */
static const char* const ASM_PEXTRD = "pextrd";

/*
 * PEXTRQ - Extract Quadword
 * Copies one qword from an XMM register into a general-purpose register or memory.
 *
 * Syntax: PEXTRQ reg/m64, xmm, imm8
 * Operands: 64-bit register or memory destination, XMM source, and a qword index.
 * Flags affected: None.
 * Notes: SSE4.1. 64-bit mode. Bit 0 of the immediate selects the low or high qword.
 *
 * Example:
 *   pextrq rax, xmm0, 1
 */
static const char* const ASM_PEXTRQ = "pextrq";

/*
 * MOVNTDQA - Load Integer Double Quadword Non-Temporal
 * Loads 16 aligned bytes with a streaming hint.
 *
 * Syntax: MOVNTDQA xmm, m128
 * Operands: XMM destination and an aligned 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The address must be 16-byte aligned. From write-combining memory this is a
 * streaming load. From ordinary write-back memory it behaves as an aligned load. It is the
 * load twin of MOVNTDQ, not a locked operation.
 *
 * Example:
 *   movntdqa xmm0, xmmword ptr [rax]
 */
static const char* const ASM_MOVNTDQA = "movntdqa";

/*
 * MPSADBW - Multiple Packed Sums of Absolute Difference
 * Computes eight sums of absolute differences of 4-byte windows.
 *
 * Syntax: MPSADBW xmm, xmm/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and an 8-bit offset select.
 * Flags affected: None.
 * Notes: SSE4.1. Each result word is the sum of absolute differences of four consecutive
 * unsigned bytes. The immediate picks which 4-byte window of the destination and which block
 * of the source are compared. Used by motion search. PSADBW is the coarser SSE2 sum over eight
 * bytes.
 *
 * Example:
 *   mpsadbw xmm0, xmm1, 0
 */
static const char* const ASM_MPSADBW = "mpsadbw";

/*
 * PHMINPOSUW - Minimum Unsigned Word and its Position
 * Finds the smallest unsigned word and writes that word and its index.
 *
 * Syntax: PHMINPOSUW xmm, xmm/m128
 * Operands: XMM destination and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.1. The low word of the destination is the minimum value. The next word is the
 * index of that word, 0 through 7, preferring the lowest index on a tie. The other lanes are
 * cleared.
 *
 * Example:
 *   phminposuw xmm0, xmm1
 */
static const char* const ASM_PHMINPOSUW = "phminposuw";

/*
 * ----------------------------------------------------------------------------
 * SSE4.2
 * ----------------------------------------------------------------------------
 *
 * CPUID.01H:ECX.SSE4_2[bit 20]. The string compares and the signed qword
 * compare. Two more SSE4.2 instructions live elsewhere because they are not
 * vector arithmetic: CRC32 is in section 17, and POPCNT is in section 10.
 *
 * PCMPESTRI and PCMPESTRM take explicit lengths in EAX and EDX. PCMPISTRI
 * and PCMPISTRM treat a zero element as the end of the string. The immediate
 * is the same control byte for all four:
 *   bit 0       0 = 16 bytes, 1 = 8 words
 *   bit 1       0 = unsigned, 1 = signed
 *   bits 3:2    00 equal any, 01 ranges, 10 equal each, 11 equal ordered
 *   bits 5:4    00 leave the result, 01 complement it, 10 and 11 are the
 *               masked forms, which complement only elements that are valid
 * For the index forms (STRI), bit 6 chooses the least significant set bit
 * when clear and the most significant when set. For the mask forms (STRM),
 * bit 6 clear leaves a bit mask in the low bits of XMM0, and bit 6 set
 * expands each result bit to a 0 or all-ones byte or word.
 */

/*
 * PCMPESTRI - Packed Compare Explicit-Length Strings, Return Index
 * Compares two XMM strings whose lengths are in EAX and EDX, and writes a match index to ECX.
 *
 * Syntax: PCMPESTRI xmm, xmm/m128, imm8
 * Operands: Two XMM string sources, the second of which may be memory, plus the imm8 control.
 * EAX and EDX hold the lengths.
 * Flags affected: CF=1 when the aggregated result is nonzero. ZF=1 when the absolute value of
 * EDX is shorter than the full 16 bytes or 8 words. SF=1 when EAX is shorter than that width.
 * OF is bit 0 of the result. AF and PF are cleared.
 * Notes: SSE4.2. Lengths are in elements, and the hardware saturates them at 16 or 8. ECX
 * receives the chosen index. If no result bit is set, ECX is 16 for a byte compare and 8 for a
 * word compare. The strings themselves are not written.
 *
 * Example:
 *   pcmpestri xmm0, xmm1, 0
 */
static const char* const ASM_PCMPESTRI = "pcmpestri";

/*
 * PCMPESTRM - Packed Compare Explicit-Length Strings, Return Mask
 * Compares two XMM strings whose lengths are in EAX and EDX, and writes a mask to XMM0.
 *
 * Syntax: PCMPESTRM xmm, xmm/m128, imm8
 * Operands: Two XMM string sources, the second of which may be memory, plus the imm8 control.
 * EAX and EDX hold the lengths. XMM0 receives the mask.
 * Flags affected: CF, ZF, SF, OF, AF, and PF are set the same way as PCMPESTRI.
 * Notes: SSE4.2. The mask is always written to XMM0, which is not encoded as a destination.
 * Bit 6 of imm8 selects a bit mask or an expanded byte or word mask. XMM0 may not be assumed
 * to keep its old value.
 *
 * Example:
 *   pcmpestrm xmm1, xmm2, 0
 */
static const char* const ASM_PCMPESTRM = "pcmpestrm";

/*
 * PCMPISTRI - Packed Compare Implicit-Length Strings, Return Index
 * Compares two XMM strings ended by a zero element, and writes a match index to ECX.
 *
 * Syntax: PCMPISTRI xmm, xmm/m128, imm8
 * Operands: Two XMM string sources, the second of which may be memory, plus the imm8 control.
 * There are no length registers.
 * Flags affected: CF=1 when the aggregated result is nonzero. ZF=1 when any element of the
 * second source is zero. SF=1 when any element of the first source is zero. OF is bit 0 of the
 * result. AF and PF are cleared.
 * Notes: SSE4.2. A zero byte, or a zero word when imm8 selects words, ends that string and is
 * not itself a valid element. ECX receives the index, or 16 or 8 when no result bit is set.
 * The strings are not written.
 *
 * Example:
 *   pcmpistri xmm0, xmm1, 0
 */
static const char* const ASM_PCMPISTRI = "pcmpistri";

/*
 * PCMPISTRM - Packed Compare Implicit-Length Strings, Return Mask
 * Compares two XMM strings ended by a zero element, and writes a mask to XMM0.
 *
 * Syntax: PCMPISTRM xmm, xmm/m128, imm8
 * Operands: Two XMM string sources, the second of which may be memory, plus the imm8 control.
 * XMM0 receives the mask.
 * Flags affected: CF, ZF, SF, OF, AF, and PF are set the same way as PCMPISTRI.
 * Notes: SSE4.2. The mask destination is implicit XMM0. Bit 6 of imm8 selects a bit mask or an
 * expanded mask, as with PCMPESTRM.
 *
 * Example:
 *   pcmpistrm xmm1, xmm2, 0
 */
static const char* const ASM_PCMPISTRM = "pcmpistrm";

/*
 * PCMPGTQ - Compare Packed Signed Quadwords for Greater Than
 * Compares signed qwords and writes all-ones or all-zeros to each lane.
 *
 * Syntax: PCMPGTQ xmm, xmm/m128
 * Operands: XMM destination/source and XMM or 128-bit memory source.
 * Flags affected: None.
 * Notes: SSE4.2. A destination qword that is greater than the source qword becomes all ones.
 * Otherwise it becomes zero. PCMPEQQ, the equality test, is SSE4.1.
 *
 * Example:
 *   pcmpgtq xmm0, xmm1
 */
static const char* const ASM_PCMPGTQ = "pcmpgtq";

/*
 * ============================================================================
 * 20. AVX Instructions
 * ============================================================================
 *
 * AVX widens XMM to YMM, 32 bytes, and adds VEX-encoded three-operand forms
 * that do not destroy a source: `vaddps ymm0, ymm1, ymm2` writes ymm0 and
 * leaves ymm1 alone. Legacy SSE is two-operand and overwrites the destination.
 * AVX and SSE share the low 128 bits of the same registers. Mixing a legacy
 * SSE instruction with AVX without VZEROUPPER can stall, because the processor
 * preserves the upper bits by merging instead of treating them as zero. Use
 * VZEROUPPER when leaving an AVX region.
 * AVX blends and masked moves, then the AVX2 integer, gather, and
 * variable-shift forms, follow the FMA entries at the end of this section.
 * A 256-bit integer instruction is AVX2. The matching 128-bit VEX form of an
 * older SSE opcode is usually plain AVX.
 *
 * Use AVX when you want 32-byte vectors or non-destructive operations. Use
 * SSE2 when you only need the Windows ABI's scalar float and double, or when
 * the target cannot assume AVX. AVX state is part of the same XSAVE area as
 * x87 and SSE; it is not a third coprocessor. The x87 stack is still separate.
 * A YMM store is 32 bytes and tears the same way a 16-byte SSE store does:
 * another thread need not observe it as one atomic value. See section 19.
 */
/*
 * VADDPS - Vector Add Packed Singles
 * Adds packed single-precision lanes in the three-operand AVX form.
 *
 * Syntax: vaddps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vaddps ymm0, ymm1, ymm2
 */
static const char* const ASM_VADDPS = "vaddps";

/*
 * VADDPD - Vector Add Packed Doubles
 * Adds packed double-precision lanes in the three-operand AVX form.
 *
 * Syntax: vaddpd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vaddpd ymm0, ymm1, ymm2
 */
static const char* const ASM_VADDPD = "vaddpd";

/*
 * VSUBPS - Vector Subtract Packed Singles
 * Subtracts packed single-precision lanes in the three-operand AVX form.
 *
 * Syntax: vsubps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vsubps ymm0, ymm1, ymm2
 */
static const char* const ASM_VSUBPS = "vsubps";

/*
 * VSUBPD - Vector Subtract Packed Doubles
 * Subtracts packed double-precision lanes in the three-operand AVX form.
 *
 * Syntax: vsubpd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vsubpd ymm0, ymm1, ymm2
 */
static const char* const ASM_VSUBPD = "vsubpd";

/*
 * VMULPS - Vector Multiply Packed Singles
 * Multiplies packed single-precision lanes in the three-operand AVX form.
 *
 * Syntax: vmulps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmulps ymm0, ymm1, ymm2
 */
static const char* const ASM_VMULPS = "vmulps";

/*
 * VMULPD - Vector Multiply Packed Doubles
 * Multiplies packed double-precision lanes in the three-operand AVX form.
 *
 * Syntax: vmulpd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmulpd ymm0, ymm1, ymm2
 */
static const char* const ASM_VMULPD = "vmulpd";

/*
 * VDIVPS - Vector Divide Packed Singles
 * Divides packed single-precision lanes in the three-operand AVX form.
 *
 * Syntax: vdivps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vdivps ymm0, ymm1, ymm2
 */
static const char* const ASM_VDIVPS = "vdivps";

/*
 * VDIVPD - Vector Divide Packed Doubles
 * Divides packed double-precision lanes in the three-operand AVX form.
 *
 * Syntax: vdivpd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vdivpd ymm0, ymm1, ymm2
 */
static const char* const ASM_VDIVPD = "vdivpd";

/*
 * VMOVAPS - Vector Move Aligned Packed Singles
 * Moves aligned packed single data using AVX encodings.
 *
 * Syntax: vmovaps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovaps ymm0, ymm1, ymm2
 */
static const char* const ASM_VMOVAPS = "vmovaps";

/*
 * VMOVAPD - Vector Move Aligned Packed Doubles
 * Moves aligned packed double data using AVX encodings.
 *
 * Syntax: vmovapd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovapd ymm0, ymm1, ymm2
 */
static const char* const ASM_VMOVAPD = "vmovapd";

/*
 * VMOVUPS - Vector Move Unaligned Packed Singles
 * Moves unaligned packed single data using AVX encodings.
 *
 * Syntax: vmovups ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovups ymm0, ymm1, ymm2
 */
static const char* const ASM_VMOVUPS = "vmovups";

/*
 * VMOVUPD - Vector Move Unaligned Packed Doubles
 * Moves unaligned packed double data using AVX encodings.
 *
 * Syntax: vmovupd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovupd ymm0, ymm1, ymm2
 */
static const char* const ASM_VMOVUPD = "vmovupd";

/*
 * VMOVDQA - Vector Move Aligned Integer Data
 * Moves aligned integer vector data using AVX encodings.
 *
 * Syntax: vmovdqa ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovdqa ymm0, ymm1, ymm2
 */
static const char* const ASM_VMOVDQA = "vmovdqa";

/*
 * VMOVDQU - Vector Move Unaligned Integer Data
 * Moves unaligned integer vector data using AVX encodings.
 *
 * Syntax: vmovdqu ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovdqu ymm0, ymm1, ymm2
 */
static const char* const ASM_VMOVDQU = "vmovdqu";

/*
 * VBROADCASTSS - Broadcast Scalar Single
 * Loads one scalar single-precision value and broadcasts it to all vector lanes.
 *
 * Syntax: vbroadcastss ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vbroadcastss ymm0, dword ptr [rax]
 */
static const char* const ASM_VBROADCASTSS = "vbroadcastss";

/*
 * VBROADCASTSD - Broadcast Scalar Double
 * Loads one scalar double-precision value and broadcasts it to all vector lanes.
 *
 * Syntax: vbroadcastsd ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vbroadcastsd ymm0, qword ptr [rax]
 */
static const char* const ASM_VBROADCASTSD = "vbroadcastsd";

/*
 * VBROADCASTF128 - Broadcast 128-bit Lane
 * Loads one 128-bit lane from memory and replicates it across a YMM destination.
 *
 * Syntax: vbroadcastf128 ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vbroadcastf128 ymm0, xmmword ptr [rax]
 */
static const char* const ASM_VBROADCASTF128 = "vbroadcastf128";

/*
 * VPERM2F128 - Permute 128-bit Lanes
 * Builds a new YMM register from selected 128-bit lanes of two YMM sources under immediate
 * control.
 *
 * Syntax: vperm2f128 ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vperm2f128 ymm0, ymm1, ymm2, 00100000b
 */
static const char* const ASM_VPERM2F128 = "vperm2f128";

/*
 * VINSERTF128 - Insert 128-bit Lane
 * Inserts a 128-bit XMM source or memory lane into a selected half of a YMM destination.
 *
 * Syntax: vinsertf128 ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vinsertf128 ymm0, ymm1, xmm2, 1
 */
static const char* const ASM_VINSERTF128 = "vinsertf128";

/*
 * VEXTRACTF128 - Extract 128-bit Lane
 * Extracts a selected 128-bit lane from a YMM source into an XMM register or memory.
 *
 * Syntax: vextractf128 ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vextractf128 xmm0, ymm1, 1
 */
static const char* const ASM_VEXTRACTF128 = "vextractf128";

/*
 * VZEROALL - Zero All XMM/YMM State
 * Clears all architectural XMM/YMM registers to zero.
 *
 * Syntax: vzeroall ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vzeroall
 */
static const char* const ASM_VZEROALL = "vzeroall";

/*
 * VZEROUPPER - Zero Upper Halves
 * Clears the upper 128 bits of all YMM registers to avoid AVX/SSE transition penalties.
 *
 * Syntax: vzeroupper ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vzeroupper
 */
static const char* const ASM_VZEROUPPER = "vzeroupper";

/*
 * VFMADD132PS - Fused Multiply-Add Packed Singles 132
 * Computes a fused multiply-add on packed single-precision lanes using the 132 operand ordering.
 *
 * Syntax: vfmadd132ps dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd132ps ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD132PS = "vfmadd132ps";

/*
 * VFMADD213PS - Fused Multiply-Add Packed Singles 213
 * Computes a fused multiply-add on packed single-precision lanes using the 213 operand ordering.
 *
 * Syntax: vfmadd213ps dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd213ps ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD213PS = "vfmadd213ps";

/*
 * VFMADD231PS - Fused Multiply-Add Packed Singles 231
 * Computes a fused multiply-add on packed single-precision lanes using the 231 operand ordering.
 *
 * Syntax: vfmadd231ps dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd231ps ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD231PS = "vfmadd231ps";

/*
 * VFMADD132PD - Fused Multiply-Add Packed Doubles 132
 * Computes a fused multiply-add on packed double-precision lanes using the 132 operand ordering.
 *
 * Syntax: vfmadd132pd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd132pd ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD132PD = "vfmadd132pd";

/*
 * VFMADD213PD - Fused Multiply-Add Packed Doubles 213
 * Computes a fused multiply-add on packed double-precision lanes using the 213 operand ordering.
 *
 * Syntax: vfmadd213pd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd213pd ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD213PD = "vfmadd213pd";

/*
 * VFMADD231PD - Fused Multiply-Add Packed Doubles 231
 * Computes a fused multiply-add on packed double-precision lanes using the 231 operand ordering.
 *
 * Syntax: vfmadd231pd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd231pd ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD231PD = "vfmadd231pd";

/*
 * VFMADD132SS - Fused Multiply-Add Scalar Single 132
 * Computes a fused multiply-add on the low scalar single-precision lane using the 132 operand
 * ordering.
 *
 * Syntax: vfmadd132ss dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd132ss ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD132SS = "vfmadd132ss";

/*
 * VFMADD213SS - Fused Multiply-Add Scalar Single 213
 * Computes a fused multiply-add on the low scalar single-precision lane using the 213 operand
 * ordering.
 *
 * Syntax: vfmadd213ss dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd213ss ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD213SS = "vfmadd213ss";

/*
 * VFMADD231SS - Fused Multiply-Add Scalar Single 231
 * Computes a fused multiply-add on the low scalar single-precision lane using the 231 operand
 * ordering.
 *
 * Syntax: vfmadd231ss dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd231ss ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD231SS = "vfmadd231ss";

/*
 * VFMADD132SD - Fused Multiply-Add Scalar Double 132
 * Computes a fused multiply-add on the low scalar double-precision lane using the 132 operand
 * ordering.
 *
 * Syntax: vfmadd132sd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd132sd ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD132SD = "vfmadd132sd";

/*
 * VFMADD213SD - Fused Multiply-Add Scalar Double 213
 * Computes a fused multiply-add on the low scalar double-precision lane using the 213 operand
 * ordering.
 *
 * Syntax: vfmadd213sd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd213sd ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD213SD = "vfmadd213sd";

/*
 * VFMADD231SD - Fused Multiply-Add Scalar Double 231
 * Computes a fused multiply-add on the low scalar double-precision lane using the 231 operand
 * ordering.
 *
 * Syntax: vfmadd231sd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd231sd ymm0, ymm1, ymm2
 */
static const char* const ASM_VFMADD231SD = "vfmadd231sd";

/*
 * ----------------------------------------------------------------------------
 * AVX blends and masked moves
 * ----------------------------------------------------------------------------
 *
 * These are AVX, CPUID.01H:ECX.AVX[bit 28], with OSXSAVE and the XMM/YMM
 * bits of XCR0 enabled. They are the 256-bit forms the float arithmetic
 * above does not cover. The mask is an explicit operand, unlike SSE4.1
 * BLENDVPS, which hides the mask in XMM0.
 */

/*
 * VBLENDPS - Blend Packed Singles
 * Selects each single from the first or second source according to an immediate bit.
 *
 * Syntax: VBLENDPS ymm, ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM source, YMM or memory source, and an 8-bit immediate.
 * Flags affected: None.
 * Notes: AVX. One bit per single, eight bits for a YMM source. A set bit selects the second
 * source. The 128-bit form uses the low 4 bits.
 *
 * Example:
 *   vblendps ymm0, ymm1, ymm2, 0F0h
 */
static const char* const ASM_VBLENDPS = "vblendps";

/*
 * VBLENDPD - Blend Packed Doubles
 * Selects each double from the first or second source according to an immediate bit.
 *
 * Syntax: VBLENDPD ymm, ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM source, YMM or memory source, and an 8-bit immediate.
 * Flags affected: None.
 * Notes: AVX. One bit per double, four bits for a YMM source. A set bit selects the second
 * source.
 *
 * Example:
 *   vblendpd ymm0, ymm1, ymm2, 0Ah
 */
static const char* const ASM_VBLENDPD = "vblendpd";

/*
 * VBLENDVPS - Variable Blend Packed Singles
 * Selects each single from the first or second source according to the sign bit of the mask.
 *
 * Syntax: VBLENDVPS ymm, ymm, ymm/m256, ymm
 * Operands: YMM destination, two YMM sources (the second may be memory), and a YMM mask
 * register.
 * Flags affected: None.
 * Notes: AVX. The mask is a register, not implicit XMM0. The sign bit of each mask single
 * selects the second source when set.
 *
 * Example:
 *   vblendvps ymm0, ymm1, ymm2, ymm3
 */
static const char* const ASM_VBLENDVPS = "vblendvps";

/*
 * VBLENDVPD - Variable Blend Packed Doubles
 * Selects each double from the first or second source according to the sign bit of the mask.
 *
 * Syntax: VBLENDVPD ymm, ymm, ymm/m256, ymm
 * Operands: YMM destination, two YMM sources (the second may be memory), and a YMM mask
 * register.
 * Flags affected: None.
 * Notes: AVX. The sign bit of each mask double selects the second source when set.
 *
 * Example:
 *   vblendvpd ymm0, ymm1, ymm2, ymm3
 */
static const char* const ASM_VBLENDVPD = "vblendvpd";

/*
 * VMASKMOVPS - Masked Move Packed Singles
 * Loads or stores packed singles under a sign-bit mask. Masked-off elements are not accessed.
 *
 * Syntax: VMASKMOVPS ymm, ymm, m256 or VMASKMOVPS m256, ymm, ymm
 * Operands: Load: destination data, mask register, memory. Store: memory, source data, mask
 * register.
 * Flags affected: None.
 * Notes: AVX. The high bit of each mask single enables that lane. A masked-off load writes
 * zero and does not fault on that element. A masked-off store does not write that element. The
 * load and the store spell the operands in different orders because the memory operand is the
 * destination only on the store.
 *
 * Example:
 *   vmaskmovps ymm0, ymm1, ymmword ptr [rax]
 */
static const char* const ASM_VMASKMOVPS = "vmaskmovps";

/*
 * VMASKMOVPD - Masked Move Packed Doubles
 * Loads or stores packed doubles under a sign-bit mask. Masked-off elements are not accessed.
 *
 * Syntax: VMASKMOVPD ymm, ymm, m256 or VMASKMOVPD m256, ymm, ymm
 * Operands: Load: destination data, mask register, memory. Store: memory, source data, mask
 * register.
 * Flags affected: None.
 * Notes: AVX. Same mask rule as VMASKMOVPS, one bit per double.
 *
 * Example:
 *   vmaskmovpd ymm0, ymm1, ymmword ptr [rax]
 */
static const char* const ASM_VMASKMOVPD = "vmaskmovpd";

/*
 * VPTEST - Logical Compare of Packed Test
 * Sets ZF and CF from a bitwise test of two YMM or XMM values, without writing them.
 *
 * Syntax: VPTEST ymm, ymm/m256
 * Operands: Two vector sources. The second may be memory. Neither is written.
 * Flags affected: ZF=1 when the AND of the operands is zero. CF=1 when the first AND NOT the
 * second is zero. AF, OF, PF, and SF are cleared.
 * Notes: AVX. The 256-bit form tests all 256 bits. PTEST is the SSE4.1 XMM instruction with
 * the same flag rule.
 *
 * Example:
 *   vptest ymm0, ymm1
 */
static const char* const ASM_VPTEST = "vptest";

/*
 * ----------------------------------------------------------------------------
 * AVX2
 * ----------------------------------------------------------------------------
 *
 * CPUID.07H:EBX.AVX2[bit 5], on top of AVX and the same XCR0 enablement.
 * The 256-bit integer forms below are AVX2. Where a 128-bit VEX form of the
 * same mnemonic exists, that narrower form is AVX unless the note says the
 * instruction is AVX2 at both widths (gathers, variable shifts, dword and
 * qword broadcasts, VPERMD, VPERMPS, VPBLENDD).
 *
 * A YMM unpack, shuffle, pack, or byte shift stays inside each 128-bit lane.
 * It does not cross the midpoint of the register. VPERMD, VPERMPS, and the
 * gathers do cross that boundary. VZEROUPPER still applies when this code
 * returns to legacy SSE.
 */

/*
 * VPADDB - Add Packed Bytes
 * AVX2. Adds 32 packed bytes. Each sum wraps modulo 256.
 *
 * Syntax: VPADDB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpaddb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPADDB = "vpaddb";

/*
 * VPADDW - Add Packed Words
 * AVX2. Adds 16 packed words. Each sum wraps modulo 65536.
 *
 * Syntax: VPADDW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpaddw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPADDW = "vpaddw";

/*
 * VPADDD - Add Packed Doublewords
 * AVX2. Adds 8 packed dwords. Each sum wraps modulo 2^32.
 *
 * Syntax: VPADDD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpaddd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPADDD = "vpaddd";

/*
 * VPADDQ - Add Packed Quadwords
 * AVX2. Adds 4 packed qwords. Each sum wraps modulo 2^64.
 *
 * Syntax: VPADDQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpaddq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPADDQ = "vpaddq";

/*
 * VPSUBB - Subtract Packed Bytes
 * AVX2. Subtracts 32 packed bytes. Each difference wraps.
 *
 * Syntax: VPSUBB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpsubb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSUBB = "vpsubb";

/*
 * VPSUBW - Subtract Packed Words
 * AVX2. Subtracts 16 packed words. Each difference wraps.
 *
 * Syntax: VPSUBW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpsubw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSUBW = "vpsubw";

/*
 * VPSUBD - Subtract Packed Doublewords
 * AVX2. Subtracts 8 packed dwords. Each difference wraps.
 *
 * Syntax: VPSUBD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpsubd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSUBD = "vpsubd";

/*
 * VPSUBQ - Subtract Packed Quadwords
 * AVX2. Subtracts 4 packed qwords. Each difference wraps.
 *
 * Syntax: VPSUBQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpsubq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSUBQ = "vpsubq";

/*
 * VPAND - Bitwise AND
 * AVX2. ANDs 256 bits.
 *
 * Syntax: VPAND ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpand ymm0, ymm1, ymm2
 */
static const char* const ASM_VPAND = "vpand";

/*
 * VPOR - Bitwise OR
 * AVX2. ORs 256 bits.
 *
 * Syntax: VPOR ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpor ymm0, ymm1, ymm2
 */
static const char* const ASM_VPOR = "vpor";

/*
 * VPXOR - Bitwise XOR
 * AVX2. XORs 256 bits. XORing a register with itself clears it, including the upper lane.
 *
 * Syntax: VPXOR ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpxor ymm0, ymm1, ymm2
 */
static const char* const ASM_VPXOR = "vpxor";

/*
 * VPMULLW - Multiply Packed Words, Low
 * AVX2. Multiplies signed words and keeps the low 16 bits of each product.
 *
 * Syntax: VPMULLW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmullw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMULLW = "vpmullw";

/*
 * VPMULHW - Multiply Packed Words, High Signed
 * AVX2. Multiplies signed words and keeps the high 16 bits of each product.
 *
 * Syntax: VPMULHW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmulhw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMULHW = "vpmulhw";

/*
 * VPMULHUW - Multiply Packed Words, High Unsigned
 * AVX2. Multiplies unsigned words and keeps the high 16 bits of each product.
 *
 * Syntax: VPMULHUW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmulhuw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMULHUW = "vpmulhuw";

/*
 * VPMULLD - Multiply Packed Doublewords, Low
 * AVX2. Multiplies signed dwords and keeps the low 32 bits of each product.
 *
 * Syntax: VPMULLD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmulld ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMULLD = "vpmulld";

/*
 * VPMULDQ - Multiply Even Doublewords to Quadwords
 * AVX2. Multiplies the even signed dwords of each 128-bit lane and writes 64-bit products. The
 * odd dwords are ignored.
 *
 * Syntax: VPMULDQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmuldq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMULDQ = "vpmuldq";

/*
 * VPMULUDQ - Multiply Even Unsigned Doublewords to Quadwords
 * AVX2. Multiplies the even unsigned dwords of each 128-bit lane and writes 64-bit products.
 *
 * Syntax: VPMULUDQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmuludq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMULUDQ = "vpmuludq";

/*
 * VPMULHRSW - Multiply Words, Round, Store High
 * AVX2. Multiplies signed words, adds 0x4000, and stores bits 30:15. The SSSE3 rounding
 * multiply, at 256 bits.
 *
 * Syntax: VPMULHRSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmulhrsw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMULHRSW = "vpmulhrsw";

/*
 * VPMADDWD - Multiply and Add Words to Doublewords
 * AVX2. Multiplies signed words and adds adjacent products into dwords, inside each 128-bit
 * lane.
 *
 * Syntax: VPMADDWD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmaddwd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMADDWD = "vpmaddwd";

/*
 * VPMADDUBSW - Multiply Unsigned Bytes by Signed Bytes and Add
 * AVX2. The SSSE3 byte multiply-add, at 256 bits, saturating each adjacent pair to a signed
 * word. It does not cross the 128-bit midpoint.
 *
 * Syntax: VPMADDUBSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpmaddubsw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMADDUBSW = "vpmaddubsw";

/*
 * VPAVGB - Average Packed Unsigned Bytes
 * AVX2. Averages unsigned bytes as (a + b + 1) >> 1.
 *
 * Syntax: VPAVGB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpavgb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPAVGB = "vpavgb";

/*
 * VPAVGW - Average Packed Unsigned Words
 * AVX2. Averages unsigned words as (a + b + 1) >> 1.
 *
 * Syntax: VPAVGW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpavgw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPAVGW = "vpavgw";

/*
 * VPSADBW - Sum of Absolute Differences of Bytes
 * AVX2. Sums absolute byte differences into a word at the bottom of each qword, the SSE2
 * PSADBW pattern in each 128-bit lane.
 *
 * Syntax: VPSADBW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpsadbw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSADBW = "vpsadbw";

/*
 * VPCMPEQB - Compare Packed Bytes for Equality
 * AVX2. Writes all-ones or zero to each byte that is equal or not.
 *
 * Syntax: VPCMPEQB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpcmpeqb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPCMPEQB = "vpcmpeqb";

/*
 * VPCMPEQW - Compare Packed Words for Equality
 * AVX2. Writes all-ones or zero to each word that is equal or not.
 *
 * Syntax: VPCMPEQW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpcmpeqw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPCMPEQW = "vpcmpeqw";

/*
 * VPCMPEQD - Compare Packed Doublewords for Equality
 * AVX2. Writes all-ones or zero to each dword that is equal or not.
 *
 * Syntax: VPCMPEQD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpcmpeqd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPCMPEQD = "vpcmpeqd";

/*
 * VPCMPEQQ - Compare Packed Quadwords for Equality
 * AVX2. Writes all-ones or zero to each qword that is equal or not.
 *
 * Syntax: VPCMPEQQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpcmpeqq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPCMPEQQ = "vpcmpeqq";

/*
 * VPCMPGTB - Compare Packed Signed Bytes for Greater Than
 * AVX2. Writes all-ones where the first source byte is greater, signed.
 *
 * Syntax: VPCMPGTB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpcmpgtb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPCMPGTB = "vpcmpgtb";

/*
 * VPCMPGTW - Compare Packed Signed Words for Greater Than
 * AVX2. Writes all-ones where the first source word is greater, signed.
 *
 * Syntax: VPCMPGTW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpcmpgtw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPCMPGTW = "vpcmpgtw";

/*
 * VPCMPGTD - Compare Packed Signed Doublewords for Greater Than
 * AVX2. Writes all-ones where the first source dword is greater, signed.
 *
 * Syntax: VPCMPGTD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpcmpgtd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPCMPGTD = "vpcmpgtd";

/*
 * VPCMPGTQ - Compare Packed Signed Quadwords for Greater Than
 * AVX2. Writes all-ones where the first source qword is greater, signed.
 *
 * Syntax: VPCMPGTQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The 256-bit form is AVX2. The first source is not destroyed unless it is also the
 * destination. Carry does not cross from one element into the next, and these particular
 * operations do not cross the 128-bit midpoint either.
 *
 * Example:
 *   vpcmpgtq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPCMPGTQ = "vpcmpgtq";

/*
 * VPANDN - Bitwise AND NOT
 * AVX2. ANDs the second source with the inverse of the first source.
 *
 * Syntax: VPANDN ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: ymm1 = NOT ymm2 AND ymm3. The inverted operand is the first source, not the
 * destination. That matches the VEX form and differs from reading legacy PANDN as 'invert the
 * old destination'.
 *
 * Example:
 *   vpandn ymm0, ymm1, ymm2
 */
static const char* const ASM_VPANDN = "vpandn";

/*
 * VPADDSB - Add Packed Signed Bytes with Saturation
 * AVX2. Signed saturating arithmetic on packed bytes.
 *
 * Syntax: VPADDSB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A sum outside -128..127 saturates. Each element saturates on its own.
 *
 * Example:
 *   vpaddsb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPADDSB = "vpaddsb";

/*
 * VPADDSW - Add Packed Signed Words with Saturation
 * AVX2. Signed saturating arithmetic on packed words.
 *
 * Syntax: VPADDSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A sum outside -32768..32767 saturates. Each element saturates on its own.
 *
 * Example:
 *   vpaddsw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPADDSW = "vpaddsw";

/*
 * VPADDUSB - Add Packed Unsigned Bytes with Saturation
 * AVX2. Unsigned saturating arithmetic on packed bytes.
 *
 * Syntax: VPADDUSB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A sum above 255 becomes 255. Each element saturates on its own.
 *
 * Example:
 *   vpaddusb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPADDUSB = "vpaddusb";

/*
 * VPADDUSW - Add Packed Unsigned Words with Saturation
 * AVX2. Unsigned saturating arithmetic on packed words.
 *
 * Syntax: VPADDUSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A sum above 65535 becomes 65535. Each element saturates on its own.
 *
 * Example:
 *   vpaddusw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPADDUSW = "vpaddusw";

/*
 * VPSUBSB - Subtract Packed Signed Bytes with Saturation
 * AVX2. Signed saturating arithmetic on packed bytes.
 *
 * Syntax: VPSUBSB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A difference outside -128..127 saturates. Each element saturates on its own.
 *
 * Example:
 *   vpsubsb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSUBSB = "vpsubsb";

/*
 * VPSUBSW - Subtract Packed Signed Words with Saturation
 * AVX2. Signed saturating arithmetic on packed words.
 *
 * Syntax: VPSUBSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A difference outside -32768..32767 saturates. Each element saturates on its own.
 *
 * Example:
 *   vpsubsw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSUBSW = "vpsubsw";

/*
 * VPSUBUSB - Subtract Packed Unsigned Bytes with Saturation
 * AVX2. Unsigned saturating arithmetic on packed bytes.
 *
 * Syntax: VPSUBUSB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A difference below 0 becomes 0. Each element saturates on its own.
 *
 * Example:
 *   vpsubusb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSUBUSB = "vpsubusb";

/*
 * VPSUBUSW - Subtract Packed Unsigned Words with Saturation
 * AVX2. Unsigned saturating arithmetic on packed words.
 *
 * Syntax: VPSUBUSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A difference below 0 becomes 0. Each element saturates on its own.
 *
 * Example:
 *   vpsubusw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSUBUSW = "vpsubusw";

/*
 * VPMINUB - Minimum Packed Unsigned Bytes
 * AVX2. Writes the smaller of each pair of unsigned bytes.
 *
 * Syntax: VPMINUB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is unsigned across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpminub ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMINUB = "vpminub";

/*
 * VPMINUW - Minimum Packed Unsigned Words
 * AVX2. Writes the smaller of each pair of unsigned words.
 *
 * Syntax: VPMINUW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is unsigned across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpminuw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMINUW = "vpminuw";

/*
 * VPMINUD - Minimum Packed Unsigned Doublewords
 * AVX2. Writes the smaller of each pair of unsigned dwords.
 *
 * Syntax: VPMINUD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is unsigned across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpminud ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMINUD = "vpminud";

/*
 * VPMINSB - Minimum Packed Signed Bytes
 * AVX2. Writes the smaller of each pair of signed bytes.
 *
 * Syntax: VPMINSB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is signed across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpminsb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMINSB = "vpminsb";

/*
 * VPMINSW - Minimum Packed Signed Words
 * AVX2. Writes the smaller of each pair of signed words.
 *
 * Syntax: VPMINSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is signed across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpminsw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMINSW = "vpminsw";

/*
 * VPMINSD - Minimum Packed Signed Doublewords
 * AVX2. Writes the smaller of each pair of signed dwords.
 *
 * Syntax: VPMINSD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is signed across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpminsd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMINSD = "vpminsd";

/*
 * VPMAXUB - Maximum Packed Unsigned Bytes
 * AVX2. Writes the larger of each pair of unsigned bytes.
 *
 * Syntax: VPMAXUB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is unsigned across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpmaxub ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMAXUB = "vpmaxub";

/*
 * VPMAXUW - Maximum Packed Unsigned Words
 * AVX2. Writes the larger of each pair of unsigned words.
 *
 * Syntax: VPMAXUW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is unsigned across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpmaxuw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMAXUW = "vpmaxuw";

/*
 * VPMAXUD - Maximum Packed Unsigned Doublewords
 * AVX2. Writes the larger of each pair of unsigned dwords.
 *
 * Syntax: VPMAXUD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is unsigned across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpmaxud ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMAXUD = "vpmaxud";

/*
 * VPMAXSB - Maximum Packed Signed Bytes
 * AVX2. Writes the larger of each pair of signed bytes.
 *
 * Syntax: VPMAXSB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is signed across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpmaxsb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMAXSB = "vpmaxsb";

/*
 * VPMAXSW - Maximum Packed Signed Words
 * AVX2. Writes the larger of each pair of signed words.
 *
 * Syntax: VPMAXSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is signed across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpmaxsw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMAXSW = "vpmaxsw";

/*
 * VPMAXSD - Maximum Packed Signed Doublewords
 * AVX2. Writes the larger of each pair of signed dwords.
 *
 * Syntax: VPMAXSD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The compare is signed across all 32 bytes. There is no qword min or max in AVX2.
 *
 * Example:
 *   vpmaxsd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPMAXSD = "vpmaxsd";

/*
 * VPSLLW - Shift Packed Words Left Logical
 * AVX2. Each word shifts left by the same immediate, a register count in XMM, or a memory
 * count. Bits leaving a word are discarded.
 *
 * Syntax: VPSLLW ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate count. A register or memory count is
 * the other form.
 * Flags affected: None
 * Notes: The immediate form takes the count shown in the example. The other form takes one
 * shared count from the low bits of an XMM register or from memory. A count at least as wide as
 * the element clears a logical shift. This is one count for every lane, not a separate count
 * per lane. VPSLLVD and its family are the per-lane counts.
 *
 * Example:
 *   vpsllw ymm0, ymm1, 3
 */
static const char* const ASM_VPSLLW = "vpsllw";

/*
 * VPSLLD - Shift Packed Doublewords Left Logical
 * AVX2. Each dword shifts left by one shared count.
 *
 * Syntax: VPSLLD ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate count. A register or memory count is
 * the other form.
 * Flags affected: None
 * Notes: The immediate form takes the count shown in the example. The other form takes one
 * shared count from the low bits of an XMM register or from memory. A count at least as wide as
 * the element clears a logical shift. This is one count for every lane, not a separate count
 * per lane. VPSLLVD and its family are the per-lane counts.
 *
 * Example:
 *   vpslld ymm0, ymm1, 3
 */
static const char* const ASM_VPSLLD = "vpslld";

/*
 * VPSLLQ - Shift Packed Quadwords Left Logical
 * AVX2. Each qword shifts left by one shared count.
 *
 * Syntax: VPSLLQ ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate count. A register or memory count is
 * the other form.
 * Flags affected: None
 * Notes: The immediate form takes the count shown in the example. The other form takes one
 * shared count from the low bits of an XMM register or from memory. A count at least as wide as
 * the element clears a logical shift. This is one count for every lane, not a separate count
 * per lane. VPSLLVD and its family are the per-lane counts.
 *
 * Example:
 *   vpsllq ymm0, ymm1, 3
 */
static const char* const ASM_VPSLLQ = "vpsllq";

/*
 * VPSRLW - Shift Packed Words Right Logical
 * AVX2. Each word shifts right by one shared count. Zeros enter at the top of the word.
 *
 * Syntax: VPSRLW ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate count. A register or memory count is
 * the other form.
 * Flags affected: None
 * Notes: The immediate form takes the count shown in the example. The other form takes one
 * shared count from the low bits of an XMM register or from memory. A count at least as wide as
 * the element clears a logical shift. This is one count for every lane, not a separate count
 * per lane. VPSLLVD and its family are the per-lane counts.
 *
 * Example:
 *   vpsrlw ymm0, ymm1, 3
 */
static const char* const ASM_VPSRLW = "vpsrlw";

/*
 * VPSRLD - Shift Packed Doublewords Right Logical
 * AVX2. Each dword shifts right by one shared count. Zeros enter.
 *
 * Syntax: VPSRLD ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate count. A register or memory count is
 * the other form.
 * Flags affected: None
 * Notes: The immediate form takes the count shown in the example. The other form takes one
 * shared count from the low bits of an XMM register or from memory. A count at least as wide as
 * the element clears a logical shift. This is one count for every lane, not a separate count
 * per lane. VPSLLVD and its family are the per-lane counts.
 *
 * Example:
 *   vpsrld ymm0, ymm1, 3
 */
static const char* const ASM_VPSRLD = "vpsrld";

/*
 * VPSRLQ - Shift Packed Quadwords Right Logical
 * AVX2. Each qword shifts right by one shared count. Zeros enter.
 *
 * Syntax: VPSRLQ ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate count. A register or memory count is
 * the other form.
 * Flags affected: None
 * Notes: The immediate form takes the count shown in the example. The other form takes one
 * shared count from the low bits of an XMM register or from memory. A count at least as wide as
 * the element clears a logical shift. This is one count for every lane, not a separate count
 * per lane. VPSLLVD and its family are the per-lane counts.
 *
 * Example:
 *   vpsrlq ymm0, ymm1, 3
 */
static const char* const ASM_VPSRLQ = "vpsrlq";

/*
 * VPSRAW - Shift Packed Words Right Arithmetic
 * AVX2. Each word shifts right by one shared count. The sign bit fills the top.
 *
 * Syntax: VPSRAW ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate count. A register or memory count is
 * the other form.
 * Flags affected: None
 * Notes: The immediate form takes the count shown in the example. The other form takes one
 * shared count from the low bits of an XMM register or from memory. A count at least as wide as
 * the element fills the word with its sign bit. This is one count for every lane, not a
 * separate count per lane. VPSRAVD is the per-lane arithmetic count.
 *
 * Example:
 *   vpsraw ymm0, ymm1, 3
 */
static const char* const ASM_VPSRAW = "vpsraw";

/*
 * VPSRAD - Shift Packed Doublewords Right Arithmetic
 * AVX2. Each dword shifts right by one shared count. The sign bit fills the top. There is no
 * arithmetic qword shift in AVX2.
 *
 * Syntax: VPSRAD ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate count. A register or memory count is
 * the other form.
 * Flags affected: None
 * Notes: The immediate form takes the count shown in the example. The other form takes one
 * shared count from the low bits of an XMM register or from memory. A count at least as wide as
 * the element fills the dword with its sign bit. This is one count for every lane, not a
 * separate count per lane. VPSRAVD is the per-lane arithmetic count. There is no VPSRAVQ.
 *
 * Example:
 *   vpsrad ymm0, ymm1, 3
 */
static const char* const ASM_VPSRAD = "vpsrad";

/*
 * VPSLLDQ - Shift Left Logical by Bytes
 * AVX2. Shifts each 128-bit lane left by an immediate number of bytes.
 *
 * Syntax: VPSLLDQ ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate byte count.
 * Flags affected: None
 * Notes: Zeros enter at the bottom of each lane. Bytes do not cross from the low 128 bits into
 * the high 128 bits. A count of 16 or more clears a lane. This is the 256-bit behavior of
 * PSLLDQ, not a shift of the whole YMM.
 *
 * Example:
 *   vpslldq ymm0, ymm1, 1
 */
static const char* const ASM_VPSLLDQ = "vpslldq";

/*
 * VPSRLDQ - Shift Right Logical by Bytes
 * AVX2. Shifts each 128-bit lane right by an immediate number of bytes.
 *
 * Syntax: VPSRLDQ ymm, ymm, imm8
 * Operands: YMM destination, YMM source, and an immediate byte count.
 * Flags affected: None
 * Notes: Zeros enter at the top of each lane. The high lane does not pull bytes from the low
 * lane. A count of 16 or more clears a lane.
 *
 * Example:
 *   vpsrldq ymm0, ymm1, 1
 */
static const char* const ASM_VPSRLDQ = "vpsrldq";

/*
 * VPSLLVD - Variable Shift Packed Doublewords Left
 * AVX2. Shifts each dword left by its own count.
 *
 * Syntax: VPSLLVD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The count is the corresponding dword of the second source. A count of 32 or more
 * clears that dword. Counts are not masked to 5 bits the way SHL masks a general-purpose
 * shift. Both the 128-bit and 256-bit forms are AVX2.
 *
 * Example:
 *   vpsllvd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSLLVD = "vpsllvd";

/*
 * VPSRLVD - Variable Shift Packed Doublewords Right Logical
 * AVX2. Shifts each dword right by its own count, filling with zeros.
 *
 * Syntax: VPSRLVD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A count of 32 or more clears that dword. Both widths are AVX2.
 *
 * Example:
 *   vpsrlvd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSRLVD = "vpsrlvd";

/*
 * VPSRAVD - Variable Shift Packed Doublewords Right Arithmetic
 * AVX2. Shifts each dword right by its own count, filling with the sign bit.
 *
 * Syntax: VPSRAVD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A count of 32 or more fills that dword with the sign bit. Both widths are AVX2. There
 * is no VPSRAVQ in AVX2.
 *
 * Example:
 *   vpsravd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSRAVD = "vpsravd";

/*
 * VPSLLVQ - Variable Shift Packed Quadwords Left
 * AVX2. Shifts each qword left by its own count.
 *
 * Syntax: VPSLLVQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A count of 64 or more clears that qword. Both widths are AVX2.
 *
 * Example:
 *   vpsllvq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSLLVQ = "vpsllvq";

/*
 * VPSRLVQ - Variable Shift Packed Quadwords Right Logical
 * AVX2. Shifts each qword right by its own count, filling with zeros.
 *
 * Syntax: VPSRLVQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: A count of 64 or more clears that qword. Both widths are AVX2.
 *
 * Example:
 *   vpsrlvq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSRLVQ = "vpsrlvq";

/*
 * VPSHUFB - Packed Shuffle Bytes
 * AVX2. Permutes bytes under a control vector. A control byte with the high bit set writes
 * zero. Indexes do not cross the 128-bit midpoint.
 *
 * Syntax: VPSHUFB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpshufb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSHUFB = "vpshufb";

/*
 * VPSHUFD - Shuffle Packed Doublewords
 * AVX2. Permutes the four dwords of each 128-bit lane by an immediate. The immediate does not
 * see the other lane.
 *
 * Syntax: VPSHUFD ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM or memory source, and an 8-bit immediate.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. The
 * immediate names the permutation inside each lane. VPERMD is the permute that does cross the
 * midpoint, and it takes the indexes from a register rather than from an immediate.
 *
 * Example:
 *   vpshufd ymm0, ymm1, 1
 */
static const char* const ASM_VPSHUFD = "vpshufd";

/*
 * VPSHUFHW - Shuffle Packed High Words
 * AVX2. Permutes the high four words of each 128-bit lane. The low four words of each lane are
 * copied.
 *
 * Syntax: VPSHUFHW ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM or memory source, and an 8-bit immediate.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. Only the
 * high four words of each lane are permuted. VPERMD crosses the midpoint; this shuffle does not.
 *
 * Example:
 *   vpshufhw ymm0, ymm1, 1
 */
static const char* const ASM_VPSHUFHW = "vpshufhw";

/*
 * VPSHUFLW - Shuffle Packed Low Words
 * AVX2. Permutes the low four words of each 128-bit lane. The high four words of each lane are
 * copied.
 *
 * Syntax: VPSHUFLW ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM or memory source, and an 8-bit immediate.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. Only the
 * low four words of each lane are permuted. VPERMD crosses the midpoint; this shuffle does not.
 *
 * Example:
 *   vpshuflw ymm0, ymm1, 1
 */
static const char* const ASM_VPSHUFLW = "vpshuflw";

/*
 * VPALIGNR - Packed Align Right
 * AVX2. Concatenates source pairs and shifts right by an immediate byte count, separately in
 * each 128-bit lane.
 *
 * Syntax: VPALIGNR ymm, ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM source, YMM or memory source, and an 8-bit immediate.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpalignr ymm0, ymm1, ymm2, 1
 */
static const char* const ASM_VPALIGNR = "vpalignr";

/*
 * VPHADDW - Horizontal Add Words
 * AVX2. Adds adjacent words inside each 128-bit lane. Pairs do not cross the midpoint.
 *
 * Syntax: VPHADDW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vphaddw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPHADDW = "vphaddw";

/*
 * VPHADDD - Horizontal Add Doublewords
 * AVX2. Adds adjacent dwords inside each 128-bit lane.
 *
 * Syntax: VPHADDD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vphaddd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPHADDD = "vphaddd";

/*
 * VPHADDSW - Horizontal Add Words with Saturation
 * AVX2. Adds adjacent signed words inside each 128-bit lane and saturates.
 *
 * Syntax: VPHADDSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vphaddsw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPHADDSW = "vphaddsw";

/*
 * VPHSUBW - Horizontal Subtract Words
 * AVX2. Subtracts adjacent words inside each 128-bit lane.
 *
 * Syntax: VPHSUBW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vphsubw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPHSUBW = "vphsubw";

/*
 * VPHSUBD - Horizontal Subtract Doublewords
 * AVX2. Subtracts adjacent dwords inside each 128-bit lane.
 *
 * Syntax: VPHSUBD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vphsubd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPHSUBD = "vphsubd";

/*
 * VPHSUBSW - Horizontal Subtract Words with Saturation
 * AVX2. Subtracts adjacent signed words inside each 128-bit lane and saturates.
 *
 * Syntax: VPHSUBSW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vphsubsw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPHSUBSW = "vphsubsw";

/*
 * VPSIGNB - Apply Sign to Packed Bytes
 * AVX2. Negates, clears, or keeps each first-source byte according to the sign of the second
 * source. The most-negative byte saturates to itself.
 *
 * Syntax: VPSIGNB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpsignb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSIGNB = "vpsignb";

/*
 * VPSIGNW - Apply Sign to Packed Words
 * AVX2. Negates, clears, or keeps each first-source word according to the sign of the second
 * source.
 *
 * Syntax: VPSIGNW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpsignw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSIGNW = "vpsignw";

/*
 * VPSIGND - Apply Sign to Packed Doublewords
 * AVX2. Negates, clears, or keeps each first-source dword according to the sign of the second
 * source.
 *
 * Syntax: VPSIGND ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpsignd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPSIGND = "vpsignd";

/*
 * VPACKSSWB - Pack Signed Words to Signed Bytes
 * AVX2. Packs signed words to signed bytes with saturation, inside each 128-bit lane.
 *
 * Syntax: VPACKSSWB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpacksswb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPACKSSWB = "vpacksswb";

/*
 * VPACKSSDW - Pack Signed Doublewords to Signed Words
 * AVX2. Packs signed dwords to signed words with saturation, inside each 128-bit lane.
 *
 * Syntax: VPACKSSDW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpackssdw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPACKSSDW = "vpackssdw";

/*
 * VPACKUSWB - Pack Signed Words to Unsigned Bytes
 * AVX2. Packs signed words to unsigned bytes with saturation, inside each 128-bit lane.
 *
 * Syntax: VPACKUSWB ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpackuswb ymm0, ymm1, ymm2
 */
static const char* const ASM_VPACKUSWB = "vpackuswb";

/*
 * VPACKUSDW - Pack Signed Doublewords to Unsigned Words
 * AVX2. Packs signed dwords to unsigned words with saturation, inside each 128-bit lane.
 *
 * Syntax: VPACKUSDW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpackusdw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPACKUSDW = "vpackusdw";

/*
 * VPUNPCKLBW - Unpack Low Bytes
 * AVX2. Interleaves the low bytes of each 128-bit lane.
 *
 * Syntax: VPUNPCKLBW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpunpcklbw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPUNPCKLBW = "vpunpcklbw";

/*
 * VPUNPCKLWD - Unpack Low Words
 * AVX2. Interleaves the low words of each 128-bit lane.
 *
 * Syntax: VPUNPCKLWD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpunpcklwd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPUNPCKLWD = "vpunpcklwd";

/*
 * VPUNPCKLDQ - Unpack Low Doublewords
 * AVX2. Interleaves the low dwords of each 128-bit lane.
 *
 * Syntax: VPUNPCKLDQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpunpckldq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPUNPCKLDQ = "vpunpckldq";

/*
 * VPUNPCKLQDQ - Unpack Low Quadwords
 * AVX2. Interleaves the low qword of each 128-bit lane.
 *
 * Syntax: VPUNPCKLQDQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpunpcklqdq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPUNPCKLQDQ = "vpunpcklqdq";

/*
 * VPUNPCKHBW - Unpack High Bytes
 * AVX2. Interleaves the high bytes of each 128-bit lane.
 *
 * Syntax: VPUNPCKHBW ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpunpckhbw ymm0, ymm1, ymm2
 */
static const char* const ASM_VPUNPCKHBW = "vpunpckhbw";

/*
 * VPUNPCKHWD - Unpack High Words
 * AVX2. Interleaves the high words of each 128-bit lane.
 *
 * Syntax: VPUNPCKHWD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpunpckhwd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPUNPCKHWD = "vpunpckhwd";

/*
 * VPUNPCKHDQ - Unpack High Doublewords
 * AVX2. Interleaves the high dwords of each 128-bit lane.
 *
 * Syntax: VPUNPCKHDQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpunpckhdq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPUNPCKHDQ = "vpunpckhdq";

/*
 * VPUNPCKHQDQ - Unpack High Quadwords
 * AVX2. Interleaves the high qword of each 128-bit lane.
 *
 * Syntax: VPUNPCKHQDQ ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Lane-local: the low 16 bytes and the high 16 bytes are processed separately. VPERMD
 * is the permute that does cross the midpoint.
 *
 * Example:
 *   vpunpckhqdq ymm0, ymm1, ymm2
 */
static const char* const ASM_VPUNPCKHQDQ = "vpunpckhqdq";

/*
 * VPABSB - Absolute Value of Packed Bytes
 * AVX2. Replaces each signed byte with its absolute value.
 *
 * Syntax: VPABSB ymm, ymm/m256
 * Operands: YMM destination and a YMM or memory source.
 * Flags affected: None
 * Notes: The most-negative element does not fit in the signed type and is left unchanged. Two
 * operands: the destination is not a source unless it is the same register.
 *
 * Example:
 *   vpabsb ymm0, ymm1
 */
static const char* const ASM_VPABSB = "vpabsb";

/*
 * VPABSW - Absolute Value of Packed Words
 * AVX2. Replaces each signed word with its absolute value.
 *
 * Syntax: VPABSW ymm, ymm/m256
 * Operands: YMM destination and a YMM or memory source.
 * Flags affected: None
 * Notes: The most-negative element does not fit in the signed type and is left unchanged. Two
 * operands: the destination is not a source unless it is the same register.
 *
 * Example:
 *   vpabsw ymm0, ymm1
 */
static const char* const ASM_VPABSW = "vpabsw";

/*
 * VPABSD - Absolute Value of Packed Doublewords
 * AVX2. Replaces each signed dword with its absolute value.
 *
 * Syntax: VPABSD ymm, ymm/m256
 * Operands: YMM destination and a YMM or memory source.
 * Flags affected: None
 * Notes: The most-negative element does not fit in the signed type and is left unchanged. Two
 * operands: the destination is not a source unless it is the same register.
 *
 * Example:
 *   vpabsd ymm0, ymm1
 */
static const char* const ASM_VPABSD = "vpabsd";

/*
 * VPMOVMSKB - Move Byte Mask
 * AVX2. Collects the sign bit of each of 32 bytes into a general-purpose register.
 *
 * Syntax: VPMOVMSKB r32, ymm
 * Operands: A 32-bit or 64-bit general-purpose destination and a YMM source.
 * Flags affected: None
 * Notes: The YMM form writes 32 bits and is AVX2. The XMM form writes 16 bits and is AVX. The
 * upper 32 bits of a 64-bit destination are cleared.
 *
 * Example:
 *   vpmovmskb eax, ymm0
 */
static const char* const ASM_VPMOVMSKB = "vpmovmskb";

/*
 * VPMOVSXBW - Sign-Extend Bytes to Words
 * AVX2. Sign-extends 16 bytes into 16 words.
 *
 * Syntax: VPMOVSXBW ymm, xmm/m128
 * Operands: YMM destination and an XMM or xmmword memory source holding 16 bytes.
 * Flags affected: None
 * Notes: The source is an XMM register or m128 memory, not a YMM register. The 128-bit VEX
 * form of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovsxbw ymm0, xmm1
 */
static const char* const ASM_VPMOVSXBW = "vpmovsxbw";

/*
 * VPMOVSXBD - Sign-Extend Bytes to Doublewords
 * AVX2. Sign-extends 8 bytes into 8 dwords.
 *
 * Syntax: VPMOVSXBD ymm, xmm/m64
 * Operands: YMM destination and an XMM or qword memory source holding 8 bytes.
 * Flags affected: None
 * Notes: The source is an XMM register or m64 memory, not a YMM register. The 128-bit VEX form
 * of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovsxbd ymm0, qword ptr [rax]
 */
static const char* const ASM_VPMOVSXBD = "vpmovsxbd";

/*
 * VPMOVSXBQ - Sign-Extend Bytes to Quadwords
 * AVX2. Sign-extends 4 bytes into 4 qwords.
 *
 * Syntax: VPMOVSXBQ ymm, xmm/m32
 * Operands: YMM destination and an XMM or dword memory source holding 4 bytes.
 * Flags affected: None
 * Notes: The source is an XMM register or m32 memory, not a YMM register. The 128-bit VEX form
 * of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovsxbq ymm0, dword ptr [rax]
 */
static const char* const ASM_VPMOVSXBQ = "vpmovsxbq";

/*
 * VPMOVSXWD - Sign-Extend Words to Doublewords
 * AVX2. Sign-extends 8 words into 8 dwords.
 *
 * Syntax: VPMOVSXWD ymm, xmm/m128
 * Operands: YMM destination and an XMM or xmmword memory source holding 8 words.
 * Flags affected: None
 * Notes: The source is an XMM register or m128 memory, not a YMM register. The 128-bit VEX
 * form of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovsxwd ymm0, xmm1
 */
static const char* const ASM_VPMOVSXWD = "vpmovsxwd";

/*
 * VPMOVSXWQ - Sign-Extend Words to Quadwords
 * AVX2. Sign-extends 4 words into 4 qwords.
 *
 * Syntax: VPMOVSXWQ ymm, xmm/m64
 * Operands: YMM destination and an XMM or qword memory source holding 4 words.
 * Flags affected: None
 * Notes: The source is an XMM register or m64 memory, not a YMM register. The 128-bit VEX form
 * of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovsxwq ymm0, qword ptr [rax]
 */
static const char* const ASM_VPMOVSXWQ = "vpmovsxwq";

/*
 * VPMOVSXDQ - Sign-Extend Doublewords to Quadwords
 * AVX2. Sign-extends 4 dwords into 4 qwords.
 *
 * Syntax: VPMOVSXDQ ymm, xmm/m128
 * Operands: YMM destination and an XMM or xmmword memory source holding 4 dwords.
 * Flags affected: None
 * Notes: The source is an XMM register or m128 memory, not a YMM register. The 128-bit VEX
 * form of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovsxdq ymm0, xmm1
 */
static const char* const ASM_VPMOVSXDQ = "vpmovsxdq";

/*
 * VPMOVZXBW - Zero-Extend Bytes to Words
 * AVX2. Zero-extends 16 bytes into 16 words.
 *
 * Syntax: VPMOVZXBW ymm, xmm/m128
 * Operands: YMM destination and an XMM or xmmword memory source holding 16 bytes.
 * Flags affected: None
 * Notes: The source is an XMM register or m128 memory, not a YMM register. The 128-bit VEX
 * form of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovzxbw ymm0, xmm1
 */
static const char* const ASM_VPMOVZXBW = "vpmovzxbw";

/*
 * VPMOVZXBD - Zero-Extend Bytes to Doublewords
 * AVX2. Zero-extends 8 bytes into 8 dwords.
 *
 * Syntax: VPMOVZXBD ymm, xmm/m64
 * Operands: YMM destination and an XMM or qword memory source holding 8 bytes.
 * Flags affected: None
 * Notes: The source is an XMM register or m64 memory, not a YMM register. The 128-bit VEX form
 * of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovzxbd ymm0, qword ptr [rax]
 */
static const char* const ASM_VPMOVZXBD = "vpmovzxbd";

/*
 * VPMOVZXBQ - Zero-Extend Bytes to Quadwords
 * AVX2. Zero-extends 4 bytes into 4 qwords.
 *
 * Syntax: VPMOVZXBQ ymm, xmm/m32
 * Operands: YMM destination and an XMM or dword memory source holding 4 bytes.
 * Flags affected: None
 * Notes: The source is an XMM register or m32 memory, not a YMM register. The 128-bit VEX form
 * of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovzxbq ymm0, dword ptr [rax]
 */
static const char* const ASM_VPMOVZXBQ = "vpmovzxbq";

/*
 * VPMOVZXWD - Zero-Extend Words to Doublewords
 * AVX2. Zero-extends 8 words into 8 dwords.
 *
 * Syntax: VPMOVZXWD ymm, xmm/m128
 * Operands: YMM destination and an XMM or xmmword memory source holding 8 words.
 * Flags affected: None
 * Notes: The source is an XMM register or m128 memory, not a YMM register. The 128-bit VEX
 * form of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovzxwd ymm0, xmm1
 */
static const char* const ASM_VPMOVZXWD = "vpmovzxwd";

/*
 * VPMOVZXWQ - Zero-Extend Words to Quadwords
 * AVX2. Zero-extends 4 words into 4 qwords.
 *
 * Syntax: VPMOVZXWQ ymm, xmm/m64
 * Operands: YMM destination and an XMM or qword memory source holding 4 words.
 * Flags affected: None
 * Notes: The source is an XMM register or m64 memory, not a YMM register. The 128-bit VEX form
 * of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovzxwq ymm0, qword ptr [rax]
 */
static const char* const ASM_VPMOVZXWQ = "vpmovzxwq";

/*
 * VPMOVZXDQ - Zero-Extend Doublewords to Quadwords
 * AVX2. Zero-extends 4 dwords into 4 qwords.
 *
 * Syntax: VPMOVZXDQ ymm, xmm/m128
 * Operands: YMM destination and an XMM or xmmword memory source holding 4 dwords.
 * Flags affected: None
 * Notes: The source is an XMM register or m128 memory, not a YMM register. The 128-bit VEX
 * form of the same mnemonic is AVX and produces an XMM result from a narrower source.
 *
 * Example:
 *   vpmovzxdq ymm0, xmm1
 */
static const char* const ASM_VPMOVZXDQ = "vpmovzxdq";

/*
 * VPBROADCASTB - Broadcast a Byte
 * AVX2. Copies one byte into every byte of the destination.
 *
 * Syntax: VPBROADCASTB ymm, m
 * Operands: YMM destination and a byte memory source, or the low byte of an XMM register.
 * Flags affected: None
 * Notes: Both the XMM and YMM forms are AVX2. The source is memory or the low element of an
 * XMM register. This is the integer broadcast. VBROADCASTSS and VBROADCASTSD, the
 * floating-point forms, are already in this section and are AVX.
 *
 * Example:
 *   vpbroadcastb ymm0, byte ptr [rax]
 */
static const char* const ASM_VPBROADCASTB = "vpbroadcastb";

/*
 * VPBROADCASTW - Broadcast a Word
 * AVX2. Copies one word into every word of the destination.
 *
 * Syntax: VPBROADCASTW ymm, m
 * Operands: YMM destination and a word memory source, or the low word of an XMM register.
 * Flags affected: None
 * Notes: Both the XMM and YMM forms are AVX2. The source is memory or the low element of an
 * XMM register. This is the integer broadcast. VBROADCASTSS and VBROADCASTSD, the
 * floating-point forms, are already in this section and are AVX.
 *
 * Example:
 *   vpbroadcastw ymm0, word ptr [rax]
 */
static const char* const ASM_VPBROADCASTW = "vpbroadcastw";

/*
 * VPBROADCASTD - Broadcast a Dword
 * AVX2. Copies one dword into every dword of the destination.
 *
 * Syntax: VPBROADCASTD ymm, m
 * Operands: YMM destination and a dword memory source, or the low dword of an XMM register.
 * Flags affected: None
 * Notes: Both the XMM and YMM forms are AVX2. The source is memory or the low element of an
 * XMM register. This is the integer broadcast. VBROADCASTSS and VBROADCASTSD, the
 * floating-point forms, are already in this section and are AVX.
 *
 * Example:
 *   vpbroadcastd ymm0, dword ptr [rax]
 */
static const char* const ASM_VPBROADCASTD = "vpbroadcastd";

/*
 * VPBROADCASTQ - Broadcast a Qword
 * AVX2. Copies one qword into every qword of the destination.
 *
 * Syntax: VPBROADCASTQ ymm, m
 * Operands: YMM destination and a qword memory source, or the low qword of an XMM register.
 * Flags affected: None
 * Notes: Both the XMM and YMM forms are AVX2. The source is memory or the low element of an
 * XMM register. This is the integer broadcast. VBROADCASTSS and VBROADCASTSD, the
 * floating-point forms, are already in this section and are AVX.
 *
 * Example:
 *   vpbroadcastq ymm0, qword ptr [rax]
 */
static const char* const ASM_VPBROADCASTQ = "vpbroadcastq";

/*
 * VBROADCASTI128 - Broadcast an Integer 128-Bit Lane
 * AVX2. Loads 16 bytes and copies that block into both lanes of a YMM register.
 *
 * Syntax: VBROADCASTI128 ymm, m128
 * Operands: YMM destination and 128-bit memory.
 * Flags affected: None
 * Notes: Memory source only. The two lanes then hold the same 128 bits. VBROADCASTF128 is the
 * floating-point spelling already listed above.
 *
 * Example:
 *   vbroadcasti128 ymm0, xmmword ptr [rax]
 */
static const char* const ASM_VBROADCASTI128 = "vbroadcasti128";

/*
 * VPERMD - Permute Doublewords
 * AVX2. Selects each destination dword from anywhere in the 256-bit data source.
 *
 * Syntax: VPERMD ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: The first source holds the indexes. The low 3 bits of each index dword select one of
 * the eight data dwords. This crosses the 128-bit midpoint, which VPSHUFD does not. Both
 * widths are AVX2.
 *
 * Example:
 *   vpermd ymm0, ymm1, ymm2
 */
static const char* const ASM_VPERMD = "vpermd";

/*
 * VPERMPS - Permute Singles
 * AVX2. Selects each destination single from anywhere in the 256-bit data source.
 *
 * Syntax: VPERMPS ymm, ymm, ymm/m256
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Same index rule as VPERMD, on single-precision elements. The first source is the
 * index vector. Both widths are AVX2.
 *
 * Example:
 *   vpermps ymm0, ymm1, ymm2
 */
static const char* const ASM_VPERMPS = "vpermps";

/*
 * VPERMQ - Permute Quadwords
 * AVX2. Permutes the four qwords of a YMM register by an immediate.
 *
 * Syntax: VPERMQ ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM or memory source, and an immediate permutation.
 * Flags affected: None
 * Notes: Each 2-bit field of the immediate selects the source qword for one destination qword.
 * Bits 1:0 select destination qword 0. This crosses the midpoint. The source is the second
 * operand, not a separate index register.
 *
 * Example:
 *   vpermq ymm0, ymm1, 4Eh
 */
static const char* const ASM_VPERMQ = "vpermq";

/*
 * VPERMPD - Permute Doubles
 * AVX2. Permutes the four doubles of a YMM register by an immediate.
 *
 * Syntax: VPERMPD ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM or memory source, and an immediate permutation.
 * Flags affected: None
 * Notes: Same immediate layout as VPERMQ. Bits 1:0 select the source double written to
 * destination lane 0.
 *
 * Example:
 *   vpermpd ymm0, ymm1, 4Eh
 */
static const char* const ASM_VPERMPD = "vpermpd";

/*
 * VPERM2I128 - Permute Two 128-Bit Integer Lanes
 * AVX2. Builds a YMM value by picking each 128-bit lane from either source, or by zeroing it.
 *
 * Syntax: VPERM2I128 ymm, ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: Immediate bits 1:0 select the source lane for the low half (0 and 1 are the first
 * source, 2 and 3 are the second). Bit 3 zeros the low half. Bits 5:4 and bit 7 do the same
 * for the high half.
 *
 * Example:
 *   vperm2i128 ymm0, ymm1, ymm2, 31h
 */
static const char* const ASM_VPERM2I128 = "vperm2i128";

/*
 * VINSERTI128 - Insert a 128-Bit Integer Lane
 * AVX2. Inserts 128 integer bits into one lane of a YMM value and copies the other lane from
 * the first source.
 *
 * Syntax: VINSERTI128 ymm, ymm, xmm/m128, imm8
 * Operands: YMM destination, YMM source, XMM or 128-bit memory insert, and an immediate lane
 * select.
 * Flags affected: None
 * Notes: Bit 0 of the immediate selects the high lane when set and the low lane when clear.
 * VINSERTF128 is the floating-point form already listed above.
 *
 * Example:
 *   vinserti128 ymm0, ymm1, xmm2, 1
 */
static const char* const ASM_VINSERTI128 = "vinserti128";

/*
 * VEXTRACTI128 - Extract a 128-Bit Integer Lane
 * AVX2. Copies one 128-bit lane of a YMM register to an XMM register or to memory.
 *
 * Syntax: VEXTRACTI128 xmm/m128, ymm, imm8
 * Operands: XMM or 128-bit memory destination, YMM source, and an immediate lane select.
 * Flags affected: None
 * Notes: Bit 0 of the immediate selects the high lane when set. VEXTRACTF128 is the
 * floating-point form already listed above.
 *
 * Example:
 *   vextracti128 xmm0, ymm1, 1
 */
static const char* const ASM_VEXTRACTI128 = "vextracti128";

/*
 * VPBLENDD - Blend Packed Doublewords
 * AVX2. Selects each dword from the first or second source according to an immediate bit.
 *
 * Syntax: VPBLENDD ymm, ymm, ymm/m256, imm8
 * Operands: YMM destination, YMM source, and a YMM or 256-bit memory source.
 * Flags affected: None
 * Notes: One bit per dword. Both the 128-bit and 256-bit forms are AVX2. A set bit selects the
 * second source.
 *
 * Example:
 *   vpblendd ymm0, ymm1, ymm2, 0F0h
 */
static const char* const ASM_VPBLENDD = "vpblendd";

/*
 * VPBLENDVB - Variable Blend Packed Bytes
 * AVX2. Selects each byte from the first or second source according to the sign bit of the
 * mask.
 *
 * Syntax: VPBLENDVB ymm, ymm, ymm/m256, ymm
 * Operands: YMM destination, two data sources, and a YMM mask.
 * Flags affected: None
 * Notes: The mask is an explicit register. Legacy PBLENDVB hides that mask in XMM0. The YMM
 * form is AVX2 and the XMM VEX form is AVX.
 *
 * Example:
 *   vpblendvb ymm0, ymm1, ymm2, ymm3
 */
static const char* const ASM_VPBLENDVB = "vpblendvb";

/*
 * VPGATHERDD - Gather Doublewords with Doubleword Indexes
 * AVX2. Gathers 8 dwords (or 4, in the XMM form) using dword indexes.
 *
 * Syntax: VPGATHERDD ymm, vm32y, ymm
 * Operands: YMM data destination, a memory operand whose index is a YMM register of dwords,
 * and a YMM mask.
 * Flags affected: None
 * Notes: The destination, the mask, and the index must be three different registers. The mask
 * uses the sign bit of each element. Completed lanes clear their mask bits, so a faulting
 * gather can be retried with the same instruction. In 64-bit mode a 32-bit index is
 * sign-extended to 64 bits before it is scaled and added to the base. Both the XMM and YMM
 * forms are AVX2.
 *
 * Example:
 *   vpgatherdd ymm0, dword ptr [rsi+ymm1*4], ymm2
 */
static const char* const ASM_VPGATHERDD = "vpgatherdd";

/*
 * VPGATHERDQ - Gather Quadwords with Doubleword Indexes
 * AVX2. Gathers 4 qwords into YMM using 4 dword indexes in XMM.
 *
 * Syntax: VPGATHERDQ ymm, vm32x, ymm
 * Operands: YMM data destination, a memory operand indexed by an XMM register of dwords, and a
 * YMM mask.
 * Flags affected: None
 * Notes: The destination, the mask, and the index must be three different registers. The mask
 * uses the sign bit of each element. Completed lanes clear their mask bits, so a faulting
 * gather can be retried with the same instruction. In 64-bit mode a 32-bit index is
 * sign-extended to 64 bits before it is scaled and added to the base. Both the XMM and YMM
 * forms are AVX2.
 *
 * Example:
 *   vpgatherdq ymm0, qword ptr [rsi+xmm1*8], ymm2
 */
static const char* const ASM_VPGATHERDQ = "vpgatherdq";

/*
 * VPGATHERQD - Gather Doublewords with Quadword Indexes
 * AVX2. Gathers 4 dwords into XMM using 4 qword indexes in YMM.
 *
 * Syntax: VPGATHERQD xmm, vm64y, xmm
 * Operands: XMM data destination, a memory operand indexed by a YMM register of qwords, and an
 * XMM mask.
 * Flags affected: None
 * Notes: The destination, the mask, and the index must be three different registers. The mask
 * uses the sign bit of each element. Completed lanes clear their mask bits, so a faulting
 * gather can be retried with the same instruction. In 64-bit mode a 32-bit index is
 * sign-extended to 64 bits before it is scaled and added to the base. Both the XMM and YMM
 * forms are AVX2.
 *
 * Example:
 *   vpgatherqd xmm0, dword ptr [rsi+ymm1*4], xmm2
 */
static const char* const ASM_VPGATHERQD = "vpgatherqd";

/*
 * VPGATHERQQ - Gather Quadwords with Quadword Indexes
 * AVX2. Gathers 4 qwords using qword indexes.
 *
 * Syntax: VPGATHERQQ ymm, vm64y, ymm
 * Operands: YMM data destination, a memory operand indexed by a YMM register of qwords, and a
 * YMM mask.
 * Flags affected: None
 * Notes: The destination, the mask, and the index must be three different registers. The mask
 * uses the sign bit of each element. Completed lanes clear their mask bits, so a faulting
 * gather can be retried with the same instruction. In 64-bit mode a 32-bit index is
 * sign-extended to 64 bits before it is scaled and added to the base. Both the XMM and YMM
 * forms are AVX2.
 *
 * Example:
 *   vpgatherqq ymm0, qword ptr [rsi+ymm1*8], ymm2
 */
static const char* const ASM_VPGATHERQQ = "vpgatherqq";

/*
 * VGATHERDPS - Gather Singles with Doubleword Indexes
 * AVX2. Gathers 8 singles using dword indexes.
 *
 * Syntax: VGATHERDPS ymm, vm32y, ymm
 * Operands: YMM data destination, a memory operand indexed by a YMM register of dwords, and a
 * YMM mask.
 * Flags affected: None
 * Notes: The destination, the mask, and the index must be three different registers. The mask
 * uses the sign bit of each element. Completed lanes clear their mask bits, so a faulting
 * gather can be retried with the same instruction. In 64-bit mode a 32-bit index is
 * sign-extended to 64 bits before it is scaled and added to the base. Both the XMM and YMM
 * forms are AVX2.
 *
 * Example:
 *   vgatherdps ymm0, dword ptr [rsi+ymm1*4], ymm2
 */
static const char* const ASM_VGATHERDPS = "vgatherdps";

/*
 * VGATHERDPD - Gather Doubles with Doubleword Indexes
 * AVX2. Gathers 4 doubles into YMM using 4 dword indexes in XMM.
 *
 * Syntax: VGATHERDPD ymm, vm32x, ymm
 * Operands: YMM data destination, a memory operand indexed by an XMM register of dwords, and a
 * YMM mask.
 * Flags affected: None
 * Notes: The destination, the mask, and the index must be three different registers. The mask
 * uses the sign bit of each element. Completed lanes clear their mask bits, so a faulting
 * gather can be retried with the same instruction. In 64-bit mode a 32-bit index is
 * sign-extended to 64 bits before it is scaled and added to the base. Both the XMM and YMM
 * forms are AVX2.
 *
 * Example:
 *   vgatherdpd ymm0, qword ptr [rsi+xmm1*8], ymm2
 */
static const char* const ASM_VGATHERDPD = "vgatherdpd";

/*
 * VGATHERQPS - Gather Singles with Quadword Indexes
 * AVX2. Gathers 4 singles into XMM using 4 qword indexes in YMM.
 *
 * Syntax: VGATHERQPS xmm, vm64y, xmm
 * Operands: XMM data destination, a memory operand indexed by a YMM register of qwords, and an
 * XMM mask.
 * Flags affected: None
 * Notes: The destination, the mask, and the index must be three different registers. The mask
 * uses the sign bit of each element. Completed lanes clear their mask bits, so a faulting
 * gather can be retried with the same instruction. In 64-bit mode a 32-bit index is
 * sign-extended to 64 bits before it is scaled and added to the base. Both the XMM and YMM
 * forms are AVX2.
 *
 * Example:
 *   vgatherqps xmm0, dword ptr [rsi+ymm1*4], xmm2
 */
static const char* const ASM_VGATHERQPS = "vgatherqps";

/*
 * VGATHERQPD - Gather Doubles with Quadword Indexes
 * AVX2. Gathers 4 doubles using qword indexes.
 *
 * Syntax: VGATHERQPD ymm, vm64y, ymm
 * Operands: YMM data destination, a memory operand indexed by a YMM register of qwords, and a
 * YMM mask.
 * Flags affected: None
 * Notes: The destination, the mask, and the index must be three different registers. The mask
 * uses the sign bit of each element. Completed lanes clear their mask bits, so a faulting
 * gather can be retried with the same instruction. In 64-bit mode a 32-bit index is
 * sign-extended to 64 bits before it is scaled and added to the base. Both the XMM and YMM
 * forms are AVX2.
 *
 * Example:
 *   vgatherqpd ymm0, qword ptr [rsi+ymm1*8], ymm2
 */
static const char* const ASM_VGATHERQPD = "vgatherqpd";

/*
 * VPMASKMOVD - Masked Move Packed Doublewords
 * AVX2. Loads or stores dwords under a sign-bit mask. Masked-off elements are not accessed.
 *
 * Syntax: VPMASKMOVD ymm, ymm, m256
 * Operands: Load form shown. The store form is memory, then the data register, then the mask.
 * Flags affected: None
 * Notes: Load spelling: destination, mask, memory. Store spelling: memory, source, mask. A
 * masked-off load writes zero. A masked-off store writes nothing. The integer twin of
 * VMASKMOVPS.
 *
 * Example:
 *   vpmaskmovd ymm0, ymm1, ymmword ptr [rax]
 */
static const char* const ASM_VPMASKMOVD = "vpmaskmovd";

/*
 * VPMASKMOVQ - Masked Move Packed Quadwords
 * AVX2. Loads or stores qwords under a sign-bit mask. Masked-off elements are not accessed.
 *
 * Syntax: VPMASKMOVQ ymm, ymm, m256
 * Operands: Load form shown. The store form is memory, then the data register, then the mask.
 * Flags affected: None
 * Notes: Same load and store operand order as VPMASKMOVD, with one mask bit per qword.
 *
 * Example:
 *   vpmaskmovq ymm0, ymm1, ymmword ptr [rax]
 */
static const char* const ASM_VPMASKMOVQ = "vpmaskmovq";

/*
 * ============================================================================
 * 21. MASM Directives and Keywords
 * ============================================================================
 *
 * Nothing in this section is a CPU instruction. Each token is a MASM directive,
 * operator, or keyword. The assembler consumes it while building the object
 * file: it switches sections, expands text, emits data, or, for INVOKE, a PROC
 * language type, and the high-level .IF / .WHILE forms, generates the real
 * instructions for you. Calling one of these a "macro" is only half right.
 * MACRO itself is textual substitution. .MODEL, .DATA, and INVOKE are
 * directives with fixed meanings. They do not change the processor.
 *
 * Banners later in this section mark four groups without moving any constant:
 * macro definitions, assembly-time conditionals, diagnostic output (.ECHO and
 * .ERR*), and calling-convention annotations (STDCALL, C, SYSCALL, and the
 * other language types). Those groups are assembler features, not opcodes.
 */
/*
 * MODEL - .MODEL Directive
 * Selects the memory model, language convention, and stack distance assumptions for traditional
 * 16-bit or 32-bit MASM programs.
 *
 * Syntax: .MODEL model [, language] [, stacktype]
 * Common usage: Used near the top of a source file before segment declarations and procedure
 * definitions. `.model flat, stdcall` means one flat 4GB address space and stdcall as the
 * default language for PROC and INVOKE. In 16-bit models it also decides whether pointers are
 * near or far and how the segments are grouped. It does not switch the CPU mode by itself.
 * ML64, the 64-bit assembler, does not use .MODEL the same way; you write the frame yourself.
 * Implication: a language on the model makes a later PROC with arguments emit a matching
 * prologue and a callee-cleanup or caller-cleanup epilogue. This generator does not rely on
 * that. It emits the moves so they stay visible.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .model flat, stdcall
 */
static const char* const ASM_MODEL = ".model";

/*
 * STACK - .STACK Directive
 * Declares the stack segment and optionally its size in simplified segment models.
 *
 * Syntax: .STACK [size]
 * Common usage: In a 16-bit DOS program `.stack 4096` tells the linker to reserve a stack
 * segment of that many bytes and the loader sets SS:SP. A modern Windows process gets its
 * stack from the PE header (StackReserve), not from this directive. 64-bit user code does
 * not use .STACK. It is a simplified-segment leftover. The example is the realistic 16-bit
 * form, not something a Windows x64 function contains.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .stack 4096
 */
static const char* const ASM_STACK = ".stack";

/*
 * DATA - .DATA Directive
 * Begins the initialized data segment in simplified segment syntax.
 *
 * Syntax: .DATA
 * Common usage: Switches the assembler's current section to initialized data. Bytes defined
 * here (DB, DW, DD, DQ) are stored in the image and are writable at run time. It emits no
 * instruction. ML64 still accepts `.data` as a section switch; the implication is only where
 * the following definitions land in the object file.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .data
 */
static const char* const ASM_DATA = ".data";

/*
 * DATAQ - .DATA? Directive
 * Begins the uninitialized data segment used for BSS-style storage.
 *
 * Syntax: .DATA?
 * Common usage: Same kind of section switch as .DATA, but the loader zeroes the range and the
 * bytes are not stored in the file. Use it for large buffers. Still relevant in 32-bit and
 * 64-bit MASM. It does not allocate a register or emit a CPU instruction; it only moves the
 * location counter into the BSS-style section.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .data?
 */
static const char* const ASM_DATAQ = ".data?";

/*
 * CONST - .CONST Directive
 * Begins a read-only or constant data segment in simplified segment syntax.
 *
 * Syntax: .CONST
 * Common usage: Use for lookup tables, literal scalars, and other immutable data.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .const
 */
static const char* const ASM_CONST = ".const";

/*
 * CODE - .CODE Directive
 * Begins the code segment in simplified segment syntax.
 *
 * Syntax: .CODE
 * Common usage: Executable instructions, PROC definitions, and labels are commonly placed here.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .code
 */
static const char* const ASM_CODE = ".code";

/*
 * STARTUP - .STARTUP Directive
 * Emits simplified-language startup code and defines the program entry point in older MASM
 * program models.
 *
 * Syntax: .STARTUP
 * Common usage: MASM expands it to the entry sequence for the current .MODEL: in a 16-bit
 * small model that is "set DS to DGROUP" plus a label the linker treats as the start. It is
 * not a CPU instruction. 32-bit and 64-bit Windows programs use a linker /ENTRY symbol and do
 * not use .STARTUP. It is a 16-bit simplified-model leftover. The realistic example is a tiny
 * DOS-style program, not a Windows x64 function.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .startup
 */
static const char* const ASM_STARTUP = ".startup";

/*
 * EXIT - .EXIT Directive
 * Emits simplified-language program termination code, optionally with a return code.
 *
 * Syntax: .EXIT [expression]
 * Common usage: MASM expands `.exit 0` into whatever the memory model uses to terminate. In a
 * 16-bit DOS model that is an INT 21h call. In a Win32 model it is a call to the imported
 * ExitProcess symbol with the return code. MASM does not invent an API; it calls the import
 * that the model already named. This is not the same text as `invoke ExitProcess, 0`, but a
 * Win32 expansion ends at that call. This generator does not emit .EXIT. It writes the call
 * so the import stays visible. 64-bit user code returns from its entry point or calls
 * ExitProcess explicitly.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .exit 0
 */
static const char* const ASM_EXIT = ".exit";

/*
 * IF - .IF High-Level Directive
 * Starts a MASM high-level conditional block whose condition is evaluated at run time and
 * lowered into compare/jump code.
 *
 * Syntax: .IF condition
 * Common usage: Pairs with .ELSE/.ELSEIF/.ENDIF to write structured assembly control flow.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .if eax == 0
 */
static const char* const ASM_IF = ".if";

/*
 * ELSE - .ELSE High-Level Directive
 * Begins the alternative block of a preceding .IF or .ELSEIF.
 *
 * Syntax: .ELSE
 * Common usage: Used in MASM high-level run-time flow blocks.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .else
 */
static const char* const ASM_ELSE = ".else";

/*
 * ELSEIF - .ELSEIF High-Level Directive
 * Adds another conditional branch to a MASM high-level .IF block.
 *
 * Syntax: .ELSEIF condition
 * Common usage: MASM expands it into additional compare/jump logic.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .elseif eax > 0
 */
static const char* const ASM_ELSEIF = ".elseif";

/*
 * ENDIF - .ENDIF High-Level Directive
 * Terminates a MASM high-level .IF block.
 *
 * Syntax: .ENDIF
 * Common usage: Closes .IF/.ELSEIF/.ELSE structures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .endif
 */
static const char* const ASM_ENDIF = ".endif";

/*
 * WHILE - .WHILE High-Level Directive
 * Starts a MASM high-level loop that evaluates a run-time condition before each iteration.
 *
 * Syntax: .WHILE condition
 * Common usage: This is not the CPU LOOP instruction. MASM reads the condition and emits CMP
 * or TEST plus a conditional jump around the body. The condition can be anything, and RCX is
 * not an implicit counter. LOOP decrements RCX and branches if it is nonzero, and compilers
 * avoid it (see the LOOP entry). `.endw` closes the block. The assembly-time WHILE later in
 * this file is a third thing: it repeats source text while an assemble-time expression is
 * true and emits no loop at all.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .while ecx != 0
 */
static const char* const ASM_WHILE = ".while";

/*
 * ENDW - .ENDW High-Level Directive
 * Terminates a MASM high-level .WHILE block.
 *
 * Syntax: .ENDW
 * Common usage: Closes a structured .WHILE loop.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .endw
 */
static const char* const ASM_ENDW = ".endw";

/*
 * REPEAT - .REPEAT High-Level Directive
 * Starts a MASM high-level repeat-until loop body.
 *
 * Syntax: .REPEAT
 * Common usage: Pairs with .UNTIL to test the condition at the end of the loop.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .repeat
 */
static const char* const ASM_REPEAT = ".repeat";

/*
 * UNTIL - .UNTIL High-Level Directive
 * Ends a .REPEAT block and provides the terminating run-time condition.
 *
 * Syntax: .UNTIL condition
 * Common usage: Used for do-while style loops.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .until eax == 0
 */
static const char* const ASM_UNTIL = ".until";

/*
 * BREAK - .BREAK High-Level Directive
 * Exits the nearest MASM high-level loop immediately.
 *
 * Syntax: .BREAK [ .IF condition ]
 * Common usage: Used inside .WHILE or .REPEAT blocks for structured early exits.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .break .if ecx == 0
 */
static const char* const ASM_BREAK = ".break";

/*
 * CONTINUE - .CONTINUE High-Level Directive
 * Skips to the next iteration of the nearest MASM high-level loop.
 *
 * Syntax: .CONTINUE [ .IF condition ]
 * Common usage: Used inside .WHILE or .REPEAT blocks for structured loop control.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .continue .if eax < 0
 */
static const char* const ASM_CONTINUE = ".continue";

/*
 * 386 - .386 Directive
 * Enables 80386 instruction set syntax and operand forms.
 *
 * Syntax: .386
 * Common usage: Placed near the top of the file to declare the minimum required CPU.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .386
 */
static const char* const ASM_386 = ".386";

/*
 * 486 - .486 Directive
 * Enables 80486 instruction set syntax and scheduling assumptions.
 *
 * Syntax: .486
 * Common usage: Declares a 486-or-later target.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .486
 */
static const char* const ASM_486 = ".486";

/*
 * 586 - .586 Directive
 * Enables Pentium-era instructions.
 *
 * Syntax: .586
 * Common usage: Declares a 586-or-later target.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .586
 */
static const char* const ASM_586 = ".586";

/*
 * 686 - .686 Directive
 * Enables Pentium Pro and later baseline instructions.
 *
 * Syntax: .686
 * Common usage: Often used for 32-bit flat-model code that expects conditional moves and other
 * newer instructions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .686
 */
static const char* const ASM_686 = ".686";

/*
 * MMX - .MMX Directive
 * Enables MMX instruction syntax.
 *
 * Syntax: .MMX
 * Common usage: Placed before MMX instruction use if the selected CPU level does not already
 * imply support.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .mmx
 */
static const char* const ASM_MMX = ".mmx";

/*
 * XMM - .XMM Directive
 * Enables SSE/XMM instruction syntax.
 *
 * Syntax: .XMM
 * Common usage: Required by some MASM configurations before using XMM registers and SSE
 * mnemonics.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .xmm
 */
static const char* const ASM_XMM = ".xmm";

/*
 * OPTION - OPTION Directive
 * Sets assembler options such as case mapping, proc visibility rules, language defaults, and
 * expression behavior.
 *
 * Syntax: OPTION option-list
 * Common usage: Used to control MASM parsing and code-generation policy for the current module.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   option casemap:none
 */
static const char* const ASM_OPTION = "option";

/*
 * INCLUDE - INCLUDE Directive
 * Textually includes another source file at assembly time.
 *
 * Syntax: INCLUDE filename
 * Common usage: Used for shared macro files, prototypes, and constants.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   include windows.inc
 */
static const char* const ASM_INCLUDE = "include";

/*
 * INCLUDELIB - INCLUDELIB Directive
 * Adds a library name to the linker input list from within source code.
 *
 * Syntax: INCLUDELIB libraryname
 * Common usage: Convenient for samples that want self-contained linker dependencies.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   includelib kernel32.lib
 */
static const char* const ASM_INCLUDELIB = "includelib";

/*
 * EXTERN - EXTERN Directive
 * Declares an external symbol defined in another module and gives MASM its type information.
 *
 * Syntax: EXTERN symbol:type
 * Common usage: Used for imported procedures, variables, and linker-resolved cross-module
 * references.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   extern printf:proc
 */
static const char* const ASM_EXTERN = "extern";

/*
 * EXTERNDEF - EXTERNDEF Directive
 * Declares a symbol that may be defined locally or resolved externally.
 *
 * Syntax: EXTERNDEF symbol:type
 * Common usage: Useful in headers shared by defining and consuming modules.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   externdef gValue:dword
 */
static const char* const ASM_EXTERNDEF = "externdef";

/*
 * PUBLIC - PUBLIC Directive
 * Exports a symbol from the current module so the linker can resolve it from other modules.
 *
 * Syntax: PUBLIC symbol[, symbol]...
 * Common usage: Applied to procedures, labels, and data that should have external linkage.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   public MyProc
 */
static const char* const ASM_PUBLIC = "public";

/*
 * PROTO - PROTO Directive
 * Declares a procedure prototype with parameter types and calling convention metadata.
 *
 * Syntax: name PROTO [params]
 * Common usage: Used with INVOKE and for documentation of external/internal procedures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MessageBoxA PROTO :QWORD, :QWORD, :QWORD, :DWORD
 */
static const char* const ASM_PROTO = "proto";

/*
 * INVOKE - INVOKE Directive
 * Calls a procedure using its prototype so MASM can marshal arguments according to the procedure
 * declaration.
 *
 * Syntax: INVOKE procname, arg1, arg2, ...
 * Common usage: MASM expands INVOKE into the argument moves or pushes and the CALL, using the
 * PROC or PROTO language type. `invoke ExitProcess, 0` does not invent an API; it calls the
 * imported ExitProcess symbol. This generator does not emit INVOKE. It writes the moves so
 * they stay visible. See the calling-convention banner later in this section.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   invoke ExitProcess, 0
 */
static const char* const ASM_INVOKE = "invoke";

/*
 * PROC - PROC Directive
 * Begins a procedure definition and associates calling convention, visibility, parameter, and
 * local-variable metadata with a label.
 *
 * Syntax: name PROC [attributes] [USES reglist] [, params]
 * Common usage: A language type on the PROC (STDCALL, C, SYSCALL, and the others) is how you
 * let MASM generate the prologue and epilogue instead of writing the pushes yourself. USES
 * adds the registers to save. This generator does not use that. It emits the frame itself so
 * the moves stay visible and checkable. The language type does not change the CPU. See the
 * calling-convention banner later in this section.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyProc PROC USES ebx esi, value:DWORD
 */
static const char* const ASM_PROC = "proc";

/*
 * ENDP - ENDP Directive
 * Terminates a procedure definition started with PROC.
 *
 * Syntax: name ENDP
 * Common usage: Used to close the lexical scope of a MASM procedure.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyProc ENDP
 */
static const char* const ASM_ENDP = "endp";

/*
 * ----------------------------------------------------------------------------
 * MASM macro directives (not CPU instructions)
 * ----------------------------------------------------------------------------
 * MACRO / ENDM / LOCAL and the text operators below (TEXTEQU, CATSTR, SUBSTR,
 * SIZESTR, INSTR, and the later REPEAT / FOR / WHILE / EXITM forms) are
 * assemble-time text substitution. They are not the CPU LOOP instruction and
 * they emit nothing until the body itself contains real instructions. The
 * constants stay where they are; this banner only marks the group.
 *
 * Syntax:
 *   name MACRO arg1, arg2
 *     ;; &arg1 pastes the argument into a larger token
 *     mov &arg1, arg2
 *   ENDM
 * Angle brackets keep a comma-list as one argument: `name MACRO <1, 2, 3>`.
 * LOCAL label inside the macro makes a fresh label on every expansion.
 */
/*
 * MACRO - MACRO Directive
 * Begins a macro definition with named parameters and optional defaults.
 *
 * Syntax: name MACRO [params] ... ENDM
 * Common usage: Textual substitution at assemble time. `&param` pastes a parameter into the
 * surrounding token (`&reg&` becomes whatever register the caller wrote). `<text>` quotes one
 * argument that contains commas or spaces. LOCAL declares a name that is unique per expansion
 * so two calls do not share a label. ENDM closes the body. This is not a CPU instruction and
 * it is unrelated to LOOP, which decrements RCX at run time. A macro runs, in the sense of
 * copying text, when it is invoked by name; the copied instructions then run later.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyMacro MACRO arg
 */
static const char* const ASM_MACRO = "macro";

/*
 * ENDM - ENDM Directive
 * Terminates a macro definition started with MACRO.
 *
 * Syntax: ENDM
 * Common usage: Closes the macro body.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ENDM
 */
static const char* const ASM_ENDM = "endm";

/*
 * LOCAL - LOCAL Directive
 * Declares local macro variables inside a MACRO or stack-based local variables inside a PROC
 * depending on context.
 *
 * Syntax: LOCAL name[:type] [, name[:type]]...
 * Common usage: In procedures it reserves stack storage; in macros it creates unique text
 * symbols.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   LOCAL buffer[64]:BYTE
 */
static const char* const ASM_LOCAL = "local";

/*
 * EQU - EQU Directive
 * Assigns a constant value or text replacement to a symbol.
 *
 * Syntax: name EQU expression
 * Common usage: Creates symbolic constants used throughout a module.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   BUF_SIZE EQU 256
 */
static const char* const ASM_EQU = "equ";

/*
 * TEXTEQU - TEXTEQU Directive
 * Assigns a literal text substitution to a symbol.
 *
 * Syntax: name TEXTEQU <text>
 * Common usage: Useful in macros and token-level metaprogramming.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   CRLF TEXTEQU <13,10>
 */
static const char* const ASM_TEXTEQU = "textequ";

/*
 * CATSTR - CATSTR Operator
 * Concatenates text strings during assembly-time macro processing.
 *
 * Syntax: symbol CATSTR text1, text2, ...
 * Common usage: Used to build identifiers or string literals in macros.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   name CATSTR <pre>, <fix>
 */
static const char* const ASM_CATSTR = "catstr";

/*
 * SUBSTR - SUBSTR Operator
 * Extracts a substring from a text macro string during assembly-time processing.
 *
 * Syntax: symbol SUBSTR text, position [, length]
 * Common usage: Used in advanced macro metaprogramming.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   piece SUBSTR <abcdef>, 2, 3
 */
static const char* const ASM_SUBSTR = "substr";

/*
 * SIZESTR - SIZESTR Operator
 * Returns the length of a text string at assembly time.
 *
 * Syntax: SIZESTR text
 * Common usage: Useful for macro-time validation and code generation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   IF SIZESTR(<abc>) EQ 3
 */
static const char* const ASM_SIZESTR = "sizestr";

/*
 * INSTR - INSTR Operator
 * Searches for one string inside another during assembly-time text processing.
 *
 * Syntax: INSTR [start,] text, substr
 * Common usage: Used in macros for token inspection and conditional expansion.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   pos = INSTR(1, <abcdef>, <cd>)
 */
static const char* const ASM_INSTR = "instr";

/*
 * SEGMENT - SEGMENT Directive
 * Begins an explicit segment definition.
 *
 * Syntax: name SEGMENT [attributes]
 * Common usage: Used instead of simplified .CODE/.DATA style in more manual MASM source layouts.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   _TEXT SEGMENT PARA PUBLIC 'CODE'
 */
static const char* const ASM_SEGMENT = "segment";

/*
 * ENDS - ENDS Directive
 * Terminates a segment definition started with SEGMENT.
 *
 * Syntax: name ENDS
 * Common usage: Closes an explicit segment block.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   _TEXT ENDS
 */
static const char* const ASM_ENDS = "ends";

/*
 * ASSUME - ASSUME Directive
 * Informs MASM which segment registers or type assumptions apply to given symbols and registers.
 *
 * Syntax: ASSUME register:segmentOrType [, ...]
 * Common usage: Emits no instruction. In 16-bit code, after `mov ax, @data` / `mov ds, ax`,
 * `assume ds:@data` tells MASM that a bare `[bx]` lives in that segment so it can check the
 * reference. Flat 32-bit and 64-bit user code does not juggle DS that way; ASSUME is a
 * segmented leftover there. The modern reason to write it is `assume gs:nothing` (or fs) so
 * MASM accepts an explicit `gs:` override without a segment association, which is how Windows
 * x64 reads the TEB. `assume cs:_TEXT, ds:_DATA` is the 16-bit form.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   assume cs:_TEXT, ds:_DATA
 */
static const char* const ASM_ASSUME = "assume";

/*
 * ORG - ORG Directive
 * Sets the current location counter within the current segment or structure.
 *
 * Syntax: ORG expression
 * Common usage: 64-bit user code does not use ORG to place functions; the linker assigns
 * addresses. ORG is for a file whose bytes must sit at a fixed address: a BIOS boot sector
 * (`org 7C00h`), a DOS .COM file (`org 100h`), or an absolute structure template. It moves the
 * assembler's counter and can emit padding. It is not a CPU instruction. The realistic modern
 * use is a bootloader or firmware image, not a Windows x64 procedure.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   org 7C00h
 */
static const char* const ASM_ORG = "org";

/*
 * ALIGN - ALIGN Directive
 * Advances the location counter to the requested power-of-two boundary, emitting fill bytes as
 * needed.
 *
 * Syntax: ALIGN n
 * Common usage: Still used. In a code section MASM pads with NOPs so the next instruction is
 * on the boundary; in data it pads with zeros. You want it before a MOVAPS or VMOVAPS source
 * that requires alignment, before a hot loop, or between two cache-line-sized locks so they
 * do not share a line. It is an assembler directive, not a CPU instruction. `align 16` below
 * is why the following load is legal for MOVAPS.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   align 16
 *   movaps xmm0, xmmword ptr [rsi]
 */
static const char* const ASM_ALIGN = "align";

/*
 * EVEN - EVEN Directive
 * Aligns the location counter to an even address boundary.
 *
 * Syntax: EVEN
 * Common usage: Historical shorthand for ALIGN 2.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   even
 */
static const char* const ASM_EVEN = "even";

/*
 * END - END Directive
 * Marks the logical end of the source module and may specify the entry label.
 *
 * Syntax: END [entryLabel]
 * Common usage: Required to terminate a MASM source file cleanly.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   end main
 */
static const char* const ASM_END = "end";

/*
 * TITLE - TITLE Directive
 * Supplies a title string for listings and diagnostics.
 *
 * Syntax: TITLE text
 * Common usage: Affects assembler output metadata rather than machine code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   title Sample Program
 */
static const char* const ASM_TITLE = "title";

/*
 * NAME_DIR - NAME Directive
 * Supplies a module name for object-module metadata in older toolchains.
 *
 * Syntax: NAME modulename
 * Common usage: Mostly historical but still recognized by MASM.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   name mymodule
 */
static const char* const ASM_NAME_DIR = "name";

/*
 * PAGE - PAGE Directive
 * Controls page width and length in generated listing files.
 *
 * Syntax: PAGE [length][, width]
 * Common usage: Affects listings only, not emitted machine code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   page 60, 132
 */
static const char* const ASM_PAGE = "page";

/*
 * COMMENT - COMMENT Directive
 * Begins a block comment delimited by a user-chosen character or token.
 *
 * Syntax: COMMENT delimiter text delimiter
 * Common usage: Useful for large disabled blocks or explanatory text in legacy MASM syntax.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   comment ! this is a block comment !
 */
static const char* const ASM_COMMENT = "comment";

/*
 * ----------------------------------------------------------------------------
 * Assembler output and error directives (not CPU instructions)
 * ----------------------------------------------------------------------------
 * ECHO and the .ERR* forms below print a message or force an assemble-time
 * error. They do not emit machine code. They are how a macro checks its
 * arguments. This banner marks that group; the constants are not reordered.
 * Conditional assembly (IF / ELSE / ENDIF) is the next group after the .ERR*
 * spellings that compare text.
 */
/*
 * ECHO_DIR - ECHO Directive
 * Emits a message to the assembler output during assembly.
 *
 * Syntax: ECHO text
 * Common usage: Useful for macro debugging and conditional-build diagnostics.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   echo Building debug configuration
 */
static const char* const ASM_ECHO_DIR = "echo";

/*
 * ERR - .ERR Directive
 * Forces the assembler to emit an error unconditionally.
 *
 * Syntax: .ERR [text]
 * Common usage: Used in macro-time assertions and unsupported-configuration checks.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .err <unsupported configuration>
 */
static const char* const ASM_ERR = ".err";

/*
 * ERRE - .ERRE Directive
 * Emits an error if an expression is true or nonzero.
 *
 * Syntax: .ERRE expression
 * Common usage: Used for compile-time assertions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .erre SIZEOF table EQ 0
 */
static const char* const ASM_ERRE = ".erre";

/*
 * ERRNZ - .ERRNZ Directive
 * Emits an error if an expression is not zero.
 *
 * Syntax: .ERRNZ expression
 * Common usage: Common for structure-size and offset validation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errnz ($ - start) - 16
 */
static const char* const ASM_ERRNZ = ".errnz";

/*
 * ERRNDEF - .ERRNDEF Directive
 * Emits an error if the named symbol is not defined.
 *
 * Syntax: .ERRNDEF symbol
 * Common usage: Used for include guards and dependency checks in assembly-time logic.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errndef MyRequiredSymbol
 */
static const char* const ASM_ERRNDEF = ".errndef";

/*
 * ERRDEF - .ERRDEF Directive
 * Emits an error if the named symbol is defined.
 *
 * Syntax: .ERRDEF symbol
 * Common usage: Useful for collision detection and single-definition enforcement.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errdef OldMacro
 */
static const char* const ASM_ERRDEF = ".errdef";

/*
 * ERRB - .ERRB Directive
 * Emits an error if a text argument is blank.
 *
 * Syntax: .ERRB <text>
 * Common usage: Used in macro parameter validation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errb <&arg>
 */
static const char* const ASM_ERRB = ".errb";

/*
 * ERRNB - .ERRNB Directive
 * Emits an error if a text argument is not blank.
 *
 * Syntax: .ERRNB <text>
 * Common usage: Used in mutually exclusive macro forms.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errnb <&arg>
 */
static const char* const ASM_ERRNB = ".errnb";

/*
 * ERRDIF - .ERRDIF Directive
 * Emits an error if two text arguments differ.
 *
 * Syntax: .ERRDIF <text1>, <text2>
 * Common usage: Used in macro-time string validation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errdif <eax>, <&reg>
 */
static const char* const ASM_ERRDIF = ".errdif";

/*
 * ERRIDN - .ERRIDN Directive
 * Emits an error if two text arguments are identical.
 *
 * Syntax: .ERRIDN <text1>, <text2>
 * Common usage: Used to reject forbidden macro arguments.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .erridn <esp>, <&reg>
 */
static const char* const ASM_ERRIDN = ".erridn";

/*
 * ----------------------------------------------------------------------------
 * Conditional assembly (not CPU instructions, and not .IF)
 * ----------------------------------------------------------------------------
 * IF, IFE, IFDEF, IFNDEF, IFB, IFNB, IFDIF, IFIDN, and the ELSE / ELSEIF /
 * ENDIF that follow are evaluated while assembling. The taken branch's text
 * is assembled; the other branch is discarded. No compare instruction is
 * emitted. This is different from .IF / .ELSE / .ENDIF earlier in the file,
 * which MASM lowers into run-time CMP and Jcc. The constants stay in place;
 * this banner only separates the group from the CPU-instruction catalogue.
 */
/*
 * IF_DIR - IF Directive
 * Begins an assembly-time conditional block evaluated by the assembler rather than by emitted
 * runtime code.
 *
 * Syntax: IF expression
 * Common usage: Pairs with ELSE/ELSEIF/ENDIF for conditional assembly.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   if DEBUG
 */
static const char* const ASM_IF_DIR = "if";

/*
 * IFE - IFE Directive
 * Begins an assembly-time conditional block taken when the expression equals zero.
 *
 * Syntax: IFE expression
 * Common usage: Used for inverse conditional assembly tests.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ife DEBUG
 */
static const char* const ASM_IFE = "ife";

/*
 * IFDEF - IFDEF Directive
 * Begins an assembly-time block if the named symbol is defined.
 *
 * Syntax: IFDEF symbol
 * Common usage: Common in include guards and feature toggles.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifdef UNICODE
 */
static const char* const ASM_IFDEF = "ifdef";

/*
 * IFNDEF - IFNDEF Directive
 * Begins an assembly-time block if the named symbol is not defined.
 *
 * Syntax: IFNDEF symbol
 * Common usage: Common in include guards and default-definition logic.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifndef MY_HEADER_GUARD
 */
static const char* const ASM_IFNDEF = "ifndef";

/*
 * IFB - IFB Directive
 * Begins an assembly-time block if a text argument is blank.
 *
 * Syntax: IFB <text>
 * Common usage: Used in macro parameter processing.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifb <&arg>
 */
static const char* const ASM_IFB = "ifb";

/*
 * IFNB - IFNB Directive
 * Begins an assembly-time block if a text argument is not blank.
 *
 * Syntax: IFNB <text>
 * Common usage: Used in macro parameter processing.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifnb <&arg>
 */
static const char* const ASM_IFNB = "ifnb";

/*
 * IFDIF - IFDIF Directive
 * Begins an assembly-time block if two text arguments differ.
 *
 * Syntax: IFDIF <text1>, <text2>
 * Common usage: Useful in macro-time string comparisons.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifdif <eax>, <&reg>
 */
static const char* const ASM_IFDIF = "ifdif";

/*
 * IFIDN - IFIDN Directive
 * Begins an assembly-time block if two text arguments are identical.
 *
 * Syntax: IFIDN <text1>, <text2>
 * Common usage: Useful in macro-time string comparisons and overload-like dispatch.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifidn <eax>, <&reg>
 */
static const char* const ASM_IFIDN = "ifidn";

/*
 * ELSE_DIR - ELSE Directive
 * Assembly-time alternative branch used with IF/IFE/IFDEF and related directives.
 *
 * Syntax: ELSE
 * Common usage: Used in conditional assembly, distinct from the run-time .ELSE directive.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   else
 */
static const char* const ASM_ELSE_DIR = "else";

/*
 * ELSEIF_DIR - ELSEIF Directive
 * Assembly-time else-if branch used with IF and related directives.
 *
 * Syntax: ELSEIF expression
 * Common usage: Lets conditional assembly choose among multiple cases.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   elseif RELEASE
 */
static const char* const ASM_ELSEIF_DIR = "elseif";

/*
 * ENDIF_DIR - ENDIF Directive
 * Terminates an assembly-time conditional block.
 *
 * Syntax: ENDIF
 * Common usage: Closes IF/IFE/IFDEF and related directives.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   endif
 */
static const char* const ASM_ENDIF_DIR = "endif";

/*
 * REPEAT_DIR - REPEAT Directive
 * Repeats a block of source text a fixed number of times at assembly time.
 *
 * Syntax: REPEAT count
 * Common usage: Pairs with ENDM to generate repeated code or data definitions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   repeat 4
 */
static const char* const ASM_REPEAT_DIR = "repeat";

/*
 * FOR - FOR Directive
 * Iterates over a comma-separated argument list during macro expansion.
 *
 * Syntax: FOR param, <list>
 * Common usage: Useful for macro-generated boilerplate and table definitions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   for reg, <eax, ebx, ecx>
 */
static const char* const ASM_FOR = "for";

/*
 * FORC - FORC Directive
 * Iterates over the characters of a text string during macro expansion.
 *
 * Syntax: FORC ch, <text>
 * Common usage: Useful for text processing macros.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   forc ch, <ABC>
 */
static const char* const ASM_FORC = "forc";

/*
 * WHILE_DIR - WHILE Directive
 * Repeats a block of source text while an assembly-time expression remains true.
 *
 * Syntax: WHILE expression
 * Common usage: Pairs with ENDM for macro-time loops.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   while index LT 10
 */
static const char* const ASM_WHILE_DIR = "while";

/*
 * EXITM - EXITM Directive
 * Terminates macro expansion immediately and optionally returns text.
 *
 * Syntax: EXITM [<text>]
 * Common usage: Used for macro-time control flow and early returns.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   exitm <eax>
 */
static const char* const ASM_EXITM = "exitm";

/*
 * GOTO - GOTO Directive
 * Transfers assembly-time control to a macro label within the current macro body.
 *
 * Syntax: GOTO label
 * Common usage: Used sparingly in complex macros that need nonstructured expansion flow.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   goto retry
 */
static const char* const ASM_GOTO = "goto";

/*
 * STRUCT - STRUCT Directive
 * Begins a structure type definition with named fields and offsets.
 *
 * Syntax: name STRUCT [alignment]
 * Common usage: Pairs with ENDS or an implicit terminator depending on style to define aggregate
 * layouts.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   POINT STRUCT
 */
static const char* const ASM_STRUCT = "struct";

/*
 * UNION - UNION Directive
 * Begins a union type definition whose members overlap at offset zero.
 *
 * Syntax: name UNION
 * Common usage: Used for type punning or alternate views of the same storage.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   VALUE UNION
 */
static const char* const ASM_UNION = "union";

/*
 * TYPEDEF - TYPEDEF Directive
 * Creates a type alias for another MASM type expression.
 *
 * Syntax: name TYPEDEF type
 * Common usage: Used to simplify prototypes and data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   PBYTE TYPEDEF PTR BYTE
 */
static const char* const ASM_TYPEDEF = "typedef";

/*
 * RECORD - RECORD Directive
 * Defines a packed bitfield record type with named fields and widths.
 *
 * Syntax: name RECORD field:width [, field:width]...
 * Common usage: Useful for symbolic masks and packed control values.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   FLAGS RECORD carry:1, zero:1, sign:1
 */
static const char* const ASM_RECORD = "record";

/*
 * LABEL - LABEL Directive
 * Associates a type with the current location counter without allocating storage.
 *
 * Syntax: name LABEL type
 * Common usage: Useful for giving multiple typed views to the same address.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value LABEL DWORD
 */
static const char* const ASM_LABEL = "label";

/*
 * DB - Define Byte
 * Allocates and optionally initializes one or more byte-sized objects.
 *
 * Syntax: name DB initializer[, initializer]...
 * Common usage: Used for strings, raw bytes, and byte arrays.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   msg db 'Hi',0
 */
static const char* const ASM_DB = "db";

/*
 * DW - Define Word
 * Allocates and optionally initializes 16-bit objects.
 *
 * Syntax: name DW initializer[, initializer]...
 * Common usage: Used for 16-bit integers and pointer components.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value dw 1234h
 */
static const char* const ASM_DW = "dw";

/*
 * DD - Define Doubleword
 * Allocates and optionally initializes 32-bit objects.
 *
 * Syntax: name DD initializer[, initializer]...
 * Common usage: Used for 32-bit integers, addresses, and single-precision floats.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value dd 12345678h
 */
static const char* const ASM_DD = "dd";

/*
 * DF - Define Far Pointer / Fword
 * Allocates and optionally initializes a 48-bit far pointer or six-byte object depending on
 * context.
 *
 * Syntax: name DF initializer[, initializer]...
 * Common usage: Used primarily in segmented or descriptor-related code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   fp df 0
 */
static const char* const ASM_DF = "df";

/*
 * DQ - Define Quadword
 * Allocates and optionally initializes 64-bit objects.
 *
 * Syntax: name DQ initializer[, initializer]...
 * Common usage: Used for 64-bit integers, pointers, and double-precision constants.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value dq 123456789ABCDEF0h
 */
static const char* const ASM_DQ = "dq";

/*
 * DT - Define Ten Bytes
 * Allocates and optionally initializes 10-byte objects.
 *
 * Syntax: name DT initializer[, initializer]...
 * Common usage: Often used for x87 80-bit extended-precision constants.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value dt 1.0
 */
static const char* const ASM_DT = "dt";

/*
 * BYTE_KW - BYTE Keyword
 * Type keyword naming an unsigned 8-bit object or memory operand size.
 *
 * Syntax: BYTE PTR operand | name BYTE initializer
 * Common usage: Used in declarations and explicit operand-size overrides.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov al, BYTE PTR [rax]
 */
static const char* const ASM_BYTE_KW = "byte";

/*
 * SBYTE - SBYTE Keyword
 * Type keyword naming a signed 8-bit object.
 *
 * Syntax: name SBYTE initializer
 * Common usage: Used in typed data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value sbyte -1
 */
static const char* const ASM_SBYTE = "sbyte";

/*
 * WORD_KW - WORD Keyword
 * Type keyword naming a 16-bit object or operand size.
 *
 * Syntax: WORD PTR operand | name WORD initializer
 * Common usage: Used in declarations and explicit operand-size overrides.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ax, WORD PTR [rax]
 */
static const char* const ASM_WORD_KW = "word";

/*
 * SWORD - SWORD Keyword
 * Type keyword naming a signed 16-bit object.
 *
 * Syntax: name SWORD initializer
 * Common usage: Used in typed data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value sword -2
 */
static const char* const ASM_SWORD = "sword";

/*
 * DWORD_KW - DWORD Keyword
 * Type keyword naming a 32-bit object or operand size.
 *
 * Syntax: DWORD PTR operand | name DWORD initializer
 * Common usage: Used in declarations and explicit operand-size overrides.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov eax, DWORD PTR [rax]
 */
static const char* const ASM_DWORD_KW = "dword";

/*
 * SDWORD - SDWORD Keyword
 * Type keyword naming a signed 32-bit object.
 *
 * Syntax: name SDWORD initializer
 * Common usage: Used in typed data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value sdword -3
 */
static const char* const ASM_SDWORD = "sdword";

/*
 * FWORD_KW - FWORD Keyword
 * Type keyword naming a 48-bit object, often a far pointer or descriptor fragment.
 *
 * Syntax: FWORD PTR operand
 * Common usage: Used mainly in segmented and system-level code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   les bx, FWORD PTR ptr48
 */
static const char* const ASM_FWORD_KW = "fword";

/*
 * QWORD_KW - QWORD Keyword
 * Type keyword naming a 64-bit object or operand size.
 *
 * Syntax: QWORD PTR operand | name QWORD initializer
 * Common usage: Used for 64-bit integers, pointers, and explicit operand sizing.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov rax, QWORD PTR [rsp]
 */
static const char* const ASM_QWORD_KW = "qword";

/*
 * SQWORD - SQWORD Keyword
 * Type keyword naming a signed 64-bit object.
 *
 * Syntax: name SQWORD initializer
 * Common usage: Used in typed data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value sqword -4
 */
static const char* const ASM_SQWORD = "sqword";

/*
 * TBYTE_KW - TBYTE Keyword
 * Type keyword naming a 10-byte object, usually x87 extended precision.
 *
 * Syntax: TBYTE PTR operand | name TBYTE initializer
 * Common usage: Used for 80-bit floating-point storage.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   fld TBYTE PTR [value]
 */
static const char* const ASM_TBYTE_KW = "tbyte";

/*
 * REAL4 - REAL4 Keyword
 * Type keyword naming a 32-bit IEEE single-precision floating-point object.
 *
 * Syntax: name REAL4 initializer
 * Common usage: Used for scalar floating data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value real4 1.0
 */
static const char* const ASM_REAL4 = "real4";

/*
 * REAL8 - REAL8 Keyword
 * Type keyword naming a 64-bit IEEE double-precision floating-point object.
 *
 * Syntax: name REAL8 initializer
 * Common usage: Used for scalar floating data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value real8 1.0
 */
static const char* const ASM_REAL8 = "real8";

/*
 * REAL10 - REAL10 Keyword
 * Type keyword naming an 80-bit extended-precision floating-point object.
 *
 * Syntax: name REAL10 initializer
 * Common usage: Used for x87 extended values.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value real10 1.0
 */
static const char* const ASM_REAL10 = "real10";

/*
 * XMMWORD_KW - XMMWORD Keyword
 * Type keyword naming a 128-bit memory operand or object size.
 *
 * Syntax: XMMWORD PTR operand
 * Common usage: Used with SSE loads, stores, and vector declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   movdqa xmm0, XMMWORD PTR [rax]
 */
static const char* const ASM_XMMWORD_KW = "xmmword";

/*
 * YMMWORD_KW - YMMWORD Keyword
 * Type keyword naming a 256-bit memory operand or object size.
 *
 * Syntax: YMMWORD PTR operand
 * Common usage: Used with AVX loads, stores, and vector declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   vmovaps ymm0, YMMWORD PTR [rax]
 */
static const char* const ASM_YMMWORD_KW = "ymmword";

/*
 * NEAR - NEAR Keyword
 * Type or distance keyword specifying a near code or pointer reference within the current code
 * segment.
 *
 * Syntax: NEAR PTR symbol
 * Common usage: Used in prototypes, labels, and explicit distance overrides.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   call NEAR PTR proc
 */
static const char* const ASM_NEAR = "near";

/*
 * FAR - FAR Keyword
 * Type or distance keyword specifying a far code or pointer reference that includes a segment
 * component.
 *
 * Syntax: FAR PTR symbol
 * Common usage: A far pointer is an offset plus a CS (or other segment) value. 64-bit user
 * code does not use it. Windows x64 calls are near: CALL pushes only RIP, and RET pops only
 * RIP. FAR is a 16-bit segmented leftover. You still see it in real-mode boot code, a BIOS
 * service, or a kernel interrupt path that must reload CS. `jmp far ptr target` is the
 * realistic form of that leftover, not something a user-mode x64 function writes. See RETF.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   jmp FAR PTR target
 */
static const char* const ASM_FAR = "far";

/*
 * PTR - PTR Operator
 * Overrides or supplies an explicit type for a memory operand or symbol reference.
 *
 * Syntax: type PTR expression
 * Common usage: Commonly used when MASM cannot infer operand size from context.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov eax, DWORD PTR [rax]
 */
static const char* const ASM_PTR = "ptr";

/*
 * OFFSET - OFFSET Operator
 * Computes the assembly-time offset/address of a symbol within its segment or flat address
 * space.
 *
 * Syntax: OFFSET symbol
 * Common usage: Used to load addresses into registers or pass pointers to procedures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov rax, OFFSET msg
 */
static const char* const ASM_OFFSET = "offset";

/*
 * ADDR - ADDR Operator
 * Passes the address of an argument to INVOKE or high-level MASM procedure machinery.
 *
 * Syntax: ADDR symbol
 * Common usage: Common with INVOKE and typed prototypes.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   invoke SomeProc, ADDR buffer
 */
static const char* const ASM_ADDR = "addr";

/*
 * SEG - SEG Operator
 * Computes the segment part of a symbol address at assembly time.
 *
 * Syntax: SEG symbol
 * Common usage: Relevant primarily in segmented-memory programming.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ax, SEG variable
 */
static const char* const ASM_SEG = "seg";

/*
 * TYPE_OP - TYPE Operator
 * Returns the size or type value associated with a symbol or type expression.
 *
 * Syntax: TYPE symbol
 * Common usage: Used in macros and generic code to reason about data sizes.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, TYPE DWORD
 */
static const char* const ASM_TYPE_OP = "type";

/*
 * SIZE_OP - SIZE Operator
 * Returns the size in bytes of a single data item for a symbol or type.
 *
 * Syntax: SIZE symbol
 * Common usage: Used in generic macros and declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   IF SIZE buffer EQ 16
 */
static const char* const ASM_SIZE_OP = "size";

/*
 * SIZEOF_OP - SIZEOF Operator
 * Returns the total size in bytes of a variable, array, or type instance.
 *
 * Syntax: SIZEOF symbol
 * Common usage: Useful for buffer lengths and structure copies.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, SIZEOF buffer
 */
static const char* const ASM_SIZEOF_OP = "sizeof";

/*
 * LENGTH_OP - LENGTH Operator
 * Returns the number of initializers in the first dimension of a data item in older MASM
 * semantics.
 *
 * Syntax: LENGTH symbol
 * Common usage: Mostly superseded by LENGTHOF but still recognized.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, LENGTH table
 */
static const char* const ASM_LENGTH_OP = "length";

/*
 * LENGTHOF_OP - LENGTHOF Operator
 * Returns the total element count of a variable or array.
 *
 * Syntax: LENGTHOF symbol
 * Common usage: Common for loop counts over static arrays.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, LENGTHOF table
 */
static const char* const ASM_LENGTHOF_OP = "lengthof";

/*
 * WIDTH_OP - WIDTH Operator
 * Returns the width in bits of a RECORD field or type.
 *
 * Syntax: WIDTH fieldOrType
 * Common usage: Used with RECORD definitions and masks.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, WIDTH carry
 */
static const char* const ASM_WIDTH_OP = "width";

/*
 * MASK_OP - MASK Operator
 * Returns a bit mask for a RECORD field.
 *
 * Syntax: MASK field
 * Common usage: Used to extract or set packed record fields symbolically.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov eax, MASK carry
 */
static const char* const ASM_MASK_OP = "mask";

/*
 * THIS - THIS Operator
 * Associates the current location with a specific type without allocating additional storage.
 *
 * Syntax: THIS type
 * Common usage: Used to create typed aliases at the current location counter.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   label THIS DWORD
 */
static const char* const ASM_THIS = "this";

/*
 * DUP_KW - DUP Operator
 * Repeats an initializer or nested initializer pattern in data declarations.
 *
 * Syntax: count DUP(initializer)
 * Common usage: Used to reserve or initialize arrays succinctly.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   buffer db 64 DUP(0)
 */
static const char* const ASM_DUP_KW = "dup";

/*
 * FLAT - FLAT Keyword
 * Names the flat address space or memory model used by modern 32-bit MASM programs.
 *
 * Syntax: FLAT
 * Common usage: Appears in .MODEL, ASSUME, and type expressions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .model flat, stdcall
 */
static const char* const ASM_FLAT = "flat";

/*
 * ----------------------------------------------------------------------------
 * Calling-convention annotations (not CPU instructions)
 * ----------------------------------------------------------------------------
 * STDCALL, C, SYSCALL, PASCAL, BASIC, FORTRAN, FASTCALL, USES, and VARARG
 * are collected here so they are read as one group. They are language types
 * and PROC attributes. The assembler consumes them. The CPU never sees them.
 *
 * A language type on a PROC or a PROTO is how you let MASM implement the
 * call setup. It tells the assembler three concrete things:
 *   - where each argument goes (stack slots, or registers for FASTCALL),
 *   - who removes those slots (the callee's `ret n` for STDCALL, the caller
 *     after the CALL for C),
 *   - how the symbol is decorated (`_name@N` for a 32-bit STDCALL whose
 *     arguments occupy N bytes).
 * INVOKE reads the PROTO and expands into the pushes or the register moves
 * and the CALL. USES adds the named callee-saved registers to the prologue
 * and epilogue MASM generates for the PROC. VARARG marks a trailing list
 * whose count is not in the prototype, so a printf-style PROTO still lets
 * INVOKE push the extra arguments and the callee learns the count another
 * way (a format string, a sentinel).
 *
 * Writing the moves yourself is the other choice. You name the registers,
 * you reserve the home space, you write the CALL, and you ignore the
 * language type. The CPU result can be the same instruction stream. The
 * difference is who wrote it down. This generator writes it down.
 * write_extern_call and write_call emit the moves, because those moves are
 * what the type dictionaries check. A language type on a PROC would hide
 * them inside the assembler.
 *
 * A SYSCALL language type on a PROTO is a MASM name for a convention. It is
 * a different token from the SYSCALL instruction. There is no VECTORCALL
 * spelling in this catalogue. The same idea would be another language type
 * if MASM grew one. PASCAL, BASIC, and FORTRAN are older language types
 * with their own argument order and decoration. They remain in the group
 * because a PROTO can still name them.
 */
/*
 * STDCALL - STDCALL Keyword
 * Calling-convention keyword for callee-clean parameter passing on the stack in 32-bit Windows-
 * style code.
 *
 * Syntax: STDCALL
 * Common usage: Used in .MODEL, PROC, and PROTO declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .model flat, stdcall
 */
static const char* const ASM_STDCALL = "stdcall";

/*
 * C_CALL - C Calling-Convention Keyword
 * Calling-convention keyword for C-style caller-clean calls.
 *
 * Syntax: C
 * Common usage: Used in .MODEL, PROC, and PROTO declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   printf PROTO C :PTR BYTE
 */
static const char* const ASM_C_CALL = "c";

/*
 * SYSCALL_CALL - SYSCALL Calling-Convention Keyword
 * MASM calling-convention keyword spelling used in prototype metadata, distinct from the CPU
 * instruction with the same mnemonic.
 *
 * Syntax: SYSCALL
 * Common usage: Used where MASM wants a language/calling-convention name rather than an opcode.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyApi PROTO SYSCALL :QWORD
 */
static const char* const ASM_SYSCALL_CALL = "syscall";

/*
 * PASCAL - PASCAL Keyword
 * Calling-convention keyword for Pascal-style parameter ordering and callee cleanup.
 *
 * Syntax: PASCAL
 * Common usage: Used in legacy Windows and educational examples.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .model small, pascal
 */
static const char* const ASM_PASCAL = "pascal";

/*
 * BASIC - BASIC Keyword
 * Calling-convention keyword for BASIC-compatible interfaces in older MASM toolchains.
 *
 * Syntax: BASIC
 * Common usage: Historical interop feature for legacy environments.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   SomeProc PROTO BASIC :WORD
 */
static const char* const ASM_BASIC = "basic";

/*
 * FORTRAN - FORTRAN Keyword
 * Calling-convention keyword for FORTRAN-compatible interfaces in older MASM toolchains.
 *
 * Syntax: FORTRAN
 * Common usage: Historical interop feature for legacy codebases.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   SomeProc PROTO FORTRAN :WORD
 */
static const char* const ASM_FORTRAN = "fortran";

/*
 * FASTCALL - FASTCALL Keyword
 * Calling-convention keyword for register-assisted parameter passing.
 *
 * Syntax: FASTCALL
 * Common usage: Used in prototypes and procedures where MASM should apply fastcall naming and
 * argument rules.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyProc PROTO FASTCALL :DWORD, :DWORD
 */
static const char* const ASM_FASTCALL = "fastcall";

/*
 * USES - USES Keyword
 * PROC attribute that tells MASM to save and restore listed registers in the generated
 * prologue/epilogue.
 *
 * Syntax: PROC USES reglist
 * Common usage: Convenient for preserving nonvolatile registers automatically in MASM
 * procedures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyProc PROC USES ebx esi edi
 */
static const char* const ASM_USES = "uses";

/*
 * VARARG - VARARG Keyword
 * Marks the end of fixed parameters in a procedure prototype to indicate additional unnamed
 * arguments may follow.
 *
 * Syntax: name PROTO arg1:type, arg2:type, VARARG
 * Common usage: Put VARARG on a PROTO when INVOKE must accept a trailing list
 * whose length is not known to the prototype. `printf PROTO C :PTR BYTE, :VARARG`
 * is the usual case: the format string is the fixed argument, and INVOKE pushes
 * whatever follows it under the C convention (the caller removes them). The
 * callee discovers the count from the format string, not from VARARG. VARARG
 * has no effect if you write the pushes yourself. This generator counts
 * arguments on the AsmCall and emits the moves in write_extern_call, so it
 * does not put VARARG on its prototypes. See the calling-convention banner
 * above.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   printf PROTO C :PTR BYTE, :VARARG
 */
static const char* const ASM_VARARG = "vararg";

/*
 * HIGH - HIGH Operator
 * Returns the high byte of an expression at assembly time.
 *
 * Syntax: HIGH expression
 * Common usage: HIGH, LOW, HIGHWORD, and LOWWORD extract a byte or a word from a constant or
 * a relocatable expression at assemble time. `HIGH 1234h` is the number 12h. A register alias
 * such as AH names a register, not a slice of an immediate, so it cannot replace HIGH. Use
 * HIGH when you are splitting a constant into a table or an instruction immediate.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov al, HIGH 1234h
 */
static const char* const ASM_HIGH = "high";

/*
 * LOW - LOW Operator
 * Returns the low byte of an expression at assembly time.
 *
 * Syntax: LOW expression
 * Common usage: Assemble-time low byte of a constant or relocatable expression, same family as
 * HIGH. `LOW 1234h` is 34h. AL is a register name, not this operator. You cannot write
 * `low rax` to read a register; that is just AL or the low 8 bits via a MOVZX.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov al, LOW 1234h
 */
static const char* const ASM_LOW = "low";

/*
 * HIGHWORD - HIGHWORD Operator
 * Returns the high 16 bits of a 32-bit expression at assembly time.
 *
 * Syntax: HIGHWORD expression
 * Common usage: Assemble-time high 16 bits of a constant or relocatable expression.
 * `HIGHWORD 12345678h` is 1234h. It does not name the high half of a register; there is no
 * register alias that means "the high word of this immediate".
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ax, HIGHWORD 12345678h
 */
static const char* const ASM_HIGHWORD = "highword";

/*
 * LOWWORD - LOWWORD Operator
 * Returns the low 16 bits of a 32-bit expression at assembly time.
 *
 * Syntax: LOWWORD expression
 * Common usage: Use this instead of a register alias when the thing you are slicing is a
 * constant or a link-time address, not a register. `LOWWORD 12345678h` is the number 5678h.
 * AX is the low 16 bits of EAX as a register name; it is not a slice of an immediate, and
 * `lowword eax` is not how you read a register. Split a constant for a table or a 16-bit
 * immediate. The other operators in this family are LOW, HIGH, and HIGHWORD.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ax, LOWWORD 12345678h
 */
static const char* const ASM_LOWWORD = "lowword";

/*
 * OPATTR - OPATTR Operator
 * Returns an attribute bitmask describing a symbol or operand's properties.
 *
 * Syntax: OPATTR operand
 * Common usage: Assemble-time only. Macros use the mask to tell a register from a memory
 * operand or an immediate and expand differently. Bit 10h means the operand is a register.
 * It does not execute, and it is not a substitute for TYPE or SIZEOF.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   Check MACRO arg
 *     IF (OPATTR(arg)) AND 10h
 *       echo register
 *     ELSE
 *       echo not a register
 *     ENDIF
 *   ENDM
 */
static const char* const ASM_OPATTR = "opattr";

/*
 * SHORT - SHORT Keyword
 * Forces a short relative jump or treats an expression as a short-distance target.
 *
 * Syntax: JMP SHORT label
 * Common usage: Used when the programmer wants an 8-bit relative branch encoding or explicit
 * distance override.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   jmp short again
 */
static const char* const ASM_SHORT = "short";
