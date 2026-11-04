/*
 * ex6_uaf.c - generator for the generation-time use-after-free example.
 *
 *   EX_ENABLE defined   -> uaf_enable(): every object gets a liveness-table
 *     entry. We allocate obj A, use it (ok), free it, then use it again -> the
 *     woven check sees A's entry marked freed and traps ("[TRAP] use after
 *     free", exit 1).
 *
 *   EX_ENABLE undefined -> no table, no checks (zero footprint): the second use
 *     of A is a plain read/print, the program prints all three uses and exits 0.
 *
 * "Using" an object means printing its payload, or storing through it.
 * uaf_use and uaf_store both check the liveness byte first. A freed
 * object takes the trap. A live store is an ordinary write of the payload.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex6_uaf.h"

/* Generate the use-after-free example into ex6_output.asm. With checks on,
 * a later use of freed object A exits 1 through the trap. With checks off,
 * that use prints and the program exits 0. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex6_output.asm");
#ifdef EX_ENABLE
    uaf_enable();
#endif
    ExConsole con = ex_console_declare();

        ExStr useA1 = ex_line("s_useA1", "use A (live)  = ", 0, 0);
        ExStr useA2 = ex_line("s_useA2", "use A (freed) = ", 0, 0);
        ExStr useB  = ex_line("s_useB", "use B (live)  = ", 0, 0);
        ExStr sNl   = ex_line("s_nl", "", 2, 0);
        ExStr sTrap = ex_line("s_trap", "[TRAP] use after free", 2, 0);
        c_word_data("objA", "111", "value");    /* A's payload */
        c_word_data("objB", "222", "value");    /* B's payload */
        uaf_globals();
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);

        UafCtx uaf = uaf_begin(&con, &print, sTrap);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            UafObj a = uaf_new("objA");
            UafObj b = uaf_new("objB");

            /* uaf_use prints the payload only after uaf_check has seen that
             * the object's liveness byte is still 1. The same check guards
             * uaf_store, which writes the new payload through the object. */
            uaf_use(&uaf, &print, &pu64, useA1, sNl, a);   /* live: prints 111 */
            uaf_store(&uaf, a, "333");                     /* live assignment  */
            uaf_use(&uaf, &print, &pu64, useB,  sNl, b);   /* live: ok         */

            uaf_free(a);                                   /* free A           */
            uaf_use(&uaf, &print, &pu64, useA2, sNl, a);   /* UAF trap         */

            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
