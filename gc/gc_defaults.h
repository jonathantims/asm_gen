
/*
 * gc_defaults.h - the "no-op / plain" fallbacks for every gc_* interface function
 * the selected option header did not override, plus the option-independent glue
 * (gc_begin / gc_start / gc_collect / gc_hex).
 *
 * gc.h includes this after the selected option header, so an option that wants special behaviour simply
 * defines the function and `#define GC_HAS_<name>` in its own header; whatever it
 * omits gets the straightforward version here.  The upshot is that ONE generator
 * body compiles unchanged against all four options - the essence of the common
 * interface.
 */


/* -------- allocator: default is the shared free-list/bump allocator --------
 * The compactor #defines GC_CUSTOM_ALLOC and supplies its own bump/copy version. */
#ifndef GC_CUSTOM_ALLOC
/* Emit the shared free-list bump allocator under the name gc_alloc. */
static inline AsmExtern gc_emit_alloc(void) { return gc_emit_freelist_alloc("gc_alloc"); }
#endif

/*
 * gc_begin - emit the whole collector runtime (in a `.code` section, after the
 * generator's own print proc) and return a heap handle wiring the pieces together.
 * `gc_emit_collect` is option-provided and may also emit barriers/mark/sweep.
 */
static inline GcHeap gc_begin(AsmExtern print) {
    GcHeap h = {0};
    h.print   = print;
    h.start   = gc_emit_start();
    h.alloc   = gc_emit_alloc();
    h.hex     = gc_emit_hex(&h.print);
    h.collect = gc_emit_collect(&h);
    return h;
}

/* Run-time entry points the generator calls from inside main(). */
static inline void gc_start(GcHeap* h)   { write_call(&h->start); }
/* Call the collect procedure stored in heap h. */
static inline void gc_collect(GcHeap* h) { write_call(&h->collect); }

/* gc_hex - print a managed value as "0x................" (runtime evidence of
 * allocation / reuse / relocation). */
static inline void gc_hex(GcHeap* h, CValue v) {
    v = c_resolve(v);
    if (v.reg < 0) write_replace_mov("rax", (AsmDataField){ v.loc, "obj", NULL });
    write_replace_mov("rcx", (AsmDataField){ (v.reg >= 0) ? v.loc : "rax", "value", NULL });
    write_call(&h->hex);
}

/* -------- object lifecycle (plain versions) -------- */

#ifndef GC_HAS_gc_new
/* Allocate an object of type t on heap h and bind it to name. */
static inline CValue gc_new(GcHeap* h, const GcType* t, const char* name) {
    return gc_emit_new(h, t, name);
}
#endif

/* gc_get - load field [obj+off] into a fresh managed register. */
#ifndef GC_HAS_gc_get
/* Load field off of obj into a fresh register tagged name. type is unused. */
static inline CValue gc_get(CValue obj, int off, CType type, const char* name) {
    (void)type;
    const char* p = gc_load_ptr(obj, "rax");
    const char* reg = gc_pick_saved();
    write_replace_mov(reg, (AsmDataField){ gc_mem(p, off), name, NULL });
    return gc_value(reg, name);
}
#endif

/* gc_set - store value into field [obj+off] (no barrier). */
#ifndef GC_HAS_gc_set
/* Store val into field off of obj with a plain move. h is unused. */
static inline void gc_set(GcHeap* h, CValue obj, int off, CValue val) {
    (void)h;
    val = c_resolve(val);
    const char* p = gc_load_ptr(obj, "rax");
    write_replace_mov(gc_mem(p, off), (AsmDataField){ val.loc, "field", NULL });
}
#endif

/* gc_assign - reassign a local managed variable (no barrier). */
#ifndef GC_HAS_gc_assign
/* Copy src over the local managed variable dst. h is unused. */
static inline void gc_assign(GcHeap* h, CValue* dst, CValue src) {
    (void)h;
    CValue d = c_resolve(*dst);
    CValue s = c_resolve(src);
    write_replace_mov(d.loc, (AsmDataField){ s.loc, "obj", NULL });
}
#endif

/* -------- roots / scopes -------- */

#ifndef GC_HAS_gc_scope_begin
/* Begin a root scope with an empty root list. */
static inline GcScope gc_scope_begin(void) { GcScope s; s.n = 0; return s; }
#endif

/* gc_root - register a managed local as a root.  The plain version just records
 * it (harmless); precise/moving options override to push the slot address onto
 * the runtime shadow stack so the collector can find - and rewrite - it. */
#ifndef GC_HAS_gc_root
/* Record the managed local v in scope s. */
static inline void gc_root(GcScope* s, CValue* v) {
    if (s->n < GC_SCOPE_MAX) s->roots[s->n++] = v;
}
#endif

/* gc_scope_end - leave a scope.  Plain version: nothing (a tracing collector
 * reclaims later; only refcount acts here). */
#ifndef GC_HAS_gc_scope_end
/* End scope s. h is unused. */
static inline void gc_scope_end(GcHeap* h, GcScope* s) { (void)h; (void)s; }
#endif

/* Pin is a no-op on every collector. No option defines GC_HAS_gc_pin.
 * A compacting collector that held an object still across a foreign call
 * would override these. None does. */
#ifndef GC_HAS_gc_pin
/* No-op pin of v. */
static inline void gc_pin(CValue* v)   { (void)v; }
#endif
#ifndef GC_HAS_gc_unpin
/* No-op unpin of v. */
static inline void gc_unpin(CValue* v) { (void)v; }
#endif

/* -------- weak references (only refcount cares) -------- */
#ifndef GC_HAS_gc_weak
/* Copy target into a callee-saved register tagged name. h is unused. */
static inline CValue gc_weak_new(GcHeap* h, CValue target, const char* name) {
    (void)h;
    const char* reg = gc_pick_saved();
    target = c_resolve(target);
    write_replace_mov(reg, (AsmDataField){ target.loc, "weak", NULL });
    return gc_value(reg, name);
}
/* Return weak unchanged. h and name are unused. */
static inline CValue gc_weak_get(GcHeap* h, CValue weak, const char* name) {
    (void)h; (void)name;
    return weak;
}
#endif

