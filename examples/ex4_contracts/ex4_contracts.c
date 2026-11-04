/*
 * ex4_contracts.c - generation-time contracts on a C++ class.
 *
 *   EX_ENABLE defined   -> contracts_enable(). Account is a cpp class.
 *     The invariant is a member that returns a bool. Method close calls it
 *     and traps when it returns non-zero. deposit's pre-condition is amount
 *     > 0, checked on entry. Its post-condition is balance >= balance at
 *     entry, recorded at the top and emitted when the function closes.
 *     deposit(100) passes. deposit(-50) traps (exit 1) before the balance
 *     changes.
 *
 *   EX_ENABLE undefined -> no checks and no invariant (zero footprint).
 *     deposit(-50) subtracts, both deposits print, exit 0. The token ct_trap
 *     is absent.
 *
 * ct_check keeps the value under test in its own variable. The message
 * address is scratch the library owns, so the body does not pin storage.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "cpp_funcs.h"
#include "ex_console.h"
#include "ex4_contracts.h"

/* Generate the contracts example into ex4_output.asm. deposit(100) keeps a
 * non-negative balance. With contracts on, deposit(-50) exits 1 on the
 * amount>0 pre-condition. With contracts off, both deposits print and the exit is 0. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex4_output.asm");
    cpp_init();
#ifdef EX_ENABLE
    contracts_enable();
#endif
    ExConsole con = ex_console_declare();

        ExStr banner  = ex_line("s_banner", "[CONTRACT] violated: ", 0, 0);
        ExStr mReqPos = ex_line("s_reqpos", "deposit pre-condition amount>0", 2, 0);
        ExStr mPost   = ex_line("s_post", "deposit post-condition balance>=entry", 2, 0);
        ExStr mInv    = ex_line("s_inv", "account invariant balance>=0", 2, 0);
        ExStr sBal    = ex_line("s_bal", "balance = ", 0, 0);
        ExStr sNl     = ex_line("s_nl", "", 2, 0);
        ct_globals();
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);
        CtCtx ct = ct_begin(&con, &print, banner);

        cpp_class_with_contracts_begin("Account");
            cpp_field("balance", C_PTR);
            CFunc* inv = cpp_class_with_contracts_invariant_begin();
                CValue held = cpp_this_field("balance", C_PTR);
                CValue bad = ct_is_negative(held);
                c_ret_value(inv, bad);
                ct_remember_invariant(inv, mInv);
            cpp_method_end();

            CFunc* ctor = cpp_ctor_begin();
                CValue fresh = cpp_this_field("balance", C_PTR);
                c_init_n(&fresh, 0);
            ct_member_end(ctor, &ct);

            /* deposit(this, amount). The post-condition names balance.
             * ct_method_end reloads that field, compares, and closes. */
            const CParam extra[] = { {"amount", C_PTR} };
            CFunc* deposit = ct_method_begin("deposit", extra, 1);
                CValue amount = c_param(deposit, 1);
                ct_require_gt(&ct, amount, "0", mReqPos);
                CValue before = cpp_this_field("balance", C_PTR);
                ct_postcondition(&ct, c_cell(before), "ct_entry", CT_GE, mPost);
                c_add(&before, amount);
                ex_puts(&print, sBal);
                ex_print_val(&pu64, cpp_this_field("balance", C_PTR));
                ex_puts(&print, sNl);
            ct_method_end(deposit, &ct);
        CppClass Account = cpp_class_end();
        cpp_reserve(&Account, "acct");
        CppObject acct;
        acct.sym = "acct";
        acct.rec = Account.rec;

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            cpp_send(ctor, acct);
            cpp_send_imm(deposit, acct, 100, "amount");
            cpp_send_imm(deposit, acct, -50, "amount");
            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
