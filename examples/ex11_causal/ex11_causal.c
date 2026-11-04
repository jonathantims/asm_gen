/*
 * ex11_causal.c - one application of the causal-profiling library.
 *
 * ex11_causal.h is the library. This file is the test case. It supplies
 * two profiled bodies and a workload that runs them on two threads.
 * EX_ENABLE is the only switch.
 *
 *   critical_step  STEP_ADDS adds. This is the long call. Delaying it
 *                  stretches the wait, because the other thread is the
 *                  empty one.
 *   side_step      returns at once. The same delay, applied here, does
 *                  not stretch the wait the way delaying the long call
 *                  does.
 *
 * The workers are not profiled. Each calls one body CAUSAL_REPS times.
 * The test case starts both workers and waits for both. The campaign,
 * which the library emits as main, warms up and then runs a baseline
 * and a trial for each selection.
 *
 *   EX_ENABLE defined   -> each profiled function can pause before it
 *     returns. The first pause site is labelled causal_delay. The
 *     campaign writes causal_rank.txt and exits 0. Each line is the
 *     function and the proportional slowdown of the test case against
 *     the proportional slowdown of that function.
 *
 *   EX_ENABLE undefined -> the wrappers are plain functions. The
 *     campaign runs the test case once and exits 0. causal_delay is
 *     absent.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex11_causal.h"

/* Enough adds that this call outlasts the empty one even when the
 * campaign pauses the empty one. A few hundred adds disappear under
 * the cost of starting the two threads. */
enum { STEP_ADDS = 50000 };

/* The long body. A local counts from 0 to STEP_ADDS and then the wrapper
 * returns. The adds are the work. The pause, when the campaign selects
 * this function, is inserted by causal_func_end, after the loop. */
static CFunc emit_critical_step(void) {
    causal_func_begin("critical_step");
        CValue n = c_local_imm_n("work", C_PTR, 0);
        CLoop w = c_loop_begin();
            c_add(&n, c_imm_n(1, C_PTR, "work"));
            CIf enough = c_if_uge(n, c_imm_n(STEP_ADDS, C_PTR, "work"));
                c_loop_break(&w);
            c_end_if(&enough);
        c_loop_end(&w);
        c_free(&n);
    return causal_func_end();
}

/* The empty body. Watching it is what makes the ranking mean something. */
static CFunc emit_side_step(void) {
    causal_func_begin("side_step");
    return causal_func_end();
}

/* A thread procedure named name. It is not profiled. It calls step
 * CAUSAL_REPS times and returns 0. */
static void emit_worker(const char* name, CFunc* step) {
    CFunc th = causal_worker_begin(name);
        causal_repeat(step);
        c_ret_zero(&th);
    c_end_function(&th);
}

/* The black box. Two threads, one per body, started together and joined
 * before the test case returns. Delaying the empty thread does not move
 * the join the way delaying the long thread does. */
static void emit_case(void) {
    causal_testcase_begin();
        causal_spawn("critical_worker");
        causal_spawn("side_worker");
        causal_join_all();
    causal_testcase_end();
}

/* Build example 11, the causal-profiling campaign, and write ex11_output.asm. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex11_output.asm");
#ifdef EX_ENABLE
    causal_enable();
#endif
    ExConsole con = ex_console_declare();
    causal_prepare();

        causal_emit_data();
        ex_console_globals();
        causal_scratch();
        CFunc critical = emit_critical_step();
        CFunc side = emit_side_step();
        emit_worker("critical_worker", &critical);
        emit_worker("side_worker", &side);
        emit_case();
        causal_campaign(&con);

    return c_prog_end();
}
