/*
 * ex9_exceptions.h - a CPU exception, resumed by a vectored handler.
 *
 * When the flag is off, the emitters produce the plain path: no handler, no
 * globals, no ud2. When the flag is on, throw stores an integer code and
 * executes ud2. That instruction is the CPU exception. Windows delivers
 * EXCEPTION_ILLEGAL_INSTRUCTION (0xC000001D) to the vectored handler
 * registered with AddVectoredExceptionHandler.
 *
 * A vectored handler is not an unwind. The operating system does not walk
 * frames, does not run destructors, and does not jump to a catch on its own.
 * It calls the handler with EXCEPTION_POINTERS in rcx and then does what the
 * handler's return value says. The exception record is at [rcx], the CONTEXT
 * at [rcx+8]. On x64 the CONTEXT has Rsp at byte 152 (0x98) and Rip at byte
 * 248 (0xF8).
 *
 * EXCEPTION_CONTINUE_EXECUTION is -1. The processor resumes at CONTEXT.Rip
 * with CONTEXT.Rsp. If the handler returned -1 and left those two fields
 * alone, execution would continue at the ud2 and the exception would be
 * raised again. The handler writes the catch label into Rip and the rsp
 * captured at exc_try_begin into Rsp, then returns -1. That pair of stores
 * is the transfer. The throw path itself has no `mov rsp` and no `jmp` to
 * the catch. Any other exception code returns 0, which is
 * EXCEPTION_CONTINUE_SEARCH, and a later handler may take it.
 *
 * Structured exception handling is the feature that unwinds for you. It
 * needs unwind metadata in .pdata and .xdata describing each frame, and a
 * frame handler the unwinder calls. This generator does not emit that
 * metadata, and the language layers do not register frame handlers. A
 * vectored handler runs first and can resume anywhere by editing CONTEXT,
 * which is why the catch works without .pdata.
 *
 * One try is active at a time. The catch address and the stack pointer are
 * the two globals the handler reads. They are the whole "where do I resume"
 * record. This is not a setjmp image of the callee-saved registers, because
 * the resume uses the CONTEXT the CPU already captured.
 */


static bool exc_enabled = false;
/* Turn this example's exception handling on before emission. */
static inline void exc_enable(void)  { exc_enabled = true; }
/* Turn this example's exception handling off before emission. */
static inline void exc_disable(void) { exc_enabled = false; }

typedef struct {
    const char* catch_label;
    const char* done_label;
    bool        active;
} ExcTry;

/* Emit exc_handler, exc_sp, and exc_code when exception handling is on. */
static inline void exc_globals(void) {
    if (exc_enabled) {
        c_ensure_bss();
        write_unitialised_data("exc_handler", "?");
        write_unitialised_data("exc_sp", "?");
        write_unitialised_data("exc_code", "?");
    }
}

static AsmExtern exc_add_veh;

/* Declare AddVectoredExceptionHandler. Call before the generated main. */
static inline void exc_declare(void) {
    if (!exc_enabled) return;
    static const char* vh[] = { "First", "Handler" };
    exc_add_veh = c_extern("AddVectoredExceptionHandler", vh, 2, C_PTR, "handle");
}

/* Register veh_trap as the first vectored handler. No-op when the feature is off. */
static inline void exc_install(void) {
    if (!exc_enabled) return;
    c_call_ext_void(&exc_add_veh, c_lit_n(1, "First"), c_addr("veh_trap", "Handler"));
}

/* The OS calls this with rcx = EXCEPTION_POINTERS*. Emit it only when the
 * feature is on: the procedure name is the zero-footprint token. */
static inline void exc_emit_handler(void) {
    if (!exc_enabled) return;
    static const AsmDataField hp[] = { {"rcx","info",NULL} };
    AsmExtern veh = write_proc("veh_trap", hp, 1, (AsmDataField){"eax","disp",NULL});
        write_replace_mov("rax", (AsmDataField){ "qword ptr [rcx]", "record", NULL });
        write_cmp((AsmDataField){ asm_mem("dword", "rax", 0), "code", NULL },
                  literal("0C000001Dh", "code"));
        write_jne("veh_search");
        write_replace_mov("rdx", (AsmDataField){ "qword ptr [rcx+8]", "ctx", NULL });
        write_replace_mov("rax", (AsmDataField){ "exc_handler", "land", NULL });
        write_replace_mov("qword ptr [rdx+248]", (AsmDataField){ "rax", "rip", NULL });   /* CONTEXT.Rip */
        write_replace_mov("rax", (AsmDataField){ "exc_sp", "sp", NULL });
        write_replace_mov("qword ptr [rdx+152]", (AsmDataField){ "rax", "rsp", NULL });   /* CONTEXT.Rsp */
        write_replace_mov("eax", (AsmDataField){ "-1", "disp", NULL });                   /* CONTINUE_EXECUTION */
        write_ret();
    write_label("veh_search");
        write_replace_mov("eax", (AsmDataField){ "0", "disp", NULL });                    /* CONTINUE_SEARCH */
        write_ret();
    write_endp(&veh);
}

/* Open a try and, when exception handling is on, store the catch address and rsp. */
static inline ExcTry exc_try_begin(void) {
    ExcTry t;
    t.active = exc_enabled;
    t.catch_label = exc_enabled ? c_new_label() : NULL;
    t.done_label = exc_enabled ? c_new_label() : NULL;
    if (exc_enabled) {
        write_lea("rax", t.catch_label);
        /* lea overwrote rax. A prior call may have tagged it. */
        change_tag((AsmDataField){ "rax", "handler", NULL }, "handler");
        write_set_mov((AsmDataField){ "exc_handler", "?", NULL }, (AsmDataField){ "rax", "handler", NULL });
        write_set_mov((AsmDataField){ "exc_sp", "?", NULL }, (AsmDataField){ "rsp", "sp", NULL });
    }
    return t;
}

/* Store the code and raise a CPU exception. The handler resumes at the catch. */
static inline void exc_throw(int code) {
    if (!exc_enabled) return;
    write_replace_mov("rax", (AsmDataField){ asm_i(code), "code", NULL });
    write_set_mov((AsmDataField){ "exc_code", "?", NULL }, (AsmDataField){ "rax", "code", NULL });
    write_ud2();
}

/* When t is active, jump to its done label and emit its catch label. */
static inline void exc_catch(ExcTry* t) {
    if (!t->active) return;
    write_jmp(t->done_label);
    write_label(t->catch_label);
}

/* When t is active, emit the done label of t. */
static inline void exc_end_try(ExcTry* t) {
    if (!t->active) return;
    write_label(t->done_label);
}

typedef void (*ExcCatchFn)(void*);

/* Emit the catch and run body only when the try is active. */
static inline void exc_catch_with(ExcTry* t, ExcCatchFn fn, void* user) {
    if (!t->active) return;
    exc_catch(t);
    fn(user);
    exc_end_try(t);
}

/* The code stored by the last throw, as a variable named code. */
static inline CValue exc_code(void) {
    CValue slot = c_wrap((AsmDataField){ "exc_code", "code", NULL }, C_LONG);
    return c_dup("code", C_LONG, slot);
}
