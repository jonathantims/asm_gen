/*
 * ex8_fuzzmock.c - fuzz an adaptable definition under mocked input.
 *
 *   EX_ENABLE defined   -> the same emitter produces read_and_double_mock,
 *     which reads mock_input. The fuzz test drives 256 inputs through it.
 *     Prints "PASS read_and_double" and "ex8 done". Exit 0.
 *
 *   EX_ENABLE undefined -> only the production function and get_input.
 *     Prints the production result and "ex8 done". Exit 0. run_all_tests
 *     is absent.
 *
 * emit_read_and_double is the single definition. SRC_REAL calls get_input;
 * SRC_MOCK loads the global. Both double the value and return it.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "../ex7_fuzz/ex7_fuzz.h"
#include "ex8_fuzzmock.h"

typedef struct { AsmExtern* get_input; } MockArgs;

/* Emit proc_name. src selects a call of get_input or a load of mock_input.
 * Double that value and return it in the function result. */
static AsmExtern emit_read_and_double(const char* proc_name, InputSource src,
                                      AsmExtern* get_input) {
    CFunc f = c_begin_function(proc_name, NULL, 0, C_PTR, "result");
        CValue v;
        if (src == SRC_REAL) v = c_call_proc(C_PTR, get_input);
        else                 v = mock_get();
        c_add(&v, v);
        c_ret_value(&f, v);
    c_end_function(&f);
    return f.ext;
}

static AsmExtern emit_mock(void* user) {
    MockArgs* m = (MockArgs*)user;
    return emit_read_and_double("read_and_double_mock", SRC_MOCK, m->get_input);
}

/* For each fuzz input, set the mock and check the result is even and doubled. */
static void rad_fuzz_body(FuzzCtx* c) {
    fuzz_loop_begin(c);
        CValue in = fuzz_next_input();
        mock_set(in);
        CValue got = c_call_proc(C_PTR, c->fn);
        fuzz_expect_even(c, got);
        CValue exp = c_dup("exp", C_PTR, in);
        c_add(&exp, in);
        fuzz_expect_eq(c, got, exp);
        c_free(&exp);
        c_free(&got);
    fuzz_loop_end(c);
}

/* Build example 8, the mocked read_and_double fuzz, and write ex8_output.asm. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex8_output.asm");
#ifdef EX_ENABLE
    testing_enable();
#endif
    ExConsole con = ex_console_declare();
    fuzz_register("read_and_double", NULL, rad_fuzz_body, NULL);

        ExStr sRes  = ex_line("s_res", "read_and_double() = ", 0, 0);
        ExStr sNl   = ex_line("s_nl", "", 2, 0);
        ExStr sDone = ex_line("s_done", "ex8 done", 2, 0);
        fuzz_emit_data();
        fuzz_emit_scratch();
        mock_emit_globals();
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);

        CFunc gin = c_begin_function("get_input", NULL, 0, C_PTR, "in");
            CValue seven = c_local_imm_n("in", C_PTR, 7);
            c_ret_value(&gin, seven);
        c_end_function(&gin);
        AsmExtern get_input = gin.ext;

        AsmExtern rad = emit_read_and_double("read_and_double", SRC_REAL, &get_input);
        MockArgs args;
        args.get_input = &get_input;
        fuzz_bind_emit("read_and_double", emit_mock, &args);
        fuzz_emit_runner(&con, &print, &pu64);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            fuzz_invoke();

            ex_puts(&print, sRes);
            ex_print_val(&pu64, c_call_proc(C_PTR, &rad));
            ex_puts(&print, sNl);

            ex_puts(&print, sDone);
            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
