/*
 * ex3_profile.h - generation-time call profiling, emitted as CSV.
 *
 * When the generation-time flag is on, each profiled call is bracketed with a
 * timestamp read (rdtsc) and, after the call returns, a CSV row
 * "<function>,<ticks>" is printed. When off, the call is emitted bare - no
 * timing, no CSV, zero footprint. The profile format is deliberately trivial
 * (CSV, not JSON): one row per call, `name,ticks`.
 */


static bool prof_enabled = false;
/* Turn this example's call profiling on before emission. Takes no parameters. */
static inline void prof_enable(void)  { prof_enabled = true; }
/* Turn this example's call profiling off before emission. Takes no parameters. */
static inline void prof_disable(void) { prof_enabled = false; }

/* Context: the console bits the woven CSV writer needs. */
typedef struct {
    ExConsole*  con;
    AsmExtern*  print;
    AsmExtern*  print_u64;
    ExStr       comma;     /* ","            */
    ExStr       newline;   /* CR LF          */
    bool        active;
} ProfCtx;

/* Emit the scratch global holding the start timestamp. Opens .data?. */
static inline void prof_globals(void) {
    c_ensure_bss();
    write_unitialised_data("prof_start", "?");
}

/* rdtsc -> rax (combine edx:eax into a 64-bit count). */
static inline void prof_rdtsc_to_rax(void) {
    asm_read_tsc("rax");
}

/* A void function that spins `iters` times. ecx is the counter. The label
 * is chosen from the function name so the two demo loops do not collide. */
static inline AsmExtern prof_emit_spin(const char* name, int iters) {
    AsmExtern f = write_proc(name, NULL, 0, asm_void);
        write_replace_mov("ecx", (AsmDataField){ c_dec(iters), "count", NULL });
        char labbuf[80];
        snprintf(labbuf, sizeof labbuf, "Lspin_%s", name);
        const char* lab = c_strdup_arena(labbuf);
        write_label(lab);
        write_dec("ecx");
        write_jnz(lab);
        write_ret();
    write_endp(&f);
    return f;
}

/* Build the profiling context for con (also stores whether the feature is active).
 * The context keeps print, print_u64, comma, and newline for the CSV rows. */
static inline ProfCtx prof_begin(ExConsole* con, AsmExtern* print,
                                 AsmExtern* print_u64, ExStr comma, ExStr newline) {
    ProfCtx c;
    c.con = con; c.print = print; c.print_u64 = print_u64;
    c.comma = comma; c.newline = newline; c.active = prof_enabled;
    return c;
}

/* Print line when profiling is on. The disabled build emits no call. */
static inline void prof_banner(ProfCtx* c, ExStr line) {
    if (c->active) ex_puts(c->print, line);
}

/* Print line when profiling is off. The timed build emits no call. */
static inline void prof_quiet(ProfCtx* c, ExStr line) {
    if (!c->active) ex_puts(c->print, line);
}

/*
 * prof_call - call the 0-argument void procedure `proc`. When profiling is on,
 * bracket it with rdtsc reads and print a CSV row "<name>,<ticks>". `name` is a
 * console string holding the function's name. When off, just emit the call.
 */
static inline void prof_call(ProfCtx* c, ExStr name, const AsmExtern* proc) {
    if (!c->active) {
        write_call(proc);
        return;
    }
    prof_rdtsc_to_rax();
    write_set_mov((AsmDataField){ "prof_start", "?", NULL }, (AsmDataField){ "rax", "ticks", NULL });   /* stash start                 */
    write_call(proc);                          /* the measured work           */
    prof_rdtsc_to_rax();                        /* rax = end                   */
    write_sub((AsmDataField){ "rax", "ticks", NULL },
              (AsmDataField){ "prof_start", "ticks", NULL });
    write_set_mov((AsmDataField){ "prof_start", "?", NULL }, (AsmDataField){ "rax", "ticks", NULL });    /* stash elapsed (print clobbers rax) */
    /* Emit the CSV row: name , ticks <newline>. */
    ex_puts(c->print, name);
    ex_puts(c->print, c->comma);
    write_replace_mov("rcx", (AsmDataField){ "prof_start", "value", NULL });
    write_call_symbol(c->print_u64->name);
    ex_puts(c->print, c->newline);
}

