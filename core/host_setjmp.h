/* host_setjmp.h emits host_setjmp and host_longjmp. Include host.h, asm.h,
 * and asm_funcs.h first. The image is what a /NODEFAULTLIB build links
 * instead of the C runtime's setjmp.
 *
 * HostJmpBuf is 256 bytes and 16-aligned. The bytes this header writes:
 *   0 rbx, 8 rbp, 16 rdi, 24 rsi, 32 r12, 40 r13, 48 r14, 56 r15,
 *   64 the caller's rsp (rsp+8 on entry), 72 the return address,
 *   80 xmm6 through 224 xmm15.
 * host_setjmp returns 0. host_longjmp returns the code it was given,
 * or 1 when that code is 0, so the caller can tell the jump from the
 * first return. rsp is restored only after the return address is in r11.
 */

/* Emit both procedures into the current assembly buffer. */
static inline void write_host_setjmp(void) {
    const char* const gpr[] = {
        ASM_RBX, ASM_RBP, ASM_RDI, ASM_RSI,
        ASM_R12, ASM_R13, ASM_R14, ASM_R15
    };
    const char* const xmm[] = {
        ASM_XMM6, ASM_XMM7, ASM_XMM8, ASM_XMM9,
        ASM_XMM10, ASM_XMM11, ASM_XMM12, ASM_XMM13,
        ASM_XMM14, ASM_XMM15
    };
    const AsmDataField setjmp_params[] = {{ASM_RCX, "buf"}};
    const AsmDataField longjmp_params[] = {
        {ASM_RCX, "buf"}, {ASM_EDX, "code"}
    };
    AsmDataField eax_code = {ASM_EAX, "code"};
    AsmExtern proc;
    int i;

    write_set_case_sensitive_identifiers();
    write_code();

    proc = write_proc("host_setjmp", setjmp_params, 1, eax_code);
    for (i = 0; i < 8; i++)
        write_mov(asm_mem(ASM_QWORD_KW, ASM_RCX, i * 8), gpr[i], gpr[i]);
    write_lea(ASM_RAX, "[rsp+8]");
    write_mov(asm_mem(ASM_QWORD_KW, ASM_RCX, 64), ASM_RAX, "rsp");
    write_mov(ASM_RAX, asm_mem(ASM_QWORD_KW, ASM_RSP, 0), "rip");
    write_mov(asm_mem(ASM_QWORD_KW, ASM_RCX, 72), ASM_RAX, "rip");
    for (i = 0; i < 10; i++)
        write_movdqu(asm_mem(ASM_XMMWORD_KW, ASM_RCX, 80 + i * 16), xmm[i]);
    write_xor(eax_code, eax_code);
    write_ret();
    write_endp(&proc);

    proc = write_proc("host_longjmp", longjmp_params, 2, asm_void);
    write_mov(ASM_R10D, ASM_EDX, "code");
    write_test(ASM_R10D, ASM_R10D);
    write_jnz("host_longjmp_go");
    write_mov(ASM_R10D, "1", "code");
    write_label("host_longjmp_go");
    for (i = 0; i < 8; i++)
        write_mov(gpr[i], asm_mem(ASM_QWORD_KW, ASM_RCX, i * 8), gpr[i]);
    for (i = 0; i < 10; i++)
        write_movdqu(xmm[i], asm_mem(ASM_XMMWORD_KW, ASM_RCX, 80 + i * 16));
    write_mov(ASM_R11, asm_mem(ASM_QWORD_KW, ASM_RCX, 72), "rip");
    write_mov(ASM_RSP, asm_mem(ASM_QWORD_KW, ASM_RCX, 64), "rsp");
    write_mov(ASM_EAX, ASM_R10D, "code");
    write_jmp(ASM_R11);
    write_endp(&proc);

    write_end("");
}
