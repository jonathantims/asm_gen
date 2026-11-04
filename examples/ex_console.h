/*
 * ex_console.h - a tiny console runtime shared by every example in this folder.
 *
 * The examples/ programs are all built directly on the C interface
 * (c_funcs.h / asm_funcs.h) and each demonstrate a *generation-time* debugging
 * feature that weaves extra code into the emitted program only when a global
 * flag is set (and leaves zero footprint when it is not). To keep every example
 * focused on its instrumentation idea, the boring plumbing - declaring the
 * three kernel32 imports, a print procedure, a decimal number printer and an
 * exit helper - lives here and is reused verbatim.
 *
 * This mirrors the per-language console helpers (cpp_console_* etc.) but stands
 * on its own so the examples do not depend on any language wrapper.
 */



/* String and file helpers live in host.h (snprintf, memcpy, strcmp, strstr).
 * This header does not include the C runtime headers. */

/* The three kernel32 imports every example uses. */
typedef struct {
    AsmExtern get_std_handle;   /* GetStdHandle(nStdHandle) -> handle           */
    AsmExtern write_file;       /* WriteFile(handle, buf, n, &written, 0) -> ok */
    AsmExtern exit_process;     /* ExitProcess(code)                            */
} ExConsole;

/* A console string: its address ("offset X") and byte length ("$ - X"). */
typedef struct { AsmDataField off; AsmDataField len; } ExStr;

/* Declare the kernel32 imports. Call once, right after ex-/language init. */
static inline ExConsole ex_console_declare(void) {
    ExConsole c;
    static const char* gsh[] = { "nStdHandle" };
    c.get_std_handle = c_extern("GetStdHandle", gsh, 1, C_PTR, "handle");
    static const char* wf[] = { "handle", "lpBuffer", "nBytes", "lpWritten", "lpOverlapped" };
    c.write_file = c_extern("WriteFile", wf, 5, C_INT, "ok");
    static const char* ep[] = { "exit_code" };
    c.exit_process = c_extern("ExitProcess", ep, 1, C_PTR, NULL);
    return c;
}

/* Emit the scratch globals used by the console: the WriteFile out-parameter and
 * a 24-byte buffer the decimal printer formats into. Call inside write_dataq(). */
static inline void ex_console_globals(void) {
    c_ensure_bss();
    write_unitialised_data("ex_written", "?");
    write_db("ex_numbuf", asm_dup(24, "?"));
}

/* Define a string named name from text_literal in the current write_data()
 * section and return address+length cells tagged to match the print
 * procedure's parameters. */
static inline ExStr ex_string(const char* name, const char* text_literal) {
    c_ensure_data();
    char offname[64], lenname[64];
    snprintf(offname, sizeof(offname), "off_%s", name);
    snprintf(lenname, sizeof(lenname), "len_%s", name);
    AsmDataField d = write_initialised_data(name, text_literal, "lpBuffer");
    ExStr s;
    s.off = write_offset_constant(c_strdup_arena(offname), d, "lpBuffer");
    char lenexpr[80];
    snprintf(lenexpr, sizeof(lenexpr), "$ - %s", name);
    s.len = write_constant(c_strdup_arena(lenname), c_strdup_arena(lenexpr), "nBytes");
    return s;
}

/* C string as a console line. nl is 0, 1 (LF), or 2 (CR LF). nul appends 0.
 * The MASM byte list stays in c_db_text. */
static inline ExStr ex_line(const char* name, const char* text, int nl, int nul) {
    return ex_string(name, c_db_text(text, nl, nul));
}

/* Emit print(buffer, length) -> WriteFile to stdout. The body is a c function.
 * GetStdHandle and WriteFile are calls, and a call saves the named argument
 * registers, so the buffer and the length do not need a manual spill.
 * Call inside write_code() before any caller. */
static inline AsmExtern ex_console_print(ExConsole* c) {
    const CParam params[] = { { "lpBuffer", C_PTR }, { "nBytes", C_PTR } };
    CFunc print = c_begin_function("print", params, 2, C_PTR, NULL);
        CValue buf = c_param(&print, 0);
        CValue len = c_param(&print, 1);
        CValue handle = c_call_ext(C_PTR, &c->get_std_handle, literal("-11", "nStdHandle"));
        c_call_ext_void(&c->write_file,
            c_cell(handle), c_cell(buf), c_cell(len),
            literal("offset ex_written", "lpWritten"),
            literal("0", "lpOverlapped"));
        c_ret(&print);
    c_end_function(&print);
    return print.ext;
}

/* Emit print_u64(rcx=value): format an unsigned 64-bit value as decimal into
 * ex_numbuf and print it (no newline). `print` is the procedure from
 * ex_console_print. Returns an AsmExtern callable with a single "value" cell.
 * The digit loop is DIV by 10. c_funcs.h has no divide, so this one loop stays
 * at the asm layer and then calls the c-level print with rcx/rdx set. */
static inline AsmExtern ex_print_u64_def(AsmExtern* print) {
    static const AsmDataField pr[] = { {"rcx","value",NULL} };
    AsmExtern pu = write_proc("print_u64", pr, 1, asm_void);
        write_push("rbx");
        write_push("rsi");
        write_push("rdi");
        write_replace_mov("rax", (AsmDataField){ "rcx", "value", NULL });
        write_lea("rsi", "ex_numbuf");
        write_add((AsmDataField){ "rsi", "ptr", NULL },
                  (AsmDataField){ "24", "end", NULL });
        write_replace_mov("rbx", (AsmDataField){ "10", "ten", NULL });
        write_test("rax", "rax");
        write_jnz("Lpu_loop");
        /* value == 0: emit a single '0' */
        write_dec("rsi");
        write_replace_mov(asm_mem("byte", "rsi", 0), (AsmDataField){ "'0'", "digit", NULL });
        write_jmp("Lpu_done");
    write_label("Lpu_loop");
        write_test("rax", "rax");
        write_jz("Lpu_done");
        write_xor((AsmDataField){"rdx","rdx",NULL}, (AsmDataField){"rdx","rdx",NULL});
        write_div("rbx");                          /* rax = rax/10, rdx = rax%10 */
        write_add((AsmDataField){ "dl", "digit", NULL },
                  (AsmDataField){ "'0'", "ascii", NULL });
        write_dec("rsi");
        write_replace_mov(asm_mem("byte", "rsi", 0), (AsmDataField){ "dl", "digit", NULL });
        write_jmp("Lpu_loop");
    write_label("Lpu_done");
        /* rcx = pointer (rsi); edx = length = (numbuf+24) - rsi */
        write_replace_mov("rcx", (AsmDataField){ "rsi", "lpBuffer", NULL });
        write_lea("rax", "ex_numbuf");
        write_add((AsmDataField){ "rax", "end", NULL },
                  (AsmDataField){ "24", "end", NULL });
        write_sub((AsmDataField){ "rax", "nBytes", NULL },
                  (AsmDataField){ "rsi", "ptr", NULL });
        write_replace_mov("rdx", (AsmDataField){ "rax", "nBytes", NULL });
        write_call_symbol(print->name);          /* rcx/rdx already set up */
        write_pop("rdi");
        write_pop("rsi");
        write_pop("rbx");
        write_ret();
    write_endp(&pu);
    return pu;
}

/* print(s). Goes through c_call so a live this or argument register is
 * saved across the call and restored for the next line. */
static inline void ex_puts(AsmExtern* print, ExStr s) {
    c_call_proc_void(print, s.off, s.len);
}

/* Print v with print_u64. A 4-byte value is widened. Any other width is
 * passed through with the tag print_u64 checks ("value"). */
static inline void ex_print_val(AsmExtern* pu64, CValue v) {
    v = c_resolve(v);
    if (v.type.size == 4) {
        CValue wide = c_zext("value", v);
        c_call_proc_void(pu64, c_cell(wide));
        c_free(&wide);
        return;
    }
    c_call_proc_void(pu64, c_treat_as(v, v.tag, "value"));
}

/* print_u64(value literal or register cell). */
static inline void ex_put_u64(AsmExtern* print_u64, AsmDataField value) {
    write_call(print_u64, value);
}

/* ExitProcess(code). code is the numeric status. */
static inline void ex_exit(ExConsole* c, long long code) {
    c_call_ext_void(&c->exit_process, c_lit_n(code, "exit_code"));
}

