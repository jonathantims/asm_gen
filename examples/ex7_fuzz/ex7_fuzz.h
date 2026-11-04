/*
 * ex7_fuzz.h - a generation-time fuzz-testing utility (shared with example 8).
 *
 * THE IDEA
 * --------
 * A single generation-time flag, testing_enabled. Alongside an ordinary function
 * you also register a *fuzz test*: a body that generates many inputs, calls the
 * function, and validates the outputs. The utility synthesises a test procedure
 * per registration and collects them; generate_test_method() emits one runner
 * that calls every registered test. When the flag is off, NOTHING is emitted -
 * no test procedures, no runner, no strings - ZERO FOOTPRINT: the shipped binary
 * contains only the function itself.
 *
 * Inputs are produced by a generated linear-congruential generator (a classic
 * splitmix/LCG step) seeded from the fuzz_seed global, so each case is a fresh
 * pseudo-random value; the sequence is deterministic and reproducible. A test
 * that fails validation jumps to a per-test fail label that prints a message and
 * exits 1; a test that validates every case prints "PASS <name>".
 *
 * The input is the fuzz_in variable. It is a global so it survives the call
 * under test. The loop counter is the fuzz_i variable. A body jumps to
 * ctx->fail with c_goto; it does not print the failure itself, because a
 * call on that arm would poison the values still live on the passing arm.
 */


/* The generation-time flag. Off by default => zero footprint. */
static bool testing_enabled = false;
/* Turn this example's fuzz testing on before emission. */
static inline void testing_enable(void)  { testing_enabled = true; }
/* Turn this example's fuzz testing off before emission. */
static inline void testing_disable(void) { testing_enabled = false; }

#define FUZZ_MAX_TESTS 16
#define FUZZ_CASES     "256"     /* cases generated per test */

/* Context handed to each test body while it is emitted. */
typedef struct FuzzCtx {
    ExConsole*       con;
    AsmExtern*       print;
    AsmExtern*       print_u64;
    const AsmExtern* fn;          /* the function under test */
    const char*      name;        /* the test's name */
    CLabel           fail;        /* c_goto here on a failed case */
    void*            user;        /* opaque, for example-specific data (ex8) */
    const char*      arg_tag;     /* parameter tag of a one-argument function */
} FuzzCtx;

typedef void (*FuzzBody)(FuzzCtx*);

/* One registered fuzz test. */
typedef struct {
    const char*      name;
    const AsmExtern* fn;
    FuzzBody         body;
    void*            user;
    ExStr            pass_msg;
    ExStr            fail_msg;
} FuzzEntry;

static FuzzEntry fuzz_entries[FUZZ_MAX_TESTS];
static int       fuzz_nentries = 0;
static AsmExtern fuzz_bound[FUZZ_MAX_TESTS];
static const char* fuzz_arg_tag[FUZZ_MAX_TESTS];
static AsmExtern fuzz_runner_slot;

/*
 * fuzz_register - name a test whose function is emitted later. arg_tag is
 * the parameter tag fuzz_call applies, or NULL when the function takes none.
 * No-op when testing is disabled.
 */
static inline void fuzz_register(const char* name, const char* arg_tag,
                                 FuzzBody body, void* user) {
    if (!testing_enabled) return;
    crash_if(fuzz_nentries >= FUZZ_MAX_TESTS, "fuzz: too many tests");
    FuzzEntry e; e.name = name; e.fn = NULL; e.body = body; e.user = user;
    e.pass_msg = (ExStr){0}; e.fail_msg = (ExStr){0};
    fuzz_arg_tag[fuzz_nentries] = arg_tag;
    fuzz_entries[fuzz_nentries++] = e;
}

/* Point a registered test at the procedure just emitted. */
static inline void fuzz_bind(const char* name, AsmExtern fn) {
    if (!testing_enabled) return;
    for (int i = 0; i < fuzz_nentries; i++) {
        if (strcmp(fuzz_entries[i].name, name) == 0) {
            fuzz_bound[i] = fn;
            fuzz_entries[i].fn = &fuzz_bound[i];
            return;
        }
    }
    host_fail("fuzz_bind: no such test");
}

typedef AsmExtern (*FuzzEmitFn)(void*);

/* When testing is on, emit the variant and bind it. Off, emit nothing. */
static inline void fuzz_bind_emit(const char* name, FuzzEmitFn emit, void* user) {
    if (!testing_enabled) return;
    fuzz_bind(name, emit(user));
}

/* Emit the data the harness needs: the LCG seed plus a PASS/FAIL string pair per
 * registered test. Call inside write_data(). No-op when disabled. */
static inline void fuzz_emit_data(void) {
    if (!testing_enabled) return;
    c_ensure_data();
    write_dq("fuzz_seed", "2463534242");        /* nonzero LCG seed */
    for (int i = 0; i < fuzz_nentries; i++) {
        char pn[64], fn[64], pt[128], ft[160];
        snprintf(pn, sizeof pn, "s_pass_%s", fuzz_entries[i].name);
        snprintf(fn, sizeof fn, "s_fail_%s", fuzz_entries[i].name);
        snprintf(pt, sizeof pt, "\"PASS %s\",13,10", fuzz_entries[i].name);
        snprintf(ft, sizeof ft, "\"[FUZZ] FAIL %s\",13,10", fuzz_entries[i].name);
        fuzz_entries[i].pass_msg = ex_string(c_strdup_arena(pn), c_strdup_arena(pt));
        fuzz_entries[i].fail_msg = ex_string(c_strdup_arena(fn), c_strdup_arena(ft));
    }
}

/* Emit the scratch globals (loop counter + current input). dataq section. */
static inline void fuzz_emit_scratch(void) {
    if (!testing_enabled) return;
    c_ensure_bss();
    write_unitialised_data("fuzz_i", "i");
    write_unitialised_data("fuzz_in", "in");
}

/*
 * fuzz_next_input - one scalar in 0..1023, not a tuple of arguments.
 * Call it once per input, and call it before other variables are live:
 * c_funcs.h has no multiply, so the step reuses scratch the pool does not
 * track. The returned variable is the fuzz_in global, tagged "in". The
 * next call overwrites that global, so copy it before asking for another
 * input. Advances fuzz_seed.
 */
static inline CValue fuzz_next_input(void) {
    write_replace_mov("rax", (AsmDataField){ "fuzz_seed", "seed", NULL });
    write_replace_mov("rcx", (AsmDataField){ "6364136223846793005", "mul", NULL });   /* PCG default multiplier, https://www.pcg-random.org/ */
    write_imul2("rax", "rcx");
    write_replace_mov("rcx", (AsmDataField){ "1442695040888963407", "inc", NULL });   /* LCG increment */
    write_add((AsmDataField){ "rax", "seed", NULL }, (AsmDataField){ "rcx", "inc", NULL });
    write_replace_mov("fuzz_seed", (AsmDataField){ "rax", "seed", NULL });
    write_shr("rax", "40");                             /* take high bits */
    write_and("rax", "1023");                           /* bound to 0..1023 */
    /* The global keeps the input across the call under test. A call
     * clobbers rax, and the harness procedure has no c frame to save it. */
    /* rax still holds the seed tag from the multiply. The global is "in". */
    change_tag((AsmDataField){ "rax", "seed", NULL }, "in");
    write_set_mov((AsmDataField){ "fuzz_in", "in", NULL },
                  (AsmDataField){ "rax", "in", NULL });
    return c_wrap((AsmDataField){ "fuzz_in", "in", NULL }, C_PTR);
}

/* The per-test loop scaffold. Emits the case loop header; the caller then emits
 * "produce input -> call fn -> validate (c_goto ctx->fail on failure)"
 * and finally fuzz_loop_end. Labels are qualified by the test name so multiple
 * tests do not collide (MASM labels are global). */
static inline void fuzz_loop_begin(FuzzCtx* c) {
    char lbl[80], done[80];
    snprintf(lbl,  sizeof lbl,  "fuzz_loop_%s",  c->name);
    snprintf(done, sizeof done, "fuzz_done_%s",  c->name);
    write_set_mov((AsmDataField){ "fuzz_i", "i", NULL }, (AsmDataField){ "0", "i", NULL });
    write_label(c_strdup_arena(lbl));
        write_cmp((AsmDataField){ "fuzz_i", "i", NULL }, literal(FUZZ_CASES, "i"));
        write_je(c_strdup_arena(done));
}
/* Close the per-test loop in c: bump fuzz_i, jump to the top, and emit the done label. */
static inline void fuzz_loop_end(FuzzCtx* c) {
    char lbl[80], done[80];
    snprintf(lbl,  sizeof lbl,  "fuzz_loop_%s",  c->name);
    snprintf(done, sizeof done, "fuzz_done_%s",  c->name);
        write_inc("fuzz_i");
        write_jmp(c_strdup_arena(lbl));
    write_label(c_strdup_arena(done));
}

/*
 * generate_test_method - synthesise one test procedure per registered test and a
 * `run_all_tests` runner that calls them in turn. Returns the runner (its .name
 * is NULL when testing is disabled / nothing was emitted). Call inside
 * write_code(), after the functions under test are defined.
 */
static inline AsmExtern generate_test_method(ExConsole* con, AsmExtern* print,
                                             AsmExtern* print_u64) {
    AsmExtern runner; runner.name = NULL;
    if (!testing_enabled || fuzz_nentries == 0) return runner;

    for (int i = 0; i < fuzz_nentries; i++) {
        FuzzEntry* e = &fuzz_entries[i];
        char proc[64];
        snprintf(proc, sizeof proc, "fuzztest_%s", e->name);
        const char* procn = c_strdup_arena(proc);

        AsmExtern t = write_proc(procn, NULL, 0, asm_void);
            /* The body jumps here on a failed case. The jump does not move
             * the stack, and the success path below already calls print at
             * the procedure's entry alignment, so this label needs no
             * adjustment of its own. */
            CLabel fail = c_forward_label();
            FuzzCtx c = { con, print, print_u64, e->fn, e->name, fail, e->user, fuzz_arg_tag[i] };
            e->body(&c);                        /* the test's generated body */
            /* success path: all cases validated */
            ex_puts(print, e->pass_msg);
            write_ret();
            c_place_label(fail);
                ex_puts(print, e->fail_msg);
                ex_exit(con, 1);
        write_endp(&t);
    }

    runner = write_proc("run_all_tests", NULL, 0, asm_void);
        for (int i = 0; i < fuzz_nentries; i++) {
            char call[80];
            snprintf(call, sizeof call, "fuzztest_%s", fuzz_entries[i].name);
            write_call_symbol(c_strdup_arena(call));
        }
        write_ret();
    write_endp(&runner);
    return runner;
}

/* Call the function under test. arg is retagged to the registered parameter. */
static inline CValue fuzz_call(FuzzCtx* c, CValue arg) {
    const char* tag = c->arg_tag ? c->arg_tag : arg.tag;
    if (arg.tag && tag && strcmp(arg.tag, tag) != 0)
        return c_call_proc(C_PTR, c->fn, c_treat_as(arg, arg.tag, tag));
    return c_call_proc(C_PTR, c->fn, c_cell(arg));
}

/* Jump to the test's fail label when got is odd. */
static inline void fuzz_expect_even(FuzzCtx* c, CValue got) {
    CValue low = c_dup("low", C_PTR, got);
    c_and_imm(&low, "1");
    CElse odd = c_if_ne_else(low, c_imm_n(0, C_PTR, "zero"));
        c_goto(c->fail);
    c_else(&odd);
    c_end_if_else(&odd);
    c_free(&low);
}

/* Jump to the test's fail label when got and exp differ. */
static inline void fuzz_expect_eq(FuzzCtx* c, CValue got, CValue exp) {
    CValue g = c_dup("got", C_PTR, got);
    CValue e = c_dup("exp", C_PTR, exp);
    CElse bad = c_if_ne_else(g, e);
        c_goto(c->fail);
    c_else(&bad);
    c_end_if_else(&bad);
    c_free(&g);
    c_free(&e);
}

/* Emit the runner. The header keeps it; the driver does not hold a null extern. */
static inline void fuzz_emit_runner(ExConsole* con, AsmExtern* print, AsmExtern* pu64) {
    fuzz_runner_slot.name = NULL;
    if (!testing_enabled) return;
    fuzz_runner_slot = generate_test_method(con, print, pu64);
}

/* Emit `call run_all_tests` when a runner was emitted. */
static inline void fuzz_invoke(void) {
    if (testing_enabled && fuzz_runner_slot.name) write_call_symbol(fuzz_runner_slot.name);
}

