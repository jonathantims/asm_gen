/*
 * gc_fuzz.h - one fixed-seed graph, appended to gc_demo so every language
 * tour already assembled for a collector runs it. No extra ml64 program.
 *
 * The generator is PCG-XSH-RR
 * (https://www.pcg-random.org/), the same stream as proximity_search.c
 * (state 0x853c49e6748fea9b, increment 0xda3e39cb94b95bdb, 32-bit
 * xorshift-rotate). The first draw is 0x152ca78d. Payloads are small
 * integers, so a conservative stack scan cannot mistake them for heap
 * pointers.
 *
 * Shape, forced so the trace is not optional: n0 is the only root,
 * n0.next = n1, n1.next = n2, and n2.next is either null or n0. n1 and n2
 * are reachable only through that link. Scalar words are stored with a
 * plain mov. gc_set is reserved for the pointer fields, because the
 * refcount barrier treats the stored word as an object.
 *
 * After gc_collect, while n0 is still rooted:
 *   the three values and the tail link match the host draws;
 *   refcount meta words are the exact counts (a freed block's meta is the
 *   object size, which is not a count we expect);
 *   mark-sweep and conservative meta words are 0 (a freed block's meta is
 *   its size);
 *   the compactor's root address changed, and n0.next is not the stale
 *   pre-move slot of n1.
 *
 * n1 and n2's registers are cleared before collect so the conservative
 * scanner does not keep them as roots. Their compact slots are left
 * holding the old address on purpose: those slots were not published with
 * gc_root, so a correct forward of the field disagrees with the slot.
 * gc_scope_end is not called. Refcount would drop n0 and cascade.
 *
 * After n0's payload is stored, and before n1 and n2 are allocated, the
 * graph copies n0 through gc_weak_new / gc_weak_get and checks that the
 * copy still sees the payload (die 32). The weak register is then cleared
 * so a later collect does not keep it as a root. That check does not show
 * a weak clearing when its target dies. gc_pin is called on n0 before
 * collect and gc_unpin after the walk. Both are no-ops, so a pin that
 * stops a move is not shown. The graph does not fill the heap, and it does
 * not assert that an unreachable object was freed. Three nodes is the cap:
 * the C# tour holds four callee-saved registers until a, b, and c are
 * released. GC_CYCLES=trial still has an empty gc_collect. GC_COALESCE
 * runs this survival check and does not check a neighbour merge.
 * GC_MOVE=sliding is still the copying collector. GC_SCAN=full keeps a
 * child whose address appears anywhere in the heap, so it does not prove
 * the descriptor walk. A failed check is ExitProcess of 21 through 32
 * (payloads, links, meta, the two compact checks, and the weak copy).
 */

static unsigned long long gc_pcg_state = 0x853c49e6748fea9bULL;
static const unsigned long long gc_pcg_inc = 0xda3e39cb94b95bdbULL;

/* PCG-XSH-RR. Same multiplier and output mix as fuzz_rng in fuzz_beh.h. */
static unsigned gc_rng(void) {
    unsigned long long old = gc_pcg_state;
    gc_pcg_state = old * 6364136223846793005ULL + gc_pcg_inc;
    unsigned xorshifted = (unsigned)(((old >> 18) ^ old) >> 27);
    unsigned rot = (unsigned)(old >> 59);
    return (xorshifted >> rot) | (xorshifted << ((0u - rot) & 31u));
}

/* Inclusive span from first through last. A reversed span returns first and does not draw. */
static int gc_frange(int first, int last) {
    if (first >= last) return first;
    return (int)(gc_rng() % (unsigned)(last - first + 1)) + first;
}

/* How many callee-saved registers the tour has not yet claimed.
 * The non-compact graph needs three for n0, n1, and n2. */
static int gc_saved_free(void) {
    int n = 0;
    int nregs = (int)(sizeof(gc_saved_regs) / sizeof(gc_saved_regs[0]));
    for (int i = 0; i < nregs; i++) {
        int idx = c_reg_index(gc_saved_regs[i]);
        if (idx >= 0 && !c_regs[idx].reserved && !c_regs[idx].in_use)
            n++;
    }
    return n;
}

/* Drop a demo binding the collector tour is finished with. Zero the
 * register so the conservative scan does not keep the abandoned object.
 * A compact slot is not a register; clearing it is still safe because the
 * demo already popped that root. */
static void gc_release_demo(CValue* v) {
    CValue c = c_resolve(*v);
    if (c.loc == NULL || c.loc[0] == 0) return;
    write_replace_mov(c.loc, (AsmDataField){ "0", "obj", NULL });
    if (c.reg >= 0) c_free(v);
}

/* Immediate 0 tagged null. Not a heap object. */
static CValue gc_null(void) {
    CValue v = { "null", C_PTR, "0", "null", -1, true, -1 };
    return v;
}

/* Plain store of non-pointer word value at field off of obj. gc_set increfs its word. */
static void gc_set_word(CValue obj, int off, int value) {
    const char* p = gc_load_ptr(obj, "rax");
    write_replace_mov(gc_mem(p, off), (AsmDataField){ gc_imm_i(value), "word", NULL });
}

/* ExitProcess(code). 40 bytes of shadow so the import stub has home slots. */
static void gc_die(int code) {
    /* Shadow space: the import stub writes its home slots. Without the
     * 32 bytes the call faults instead of exiting. */
    AsmExtern ep = { "ExitProcess", NULL, 0, asm_void, 1 };
    write_sub(gc_op("rsp"), literal("40", "shadow"));
    write_replace_mov("rcx", (AsmDataField){ gc_imm_i(code), "code", NULL });
    write_call(&ep);
}

/* Exit `code` when got != expect. */
static void gc_die_if_ne(const char* got, const char* expect, int code) {
    write_cmp(gc_op(got), literal(expect, "expect"));
    const char* ok = c_new_label();
    write_je(ok);
    gc_die(code);
    write_label(ok);
}

/* Exit `code` when got == expect. */
static void gc_die_if_eq(const char* got, const char* expect, int code) {
    write_cmp(gc_op(got), literal(expect, "expect"));
    const char* ok = c_new_label();
    write_jne(ok);
    gc_die(code);
    write_label(ok);
}

/* Park the object's current address in rdx. Compact reloads the slot. */
static void gc_cursor(CValue obj) {
    const char* p = gc_load_ptr(obj, "rdx");
    if (strcmp(p, "rdx") != 0)
        write_replace_mov("rdx", (AsmDataField){ p, "obj", NULL });
}

/* Load [rdx+off] and die `code` when it is not expect. rdx is the cursor. */
static void gc_check_word(int off, int expect, int code) {
    write_replace_mov("rcx", (AsmDataField){ gc_mem("rdx", off), "field", NULL });
    gc_die_if_ne("rcx", gc_imm_i(expect), code);
}

/* rdx = [rdx+GC_HDR], the next pointer. */
static void gc_advance(void) {
    write_replace_mov("rdx", (AsmDataField){ gc_mem("rdx", GC_HDR), "next", NULL });
}

/* Meta word the walk must see. Refcount: the live count. Everyone else:
 * 0, because a survivor's mark has been cleared and a moved copy's meta
 * was 0 when it was copied. A freed block stores its byte size there. */
static int gc_meta_of(int refs) {
#if GC_SELECTED == GCV_refcount
    return refs;
#else
    (void)refs;
    return 0;
#endif
}

/* Copy obj through a weak ref and die code when the word at off is not expect.
 * heap is passed to gc_weak_new. The saved register is cleared before return
 * so collect does not treat it as a root. A tour with no free saved register
 * skips the check. */
static void gc_probe_weak(GcHeap* heap, CValue obj, int off, int expect, int code) {
    if (gc_saved_free() < 1) return;
    CValue weak = gc_weak_new(heap, obj, "gweak");
    CValue got = gc_weak_get(heap, weak, "gweak");
    gc_cursor(got);
    gc_check_word(off, expect, code);
    write_replace_mov(got.loc, (AsmDataField){ "0", "weak", NULL });
    if (got.reg >= 0) {
        c_regs[got.reg].in_use = false;
        c_reg_owner[got.reg] = NULL;
        c_reg_owner_name[got.reg] = NULL;
    }
}

/* Build n0->n1->n2 of type on heap, optionally a cycle, collect, and check the walk.
 * demo_a, demo_b, and demo_c are cleared first. Die codes 21..26 are payloads and
 * links, 27..29 are meta words, 30 and 31 are the two compact checks, and 32 is
 * the weak copy of n0's payload. */
static void gc_fuzz(GcHeap* heap, const GcType* type,
                    CValue* demo_a, CValue* demo_b, CValue* demo_c) {
    unsigned first = gc_rng();
    if (first != 0x152ca78du) {
        printf("gc fuzz: first draw %u\n", first);
        exit(1);
    }

    gc_release_demo(demo_a);
    gc_release_demo(demo_b);
    gc_release_demo(demo_c);

#if GC_SELECTED != GCV_compact
    {
        int free_n = gc_saved_free();
        if (free_n < 3) {
            printf("gc fuzz: %d callee-saved registers free, need 3\n", free_n);
            exit(1);
        }
    }
#endif

    int val[3] = { 0, 0, 0 };
    for (int i = 0; i < 3; i++) {
        int v;
        int guard = 0;
        for (;;) {
            int clash = 0;
            v = gc_frange(1, 1023);
            for (int j = 0; j < i; j++) if (val[j] == v) clash = 1;
            if (!clash) break;
            if (++guard > 32) {
                printf("gc fuzz: could not draw three payloads\n");
                exit(1);
            }
        }
        val[i] = v;
    }
    int cycle = (gc_rng() & 1u) != 0;
    printf("gc fuzz %s values %d %d %d cycle %d\n",
           GC_OPTION_NAME, val[0], val[1], val[2], cycle);

    static const char* names[3] = { "gf0", "gf1", "gf2" };
    CValue g[3];
    g[0] = gc_new(heap, type, names[0]);
    gc_set_word(g[0], GC_HDR + 8, val[0]);
    gc_probe_weak(heap, g[0], GC_HDR + 8, val[0], 32);
    for (int i = 1; i < 3; i++)
        g[i] = gc_new(heap, type, names[i]);
    for (int i = 1; i < 3; i++)
        gc_set_word(g[i], GC_HDR + 8, val[i]);

    gc_set(heap, g[0], GC_HDR, g[1]);
    gc_set(heap, g[1], GC_HDR, g[2]);
    if (cycle) gc_set(heap, g[2], GC_HDR, g[0]);
    else       gc_set(heap, g[2], GC_HDR, gc_null());

    GcScope sc = gc_scope_begin();
    gc_root(&sc, &g[0]);

    /* n1 and n2 must not be roots. Registers are cleared. Compact slots
     * keep the pre-move address; see the file comment. */
#if GC_SELECTED != GCV_compact
    gc_release_demo(&g[1]);
    gc_release_demo(&g[2]);
#endif

#if GC_SELECTED == GCV_compact
    /* gc_hexbuf is scratch the tour has already finished printing with.
     * It is not a root slot, so collect will not rewrite it. */
    {
        const char* p = gc_load_ptr(g[0], "rax");
        write_replace_mov(gc_glob("gc_hexbuf"), (AsmDataField){ p, "obj", NULL });
    }
#endif

    gc_pin(&g[0]);
    gc_collect(heap);

#if GC_SELECTED == GCV_compact
    {
        const char* p = gc_load_ptr(g[0], "rax");
        write_replace_mov("rdx", (AsmDataField){ gc_glob("gc_hexbuf"), "obj", NULL });
        gc_die_if_eq("rdx", p, 31);
    }
#endif

    gc_cursor(g[0]);
    gc_check_word(GC_OFF_META, gc_meta_of(cycle ? 2 : 1), 27);
    gc_check_word(GC_HDR + 8, val[0], 21);
    gc_advance();
    gc_die_if_eq("rdx", "0", 22);

#if GC_SELECTED == GCV_compact
    /* n0.next was forwarded. g[1]'s slot was not a root, so it still
     * holds the from-space address. The two must differ. */
    write_replace_mov("rcx", (AsmDataField){ g[1].loc, "old", NULL });
    gc_die_if_eq("rdx", "rcx", 30);
#endif

    gc_check_word(GC_OFF_META, gc_meta_of(2), 28);
    gc_check_word(GC_HDR + 8, val[1], 23);
    gc_advance();
    gc_die_if_eq("rdx", "0", 24);
    gc_check_word(GC_OFF_META, gc_meta_of(2), 29);
    gc_check_word(GC_HDR + 8, val[2], 25);
    gc_advance();
    if (cycle) {
        const char* p = gc_load_ptr(g[0], "rax");
        gc_die_if_ne("rdx", p, 26);
    } else {
        gc_die_if_ne("rdx", "0", 26);
    }
    gc_unpin(&g[0]);
}
