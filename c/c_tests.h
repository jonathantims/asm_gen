/*
 * c_tests.h - generation-time checks for the C layer.
 * Included from test/test.c after asm_tests.h and c_funcs.h.
 *
 * c_fuzz_rng is an LCG with the PCG default multiplier
 * (6364136223846793005, https://www.pcg-random.org/) and a 64-bit rotate.
 * It is not the 32-bit PCG-XSH-RR stream used by fuzz_beh.h and gc_fuzz.h.
 */

static unsigned long long c_fuzz_state = 0xC0FFEEULL;

/* One draw. The multiplier is the PCG default; the increment is the
 * Numerical Recipes LCG increment, not the PCG stream increment. */
static unsigned long long c_fuzz_rng(void) {
    c_fuzz_state = c_fuzz_state * 6364136223846793005ULL + 1442695040888963407ULL;
    unsigned long long x = c_fuzz_state;
    unsigned rot = (unsigned)(x >> 59);
    x ^= x >> 18;
    x = (x >> rot) | (x << ((64 - rot) & 63));
    return x;
}

/* Check width names for drawn register sizes, the spill enable flag, and
 * align_pad of a 4-byte field before an 8-byte field. */
static void c_run_fuzz(void) {
    int saw1 = 0, saw2 = 0, saw4 = 0, saw8 = 0, saw_spill = 0;
    static const int sizes[] = {1, 2, 4, 8};
    for (int i = 0; i < 64; i++) {
        int size = sizes[c_fuzz_rng() % 4];
        int w = c_width_index(size);
        if (size == 1) { expect_true(w == 0); saw1 = 1; }
        if (size == 2) { expect_true(w == 1); saw2 = 1; }
        if (size == 4) { expect_true(w == 2); saw4 = 1; }
        if (size == 8) { expect_true(w == 3); saw8 = 1; }
        expect_true(strcmp(c_reg_name(0, size), c_regs[0].names[w]) == 0);
    }
    int was = c_spill_enabled;
    c_spill_off();
    expect_true(c_spill_enabled == 0);
    c_spill_on();
    expect_true(c_spill_enabled == 1);
    c_spill_enabled = was;
    saw_spill = 1;
    expect_true(saw1 && saw2 && saw4 && saw8 && saw_spill);

    /* Known Win32 layout: a 4-byte field then an 8-byte field under ALIGN_C
     * inserts 4 bytes of pad. ALIGN_NONE inserts none. */
    expect_true(align_pad(4, 8, ALIGN_C) == 4);
    expect_true(align_pad(4, 8, ALIGN_NONE) == 0);
}

/* Register names, c_mark_const, and c_cell of a disposed value.
 * Nothing here writes asm_output. The disposed check is a flag on the
 * CValue, so this does not call c_dispose and does not touch the asm
 * disposed list. */
static void c_run_unit_tests(void) {
    static const char* qnames[] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    int i;
    for (i = 0; i < 16; i++) {
        expect_true(c_reg_index(qnames[i]) == i);
        expect_true(c_reg_index(c_regs[i].names[0]) == i);
        expect_true(c_reg_index(c_regs[i].names[1]) == i);
        expect_true(c_reg_index(c_regs[i].names[2]) == i);
        expect_true(c_reg_index(c_regs[i].names[3]) == i);
        expect_true(strcmp(c_reg_name(i, 1), c_regs[i].names[0]) == 0);
        expect_true(strcmp(c_reg_name(i, 2), c_regs[i].names[1]) == 0);
        expect_true(strcmp(c_reg_name(i, 4), c_regs[i].names[2]) == 0);
        expect_true(strcmp(c_reg_name(i, 8), c_regs[i].names[3]) == 0);
    }
    expect_true(c_width_index(1) == 0);
    expect_true(c_width_index(2) == 1);
    expect_true(c_width_index(4) == 2);
    expect_true(c_width_index(8) == 3);
    expect_true(c_reg_index("nope") < 0);

    {
        CValue bare = { "limit", C_INT, "rax", "limit", -1, true, -1 };
        expect_true(bare.id == 0);
        expect_true(!bare.is_const);
        c_mark_const(&bare);
        expect_true(bare.is_const);
    }

    {
        HostJmpBuf env;
        CValue dead = { "scratch", C_INT, "rcx", "scratch", -1, true, -1 };
        dead.disposed = true;
        host_fail_hit = 0;
        host_fail_msg[0] = 0;
        host_fail_env = &env;
        if (_setjmp(&env) == 0) {
            (void)c_cell(dead);
            host_fail_env = 0;
        }
        expect_true(host_fail_hit);
        expect_true(strstr(host_fail_msg, "disposed") != NULL);
        host_fail_env = 0;
        host_fail_hit = 0;
    }
}
