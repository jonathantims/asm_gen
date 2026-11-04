
/*
 * gc_refcount.h - Option A: REFERENCE COUNTING.
 *
 * Every object carries a count in its meta word [obj+8].  A write barrier keeps
 * it accurate: storing a pointer INCREFs the new referent and DECREFs the one it
 * replaces.  When a count hits zero the object is freed *immediately* - and,
 * crucially, its own pointer fields are DECREF'd first, so freeing the head of a
 * list/tree cascades and reclaims the whole structure eagerly.  Freed blocks go
 * on gc_common's free list, so the next gc_new can take a freed block. The
 * tour prints addresses. It does not assert that the third object reused one.
 * gc/gc_fuzz.h checks that a rooted object still holds its payload after
 * gc_collect.
 *
 * The classic weakness of pure counting is CYCLES (A->B->A never reaches zero).
 * The sub-flag names the remedy. Both values emit the same gc_collect: an
 * empty ret. Mark-sweep is the collector that traces.
 *
 *   -DGC_CYCLES=weak   (default) gc_weak_* copies the pointer and does not
 *                      incref, so a caller can leave one edge of a cycle
 *                      uncounted. The tours and gc_fuzz.h never call it.
 *   -DGC_CYCLES=trial  the same empty gc_collect. The name is a backup trace
 *                      this collector does not run. The flag build exits 0.
 *                      It does not show a cycle reclaimed by tracing.
 *
 * Interface functions overridden here: gc_new (seed count=1), gc_set / gc_assign
 * (the write barrier), gc_scope_end (decref a scope's roots), and gc_collect.
 * Everything else falls through to gc_defaults.h.
 */


/* --- sub-flag: cycle-handling strategy ------------------------------------ */
#define GC_CYCLES_weak  1
#define GC_CYCLES_trial 2
#ifndef GC_CYCLES
#  define GC_CYCLES weak
#endif
#define GC_CYCLES_EXP2(x) GC_CYCLES_##x
#define GC_CYCLES_EXP(x)  GC_CYCLES_EXP2(x)
#define GC_CYCLES_SEL     GC_CYCLES_EXP(GC_CYCLES)

/* The two write-barrier primitives, emitted by gc_emit_collect and referenced by
 * the barrier interface functions below. Private convention: rcx = object. */
static AsmExtern gc_rc_incref;
static AsmExtern gc_rc_decref;

/*
 * gc_emit_collect - emit gc_incref, gc_decref (with the cascading free), and the
 * gc_collect proc; store the barriers in the file-static externs above and return
 * the collect proc for gc_begin to record in the heap handle.
 */
static inline AsmExtern gc_emit_collect(GcHeap* h) {
    (void)h;

    /* -------- gc_incref(rcx = obj): ++[obj+8], NULL-safe -------- */
    gc_rc_incref = write_proc("gc_incref", NULL, 0, asm_void);
    {
        const char* Lskip = c_new_label();
        write_cmp(gc_op("rcx"), literal("0", "obj"));
        write_je(Lskip);
            write_replace_mov("rax", (AsmDataField){ gc_mem("rcx", GC_OFF_META), "refcount", NULL });
            write_add(gc_op("rax"), literal("1", "one"));
            write_replace_mov(gc_mem("rcx", GC_OFF_META), (AsmDataField){ "rax", "refcount", NULL });
        write_label(Lskip);
        write_ret();
    }
    write_endp(&gc_rc_incref);

    /* -------- gc_decref(rcx = obj): --[obj+8]; free at zero -------- */
    gc_rc_decref = write_proc("gc_decref", NULL, 0, asm_void);
    {
        const char* Lskip  = c_new_label();
        const char* Lalive = c_new_label();
        const char* Lloop  = c_new_label();
        const char* Ldone  = c_new_label();

        write_cmp(gc_op("rcx"), literal("0", "obj"));
        write_je(Lskip);                                   /* NULL -> nothing to do */

        write_replace_mov("rax", (AsmDataField){ gc_mem("rcx", GC_OFF_META), "refcount", NULL });
        write_sub(gc_op("rax"), literal("1", "one"));
        write_replace_mov(gc_mem("rcx", GC_OFF_META), (AsmDataField){ "rax", "refcount", NULL });
        write_cmp(gc_op("rax"), literal("0", "refcount"));
        write_jne(Lalive);                                 /* still referenced      */

        /* count hit zero -> free.  First DECREF each pointer field so the drop
         * cascades through the object graph (this is what makes counting reclaim
         * whole data structures, not just leaves). */
        write_replace_mov("r8", (AsmDataField){ "rcx", "obj", NULL });                     /* r8  = obj (survives)  */
        write_replace_mov("r9", (AsmDataField){ gc_mem("r8", GC_OFF_DESC), "gcdesc", NULL });   /* r9 = descriptor  */
        write_replace_mov("r10", (AsmDataField){ gc_mem("r9", GC_DESC_NPTRS), "nptrs", NULL }); /* r10 = #ptr fields */
        write_replace_mov("r11", (AsmDataField){ "0", "idx", NULL });                      /* r11 = field index     */
    write_label(Lloop);
        write_cmp(gc_op("r11"), gc_op("r10"));
        write_jge(Ldone);
            /* off = [desc + GC_DESC_OFF0 + idx*8] */
            write_replace_mov("rax", (AsmDataField){ "r11", "idx", NULL });
            write_shl("rax", "3");
            write_lea("rcx", "[r9+16]");                   /* &off0 (GC_DESC_OFF0=16) */
            write_add(gc_op("rcx"), gc_op("rax"));
            write_replace_mov("rcx", (AsmDataField){ "qword ptr [rcx]", "fieldoff", NULL });
            /* child = [obj + off] */
            write_replace_mov("rcx", (AsmDataField){ "qword ptr [r8+rcx]", "obj", NULL });
            /* recurse; save our scratch across the call */
            write_push("r8"); write_push("r9");
            write_push("r10"); write_push("r11");
            write_call(&gc_rc_decref);
            write_pop("r11"); write_pop("r10");
            write_pop("r9"); write_pop("r8");
            write_add(gc_op("r11"), literal("1", "one"));
            write_jmp(Lloop);
    write_label(Ldone);
        /* push the block onto the free list: size (from desc) in its meta word,
         * old head in its first word. */
        write_replace_mov("r10", (AsmDataField){ gc_mem("r9", GC_DESC_SIZE), "blocksize", NULL });
        write_replace_mov("r11", (AsmDataField){ "qword ptr [gc_freehead]", "freeblock", NULL });
        write_replace_mov(gc_mem("r8", GC_OFF_DESC), (AsmDataField){ "r11", "freeblock", NULL });   /* block.next  */
        write_replace_mov(gc_mem("r8", GC_OFF_META), (AsmDataField){ "r10", "blocksize", NULL });   /* block.size  */
        write_replace_mov("qword ptr [gc_freehead]", (AsmDataField){ "r8", "freeblock", NULL });

        write_label(Lalive);
        write_label(Lskip);
        write_ret();
    }
    write_endp(&gc_rc_decref);

    /* -------- gc_collect: cycle backup -------- */
    AsmExtern collect = write_proc("gc_collect", NULL, 0, asm_void);
#if GC_CYCLES_SEL == GC_CYCLES_trial
        /* Counting does not reclaim a cycle. Each object in the cycle holds a
         * count for the others, so none of them reach zero. A weak reference
         * is the manual break: that edge is not counted, and the cycle can
         * drop to zero when the last strong reference goes away. GC_CYCLES=trial
         * names a backup trace that this collector does not run. The procedure
         * is an empty ret. The mark-sweep collector is the one that traces. */
        write_ret();
#else
        /* GC_CYCLES=weak: same empty collect. Cycles are the caller's problem.
         * A weak edge is how the caller stops counting one link of the cycle.
         * Nothing here scans the heap. */
        write_ret();
#endif
    write_endp(&collect);
    return collect;
}

/* ------------------------------------------------------------------------- *
 *  Interface overrides
 * ------------------------------------------------------------------------- */

/* gc_new: allocate, then seed the reference count to 1 (the creating reference). */
#define GC_HAS_gc_new
/* Allocate an object of type t on heap h, name it name, and set its reference count to 1. */
static inline CValue gc_new(GcHeap* h, const GcType* t, const char* name) {
    CValue v = gc_emit_new(h, t, name);
    write_replace_mov(gc_mem(v.loc, GC_OFF_META), (AsmDataField){ "1", "refcount", NULL });
    return v;
}

/* gc_set: the WRITE BARRIER - incref the new referent, decref the old one, store.
 * Ordering (incref-before-decref) is deliberate: it is safe even when new==old. */
#define GC_HAS_gc_set
/* Store val at field off of obj, increfing val and decrefing the previous field. h is unused. */
static inline void gc_set(GcHeap* h, CValue obj, int off, CValue val) {
    (void)h;
    val = c_resolve(val);
    const char* p = gc_load_ptr(obj, "rax");   /* obj stays in its callee-saved reg */
    write_replace_mov("rcx", (AsmDataField){ val.loc, "obj", NULL });
    write_call(&gc_rc_incref);
    write_replace_mov("rcx", (AsmDataField){ gc_mem(p, off), "obj", NULL });   /* old referent */
    write_call(&gc_rc_decref);
    write_replace_mov(gc_mem(p, off), (AsmDataField){ val.loc, "field", NULL });
}

/* gc_assign: barrier for a local variable reassignment (dst holds the old ref). */
#define GC_HAS_gc_assign
/* Reassign dst to src, increfing src and decrefing the previous dst. h is unused. */
static inline void gc_assign(GcHeap* h, CValue* dst, CValue src) {
    (void)h;
    CValue d = c_resolve(*dst);
    CValue s = c_resolve(src);
    write_replace_mov("rcx", (AsmDataField){ s.loc, "obj", NULL });
    write_call(&gc_rc_incref);
    write_replace_mov("rcx", (AsmDataField){ d.loc, "obj", NULL });
    write_call(&gc_rc_decref);
    write_replace_mov(d.loc, (AsmDataField){ s.loc, "obj", NULL });
}

/* gc_scope_end: leaving a scope drops its owning references (reverse order). */
#define GC_HAS_gc_scope_end
/* Drop each root in scope s by decref, from last to first. h is unused. */
static inline void gc_scope_end(GcHeap* h, GcScope* s) {
    (void)h;
    for (int i = s->n - 1; i >= 0; i--) {
        write_replace_mov("rcx", (AsmDataField){ c_resolve(*s->roots[i]).loc, "obj", NULL });
        write_call(&gc_rc_decref);
    }
}

/* gc_weak_new / gc_weak_get: a WEAK reference is just the raw pointer with NO
 * incref, so it never keeps its target alive - the sanctioned way to break a
 * cycle under GC_CYCLES=weak. */
#define GC_HAS_gc_weak
/* Copy target into a callee-saved register tagged name. h is unused. */
static inline CValue gc_weak_new(GcHeap* h, CValue target, const char* name) {
    (void)h;
    const char* reg = gc_pick_saved();
    target = c_resolve(target);
    write_replace_mov(reg, (AsmDataField){ target.loc, "weak", NULL });
    return gc_value(reg, name);
}
/* Return the weak pointer weak. h and name are unused. */
static inline CValue gc_weak_get(GcHeap* h, CValue weak, const char* name) {
    (void)h; (void)name;
    return weak;   /* the demo's weak target outlives the get; no liveness check */
}

