/*
 * py_main.c - A Python feature tour emitted through py_funcs.h.
 *
 * Generates py_output.asm, a console program demonstrating (all at RUN time):
 *
 *   1. Reference counting: an object starts at refcount 1; incref bumps it to 2;
 *      the first decref drops it to 1 (still alive) and the second to 0 (which
 *      deallocates / runs __del__). The alive-vs-freed decision is a runtime
 *      branch on the refcount.
 *   2. Duck typing: make_speak(x) dispatches x.speak() through the object's type
 *      table. Dog and Cat share no base class, yet both work - "if it quacks..."
 *   3. EAFP: require_quack(duck) attempts .quack() (the field is there, so it
 *      succeeds). try_quack(dog) looks first and handles the missing field.
 *      Exceptions themselves are demonstrated in examples/ex9_exceptions; this tour only
 *      shows that choice.
 *
 * Each step prints a line proving it ran; the program exits 0.
 */

#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "gc.h"
#include "py_funcs.h"

/* Generate the Python feature tour. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    char fname[64];
    snprintf(fname, sizeof fname, "py_output_%s.asm", GC_OPTION_STR);
    c_prog_begin(fname);
    py_init();

    PyConsole con = py_console_declare();
    gc_externs();                       /* declare VirtualAlloc for the collector */

        PyStr sBanner = py_line("p_banner", "[Python] feature tour");
        PyStr sRcHdr  = py_line("p_rchdr",  "  refcount: o = obj()  # rc = 1");
        PyStr sRcInc  = py_line("p_rcinc",  "  b = o                # incref, rc = 2");
        PyStr sRcAlive= py_line("p_rcalive","  del b                # decref, rc = 1 (still alive)");
        PyStr sRcFreed= py_line("p_rcfreed","  del o                # decref, rc = 0 -> __del__, freed");
        PyStr sDuckHdr= py_line("p_duckhdr","  duck typing: make_speak(dog); make_speak(cat)");
        PyStr sWoof   = py_line("p_woof",   "    Dog.speak(): Woof");
        PyStr sMeow   = py_line("p_meow",   "    Cat.speak(): Meow");
        PyStr sEafpHdr= py_line("p_eafphdr","  EAFP: require_quack(duck); try_quack(dog)");
        PyStr sQuack  = py_line("p_quack",  "    Duck.quack(): Quack");
        PyStr sTypeErr= py_line("p_typeerr","    TypeError: 'Dog' object has no attribute 'quack'");

        /* Member-field demo strings. */
        PyStr sCntHdr = py_line("p_cnthdr","  member field: c = Counter(); c.bump(); c.bump()");
        PyStr sCntHit = py_line("p_cnthit","    Counter.n reached 2 (self.n mutated across calls)");

        /* ---- GC tour data ----
         * A "Node" managed type: 16-byte GC header, one pointer field `next`,
         * one scalar payload word - total 32 bytes (computed by the builder). */
        PyStr sGcHdr  = py_line("p_gchdr",  "  gc [" GC_OPTION_NAME "] collector tour");
        PyStr sGcAlloc= py_line("p_gcalloc","    allocated a, b; a.next = b; live addresses:");
        PyStr sGcReuse= py_line("p_gcreuse","    dropped roots + collected; new object reuses freed space:");
        GcTypeDesc nodeDesc = gc_type_begin("Node");
            gc_field_ptr(&nodeDesc, "next");
            gc_field_word(&nodeDesc, "value");
        GcType Node = gc_type_end(&nodeDesc);

        py_console_globals();
        gc_data_globals();              /* heap base/cursor/free-list/roots/... */

        AsmExtern print = py_console_print(&con);

        /* Emit the whole collector runtime (start / alloc / hex / collect +
         * option-specific barriers) and wire up a heap handle. */
        GcHeap heap = gc_begin(print);

        /* Types Dog / Cat / Duck. Each `class` declares its methods; the builder
         * interns each method name to a shared table slot (speak -> 0,
         * quack -> 1) and bakes the per-type method table. Dog and Cat expose
         * only speak; Duck exposes both speak and quack. */
        py_class_begin("Dog");
            CFunc* dogSpeak = py_method_begin("speak");
                py_puts(&print, sWoof);
                c_ret(dogSpeak);
            py_method_end();
        PyClass Dog = py_class_end();

        py_class_begin("Cat");
            CFunc* catSpeak = py_method_begin("speak");
                py_puts(&print, sMeow);
                c_ret(catSpeak);
            py_method_end();
        PyClass Cat = py_class_end();
        (void)catSpeak;

        py_class_begin("Duck");
            CFunc* duckSpeak = py_method_begin("speak");
                py_puts(&print, sQuack);   /* not used in this tour, but a valid slot */
                c_ret(duckSpeak);
            py_method_end();
            CFunc* duckQuack = py_method_begin("quack");
                py_puts(&print, sQuack);
                c_ret(duckQuack);
            py_method_end();
        PyClass Duck = py_class_end();
        (void)dogSpeak; (void)duckSpeak; (void)duckQuack;

        /* Counter: a class with a member field n.
         * bump() does self.n += 1; report() prints only once n has reached 2,
         * proving the field persists across separate method calls. */
        py_class_begin("Counter");
            py_field("n");
            CFunc* cntBump = py_method_begin("bump");
                py_field_add_imm(py_this_field("n", C_INT), 1);
                c_ret(cntBump);
            py_method_end();
            CFunc* cntReport = py_method_begin("report");
                CIf reached = c_if_uge(py_this_field("n", C_INT),
                                       c_imm_n(2, C_INT, "n"));
                    py_puts(&print, sCntHit);
                c_end_if(&reached);
                c_ret(cntReport);
            py_method_end();
        PyClass Counter = py_class_end();

        /* Instances start with a refcount of 1 and a pointer to the type's
         * method table. Counter also reserves a slot for its field n. */
        /* Ref is an empty class. Its instance is the object the refcount
         * steps increment and decrement. */
        py_class_begin("Ref");
        PyClass Ref = py_class_end();

        PyObject dog     = py_construct(&Dog,     "dog");
        PyObject cat     = py_construct(&Cat,     "cat");
        PyObject duck    = py_construct(&Duck,    "duck");
        PyObject counter = py_construct(&Counter, "counter");
        PyObject refobj  = py_construct(&Ref,     "refobj");

        /* Reference-counting primitives. */
        AsmExtern incref, decref;
        py_emit_refcount_ops(&print, sRcAlive, sRcFreed, &incref, &decref);

        /* make_speak(x): duck-typed dispatch of `speak` (implicit self). */
        CFunc makeSpeak = c_begin_function("make_speak", (CParam[]){ {"self", C_PTR} }, 1, C_PTR, NULL);
            py_dispatch_name(c_param(&makeSpeak, 0), "speak");
            c_ret(&makeSpeak);
        c_end_function(&makeSpeak);

        /* Exceptions themselves are demonstrated in examples/ex9_exceptions, so this
         * tour only shows the choice between handling a missing field and
         * failing on it. */
        /* require_quack(x): attempt the use. Duck has quack, so this succeeds.
         * A missing field prints TypeError and exits nonzero. */
        CFunc requireQuack = c_begin_function("require_quack", (CParam[]){ {"self", C_PTR} }, 1, C_PTR, NULL);
            py_dispatch_required_name(c_param(&requireQuack, 0), "quack",
                                      &print, sTypeErr, &con);
            c_ret(&requireQuack);
        c_end_function(&requireQuack);

        /* try_quack(x): look before the use. Dog has no quack; the tour
         * handles that arm and does not exit. One return: both arms share it. */
        CFunc tryQuack = c_begin_function("try_quack", (CParam[]){ {"self", C_PTR} }, 1, C_PTR, NULL);
            CElse noQuack = py_dispatch_checked_name(c_param(&tryQuack, 0), "quack");
            c_else(&noQuack);
                py_puts(&print, sTypeErr);   /* absent: raise TypeError */
            c_end_if_else(&noQuack);
            c_ret(&tryQuack);
        c_end_function(&tryQuack);

        /* ---- main ---- */
        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            py_puts(&print, sBanner);

            /* 1. Reference counting. */
            py_puts(&print, sRcHdr);
            py_puts(&print, sRcInc);
            py_incref(&incref, refobj);   /* rc 1 -> 2 */
            py_decref(&decref, refobj);   /* rc 2 -> 1: alive */
            py_decref(&decref, refobj);   /* rc 1 -> 0: freed */

            /* 2. Duck typing (implicit self). */
            py_puts(&print, sDuckHdr);
            py_send(&makeSpeak, dog);
            py_send(&makeSpeak, cat);

            /* 3. EAFP: required on the duck (field present); checked on the dog. */
            py_puts(&print, sEafpHdr);
            py_send(&requireQuack, duck);
            py_send(&tryQuack, dog);

            /* 3b. Member field mutated across two bump() calls, then reported. */
            py_puts(&print, sCntHdr);
            py_send(cntBump,   counter);
            py_send(cntBump,   counter);
            py_send(cntReport, counter);

            /* 4. Garbage collection (option chosen by -DGC_OPTION). This body is
             *    the shared, collector-agnostic gc_demo(): it compiles against
             *    every option through the common gc_* interface. */
            gc_demo(&print, &heap, &Node,
                    gc_msg_of(sGcHdr),
                    gc_msg_of(sGcAlloc),
                    gc_msg_of(sGcReuse));

            py_exit(&con, "0");
        c_end_function(&mainf);

    return c_prog_end();
}
