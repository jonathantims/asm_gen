/*
 * asm_tests.h - generation-time fuzz for the asm layer.
 * Included from test/test.c after host.h, asm.h, and asm_funcs.h.
 * Single-threaded, like the generator.
 * A failed expectation ends the process with the source line, so a skipped
 * case cannot pass by accident.
 *
 * asm_fuzz_rng uses a PCG default multiplier
 * (https://www.pcg-random.org/) with a 64-bit rotate. The output is not
 * the 32-bit PCG-XSH-RR stream in fuzz_beh.h.
 */

static unsigned long long asm_fuzz_state = 0xA5A5A5A5ULL;

/* One draw. Multiplier: PCG default. Increment: Numerical Recipes LCG. */
static unsigned long long asm_fuzz_rng(void) {
    asm_fuzz_state = asm_fuzz_state * 6364136223846793005ULL + 1442695040888963407ULL;
    unsigned long long x = asm_fuzz_state;
    unsigned rot = (unsigned)(x >> 59);
    x ^= x >> 18;
    x = (x >> rot) | (x << ((64 - rot) & 63));
    return x;
}

/* Check align_pad for ALIGN_NONE, ALIGN_C, ALIGN_CACHE, and ALIGN_PAGE, plus
 * register-name classification, call-frame sizes, and snprintf. A failed
 * expectation ends the process. */
static void asm_run_fuzz(void) {
    int saw_none = 0, saw_c = 0, saw_cache = 0, saw_page = 0;
    int saw_frame8 = 0, saw_frame0 = 0, saw_reg = 0, saw_mem = 0;
    for (int i = 0; i < 300; i++) {
        int off = (int)(asm_fuzz_rng() % 8000);
        int size = 1 << (int)(asm_fuzz_rng() % 4);
        int none = align_pad(off, size, ALIGN_NONE);
        expect_true(none == 0);
        saw_none = 1;

        int cpad = align_pad(off, size, ALIGN_C);
        int natural = size > 8 ? 8 : size;
        expect_true(cpad >= 0 && cpad < natural);
        if (natural > 1) expect_true(((off + cpad) % natural) == 0);
        saw_c = 1;

        int cache = align_pad(off, size, ALIGN_CACHE);
        int at = off + cache;
        expect_true(cache >= cpad);
        if (size < 64) expect_true((at % 64) + size <= 64);
        saw_cache = 1;

        int page = align_pad(off, size, ALIGN_PAGE);
        at = off + page;
        expect_true(page >= cpad);
        if (size < 4096) expect_true((at % 4096) + size <= 4096);
        saw_page = 1;

        if (asm_name_is_register("rcx") && asm_name_is_register("r15")) saw_reg = 1;
        if (!asm_name_is_register("qword ptr [rsp+40]") &&
            !asm_name_is_register("0")) saw_mem = 1;
    }
    int saved = asm_rsp_mod16;
    asm_rsp_mod16 = 8;
    expect_true(asm_call_frame_size(32) == 40);
    expect_true(asm_call_frame_size(40) == 40);
    saw_frame8 = 1;
    asm_rsp_mod16 = 0;
    expect_true(asm_call_frame_size(32) == 32);
    expect_true(asm_call_frame_size(40) == 48);
    saw_frame0 = 1;
    asm_rsp_mod16 = saved;

    expect_true(saw_none && saw_c && saw_cache && saw_page);
    expect_true(saw_frame8 && saw_frame0 && saw_reg && saw_mem);

    char buf[64];
    expect_true(snprintf(buf, sizeof buf, "%s %d %u", "ok", -3, 4u) > 0);
    expect_true(strcmp(buf, "ok -3 4") == 0);
}
