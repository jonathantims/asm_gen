/*
 * ex11_causal.h - the causal-profiling library used by ex11_causal.c.
 *
 * The application hands this library a test case and the functions that
 * test case calls. The test case is a black box: it may spawn threads.
 * The profiled functions are ordinary bodies wrapped with
 * causal_func_begin / causal_func_end. When the feature is on, the
 * wrapper inserts a delay before the return, and only when this function
 * is the one the campaign selected. The first such site is labelled
 * causal_delay. A build that never calls causal_enable emits neither the
 * label nor the spin.
 *
 * The application works in this order:
 *
 *   causal_enable       optional. Off unless this runs.
 *   causal_prepare      once, before write_data. Kernel imports.
 *   causal_emit_data    inside write_data. The campaign's globals, and
 *                       only when the feature is on.
 *   causal_scratch      inside write_dataq. One thread handle per spawn
 *                       the test case may make.
 *   causal_func_begin   open a profiled function. The name is its symbol.
 *     body              c_funcs.h. No ret of its own.
 *   causal_func_end     delay (when on), then ret. Returns the CFunc.
 *   causal_repeat       call a profiled function CAUSAL_REPS times.
 *                       Used inside a worker the test case spawns.
 *   causal_testcase_begin
 *     causal_spawn      CreateThread of a worker that already exists.
 *     causal_join_all   WaitForSingleObject on every spawn.
 *   causal_testcase_end the procedure is named causal_testcase.
 *   causal_campaign     emits main. On: a baseline, then trials, then a
 *                       file of names. Off: run the test case once and
 *                       exit 0.
 *
 * Trials. Four unrecorded runs first, so the core is not still changing
 * rate. Then CAUSAL_PER trials per profiled function, round-robin. Each
 * trial is paired with its own baseline, taken immediately before it,
 * where the target id is -1 and nothing matches. One shared baseline
 * would treat a change in the core's rate as the effect of the function.
 * The trial draws a delay from a fixed LCG (not from a live rdtsc),
 * stores that delay, and times one call of the test case with rdtsc.
 * The record is the function id, the delay, the paired baseline, the
 * cycle delta, the function's own cycles, and the pause that elapsed.
 *
 * The delay is a spin on the time-stamp counter, up to the stored
 * budget or 100000 iterations, whichever comes first. Sleep is the wrong
 * tool: its resolution is about a millisecond. The budget is 49152 plus
 * a value in 0..16383, so one call pauses for tens of thousands of
 * cycles. A smaller budget is lost under the run-to-run drift of the
 * core. Across CAUSAL_REPS calls the pause is a large fraction of the
 * long body, and still short of that body, so the empty thread stays
 * the short one.
 *
 * What the rank file reports. One line per function, largest effect
 * first. A tie keeps the earlier function. CreateFileA failing still
 * exits 0. A line looks like
 *
 *   critical_step e2e=85/1000 loc=90/1000 effect=944/1000
 *
 * The unit is thousandths of the undelayed time. 1000 means that time
 * doubled. The two proportions, and the comparison of them:
 *
 *   e2e     how much the test case slowed. Sum of (delta - paired
 *           baseline) over this function's trials, divided by the sum
 *           of those baselines.
 *   loc     how much the function itself slowed. Sum of the pause that
 *           actually elapsed, divided by the sum of the time from the
 *           function's entry to that pause. An empty body is only the
 *           cost of the two timestamp reads, so its loc figure is large.
 *   effect  e2e / loc. 1000 means the program slowed by the same
 *           proportion as the function. 0 means the pause did not move
 *           the test case. That is the critical-path fraction in this
 *           experiment: a pause on the path shows up in the join, and a
 *           pause in the slack does not.
 *
 * A zero sum of paired baselines, or a function that recorded neither
 * body time nor a pause, stores 0 for that proportion instead of
 * dividing by zero. The run totals are plain adds. A profiled
 * function runs on one thread.
 *
 * Stronger analysis, not what this toy runs:
 *
 *   Virtual speedup (Coz). Instead of inserting a pause, speed the
 *     function up virtually: at progress points, subtract time from the
 *     thread so the experiment estimates the effect of optimising that
 *     function. Progress points are program locations that count useful
 *     work. Other threads are paused so the sped-up thread's share of
 *     progress stays in proportion.
 *   Correlation. Across trials, correlate the per-function delay with
 *     the program delta. A high correlation means the function sits on
 *     the critical path.
 *   Regression. Fit delta = a + b * delay. The slope b is the fraction
 *     of the delay that was on the critical path. The intercept absorbs
 *     the baseline.
 *   Multiple regression. When one trial delays several functions, each
 *     coefficient is that function's critical-path fraction holding the
 *     others fixed.
 *   Bootstrap intervals. Resample the trials, recompute the slope, and
 *     keep the interval, so a small effect is not ranked above noise.
 *
 * Magnitudes. CAUSAL_REPS is 2000 calls of the body per test-case run.
 * CAUSAL_PER is 4 trials per function. The application's long body is
 * tens of thousands of adds, so it stays ahead of the empty body even
 * after the empty body has been paused. The campaign is still a fraction
 * of a second.
 * A single thread would add the same pause to the only timeline, and the
 * ranking would not mean anything. The test case runs the bodies side
 * by side.
 */

#define CAUSAL_MAX     4
#define CAUSAL_REPS    2000
#define CAUSAL_PER     4
#define CAUSAL_THREADS 4

static const char* causal_names[CAUSAL_MAX];
static int causal_n = 0;
static int causal_on = 0;
static int causal_labelled = 0;
static int causal_nspawns = 0;
static int causal_in_func = 0;
static int causal_in_case = 0;
static int causal_kernel_ready = 0;
static CFunc causal_cur;
static int causal_cur_id = -1;
static CFunc causal_case_fn;
static CValue causal_enter;
static ExStr causal_name_str[CAUSAL_MAX];
static ExStr causal_e2e_lit;
static ExStr causal_loc_lit;
static ExStr causal_fx_lit;
static ExStr causal_end_lit;
static AsmExtern causal_fmt;

static AsmExtern causal_create_thread;
static AsmExtern causal_wait;
static AsmExtern causal_create_file;
static AsmExtern causal_close;

static const char* causal_id_imm[CAUSAL_MAX] = { "0", "1", "2", "3" };
static const char* causal_s_tag[CAUSAL_MAX]  = { "s0", "s1", "s2", "s3" };
static const char* causal_i_tag[CAUSAL_MAX]  = { "id0", "id1", "id2", "id3" };

/* Decimal text of v, copied into the arena for an immediate. */
static inline const char* causal_imm(int v) {
    char buf[32];
    int n = snprintf(buf, sizeof buf, "%d", v);
    crash_if(n < 0 || n >= (int)sizeof buf, "causal: number too long");
    return c_strdup_arena(buf);
}

/* Arm the wrappers. Call before causal_prepare. Off unless this runs,
 * which is the zero-footprint build. */
static inline void causal_enable(void) { causal_on = 1; }

/* Kernel imports. CreateThread and WaitForSingleObject are how the test
 * case splits work off, in both builds. The file imports exist only when
 * the campaign writes a result. */
static inline void causal_prepare(void) {
    crash_if(causal_kernel_ready, "causal: prepare twice");
    causal_kernel_ready = 1;
    static const char* ct_tags[] = {
        "lpThreadAttributes", "dwStackSize", "lpStartAddress",
        "lpParameter", "dwCreationFlags", "lpThreadId"
    };
    causal_create_thread = c_extern("CreateThread", ct_tags, 6, C_PTR, "handle");
    static const char* wait_tags[] = { "hHandle", "dwMilliseconds" };
    causal_wait = c_extern("WaitForSingleObject", wait_tags, 2, C_INT, "ok");
    if (!causal_on) return;
    static const char* cf_tags[] = {
        "lpFileName", "dwDesiredAccess", "dwShareMode", "lpSecurityAttributes",
        "dwCreationDisposition", "dwFlagsAndAttributes", "hTemplateFile"
    };
    causal_create_file = c_extern("CreateFileA", cf_tags, 7, C_PTR, "handle");
    static const char* cl_tags[] = { "hObject" };
    causal_close = c_extern("CloseHandle", cl_tags, 1, C_INT, "ok");
}

/* Campaign globals. Call from write_data(), before any profiled body.
 * Nothing is emitted when the feature is off, so the disabled asm has
 * no target word and no spin budget. */
static inline void causal_emit_data(void) {
    if (!causal_on) return;
    c_ensure_data();
    int slots = CAUSAL_MAX * CAUSAL_PER;
    write_dq("causal_target", "-1");
    write_dq("causal_budget", "0");
    write_dq("causal_seed", "2463534242");
    write_dq("causal_base", asm_dup(slots, "0"));
    write_dq("causal_t0", "0");
    write_dq("causal_delta", asm_dup(slots, "0"));
    write_dq("causal_lag", asm_dup(slots, "0"));
    write_dq("causal_pick", asm_dup(slots, "0"));
    write_dq("causal_body", asm_dup(slots, "0"));
    write_dq("causal_spin", asm_dup(slots, "0"));
    write_dq("causal_run_body", "0");
    write_dq("causal_run_spin", "0");
    write_dq("causal_e2e", asm_dup(CAUSAL_MAX, "0"));
    write_dq("causal_loc", asm_dup(CAUSAL_MAX, "0"));
    write_dq("causal_fx", asm_dup(CAUSAL_MAX, "0"));
    write_dq("causal_nptr", "0");
    write_dq("causal_nlen", "0");
}

/* One kernel handle per spawn the test case may make. Call from
 * write_dataq(). Both builds spawn, so the handles exist either way. */
static inline void causal_scratch(void) {
    c_ensure_bss();
    for (int i = 0; i < CAUSAL_THREADS; i++) {
        char sym[32];
        snprintf(sym, sizeof sym, "causal_th%d", i);
        write_unitialised_data(c_strdup_arena(sym), "hHandle");
    }
}

/* Open a profiled function named name. The body uses c_funcs.h and does not return;
 * causal_func_end returns. The id is the position in this list. */
static inline void causal_func_begin(const char* name) {
    crash_if(causal_in_func || causal_in_case, "causal: a function is already open");
    crash_if(name == NULL || name[0] == 0, "causal: a profiled function needs a name");
    crash_if(causal_n >= CAUSAL_MAX, "causal: too many profiled functions");
    causal_cur_id = causal_n;
    causal_names[causal_n++] = name;
    causal_cur = c_begin_function(name, NULL, 0, C_PTR, NULL);
    causal_in_func = 1;
    if (!causal_on) return;
    /* This frame's slot, so two threads timing two functions do not share
     * the start. The body runs, then causal_func_end reads the slot. */
    causal_enter = c_stack_local("tenter", C_PTR, ALIGN_C);
    asm_read_tsc("rax");
    write_replace_mov(causal_enter.loc, (AsmDataField){ "rax", "tenter", NULL });
}

/* Body cycles for this call of id, added to the run total only when this
 * function is the one the campaign selected. The other thread leaves
 * the total alone. */
static void causal_note_body(int id) {
    const char* skip = c_new_label();
    asm_read_tsc("rax");
    write_sub((AsmDataField){ "rax", "dt", NULL },
              (AsmDataField){ causal_enter.loc, "tenter", NULL });
    write_cmp((AsmDataField){ asm_mem("qword", "causal_target", 0), "id", NULL },
              literal(causal_imm(id), "id"));
    write_jne(skip);
    write_add((AsmDataField){ asm_mem("qword", "causal_run_body", 0), "cycles", NULL },
              (AsmDataField){ "rax", "dt", NULL });
    write_label(skip);
}

/* Spin until the time-stamp counter has moved by causal_budget, or until
 * the iteration cap, when the selected function is id. r10 holds the start.
 * r11 counts iterations. Both are caller-saved; the function is about to return. */
static void causal_emit_delay(int id) {
    const char* skip = c_new_label();
    const char* spin = c_new_label();
    const char* done = c_new_label();
    write_cmp((AsmDataField){ asm_mem("qword", "causal_target", 0), "id", NULL },
              literal(causal_imm(id), "id"));
    write_jne(skip);
    if (!causal_labelled) {
        write_label("causal_delay");
        causal_labelled = 1;
    }
    asm_read_tsc("r10");
    write_replace_mov("r11", (AsmDataField){ "0", "spins", NULL });
    write_label(spin);
        asm_read_tsc("rax");
        write_sub((AsmDataField){ "rax", "dt", NULL },
                  (AsmDataField){ "r10", "t0", NULL });
        write_inc("r11");
        write_cmp((AsmDataField){ "r11", "spins", NULL }, literal("100000", "cap"));
        write_jae(done);
        write_cmp((AsmDataField){ "rax", "dt", NULL },
                  (AsmDataField){ asm_mem("qword", "causal_budget", 0), "cycles", NULL });
        write_jb(spin);
    write_label(done);
    write_add((AsmDataField){ asm_mem("qword", "causal_run_spin", 0), "cycles", NULL },
              (AsmDataField){ "rax", "dt", NULL });
    write_label(skip);
}

/* Close the profiled function. The delay is emitted only when the
 * feature is on, so a disabled build has no spin and no label. */
static inline CFunc causal_func_end(void) {
    crash_if(!causal_in_func, "causal: causal_func_end without begin");
    if (causal_on) {
        causal_note_body(causal_cur_id);
        causal_emit_delay(causal_cur_id);
    }
    c_ret(&causal_cur);
    c_end_function(&causal_cur);
    causal_in_func = 0;
    return causal_cur;
}

/* Call fn CAUSAL_REPS times. The caller is a worker inside the test
 * case, already inside its own c function. */
static inline void causal_repeat(CFunc* fn) {
    crash_if(fn == NULL, "causal: repeat needs a function");
    CValue k = c_local_imm("k", C_PTR, "0");
    CLoop L = c_loop_begin();
        c_call(fn);
        c_add(&k, c_imm("1", C_PTR, "k"));
        CIf stop = c_if_uge(k, c_imm(causal_imm(CAUSAL_REPS), C_PTR, "k"));
            c_loop_break(&L);
        c_end_if(&stop);
    c_loop_end(&L);
    c_free(&k);
}

/* Open the test case. The body may causal_spawn and must causal_join_all
 * before causal_testcase_end. The symbol is causal_testcase. */
static inline void causal_testcase_begin(void) {
    crash_if(!causal_kernel_ready, "causal: call causal_prepare first");
    crash_if(causal_in_func || causal_in_case, "causal: a block is already open");
    crash_if(causal_n < 1, "causal: wrap a function before the test case");
    causal_nspawns = 0;
    causal_case_fn = c_begin_function("causal_testcase", NULL, 0, C_PTR, NULL);
    causal_in_case = 1;
}

/* Open a thread procedure and commit its stack. A new thread does not
 * inherit main's committed pages. */
static inline CFunc causal_worker_begin(const char* name) {
    CFunc th = c_begin_function(name, NULL, 0, C_INT, "code");
    c_commit_stack();
    return th;
}

/* Start `proc` on a new thread and keep the handle in causal_th<i>.
 * `proc` is a procedure the application has already emitted. The index
 * is how many spawns this test case has emitted so far. */
static inline void causal_spawn(const char* proc) {
    crash_if(!causal_in_case, "causal: spawn belongs in the test case");
    crash_if(proc == NULL || proc[0] == 0, "causal: spawn needs a procedure");
    crash_if(causal_nspawns >= CAUSAL_THREADS, "causal: too many threads");
    int i = causal_nspawns++;
    char th[32];
    snprintf(th, sizeof th, "causal_th%d", i);
    CValue h = c_call_ext(C_PTR, &causal_create_thread,
        literal("0", "lpThreadAttributes"),
        literal("0", "dwStackSize"),
        c_addr(proc, "lpStartAddress"),
        literal("0", "lpParameter"),
        literal("0", "dwCreationFlags"),
        literal("0", "lpThreadId"));
    /* causal_th<i> is a data global tagged hHandle. The CreateThread
     * result is a handle in a register; storing it updates the global,
     * it does not repurpose the global. */
    write_set_mov((AsmDataField){ c_strdup_arena(th), "hHandle", NULL },
                  (AsmDataField){ c_cell(h).name, "handle", NULL });
    c_free(&h);
}

/* Wait until every thread this test case spawned has returned. */
static inline void causal_join_all(void) {
    crash_if(!causal_in_case, "causal: join belongs in the test case");
    crash_if(causal_nspawns < 1, "causal: join with no spawn");
    for (int i = 0; i < causal_nspawns; i++) {
        char th[32];
        snprintf(th, sizeof th, "causal_th%d", i);
        /* The symbol alone is its address. WaitForSingleObject needs the
         * qword stored there, which is the thread handle. */
        c_call_ext_void(&causal_wait,
            (AsmDataField){ asm_mem("qword", th, 0), "hHandle", NULL },
            literal("-1", "dwMilliseconds"));
    }
}

/* Close the open test case. It must have spawned, then it returns and ends. */
static inline void causal_testcase_end(void) {
    crash_if(!causal_in_case, "causal: causal_testcase_end without begin");
    crash_if(causal_nspawns < 1, "causal: the test case spawned nothing");
    c_ret(&causal_case_fn);
    c_end_function(&causal_case_fn);
    causal_in_case = 0;
}

/* Time one call of the test case. The caller has already stored the
 * target id and the budget. Returns the rdtsc delta in rax. The 40-byte
 * frame is the shadow plus the pad that puts rsp at 0 mod 16. */
static inline AsmExtern causal_emit_time(void) {
    AsmExtern time = write_proc("causal_time", NULL, 0,
                                (AsmDataField){ "rax", "cycles", NULL });
        write_replace_mov(asm_mem("qword", "causal_run_body", 0), (AsmDataField){ "0", "cycles", NULL });
        write_replace_mov(asm_mem("qword", "causal_run_spin", 0), (AsmDataField){ "0", "cycles", NULL });
        asm_read_tsc("rax");
        write_replace_mov(asm_mem("qword", "causal_t0", 0), (AsmDataField){ "rax", "cycles", NULL });
        write_sub((AsmDataField){ "rsp", "sp", NULL },
                  (AsmDataField){ "40", "shadow", NULL });
        asm_note_rsp_sub(40);
        write_call_symbol("causal_testcase");
        write_add((AsmDataField){ "rsp", "sp", NULL },
                  (AsmDataField){ "40", "shadow", NULL });
        asm_note_rsp_add(40);
        asm_read_tsc("rax");
        write_sub((AsmDataField){ "rax", "cycles", NULL },
                  (AsmDataField){ asm_mem("qword", "causal_t0", 0), "cycles", NULL });
        write_ret();
    write_endp(&time);
    return time;
}

/* Draw the next delay and select `id` for trial `trial`. The LCG uses
 * the PCG default multiplier (https://www.pcg-random.org/). The pool is
 * empty here: this writes rax and rcx directly, then the next call
 * allocates afresh. Budget is 49152 + (seed >> 40) & 16383. */
static void causal_arm(int trial, int id) {
    write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "causal_seed", 0), "seed", NULL });
    write_replace_mov("rcx", (AsmDataField){ "6364136223846793005", "mul", NULL });
    write_imul2("rax", "rcx");
    write_replace_mov("rcx", (AsmDataField){ "1442695040888963407", "inc", NULL });
    write_add((AsmDataField){ "rax", "seed", NULL },
              (AsmDataField){ "rcx", "inc", NULL });
    write_replace_mov(asm_mem("qword", "causal_seed", 0), (AsmDataField){ "rax", "seed", NULL });
    write_shr("rax", "40");
    write_and("rax", "16383");
    write_add((AsmDataField){ "rax", "lag", NULL },
              (AsmDataField){ "49152", "floor", NULL });
    write_replace_mov(asm_mem("qword", "causal_budget", 0), (AsmDataField){ "rax", "cycles", NULL });
    write_replace_mov(asm_mem("qword", "causal_lag", trial * 8), (AsmDataField){ "rax", "cycles", NULL });
    write_replace_mov(asm_mem("qword", "causal_pick", trial * 8), (AsmDataField){ causal_imm(id), "id", NULL });
    write_replace_mov(asm_mem("qword", "causal_target", 0), (AsmDataField){ causal_imm(id), "id", NULL });
}

/* Name strings and the fixed pieces of a rank line. Emitted after the
 * functions exist, by reopening .data. */
static void causal_emit_names(void) {
    write_data();
        write_db("c_file", "\"causal_rank.txt\",0");
        for (int i = 0; i < causal_n; i++) {
            char text[80];
            snprintf(text, sizeof text, "\"%s\"", causal_names[i]);
            char sym[32];
            snprintf(sym, sizeof sym, "c_name%d", i);
            causal_name_str[i] = ex_string(c_strdup_arena(sym), c_strdup_arena(text));
        }
        causal_e2e_lit = ex_string("c_e2e", "\" e2e=\"");
        causal_loc_lit = ex_string("c_loc", "\"/1000 loc=\"");
        causal_fx_lit = ex_string("c_fx", "\"/1000 effect=\"");
        causal_end_lit = ex_string("c_end", "\"/1000\",13,10");
    write_code();
}

/* Format a signed qword in rcx into ex_numbuf. causal_nptr receives the
 * first digit (or the minus) and causal_nlen the byte count. The digit
 * loop is DIV by 10. c_funcs.h has no divide, so this stays at the asm
 * layer. rbx, rsi, and rdi are pushed because a c caller may hold them. */
static void causal_emit_fmt(void) {
    static const AsmDataField pr[] = { {"rcx", "value", NULL} };
    const char* pos = c_new_label();
    const char* dig = c_new_label();
    const char* zero = c_new_label();
    const char* sign = c_new_label();
    const char* store = c_new_label();
    causal_fmt = write_proc("causal_fmt", pr, 1, asm_void);
        write_push("rbx");
        write_push("rsi");
        write_push("rdi");
        write_replace_mov("rax", (AsmDataField){ "rcx", "value", NULL });
        write_xor((AsmDataField){ "r8", "sign", NULL }, (AsmDataField){ "r8", "sign", NULL });
        write_test("rax", "rax");
        write_jns(pos);
        write_neg("rax");
        write_replace_mov("r8", (AsmDataField){ "1", "sign", NULL });
    write_label(pos);
        write_lea("rsi", "ex_numbuf");
        write_add((AsmDataField){ "rsi", "ptr", NULL }, (AsmDataField){ "24", "end", NULL });
        write_replace_mov("rbx", (AsmDataField){ "10", "ten", NULL });
        write_test("rax", "rax");
        write_jz(zero);
    write_label(dig);
        write_test("rax", "rax");
        write_jz(sign);
        write_xor((AsmDataField){ "rdx", "z", NULL }, (AsmDataField){ "rdx", "z", NULL });
        write_div("rbx");
        write_add((AsmDataField){ "dl", "digit", NULL }, (AsmDataField){ "'0'", "ascii", NULL });
        write_dec("rsi");
        write_replace_mov(asm_mem("byte", "rsi", 0), (AsmDataField){ "dl", "digit", NULL });
        write_jmp(dig);
    write_label(zero);
        write_dec("rsi");
        write_replace_mov(asm_mem("byte", "rsi", 0), (AsmDataField){ "'0'", "digit", NULL });
    write_label(sign);
        write_test("r8", "r8");
        write_jz(store);
        write_dec("rsi");
        write_replace_mov(asm_mem("byte", "rsi", 0), (AsmDataField){ "'-'", "sign", NULL });
    write_label(store);
        write_replace_mov(asm_mem("qword", "causal_nptr", 0), (AsmDataField){ "rsi", "ptr", NULL });
        write_lea("rax", "ex_numbuf");
        write_add((AsmDataField){ "rax", "end", NULL }, (AsmDataField){ "24", "end", NULL });
        write_sub((AsmDataField){ "rax", "nBytes", NULL }, (AsmDataField){ "rsi", "ptr", NULL });
        write_replace_mov(asm_mem("qword", "causal_nlen", 0), (AsmDataField){ "rax", "nBytes", NULL });
        write_pop("rdi");
        write_pop("rsi");
        write_pop("rbx");
        write_ret();
    write_endp(&causal_fmt);
}

/* e2e, loc, and effect for each function, in thousandths. See the file
 * comment. Volatile registers only: this runs inside main, which has
 * already saved the callee-saved set and has nothing live. */
static void causal_compute_effects(void) {
    for (int id = 0; id < causal_n; id++) {
        const char* e2e_zero = c_new_label();
        const char* e2e_store = c_new_label();
        const char* loc_div = c_new_label();
        const char* loc_zero = c_new_label();
        const char* loc_store = c_new_label();
        const char* fx_zero = c_new_label();
        const char* fx_store = c_new_label();
        write_xor((AsmDataField){ "r8", "z", NULL }, (AsmDataField){ "r8", "z", NULL });
        write_xor((AsmDataField){ "r9", "z", NULL }, (AsmDataField){ "r9", "z", NULL });
        write_xor((AsmDataField){ "r10", "z", NULL }, (AsmDataField){ "r10", "z", NULL });
        write_xor((AsmDataField){ "r11", "z", NULL }, (AsmDataField){ "r11", "z", NULL });
        for (int t = 0; t < CAUSAL_PER; t++) {
            int trial = id + t * causal_n;
            write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "causal_delta", trial * 8), "delta", NULL });
            write_sub((AsmDataField){ "rax", "extra", NULL },
                      (AsmDataField){ asm_mem("qword", "causal_base", trial * 8), "cycles", NULL });
            write_add((AsmDataField){ "r8", "extra", NULL }, (AsmDataField){ "rax", "extra", NULL });
            write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "causal_spin", trial * 8), "cycles", NULL });
            write_add((AsmDataField){ "r9", "spin", NULL }, (AsmDataField){ "rax", "spin", NULL });
            write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "causal_body", trial * 8), "cycles", NULL });
            write_add((AsmDataField){ "r10", "body", NULL }, (AsmDataField){ "rax", "body", NULL });
            write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "causal_base", trial * 8), "cycles", NULL });
            write_add((AsmDataField){ "r11", "base", NULL }, (AsmDataField){ "rax", "cycles", NULL });
        }
        write_cmp((AsmDataField){ "r11", "denom", NULL }, literal("0", "zero"));
        write_je(e2e_zero);
        write_replace_mov("rax", (AsmDataField){ "r8", "extra", NULL });
        write_imul3("rax", "rax", "1000");
        write_cqo();
        write_idiv("r11");
        write_jmp(e2e_store);
        write_label(e2e_zero);
        write_xor((AsmDataField){ "rax", "z", NULL }, (AsmDataField){ "rax", "z", NULL });
        write_label(e2e_store);
        write_replace_mov(asm_mem("qword", "causal_e2e", id * 8), (AsmDataField){ "rax", "e2e", NULL });

        write_cmp((AsmDataField){ "r10", "body", NULL }, literal("0", "zero"));
        write_jne(loc_div);
        write_cmp((AsmDataField){ "r9", "spin", NULL }, literal("0", "zero"));
        write_je(loc_zero);
        write_replace_mov("r10", (AsmDataField){ "1", "body", NULL });
        write_label(loc_div);
        write_replace_mov("rax", (AsmDataField){ "r9", "spin", NULL });
        write_imul3("rax", "rax", "1000");
        write_xor((AsmDataField){ "rdx", "z", NULL }, (AsmDataField){ "rdx", "z", NULL });
        write_div("r10");
        write_jmp(loc_store);
        write_label(loc_zero);
        write_xor((AsmDataField){ "rax", "z", NULL }, (AsmDataField){ "rax", "z", NULL });
        write_label(loc_store);
        write_replace_mov(asm_mem("qword", "causal_loc", id * 8), (AsmDataField){ "rax", "loc", NULL });

        write_cmp((AsmDataField){ asm_mem("qword", "causal_loc", id * 8), "loc", NULL },
                  literal("0", "zero"));
        write_je(fx_zero);
        write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "causal_e2e", id * 8), "e2e", NULL });
        write_imul3("rax", "rax", "1000");
        write_cqo();
        write_replace_mov("rcx", (AsmDataField){ asm_mem("qword", "causal_loc", id * 8), "loc", NULL });
        write_idiv("rcx");
        write_jmp(fx_store);
        write_label(fx_zero);
        write_xor((AsmDataField){ "rax", "z", NULL }, (AsmDataField){ "rax", "z", NULL });
        write_label(fx_store);
        write_replace_mov(asm_mem("qword", "causal_fx", id * 8), (AsmDataField){ "rax", "effect", NULL });
    }
}

/* order[] starts as 0, 1, ... and is sorted so the largest effect is
 * first. Equal effects are left alone, which keeps the earlier function. */
static void causal_rank(CValue* order) {
    CValue score[CAUSAL_MAX];
    causal_compute_effects();
    for (int id = 0; id < causal_n; id++) {
        score[id] = c_local_imm(causal_s_tag[id], C_PTR, "0");
        order[id] = c_local_imm(causal_i_tag[id], C_PTR, causal_id_imm[id]);
        /* The slot was stored as "effect". The score register keeps its
         * own tag; write_set_mov checks each side against the dictionary. */
        CValue fx = c_wrap((AsmDataField){
            asm_mem("qword", "causal_fx", id * 8), "effect", NULL }, C_PTR);
        c_assign(&score[id], fx);
    }
    for (int pass = 0; pass < causal_n - 1; pass++) {
        for (int j = 0; j < causal_n - 1 - pass; j++) {
            CIf sw = c_if_sgt(score[j + 1], score[j]);
                CValue td = c_dup("swap", C_PTR, score[j]);
                c_assign(&score[j], score[j + 1]);
                c_assign(&score[j + 1], td);
                c_free(&td);
                CValue ti = c_dup("swap", C_PTR, order[j]);
                c_assign(&order[j], order[j + 1]);
                c_assign(&order[j + 1], ti);
                c_free(&ti);
            c_end_if(&sw);
        }
    }
    for (int id = 0; id < causal_n; id++) c_free(&score[id]);
}

/* Write buf of length len to the handle h through con. */
static void causal_file_write(ExConsole* con, CValue h, AsmDataField buf, AsmDataField len) {
    c_call_ext_void(&con->write_file,
        c_cell(h), buf, len,
        literal("offset ex_written", "lpWritten"),
        literal("0", "lpOverlapped"));
}

/* Write one signed thousandth from causal_<which>[id*8], the qword of sym
 * at disp, through handle h and con. */
static void causal_file_number(ExConsole* con, CValue h, const char* sym, int disp) {
    /* The slot keeps the tag it was stored with (e2e, loc, effect).
     * causal_fmt's parameter is "value": same bits, the formatter's tag. */
    const char* loc = asm_mem("qword", sym, disp);
    const char* have = asm_type_lookup(loc);
    AsmDataField cell;
    if (have == NULL) {
        cell.name = loc;
        cell.type = "value";
        cell.use_as = NULL;
    } else {
        AsmDataField raw = { loc, have, NULL };
        cell = treat_as(raw, have, "value");
    }
    c_call_proc_void(&causal_fmt, cell);
    causal_file_write(con, h,
        (AsmDataField){ asm_mem("qword", "causal_nptr", 0), "lpBuffer", NULL },
        (AsmDataField){ asm_mem("qword", "causal_nlen", 0), "nBytes", NULL });
}

/* Write the ranked lines from order through con. A failed CreateFileA skips
 * the writes and still returns, so the caller can exit 0. */
static void causal_write_rank(ExConsole* con, CValue* order) {
    CValue h = c_call_ext(C_PTR, &causal_create_file,
        c_addr("c_file", "lpFileName"),
        literal("40000000h", "dwDesiredAccess"),
        literal("0", "dwShareMode"),
        literal("0", "lpSecurityAttributes"),
        literal("2", "dwCreationDisposition"),
        literal("80h", "dwFlagsAndAttributes"),
        literal("0", "hTemplateFile"));
    const char* opened = c_new_label();
    CIf bad = c_if_eq(h, c_imm("-1", C_PTR, "handle"));
        write_jmp(opened);
    c_end_if(&bad);
    for (int rank = 0; rank < causal_n; rank++) {
        for (int k = 0; k < causal_n; k++) {
            const char* skip = c_new_label();
            CValue idv = c_dup("rid", C_PTR, order[rank]);
            write_cmp(c_cell(idv), literal(causal_id_imm[k], "id"));
            c_free(&idv);
            write_jne(skip);
            causal_file_write(con, h, causal_name_str[k].off, causal_name_str[k].len);
            causal_file_write(con, h, causal_e2e_lit.off, causal_e2e_lit.len);
            causal_file_number(con, h, "causal_e2e", k * 8);
            causal_file_write(con, h, causal_loc_lit.off, causal_loc_lit.len);
            causal_file_number(con, h, "causal_loc", k * 8);
            causal_file_write(con, h, causal_fx_lit.off, causal_fx_lit.len);
            causal_file_number(con, h, "causal_fx", k * 8);
            causal_file_write(con, h, causal_end_lit.off, causal_end_lit.len);
            write_label(skip);
        }
    }
    c_call_ext_void(&causal_close, c_treat_as(h, "handle", "hObject"));
    write_label(opened);
    c_free(&h);
    for (int id = 0; id < causal_n; id++) c_free(&order[id]);
}

/* Time the test case with nothing selected by calling time. trial < 0 discards
 * the cycles (the warmup). Otherwise they are that trial's paired baseline. */
static void causal_baseline(AsmExtern* time, int trial) {
    write_replace_mov(asm_mem("qword", "causal_target", 0), (AsmDataField){ "-1", "id", NULL });
    write_replace_mov(asm_mem("qword", "causal_budget", 0), (AsmDataField){ "0", "cycles", NULL });
    CValue base = c_call_proc(C_PTR, time);
    if (trial >= 0) {
        write_replace_mov(asm_mem("qword", "causal_base", trial * 8), (AsmDataField){ c_cell(base).name, "cycles", NULL });
    }
    c_free(&base);
}

/*
 * Emit main.
 *
 * On: four unrecorded runs, then CAUSAL_PER trials of each function.
 * Each trial is preceded by its own baseline. The trial stores id,
 * delay, delta, the function's own cycles, and the pause that elapsed.
 * causal_rank.txt lists the functions by effect, with the end-to-end
 * and per-function proportional slowdowns. Then exit 0.
 *
 * Off: call the test case once and exit 0. No delay, no file, no label.
 */
static inline void causal_campaign(ExConsole* con) {
    crash_if(causal_n < 1, "causal: wrap at least one function");
    crash_if(causal_in_func || causal_in_case, "causal: close the test case first");
    crash_if(con == NULL, "causal: campaign needs the console");
    if (!causal_on) {
        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            c_call(&causal_case_fn);
            ex_exit(con, 0);
        c_end_function(&mainf);
        return;
    }
    causal_emit_names();
    causal_emit_fmt();
    AsmExtern time = causal_emit_time();
    CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
        for (int w = 0; w < 4; w++) causal_baseline(&time, -1);
        int ntrials = causal_n * CAUSAL_PER;
        for (int t = 0; t < ntrials; t++) {
            causal_baseline(&time, t);
            causal_arm(t, t % causal_n);
            CValue d = c_call_proc(C_PTR, &time);
            write_replace_mov(asm_mem("qword", "causal_delta", t * 8), (AsmDataField){ c_cell(d).name, "delta", NULL });
            c_free(&d);
            write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "causal_run_body", 0), "cycles", NULL });
            write_replace_mov(asm_mem("qword", "causal_body", t * 8), (AsmDataField){ "rax", "cycles", NULL });
            write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "causal_run_spin", 0), "cycles", NULL });
            write_replace_mov(asm_mem("qword", "causal_spin", t * 8), (AsmDataField){ "rax", "cycles", NULL });
        }
        CValue order[CAUSAL_MAX];
        causal_rank(order);
        causal_write_rank(con, order);
        ex_exit(con, 0);
    c_end_function(&mainf);
}
