/*
 * ex3_profile.c - generator for the generation-time call-profiling example.
 *
 *   EX_ENABLE defined   -> prof_enable(): each profiled call is timed and a
 *     CSV row "<function>,<ticks>" is printed. The program prints a small
 *     CSV report then exits 0.
 *
 *   EX_ENABLE undefined -> profiling disabled: the calls are emitted bare
 *     (no timing, no CSV) - zero footprint. The program prints just a
 *     "[done]" line and exits 0.
 *
 * Two work functions (spin loops of different lengths) are profiled so the CSV
 * shows two rows with different tick counts.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex3_profile.h"

static const char* work_small = "work_small";
static const char* work_large = "work_large";

/* Generate the call-profiling example into ex3_output.asm. Both builds exit 0.
 * Profiling on prints a CSV row for each timed spin. Profiling off prints a
 * done line after the two spin calls. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex3_output.asm");
#ifdef EX_ENABLE
    prof_enable();
#endif
    ExConsole con = ex_console_declare();

        ExStr sHdr   = ex_line("s_hdr", "function,ticks", 2, 0);
        ExStr sComma = ex_line("s_comma", ",", 0, 0);
        ExStr sNl    = ex_line("s_nl", "", 2, 0);
        ExStr sWork1 = ex_line("s_work1", work_small, 0, 0);
        ExStr sWork2 = ex_line("s_work2", work_large, 0, 0);
        ExStr sDone  = ex_line("s_done", "[done] profiling disabled; no timing compiled in", 2, 0);
        ex_console_globals();
        prof_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);

        AsmExtern work1 = prof_emit_spin(work_small, 100000);
        AsmExtern work2 = prof_emit_spin(work_large, 3000000);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            ProfCtx prof = prof_begin(&con, &print, &pu64, sComma, sNl);

            prof_banner(&prof, sHdr);
            prof_call(&prof, sWork1, &work1);
            prof_call(&prof, sWork2, &work2);
            prof_quiet(&prof, sDone);
            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
