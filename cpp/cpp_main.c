/*
 * cpp_main.c - A C++ feature tour emitted through cpp_funcs.h.
 *
 * Generates cpp_output.asm, a console program. The tour prints a banner and
 * exits 0. Classes, virtual calls, RAII, placement, a stack reservation,
 * operator+, and one greet body instantiated as int and as long come from
 * cpp_funcs.h.
 */

#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "cpp_funcs.h"

typedef struct {
    AsmExtern* print;
    CppStr as_int;
    CppStr as_long;
} CppGreet;

/* One greet body. The type argument chooses which line is printed. */
static void cpp_greet_body(CFunc* fn, const char* type, void* user) {
    CppGreet* g = (CppGreet*)user;
    if (strcmp(type, "int") == 0) cpp_puts(g->print, g->as_int);
    else if (strcmp(type, "long") == 0) cpp_puts(g->print, g->as_long);
    else cpp_fail("greet: type is not int or long");
    c_ret(fn);
}

/* Emit the C++ feature tour. The generated program demonstrates
 * virtual dispatch, a second interface vptr, RAII scope exit, placement,
 * a stack object, operator+, two monomorphised greetings, and
 * a member field updated through this. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("cpp_output.asm");
    cpp_init();

    CppConsole con = cpp_console_declare();

        CppStr sBanner = cpp_line("s_banner", "[C++] feature tour");
        CppStr sCircle = cpp_line("s_circle", "  Circle::draw() dispatched via vtable");
        CppStr sSquare = cpp_line("s_square", "  Square::draw() dispatched via vtable");
        CppStr sNamed  = cpp_line("s_named",  "  Circle::label() via the second interface vptr");
        CppStr sGuardC = cpp_line("s_guard_c", "  Guard() constructed");
        CppStr sGuardD = cpp_line("s_guard_d", "  ~Guard() destroyed at scope exit (RAII)");
        CppStr sOpAdd  = cpp_line("s_op_add", "  Vec2::operator+ (operator overloading)");
        CppStr sGInt   = cpp_line("s_g_int", "  greet<int> (template monomorphisation)");
        CppStr sGLong  = cpp_line("s_g_long", "  greet<long> (template monomorphisation)");
        CppStr sCount  = cpp_line("s_count", "  Counter::value reached 2 (member field mutated via this)");

        cpp_console_globals();

        AsmExtern print = cpp_console_print(&con);

        /* Shape fixes slot 0 = draw. Named fixes slot 0 = label. They are
         * separate vtables. cpp_implements, called inside Circle below, is
         * what tells the generator about the relationship: it copies those
         * slot numbers and reserves one vptr per interface. There is no
         * base-clause on the class for the generator to read. */
        cpp_class_begin("Shape");
            cpp_class_interface();
            cpp_virtual_decl("draw");
        CppClass Shape = cpp_class_end();

        cpp_class_begin("Named");
            cpp_class_interface();
            cpp_virtual_decl("label");
        CppClass Named = cpp_class_end();

        cpp_class_begin("Circle");
            cpp_implements(&Shape);
            cpp_implements(&Named);
            CFunc* circleDraw = cpp_virtual_begin("draw");
                cpp_puts(&print, sCircle);
                c_ret(circleDraw);
            cpp_method_end();
            CFunc* circleLabel = cpp_virtual_begin("label");
                cpp_puts(&print, sNamed);
                c_ret(circleLabel);
            cpp_method_end();
        CppClass Circle = cpp_class_end();

        cpp_class_begin("Square");
            cpp_implements(&Shape);
            CFunc* squareDraw = cpp_virtual_begin("draw");
                cpp_puts(&print, sSquare);
                c_ret(squareDraw);
            cpp_method_end();
        CppClass Square = cpp_class_end();

        /* "circle" is the data label of this instance. It is the symbol
         * the later send uses as the object's address. It is not the class
         * name and not a constructor argument. "square" is a second label. */
        CppObject circle = cpp_construct(&Circle, "circle");
        CppObject square = cpp_construct(&Square, "square");

        CFunc render = cpp_begin_function("render", (CParam[]){ { "this", C_PTR } }, 1, C_PTR, NULL);
            cpp_vsend(c_param(&render, 0), &Shape, "draw");
            c_ret(&render);
        cpp_end_function(&render);

        cpp_class_begin("Guard");
            CFunc* guardCtor = cpp_ctor_begin();
                cpp_puts(&print, sGuardC);
                c_ret(guardCtor);
            cpp_method_end();
            CFunc* guardDtor = cpp_dtor_begin();
                cpp_puts(&print, sGuardD);
                c_ret(guardDtor);
            cpp_method_end();
        CppClass Guard = cpp_class_end();

        /* operator+ is a method the header opens and calls. The body adds 1
         * to x so two zero vectors still take the tour's print. */
        cpp_class_begin("Vec2");
            cpp_field("x", C_PTR);
            cpp_field("y", C_PTR);
            CFunc* vecAdd = cpp_operator_begin("+");
                CValue x = cpp_this_field("x", C_PTR);
                c_add(&x, c_imm_n(1, C_PTR, "x"));
                c_ret(vecAdd);
            cpp_method_end();
        CppClass Vec2 = cpp_class_end();
        (void)vecAdd;

        cpp_class_begin("Counter");
            cpp_field("value", C_PTR);
            CFunc* counterBump = cpp_method_begin("bump");
                CValue bv = cpp_this_field("value", C_PTR);
                c_add(&bv, c_imm_n(1, C_PTR, "value"));
                c_ret(counterBump);
            cpp_method_end();
            CFunc* counterReport = cpp_method_begin("report");
                CValue rv = cpp_this_field("value", C_PTR);
                CIf reached = c_if_uge(rv, c_imm_n(2, C_PTR, "value"));
                    cpp_puts(&print, sCount);
                c_end_if(&reached);
                c_ret(counterReport);
            cpp_method_end();
        CppClass Counter = cpp_class_end();

        CppGreet greet;
        greet.print = &print;
        greet.as_int = sGInt;
        greet.as_long = sGLong;
        CFunc greetInt = cpp_instantiate("greet", "int", cpp_greet_body, &greet);
        CFunc greetLong = cpp_instantiate("greet", "long", cpp_greet_body, &greet);

        CFunc mainf = cpp_begin_function("main", NULL, 0, C_INT, "exit_code");
            cpp_puts(&print, sBanner);

            cpp_send(&render, circle);
            cpp_send(&render, square);
            cpp_vsend_obj(circle, &Named, "label");

            CppScope scope = cpp_scope_begin();
                cpp_construct(&Guard, "guard_obj");
                /* Reservation, then placement over that storage. Heap allocation
                 * is the gc/ collectors, not this layer. */
                cpp_reserve(&Guard, "guard_slot");
                cpp_emplace(&Guard, "guard_slot");
                cpp_stack_construct(&Guard);
            cpp_scope_end(&scope);

            CppObject vecA = cpp_construct(&Vec2, "vec_a");
            CppObject vecB = cpp_construct(&Vec2, "vec_b");
            cpp_add(vecA, vecB);
            CValue sum = cpp_field_ref(&vecA, "x", C_PTR);
            CIf show = c_if_uge(sum, c_imm_n(1, C_PTR, "x"));
                cpp_puts(&print, sOpAdd);
            c_end_if(&show);

            c_call(&greetInt);
            c_call(&greetLong);

            CppObject counter = cpp_construct(&Counter, "counter");
            cpp_send(counterBump, counter);
            cpp_send(counterBump, counter);
            cpp_send(counterReport, counter);

            cpp_exit(&con, 0);
        cpp_end_function(&mainf);

    return c_prog_end();
}
