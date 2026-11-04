/*
 * fuzz_beh.h - deterministic PCG programs checked against a host oracle.
 *
 * The generator is PCG-XSH-RR
 * (https://www.pcg-random.org/), the same stream as proximity_search.c:
 * state * 6364136223846793005 + increment, then the 32-bit xorshift-rotate.
 * The seed is fixed, so a failing run repeats. Included from beh_tests.h.
 *
 * fuzz_rng draws one word. fuzz_range maps a draw onto an inclusive span.
 * fuzz_u64 / fuzz_s32 / fuzz_imm_i turn host numbers into immediate text
 * the emitter can paste. fuzz_align_c_at is the independent ALIGN_C oracle.
 * fuzz_host_checks locks the first draw and checks align_pad. The emit
 * helpers then build one procedure each (ALU, loop, compares, switch,
 * struct load). fuzz_eq_check folds a result into the behavior program's
 * exit code. fuzz_reject_samples asks the generator to reject unsaved
 * preserved-register uses.
 */
static unsigned long long fuzz_pcg_state = 0x853c49e6748fea9bULL;
static const unsigned long long fuzz_pcg_inc = 0xda3e39cb94b95bdbULL;
static unsigned fuzz_pcg_draws = 0;

/* PCG-XSH-RR. State update, then xorshift, then a rotate by the top bits. */
static unsigned fuzz_rng(void) {
    unsigned long long old = fuzz_pcg_state;
    fuzz_pcg_state = old * 6364136223846793005ULL + fuzz_pcg_inc;
    unsigned xorshifted = (unsigned)(((old >> 18) ^ old) >> 27);
    unsigned rot = (unsigned)(old >> 59);
    fuzz_pcg_draws++;
    return (xorshifted >> rot) | (xorshifted << ((0u - rot) & 31u));
}

/* Inclusive span from first to last. A reversed span returns first and does not draw. */
static int fuzz_range(int first, int last) {
    if (first >= last) return first;
    return (int)(fuzz_rng() % (unsigned)(last - first + 1)) + first;
}

/* Decimal for an unsigned value. Arena-backed: write() keeps the pointer. */
static const char* fuzz_u64(unsigned long long v) {
    char rev[32];
    int n = 0;
    if (v == 0) rev[n++] = '0';
    while (v && n < 32) {
        rev[n++] = (char)('0' + (int)(v % 10));
        v /= 10;
    }
    char buf[32];
    int j = 0;
    while (n > 0) buf[j++] = rev[--n];
    buf[j] = 0;
    return c_strdup_arena(buf);
}

/* 32-bit bit pattern in bits as a signed immediate, so `mov r32, imm` fits. */
static const char* fuzz_s32(unsigned bits) {
    if (bits <= 2147483647u) return fuzz_u64(bits);
    unsigned mag = (~bits) + 1u;
    char buf[16];
    snprintf(buf, sizeof buf, "-%u", mag);
    return c_strdup_arena(buf);
}

/* Signed decimal, arena-backed. `mov rax, -3` sign-extends. */
static const char* fuzz_imm_i(int v) {
    if (v >= 0) return fuzz_u64((unsigned long long)v);
    char buf[16];
    snprintf(buf, sizeof buf, "%d", v);
    return c_strdup_arena(buf);
}

/* ALIGN_C offset after placing a field of size at off. Alignment is size, at most 8. */
static int fuzz_align_c_at(int off, int size) {
    int n = size > 8 ? 8 : size;
    if (n < 1) n = 1;
    int rem = n > 1 ? (off % n) : 0;
    return rem ? off + (n - rem) : off;
}

/* First draw must be 0x152ca78d. Then 80 align_pad checks. */
static void fuzz_host_checks(void) {
    unsigned first = fuzz_rng();
    if (first != 0x152ca78du) {
        printf("  [FAIL] pcg first draw %u\n", first);
        expect_true(0);
    }
    expect_true(align_pad(0, 4, ALIGN_NONE) == 0);
    expect_true(align_pad(-1, 4, ALIGN_C) == 0);

    static const int sizes[] = {1, 2, 3, 4, 5, 8, 16};
    for (int i = 0; i < 80; i++) {
        int off = fuzz_range(0, 9000);
        int size = sizes[fuzz_range(0, 6)];
        int none = align_pad(off, size, ALIGN_NONE);
        if (none != 0) {
            printf("  [FAIL] ALIGN_NONE off %d size %d pad %d draw %u\n",
                   off, size, none, fuzz_pcg_draws);
            expect_true(0);
        }
        int cpad = align_pad(off, size, ALIGN_C);
        int cat = fuzz_align_c_at(off, size);
        if (off + cpad != cat) {
            printf("  [FAIL] ALIGN_C off %d size %d pad %d expect %d draw %u\n",
                   off, size, cpad, cat - off, fuzz_pcg_draws);
            expect_true(0);
        }
        int cache = align_pad(off, size, ALIGN_CACHE);
        int at = off + cache;
        if (cache < cpad || (size < 64 && (at % 64) + size > 64)) {
            printf("  [FAIL] ALIGN_CACHE off %d size %d pad %d draw %u\n",
                   off, size, cache, fuzz_pcg_draws);
            expect_true(0);
        }
        int page = align_pad(off, size, ALIGN_PAGE);
        at = off + page;
        if (page < cpad || (size < 4096 && (at % 4096) + size > 4096)) {
            printf("  [FAIL] ALIGN_PAGE off %d size %d pad %d draw %u\n",
                   off, size, page, fuzz_pcg_draws);
            expect_true(0);
        }
    }
}

static int fuzz_struct_off = 0;

/* Three random-width fields, then a qword 44. The load uses an offset this
 * file computed, not align_pad, so a padding bug reads the wrong bytes. */
static void fuzz_emit_struct_data(void) {
    CStruct s = c_struct_begin("fzblob", "0");
    c_struct_set_align(&s, ALIGN_C);
    static const char* names[] = {"", "sb1", "sb2", "sq"};
    static const char* vals[] = {"11", "22", "33", "44"};
    static const char* tags[] = {"b0", "b1", "b2", "sq"};
    int off = 0;
    for (int i = 0; i < 4; i++) {
        int size = (i == 3) ? 8 : (1 << fuzz_range(0, 2));
        CType ty = C_CHAR;
        if (size == 2) ty = C_SHORT;
        else if (size == 4) ty = C_INT;
        else if (size == 8) ty = C_LONG;
        off = fuzz_align_c_at(off, size);
        if (i == 3) fuzz_struct_off = off;
        c_struct_field(&s, names[i], vals[i], tags[i], ty);
        off += size;
    }
    (void)c_struct_emit(&s);
}

/* Host twin of one ALU step. acc and src are the operands and sh is the shift.
 * bits is 32 or 64. op 0..8 matches the emitter. */
static unsigned long long fuzz_apply(int op, unsigned long long acc,
                                     unsigned long long src, int sh, int bits) {
    unsigned long long mask = (bits == 32) ? 0xffffffffull : ~0ull;
    acc &= mask;
    src &= mask;
    unsigned long long r = acc;
    if (op == 0) r = acc + src;
    else if (op == 1) r = acc - src;
    else if (op == 2) r = acc & src;
    else if (op == 3) r = acc | src;
    else if (op == 4) r = acc ^ src;
    else if (op == 5) r = acc * src;
    else if (op == 6) r = acc << sh;
    else if (op == 7) r = ~acc;
    else r = 0;
    return (bits == 32) ? (r & mask) : r;
}

static const int fuzz_pool[] = {
    0, 1, -1, 41, -41, 2000000000, -2000000000, 2147483647, -2147483647
};
static const char* fuzz_sh[] = {"0", "1", "2", "3", "4", "5", "6", "7"};

/* nlive locals from names, then steps of add/sub/and/or/xor/imul/shl/not/zero.
 * bits is 32 or 64. fn is the procedure and acc_name is the accumulator.
 * Some steps call clobber so a spill restore sits on the path.
 * expect receives the host result. */
static CFunc fuzz_emit_alu(CFunc* clobber, int bits, const char* fn,
                           const char* acc_name, const char** names, int nlive,
                           int steps, const char** expect) {
    CType ty = (bits == 32) ? C_INT : C_LONG;
    CFunc f = c_begin_function(fn, NULL, 0, ty, acc_name);
    unsigned long long regs[6];
    CValue vars[6];
    unsigned long long acc = 0;
    for (int i = 0; i < nlive; i++) {
        int iv = (bits == 32) ? fuzz_pool[fuzz_range(0, 8)] : fuzz_range(0, 1023);
        unsigned bitsv = (unsigned)iv;
        regs[i] = (bits == 32) ? bitsv : (unsigned long long)iv;
        const char* text = (bits == 32) ? fuzz_s32(bitsv) : fuzz_u64(regs[i]);
        vars[i] = c_local_imm(names[i], ty, text);
        if (i == 0) acc = regs[0];
    }
    CValue accv = c_local_imm(acc_name, ty, (bits == 32) ? fuzz_s32((unsigned)acc)
                                                         : fuzz_u64(acc));
    for (int step = 0; step < steps; step++) {
        int op = fuzz_range(0, 8);
        int si = fuzz_range(0, nlive - 1);
        int sh = fuzz_range(0, 7);
        unsigned long long src = regs[si];
        CValue srcv = vars[si];
        if (op == 0) c_add(&accv, srcv);
        else if (op == 1) c_sub(&accv, srcv);
        else if (op == 2) c_and(&accv, srcv);
        else if (op == 7) {
            CValue d = c_resolve(accv);
            write_not(d.loc);
        } else if (op == 8) c_zero(&accv);
        else {
            CValue d = c_resolve(accv);
            CValue s = c_resolve(srcv);
            if (op == 3) write_or(d.loc, s.loc);
            else if (op == 4) write_xor((AsmDataField){ d.loc, acc_name, NULL },
                                        (AsmDataField){ s.loc, names[si], NULL });
            else if (op == 5) write_imul2(d.loc, s.loc);
            else write_shl(d.loc, fuzz_sh[sh]);
        }
        acc = fuzz_apply(op, acc, src, sh, bits);
        if (fuzz_range(0, 3) == 0) c_call(clobber);
    }
    c_ret_value(&f, accv);
    c_end_function(&f);
    *expect = (bits == 32) ? fuzz_s32((unsigned)acc) : fuzz_u64(acc);
    return f;
}

/* Sum 0 + 1 + ... + (K-1) for a random K in 1..16. expect receives that sum. */
static CFunc fuzz_emit_loop(const char** expect) {
    int k = fuzz_range(1, 16);
    CFunc f = c_begin_function("fuzz_loop", NULL, 0, C_LONG, "lsum");
    CValue sum = c_local_imm("lsum", C_LONG, "0");
    CValue one = c_local_imm("lone", C_LONG, "1");
    CValue ctr = c_local_imm("lctr", C_LONG, "0");
    CValue lim = c_local_imm("llim", C_LONG, fuzz_u64((unsigned long long)k));
    CLoop loop = c_loop_begin();
        CIf br = c_if_uge(ctr, lim);
            c_loop_break(&loop);
        c_end_if(&br);
        c_add(&sum, ctr);
        c_add(&ctr, one);
    c_loop_end(&loop);
    c_ret_value(&f, sum);
    c_end_function(&f);
    unsigned long long total = (unsigned long long)(k - 1) * (unsigned long long)k / 2ull;
    *expect = fuzz_u64(total);
    return f;
}

/* Six compares. Each taken arm writes 1 into a stack slot that started at 2.
 * The mix is the six results packed two bits at a time. expect receives the mix. */
static CFunc fuzz_emit_branch(const char** expect) {
    static const char* an[] = {"ba0", "ba1", "ba2", "ba3", "ba4", "ba5"};
    static const char* bn[] = {"bb0", "bb1", "bb2", "bb3", "bb4", "bb5"};
    static const char* sn[] = {"bs0", "bs1", "bs2", "bs3", "bs4", "bs5"};
    CFunc f = c_begin_function("fuzz_branch", NULL, 0, C_LONG, "bmix");
    CValue mix = c_local_imm("bmix", C_LONG, "0");
    unsigned long long oracle = 0;
    for (int i = 0; i < 6; i++) {
        int a = fuzz_range(-30, 40);
        int b = fuzz_range(-30, 40);
        int kind = fuzz_range(0, 2);
        int taken;
        if (kind == 0) taken = (a == b);
        else if (kind == 1) {
            unsigned long long ua = (unsigned long long)(long long)a;
            unsigned long long ub = (unsigned long long)(long long)b;
            taken = ua > ub;
        } else taken = (a > b);
        int r = taken ? 1 : 2;
        CValue va = c_local_imm(an[i], C_LONG, fuzz_imm_i(a));
        CValue vb = c_local_imm(bn[i], C_LONG, fuzz_imm_i(b));
        CValue slot = c_stack_local(sn[i], C_LONG, ALIGN_C);
        c_materialize(slot.loc, "2", sn[i]);
        if (kind == 0) {
            CIf cif = c_if_eq(va, vb);
                c_assign_imm(&slot, "1");
            c_end_if(&cif);
        } else if (kind == 1) {
            CIf cif = c_if_ugt(va, vb);
                c_assign_imm(&slot, "1");
            c_end_if(&cif);
        } else {
            CIf cif = c_if_sgt(va, vb);
                c_assign_imm(&slot, "1");
            c_end_if(&cif);
        }
        c_free(&va);
        c_free(&vb);
        c_add(&mix, mix);
        c_add(&mix, mix);
        c_add(&mix, slot);
        oracle = oracle * 4ull + (unsigned long long)r;
    }
    c_ret_value(&f, mix);
    c_end_function(&f);
    *expect = fuzz_u64(oracle);
    return f;
}

/* Switch on a selector in 0..3. The default arm is selector 3.
 * expect receives the selected arm's value. */
static CFunc fuzz_emit_switch(const char** expect) {
    static const char* digs[] = {"0", "1", "2", "3"};
    static const char* outs[] = {"10", "20", "30", "40"};
    int sel = fuzz_range(0, 3);
    CFunc f = c_begin_function("fuzz_switch", NULL, 0, C_LONG, "fout");
    CValue s = c_local_imm("fsel", C_LONG, digs[sel]);
    CValue out = c_stack_local("fout", C_LONG, ALIGN_C);
    c_materialize(out.loc, "0", "fout");
    CSwitch sw = c_switch(s);
        c_case(&sw, "0"); c_assign_imm(&out, "10"); c_break(&sw);
        c_case(&sw, "1"); c_assign_imm(&out, "20"); c_break(&sw);
        c_case(&sw, "2"); c_assign_imm(&out, "30"); c_break(&sw);
        c_default(&sw);   c_assign_imm(&out, "40"); c_break(&sw);
    c_end_switch(&sw);
    c_ret_value(&f, out);
    c_end_function(&f);
    *expect = outs[sel];
    return f;
}

/* Load the trailing qword of fzblob from the offset fuzz_emit_struct_data stored. */
static CFunc fuzz_emit_struct_fn(void) {
    CFunc f = c_begin_function("fuzz_struct", NULL, 0, C_LONG, "sq");
    const char* mem = asm_mem("qword", "fzblob", fuzz_struct_off);
    CValue q = c_wrap((AsmDataField){ mem, "sq", NULL }, C_LONG);
    c_ret_value(&f, q);
    c_end_function(&f);
    return f;
}

/* If code is still 0 and got is not imm, set code to id.
 * imm is held in a local named slot of type ty. */
static void fuzz_eq_check(CValue* code, const char* id, CValue got,
                          const char* imm, const char* slot, CType ty) {
    CValue expect = c_local_imm(slot, ty, imm);
    CIf open = c_if_eq(*code, c_imm("0", C_LONG, "code"));
        CElse arm = c_if_eq_else(got, expect);
        c_else(&arm);
            c_assign_imm(code, id);
        c_end_if_else(&arm);
    c_end_if(&open);
    c_free(&got);
    c_free(&expect);
}

static void beh_expect_fail(const char* needle, void (*fn)(void));

static unsigned fuzz_bad_mask = 0;
static const char* fuzz_bad_reg = "rbx";
static const char* fuzz_rej_callee = "bad_callee_r";
static const char* fuzz_rej_caller = "bad_caller_r";
static const char* fuzz_rej_write = "bad_write_r";

/* A callee whose volatile mask hits a preserved register this caller did not save. */
static void fail_volatile_rand(void) {
    AsmExtern callee = write_proc(fuzz_rej_callee, NULL, 0, asm_void);
    callee.volatile_mask = fuzz_bad_mask;
    write_ret();
    write_endp(&callee);
    AsmExtern caller = write_proc(fuzz_rej_caller, NULL, 0, asm_void);
    asm_set_preserved(fuzz_bad_mask);
    write_call(&callee);
    write_endp(&caller);
}

/* A write to a preserved register this procedure did not save. */
static void fail_unsaved_rand(void) {
    AsmExtern p = write_proc(fuzz_rej_write, NULL, 0, asm_void);
    asm_set_preserved(fuzz_bad_mask);
    write_replace_mov(fuzz_bad_reg, (AsmDataField){ "0", "tmp", NULL });
    write_endp(&p);
}

/* Unique procedure name from prefix and n so eight samples can sit in one generation. */
static const char* fuzz_rej_name(const char* prefix, int n) {
    char buf[40];
    snprintf(buf, sizeof buf, "%s_%d", prefix, n);
    return c_strdup_arena(buf);
}

/* Eight draws. Even samples are unsaved calls, odd samples are unsaved writes. */
static void fuzz_reject_samples(void) {
    static const int bits[] = {3, 5, 6, 7, 12, 13, 14, 15};
    static const char* regs[] = {
        "rbx", "rbp", "rsi", "rdi", "r12", "r13", "r14", "r15"
    };
    for (int i = 0; i < 8; i++) {
        int k = fuzz_range(0, 7);
        fuzz_bad_mask = 1u << bits[k];
        fuzz_bad_reg = regs[k];
        printf("  fuzz reject %s\n", regs[k]);
        if ((i & 1) == 0) {
            fuzz_rej_callee = fuzz_rej_name("bad_callee_r", i);
            fuzz_rej_caller = fuzz_rej_name("bad_caller_r", i);
            beh_expect_fail("call clobbers a non-volatile", fail_volatile_rand);
        } else {
            fuzz_rej_write = fuzz_rej_name("bad_write_r", i);
            beh_expect_fail("write to a non-volatile", fail_unsaved_rand);
        }
    }
}
