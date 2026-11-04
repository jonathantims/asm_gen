/*
 * csharp_main.c - A C# feature tour emitted through csharp_funcs.h.
 *
 * Generates csharp_output.asm, a console program demonstrating:
 *
 *   1. Virtual dispatch via the MethodTable: describe(Animal) reaches
 *      Animal.Speak or Dog.Speak purely from the object's MethodTable pointer.
 *   2. Boxing + unboxing with a runtime type check: unboxing the boxed int as
 *      `int` succeeds; unboxing it as `string` is caught as InvalidCastException
 *      by a runtime branch on the box's type handle. The same check is the
 *      runtime MethodTable: an Animal-typed reference whose table is Dog$vtable
 *      downcasts to Dog; a real Animal does not. Checks happen at run time,
 *      not generation time.
 *   3. using / IDisposable: Dispose() runs deterministically at scope exit.
 *
 * Each step prints a line proving it ran; the program exits 0.
 */

#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "gc.h"
#include "csharp_funcs.h"

/* The generator emits the C# feature tour (MethodTable dispatch, unbox,
 * using, and the selected collector). */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    char fname[64];
    snprintf(fname, sizeof fname, "csharp_output_%s.asm", GC_OPTION_STR);
    c_prog_begin(fname);
    cs_init();

    CsConsole con = cs_console_declare();
    gc_externs();                       /* declare VirtualAlloc for the collector */

        CsStr sBanner  = cs_line("s_banner", "[C#] feature tour");
        CsStr sAnimal  = cs_line("s_animal", "  Animal.Speak(): ...");
        CsStr sDog     = cs_line("s_dog", "  Dog.Speak(): Woof (MethodTable dispatch)");
        CsStr sUnboxOk = cs_line("s_unbox_ok", "  unbox<int> OK: box holds an Int32");
        CsStr sBoxPay  = cs_line("s_boxpay", "  box payload 42");
        CsStr sUnboxBad= cs_line("s_unbox_bad", "  InvalidCastException caught: box is int, not string (runtime check)");
        CsStr sCastOk  = cs_line("s_cast_ok", "  downcast to Dog accepted (Animal reference, Dog MethodTable)");
        CsStr sCastBad = cs_line("s_cast_bad", "  downcast to Dog rejected (runtime check)");
        CsStr sOpened  = cs_line("s_opened", "  FileStream opened (using)");
        CsStr sDisposed= cs_line("s_disposed", "  Dispose() at end of using block");
        /* Reached only when a runtime check takes the wrong arm. */
        CsStr sNever1  = cs_line("s_never1", "  (unreachable) unbox<int> failed");
        CsStr sNever2  = cs_line("s_never2", "  (unreachable) unbox<string> OK");
        CsStr sNever3  = cs_line("s_never3", "  (unreachable) downcast check disagreed");

        /* ---- GC tour strings, plus CLI type handles / Node / Box ---- */
        CsStr sGcHdr  = cs_line("cs_gchdr", "  gc [" GC_OPTION_NAME "] collector tour");
        CsStr sGcAlloc= cs_line("cs_gcalloc", "    allocated a, b; a.next = b; live addresses:");
        CsStr sGcReuse= cs_line("cs_gcreuse", "    dropped roots + collected; new object reuses freed space:");
        CsCli cli = cs_cli_init();

        cs_console_globals();
        gc_data_globals();

        AsmExtern print = cs_console_print(&con);
        GcHeap heap = gc_begin(print);   /* emit the collector runtime */

        /* Animal: a base class whose Speak() is virtual (MethodTable slot 0).
         * Dog derives and overrides it. The builder bakes each type's
         * MethodTable ("Animal$vtable", "Dog$vtable") and the slot numbering. */
        cs_class_begin("Animal");
            CFunc* animalSpeak = cs_virtual_begin("Speak");
                cs_puts(&print, sAnimal);
                c_ret(animalSpeak);
            cs_method_end();
        CsClass Animal = cs_class_end();

        cs_class_begin("Dog");
        cs_extends(&Animal);
            CFunc* dogSpeak = cs_virtual_begin("Speak");
                cs_puts(&print, sDog);
                c_ret(dogSpeak);
            cs_method_end();
        CsClass Dog = cs_class_end();

        /* describe(Animal): a virtual call; it knows nothing of the concrete
         * type, dispatching Speak through the receiver's MethodTable. */
        CFunc describe = c_begin_function("describe", (CParam[]){ {"this", C_PTR} }, 1, C_PTR, NULL);
            cs_vsend(c_param(&describe, 0), &Animal, "Speak");
            c_ret(&describe);
        c_end_function(&describe);

        /* FileStream: a class with a ctor, Dispose, and a member field `open`
         * (no virtuals, so no MethodTable). The ctor sets this->open = 1 and
         * Dispose reads it back, proving the field round-trips through the
         * object. `using` calls Dispose() deterministically at scope end. */
        cs_class_begin("FileStream");
            cs_field("open");                      /* 1 = handle is open */
            CFunc* fileOpen = cs_method_begin("ctor");
                cs_puts(&print, sOpened);
                cs_field_store_imm(cs_this_field("open", C_INT), "1");
                c_ret(fileOpen);
            cs_method_end();
            CFunc* fileDispose = cs_method_begin("Dispose");
                /* Only report a real close when the field says we were open. */
                CIf wasOpen = c_if_uge(cs_this_field("open", C_INT),
                                       c_imm_n(1, C_INT, "open"));
                    cs_puts(&print, sDisposed);
                c_end_if(&wasOpen);
                c_ret(fileDispose);
            cs_method_end();
        CsClass FileStream = cs_class_end();

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            cs_puts(&print, sBanner);

            /* Instances and the box live on the GC heap. cs_construct calls
             * gc_new. The string is the pointer's dictionary tag, not a data
             * label. MethodTables are the globals; these objects are not.
             * gc_demo starts a fresh heap of its own later; these objects
             * are used first. */
            gc_start(&heap);
            CsObject animal = cs_construct(&heap, &Animal, "animal");
            CsObject dog    = cs_construct(&heap, &Dog, "dog");
            CsObject fileObj = cs_construct(&heap, &FileStream, "fileObj");
            CValue box = cs_box(&heap, &cli.box, "Int32_type", 42, "intBox");
            CValue boxed = cs_box_payload(box);
            CIf pay = c_if_eq(boxed, c_imm_n(42, C_LONG, "boxed"));
                cs_puts(&print, sBoxPay);
            c_end_if(&pay);

            /* 1. MethodTable dispatch on two dynamic types (implicit `this`). */
            cs_send(&describe, animal);
            cs_send(&describe, dog);

            /* Downcast dog to Dog (match) and call the virtual slot directly. */
            CElse asDog = cs_downcast_begin(dog.ptr, &Dog);
                cs_send(dogSpeak, dog);
            c_else(&asDog);
                cs_puts(&print, sNever3);
            c_end_if_else(&asDog);

            /* Same pointer, Animal-typed reference. [obj+16] is still
             * Dog$vtable, so the runtime check matches. */
            CValue asAnimal = dog.ptr;
            CElse poly = cs_downcast_begin(asAnimal, &Dog);
                cs_puts(&print, sCastOk);
            c_else(&poly);
                cs_puts(&print, sNever3);
            c_end_if_else(&poly);

            /* A real Animal is not a Dog, so that cast takes the mismatch arm. */
            CElse notDog = cs_downcast_begin(animal.ptr, &Dog);
                cs_puts(&print, sNever3);
            c_else(&notDog);
                cs_puts(&print, sCastBad);
            c_end_if_else(&notDog);

            /* 2a. Unbox as int -> type handle matches -> OK path. */
            CElse unboxInt = cs_unbox_begin(box, "Int32_type");
                cs_puts(&print, sUnboxOk);
            c_else(&unboxInt);
                cs_puts(&print, sNever1);   /* unreachable: box really is int */
            c_end_if_else(&unboxInt);

            /* 2b. Unbox as string -> mismatch -> InvalidCastException caught. */
            CElse unboxStr = cs_unbox_begin(box, "String_type");
                cs_puts(&print, sNever2);   /* unreachable: box is not string */
            c_else(&unboxStr);
                cs_puts(&print, sUnboxBad);
            c_end_if_else(&unboxStr);

            /* 3. using(var fs = new FileStream(...)) { } -> Dispose at scope end. */
            CsUsing u = cs_using_begin();
            cs_send(fileOpen, fileObj);
            cs_using_own(&u, fileDispose, fileObj);
            cs_using_end(&u);

            /* 4. Garbage collection (option chosen by -DGC_OPTION). */
            gc_demo(&print, &heap, &cli.node,
                    gc_msg_of(sGcHdr),
                    gc_msg_of(sGcAlloc),
                    gc_msg_of(sGcReuse));

            cs_exit(&con, "0");
        c_end_function(&mainf);

    return c_prog_end();
}
