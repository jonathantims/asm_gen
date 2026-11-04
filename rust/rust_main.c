/*
 * rust_main.c - A Rust feature tour emitted through rust_funcs.h.
 *
 * Generates rust_output.asm, a console program demonstrating:
 *
 *   1. Ownership & MOVE semantics checked at GENERATION time: `buf` is moved
 *      into consume(); consume takes ownership and drops it. Because the move
 *      invalidated `buf` at generation time, end-of-scope does NOT drop it again
 *      (no double free). A use-after-move would abort code generation - see the
 *      commented rust_use(&buf) below.
 *   2. Deterministic DROP in LIFO order: values still owned at scope end drop in
 *      reverse; `keep` drops, moved-out `buf` is skipped.
 *   3. Result<T,E> + the `?` operator: checked_div(10,2)? yields Ok and continues;
 *      checked_div(1,0)? yields Err and `?` early-returns from run().
 *
 * Each step prints a line proving it ran; the program exits 0.
 */

#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "gc.h"
#include "rust_funcs.h"

/* Generate the Rust feature tour. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    char fname[64];
    snprintf(fname, sizeof fname, "rust_output_%s.asm", GC_OPTION_STR);
    c_prog_begin(fname);
    rust_init();

    RsConsole con = rs_console_declare();
    gc_externs();                       /* declare VirtualAlloc for the collector */

        RsStr sBanner  = rs_line("rs_banner",  "[Rust] feature tour");
        RsStr sLet     = rs_line("rs_let",     "  let buf = vec![..]; let keep = vec![..];");
        RsStr sMoveIn  = rs_line("rs_movein",  "  move buf into consume():");
        RsStr sConsume = rs_line("rs_consume", "    consume(buf): took ownership of buf");
        RsStr sDropBuf = rs_line("rs_dropbuf", "    drop(buf): freed (inside consume)");
        RsStr sScope   = rs_line("rs_scope",   "  end of scope, LIFO drop of values still owned:");
        RsStr sDropKeep= rs_line("rs_dropkeep","    drop(keep): freed");
        RsStr sDivOk   = rs_line("rs_divok",   "  checked_div(10,2)? = Ok, continue");
        RsStr sDivBad  = rs_line("rs_divbad",  "  checked_div(1,0)? = Err, ? early-returns from run()");
        RsStr sNever   = rs_line("rs_never",   "    (unreachable) checked_div(1,0) = Ok");

        /* ---- GC tour data ---- */
        RsStr sGcHdr  = rs_line("rs_gchdr",  "  gc [" GC_OPTION_NAME "] collector tour");
        RsStr sGcAlloc= rs_line("rs_gcalloc","    allocated a, b; a.next = b; live addresses:");
        RsStr sGcReuse= rs_line("rs_gcreuse","    dropped roots + collected; new object reuses freed space:");
        GcTypeDesc nodeDesc = gc_type_begin("Node");
            gc_field_ptr(&nodeDesc, "next");
            gc_field_word(&nodeDesc, "value");
        GcType Node = gc_type_end(&nodeDesc);

        rs_console_globals();
        rust_reserve("buf");
        rust_reserve("keep");
        gc_data_globals();

        AsmExtern print = rs_console_print(&con);
        GcHeap heap = gc_begin(print);   /* emit the collector runtime */

        /* Drop::drop implementations. */
        CFunc bufDrop = rs_fn_begin("Buf_drop");
            rs_puts(&print, sDropBuf);
            c_ret(&bufDrop);
        rs_fn_end(&bufDrop);

        CFunc keepDrop = rs_fn_begin("Keep_drop");
            rs_puts(&print, sDropKeep);
            c_ret(&keepDrop);
        rs_fn_end(&keepDrop);

        /* fn consume(buf: Vec) - the parameter is dropped by the scope. */
        CFunc consume = rs_fn_begin("consume");
            rs_puts(&print, sConsume);
            RustScope owned = rust_scope_begin();
            RustVal taken;
            rust_bind(&owned, &taken, "buf", c_param(&consume, 0), &bufDrop);
            rust_scope_end(&owned);
            c_ret(&consume);
        rs_fn_end(&consume);

        /* fn checked_div(a, b) -> Result. The value is what `?` inspects. */
        const CParam divp[] = { {"a", C_PTR}, {"b", C_PTR} };
        CFunc checkedDiv = c_begin_function("checked_div", divp, 2, C_PTR, "is_err");
            CElse divByZero = c_if_eq_else(c_param(&checkedDiv, 1), c_imm_n(0, C_PTR, "b"));
                rust_ret_err(&checkedDiv);
            c_else(&divByZero);
                rust_ret_ok(&checkedDiv);
            c_end_if_else(&divByZero);
        c_end_function(&checkedDiv);

        /* fn run() -> Result. Two uses of `?`. */
        CFunc run = c_begin_function("run", NULL, 0, C_PTR, NULL);
            RustCatch cat = rust_catch_begin();
                CValue ok = c_call_proc(C_PTR, &checkedDiv.ext, c_lit_n(10, "a"), c_lit_n(2, "b"));
                rust_try_result(&cat, ok);
                rs_puts(&print, sDivOk);

                CValue err = c_call_proc(C_PTR, &checkedDiv.ext, c_lit_n(1, "a"), c_lit_n(0, "b"));
                rust_try_result(&cat, err);
                rs_puts(&print, sNever);
            rust_catch(&cat);
                rs_puts(&print, sDivBad);    /* `?` early-returns here from run() */
            rust_catch_end(&cat);
            c_ret(&run);
        c_end_function(&run);

        /* ---- fn main() ---- */
        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            rs_puts(&print, sBanner);

            RustScope scope = rust_scope_begin();
            RustVal buf, keep;
            rust_let_owned(&scope, &buf, "buf", &bufDrop);
            rust_let_owned(&scope, &keep, "keep", &keepDrop);
            rs_puts(&print, sLet);

            /* Move `buf` into consume(): ownership transfers. */
            rs_puts(&print, sMoveIn);
            c_call(&consume, rust_move(&buf));

            /*
             * Uncommenting the next line aborts code generation with
             *   error[E0382]: use of moved value `buf`
             * exactly like the Rust borrow checker:
             *
             *   c_call(&consume, rust_use(&buf));
             */

            /* End of scope: drop still-owned values (LIFO). buf is skipped. */
            rs_puts(&print, sScope);
            rust_scope_end(&scope);

            /* Result / `?` demonstration. */
            c_call(&run);

            /* 4. Garbage collection (option chosen by -DGC_OPTION). */
            gc_demo(&print, &heap, &Node,
                    gc_msg_of(sGcHdr),
                    gc_msg_of(sGcAlloc),
                    gc_msg_of(sGcReuse));

            rs_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
