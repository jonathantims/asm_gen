/*
 * js_main.c - A JavaScript feature tour emitted through js_funcs.h.
 *
 * Generates js_output_<option>.asm. At run time it shows:
 *
 *   1. Two closures. makeCounter() allocates a leaf cell on the collector
 *      heap. tick(env) adds one to that cell's count. Counter A's three
 *      ticks are 1, 2, 3. Counter B's first tick is 1. The two counters are
 *      different objects, so their counts do not interfere.
 *   2. Prototype lookup. child.greet is not an own property; the walk follows
 *      __proto__ and returns the function, and the caller calls it. child.missing
 *      is 0, so the caller prints undefined.
 *   3. A property added at run time with js_set, then found on the own list.
 *   4. Truthiness of a tagged number, an empty string, null, a nonempty string,
 *      and an object.
 *   5. The garbage-collection tour selected by -DGC_OPTION.
 *
 * The program prints [JavaScript] feature tour and exits 0.
 */

#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "gc.h"
#include "js_funcs.h"

/* Look prop up on the receiver of fn. 0 means undefined and print writes undef;
 * otherwise call the pointer with this. */
static void js_call_or_undef(CFunc* fn, const char* prop, AsmExtern* print, JsStr undef) {
    CValue self = c_dup("self", C_PTR, c_param(fn, 0));
    CValue found = js_lookup(self, prop);
    CElse miss = c_if_eq_else(found, c_imm("0", C_PTR, "fn"));
        js_puts(print, undef);
    c_else(&miss);
        js_call_fn(found, self);
    c_end_if_else(&miss);
}

/* Print when_true when v is truthy and when_false when it is falsy, through
 * print, then release v. */
static void js_truth(CValue v, AsmExtern* print, JsStr when_true, JsStr when_false) {
    CElse t = js_if_truthy_else(v);
        js_puts(print, when_true);
    c_else(&t);
        js_puts(print, when_false);
    c_end_if_else(&t);
    c_free(&v);
}

/* makeCounter allocates one environment and returns it. The count starts at 0. */
static CFunc js_make_counter(GcHeap* h, GcType* env) {
    CFunc f = c_begin_function("makeCounter", NULL, 0, C_PTR, "env");
        CValue obj = js_counter_new(h, env);
        c_ret_value(&f, obj);
    c_end_function(&f);
    return f;
}

/* tick(env) adds one to env.count and prints the line for 1, for 2, or
 * for any later value. */
static CFunc js_tick(AsmExtern* print, JsStr one, JsStr two, JsStr more) {
    const CParam tp[] = { {"env", C_PTR} };
    CFunc tick = c_begin_function("tick", tp, 1, C_PTR, NULL);
        CValue env = c_param(&tick, 0);
        js_field_add_imm(js_counter_field(env), 1);
        CValue count = c_dup("count", C_PTR, js_counter_field(env));
        CSwitch sw = c_switch(count);
            c_case_n(&sw, 1);
                js_puts(print, one);
                c_ret(&tick);
            c_case_n(&sw, 2);
                js_puts(print, two);
                c_ret(&tick);
            c_default(&sw);
                js_puts(print, more);
                c_ret(&tick);
        c_end_switch(&sw);
    c_end_function(&tick);
    return tick;
}

/* Generate the JavaScript feature tour. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    char fname[64];
    snprintf(fname, sizeof fname, "js_output_%s.asm", GC_OPTION_STR);
    c_prog_begin(fname);
    js_init();

    JsConsole con = js_console_declare();
    gc_externs();

        JsStr sBanner = js_line("j_banner", "[JavaScript] feature tour");
        JsStr sCloHdr = js_line("j_clohdr", "  closure: two makeCounter() calls");
        JsStr sA      = js_line("j_ca", "  counter A");
        JsStr sC1     = js_line("j_c1", "    tick() -> 1");
        JsStr sC2     = js_line("j_c2", "    tick() -> 2  (same env)");
        JsStr sC3     = js_line("j_c3", "    tick() -> 3  (state persisted)");
        JsStr sB      = js_line("j_cb", "  counter B, first tick is 1 (fresh env)");
        JsStr sProHdr = js_line("j_prohdr", "  prototype chain: child.greet(); child.missing");
        JsStr sGreet  = js_line("j_greet", "    child.greet() -> found on prototype: Hello");
        JsStr sUndef  = js_line("j_undef", "    child.missing -> undefined");
        JsStr sSetHdr = js_line("j_sethdr", "  runtime property: js_set(child, extra)");
        JsStr sSetHit = js_line("j_sethit", "    child.extra() -> found on own list");
        JsStr sTruHdr = js_line("j_truhdr", "  truthiness: number, string, null, object");
        JsStr s42t    = js_line("j_42t", "    42 is truthy");
        JsStr s42f    = js_line("j_42f", "    42 is falsy");
        JsStr s0t     = js_line("j_0t", "    0 is truthy");
        JsStr s0f     = js_line("j_0f", "    0 is falsy");
        JsStr sEt     = js_line("j_et", "    empty string is truthy");
        JsStr sEf     = js_line("j_ef", "    empty string is falsy");
        JsStr sNt     = js_line("j_nt", "    null is truthy");
        JsStr sNf     = js_line("j_nf", "    null is falsy");
        JsStr sHt     = js_line("j_ht", "    nonempty string is truthy");
        JsStr sHf     = js_line("j_hf", "    nonempty string is falsy");
        JsStr sOt     = js_line("j_ot", "    object is truthy");
        JsStr sOf     = js_line("j_of", "    object is falsy");
        JsStr sFldHdr = js_line("j_fldhdr", "  own field: child.bump(); child.bump(); child.report()");
        JsStr sFldHit = js_line("j_fldhit", "    child.count === 2 (own field persisted)");

        JsStr sGcHdr  = js_line("j_gchdr", "  gc [" GC_OPTION_NAME "] collector tour");
        JsStr sGcAlloc= js_line("j_gcalloc", "    allocated a, b; a.next = b; live addresses:");
        JsStr sGcReuse= js_line("j_gcreuse", "    dropped roots + collected; new object reuses freed space:");
        GcTypeDesc nodeDesc = gc_type_begin("Node");
            gc_field_ptr(&nodeDesc, "next");
            gc_field_word(&nodeDesc, "value");
        GcType Node = gc_type_end(&nodeDesc);
        GcType Env = js_env_type();

        js_data();

        js_console_globals();
        gc_data_globals();

        AsmExtern print = js_console_print(&con);
        GcHeap heap = gc_begin(print);
        js_emit_runtime();

        js_class_begin("Proto");
            CFunc* protoGreet = js_method_begin("greet");
                js_puts(&print, sGreet);
                c_ret(protoGreet);
            js_method_end();
        JsClass Proto = js_class_end();

        js_class_begin("Child");
            /* count is an own field after the header, not a list property.
             * The list does not distinguish data from methods. */
            js_field("count");
            CFunc* childBump = js_method_begin("bump");
                js_field_add_imm(js_this_field("count", C_INT), 1);
                c_ret(childBump);
            js_method_end();
            CFunc* childReport = js_method_begin("report");
                CIf hit = c_if_uge(js_this_field("count", C_INT),
                                   c_imm_n(2, C_INT, "count"));
                    js_puts(&print, sFldHit);
                c_end_if(&hit);
                c_ret(childReport);
            js_method_end();
        JsClass Child = js_class_end();

        JsObject protoObj = js_construct(&Proto, "proto_obj", NULL);
        JsObject childObj = js_construct(&Child, "child_obj", "proto_obj");
        (void)protoObj;

        CFunc extra = js_function_begin("extra_fn");
            js_puts(&print, sSetHit);
            c_ret(&extra);
        js_function_end(&extra);

        /* makeCounter returns a fresh heap cell. tick's parameter is that
         * cell, not a dummy receiver. */
        CFunc makeCounter = js_make_counter(&heap, &Env);
        CFunc tick = js_tick(&print, sC1, sC2, sC3);

        CFunc getGreet = js_function_begin("get_greet");
            js_call_or_undef(&getGreet, "greet", &print, sUndef);
            c_ret(&getGreet);
        js_function_end(&getGreet);

        CFunc getMissing = js_function_begin("get_missing");
            js_call_or_undef(&getMissing, "missing", &print, sUndef);
            c_ret(&getMissing);
        js_function_end(&getMissing);

        CFunc getExtra = js_function_begin("get_extra");
            js_call_or_undef(&getExtra, "extra", &print, sUndef);
            c_ret(&getExtra);
        js_function_end(&getExtra);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            /* makeCounter allocates before the collector tour. gc_demo
             * starts the heap again for its own objects. */
            gc_start(&heap);
            js_puts(&print, sBanner);

            /* Two calls allocate two cells. A's three ticks print 1, then 2,
             * then 3, because tick writes the count back into that cell.
             * B is a different cell, so its first tick is 1. The calls save
             * the cell pointers; they do not need a manual spill. */
            js_puts(&print, sCloHdr);
            CValue c1 = c_call_proc(C_PTR, &makeCounter.ext);
            CValue c2 = c_call_proc(C_PTR, &makeCounter.ext);
            js_puts(&print, sA);
            c_call(&tick, c_cell(c1));
            c_call(&tick, c_cell(c1));
            c_call(&tick, c_cell(c1));
            js_puts(&print, sB);
            c_call(&tick, c_cell(c2));
            /* The registers go back to the pool. The cells stay on the
             * heap until gc_demo starts a fresh one. */
            // TODO: is this just the fact the names themselves are undefined (like when a variable name is deleted in JavaScript or Python) -- this doesn't refer to CPU registers does it?
            c_free(&c1);
            c_free(&c2);

            /* greet is not on the child. Lookup walks __proto__ and finds
             * Proto.greet. missing is on neither object, so the word is 0
             * and the caller prints undefined. */
            js_puts(&print, sProHdr);
            js_send(&getGreet, childObj);
            js_send(&getMissing, childObj);

            /* js_set prepends a node on the child's own list. The next
             * lookup finds extra there and calls it. The node comes from
             * the sixteen-node pool, which is not resized. */
            js_puts(&print, sSetHdr);
            CValue child = js_word_lea("child", "child_obj");
            CValue extra_p = js_word_lea("extra", "extra_fn");
            js_set(child, "extra", extra_p);
            c_free(&child);
            c_free(&extra_p);
            js_send(&getExtra, childObj);

            /* count is a header field, not a property-list entry. Two bumps
             * leave it at 2, and report prints that. */
            js_puts(&print, sFldHdr);
            js_send(childBump, childObj);
            js_send(childBump, childObj);
            js_send(childReport, childObj);

            /* 0 is null. Tag 01 is a number, falsy when the value is 0
             * (the word is 1). Tag 10 is a string, falsy when the first
             * byte is 0. Any other nonzero word is an object and is truthy. */
            js_puts(&print, sTruHdr);
            js_truth(js_number("n42", 42), &print, s42t, s42f);
            js_truth(js_number("n0", 0), &print, s0t, s0f);
            js_truth(js_string_val("empty", "js_empty"), &print, sEt, sEf);
            js_truth(c_local_imm("nullv", C_PTR, "0"), &print, sNt, sNf);
            js_truth(js_string_val("hi", "js_hi"), &print, sHt, sHf);
            js_truth(js_word_lea("objv", "child_obj"), &print, sOt, sOf);

            /* The collector tour. gc_demo starts a fresh heap, so the
             * counter cells are not part of that collection. */

            gc_demo(&print, &heap, &Node,
                    gc_msg_of(sGcHdr),
                    gc_msg_of(sGcAlloc),
                    gc_msg_of(sGcReuse));

            js_exit(&con, "0");
        c_end_function(&mainf);

    return c_prog_end();
}
