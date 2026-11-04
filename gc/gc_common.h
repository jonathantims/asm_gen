
/*
 * gc_common.h - Option-independent scaffolding shared by every garbage-collector
 * header (gc_refcount.h / gc_marksweep.h / gc_compact.h / gc_conservative.h).
 *
 * What lives here:
 *   - the uniform managed-object header layout and per-type GC descriptors,
 *   - a real heap backed by VirtualAlloc (reserve+commit) with a free-list +
 *     bump-frontier allocator (the non-moving options share it; the compactor
 *     overrides gc_alloc with a bump/copy allocator of its own),
 *   - a runtime ROOT SHADOW STACK (an explicit, precise, complete root set that
 *     the tracing/moving collectors walk - and, for the compactor, rewrite),
 *   - a hex printer so the generated program can prove reclamation/relocation at
 *     run time (it prints real heap addresses and object counts),
 *   - the common gc_* interface *types*; the interface *functions* are supplied
 *     by the selected option header, with gc_defaults.h filling any it omits.
 *
 * Everything is emitted through the existing write_* / c_* emitters - no raw text
 * - so the generated program stays tag-validated exactly like the rest of the
 * project.  See GC_imp.md for the overall design and GC.md for the analysis.
 */


/* ======================================================================== *
 *  Option selection
 *
 *  A generator picks an option on the command line, e.g. `cl /DGC_OPTION=compact`.
 *  We map the bare token to a small integer so option headers can also test
 *  their sub-flags with #if.  gc.h does the actual #include of the right header.
 * ======================================================================== */

#define GCV_refcount     1
#define GCV_marksweep    2
#define GCV_compact      3
#define GCV_conservative 4

#ifndef GC_OPTION
#  define GC_OPTION refcount        /* default when none is given */
#endif

#define GC_EXPAND2(x) GCV_##x
#define GC_EXPAND(x)  GC_EXPAND2(x)
#define GC_SELECTED   GC_EXPAND(GC_OPTION)   /* one of the GCV_* integers above */

/* The option token as a string, e.g. "refcount" - used to name per-option output
 * files so the 16 language x option builds never clobber one another. */
#define GC_XSTR(x)      #x
#define GC_STR(x)       GC_XSTR(x)
#define GC_OPTION_STR   GC_STR(GC_OPTION)

/* A short human label for the selected option, used in the printed sub-banner. */
#if   GC_SELECTED == GCV_refcount
#  define GC_OPTION_NAME "refcount"
#elif GC_SELECTED == GCV_marksweep
#  define GC_OPTION_NAME "mark-sweep"
#elif GC_SELECTED == GCV_compact
#  define GC_OPTION_NAME "compacting"
#elif GC_SELECTED == GCV_conservative
#  define GC_OPTION_NAME "conservative"
#else
#  error "GC_OPTION must be one of: refcount marksweep compact conservative"
#endif

/* ======================================================================== *
 *  Uniform managed-object layout
 *
 *  Every heap object begins with a two-word header the collector owns:
 *
 *      [obj+0]  = descriptor pointer  (offset <Type>_gcdesc): tells the collector
 *                 the object's size and which fields are pointers.
 *      [obj+8]  = meta word: the collector's per-object bookkeeping.  Its meaning
 *                 depends on the option -  refcount (A), mark bit (B, D), or a
 *                 forwarding address / mark (C).
 *      [obj+16] = first payload field.
 *
 *  A per-type descriptor (emitted by gc_typedesc) is a small contiguous array:
 *
 *      <Type>_gcdesc:  dq <total size in bytes, incl. 16-byte header>
 *                      dq <number of pointer fields>
 *                      dq <byte offset of pointer field 0 from obj base>
 *                      dq <byte offset of pointer field 1>
 *                      ...
 * ======================================================================== */

#define GC_HDR        16    /* header size in bytes                               */
#define GC_OFF_DESC   0     /* [obj+0]  descriptor pointer                        */
#define GC_OFF_META   8     /* [obj+8]  refcount / mark / forwarding             */

#define GC_DESC_SIZE   0    /* [desc+0]  total object size                        */
#define GC_DESC_NPTRS  8    /* [desc+8]  pointer-field count                      */
#define GC_DESC_OFF0   16   /* [desc+16] first pointer-field offset               */

/* Heap size to reserve+commit (1 MiB is plenty for the demos). */
#define GC_HEAP_BYTES_TOK "100000h"
/* Semispace midpoint: the compactor treats the heap as two 512 KiB halves and
 * copies live objects from one into the other; this is the base of the 2nd half. */
#define GC_HEAP_HALF_TOK  "80000h"
/* Root shadow-stack capacity (entries, each an 8-byte slot address). */
#define GC_ROOTS_CAP   256

/* ======================================================================== *
 *  Interface types (the functions are per-option; see gc_defaults.h)
 * ======================================================================== */

/* A per-type descriptor handle returned by gc_typedesc. `label` is the emitted
 * `<Type>_gcdesc` symbol; `size` is the total object size (header + payload). */
typedef struct {
    const char* label;
    int         size;
    int         n_ptrs;
} GcType;

/* The runtime pieces the option header emitted, shared with the generator so it can
 * call them.  Not every option fills every field (e.g. refcount has no mark
 * routine); unused ones stay zeroed. `print` is the language wrapper's own print
 * proc (rcx=lpBuffer, edx=nBytes) - all four wrappers expose the identical shape,
 * which is how gc_common stays language-agnostic. */
typedef struct {
    AsmExtern start;    /* gc_start:  VirtualAlloc the heap, capture stack base  */
    AsmExtern alloc;    /* gc_alloc:  rcx=size, rdx=descOffset -> rax=obj         */
    AsmExtern collect;  /* gc_collect: run the collector                          */
    AsmExtern hex;      /* gc_print_hex: rcx=value -> prints "0x................" */
    AsmExtern print;    /* the generator's language print proc                       */
} GcHeap;

/* A lexical scope's registered roots.  For refcount, gc_scope_end decrefs each;
 * for the precise/moving options, gc_root spills the pointer to a stack slot and
 * pushes the slot's address on the runtime shadow stack, and gc_scope_end pops
 * them. */
#define GC_SCOPE_MAX 32
typedef struct {
    CValue* roots[GC_SCOPE_MAX];
    int     n;
} GcScope;

/* ======================================================================== *
 *  Small emit helpers
 * ======================================================================== */

/* "qword ptr [base]" or "qword ptr [base+off]" (arena-stored so it outlives emit). */
static inline const char* gc_mem(const char* base, int off) {
    return asm_mem("qword", base, off);
}

/* "qword ptr [label]" - a RIP-relative access to the named global label. */
static inline const char* gc_glob(const char* label) {
    return asm_mem("qword", label, 0);
}

/* A bare operand cell with a throwaway tag (collector internals do their own
 * pointer bookkeeping, so these tags are cosmetic - see write_add/cmp, which do
 * not consult them). */
static inline AsmDataField gc_op(const char* s) {
    AsmDataField f = { s, "gcword", NULL };
    return f;
}

/* An immediate decimal cell for integer v. */
static inline const char* gc_imm_i(int v) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%d", v);
    return c_strdup_arena(buf);
}

/* The VirtualAlloc import, declared once by gc_externs() and reused by gc_begin. */
static AsmExtern gc_virtualalloc;

/* ======================================================================== *
 *  Per-type GC descriptors (call inside the generator's `.data` section)
 * ======================================================================== */

/*
 * gc_typedesc - emit a `<name>_gcdesc` descriptor and return a handle.
 *
 *   size         total object size in bytes, INCLUDING the 16-byte header.
 *   ptr_offsets  byte offset (from the object base) of each pointer field; these
 *                are what the tracing collectors follow.  Pass NULL / 0 for a
 *                pointer-free (leaf) type.
 *
 * The descriptor is a contiguous run of qwords: size, n_ptrs, off0, off1, ...
 * Must be called inside an initialised `.data` (or `.const`) section.
 */
static inline GcType gc_typedesc(const char* name, int size,
                                 const int* ptr_offsets, int n_ptrs) {
    char lbl[64];
    snprintf(lbl, sizeof(lbl), "%s_gcdesc", name);
    const char* base = c_strdup_arena(lbl);

    write_dq(base, gc_imm_i(size));
    asm_global_dict_add(base, "gcdesc");

    snprintf(lbl, sizeof(lbl), "%s_gcdesc_np", name);
    write_dq(c_strdup_arena(lbl), gc_imm_i(n_ptrs));

    for (int i = 0; i < n_ptrs; i++) {
        snprintf(lbl, sizeof(lbl), "%s_gcdesc_p%d", name, i);
        write_dq(c_strdup_arena(lbl), gc_imm_i(ptr_offsets[i]));
    }

    GcType t = { base, size, n_ptrs };
    return t;
}

/* "offset <Type>_gcdesc" - the descriptor address, for stashing in an object. */
static inline const char* gc_desc_offset(const GcType* t) {
    char buf[80];
    snprintf(buf, sizeof(buf), "offset %s", t->label);
    return c_strdup_arena(buf);
}

/* ------------------------------------------------------------------------ *
 *  Field-based type builder (hides the 16-byte header and byte arithmetic)
 * ------------------------------------------------------------------------ *
 *
 * gc_typedesc takes raw byte sizes and offsets. The builder below lets a generator
 * describe a managed type field-by-field instead, so it never writes GC_HDR or
 * hand-computes an offset:
 *
 *     GcTypeDesc d = gc_type_begin("Node");
 *         gc_field_ptr(&d, "next");    // a traced pointer field
 *         gc_field_word(&d, "value");  // a scalar payload word
 *     GcType Node = gc_type_end(&d);
 *
 * The running size starts past the header; each field advances it by 8 bytes and
 * pointer fields record their offset so the tracing collectors can follow them.
 */
#define GC_TYPE_MAX_PTRS 16
typedef struct {
    const char* name;
    int         size;                     /* running total, starts at GC_HDR */
    int         ptr_offsets[GC_TYPE_MAX_PTRS];
    int         n_ptrs;
} GcTypeDesc;

/* Begin describing managed type name; the object starts with the collector header. */
static inline GcTypeDesc gc_type_begin(const char* name) {
    GcTypeDesc d;
    d.name = name;
    d.size = GC_HDR;   /* fields follow the two-word header */
    d.n_ptrs = 0;
    return d;
}
/* Add an 8-byte pointer field (traced by the collector). `name` is documentary. */
static inline void gc_field_ptr(GcTypeDesc* d, const char* name) {
    (void)name;
    d->ptr_offsets[d->n_ptrs++] = d->size;
    d->size += 8;
}
/* Add an 8-byte scalar (non-pointer) field. `name` is documentary. */
static inline void gc_field_word(GcTypeDesc* d, const char* name) {
    (void)name;
    d->size += 8;
}
/* Finish the type: emit its descriptor and return the handle. Call inside the
 * generator's initialised `.data` (or `.const`) section, like gc_typedesc. */
static inline GcType gc_type_end(GcTypeDesc* d) {
    return gc_typedesc(d->name, d->size, d->ptr_offsets, d->n_ptrs);
}

/* ======================================================================== *
 *  Phase 1 - externs (call in the generator's extern-declaration phase)
 * ======================================================================== */

/* Declare the Win32 imports the collector needs.  Must be called BEFORE the
 * first `.data` section (MASM wants `extern` lines up top), alongside the
 * language wrapper's own *_console_declare(). */
static inline void gc_externs(void) {
    static const char* va[] = { "lpAddress", "dwSize", "flAllocationType", "flProtect" };
    gc_virtualalloc = c_extern("VirtualAlloc", va, 4, C_PTR, "heapbase");
}

/* ======================================================================== *
 *  Phase 2 - data globals (call inside the generator's `.data?` section)
 * ======================================================================== */

/* Emit the collector's mutable globals. Opens the uninitialised section. */
static inline void gc_data_globals(void) {
    c_ensure_bss();
    write_unitialised_data("gc_base",       "heapbase");   /* heap start          */
    write_unitialised_data("gc_cursor",     "cursor");     /* bump frontier       */
    write_unitialised_data("gc_end",        "heapend");    /* heap limit          */
    write_unitialised_data("gc_freehead",   "freeblock");  /* free-list head (0=empty) */
    write_unitialised_data("gc_stackbase",  "stackbase");  /* captured rsp (conservative) */
    write_unitialised_data("gc_roots_top",  "rootstop");   /* shadow-stack depth  */
    write_unitialised_data("gc_scan",       "scanptr");    /* Cheney/compact scan  */
    write_unitialised_data("gc_tospace",    "tospace");    /* compact: to-space base */

    /* Root shadow stack: GC_ROOTS_CAP entries.  Precise options (mark-sweep,
     * compact) push a live pointer (mark-sweep) or a root-SLOT address (compact)
     * here; the collector walks gc_roots[0 .. gc_roots_top). */
    write_dq("gc_roots", "256 dup (?)");

    /* Allocation side-table: every managed object the tracing/moving collectors
     * create is recorded here so a sweep can enumerate the whole heap without a
     * fragile linear walk.  gc_allobjs_top is the count. */
    write_unitialised_data("gc_allobjs_top", "count");
    write_dq("gc_allobjs", "4096 dup (?)");

    /* Root-value slots: the compactor keeps each managed local in one of these
     * memory slots (not a register) so the collector can REWRITE it after moving
     * the object; gc_roots then holds the slot addresses. */
    write_unitialised_data("gc_slots_top", "count");
    write_dq("gc_slots", "256 dup (?)");

    /* A 24-byte scratch buffer the hex printer formats into: "0x" + 16 digits +
     * CR + LF + a spare. */
    write_unitialised_data("gc_hexbuf", "hexbuf");
    write_dq("gc_hexbuf_1", "?");
    write_dq("gc_hexbuf_2", "?");
}

/* ======================================================================== *
 *  Phase 3a - runtime prelude procedures (call inside `.code`)
 *
 *  These are option-independent: heap bootstrap, the shared free-list/bump
 *  allocator, and a hex printer.  The option header emits its own collector
 *  (mark/sweep/compact) and, for the compactor, its own gc_alloc.
 * ======================================================================== */

/*
 * gc_emit_start - emit `gc_start`, which VirtualAllocs the heap, records
 * base/cursor/end, empties the free list and shadow stack, and captures the
 * current rsp as the conservative scanner's stack base.  Private convention:
 * no args, no result.
 */
static inline AsmExtern gc_emit_start(void) {
    AsmExtern p = write_proc("gc_start", NULL, 0, asm_void);
        /* Capture a stack upper-bound for conservative scanning (Option D). */
        write_replace_mov("qword ptr [gc_stackbase]", (AsmDataField){ "rsp", "stackbase", NULL });

        /* base = VirtualAlloc(NULL, GC_HEAP_BYTES, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE). */
        write_extern_call(&gc_virtualalloc,
            literal("0", "lpAddress"),
            literal(GC_HEAP_BYTES_TOK, "dwSize"),
            literal("3000h", "flAllocationType"),   /* MEM_COMMIT | MEM_RESERVE */
            literal("4", "flProtect"));              /* PAGE_READWRITE          */

        write_replace_mov("qword ptr [gc_base]", (AsmDataField){ "rax", "heapbase", NULL });
        write_replace_mov("qword ptr [gc_cursor]", (AsmDataField){ "rax", "cursor", NULL });
        write_replace_mov("r10", (AsmDataField){ "rax", "heapbase", NULL });
        write_add(gc_op("r10"), literal(GC_HEAP_BYTES_TOK, "dwSize"));
        write_replace_mov("qword ptr [gc_end]", (AsmDataField){ "r10", "heapend", NULL });

        /* Semispace midpoint (base of the 2nd half) for the compactor; other
         * options never read gc_tospace. */
        write_replace_mov("r10", (AsmDataField){ "rax", "heapbase", NULL });
        write_add(gc_op("r10"), literal(GC_HEAP_HALF_TOK, "dwSize"));
        write_replace_mov("qword ptr [gc_tospace]", (AsmDataField){ "r10", "tospace", NULL });

        write_replace_mov("qword ptr [gc_freehead]", (AsmDataField){ "0", "freeblock", NULL });
        write_replace_mov("qword ptr [gc_roots_top]", (AsmDataField){ "0", "rootstop", NULL });
        write_replace_mov("qword ptr [gc_allobjs_top]", (AsmDataField){ "0", "count", NULL });
        write_replace_mov("qword ptr [gc_slots_top]", (AsmDataField){ "0", "count", NULL });
        write_ret();
    write_endp(&p);
    return p;
}

/*
 * gc_emit_freelist_alloc - emit the shared allocator used by the non-moving
 * options (refcount, mark-sweep, conservative).
 *
 * Private convention: rcx = requested size (bytes, incl. header), rdx = the
 * type descriptor's address (offset <Type>_gcdesc); returns rax = object.
 *
 * Policy: first-fit over the free list, else carve from the bump frontier.  A
 * reused free block keeps its (larger-or-equal) size; we do not split it here
 * (a deliberately simple, still-correct policy - the coalescing mark-sweep adds
 * neighbour merging on top).  On genuine exhaustion it exits with code 0xGC.
 */
static inline AsmExtern gc_emit_freelist_alloc(const char* proc_name) {
    const char* Lsearch = c_new_label();
    const char* Lnext   = c_new_label();
    const char* Lcarve  = c_new_label();
    const char* Lgot    = c_new_label();
    const char* Lunlink_head = c_new_label();
    const char* Linit   = c_new_label();
    const char* Loom    = c_new_label();

    AsmExtern p = write_proc(proc_name, NULL, 0, asm_void);
        /* --- first-fit search of the free list ---
         * r8  = current block, r9 = previous block (0 = none / at head). */
        write_replace_mov("r8", (AsmDataField){ "qword ptr [gc_freehead]", "freeblock", NULL });
        write_replace_mov("r9", (AsmDataField){ "0", "freeblock", NULL });
    write_label(Lsearch);
        write_cmp(gc_op("r8"), literal("0", "freeblock"));
        write_je(Lcarve);                                  /* end of list -> carve */
        /* a free block stores its byte size in its meta word [+8] */
        write_replace_mov("r10", (AsmDataField){ gc_mem("r8", GC_OFF_META), "blocksize", NULL });
        write_cmp(gc_op("r10"), gc_op("rcx"));
        write_jb(Lnext);                                   /* too small -> next    */
        /* big enough: unlink r8 (prev.next = r8.next, or head = r8.next) */
        write_cmp(gc_op("r9"), literal("0", "freeblock"));
        write_je(Lunlink_head);
            write_replace_mov("r11", (AsmDataField){ gc_mem("r8", GC_OFF_DESC), "freeblock", NULL }); /* r8.next at [+0] */
            write_replace_mov(gc_mem("r9", GC_OFF_DESC), (AsmDataField){ "r11", "freeblock", NULL });
            write_jmp(Lgot);
        write_label(Lunlink_head);
            write_replace_mov("r11", (AsmDataField){ gc_mem("r8", GC_OFF_DESC), "freeblock", NULL });
            write_replace_mov("qword ptr [gc_freehead]", (AsmDataField){ "r11", "freeblock", NULL });
        write_label(Lgot);
            write_replace_mov("rax", (AsmDataField){ "r8", "obj", NULL });
            write_jmp(Linit);
    write_label(Lnext);
        write_replace_mov("r9", (AsmDataField){ "r8", "freeblock", NULL });
        write_replace_mov("r8", (AsmDataField){ gc_mem("r8", GC_OFF_DESC), "freeblock", NULL });   /* r8 = r8.next */
        write_jmp(Lsearch);

        /* --- carve a fresh block from the bump frontier --- */
    write_label(Lcarve);
        write_replace_mov("rax", (AsmDataField){ "qword ptr [gc_cursor]", "cursor", NULL });   /* obj = cursor      */
        write_replace_mov("r10", (AsmDataField){ "rax", "obj", NULL });
        write_add(gc_op("r10"), gc_op("rcx"));                 /* r10 = obj + size  */
        write_cmp(gc_op("r10"), gc_op(gc_glob("gc_end")));
        write_ja(Loom);                                        /* past the limit    */
        write_replace_mov("qword ptr [gc_cursor]", (AsmDataField){ "r10", "cursor", NULL });   /* commit the bump   */
        write_jmp(Linit);

        /* --- out of memory: exit with a distinctive code --- */
    write_label(Loom);
        {
            AsmExtern ep = { "ExitProcess", NULL, 0, asm_void, 1 };
            write_replace_mov("rcx", (AsmDataField){ "0BADh", "exit_code", NULL });    /* distinctive OOM code */
            write_call(&ep);
        }

        /* --- initialise the header of the object now in rax ---
         * [obj+0] = descriptor (rdx passed in), [obj+8] = 0 (meta base). */
    write_label(Linit);
        write_replace_mov(gc_mem("rax", GC_OFF_DESC), (AsmDataField){ "rdx", "gcdesc", NULL });
        write_replace_mov(gc_mem("rax", GC_OFF_META), (AsmDataField){ "0", "meta", NULL });
        write_ret();
    write_endp(&p);
    return p;
}

/* ======================================================================== *
 *  Phase 3b - hex printer (call inside `.code`, after the language print proc)
 *
 *  gc_print_hex formats rcx as "0x" + 16 hex digits + CRLF into gc_hexbuf and
 *  calls the language's print proc.  This lets the tour print REAL runtime
 *  evidence - heap addresses before/after a compaction, object counts after a
 *  sweep - so reclamation/relocation is observable, not just asserted.
 *  Private convention: rcx = value; clobbers volatiles; no result.
 * ======================================================================== */
/* Emit gc_print_hex: write rcx as 0x and 16 hex digits plus CRLF into gc_hexbuf, then call print. */
static inline AsmExtern gc_emit_hex(const AsmExtern* print) {
    const char* Lloop = c_new_label();
    const char* Ldigit = c_new_label();
    const char* Lletter = c_new_label();
    const char* Lstore = c_new_label();

    AsmExtern p = write_proc("gc_print_hex", NULL, 0, asm_void);
        /* buffer: [0]='0' [1]='x' [2..17]=digits [18]=CR [19]=LF */
        write_replace_mov("byte ptr [gc_hexbuf]", (AsmDataField){ "48", "ch", NULL });   /* '0' */
        write_replace_mov("byte ptr [gc_hexbuf+1]", (AsmDataField){ "120", "ch", NULL });  /* 'x' */
        write_replace_mov("byte ptr [gc_hexbuf+18]", (AsmDataField){ "13", "ch", NULL });   /* CR  */
        write_replace_mov("byte ptr [gc_hexbuf+19]", (AsmDataField){ "10", "ch", NULL });   /* LF  */
        /* r8 = value, r9 = digit index (15..0), write from the low nibble up. */
        write_replace_mov("r8", (AsmDataField){ "rcx", "value", NULL });
        write_replace_mov("r9", (AsmDataField){ "15", "idx", NULL });
    write_label(Lloop);
        write_replace_mov("r10", (AsmDataField){ "r8", "value", NULL });
        write_and("r10", "0Fh");                           /* low nibble          */
        write_cmp(gc_op("r10"), literal("9", "nibble"));
        write_ja(Lletter);
    write_label(Ldigit);
        write_add(gc_op("r10"), literal("48", "ch"));      /* '0'..'9'            */
        write_jmp(Lstore);
    write_label(Lletter);
        write_add(gc_op("r10"), literal("87", "ch"));      /* 'a'..'f' (10+87=97) */
    write_label(Lstore);
        /* gc_hexbuf[2 + r9] = r10b */
        write_lea("r11", "[gc_hexbuf+2]");
        write_add(gc_op("r11"), gc_op("r9"));
        write_replace_mov("byte ptr [r11]", (AsmDataField){ "r10b", "ch", NULL });
        write_shr("r8", "4");                              /* next nibble         */
        write_dec("r9");
        write_cmp(gc_op("r9"), literal("0", "idx"));
        write_jge(Lloop);
        /* print(gc_hexbuf, 20) */
        write_extern_call(print,
            literal("offset gc_hexbuf", "lpBuffer"),
            literal("20", "nBytes"));
        write_ret();
    write_endp(&p);
    return p;
}

/* ======================================================================== *
 *  Managed pointers live in callee-saved registers
 *
 *  A write barrier (refcount incref/decref) and gc_alloc/gc_collect all CALL
 *  runtime routines that freely clobber the volatile registers (rax, rcx, rdx,
 *  r8-r11).  So any managed pointer that must survive such a call is kept in a
 *  CALLEE-SAVED register (rbx, r12-r15, rsi, rdi), which the runtime routines
 *  never touch.  main() itself never returns (it ends in ExitProcess), so it may
 *  use these without save/restore.  This is the register-discipline the precise
 *  options rely on instead of a general spiller for the common case. c_funcs.h
 *  spills by default into the current function's frame. C_DISABLE_SPILL turns
 *  that frame off. C_ENABLE_SPILL is accepted and changes nothing. These
 *  collectors do not use that spiller. They keep this callee-saved discipline
 *  even when the C pool would otherwise evict a victim.
 * ======================================================================== */

static const char* const gc_saved_regs[] = { "rbx", "r12", "r13", "r14", "r15", "rsi", "rdi" };

/* Reserve a free callee-saved register for a managed pointer and return its
 * 64-bit name.  Aborts if all are taken. */
static inline const char* gc_pick_saved(void) {
    int n = (int)(sizeof(gc_saved_regs) / sizeof(gc_saved_regs[0]));
    for (int i = 0; i < n; i++) {
        int idx = c_reg_index(gc_saved_regs[i]);
        if (idx >= 0 && !c_regs[idx].reserved && !c_regs[idx].in_use) {
            c_regs[idx].in_use = true;
            return c_regs[idx].names[3];
        }
    }
    fprintf(stderr, "gc: out of callee-saved registers for managed pointers\n");
    exit(1);
}

/* A managed-pointer CValue bound to callee-saved register `reg` and tagged name. */
static inline CValue gc_value(const char* reg, const char* name) {
    CValue v = { name, C_PTR, reg, name, c_reg_index(reg), true, -1 };
    return v;
}

/*
 * gc_load_ptr - obtain a register holding the *current* object address for a
 * managed value.
 *
 *   - Non-moving options keep the pointer in a callee-saved register (obj.reg>=0),
 *     so we just return it.
 *   - The compactor keeps the pointer in a memory ROOT SLOT that the collector
 *     rewrites when it relocates the object; there we reload from the slot into
 *     `scratch` on every use, so code always sees the post-move address.
 *
 * This one helper is what lets a single generator body work with both moving and
 * non-moving collectors unchanged.
 */
static inline const char* gc_load_ptr(CValue obj, const char* scratch) {
    obj = c_resolve(obj);
    if (obj.reg >= 0) return obj.loc;
    write_replace_mov(scratch, (AsmDataField){ obj.loc, "obj", NULL });
    return scratch;
}

/*
 * gc_emit_new - the option-independent core of gc_new: call gc_alloc(size, desc)
 * and park the returned object in a fresh callee-saved register.  Options layer
 * their own header init on top (e.g. refcount sets the count to 1).
 */
static inline CValue gc_emit_new(GcHeap* h, const GcType* t, const char* name) {
    write_replace_mov("rcx", (AsmDataField){ gc_imm_i(t->size), "size", NULL });
    write_replace_mov("rdx", (AsmDataField){ gc_desc_offset(t), "gcdesc", NULL });
    write_call(&h->alloc);                       /* rax = fresh object */
    const char* reg = gc_pick_saved();
    write_replace_mov(reg, (AsmDataField){ "rax", "obj", NULL });
    return gc_value(reg, name);
}

