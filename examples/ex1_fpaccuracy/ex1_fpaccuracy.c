/*
 * ex1_fpaccuracy.c - a short floating-point calculation with three checks.
 *
 *   no flag                         -> FP_PLAIN. No shadow, no tree, no trap.
 *                                      Exit 0. There is no fp_trap: c_funcs.h
 *                                      has no floating-point compare, so the
 *                                      trap stays a library label.
 *   EX_ENABLE                       -> FP_SHADOW. Each add, multiply, and
 *                                      divide is mirrored at higher precision.
 *                                      The float result is compared with that
 *                                      double. Exit 1 via fp_trap.
 *   EX_FP_TREE (wins if both flags) -> FP_TREE. The same calculation is an
 *                                      expression tree evaluated at 53-bit
 *                                      precision into fp_ref. Exit 1 via fp_trap.
 *
 * The calculation is acc = 0; a million times acc += 0.1; acc = acc / 2;
 * acc = acc * 1.1. The division is the extra step. The million inexact adds
 * are what push the single-precision result away from the oracle.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex1_fpaccuracy.h"

/* Generate the floating-point shadow example into ex1_output.asm.
 * Plain mode exits 0. Shadow and tree modes exit 1 through fp_trap when
 * the scaled absolute error of the million-add run exceeds 1000. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex1_output.asm");
#ifdef EX_FP_TREE
    fp_set_mode(FP_TREE);
    fp_set_precision(53);
#elif defined(EX_ENABLE)
    fp_set_mode(FP_SHADOW);
#endif
    ExConsole con = ex_console_declare();

    FpVal acc  = fp_acc("fp_acc");
    FpVal step = fp_const("fp_step", "0.1", 0.1);
    FpVal two  = fp_const("fp_two", "2.0", 2.0);
    FpVal gain = fp_const("fp_gain", "1.1", 1.1);
    fp_repeat(&acc, &step, 1000000);
    fp_divide(&acc, &two);
    fp_multiply(&acc, &gain);

        ExStr sErr  = ex_line("s_err", "abs error x1e6 = ", 0, 0);
        ExStr sNl   = ex_line("s_nl", "", 2, 0);
        ExStr sTrap = ex_line("s_trap", "[TRAP] floating-point error exceeded threshold", 2, 0);
        fp_emit_recorded();
        fp_emit_scratch();
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);
        FpCtx fp = fp_begin(&con, &print, sTrap, &acc);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            fp_emit_program();
            fp_check(&fp, &pu64, sErr, sNl, &acc);
            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
