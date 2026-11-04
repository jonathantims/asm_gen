/*
 * ex2_overflow.c - generator for the generation-time overflow-check example.
 *
 * The SAME source emits two programs depending on whether EX_ENABLE is defined
 * at generator-compile time:
 *
 *   - EX_ENABLE defined  -> ovf_enable(): overflow checks are woven in. The
 *     program performs a safe add (no trap) then an overflowing 32-bit add,
 *     which triggers the trap: prints "[TRAP] integer overflow" and exits 1.
 *
 *   - EX_ENABLE undefined -> checks disabled: the emitted code contains no
 *     overflow check and no trap handler at all (zero footprint). The
 *     overflowing add silently wraps, the program prints "[done] no checks"
 *     and exits 0.
 *
 * build_examples.bat builds both and asserts the two behaviours, and also diffs
 * the disabled output against a baseline to prove the zero-footprint claim.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex2_overflow.h"

/* Generate the integer-overflow example into ex2_output.asm. The safe add
 * prints in both builds. With checks on, the overflowing add exits 1 at the
 * trap. With checks off, that add wraps, a done line prints, and the exit is 0. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex2_output.asm");
#ifdef EX_ENABLE
    ovf_enable();
#endif
    ExConsole con = ex_console_declare();

        ExStr sSafe = ex_line("s_safe", "[ok] 100 + 200 = 300 (no overflow)", 2, 0);
        ExStr sDone = ex_line("s_done", "[done] overflowing add wrapped; no checks compiled in", 2, 0);
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        OvfCtx ovf = ovf_begin(&con, &print, "[TRAP] integer overflow detected");

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            /* 1. A safe signed add: 100 + 200. With checks on this does NOT trap. */
            CValue acc = c_local_imm_n("acc", C_INT, 100);
            CValue addend = c_local_imm_n("addend", C_INT, 200);
            ovf_add(&ovf, c_cell(acc), c_cell(addend));
            ex_puts(&print, sSafe);

            /* 2. An overflowing signed add. The same locals, after the print. */
            c_assign_imm(&acc, c_dec(2000000000));
            c_assign_imm(&addend, c_dec(2000000000));
            ovf_add(&ovf, c_cell(acc), c_cell(addend));

            /* Reached only when checks are disabled (the add just wraps). */
            ex_puts(&print, sDone);
            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
