
/*
 * gc_compact.h - Option C: COMPACTING / MOVING collection (a semispace copying
 * collector, Cheney's algorithm).
 *
 * The heap is two equal halves.  Objects are bump-allocated into the current
 * "from" half.  gc_collect copies every LIVE object into the "to" half, packed
 * tightly with no gaps, then swaps the halves - so collection both reclaims all
 * garbage AND eliminates fragmentation in one pass.  Because objects MOVE, every
 * live pointer to them must be found and rewritten; that is what makes a moving
 * collector demanding.
 *
 * How we make relocation correct - the ROOT SLOT discipline:
 *   A moving collector cannot fix up a pointer sitting in a CPU register it
 *   cannot see.  So, unlike the non-moving options, the compactor keeps every
 *   managed local in a memory SLOT (gc_slots[i]).  gc_root registers the slot's
 *   ADDRESS on the shadow stack; the collector rewrites *slot to the object's new
 *   home; and gc_load_ptr reloads the pointer from the slot on every use, so the
 *   mutator always sees the post-move address.  This is the moving-collector
 *   counterpart of the register discipline the non-moving options use.
 *
 * Forwarding pointers: when an object is copied, its OLD meta word [obj+8] is
 * overwritten with the NEW address (a "forwarding pointer").  Any later pointer
 * to the old copy is detected (meta != 0) and redirected to the new one, so
 * shared structure stays shared and cycles terminate.
 *
 * Sub-flag GC_MOVE selects the moving strategy; `copying` (default, implemented
 * here) evacuates to the other half.  `sliding` is accepted and ignored: that
 * flag build is still this copying collector.  GC.md describes sliding as the
 * classic in-place alternative.  gc_pin / gc_unpin are no-ops on this collector
 * and on the other three; no header defines GC_HAS_gc_pin.  The build does not
 * check a slide or a pin.
 */


#ifndef GC_MOVE
#  define GC_MOVE copying
#endif

/* C-side slot allocator: each gc_new claims the next gc_slots[] entry.  One
 * generator run emits one program, so this simple counter suffices. */
static int gc_compact_slot = 0;

/* Extract the byte offset baked into slot operand loc ("qword ptr [gc_slots+N]"). */
static inline int gc_slot_off(const char* loc) {
    const char* plus = strchr(loc, '+');
    return plus ? atoi(plus + 1) : 0;
}

/* -------- allocator: bump within the current half -------- */
#define GC_CUSTOM_ALLOC
/* Emit gc_alloc: bump the cursor by rcx within the current half and write the rdx descriptor. */
static inline AsmExtern gc_emit_alloc(void) {
    const char* Loom = c_new_label();
    AsmExtern p = write_proc("gc_alloc", NULL, 0, asm_void);   /* rcx=size, rdx=desc -> rax */
        write_replace_mov("rax", (AsmDataField){ "qword ptr [gc_cursor]", "cursor", NULL });   /* obj = cursor          */
        write_replace_mov("r10", (AsmDataField){ "rax", "obj", NULL });
        write_add(gc_op("r10"), gc_op("rcx"));                 /* end = obj + size      */
        /* limit = gc_base + half */
        write_replace_mov("r11", (AsmDataField){ "qword ptr [gc_base]", "heapbase", NULL });
        write_add(gc_op("r11"), literal(GC_HEAP_HALF_TOK, "dwSize"));
        write_cmp(gc_op("r10"), gc_op("r11"));
        write_ja(Loom);
        write_replace_mov("qword ptr [gc_cursor]", (AsmDataField){ "r10", "cursor", NULL });   /* commit bump           */
        /* init header */
        write_replace_mov(gc_mem("rax", GC_OFF_DESC), (AsmDataField){ "rdx", "gcdesc", NULL });
        write_replace_mov(gc_mem("rax", GC_OFF_META), (AsmDataField){ "0", "meta", NULL });
        write_ret();
    write_label(Loom);
        {
            AsmExtern ep = { "ExitProcess", NULL, 0, asm_void, 1 };
            write_replace_mov("rcx", (AsmDataField){ "0BADh", "exit_code", NULL });
            write_call(&ep);
        }
    write_endp(&p);
    return p;
}

/* gc_forward(rcx = obj) -> rax = new address in to-space (copy once).
 *
 * Cheney's copy step. A null object returns 0. The meta word of an object
 * that has already been copied holds the to-space address, and that address
 * is returned without a second copy. Otherwise the object is copied to the
 * to-space free pointer (gc_scan stands in for that pointer while collection
 * runs), the descriptor's size is the byte count, the free pointer advances,
 * and the new address is stored in the old object's meta word. Later reads of
 * the old object, including a pointer field that still names it, follow that
 * word and see the new address.
 */
static AsmExtern gc_cp_forward;
/* Emit gc_forward: copy the object in rcx into to-space once and return the new address in rax. */
static inline void gc_emit_forward(void) {
    const char* Lz    = c_new_label();
    const char* Ldone = c_new_label();

    gc_cp_forward = write_proc("gc_forward", NULL, 0, asm_void);
        write_cmp(gc_op("rcx"), literal("0", "obj"));
        write_je(Lz);                                          /* NULL -> 0             */
        write_replace_mov("rax", (AsmDataField){ gc_mem("rcx", GC_OFF_META), "fwd", NULL });
        write_cmp(gc_op("rax"), literal("0", "fwd"));
        write_jne(Ldone);                                      /* already copied        */

        write_replace_mov("r8", (AsmDataField){ "rcx", "obj", NULL });                         /* save obj              */
        write_replace_mov("rax", (AsmDataField){ "qword ptr [gc_scan]", "free", NULL });       /* new = free (gc_scan is the free ptr while copying) */
        write_replace_mov("r9", (AsmDataField){ gc_mem("r8", GC_OFF_DESC), "gcdesc", NULL });
        write_replace_mov("r9", (AsmDataField){ gc_mem("r9", GC_DESC_SIZE), "size", NULL });   /* r9 = object size      */
        /* memcpy obj -> new: rsi=obj, rdi=new, rcx=size */
        write_replace_mov("rsi", (AsmDataField){ "r8", "obj", NULL });
        write_replace_mov("rdi", (AsmDataField){ "rax", "obj", NULL });
        write_replace_mov("rcx", (AsmDataField){ "r9", "size", NULL });
        write_rep_movsb((AsmDataField){"rdi","obj",NULL},
                        (AsmDataField){"rsi","obj",NULL},
                        (AsmDataField){"rcx","size",NULL});
        /* free += size */
        write_replace_mov("r10", (AsmDataField){ "qword ptr [gc_scan]", "free", NULL });
        write_add(gc_op("r10"), gc_op("r9"));
        write_replace_mov("qword ptr [gc_scan]", (AsmDataField){ "r10", "free", NULL });
        /* forwarding pointer in old object */
        write_replace_mov(gc_mem("r8", GC_OFF_META), (AsmDataField){ "rax", "fwd", NULL });
    write_label(Ldone);
        write_ret();
    write_label(Lz);
        write_xor((AsmDataField){"rax","obj",NULL}, (AsmDataField){"rax","obj",NULL});
        write_ret();
    write_endp(&gc_cp_forward);
}

/*
 * gc_emit_collect - Cheney copying collection.  Saves rsi/rdi (callee-saved in
 * the Win64 ABI) around the block copies; uses only volatile registers otherwise,
 * so it does not disturb the mutator's slots.
 *
 * Three passes:
 * 1. Every published root slot is forwarded: *slot = gc_forward(*slot). Only
 *    slots that gc_root pushed are in this list. A binding created by gc_new
 *    has a slot so its pointer can be rewritten, and that slot is a root only
 *    after gc_root publishes it.
 * 2. The Cheney scan walks objects already copied into to-space, from the
 *    half's base up to the free pointer. Each object's descriptor lists its
 *    pointer fields. Each field is forwarded and the new address is written
 *    back. An object reached only through a field is copied here. It does not
 *    get a root slot of its own. Copying a field can append more objects, so
 *    the scan continues until it catches the free pointer.
 * 3. The two heap halves exchange roles. The free pointer becomes the
 *    allocation cursor in the half that just received the live objects.
 */

static inline AsmExtern gc_emit_collect(GcHeap* h) {
    (void)h;
    gc_emit_forward();

    const char* Lrloop = c_new_label();
    const char* Lrdone = c_new_label();
    const char* Lcloop = c_new_label();
    const char* Lcdone = c_new_label();
    const char* Lfloop = c_new_label();
    const char* Lfdone = c_new_label();

    AsmExtern collect = write_proc("gc_collect", NULL, 0, asm_void);
        write_push("rsi");
        write_push("rdi");

        /* to-space free/scan pointers both start at the other half's base.
         * gc_scan doubles as the Cheney "free" pointer inside gc_forward. */
        write_replace_mov("r11", (AsmDataField){ "qword ptr [gc_tospace]", "tospace", NULL });
        write_replace_mov("qword ptr [gc_scan]", (AsmDataField){ "r11", "free", NULL });       /* free = tospace base   */
        write_replace_mov("r8", (AsmDataField){ "r11", "free", NULL });                        /* scan cursor (local)   */

        /* ---- forward every root: *slot = gc_forward(*slot) ---- */
        write_replace_mov("r9", (AsmDataField){ "0", "idx", NULL });
    write_label(Lrloop);
        write_cmp(gc_op("r9"), gc_op("qword ptr [gc_roots_top]"));
        write_jge(Lrdone);
            write_lea("r10", "[gc_roots]");
            write_replace_mov("r10", (AsmDataField){ "qword ptr [r10+r9*8]", "slotaddr", NULL });   /* slot address     */
            write_replace_mov("rcx", (AsmDataField){ "qword ptr [r10]", "obj", NULL });             /* obj = *slot      */
            write_push("r8"); write_push("r9"); write_push("r10");
            write_call(&gc_cp_forward);                             /* rax = new addr   */
            write_pop("r10"); write_pop("r9"); write_pop("r8");
            write_replace_mov("qword ptr [r10]", (AsmDataField){ "rax", "obj", NULL });             /* *slot = new      */
            write_add(gc_op("r9"), literal("1", "one"));
            write_jmp(Lrloop);
    write_label(Lrdone);

        /* ---- Cheney scan: process copied objects, forwarding their fields ---- */
    write_label(Lcloop);
        write_cmp(gc_op("r8"), gc_op("qword ptr [gc_scan]"));      /* scan < free ?     */
        write_jge(Lcdone);
            write_replace_mov("r9", (AsmDataField){ gc_mem("r8", GC_OFF_DESC), "gcdesc", NULL });  /* desc              */
            write_replace_mov("r10", (AsmDataField){ gc_mem("r9", GC_DESC_NPTRS), "nptrs", NULL });
            write_replace_mov("r11", (AsmDataField){ "0", "idx", NULL });
        write_label(Lfloop);
            write_cmp(gc_op("r11"), gc_op("r10"));
            write_jge(Lfdone);
                /* off = [desc + GC_DESC_OFF0 + idx*8] */
                write_replace_mov("rax", (AsmDataField){ "r11", "idx", NULL });
                write_shl("rax", "3");
                write_lea("rcx", "[r9+16]");
                write_add(gc_op("rcx"), gc_op("rax"));
                write_replace_mov("rcx", (AsmDataField){ "qword ptr [rcx]", "fieldoff", NULL });
                /* &field = obj + off ; child = *field */
                write_push("r8"); write_push("r9");
                write_push("r10"); write_push("r11");
                write_replace_mov("rax", (AsmDataField){ "r8", "obj", NULL });
                write_add(gc_op("rax"), gc_op("rcx"));             /* rax = &field      */
                write_push("rax");                                 /* keep &field       */
                write_replace_mov("rcx", (AsmDataField){ "qword ptr [rax]", "obj", NULL });        /* child             */
                write_call(&gc_cp_forward);                        /* rax = new child   */
                write_pop("rcx");                                  /* rcx = &field      */
                write_replace_mov("qword ptr [rcx]", (AsmDataField){ "rax", "obj", NULL });        /* *field = new      */
                write_pop("r11"); write_pop("r10");
                write_pop("r9"); write_pop("r8");
                write_add(gc_op("r11"), literal("1", "one"));
                write_jmp(Lfloop);
        write_label(Lfdone);
            /* scan += size */
            write_replace_mov("r9", (AsmDataField){ gc_mem("r8", GC_OFF_DESC), "gcdesc", NULL });
            write_replace_mov("r9", (AsmDataField){ gc_mem("r9", GC_DESC_SIZE), "size", NULL });
            write_add(gc_op("r8"), gc_op("r9"));
            write_jmp(Lcloop);
    write_label(Lcdone);

        /* ---- flip: old from-space becomes the new to-space ---- */
        write_replace_mov("rax", (AsmDataField){ "qword ptr [gc_base]", "heapbase", NULL });      /* old from base     */
        write_replace_mov("r9", (AsmDataField){ "qword ptr [gc_tospace]", "tospace", NULL });    /* old to base       */
        write_replace_mov("qword ptr [gc_base]", (AsmDataField){ "r9", "heapbase", NULL });       /* new from = old to */
        write_replace_mov("qword ptr [gc_tospace]", (AsmDataField){ "rax", "tospace", NULL });    /* new to = old from */
        write_replace_mov("r10", (AsmDataField){ "qword ptr [gc_scan]", "free", NULL });
        write_replace_mov("qword ptr [gc_cursor]", (AsmDataField){ "r10", "cursor", NULL });      /* frontier = free   */

        write_pop("rdi");
        write_pop("rsi");
        write_ret();
    write_endp(&collect);
    return collect;
}

/* ------------------------------------------------------------------------- *
 *  Interface overrides
 * ------------------------------------------------------------------------- */

/* gc_new: allocate, then park this binding in a slot and return a CValue that
 * lives in that slot (reg = -1).
 *
 * The slot is the mutator's handle, not a claim that every heap object is a
 * root. gc_new records the binding so a later move can rewrite the pointer
 * the program is holding. gc_root is what publishes that slot to the
 * collector. An object reached only as a field of a live object is found by
 * the Cheney scan of the descriptor. It is copied, and its fields are
 * forwarded, without a slot of its own. */
#define GC_HAS_gc_new
/* Allocate an object of type t on heap h and return the slot binding name. */
static inline CValue gc_new(GcHeap* h, const GcType* t, const char* name) {
    write_replace_mov("rcx", (AsmDataField){ gc_imm_i(t->size), "size", NULL });
    write_replace_mov("rdx", (AsmDataField){ gc_desc_offset(t), "gcdesc", NULL });
    write_call(&h->alloc);                                    /* rax = obj             */

    int off = gc_compact_slot++ * 8;
    const char* slot = gc_mem("gc_slots", off);
    write_replace_mov(slot, (AsmDataField){ "rax", "obj", NULL });                            /* slot = obj            */

    CValue v = { name, C_PTR, slot, name, -1, true, -1 };    /* reg=-1: memory slot   */
    return v;
}

/* gc_root: push the slot's ADDRESS onto the shadow stack so the collector both
 * finds the root and can rewrite it after moving. */
#define GC_HAS_gc_root
/* Record v in scope s and push the address of its slot onto the shadow stack. */
static inline void gc_root(GcScope* s, CValue* v) {
    if (s->n < GC_SCOPE_MAX) s->roots[s->n++] = v;
    int off = gc_slot_off(v->loc);
    char addr[48];
    if (off == 0) snprintf(addr, sizeof(addr), "[gc_slots]");
    else          snprintf(addr, sizeof(addr), "[gc_slots+%d]", off);
    write_lea("r10", c_strdup_arena(addr));                   /* r10 = &slot           */
    write_replace_mov("rax", (AsmDataField){ "qword ptr [gc_roots_top]", "rootstop", NULL });
    write_lea("r11", "[gc_roots]");
    write_replace_mov("qword ptr [r11+rax*8]", (AsmDataField){ "r10", "slotaddr", NULL });
    write_add(gc_op("rax"), literal("1", "one"));
    write_replace_mov("qword ptr [gc_roots_top]", (AsmDataField){ "rax", "rootstop", NULL });
}

#define GC_HAS_gc_scope_end
/* Pop the roots of scope s off the shadow stack. h is unused. */
static inline void gc_scope_end(GcHeap* h, GcScope* s) {
    (void)h;
    if (s->n == 0) return;
    write_replace_mov("rax", (AsmDataField){ "qword ptr [gc_roots_top]", "rootstop", NULL });
    write_sub(gc_op("rax"), literal(gc_imm_i(s->n), "count"));
    write_replace_mov("qword ptr [gc_roots_top]", (AsmDataField){ "rax", "rootstop", NULL });
}

/* gc_set: store a pointer into a field.  Both operands may be memory slots, so we
 * stage them through registers (x86 has no memory-to-memory move). */
#define GC_HAS_gc_set
/* Store val into field off of obj, staging slot operands through registers. h is unused. */
static inline void gc_set(GcHeap* h, CValue obj, int off, CValue val) {
    (void)h;
    const char* op = gc_load_ptr(obj, "r10");   /* object base */
    const char* vp = gc_load_ptr(val, "rax");   /* value       */
    write_replace_mov(gc_mem(op, off), (AsmDataField){ vp, "field", NULL });
}

