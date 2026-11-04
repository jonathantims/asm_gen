/*
 * ex10_bounds.c - generator for the generation-time bounds-checking example.
 *
 *   EX_ENABLE defined   -> bounds_enable(): every access is range-checked. The
 *     in-range access prints its value; the out-of-range access traps
 *     ("[TRAP] index out of bounds", exit 1) before printing.
 *
 *   EX_ENABLE undefined -> checks disabled: the fat pointer degrades to a bare
 *     pointer, both accesses are plain loads (zero footprint). The program
 *     prints both values (the second is whatever lies past the array) and
 *     exits 0.
 *
 * Array: 4 ints {10,20,30,40}. In-range index 2 -> 30. Out-of-range index 5.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex10_bounds.h"

/* Build example 10, the bounds-checked array loads, and write ex10_output.asm. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex10_output.asm");
#ifdef EX_ENABLE
    bounds_enable();
#endif
    ExConsole con = ex_console_declare();

        c_dwords("arr", "10, 20, 30, 40");
        ExStr sIn   = ex_line("s_in", "arr[2] = ", 0, 0);
        ExStr sOut  = ex_line("s_out", "arr[5] = ", 0, 0);
        ExStr sNl   = ex_line("s_nl", "", 2, 0);
        ExStr sTrap = ex_line("s_trap", "[TRAP] index out of bounds", 2, 0);
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            BoundsCtx bc = bounds_begin(&con, &print, sTrap);
            FatPtr arr = fatptr("arr", 4, 4);

            ex_puts(&print, sIn);
            CValue in_range = bounds_load32(&bc, arr, 2);
            ex_print_val(&pu64, in_range);
            c_free(&in_range);
            ex_puts(&print, sNl);

            ex_puts(&print, sOut);
            CValue out_range = bounds_load32(&bc, arr, 5);
            ex_print_val(&pu64, out_range);
            c_free(&out_range);
            ex_puts(&print, sNl);

            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
