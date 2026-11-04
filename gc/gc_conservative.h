
/*
 * gc_conservative.h - Option D: CONSERVATIVE (Boehm-style) collection.
 *
 * The defining idea: the collector gets NO help from the compiler about where
 * roots are.  Instead it SCANS the machine stack (and the callee-saved registers,
 * which it spills to the stack first) and treats any word that happens to hold a
 * value equal to the address of a known heap object as a potential root.  This is
 * "conservative": a non-pointer integer that merely looks like an address will
 * keep an object alive (a false positive), so the collector may retain slightly
 * more than necessary - and, crucially, it must therefore NOT move objects (it
 * cannot safely rewrite a value it only guesses is a pointer).
 *
 * Reclamation is otherwise mark-sweep: from the conservatively-found roots we mark
 * precisely (once a word is confirmed to be a real object we can trust its
 * descriptor and follow only its true pointer fields), then sweep the allocation
 * table, freeing everything unmarked.
 *
 * Consequently gc_root / gc_scope_end are NO-OPS here - the whole point is that
 * roots need not be declared.  The demo keeps its live objects in callee-saved
 * registers, so this collector conservatively finds and RETAINS them (an honest
 * demonstration of conservative retention) rather than reclaiming them the way the
 * precise options do once their roots are dropped.
 *
 * Sub-flag GC_SCAN:  stack (default) scans the stack + spilled registers; full
 * additionally scans the whole allocated heap region conservatively (so an object
 * reachable only through a pointer buried in another object is still found even
 * without consulting descriptors). The full-scan build runs the same graph as
 * the stack scan. A pointer sitting in any heap word keeps the child, so that
 * build does not by itself prove the descriptor walk.
 */


#ifndef GC_SCAN
#  define GC_SCAN stack
#endif
#define GC_SCAN_stack 1
#define GC_SCAN_full  2
#define GC_SCAN_EXP2(x) GC_SCAN_##x
#define GC_SCAN_EXP(x)  GC_SCAN_EXP2(x)
#define GC_SCAN_SEL     GC_SCAN_EXP(GC_SCAN)

/* Allocator: shared free-list carve, wrapped to record objects in gc_allobjs
 * (so the sweep can enumerate the heap). */
#define GC_CUSTOM_ALLOC
/* Emit gc_alloc: carve through gc_alloc_raw and record the object in gc_allobjs. */
static inline AsmExtern gc_emit_alloc(void) {
    gc_emit_freelist_alloc("gc_alloc_raw");
    AsmExtern p = write_proc("gc_alloc", NULL, 0, asm_void);
    {
        AsmExtern raw = { "gc_alloc_raw", NULL, 0, asm_void };
        write_call(&raw);
        write_replace_mov("r10", (AsmDataField){ asm_mem("qword", "gc_allobjs_top", 0), "count", NULL });
        write_lea("r11", "[gc_allobjs]");
        write_replace_mov(asm_mem_index("qword", "r11", "r10", 8, 0), (AsmDataField){ "rax", "obj", NULL });
        write_add(gc_op("r10"), literal("1", "one"));
        write_replace_mov(asm_mem("qword", "gc_allobjs_top", 0), (AsmDataField){ "r10", "count", NULL });
        write_ret();
    }
    write_endp(&p);
    return p;
}

/* gc_mark(rcx = obj): precise recursive mark (safe once obj is confirmed real).
 *
 * A null object returns. An object whose meta word is already set returns, so
 * a cycle does not recurse forever. Otherwise the meta word becomes 1 and
 * each pointer field in the descriptor is loaded and marked by a recursive
 * call. The walk's registers are pushed around that call because the callee
 * reuses them. The mark is precise: it follows descriptor fields, not raw
 * words. gc_try_root is what decides that a scanned word is an object before
 * this routine runs.
 */
static AsmExtern gc_cn_mark;
/* Emit gc_mark: mark rcx and recurse through each descriptor pointer field. */
static inline void gc_emit_mark(void) {
    const char* Lret  = c_new_label();
    const char* Lloop = c_new_label();
    gc_cn_mark = write_proc("gc_mark", NULL, 0, asm_void);
        write_comment("return when the object is null or already marked");
        write_cmp(gc_op("rcx"), literal("0", "obj"));
        write_je(Lret);
        write_cmp(gc_op(gc_mem("rcx", GC_OFF_META)), literal("0", "mark"));
        write_jne(Lret);
        write_comment("mark it, then walk descriptor pointer fields");
        write_replace_mov(gc_mem("rcx", GC_OFF_META), (AsmDataField){ "1", "mark", NULL });
        write_replace_mov("r8", (AsmDataField){ gc_mem("rcx", GC_OFF_DESC), "gcdesc", NULL });
        write_replace_mov("r9", (AsmDataField){ gc_mem("r8", GC_DESC_NPTRS), "nptrs", NULL });
        write_replace_mov("r10", (AsmDataField){ "0", "idx", NULL });
    write_label(Lloop);
        write_comment("stop once the field index reaches nptrs");
        write_cmp(gc_op("r10"), gc_op("r9"));
        write_jge(Lret);
            write_replace_mov("rax", (AsmDataField){ "r10", "idx", NULL });
            write_shl("rax", "3");
            write_lea("r11", "[r8+16]");
            write_add(gc_op("r11"), gc_op("rax"));
            write_replace_mov("r11", (AsmDataField){ asm_mem("qword", "r11", 0), "fieldoff", NULL });
            write_replace_mov("rax", (AsmDataField){ asm_mem_index("qword", "rcx", "r11", 1, 0), "obj", NULL });
            write_push("rcx"); write_push("r8");
            write_push("r9");  write_push("r10");
            write_replace_mov("rcx", (AsmDataField){ "rax", "obj", NULL });
            write_call(&gc_cn_mark);
            write_pop("r10"); write_pop("r9");
            write_pop("r8");  write_pop("rcx");
            write_add(gc_op("r10"), literal("1", "one"));
            write_jmp(Lloop);
    write_label(Lret);
        write_ret();
    write_endp(&gc_cn_mark);
}

/* gc_try_root(rcx = candidate word): if it equals a known object address, mark.
 *
 * The candidate is compared with every address recorded in gc_allobjs. A hit
 * means the word is the start of a real object, and gc_mark traces it. A
 * miss, including an interior pointer, is ignored. This is the conservative
 * step: the collector does not know which stack words are pointers, so any
 * word that looks exactly like a known object is treated as a root. That
 * retains some non-pointers and misses interior pointers. Both are the
 * documented cost of scanning without a stack map.
 *
 * Comparing every stack slot with every allocation is O(slots * allocs).
 * A candidate outside [gc_base, gc_cursor) cannot be an object, so it is
 * rejected before the list walk. The list walk is still required: the
 * range has holes, and an interior pointer must not count. A production
 * collector would replace that walk with a bitmap, a card table, or a
 * sorted table of intervals. This one does not, and it still does not
 * move objects and still has no stack map.
 */
static AsmExtern gc_cn_try;
/* Emit gc_try_root: mark rcx when it equals an address recorded in gc_allobjs. */
static inline void gc_emit_try_root(void) {
    const char* Ltl    = c_new_label();
    const char* Ltdone = c_new_label();
    const char* Ltfound= c_new_label();
    gc_cn_try = write_proc("gc_try_root", NULL, 0, asm_void);
        write_cmp(gc_op("rcx"), gc_op(asm_mem("qword", "gc_base", 0)));
        write_jb(Ltdone);
        write_cmp(gc_op("rcx"), gc_op(asm_mem("qword", "gc_cursor", 0)));
        write_jae(Ltdone);
        write_replace_mov("r10", (AsmDataField){ "0", "idx", NULL });
    write_label(Ltl);
        write_cmp(gc_op("r10"), gc_op(asm_mem("qword", "gc_allobjs_top", 0)));
        write_jge(Ltdone);
            write_lea("r11", "[gc_allobjs]");
            write_replace_mov("rax", (AsmDataField){ asm_mem_index("qword", "r11", "r10", 8, 0), "obj", NULL });
            write_cmp(gc_op("rax"), gc_op("rcx"));
            write_je(Ltfound);
            write_add(gc_op("r10"), literal("1", "one"));
            write_jmp(Ltl);
    write_label(Ltfound);
        write_call(&gc_cn_mark);        /* rcx still = the object */
    write_label(Ltdone);
        write_ret();
    write_endp(&gc_cn_try);
}

/*
 * gc_emit_collect - conservative mark, then sweep.  Spills the callee-saved
 * registers so any managed pointer held only in a register is on the stack and
 * gets scanned; restores them before returning.
 *
 * The callee-saved registers are pushed so a pointer that lives only in a
 * register is a word on the stack. Marks are cleared. The stack is then
 * scanned a word at a time and each word is offered to gc_try_root. With
 * GC_SCAN=full the heap is scanned the same way, which retains more
 * false pointers and is the slower option. The sweep walks gc_allobjs and
 * returns unmarked objects to the free list. The saved registers are popped
 * before the return, so the mutator still holds the same pointers. Objects
 * are not moved. gc_root and gc_scope_end stay empty: a conservative
 * collector finds roots by looking, and it has no slot to rewrite.
 */
static inline AsmExtern gc_emit_collect(GcHeap* h) {
    (void)h;
    gc_emit_mark();
    gc_emit_try_root();

    const char* Lclr   = c_new_label();
    const char* Lclrd  = c_new_label();
    const char* Lclrn  = c_new_label();
    const char* Lscan  = c_new_label();
    const char* Lscand = c_new_label();
#if GC_SCAN_SEL == GC_SCAN_full
    const char* Lhscan = c_new_label();
    const char* Lhscand= c_new_label();
#endif
    const char* Lsloop = c_new_label();
    const char* Lsdone = c_new_label();
    const char* Lsnext = c_new_label();
    const char* Lsclear= c_new_label();
    const char* Lshole = c_new_label();

    AsmExtern collect = write_proc("gc_collect", NULL, 0, asm_void);
        /* Spill the Windows callee-saved GPRs so a pointer that lives only
         * in a register is a word the scan will see. rbp is the frame
         * pointer and is not part of this set. rsp is the scan cursor. */
        asm_push_preserved_win64();

        /* ---- clear all marks ---- */
        write_replace_mov("r8", (AsmDataField){ "0", "idx", NULL });
    write_label(Lclr);
        write_cmp(gc_op("r8"), gc_op(asm_mem("qword", "gc_allobjs_top", 0)));
        write_jge(Lclrd);
            write_lea("r9", "[gc_allobjs]");
            write_replace_mov("r10", (AsmDataField){ asm_mem_index("qword", "r9", "r8", 8, 0), "obj", NULL });
            write_cmp(gc_op("r10"), literal("0", "obj"));
            write_je(Lclrn);                       /* hole: skip, keep going       */
            write_replace_mov(gc_mem("r10", GC_OFF_META), (AsmDataField){ "0", "mark", NULL });
        write_label(Lclrn);
            write_add(gc_op("r8"), literal("1", "one"));
            write_jmp(Lclr);
    write_label(Lclrd);

        /* ---- scan the stack for root candidates ---- */
        write_replace_mov("r8", (AsmDataField){ "rsp", "stackptr", NULL });                 /* low end (current)   */
        write_replace_mov("r9", (AsmDataField){ asm_mem("qword", "gc_stackbase", 0), "stackbase", NULL });  /* high end      */
    write_label(Lscan);
        write_cmp(gc_op("r8"), gc_op("r9"));
        write_jae(Lscand);                                  /* p >= base -> done   */
            write_replace_mov("rcx", (AsmDataField){ asm_mem("qword", "r8", 0), "candidate", NULL });
            write_push("r8"); write_push("r9");
            write_call(&gc_cn_try);
            write_pop("r9"); write_pop("r8");
            write_add(gc_op("r8"), literal("8", "eight"));
            write_jmp(Lscan);
    write_label(Lscand);

#if GC_SCAN_SEL == GC_SCAN_full
        /* ---- also scan the whole allocated heap region conservatively ---- */
        write_replace_mov("r8", (AsmDataField){ asm_mem("qword", "gc_base", 0), "heapbase", NULL });
        write_replace_mov("r9", (AsmDataField){ asm_mem("qword", "gc_cursor", 0), "cursor", NULL });
    write_label(Lhscan);
        write_cmp(gc_op("r8"), gc_op("r9"));
        write_jae(Lhscand);
            write_replace_mov("rcx", (AsmDataField){ asm_mem("qword", "r8", 0), "candidate", NULL });
            write_push("r8"); write_push("r9");
            write_call(&gc_cn_try);
            write_pop("r9"); write_pop("r8");
            write_add(gc_op("r8"), literal("8", "eight"));
            write_jmp(Lhscan);
    write_label(Lhscand);
#endif

        /* ---- sweep: free every unmarked object ---- */
        write_replace_mov("r8", (AsmDataField){ "0", "idx", NULL });
    write_label(Lsloop);
        write_cmp(gc_op("r8"), gc_op(asm_mem("qword", "gc_allobjs_top", 0)));
        write_jge(Lsdone);
            write_lea("r9", "[gc_allobjs]");
            write_replace_mov("r10", (AsmDataField){ asm_mem_index("qword", "r9", "r8", 8, 0), "obj", NULL });
            write_cmp(gc_op("r10"), literal("0", "obj"));
            write_je(Lshole);
            write_cmp(gc_op(gc_mem("r10", GC_OFF_META)), literal("0", "mark"));
            write_jne(Lsclear);
            write_replace_mov("r11", (AsmDataField){ gc_mem("r10", GC_OFF_DESC), "gcdesc", NULL });
            write_replace_mov("r11", (AsmDataField){ gc_mem("r11", GC_DESC_SIZE), "blocksize", NULL });
            write_replace_mov("rcx", (AsmDataField){ asm_mem("qword", "gc_freehead", 0), "freeblock", NULL });
            write_replace_mov(gc_mem("r10", GC_OFF_DESC), (AsmDataField){ "rcx", "freeblock", NULL });
            write_replace_mov(gc_mem("r10", GC_OFF_META), (AsmDataField){ "r11", "blocksize", NULL });
            write_replace_mov(asm_mem("qword", "gc_freehead", 0), (AsmDataField){ "r10", "freeblock", NULL });
            write_replace_mov(asm_mem_index("qword", "r9", "r8", 8, 0), (AsmDataField){ "0", "obj", NULL });
            write_jmp(Lsnext);
        write_label(Lsclear);
            write_replace_mov(gc_mem("r10", GC_OFF_META), (AsmDataField){ "0", "mark", NULL });
        write_label(Lshole);
        write_label(Lsnext);
            write_add(gc_op("r8"), literal("1", "one"));
            write_jmp(Lsloop);
    write_label(Lsdone);

        asm_pop_preserved_win64();
        write_ret();
    write_endp(&collect);
    return collect;
}

/* Conservative collection needs no declared roots: gc_root / gc_scope_end are the
 * inherited no-op defaults, which is exactly the point of this option. */

