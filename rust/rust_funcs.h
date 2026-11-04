
/*
 * rust_funcs.h - A Rust-flavoured code-generation layer over c_funcs.h / asm_funcs.h.
 *
 * Models three defining Rust ideas:
 *   - Ownership & MOVE semantics, checked at GENERATION time. Each owned value
 *     has a `moved` flag; rust_move() invalidates the source, and rust_use()
 *     aborts code generation on a use-after-move - reproducing the borrow
 *     checker as a build-time error (the Rust teaching point).
 *   - Deterministic DROP: values still owned at scope end are dropped in LIFO
 *     order; moved-out values are skipped (no double free).
 *   - Result<T,E> and the `?` operator: a fallible call returns a discriminant;
 *     `?` early-returns on Err.
 *
 * SELF-CONTAINED: duplicates a small console prelude rather than sharing one.
 *
 * See rust_interface.md for the full narrative.
 */


/* ======================================================================== *
 *  Console runtime  (self-contained)
 * ======================================================================== */

typedef struct {
    AsmExtern get_std_handle;
    AsmExtern write_file;
    AsmExtern exit_process;
} RsConsole;

typedef struct { AsmDataField off; AsmDataField len; } RsStr;

/* Declare GetStdHandle, WriteFile, and ExitProcess for the Rust console. */
static inline RsConsole rs_console_declare(void) {
    RsConsole c;
    static const char* gsh[] = { "nStdHandle" };
    c.get_std_handle = c_extern("GetStdHandle", gsh, 1, C_PTR, "handle");
    static const char* wf[] = { "handle", "lpBuffer", "nBytes", "lpWritten", "lpOverlapped" };
    c.write_file = c_extern("WriteFile", wf, 5, C_INT, "ok");
    static const char* ep[] = { "exit_code" };
    c.exit_process = c_extern("ExitProcess", ep, 1, C_PTR, NULL);
    return c;
}

/* Reserve the WriteFile byte-count global in uninitialised data. */
static inline void rs_console_globals(void) {
    c_ensure_bss();
    write_unitialised_data("written", "?");
}

/* Define string name from a MASM db operand and return its address and length. */
static inline RsStr rs_string(const char* name, const char* text_literal) {
    CStr s = c_string(name, text_literal, "lpBuffer", "nBytes");
    RsStr o;
    o.off = s.off;
    o.len = s.len;
    return o;
}

/* Address and length of a C string plus CRLF. */
static inline RsStr rs_line(const char* name, const char* text) {
    return rs_string(name, c_db_text(text, 2, 0));
}

/* Emit print, which writes a buffer using the console externs in c. */
static inline AsmExtern rs_console_print(RsConsole* c) {
    static const AsmDataField pr[] = { {"rcx","lpBuffer",NULL}, {"edx","nBytes",NULL} };
    AsmExtern print = write_proc("print", pr, 2, asm_void);
        write_push("rsi");
        write_push("rdi");
        write_replace_mov("rsi", (AsmDataField){ "rcx", "lpBuffer", NULL });
        write_replace_mov("edi", (AsmDataField){ "edx", "nBytes", NULL });
        write_extern_call(&c->get_std_handle, literal("-11", "nStdHandle"));
        write_extern_call(&c->write_file,
            (AsmDataField){"rax","handle",NULL},
            (AsmDataField){"rsi","lpBuffer",NULL},
            (AsmDataField){"rdi","nBytes",NULL},
            literal("offset written","lpWritten"),
            literal("0","lpOverlapped"));
        write_pop("rdi");
        write_pop("rsi");
        write_ret();
    write_endp(&print);
    return print;
}

/* Call print with the address and length stored in s. */
static inline void rs_puts(AsmExtern* print, RsStr s) { write_call(print, s.off, s.len); }
/* Exit the generated program through console c with status code. */
static inline void rs_exit(RsConsole* c, long long code) {
    c_call_ext_void(&c->exit_process, c_lit_n(code, "exit_code"));
}

/* Open method or free fn name with the receiver in rcx (param 0). */
static inline CFunc rs_fn_begin(const char* name) {
    const CParam p[] = { {"self", C_PTR} };
    return c_begin_function(name, p, 1, C_PTR, NULL);
}
/* Close function f. */
static inline void rs_fn_end(CFunc* f) { c_end_function(f); }

/* ======================================================================== *
 *  Ownership, moves, and Drop  (generation-time borrow checking)
 * ======================================================================== */

/* An owned value. `moved` is tracked entirely at generation time. */
typedef struct {
    const char*      name;        /* variable name (for diagnostics)          */
    const char*      obj;         /* the value's storage symbol (bare name)    */
    const AsmExtern* drop;        /* its Drop::drop procedure                   */
    bool             moved;       /* true once moved out (source invalidated)   */
    bool             is_param;    /* bound from a parameter, not a bss symbol  */
    CValue           param;
} RustVal;

#define RUST_SCOPE_MAX 16
typedef struct { RustVal* owned[RUST_SCOPE_MAX]; int n; } RustScope;

/* Begin an empty ownership scope. */
static inline RustScope rust_scope_begin(void) { RustScope s; s.n = 0; return s; }

/* `let name = <owner>;` - bind an owned value and register it for Drop. `obj` is
 * the bare storage symbol (its address is formed internally on use/drop). `drop`
 * is the Drop::drop procedure (its `.ext` is taken internally). */
static inline void rust_let(RustScope* s, RustVal* out, const char* name,
                            const char* obj, const CFunc* drop) {
    out->name = name;
    out->obj = obj;
    out->drop = &drop->ext;
    out->moved = false;
    out->is_param = false;
    s->owned[s->n++] = out;
}

/* Storage for an owned object named name. The symbol is name_obj. */
static inline const char* rust_obj_sym(const char* name) {
    char sym[64];
    snprintf(sym, sizeof sym, "%s_obj", name);
    return c_strdup_arena(sym);
}

static inline void rust_reserve(const char* name) {
    c_ensure_bss();
    write_unitialised_data(rust_obj_sym(name), "?");
}

/* let name = owned object. rust_reserve(name) already emitted the storage. */
static inline void rust_let_owned(RustScope* s, RustVal* out, const char* name,
                                  const CFunc* drop) {
    rust_let(s, out, name, rust_obj_sym(name), drop);
}

/* Bind a parameter as an owned value so scope end drops it. */
static inline void rust_bind(RustScope* s, RustVal* out, const char* name,
                             CValue param, const CFunc* drop) {
    out->name = name;
    out->obj = NULL;
    out->drop = &drop->ext;
    out->moved = false;
    out->is_param = true;
    out->param = param;
    s->owned[s->n++] = out;
}

/*
 * rust_use - assert a value is still usable HERE (generation-time). Aborts code
 * generation on a use-after-move, exactly as rustc rejects the program. Returns
 * the value's storage cell so it can be passed to a call.
 */
static inline AsmDataField rust_use(RustVal* v) {
    if (v->moved) {
        char buf[256];
        snprintf(buf, sizeof buf, "rust_funcs: error[E0382]: use of moved value `%s`", v->name);
        host_fail(buf);
    }
    return c_addr(v->obj, "self");
}

/*
 * rust_move - move a value out of `src`. The source is invalidated (any later
 * rust_use aborts generation); the returned cell is the new owner's storage.
 */
static inline AsmDataField rust_move(RustVal* src) {
    AsmDataField cell = rust_use(src);   /* must be live to move */
    src->moved = true;
    return cell;
}

/* Drop everything still owned, in reverse (LIFO) order; skip moved-out values. */
static inline void rust_scope_end(RustScope* s) {
    for (int i = s->n - 1; i >= 0; i--) {
        RustVal* v = s->owned[i];
        if (!v->moved) {
            if (v->is_param) write_call(v->drop, c_cell(v->param));
            else             write_call(v->drop, c_addr(v->obj, "self"));
        }
    }
}

/* ======================================================================== *
 *  Result<T,E> and the `?` operator
 * ======================================================================== */

/*
 * A Result-producing function leaves its discriminant in rax (0 = Ok, 1 = Err).
 * rust_ok / rust_err only set that discriminant, which is what rust_try reads.
 * rust_ret_ok / rust_ret_err do the same and then c_ret, so a branch that only
 * returns the result is one call. They must not drop the value in rax: rust_try
 * still tests it after the callee returns.
 *
 * Each c_ret emits the frame epilogue, including a second return in the same
 * function. The discriminant stays in rax for rust_try.
 */
static inline void rust_set_result(const char* discriminant) {
    (void)c_into("rax", C_PTR, c_imm(discriminant, C_PTR, "is_err"), "is_err");
}
/* Set the Ok discriminant (0) in rax. */
static inline void rust_ok(void)  { rust_set_result("0"); }  /* Ok  */
/* Set the Err discriminant (1) in rax. */
static inline void rust_err(void) { rust_set_result("1"); }  /* Err */
/* Set the Ok discriminant (0), then return from the open function f. */
static inline void rust_ret_ok(CFunc* f) {
    rust_ok();
    c_ret(f);
}
/* Set the Err discriminant (1), then return from the open function f. */
static inline void rust_ret_err(CFunc* f) {
    rust_err();
    c_ret(f);
}

/*
 * The `?` operator, as a structured try/catch over one or more fallible calls.
 * rust_catch_begin reserves the shared error and join labels; rust_try lowers a
 * single `expr?` (on Err, jump to the error arm; on Ok, fall through);
 * rust_catch opens the error arm; rust_catch_end closes it:
 *
 *     RustCatch cat = rust_catch_begin();
 *         c_call(&checked_div, ...); rust_try(&cat);   // expr?
 *         <Ok path continues>
 *     rust_catch(&cat);
 *         <Err path: early-return body>
 *     rust_catch_end(&cat);
 */
typedef struct { const char* err; const char* end; } RustCatch;

/* Reserve the error and join labels for one `?` region. */
static inline RustCatch rust_catch_begin(void) {
    RustCatch c = { c_new_label(), c_new_label() };
    return c;
}
/* Lower one `?` on c: jump to the error arm when rax is Err, and fall through on Ok. */
static inline void rust_try(RustCatch* c) {
    write_if_not_equal_then_jump_to(c->err,
        (AsmDataField){"rax","is_err",NULL},
        literal("0", "is_err"));
}

/* `result?` where result is the value checked_div returned. */
static inline void rust_try_result(RustCatch* c, CValue result) {
    write_if_not_equal_then_jump_to(c->err, c_cell(result), literal("0", "is_err"));
}
/* Open the Err arm of c, sending the Ok path to the join label. */
static inline void rust_catch(RustCatch* c) {
    write_jmp(c->end);
    write_label(c->err);
}
/* Close c by placing its join label. */
static inline void rust_catch_end(RustCatch* c) { write_label(c->end); }

/* ======================================================================== *
 *  Initialisation
 * ======================================================================== */

/*
 * rust_init - per-language setup a Rust generator calls once, in place of poking
 * assembler options directly. Rust is case-sensitive and links mixed-case Win32
 * externs, so it selects case-sensitive identifiers. (Rust has no class builder,
 * so there is no builder state to reset.)
 */
static inline void rust_init(void) {
    write_set_case_sensitive_identifiers();
}

