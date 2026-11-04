
/*
 * gc_marksweep.h - Option B: MARK-SWEEP tracing collection.
 *
 * Nothing happens on ordinary pointer writes (no barrier).  Storage is reclaimed
 * only when gc_collect runs, in two phases:
 *
 *   MARK  - starting from the roots (the shadow stack gc_roots), recursively set
 *           the mark bit (the meta word [obj+8]) on every reachable object,
 *           following the pointer fields named by each object's descriptor.
 *   SWEEP - walk the allocation side-table gc_allobjs; every object still
 *           unmarked is dead, so push it onto the free list (making its space
 *           reusable) and clear it from the table; every marked object survives
 *           and has its mark cleared for the next cycle.
 *
 * Because it traces from roots, mark-sweep reclaims CYCLES that pure counting
 * cannot - the demo's a<->b style graph is collected with no special handling.
 *
 * Sub-flag:
 *   -DGC_COALESCE   after sweeping, merge free blocks that are physically
 *                   adjacent in the heap into one larger block, curbing the
 *                   fragmentation a simple free list would otherwise accumulate.
 *                   The merge is emitted when the flag is set. The tour and
 *                   gc_fuzz.h do not check that two neighbours became one
 *                   block, and they do not check that the next allocation
 *                   reused the lower address.
 *
 * Precise roots: this option keeps managed locals in callee-saved registers (fast,
 * non-moving) and, at gc_root, SPILLS the current pointer value into the shadow
 * stack so the collector - which runs in its own procedure and cannot see the
 * mutator's registers - can find every root.  (The value is captured at root time;
 * the demo does not reassign a rooted local, so no re-spill is needed.  A moving
 * collector needs the stronger slot mechanism instead - see gc_compact.h.)
 */


/* -------- sub-flag: adjacent-free coalescing -------- */
#ifdef GC_COALESCE
#  define GC_COALESCE_ON 1
#else
#  define GC_COALESCE_ON 0
#endif

/* Custom allocator: the shared free-list/bump carve, wrapped to also record each
 * new object in gc_allobjs so the sweep can enumerate the heap. */
#define GC_CUSTOM_ALLOC

/* Emit gc_alloc: carve through gc_alloc_raw and append the object to gc_allobjs. */
static inline AsmExtern gc_emit_alloc(void) {
    /* raw carve (rcx=size, rdx=desc -> rax=obj) */
    gc_emit_freelist_alloc("gc_alloc_raw");

    AsmExtern p = write_proc("gc_alloc", NULL, 0, asm_void);
    {
        AsmExtern raw = { "gc_alloc_raw", NULL, 0, asm_void };
        write_call(&raw);                                  /* rax = obj */
        /* gc_allobjs[gc_allobjs_top++] = rax */
        write_replace_mov("r10", (AsmDataField){ "qword ptr [gc_allobjs_top]", "count", NULL });
        write_lea("r11", "[gc_allobjs]");
        write_replace_mov("qword ptr [r11+r10*8]", (AsmDataField){ "rax", "obj", NULL });
        write_add(gc_op("r10"), literal("1", "one"));
        write_replace_mov("qword ptr [gc_allobjs_top]", (AsmDataField){ "r10", "count", NULL });
        write_ret();
    }
    write_endp(&p);
    return p;
}

/* gc_mark(rcx = obj): set the mark bit and recurse into pointer fields. */
static AsmExtern gc_ms_mark;

/* Emit gc_mark: set the mark on rcx and recurse into each pointer field. */
static inline void gc_emit_mark(void) {
    const char* Lret  = c_new_label();
    const char* Lloop = c_new_label();

    gc_ms_mark = write_proc("gc_mark", NULL, 0, asm_void);
        write_cmp(gc_op("rcx"), literal("0", "obj"));
        write_je(Lret);                                    /* NULL                  */
        write_cmp(gc_op(gc_mem("rcx", GC_OFF_META)), literal("0", "mark"));
        write_jne(Lret);                                   /* already marked        */
        write_replace_mov(gc_mem("rcx", GC_OFF_META), (AsmDataField){ "1", "mark", NULL });

        write_replace_mov("r8", (AsmDataField){ gc_mem("rcx", GC_OFF_DESC), "gcdesc", NULL });     /* descriptor    */
        write_replace_mov("r9", (AsmDataField){ gc_mem("r8", GC_DESC_NPTRS), "nptrs", NULL });     /* #ptr fields   */
        write_replace_mov("r10", (AsmDataField){ "0", "idx", NULL });
    write_label(Lloop);
        write_cmp(gc_op("r10"), gc_op("r9"));
        write_jge(Lret);
            /* off = [desc + GC_DESC_OFF0 + idx*8] */
            write_replace_mov("rax", (AsmDataField){ "r10", "idx", NULL });
            write_shl("rax", "3");
            write_lea("r11", "[r8+16]");
            write_add(gc_op("r11"), gc_op("rax"));
            write_replace_mov("r11", (AsmDataField){ "qword ptr [r11]", "fieldoff", NULL });
            /* child = [obj + off] */
            write_replace_mov("rax", (AsmDataField){ "qword ptr [rcx+r11]", "obj", NULL });
            /* recurse; preserve obj + loop state across the call */
            write_push("rcx"); write_push("r8");
            write_push("r9");  write_push("r10");
            write_replace_mov("rcx", (AsmDataField){ "rax", "obj", NULL });
            write_call(&gc_ms_mark);
            write_pop("r10"); write_pop("r9");
            write_pop("r8");  write_pop("rcx");
            write_add(gc_op("r10"), literal("1", "one"));
            write_jmp(Lloop);
    write_label(Lret);
        write_ret();
    write_endp(&gc_ms_mark);
}

#if GC_COALESCE_ON
/* gc_coalesce: O(n^2) merge of physically adjacent free blocks.  For each free
 * block B, scan the list for a block N at address B + B.size and, if found,
 * absorb it (B.size += N.size; unlink N).  Correct regardless of free-list order. */
static inline void gc_emit_coalesce(void) {
    const char* Louter = c_new_label();
    const char* Lodone = c_new_label();
    const char* Linner = c_new_label();
    const char* Lidone = c_new_label();
    const char* LnextI = c_new_label();
    const char* Lmerge = c_new_label();

    AsmExtern p = write_proc("gc_coalesce", NULL, 0, asm_void);
        write_replace_mov("r8", (AsmDataField){ "qword ptr [gc_freehead]", "freeblock", NULL });   /* B */
    write_label(Louter);
        write_cmp(gc_op("r8"), literal("0", "freeblock"));
        write_je(Lodone);
            /* end = B + B.size */
            write_replace_mov("rax", (AsmDataField){ "r8", "freeblock", NULL });
            write_add(gc_op("rax"), gc_op(gc_mem("r8", GC_OFF_META)));   /* rax = B end */
            /* scan for N == rax, tracking prev */
            write_replace_mov("r9", (AsmDataField){ "qword ptr [gc_freehead]", "freeblock", NULL });     /* N */
            write_replace_mov("r10", (AsmDataField){ "0", "freeblock", NULL });                          /* prevN */
        write_label(Linner);
            write_cmp(gc_op("r9"), literal("0", "freeblock"));
            write_je(Lidone);
            write_cmp(gc_op("r9"), gc_op("rax"));
            write_je(Lmerge);
                write_replace_mov("r10", (AsmDataField){ "r9", "freeblock", NULL });
                write_replace_mov("r9", (AsmDataField){ gc_mem("r9", GC_OFF_DESC), "freeblock", NULL }); /* N = N.next */
                write_jmp(Linner);
        write_label(Lmerge);
            /* B.size += N.size */
            write_replace_mov("r11", (AsmDataField){ gc_mem("r9", GC_OFF_META), "blocksize", NULL });
            write_replace_mov("rcx", (AsmDataField){ gc_mem("r8", GC_OFF_META), "blocksize", NULL });
            write_add(gc_op("rcx"), gc_op("r11"));
            write_replace_mov(gc_mem("r8", GC_OFF_META), (AsmDataField){ "rcx", "blocksize", NULL });
            /* unlink N: prevN.next = N.next, or head = N.next */
            write_replace_mov("r11", (AsmDataField){ gc_mem("r9", GC_OFF_DESC), "freeblock", NULL });    /* N.next */
            write_cmp(gc_op("r10"), literal("0", "freeblock"));
            write_je(LnextI);
                write_replace_mov(gc_mem("r10", GC_OFF_DESC), (AsmDataField){ "r11", "freeblock", NULL });
                write_jmp(Lidone);
        write_label(LnextI);
                write_replace_mov("qword ptr [gc_freehead]", (AsmDataField){ "r11", "freeblock", NULL });
        write_label(Lidone);
            /* advance B */
            write_replace_mov("r8", (AsmDataField){ gc_mem("r8", GC_OFF_DESC), "freeblock", NULL });
            write_jmp(Louter);
    write_label(Lodone);
        write_ret();
    write_endp(&p);
}
#endif

/*
 * gc_emit_collect - emit gc_mark and gc_collect (mark then sweep).  Uses volatile
 * registers only, so it never disturbs the callee-saved registers holding the
 * mutator's live managed pointers.
 */
static inline AsmExtern gc_emit_collect(GcHeap* h) {
    (void)h;
    gc_emit_mark();
#if GC_COALESCE_ON
    gc_emit_coalesce();
#endif

    const char* Lmloop = c_new_label();
    const char* Lmdone = c_new_label();
    const char* Lsloop = c_new_label();
    const char* Lsdone = c_new_label();
    const char* Lsnext = c_new_label();
    const char* Lsclear= c_new_label();
    const char* Lshole = c_new_label();

    AsmExtern collect = write_proc("gc_collect", NULL, 0, asm_void);
        /* ---- MARK from every root ---- */
        write_replace_mov("r8", (AsmDataField){ "0", "idx", NULL });
    write_label(Lmloop);
        write_cmp(gc_op("r8"), gc_op("qword ptr [gc_roots_top]"));
        write_jge(Lmdone);
            write_lea("r9", "[gc_roots]");
            write_replace_mov("rcx", (AsmDataField){ "qword ptr [r9+r8*8]", "obj", NULL });
            write_push("r8");
            write_call(&gc_ms_mark);
            write_pop("r8");
            write_add(gc_op("r8"), literal("1", "one"));
            write_jmp(Lmloop);
    write_label(Lmdone);

        /* ---- SWEEP the allocation table ---- */
        write_replace_mov("r8", (AsmDataField){ "0", "idx", NULL });
    write_label(Lsloop);
        write_cmp(gc_op("r8"), gc_op("qword ptr [gc_allobjs_top]"));
        write_jge(Lsdone);
            write_lea("r9", "[gc_allobjs]");
            write_replace_mov("r10", (AsmDataField){ "qword ptr [r9+r8*8]", "obj", NULL });
            write_cmp(gc_op("r10"), literal("0", "obj"));
            write_je(Lshole);                              /* already-swept slot    */
            write_cmp(gc_op(gc_mem("r10", GC_OFF_META)), literal("0", "mark"));
            write_jne(Lsclear);                            /* marked -> survives    */
            /* dead: push onto free list (size from descriptor) */
            write_replace_mov("r11", (AsmDataField){ gc_mem("r10", GC_OFF_DESC), "gcdesc", NULL });
            write_replace_mov("r11", (AsmDataField){ gc_mem("r11", GC_DESC_SIZE), "blocksize", NULL });
            write_replace_mov("rcx", (AsmDataField){ "qword ptr [gc_freehead]", "freeblock", NULL });
            write_replace_mov(gc_mem("r10", GC_OFF_DESC), (AsmDataField){ "rcx", "freeblock", NULL });   /* next   */
            write_replace_mov(gc_mem("r10", GC_OFF_META), (AsmDataField){ "r11", "blocksize", NULL });   /* size   */
            write_replace_mov("qword ptr [gc_freehead]", (AsmDataField){ "r10", "freeblock", NULL });
            write_replace_mov("qword ptr [r9+r8*8]", (AsmDataField){ "0", "obj", NULL });  /* drop from table       */
            write_jmp(Lsnext);
        write_label(Lsclear);
            write_replace_mov(gc_mem("r10", GC_OFF_META), (AsmDataField){ "0", "mark", NULL });  /* clear for next   */
        write_label(Lshole);
        write_label(Lsnext);
            write_add(gc_op("r8"), literal("1", "one"));
            write_jmp(Lsloop);
    write_label(Lsdone);
#if GC_COALESCE_ON
        {
            AsmExtern co = { "gc_coalesce", NULL, 0, asm_void };
            write_call(&co);
        }
#endif
        write_ret();
    write_endp(&collect);
    return collect;
}

/* ------------------------------------------------------------------------- *
 *  Precise roots: spill the pointer value into the shadow stack.
 * ------------------------------------------------------------------------- */

#define GC_HAS_gc_root
/* Spill pointer v onto the shadow stack and record v in scope s. */
static inline void gc_root(GcScope* s, CValue* v) {
    if (s->n < GC_SCOPE_MAX) s->roots[s->n++] = v;
    /* gc_roots[gc_roots_top++] = v (the current pointer value) */
    write_replace_mov("rax", (AsmDataField){ "qword ptr [gc_roots_top]", "rootstop", NULL });
    write_lea("r10", "[gc_roots]");
    write_replace_mov("qword ptr [r10+rax*8]", (AsmDataField){ v->loc, "obj", NULL });
    write_add(gc_op("rax"), literal("1", "one"));
    write_replace_mov("qword ptr [gc_roots_top]", (AsmDataField){ "rax", "rootstop", NULL });
}

#define GC_HAS_gc_scope_end
/* Pop the roots counted in scope s off the shadow stack. h is unused. */
static inline void gc_scope_end(GcHeap* h, GcScope* s) {
    (void)h;
    if (s->n == 0) return;
    /* pop this scope's roots off the shadow stack */
    write_replace_mov("rax", (AsmDataField){ "qword ptr [gc_roots_top]", "rootstop", NULL });
    write_sub(gc_op("rax"), literal(gc_imm_i(s->n), "count"));
    write_replace_mov("qword ptr [gc_roots_top]", (AsmDataField){ "rax", "rootstop", NULL });
}

