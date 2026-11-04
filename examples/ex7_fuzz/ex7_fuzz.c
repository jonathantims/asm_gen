/*
 * ex7_fuzz.c - generator for the generation-time fuzz-testing example.
 *
 *   EX_ENABLE defined   -> testing_enable(): a fuzz test is synthesised next
 *     to mul2. It generates 256 pseudo-random inputs, calls mul2, and checks
 *     the result is even AND equals input*2. run_all_tests prints "PASS mul2".
 *     Exit 0.
 *
 *   EX_ENABLE undefined -> no tests (zero footprint). Exit 0.
 *
 * mul2 doubles its argument and returns that value. The fuzz body sits
 * in this file, next to mul2. The harness emits the body into the
 * generated test procedure. A failing case jumps to the test's fail
 * label; the body does not print the failure itself.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex7_fuzz.h"

/* Inside the harness procedure, for each of 256 inputs: call mul2 and
 * check the result is even and equals input*2. */
static void mul2_fuzz_body(FuzzCtx* c) {
    fuzz_loop_begin(c);
        CValue in = fuzz_next_input();
        CValue got = fuzz_call(c, in);
        fuzz_expect_even(c, got);
        CValue exp = c_dup("exp", C_PTR, in);
        c_add(&exp, in);
        fuzz_expect_eq(c, got, exp);
        c_free(&exp);
        c_free(&got);
    fuzz_loop_end(c);
}

/* mul2(x) = x + x. Adding the parameter to itself doubles it, and
 * c_ret_value returns that value. */
static AsmExtern emit_mul2(void) {
    const CParam params[] = { { "x", C_PTR } };
    CFunc f = c_begin_function("mul2", params, 1, C_PTR, "result");
        CValue x = c_param(&f, 0);
        c_add(&x, x);
        c_ret_value(&f, x);
    c_end_function(&f);
    return f.ext;
}

/* Build example 7, the generation-time fuzz of mul2, and write ex7_output.asm. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex7_output.asm");
#ifdef EX_ENABLE
    testing_enable();
#endif
    ExConsole con = ex_console_declare();
    fuzz_register("mul2", "x", mul2_fuzz_body, NULL);

        ExStr sDone = ex_line("s_done", "ex7 done", 2, 0);
        fuzz_emit_data();
        fuzz_emit_scratch();
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);

        fuzz_bind("mul2", emit_mul2());
        fuzz_emit_runner(&con, &print, &pu64);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            fuzz_invoke();
            ex_puts(&print, sDone);
            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
