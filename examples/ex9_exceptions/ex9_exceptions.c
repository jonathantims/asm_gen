/*
 * ex9_exceptions.c - withdraw either returns a difference or raises code 42.
 *
 *   EX_ENABLE defined   -> an overdraft raises code 42. The vectored handler
 *     resumes in main's catch, which prints "[CAUGHT] overdraft, code = 42"
 *     and exits 0. The "result =" line is skipped.
 *
 *   EX_ENABLE undefined -> withdraw(100, 150) returns 100-150, main prints
 *     it, and the program exits 0. No veh_trap, no handler globals.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex9_exceptions.h"

typedef struct {
    AsmExtern* print;
    AsmExtern* pu64;
    ExStr caught;
    ExStr nl;
    ExConsole* con;
} ExcDemo;

/* The catch the header emits only when exceptions are on. */
static void withdraw_catch(void* user) {
    ExcDemo* g = (ExcDemo*)user;
    ex_puts(g->print, g->caught);
    CValue code = exc_code();
    ex_print_val(g->pu64, code);
    c_free(&code);
    ex_puts(g->print, g->nl);
    ex_exit(g->con, 0);
}

/* Emit withdraw. An overdraft throws 42 when the feature is on.
 * Return the balance minus the amount. */
static AsmExtern emit_withdraw(void) {
    const CParam params[] = { { "balance", C_INT }, { "amount", C_INT } };
    CFunc w = c_begin_function("withdraw", params, 2, C_INT, "result");
        CValue balance = c_param(&w, 0);
        CValue amount = c_param(&w, 1);
        CIf over = c_if_ugt(amount, balance);
            exc_throw(42);
        c_end_if(&over);
        CValue result = c_dup("result", C_INT, balance);
        c_sub(&result, amount);
        c_ret_value(&w, result);
    c_end_function(&w);
    return w.ext;
}

/* Build example 9, the withdraw exception program, and write ex9_output.asm. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex9_output.asm");
#ifdef EX_ENABLE
    exc_enable();
#endif
    ExConsole con = ex_console_declare();
    exc_declare();

        ExStr sResult = ex_line("s_result", "result = ", 0, 0);
        ExStr sCaught = ex_line("s_caught", "[CAUGHT] overdraft, code = ", 0, 0);
        ExStr sNl     = ex_line("s_nl", "", 2, 0);
        exc_globals();
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);
        exc_emit_handler();
        AsmExtern withdraw = emit_withdraw();
        ExcDemo caught;
        caught.print = &print;
        caught.pu64 = &pu64;
        caught.caught = sCaught;
        caught.nl = sNl;
        caught.con = &con;

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            exc_install();

            ExcTry t = exc_try_begin();
                CValue result = c_call_proc(C_INT, &withdraw,
                                            c_cell(c_imm_n(100, C_INT, "balance")),
                                            c_cell(c_imm_n(150, C_INT, "amount")));
                ex_puts(&print, sResult);
                ex_print_val(&pu64, result);
                ex_puts(&print, sNl);
                c_free(&result);
                ex_exit(&con, 0);
            exc_catch_with(&t, withdraw_catch, &caught);

        c_end_function(&mainf);

    return c_prog_end();
}
