/* Source files include host.h, asm.h, and asm_funcs.h before this header.
 * Generation state in this file is process-global and single-threaded.
 * Split generation into multiple processes if you need concurrency.
 */
/*
 * c_funcs.h - A C-like code-generation layer over asm_funcs.h.
 *
 * Where asm_funcs.h emits MASM with typed *locations* (you name every register
 * and stack slot), c_funcs.h raises the abstraction to typed *values*: you
 * declare functions, parameters, and local variables WITHOUT naming storage,
 * and this layer performs automatic register allocation. Structs are laid out
 * automatically from their field types. Every construct still flows through the
 * existing write_* emitters, so the type-tag dictionaries keep validating the
 * generated program exactly as before.
 *
 * Design mirrors asm_funcs.h conventions:
 *   - paired open/close helpers return a descriptor the closer consumes
 *     (c_begin_function -> c_end_function),
 *   - a declaration returns the value reused by later references (c_var, c_param),
 *   - each logical value owns a unique type tag (its variable name),
 *   - all output is produced by asm_funcs.h; this header only plans locations.
 *
 * See c_interface.md for the full narrative documentation.
 */

/* ======================================================================== *
 *  Types
 * ======================================================================== */

/*
 * A C scalar type. `size` (1/2/4/8 bytes) selects the register width. Struct
 * alignment is not implied by the type: it is applied only when the active
 * AlignMode is not ALIGN_NONE, or when that struct overrides the mode.
 * `name` documents the type and `is_signed` selects signed vs unsigned
 * comparisons where that matters.
 */
typedef struct {
    const char* name;
    int         size;
    bool        is_signed;
} CType;

static const CType C_CHAR  = { "char",  1, true  };
static const CType C_BOOL  = { "bool",  1, false };
static const CType C_SHORT = { "short", 2, true  };
static const CType C_INT   = { "int",   4, true  };
static const CType C_UINT  = { "uint",  4, false };
static const CType C_LONG  = { "long",  8, true  };
static const CType C_PTR   = { "ptr",   8, false };

/* ======================================================================== *
 *  Register pool  (the heart of automatic allocation)
 * ======================================================================== */

/*
 * One general-purpose register, with its four width-specific names
 * [byte, word, dword, qword]. `is_volatile` records the Windows x64
 * caller-saved set (rax, rcx, rdx, r8-r11). Volatility belongs to the
 * calling convention; this table is that convention's built-in shortcut,
 * used by the allocator. The per-procedure preserved mask lives in the asm
 * layer (asm_set_preserved / ASM_WIN64_PRESERVED) and is what the write
 * checker consults. `reserved` locks rsp/rbp out of allocation. `in_use`
 * is set while a live CValue owns the register.
 */
typedef struct {
    const char* names[4];
    bool        is_volatile;
    bool        reserved;
    bool        in_use;
} CReg;

static CReg c_regs[16] = {
    { { "al",   "ax",   "eax",  "rax" }, true,  false, false }, /*  0 rax */
    { { "cl",   "cx",   "ecx",  "rcx" }, true,  false, false }, /*  1 rcx */
    { { "dl",   "dx",   "edx",  "rdx" }, true,  false, false }, /*  2 rdx */
    { { "bl",   "bx",   "ebx",  "rbx" }, false, false, false }, /*  3 rbx */
    { { "spl",  "sp",   "esp",  "rsp" }, false, true,  false }, /*  4 rsp */
    { { "bpl",  "bp",   "ebp",  "rbp" }, false, true,  false }, /*  5 rbp */
    { { "sil",  "si",   "esi",  "rsi" }, false, false, false }, /*  6 rsi */
    { { "dil",  "di",   "edi",  "rdi" }, false, false, false }, /*  7 rdi */
    { { "r8b",  "r8w",  "r8d",  "r8"  }, true,  false, false }, /*  8 r8  */
    { { "r9b",  "r9w",  "r9d",  "r9"  }, true,  false, false }, /*  9 r9  */
    { { "r10b", "r10w", "r10d", "r10" }, true,  false, false }, /* 10 r10 */
    { { "r11b", "r11w", "r11d", "r11" }, true,  false, false }, /* 11 r11 */
    { { "r12b", "r12w", "r12d", "r12" }, false, false, false }, /* 12 r12 */
    { { "r13b", "r13w", "r13d", "r13" }, false, false, false }, /* 13 r13 */
    { { "r14b", "r14w", "r14d", "r14" }, false, false, false }, /* 14 r14 */
    { { "r15b", "r15w", "r15d", "r15" }, false, false, false }, /* 15 r15 */
};

/*
 * Order in which c_alloc hands out registers. Volatile scratch first
 * (rax, r8-r11) so short-lived temporaries never touch a callee-saved register
 * we would otherwise have to preserve; then the callee-saved set; then the ABI
 * argument / string-op registers (rcx, rsi, rdi, rdx) last, since those are
 * usually wanted for a specific purpose and are reserved explicitly.
 */
static const int c_alloc_order[] = { 0, 8, 9, 10, 11, 3, 12, 13, 14, 15, 1, 6, 7, 2 };

/* Maps a byte size onto the register-name slot. names[0] is the byte form
 * (al), names[1] the word (ax), names[2] the dword (eax), names[3] the
 * qword (rax). Sizes other than 1, 2, 4, and 8 have no such name. */
static inline int c_width_index(int size) {
    switch (size) {
        case 1: return 0;
        case 2: return 1;
        case 4: return 2;
        case 8: return 3;
        default:
            crash_if(1, "c_funcs: unsupported operand size");
            return 0;
    }
}

/* The sized name of register `idx` for a value of `size` bytes. */
static inline const char* c_reg_name(int idx, int size) {
    return c_regs[idx].names[c_width_index(size)];
}

/* Find the pool index of a register given any of its width-names, or -1. */
static inline int c_reg_index(const char* name) {
    for (int i = 0; i < 16; i++) {
        for (int w = 0; w < 4; w++) {
            if (strcmp(c_regs[i].names[w], name) == 0) return i;
        }
    }
    return -1;
}

/* Spill is on unless the translation unit is built with C_DISABLE_SPILL.
 * C_ENABLE_SPILL remains accepted so existing build lines still compile;
 * it does not change the default. c_spill_off / c_spill_on disable and
 * re-enable eviction inside one generation, without removing a frame that
 * c_begin_function already opened. */
#ifdef C_ENABLE_SPILL
#endif
#ifdef C_DISABLE_SPILL
static int c_spill_enabled = 0;
#else
static int c_spill_enabled = 1;
#endif

struct CValue;
static struct CValue* c_reg_owner[16];
/* 0 is unused. Live values occupy 1 .. c_nowned-1 of c_owned, declared
 * with CValue below. Reset at each procedure boundary. */
static int c_nowned = 1;
static int c_frame_open = 0;
static inline void c_spill(struct CValue* v);
static inline void c_spill_volatiles(void);
static inline void c_emit_epilogue(void);

/* ------------------------------------------------------------------------ *
 *  Call-clobber safety net  (see "Guarding against clobbered values" below)
 * ------------------------------------------------------------------------ *
 *
 * The Windows x64 ABI splits the registers into two classes. The *caller-saved*
 * (a.k.a. "volatile") set - rax, rcx, rdx, r8-r11 (and xmm0-xmm5) - may be
 * freely overwritten by any function you CALL; the callee is under no obligation
 * to preserve them. So the instant you emit a `call`, every value you were
 * keeping in one of those registers is - as far as correctness goes - gone. Read
 * it afterwards and you get whatever the callee happened to leave behind, i.e.
 * garbage. This is the single most common code-generation mistake at this layer:
 * a value computed before a call (a running total, an accumulator, a pointer)
 * is silently destroyed by the call and then used again.
 *
 * We cannot always tell *at the call itself* whether a live caller-saved value
 * still matters - the register pool has no mid-procedure "free", so a value that
 * is simply dead (never read again) looks identical to one that must survive.
 * Aborting at the call would therefore fire constantly on perfectly correct code
 * whose temporaries just happen to be dead. Instead we use a "poison on use"
 * scheme, which has *no* false positives: a call marks every live caller-saved
 * register as clobbered, and the error is raised only if the generator later
 * *reads* one of those values (through c_cell) without first re-establishing it.
 * A dead value that is never read again is never flagged; a value you genuinely
 * need across the call is caught the moment you touch it. Re-materialising a
 * value into a register (any c_dup / c_local_imm / c_into / c_assign / reload)
 * clears its poison, and moving the value into a callee-saved register
 * (c_into rbx / r12-r15) or to memory before the call keeps it valid throughout.
 * The asm layer checks a procedure that opted in (c_begin_function sets
 * ASM_WIN64_PRESERVED and saves that set). A write to a preserved register
 * that was not saved, or a call whose volatile mask overlaps an unsaved
 * preserved register, aborts generation. This layer's poison-on-use is the
 * allocator's view of the same Windows x64 volatile set.
 */

/* Logical name of the value currently occupying each register, purely for
 * diagnostics ("value \"total\" in r8 ..."). */
static const char* c_reg_owner_name[16] = {0};
/* Width last stored in the register, so a spill copies the value and not a
 * neighbouring qword. 0 means the register has no tracked value. */
static int c_reg_size[16] = {0};
/* If non-NULL, the value in this register was destroyed by a call to the named
 * callee and has not been re-established since; reading it is an error.
 * A spill that is restored clears this: the register holds the same value
 * the caller of that call expects, so the write is not a clobber. */
static const char* c_reg_clobbered_by[16] = {0};

/* Clear the "clobbered" poison on whichever register `loc` names (no-op for a
 * memory operand or immediate). Called by every helper that writes a fresh value
 * into a register, since that value is valid again from this point on. */
static inline void c_reg_revalidate(const char* loc) {
    if (!loc) return;
    int idx = c_reg_index(loc);
    if (idx >= 0) c_reg_clobbered_by[idx] = NULL;
}

/* After emitting a `call` to callee, mark every live caller-saved register as clobbered so
 * that reading its value later (without refreshing it) is reported. */
static inline void c_mark_call_clobbers(const char* callee) {
    for (int idx = 0; idx < 16; idx++) {
        if (c_regs[idx].is_volatile && c_regs[idx].in_use) {
            c_reg_clobbered_by[idx] = callee;
        }
    }
}

/* ------------------------------------------------------------------------ *
 *  Flow-sensitive clobber tracking
 * ------------------------------------------------------------------------ *
 *
 * The poison above is per-emission-point, but control flow is not straight-line:
 * a call on a branch that then returns must NOT leave its registers looking
 * clobbered on the sibling/fall-through path. So the structured control-flow
 * helpers (c_if_* / c_else / c_switch and c_ret / c_goto / c_break) snapshot and
 * merge the clobber state around each region, and `c_path_dead` records when the
 * current point is unreachable by fall-through (right after a return or an
 * unconditional jump) so a terminated branch contributes nothing to the join.
 *
 * At a join the state is the UNION of the clobbers on every path that can reach
 * it ("may-clobber"): a value clobbered on ANY reaching path is unsafe to read.
 * This is conservative in the safe direction - it never invents a clobber that
 * cannot happen, so it has no false positives, while still catching a value that
 * some reachable path destroyed.
 */
static bool c_path_dead = false;

typedef struct { const char* by[16]; } CClobberState;

/* Snapshot every register's clobber poison into a state the caller can restore. */
static inline CClobberState c_clobber_save(void) {
    CClobberState s;
    for (int i = 0; i < 16; i++) s.by[i] = c_reg_clobbered_by[i];
    return s;
}
/* Replace the current clobber poison with the snapshot s. */
static inline void c_clobber_restore(CClobberState s) {
    for (int i = 0; i < 16; i++) c_reg_clobbered_by[i] = s.by[i];
}
/* Clear the clobber poison on every register. */
static inline void c_clobber_clear_all(void) {
    for (int i = 0; i < 16; i++) c_reg_clobbered_by[i] = NULL;
}
/* Merge `s` into the current state: a register stays clobbered if it is
 * clobbered in either (union / may-clobber). */
static inline void c_clobber_union_into(CClobberState s) {
    for (int i = 0; i < 16; i++)
        if (c_reg_clobbered_by[i] == NULL) c_reg_clobbered_by[i] = s.by[i];
}

/* Release every non-reserved register (used at each procedure boundary). */
static inline void c_pool_reset(void) {
    for (int i = 0; i < 16; i++) {
        c_regs[i].in_use       = false;
        c_reg_owner_name[i]    = NULL;
        c_reg_clobbered_by[i]  = NULL;
        c_reg_size[i]          = 0;
    }
    for (int i = 0; i < 16; i++) c_reg_owner[i] = NULL;
    c_nowned = 1;
}

/* ======================================================================== *
 *  Values
 * ======================================================================== */

/* Forward declaration: the arena strdup (defined in the bump-allocated strings
 * section below) is used by data helpers that appear before that definition. */
static inline char* c_strdup_arena(const char* s);

/*
 * A tracked C value. `loc` is its current location string (a sized register such
 * as "eax", or a memory operand); `tag` is the type tag used with the asm layer
 * (defaults to the variable name so each value is validated independently);
 * `reg` is the owning pool index, or -1 for memory / immediates / return cells.
 */
/* Alignment of a memory object is a property of the definition that places
 * it. Structs use align_pad. Stack locals use c_stack_local, which places
 * the value in the frame at that alignment. A register-resident CValue has
 * no address. align_pad in host.h is the one padding calculation.
 *
 * c_set_preserved / c_set_frame_align apply to the next c_begin_function.
 * The default is the Windows x64 callee-saved set and 16-byte slots.
 * ALIGN_CACHE and ALIGN_PAGE need c_set_frame_align first: rbp is only
 * guaranteed 16-byte aligned, so those frames keep an aligned base in r12.
 */
static unsigned c_pending_preserved = ASM_WIN64_PRESERVED;
static int c_pending_align = 16;

/* Record mask as the preserved-register set for the next function frame. */
static inline void c_set_preserved(unsigned mask) { c_pending_preserved = mask; }
/* Record mode as the next frame alignment. ALIGN_CACHE stores 64, ALIGN_PAGE
 * stores 4096, and the remaining modes store 16. */
static inline void c_set_frame_align(AlignMode mode) {
    if (mode == ALIGN_CACHE) c_pending_align = 64;
    else if (mode == ALIGN_PAGE) c_pending_align = 4096;
    else c_pending_align = 16;
}

typedef struct CValue {
    const char* name;
    CType       type;
    const char* loc;
    const char* tag;
    int         reg;          /* pool index, or -1 when this value is not in the pool */
    bool        live;         /* true while the value may still be read */
    int         spill_slot;   /* frame key while spilled; -1 when it has no slot */
    int         id;           /* index into c_owned, or 0 when this value is not tracked */
    bool        disposed;     /* c_dispose: a later read or write is a generation error */
    bool        is_const;     /* c_mark_const: a later set is a generation error */
} CValue;

/* Canonical copy of every tracked value. Eviction writes this struct, so a
 * CValue the caller holds by value still names the current location through
 * its id. id 0 is "not tracked" (an immediate, a wrapped global, a pinned
 * register that was never entered). The zero-initialised id of an old
 * positional initializer is that untracked case. */
#define C_OWNED_CAP 512
static CValue c_owned[C_OWNED_CAP];

/* Return the canonical tracked copy of value v when v.id selects an owned slot,
 * and return v when the id is outside that table. */
static inline CValue c_resolve(CValue v) {
    if (v.id > 0 && v.id < c_nowned) return c_owned[v.id];
    return v;
}

/* Remember v and, when evictable, point the register's owner at that slot.
 * The caller can drop its copy. A later spill updates the slot, and
 * c_resolve / c_cell see the new location. */
static inline CValue c_track(CValue v, int evictable) {
    crash_if(c_nowned >= C_OWNED_CAP, "c_funcs: too many live values");
    v.id = c_nowned;
    c_owned[c_nowned] = v;
    if (evictable && v.reg >= 0)
        c_reg_owner[v.reg] = &c_owned[c_nowned];
    c_nowned++;
    return c_owned[v.id];
}

/* Build the asm operand cell {location, tag} for a value. Reading a value whose
 * caller-saved register was clobbered by an intervening call is a generation-time
 * error (see "Call-clobber safety net" above). */
static inline AsmDataField c_cell(CValue v) {
    v = c_resolve(v);
    if (v.disposed) {
        char buf[512];
        snprintf(buf, sizeof buf,
            "c_funcs: use of disposed value \"%s\"",
            v.name ? v.name : "?");
        host_fail(buf);
    }
    if (v.reg >= 0 && c_reg_clobbered_by[v.reg] != NULL) {
        char buf[512];
        snprintf(buf, sizeof buf,
            "c_funcs: use of value \"%s\" in %s after it was clobbered by the "
            "call to \"%s\". A call saves and restores named registers when "
            "the frame is open. This value was not saved (spill is off, there "
            "is no frame, or the register has no owner name).",
            v.name ? v.name : "?", v.loc ? v.loc : "?",
            c_reg_clobbered_by[v.reg]);
        host_fail(buf);
    }
    AsmDataField f = { v.loc, v.tag, NULL };
    return f;
}

/*
 * Wrap an existing asm operand (a global, constant, or struct-field reference
 * produced by asm_funcs.h) as a memory-resident CValue, so it can flow through
 * the c_* helpers. Its location is not owned by the register pool (reg = -1).
 */
static inline CValue c_wrap(AsmDataField f, CType type) {
    CValue v = { f.type, type, f.name, f.type, -1, true, -1 };
    return v;
}

/*
 * An immediate operand as a CValue (reg = -1, not owned by the pool). Use for
 * comparison right-hand sides and initial values, e.g. c_imm("0", C_INT,
 * "result") reads better than c_wrap(literal("0", "result"), C_INT).
 */
static inline CValue c_imm(const char* value, CType type, const char* tag) {
    CValue v = { value, type, value, tag, -1, true, -1 };
    return v;
}

/*
 * Cross a tag boundary for a tracked CValue (chiefly a constant used where a
 * different tag is expected at a call boundary). Mirrors asm_funcs.h's treat_as
 * and returns the checked call operand.
 */
static inline AsmDataField c_treat_as(CValue x, const char* existing_tag,
                                      const char* as_tag) {
    return treat_as(c_cell(x), existing_tag, as_tag);
}

/*
 * Declare a named constant (an equate) and return it as a CValue tagged `tag`.
 * Wraps write_constant, so the constant's tag is registered in the global type
 * dictionary and can be used in comparisons/arithmetic directly.
 */
static inline CValue c_constant(const char* name, const char* value,
                                CType type, const char* tag) {
    return c_wrap(write_constant(name, value, tag), type);
}

/* Open .data / .data? only between procedures. Inside a PROC the bytes
 * would be emitted into the procedure body. */
static inline void c_ensure_data(void) {
    if (asm_proc_depth == 0) write_data();
}
static inline void c_ensure_bss(void) {
    if (asm_proc_depth == 0) write_dataq();
}

/*
 * Declare an uninitialised data global (a program variable) and return it as a
 * memory-resident CValue tagged `tag`. Wraps write_unitialised_data. Assign to
 * it with c_assign / c_assign_imm and read it like any other value.
 */
static inline CValue c_global(const char* name, CType type, const char* tag) {
    c_ensure_bss();
    return c_wrap(write_unitialised_data(name, tag), type);
}

/*
 * c_byte_length - define a constant equal to the byte length of a just-emitted
 * data field and return it tagged `tag`. Hides the assembler's `$ - <label>`
 * length idiom (the location counter `$` minus the field's address) behind a
 * named helper, so generators say c_byte_length("msgLen", msg, "len") instead of
 * repeating write_constant(..., "$ - msg", ...). Must be called immediately
 * after the data whose length is wanted, in the same section.
 */
static inline AsmDataField c_byte_length(const char* name, AsmDataField data,
                                         const char* tag) {
    char expr[96];
    snprintf(expr, sizeof(expr), "$ - %s", data.name);
    return write_constant(name, c_strdup_arena(expr), tag);
}

/*
 * c_ptr_data - define an initialised pointer-sized (qword) global holding the
 * address of another symbol, e.g. c_ptr_data("circle", "Circle_vtable", "vptr")
 * emits `circle dq offset Circle_vtable`. Keeps the "offset" keyword out of the
 * generator when building vtables, object headers, and method tables.
 */
static inline AsmDataField c_ptr_data(const char* name, const char* target,
                                      const char* tag) {
    c_ensure_data();
    char buf[96];
    snprintf(buf, sizeof(buf), "offset %s", target);
    write_dq(name, c_strdup_arena(buf));
    asm_global_dict_add(name, tag);
    AsmDataField out = { name, tag, NULL };
    return out;
}

/*
 * c_word_data - define an initialised pointer-sized (qword) global holding a
 * plain integer/constant value, e.g. c_word_data("refobj", "1", "refcount").
 * The pointer-valued sibling is c_ptr_data.
 */
static inline AsmDataField c_word_data(const char* name, const char* value,
                                       const char* tag) {
    c_ensure_data();
    write_dq(name, value);
    asm_global_dict_add(name, tag);
    AsmDataField out = { name, tag, NULL };
    return out;
}

/*
 * c_real4 - define a 4-byte float global.
 * name is the global. decimal is the initializer text, such as "12.0".
 */
static inline void c_real4(const char* name, const char* decimal) {
    c_ensure_data();
    write_real4_def(name, decimal);
}

/*
 * c_real8 - define an 8-byte float global.
 * name is the global. decimal is the initializer text, such as "12.0".
 */
static inline void c_real8(const char* name, const char* decimal) {
    c_ensure_data();
    write_real8_def(name, decimal);
}

/*
 * c_dwords - define a list of 4-byte integers under one label.
 * name is the label. values is a comma-separated list of integer literals,
 * for example "10, 20, 30, 40". The label is not entered in the type
 * dictionary: an indexed load addresses the label by name, and a tag on
 * the label would make that address fail the dictionary check.
 */
static inline void c_dwords(const char* name, const char* values) {
    c_ensure_data();
    write_dd(name, values);
}

/*
 * A read-only string datum: its address (off) and byte length (len) as operand
 * cells, ready to hand straight to a call. Built by c_string.
 */
typedef struct { AsmDataField off; AsmDataField len; } CStr;

/*
 * c_string - emit a read-only string and return handles to its address and its
 * byte length. Encapsulates the three raw emitter steps a generator would otherwise
 * write by hand: write_initialised_data for the bytes, write_offset_constant for
 * its address, and the `$ - <name>` location-counter idiom (via c_byte_length)
 * for its length. So the generator never spells out an offset constant, an
 * AsmDataField, or the assembler length idiom.
 *
 *   text    - the MASM byte content, e.g. "\"hello\",13,10" or "\"path\",0".
 *   off_tag - tags the address so it matches the parameter it is passed to
 *             (call-argument validation checks this); e.g. "lpFileName".
 *   len_tag - tags the length likewise (e.g. "len"); harmless if unused.
 */
static inline CStr c_string(const char* name, const char* text,
                            const char* off_tag, const char* len_tag) {
    c_ensure_data();
    char offname[80], lenname[80];
    /* snprintf is the local formatter in host.h (it covers %s %d %u %zu). */
    snprintf(offname, sizeof(offname), "off_%s", name);
    snprintf(lenname, sizeof(lenname), "len_%s", name);
    AsmDataField d = write_initialised_data(name, text, off_tag);
    CStr s;
    s.off = write_offset_constant(c_strdup_arena(offname), d, off_tag);
    s.len = c_byte_length(c_strdup_arena(lenname), d, len_tag);
    return s;
}

/*
 * c_addr - the address of a named symbol, as a call/init operand.
 *
 * Produces the assembler "offset <symbol>" operand tagged `tag`, but keeps the
 * word "offset" (an assembler concept) out of the generator: callers pass the bare
 * symbol name (e.g. c_addr("circle", "this")) instead of literal("offset circle",
 * "this"). The result is arena-stored so it outlives emission.
 */
static inline AsmDataField c_addr(const char* symbol, const char* tag);

/* ======================================================================== *
 *  Bump-allocated strings (labels, stack slots) must outlive emission
 * ======================================================================== */

/*
 * write() stores token pointers without copying, so any string this layer
 * synthesises (labels, computed stack slots) must have long-lived storage. We
 * borrow asm_funcs.h's bump arena for that.
 */
static inline char* c_strdup_arena(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)asm_bump_alloc(n);
    memcpy(p, s, n);
    return p;
}

/* Decimal text for an immediate. The driver passes a number. */
static inline const char* c_dec(long long n) {
    char buf[32];
    snprintf(buf, sizeof buf, "%lld", n);
    return c_strdup_arena(buf);
}

/* Decimal when the value fits a signed 32-bit immediate. Larger values
 * are MASM hex, produced here so the driver never writes an h suffix.
 * 2147483648 (CW_USEDEFAULT) is the case decimal ml64 rejects. */
static inline const char* c_uimm_text(unsigned long long n) {
    if (n <= 2147483647ull) return c_dec((long long)n);
    char rev[20];
    int r = 0;
    unsigned long long x = n;
    while (x) {
        unsigned d = (unsigned)(x & 15ull);
        rev[r++] = (char)(d < 10 ? '0' + d : 'A' + (d - 10));
        x >>= 4;
    }
    char buf[24];
    int i = 0;
    if (rev[r - 1] >= 'A') buf[i++] = '0';
    while (r) buf[i++] = rev[--r];
    buf[i++] = 'h';
    buf[i] = 0;
    return c_strdup_arena(buf);
}

static inline CValue c_imm_n(long long n, CType type, const char* tag) {
    return c_imm(c_dec(n), type, tag);
}

static inline AsmDataField c_lit_n(long long n, const char* tag) {
    return literal(c_dec(n), tag);
}

static inline AsmDataField c_lit_u(unsigned long long n, const char* tag) {
    return literal(c_uimm_text(n), tag);
}

/*
 * c_db_text - MASM db operand for a C string.
 * nl is 0 (none), 1 (byte 10), or 2 (13,10). nul appends a 0 byte.
 * An empty string with a newline is just the newline bytes. The tours'
 * strings have no embedded quotes, so the text is wrapped as one quoted
 * piece. The result lives in the arena.
 */
static inline const char* c_db_text(const char* text, int nl, int nul) {
    size_t n = text ? strlen(text) : 0;
    char* buf = (char*)asm_bump_alloc(n + 16);
    int i = 0;
    if (n > 0) {
        buf[i++] = '"';
        memcpy(buf + i, text, n);
        i += (int)n;
        buf[i++] = '"';
    }
    if (nl == 1 || nl == 2) {
        if (i) buf[i++] = ',';
        if (nl == 2) {
            memcpy(buf + i, "13,10", 5);
            i += 5;
        } else {
            memcpy(buf + i, "10", 2);
            i += 2;
        }
    }
    if (nul) {
        if (i) buf[i++] = ',';
        buf[i++] = '0';
    }
    if (i == 0) {
        buf[i++] = '"';
        buf[i++] = '"';
    }
    buf[i] = 0;
    return buf;
}

/* Definition of c_addr (declared above): "offset <symbol>", arena-stored.
 * tag is the cell's type tag. */
static inline AsmDataField c_addr(const char* symbol, const char* tag) {
    char buf[96];
    snprintf(buf, sizeof(buf), "offset %s", symbol);
    AsmDataField f = { c_strdup_arena(buf), tag, NULL };
    return f;
}

/*
 * c_offset - the "offset <symbol>" address expression as a plain string, for use
 * as a *data initializer* (e.g. a struct field that stores a code or data
 * address, such as a WNDCLASS lpfnWndProc slot). Same idea as c_addr but returns
 * the bare initializer text rather than a tagged operand cell, keeping the
 * assembler "offset" keyword out of the generator's struct-field declarations.
 */
static inline const char* c_offset(const char* symbol) {
    char buf[96];
    snprintf(buf, sizeof(buf), "offset %s", symbol);
    return c_strdup_arena(buf);
}

static int c_label_counter = 0;

/* A fresh label for a branch this layer invents (the skip target of an if,
 * the join of a switch, the back-edge of a loop). The name is not descriptive
 * because the caller did not name the branch; the structured helper is the
 * documentation. User-named labels go through write_label directly. */
static inline const char* c_new_label(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Lc_%d", c_label_counter++);
    return c_strdup_arena(buf);
}

/* ======================================================================== *
 *  Functions  (parameters auto-assigned to ABI locations)
 * ======================================================================== */

#define C_MAX_PARAMS 16

/* One declared parameter: a name (also its tag) and its C type. */
typedef struct {
    const char* name;
    CType       type;
} CParam;

/*
 * A function being defined. `ext` is the descriptor write_proc returned (reused
 * by internal calls and by c_end_function). `params[]` binds each parameter to
 * its automatically chosen ABI location.
 */
typedef struct {
    AsmExtern ext;
    CValue    params[C_MAX_PARAMS];
    int       nparam;
    bool      has_ret;
} CFunc;

/* The ABI return register name for a result of the given size. */
static inline const char* c_ret_reg_name(int size) {
    switch (size) {
        case 1: return "al";
        case 2: return "ax";
        case 4: return "eax";
        default: return "rax";
    }
}

/*
 * c_extern - Declare an external function that follows the Windows x64 calling
 * convention, without naming any storage.
 *
 * The first four parameters travel in rcx/rdx/r8/r9 and any beyond the fourth in
 * the caller's stack slots (qword ptr [rsp+40], [rsp+48], ...); the result comes
 * back in the ABI return register sized to `ret` (al/ax/eax/rax). Pass
 * ret_tag = NULL for a void function.
 *
 * `param_tags` supplies one type tag per parameter (the tag each argument is
 * validated against at the call site). It is only read here, so a local/compound
 * array is fine; the ABI cells this builds are stored in the bump arena so they
 * outlive every matching call. Returns the AsmExtern; pass its address to
 * c_call_ext / c_call_ext_void.
 *
 * For a function that does NOT follow this convention, drop down to the
 * asm_funcs.h API (write_common_extern / write_extern) and name the locations
 * yourself.
 */
static inline AsmExtern c_extern(const char* name,
                                 const char* const* param_tags, int nparam,
                                 CType ret, const char* ret_tag) {
    crash_if(nparam > C_MAX_PARAMS, "c_extern: too many parameters");

    /* Param cells must outlive every call, so they live in the arena. */
    AsmDataField* cells = NULL;
    if (nparam > 0) {
        cells = (AsmDataField*)asm_bump_alloc(sizeof(AsmDataField) * (size_t)nparam);
    }

    static const char* arg_regs[4] = { "rcx", "rdx", "r8", "r9" };

    /* use_as stays NULL on a cell this function builds. NULL means the call
     * validates the argument against the real type. treat_as is the only
     * writer of a different use_as, and it is how a generator deliberately
     * crosses a tag boundary (a size constant passed as a dwMaxLow, for
     * example). Setting use_as here would hide that crossing inside the
     * declaration. asm_copy_call_params preserves a use_as the caller already
     * set; these cells are new, so there is nothing to preserve. */
    for (int i = 0; i < nparam; i++) {
        const char* loc;
        if (i < 4) {
            loc = arg_regs[i];
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "qword ptr [rsp+%d]", 40 + (i - 4) * 8);
            loc = c_strdup_arena(buf);
        }
        cells[i].name = loc;
        cells[i].type = param_tags[i];
        cells[i].use_as = NULL; /* real type; see the note above the loop */
        cells[i].is_register = (i < 4);
    }

    AsmDataField ret_cell = asm_void;
    if (ret_tag != NULL) {
        ret_cell.name = c_ret_reg_name(ret.size);
        ret_cell.type = ret_tag;
        ret_cell.use_as = NULL; /* NULL means validate as the real type */
    }

    return write_common_extern(name, cells, nparam, ret_cell);
}

/*
 * c_begin_function - Open a procedure whose parameters and result live in
 * automatically chosen ABI locations.
 *
 * The first four parameters travel in rcx/rdx/r8/r9 (sized to each type). With
 * spilling on, a later parameter is qword ptr [rbp+48], [rbp+56], ... because
 * the prologue is push rbp / mov rbp, rsp. With spilling off it is
 * qword ptr [rsp+40], [rsp+48], .... write_call does not store stack arguments,
 * and write_extern_call accepts only the [rsp+40] spelling, so a fifth
 * parameter of a spilled c_begin_function is not reached by either caller.
 * ret_tag names the result value's tag; pass NULL for a void procedure.
 * Returns a CFunc describing the procedure; pass its address to c_param /
 * c_end_function. Register-only internal callers pass &fn.ext to c_call_proc.
 */
static inline CFunc c_begin_function(const char* name,
                                     const CParam* params, int nparam,
                                     CType ret, const char* ret_tag) {
    crash_if(nparam > C_MAX_PARAMS, "c_begin_function: too many parameters");

    /* Parameter cells must outlive every call, so they live in the arena. */
    AsmDataField* cells = NULL;
    if (nparam > 0) {
        cells = (AsmDataField*)asm_bump_alloc(sizeof(AsmDataField) * (size_t)nparam);
    }

    static const char* arg_regs[4] = { "rcx", "rdx", "r8", "r9" };

    CFunc f;
    f.nparam = nparam;
    f.has_ret = (ret_tag != NULL);

    for (int i = 0; i < nparam; i++) {
        const char* loc;
        int idx = -1;
        if (i < 4) {
            idx = c_reg_index(arg_regs[i]);
            loc = c_reg_name(idx, params[i].type.size);
        } else if (c_spill_enabled) {
            /* After push rbp, incoming stack args are at [rbp+48], [rbp+56], ... */
            char buf[32];
            snprintf(buf, sizeof(buf), "qword ptr [rbp+%d]", 48 + (i - 4) * 8);
            loc = c_strdup_arena(buf);
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "qword ptr [rsp+%d]", 40 + (i - 4) * 8);
            loc = c_strdup_arena(buf);
        }
        cells[i].name = loc;
        cells[i].type = params[i].name;   /* the parameter's tag */
        /* NULL use_as: the parameter is validated as its real tag. A later
         * treat_as at a call site is what crosses to a different tag. */
        cells[i].use_as = NULL;
        cells[i].is_register = (i < 4);

        CValue v = { params[i].name, params[i].type, loc,
                     params[i].name, idx, true, -1 };
        f.params[i] = v;
    }

    AsmDataField ret_cell = asm_void;
    if (ret_tag != NULL) {
        ret_cell.name = c_ret_reg_name(ret.size);
        ret_cell.type = ret_tag;
        ret_cell.use_as = NULL; /* the result's real tag; treat_as crosses it */
    }

    /* Fresh register state for the new body, then reserve the parameter regs. */
    c_pool_reset();
    c_path_dead = false;   /* function entry is reachable, nothing clobbered yet */
    f.ext = write_proc(name, cells, nparam, ret_cell);
    c_frame_open = 0;
    if (c_spill_enabled) {
        /* The asm spill frame saves whatever preserved mask is current, so a
         * convention other than Windows x64 uses the same slots and the same
         * epilogue. c_set_preserved / c_set_frame_align change the next frame. */
        unsigned preserved = c_pending_preserved;
        int align = c_pending_align;
        c_pending_preserved = ASM_WIN64_PRESERVED;
        c_pending_align = 16;
        asm_frame_prologue(preserved, align);
        /* Caller-saved set: every GPR except rsp and the preserved mask.
         * c_call_* spills a live value when the callee's mask includes it. */
        f.ext.volatile_mask = (~preserved) & 0xFFFFu & ~(1u << 4);
        if (asm_frame_use_base) c_regs[12].reserved = true;
        c_frame_open = 1;
    }
    for (int i = 0; i < nparam; i++) {
        if (f.params[i].reg >= 0) {
            c_regs[f.params[i].reg].in_use = true;
            c_reg_owner_name[f.params[i].reg] = params[i].name;
            c_reg_size[f.params[i].reg] = params[i].type.size;
        }
        /* The parameter lives in this function's value table, so a later
         * allocation can evict it without the caller registering a pointer.
         * Stack parameters (reg < 0) are already in memory and are not
         * eviction victims. */
        f.params[i] = c_track(f.params[i], f.params[i].reg >= 0);
    }
    return f;
}

/* The value bound to parameter i of f, at its current location. */
static inline CValue c_param(CFunc* f, int i) {
    return c_resolve(f->params[i]);
}

/* The cell the function's result is returned in (for zeroing, etc.). */
static inline AsmDataField c_ret_cell(CFunc* f) {
    return f->ext.return_reg;
}

/* Emit `ret` (validated against the declared return type).
 * f is unused. f is the open function; the return register was recorded
 * when the function was opened. */
static inline void c_ret(CFunc* f) {
    (void)f;
    c_emit_epilogue();
    write_ret();
    /* Fall-through is unreachable, so c_end_if must not union this body's
     * clobber state into the join. */
    c_path_dead = true;
}

static inline void c_materialize(const char* loc, const char* src, const char* tag);

/* Move v into the declared return register, then return.
 * c_ret returns void: it does not place a value. A value that already
 * occupies the return register is left there; its tag is corrected when
 * it is not the declared return tag. */
static inline void c_ret_value(CFunc* f, CValue v) {
    AsmDataField ret = c_ret_cell(f);
    crash_if(ret.name == NULL, "c_ret_value: function returns void");
    v = c_resolve(v);
    crash_if(v.loc == NULL, "c_ret_value: empty value");
    if (strcmp(v.loc, ret.name) != 0) {
        c_materialize(ret.name, v.loc, ret.type);
    } else if (v.tag == NULL || strcmp(v.tag, ret.type) != 0) {
        AsmDataField cur = { ret.name, v.tag ? v.tag : ret.type, NULL };
        change_tag(cur, ret.type);
    }
    c_ret(f);
}

/* Set the result to zero and return (xor of the return register). */
static inline void c_ret_zero(CFunc* f) {
    AsmDataField r = c_ret_cell(f);
    write_xor(r, r);
    c_emit_epilogue();
    write_ret();
    c_path_dead = true;
}

/* Close the procedure f: release the register pool and emit `endp`. */
static inline void c_end_function(CFunc* f) {
    c_pool_reset();
    c_frame_open = 0;
    c_regs[12].reserved = false;
    asm_frame_end();
    write_endp(&f->ext);
}

/*
 * c_commit_stack - commit the stack for a thread opened with c_begin_function.
 * main's prologue does this itself. A new thread does not inherit those pages.
 */
static inline void c_commit_stack(void) {
    write_commit_stack(ASM_STACK_PROBE_STEPS);
}

/* ======================================================================== *
 *  Local variables and moves
 * ======================================================================== */

/* A write to a preserved register that this frame did not save is a generation
 * error (asm_note_write_reg). Spilling that register and restoring it clears
 * the allocator's clobber mark: the caller's value is back, so the temporary
 * use is not a clobber. c_reload and the restore after c_call_* both clear it.
 *
 * Slots come from asm_frame_slot. They are packed to the value's size, so a
 * byte does not consume a qword. The frame is re-entrant: each call has its
 * own rbp. Every value created by c_dup, c_local_imm, c_param, and a call
 * is entered in c_owned. Eviction rewrites that slot. The caller's CValue
 * is a copy, and c_cell reads the slot through the id, so the caller does
 * not register a stable address. A call does not evict: it saves the
 * register and restores the same register. A pinned value (c_into) is
 * tracked but is not an eviction victim. An untracked value with no name
 * is left alone and poisoned if a call clobbers it.
 */

static inline void c_spill_off(void) { c_spill_enabled = 0; }
/* Turn register eviction on. */
static inline void c_spill_on(void)  { c_spill_enabled = 1; }

static inline int c_alloc_reg(void);

/* Copy v's register to a frame slot and free the register. A disposed
 * value is not a spill victim: the caller has finished with it. */
static inline void c_spill(struct CValue* v) {
    if (!v) return;
    if (v->id > 0 && v->id < c_nowned) v = &c_owned[v->id];
    if (v->disposed) return;
    if (v->reg < 0) return;
    crash_if(!c_frame_open, "c_spill: no frame (spill is off, or this is not a c function)");
    int size = v->type.size;
    if (size != 1 && size != 2 && size != 4 && size != 8) size = 8;
    int lo = 0;
    const char* mem = asm_frame_slot_at(size, size, &lo);
    write_replace_mov(mem, (AsmDataField){ c_reg_name(v->reg, size), v->tag, NULL });
    c_regs[v->reg].in_use = false;
    c_reg_owner[v->reg] = NULL;
    c_reg_owner_name[v->reg] = NULL;
    c_reg_clobbered_by[v->reg] = NULL;
    c_reg_size[v->reg] = 0;
    v->loc = mem;
    v->reg = -1;
    v->spill_slot = lo;
}

/* Bring a spilled value back into a register and release the frame slot.
 * A value that is already in a register is left where it is. */
static inline void c_reload(struct CValue* v) {
    if (!v) return;
    if (v->id > 0 && v->id < c_nowned) v = &c_owned[v->id];
    if (v->reg >= 0) return;
    int idx = c_alloc_reg();
    int size = v->type.size;
    if (size != 1 && size != 2 && size != 4 && size != 8) size = 8;
    const char* reg = c_reg_name(idx, size);
    write_replace_mov(reg, (AsmDataField){ v->loc, v->tag, NULL });
    if (v->spill_slot >= 0) asm_frame_release(v->spill_slot, size);
    v->loc = reg;
    v->reg = idx;
    v->spill_slot = -1;
    c_reg_owner[idx] = v;
    c_reg_owner_name[idx] = v->name;
    c_reg_clobbered_by[idx] = NULL;
    c_reg_size[idx] = size;
}

/* Evict values whose address was registered. Used when the allocator is
 * empty. A call does not use this: it saves and restores the register. */
static inline void c_spill_volatiles(void) {
    if (!c_spill_enabled) return;
    for (int idx = 0; idx < 16; idx++) {
        if (c_regs[idx].is_volatile && c_reg_owner[idx] != NULL)
            c_spill(c_reg_owner[idx]);
    }
}

/* The epilogue is the inverse of asm_frame_prologue. One preserved list,
 * recorded when the frame opened, not a second copy of the Windows set. */
static inline void c_emit_epilogue(void) {
    if (!c_frame_open) return;
    asm_frame_epilogue();
}

/* Registers saved around the call that is being emitted. Restored into the
 * same registers, so the caller's CValue does not change. `tracked` is the
 * sized name the type dictionary uses (eax versus rax). The call records
 * its result tag on the return register; the restore puts the saved tag
 * back, or a later read of the caller's value fails the dictionary check. */
typedef struct {
    int idx;
    int lo;
    const char* mem;
    const char* tag;
    const char* tracked;
} CCallSave;
static CCallSave c_call_saves[16];
static int c_call_nsave = 0;

/* Return the volatile register mask of ext, or the Windows x64 set when ext
 * is null or that mask is zero. */
static inline unsigned c_call_mask(const AsmExtern* ext) {
    if (ext && ext->volatile_mask) return ext->volatile_mask;
    return ASM_WIN64_VOLATILE;
}


/* Copy every named live register in `mask` to a frame slot. The CValue
 * keeps its register. c_restore_across writes the slot back after the
 * call. A disposed value is not saved: the caller has finished with it,
 * and restoring it would bring back a value that must not be used. */
static inline void c_save_across(unsigned mask) {
    c_call_nsave = 0;
    if (!c_spill_enabled || !c_frame_open) return;
    for (int idx = 0; idx < 16; idx++) {
        if (!c_regs[idx].in_use || c_regs[idx].reserved) continue;
        if ((mask & (1u << idx)) == 0) continue;
        if (c_reg_owner[idx] != NULL && c_reg_owner[idx]->disposed) continue;
        if (c_reg_owner_name[idx] == NULL && c_reg_owner[idx] == NULL) continue;
        int lo = 0;
        int size = c_reg_size[idx];
        if (size != 1 && size != 2 && size != 4 && size != 8) size = 8;
        const char* tracked = c_reg_name(idx, size);
        const char* tag = asm_type_dict_get(tracked);
        if (tag == NULL)
            tag = c_reg_owner_name[idx] ? c_reg_owner_name[idx] : "spill";
        const char* mem = asm_frame_slot_at(8, 8, &lo);
        write_replace_mov(mem, (AsmDataField){ c_regs[idx].names[3], tag, NULL });
        c_call_saves[c_call_nsave].idx = idx;
        c_call_saves[c_call_nsave].lo = lo;
        c_call_saves[c_call_nsave].mem = mem;
        c_call_saves[c_call_nsave].tag = tag;
        c_call_saves[c_call_nsave].tracked = tracked;
        c_call_nsave++;
    }
}

/* Write the slots from c_save_across back into the same registers, in
 * reverse order, and release the slots. `hold` is the register that
 * received the call result; it is not marked clobbered and is not
 * restored over. A register that was in the mask and was not saved is
 * poisoned with `callee`, so a later read fails in c_cell. */
static inline void c_restore_across(const char* callee, unsigned mask, int hold) {
    for (int idx = 0; idx < 16; idx++) {
        if (idx == hold) { c_reg_clobbered_by[idx] = NULL; continue; }
        if (!c_regs[idx].in_use || c_regs[idx].reserved) continue;
        if ((mask & (1u << idx)) == 0) continue;
        int saved = 0;
        for (int s = 0; s < c_call_nsave; s++) if (c_call_saves[s].idx == idx) saved = 1;
        if (!saved) c_reg_clobbered_by[idx] = callee;
    }
    for (int i = c_call_nsave - 1; i >= 0; i--) {
        int idx = c_call_saves[i].idx;
        write_replace_mov(c_regs[idx].names[3], (AsmDataField){ c_call_saves[i].mem, c_call_saves[i].tag, NULL });
        asm_type_dict_set(c_call_saves[i].tracked, c_call_saves[i].tag);
        c_reg_clobbered_by[idx] = NULL;
        asm_frame_release(c_call_saves[i].lo, 8);
    }
    c_call_nsave = 0;
}

/* Evict one owned value and return its register, now marked in use.
 * Used when c_alloc_reg and c_hold_reg find the pool empty. */
static inline int c_alloc_reg_spilling(void);

/* A free register to hold a call result. It is in use but unnamed, so
 * c_save_across will not restore over it. */
static inline int c_hold_reg(void) {
    int n = (int)(sizeof(c_alloc_order) / sizeof(c_alloc_order[0]));
    for (int k = 0; k < n; k++) {
        int idx = c_alloc_order[k];
        if (!c_regs[idx].reserved && !c_regs[idx].in_use) {
            c_regs[idx].in_use = true;
            c_reg_owner[idx] = NULL;
            c_reg_owner_name[idx] = NULL;
            c_reg_clobbered_by[idx] = NULL;
            return idx;
        }
    }
    if (c_spill_enabled) {
        int idx = c_alloc_reg_spilling();
        c_reg_owner[idx] = NULL;
        c_reg_owner_name[idx] = NULL;
        c_reg_clobbered_by[idx] = NULL;
        return idx;
    }
    crash_if(1, "c_funcs: out of registers");
    return 0;
}

/* Evict one owned value and return its register, marked in use. */
static inline int c_alloc_reg_spilling(void) {
    for (int idx = 0; idx < 16; idx++) {
        if (c_reg_owner[idx] != NULL && c_reg_owner[idx]->disposed) continue;
        if (c_reg_owner[idx] != NULL && !c_regs[idx].reserved) {
            c_spill(c_reg_owner[idx]);
            c_regs[idx].in_use = true;
            return idx;
        }
    }
    crash_if(1, "c_funcs: out of registers and no spillable value");
    return 0;
}

/* The next free register in c_alloc_order. When the pool is empty and
 * spilling is on, evicts one value and returns that register. */
static inline int c_alloc_reg(void) {
    int n = (int)(sizeof(c_alloc_order) / sizeof(c_alloc_order[0]));
    for (int k = 0; k < n; k++) {
        int idx = c_alloc_order[k];
        if (!c_regs[idx].reserved && !c_regs[idx].in_use) {
            c_regs[idx].in_use = true;
            return idx;
        }
    }
    if (c_spill_enabled) return c_alloc_reg_spilling();
    crash_if(1, "c_funcs: out of registers");
    return 0;
}

/*
 * Emit `mov <loc>, <src>` establishing `loc` as a value tagged `tag`, choosing
 * write_init_mov vs write_replace_mov by whether the dictionary already tracks
 * the location. This keeps the local dictionary consistent as registers are
 * reused across a procedure.
 */
static inline void c_materialize(const char* loc, const char* src, const char* tag) {
    if (asm_type_lookup(loc) == NULL) {
        write_init_mov(loc, src, tag);
    } else {
        AsmDataField cell = { src, tag, NULL };
        write_replace_mov(loc, cell);
    }
    c_reg_revalidate(loc);   /* loc now holds a fresh, valid value */
}

/*
 * c_init - store imm into dst, creating the dictionary entry when dst is new.
 * dst is the variable. imm is the immediate text. A variable that is already
 * tracked is overwritten in place; a new one is recorded under its own tag.
 */
static inline void c_init(CValue* dst, const char* imm) {
    CValue d = c_resolve(*dst);
    c_materialize(d.loc, imm, d.tag);
}

/* c_init of a decimal number. The driver passes the number. */
static inline void c_init_n(CValue* dst, long long n) {
    c_init(dst, c_dec(n));
}

/*
 * c_dup - Copy a value into a freshly allocated register as a new variable.
 *
 * The register is chosen automatically. The new variable is tagged with `name`
 * (its own logical identity), so name it after what it holds (e.g. "head",
 * "len") to keep tags aligned with the source's meaning.
 */
static inline CValue c_dup(const char* name, CType type, CValue src) {
    src = c_resolve(src);
    int idx = c_alloc_reg();
    const char* loc = c_reg_name(idx, type.size);
    c_materialize(loc, src.loc, name);
    c_reg_owner_name[idx] = name;
    c_reg_size[idx] = type.size;
    CValue v = { name, type, loc, name, idx, true, -1 }; /* true = live */
    return c_track(v, 1);
}

/*
 * c_zext - zero-extend a 4-byte src into an 8-byte variable named name.
 * The move writes the 4-byte spelling of a fresh register, and that spelling
 * zero-extends the whole register. A move from a 4-byte source into the
 * 8-byte name is not a legal encoding. Only a 4-byte source is accepted;
 * a byte or a word would need a different widening move.
 * name is the new variable. src is the 4-byte value.
 */
static inline CValue c_zext(const char* name, CValue src) {
    src = c_resolve(src);
    crash_if(src.type.size != 4, "c_zext: source must be a 4-byte value");
    int idx = c_alloc_reg();
    const char* narrow = c_reg_name(idx, 4);
    c_materialize(narrow, src.loc, name);
    const char* wide = c_reg_name(idx, 8);
    asm_type_dict_set(wide, name);
    c_reg_owner_name[idx] = name;
    c_reg_size[idx] = 8;
    CValue v = { name, C_LONG, wide, name, idx, true, -1 };
    return c_track(v, 1);
}

/*
 * c_into - Copy a value into a specific named register, pinning it.
 * reg receives src, sized by type. The result is tagged tag.
 */
static inline CValue c_into(const char* reg, CType type, CValue src, const char* tag) {
    src = c_resolve(src);
    int idx = c_reg_index(reg);
    if (idx < 0) {
        fprintf(stderr, "c_into: unknown register \"%s\"\n", reg);
        exit(1);
    }
    c_regs[idx].in_use = true;
    const char* loc = c_reg_name(idx, type.size);
    c_materialize(loc, src.loc, tag);
    c_reg_owner_name[idx] = tag;
    c_reg_size[idx] = type.size;
    /* Pinned: tracked so c_cell stays honest, not an eviction victim. */
    CValue v = { tag, type, loc, tag, idx, true, -1 };
    return c_track(v, 0);
}

/* c_local_imm - Allocate an uninitialised register variable and give it an
 * initial immediate value. name is the variable, type sizes the register, and
 * imm is the immediate. */
static inline CValue c_local_imm(const char* name, CType type, const char* imm) {
    int idx = c_alloc_reg();
    const char* loc = c_reg_name(idx, type.size);
    c_materialize(loc, imm, name);
    c_reg_owner_name[idx] = name;
    c_reg_size[idx] = type.size;
    CValue v = { name, type, loc, name, idx, true, -1 };
    return c_track(v, 1);
}

static inline CValue c_local_imm_n(const char* name, CType type, long long n) {
    return c_local_imm(name, type, c_dec(n));
}

/* A stack local in the current frame, aligned for `mode`.
 * ALIGN_NONE packs to a byte. ALIGN_C aligns to the value's size (at most 8).
 * ALIGN_CACHE and ALIGN_PAGE require c_set_frame_align before the function,
 * because the frame base has to be aligned to 64 or 4096 and rbp is not.
 * The local is uninitialised memory. Its CValue names that operand. */
static inline CValue c_stack_local(const char* name, CType type, AlignMode mode) {
    crash_if(!c_frame_open, "c_stack_local: no frame");
    int size = type.size;
    if (size != 1 && size != 2 && size != 4 && size != 8) size = 8;
    int align = 1;
    if (mode == ALIGN_C) align = size;
    else if (mode == ALIGN_CACHE) align = 64;
    else if (mode == ALIGN_PAGE) align = 4096;
    int lo = 0;
    const char* mem = asm_frame_slot_at(size, align, &lo);
    CValue v = { name, type, mem, name, -1, true, lo };
    return v;
}

/* A set of a disposed value, or of a value marked const, is a generation
 * error. Reads go through c_cell and are allowed for a const value. */
static inline void c_reject_set(CValue v) {
    v = c_resolve(v);
    if (v.disposed)
        host_fail("c_funcs: write of a disposed value");
    if (v.is_const) {
        char buf[256];
        snprintf(buf, sizeof buf, "c_funcs: write of const value \"%s\"",
                 v.name ? v.name : "?");
        host_fail(buf);
    }
}

/* dst := src, preserving dst's meaning (dst must already be tracked). */
static inline void c_assign(CValue* dst, CValue src) {
    c_reject_set(*dst);
    CValue d = c_resolve(*dst);
    c_reg_revalidate(d.loc);   /* dst is being overwritten, not read */
    write_set_mov(c_cell(*dst), c_cell(src));
}

/* dst := immediate, preserving dst's meaning. */
static inline void c_assign_imm(CValue* dst, const char* imm) {
    c_reject_set(*dst);
    CValue d = c_resolve(*dst);
    c_reg_revalidate(d.loc);
    write_set_mov(c_cell(*dst), literal(imm, d.tag));
}

/* dst += src / dst -= src (read-modify-write). dst is read here, so a value
 * clobbered by an intervening call is correctly reported. */
static inline void c_add(CValue* dst, CValue src) {
    c_reject_set(*dst);
    write_add(c_cell(*dst), c_cell(src));
}
/* Subtract src from dst in place. */
static inline void c_sub(CValue* dst, CValue src) {
    c_reject_set(*dst);
    write_sub(c_cell(*dst), c_cell(src));
}

/* dst &= src. The asm AND takes register or memory text, not a cell. */
static inline void c_and(CValue* dst, CValue src) {
    c_reject_set(*dst);
    CValue d = c_resolve(*dst);
    CValue s = c_resolve(src);
    write_and(d.loc, s.loc);
}
/* Bitwise-and dst with the immediate imm. */
static inline void c_and_imm(CValue* dst, const char* imm) {
    c_reject_set(*dst);
    CValue d = c_resolve(*dst);
    write_and(d.loc, imm);
}

/* dst := 0. */
static inline void c_zero(CValue* dst) {
    c_reject_set(*dst);
    CValue d = c_resolve(*dst);
    c_reg_revalidate(d.loc);   /* fully overwritten regardless of prior value */
    write_xor(c_cell(*dst), c_cell(*dst));
}

/*
 * Relabel value v in place to tag after it has been repurposed by an in-place op
 * (e.g. a pointer that became a write cursor). Updates the dictionary too.
 */
static inline void c_relabel(CValue* v, const char* tag) {
    AsmDataField n = change_tag(c_cell(*v), tag);
    if (v->id > 0 && v->id < c_nowned) c_owned[v->id].tag = n.type;
    v->tag = n.type;
}

/* Release a value's register back to the pool. The value may still be
 * read if the caller kept the location; c_dispose is the form that also
 * makes a later use a generation error. */
static inline void c_free(CValue* v) {
    CValue* c = v;
    if (v->id > 0 && v->id < c_nowned) c = &c_owned[v->id];
    if (c->reg >= 0) {
        c_regs[c->reg].in_use = false;
        if (c_reg_owner[c->reg] == c) c_reg_owner[c->reg] = NULL;
        c_reg_owner_name[c->reg] = NULL;
        c_reg_size[c->reg] = 0;
    }
    c->live = false;
    c->reg = -1;
    v->live = false;
    v->reg = -1;
}

/* The value is finished. Its register is returned to the pool, a spill
 * slot is released, and the location is marked disposed in the asm layer.
 * A later c_cell, c_assign, or other set is a generation error. A call
 * does not spill or restore it, because it no longer owns a register. */
static inline void c_dispose(CValue* v) {
    CValue* c = v;
    if (v->id > 0 && v->id < c_nowned) c = &c_owned[v->id];
    if (c->loc) asm_dispose_location(c->loc);
    if (c->reg >= 0) {
        int idx = c->reg;
        c_regs[idx].in_use = false;
        if (c_reg_owner[idx] == c) c_reg_owner[idx] = NULL;
        c_reg_owner_name[idx] = NULL;
        c_reg_clobbered_by[idx] = NULL;
        c_reg_size[idx] = 0;
    }
    if (c->spill_slot >= 0) {
        int size = c->type.size;
        if (size != 1 && size != 2 && size != 4 && size != 8) size = 8;
        asm_frame_release(c->spill_slot, size);
    }
    c->disposed = true;
    c->live = false;
    c->reg = -1;
    c->spill_slot = -1;
    v->disposed = true;
    v->live = false;
    v->reg = -1;
    v->spill_slot = -1;
}

/* After this, c_assign, c_assign_imm, c_add, c_sub, c_and, c_and_imm, and
 * c_zero abort. The value can still be read. Call it after the initial
 * store: c_local_imm and c_dup write the value first, then this locks it. */
static inline void c_mark_const(CValue* v) {
    CValue* c = v;
    if (v->id > 0 && v->id < c_nowned) c = &c_owned[v->id];
    c->is_const = true;
    v->is_const = true;
}

/*
 * `rep movsb`: copy `count` bytes from `src` to `dst`. The instruction mandates
 * rdi/rsi/rcx and clobbers all three, so this helper moves the operands into
 * those registers itself — the generator passes ordinary (auto-allocated) values
 * and never names rdi/rsi/rcx. Sources are moved before `count` lands in rcx, so
 * a source that happens to live in rcx is read before it is overwritten. Because
 * the caller keeps its own values in auto-allocated registers (which the pool
 * hands out ahead of rsi/rdi/rcx), rep movsb's clobber never touches a live
 * caller value; reuse a source across several copies by holding it in such a
 * register (e.g. c_dup a parameter first).
 */
static inline void c_rep_movsb(CValue dst, CValue src, CValue count) {
    dst = c_resolve(dst);
    src = c_resolve(src);
    count = c_resolve(count);
    if (!dst.loc || strcmp(dst.loc, "rdi") != 0)
        c_materialize("rdi", dst.loc, dst.tag);
    if (!src.loc || strcmp(src.loc, "rsi") != 0)
        c_materialize("rsi", src.loc, src.tag);
    if (!count.loc || strcmp(count.loc, "rcx") != 0)
        c_materialize("rcx", count.loc, count.tag);
    AsmDataField rdi = { "rdi", dst.tag, NULL };
    AsmDataField rsi = { "rsi", src.tag, NULL };
    AsmDataField rcx = { "rcx", count.tag, NULL };
    write_rep_movsb(rdi, rsi, rcx);
}

/* True when loc is a register rep movsb overwrites. */
static inline int c_loc_is_rep_reg(const char* loc) {
    if (!loc) return 0;
    return strcmp(loc, "rcx") == 0 || strcmp(loc, "rsi") == 0 || strcmp(loc, "rdi") == 0
        || strcmp(loc, "ecx") == 0 || strcmp(loc, "esi") == 0 || strcmp(loc, "edi") == 0;
}

/*
 * c_copy_bytes - copy count bytes from src to dst.
 * A source that lives in rcx, rsi, or rdi is copied into a pool register
 * first, and that register is returned, so a second copy of the same
 * source reloads the saved bytes. The destination value is left where it
 * was. rep movsb still uses rcx for the count of this one copy.
 */
static inline CValue c_copy_bytes(CValue dst, CValue src, CValue count) {
    src = c_resolve(src);
    if (c_loc_is_rep_reg(src.loc))
        src = c_dup(src.tag ? src.tag : "src", src.type, src);
    c_rep_movsb(dst, src, count);
    return src;
}

/* ======================================================================== *
 *  Control flow  (labels generated automatically)
 * ======================================================================== */

/* A pending forward "skip" target closed by c_end_if. `entry` snapshots the
 * clobber state at the branch so a returning body does not leak clobbers. */
typedef struct { const char* end_label; CClobberState entry; int id; } CIf;

/* Open-block stacks. c_end_* crashes unless it closes the most recent block
 * of that kind. Generation is single-threaded; these stacks are process-global. */
#define C_BLOCK_MAX 32
static int c_block_seq = 1;
static int c_if_stack[C_BLOCK_MAX], c_if_sp = 0;
static int c_else_stack[C_BLOCK_MAX], c_else_sp = 0;
static int c_sw_stack[C_BLOCK_MAX], c_sw_sp = 0;
static int c_loop_stack[C_BLOCK_MAX], c_loop_sp = 0;

/* Push a fresh block id onto stack at sp and return that id. */
static inline int c_block_push(int* stack, int* sp) {
    crash_if(*sp >= C_BLOCK_MAX, "too many open control-flow blocks");
    int id = c_block_seq++;
    stack[(*sp)++] = id;
    return id;
}
/* Pop stack at sp when the top entry equals id, crashing with what on a mismatch. */
static inline void c_block_pop(int* stack, int* sp, int id, const char* what) {
    crash_if(*sp <= 0 || stack[*sp - 1] != id, what);
    (*sp)--;
}
/* A named point that can be jumped back to. */
typedef struct { const char* name; } CLabel;

/* Emit a label here and return a handle to jump back to it. */
static inline CLabel c_label(void) {
    CLabel l = { c_new_label() };
    write_label(l.name);
    c_path_dead = false;   /* a label is a join: reachable */
    return l;
}

/* Reserve a label that c_place_label will emit later. c_goto may target it
 * before it is placed. */
static inline CLabel c_forward_label(void) {
    CLabel label = { c_new_label() };
    return label;
}

/*
 * c_place_label - emit label here and make the following code reachable.
 * label is the handle c_forward_label returned.
 */
static inline void c_place_label(CLabel label) {
    write_label(label.name);
    c_path_dead = false;
}

/* Unconditional jump to a previously created label. */
static inline void c_goto(CLabel l) { write_jmp(l.name); c_path_dead = true; }

/* A loop. c_loop_begin emits the start label and indents the body.
 * c_loop_end jumps back and emits the exit label. */
typedef struct { const char* start; const char* end; int id; } CLoop;

/* Open a loop, emit its start label, and indent the body. */
static inline CLoop c_loop_begin(void) {
    CLoop L;
    L.start = c_new_label();
    L.end = c_new_label();
    L.id = c_block_push(c_loop_stack, &c_loop_sp);
    write_label(L.start);
    asm_indent_level++;
    c_path_dead = false;
    return L;
}
/* Jump to the start label of L and mark the path after it dead. */
static inline void c_loop_continue(CLoop* L) { write_jmp(L->start); c_path_dead = true; }
/* Jump to the end label of L and mark the path after it dead. */
static inline void c_loop_break(CLoop* L) { write_jmp(L->end); c_path_dead = true; }
/* Close loop L: pop its block, jump back to the start while the path is live,
 * and emit the exit label. */
static inline void c_loop_end(CLoop* L) {
    c_block_pop(c_loop_stack, &c_loop_sp, L->id, "c_loop_end does not close the most recent open loop");
    if (!c_path_dead) write_jmp(L->start);
    asm_indent_level--;
    c_path_dead = false;
    write_label(L->end);
}

/*
 * The one-armed conditionals below run the guarded body (the code between
 * the c_if_* call and its matching c_end_if) exactly when the named condition
 * holds. Each is implemented by branching past the body on the negated
 * condition, using asm_funcs.h's folded compare+jump helpers.
 *
 * a and b are the compared values. ult and slt are unsigned and signed
 * less-than. ugt and sgt are the greater-than forms. uge is unsigned
 * greater-or-equal. eq and ne are equality and inequality.
 */

/* body runs when a == b */
static inline CIf c_if_eq(CValue a, CValue b) {
    CIf c = { c_new_label() };
    write_if_not_equal_then_jump_to(c.end_label, c_cell(a), c_cell(b));
    c.entry = c_clobber_save();
    c.id = c_block_push(c_if_stack, &c_if_sp);
    return c;
}
/* body runs when a >  b (unsigned) */
static inline CIf c_if_ugt(CValue a, CValue b) {
    CIf c = { c_new_label() };
    write_if_unsigned_less_than_or_equal_then_jump_to(c.end_label, c_cell(a), c_cell(b));
    c.entry = c_clobber_save();
    c.id = c_block_push(c_if_stack, &c_if_sp);
    return c;
}
/* body runs when a >= b (unsigned) */
static inline CIf c_if_uge(CValue a, CValue b) {
    CIf c = { c_new_label() };
    write_if_unsigned_less_than_then_jump_to(c.end_label, c_cell(a), c_cell(b));
    c.entry = c_clobber_save();
    c.id = c_block_push(c_if_stack, &c_if_sp);
    return c;
}
/* body runs when a >  b (signed) */
static inline CIf c_if_sgt(CValue a, CValue b) {
    CIf c = { c_new_label() };
    write_if_signed_less_than_or_equal_then_jump_to(c.end_label, c_cell(a), c_cell(b));
    c.entry = c_clobber_save();
    c.id = c_block_push(c_if_stack, &c_if_sp);
    return c;
}
/* Run the body when a is unsigned-less than b.
 * a is the left-hand value. b is the right-hand value. */
static inline CIf c_if_ult(CValue a, CValue b) {
    CIf c = { c_new_label() };
    write_if_unsigned_greater_than_or_equal_then_jump_to(c.end_label, c_cell(a), c_cell(b));
    c.entry = c_clobber_save();
    c.id = c_block_push(c_if_stack, &c_if_sp);
    return c;
}
/* Run the body when a is signed-less than b.
 * a is the left-hand value. b is the right-hand value. */
static inline CIf c_if_slt(CValue a, CValue b) {
    CIf c = { c_new_label() };
    write_if_signed_greater_than_or_equal_then_jump_to(c.end_label, c_cell(a), c_cell(b));
    c.entry = c_clobber_save();
    c.id = c_block_push(c_if_stack, &c_if_sp);
    return c;
}
/* Run the body when a is not equal to b.
 * a is the left-hand value. b is the right-hand value. */
static inline CIf c_if_ne(CValue a, CValue b) {
    CIf c = { c_new_label() };
    write_if_equal_then_jump_to(c.end_label, c_cell(a), c_cell(b));
    c.entry = c_clobber_save();
    c.id = c_block_push(c_if_stack, &c_if_sp);
    return c;
}

/* Close a structured conditional (emit its skip target). The join is reachable
 * from the skip edge (entry state) and, unless the body returned/jumped away,
 * from the body's fall-through - so merge the two. */
static inline void c_end_if(CIf* c) {
    c_block_pop(c_if_stack, &c_if_sp, c->id, "c_end_if does not close the most recent open if");
    if (c_path_dead) {
        c_clobber_restore(c->entry);      /* body cannot reach the join */
    } else {
        c_clobber_union_into(c->entry);   /* join = body-path OR skip-path */
    }
    c_path_dead = false;
    write_label(c->end_label);
}

/* ------------------------------------------------------------------------ *
 *  Two-armed conditionals (if / else)
 * ------------------------------------------------------------------------ *
 *
 * A CElse carries two forward labels: `else_label` (jumped to when the guard
 * fails) and `done_label` (the join point after both arms). Usage mirrors the
 * one-armed forms:
 *
 *     CElse c = c_if_eq_else(a, b);
 *         <then-body: runs when a == b>
 *     c_else(&c);
 *         <else-body: runs when a != b>
 *     c_end_if_else(&c);
 *
 * Each constructor branches to else_label on the NEGATED condition; c_else jumps
 * the then-body over the else-body and drops the else_label; c_end_if_else emits
 * the join label.
 */
typedef struct {
    const char*   else_label;
    const char*   done_label;
    CClobberState entry;        /* state at the branch (start of both arms) */
    CClobberState then_state;   /* clobber state at the end of the then-arm */
    bool          then_dead;    /* did the then-arm return / jump away? */
    int           id;
} CElse;

/* then-body runs when a == b (else when a != b) */
static inline CElse c_if_eq_else(CValue a, CValue b) {
    CElse c; c.else_label = c_new_label(); c.done_label = c_new_label();
    write_if_not_equal_then_jump_to(c.else_label, c_cell(a), c_cell(b));
    c.entry = c_clobber_save();
    c.id = c_block_push(c_else_stack, &c_else_sp);
    return c;
}
/* then-body runs when a != b (else when a == b) */
static inline CElse c_if_ne_else(CValue a, CValue b) {
    CElse c; c.else_label = c_new_label(); c.done_label = c_new_label();
    write_cmp(c_cell(a), c_cell(b));
    write_je(c.else_label);
    c.entry = c_clobber_save();
    c.id = c_block_push(c_else_stack, &c_else_sp);
    return c;
}

/* Switch to the else arm: remember how the then-arm left things, then reset the
 * clobber state to the branch entry so the else-arm starts clean. */
static inline void c_else(CElse* c) {
    write_jmp(c->done_label);
    c->then_state = c_clobber_save();
    c->then_dead  = c_path_dead;
    c_clobber_restore(c->entry);
    c_path_dead = false;
    write_label(c->else_label);
}
/* Close a two-armed conditional: the join state is the union of whichever arms
 * fall through to it. */
static inline void c_end_if_else(CElse* c) {
    c_block_pop(c_else_stack, &c_else_sp, c->id,
                "c_end_if_else does not close the most recent open if/else");
    bool else_dead = c_path_dead;
    CClobberState else_state = c_clobber_save();
    c_clobber_clear_all();
    bool any = false;
    if (!c->then_dead) { c_clobber_union_into(c->then_state); any = true; }
    if (!else_dead)    { c_clobber_union_into(else_state);    any = true; }
    if (!any) {
        c_clobber_restore(c->entry);   /* both arms terminated: join unreachable */
        c_path_dead = true;
    } else {
        c_path_dead = false;
    }
    write_label(c->done_label);
}

/* ------------------------------------------------------------------------ *
 *  Multiway switch on a value (equality cases, no fall-through)
 * ------------------------------------------------------------------------ *
 *
 * Each case compares the switch value against a constant and, on a mismatch,
 * skips to the next case; a matching case falls into its body. Because there is
 * no implicit fall-through, a case body must divert control itself (return, or
 * c_break) before the next case is reached:
 *
 *     CSwitch sw = c_switch(count);
 *         c_case(&sw, "1"); <body>            (body ends by returning / c_break)
 *         c_case(&sw, "2"); <body>
 *         c_default(&sw);   <body>
 *     c_end_switch(&sw);
 */
typedef struct { CValue v; const char* skip; const char* done; CClobberState entry; int id; } CSwitch;

/* Open a switch on value v and return its handle with a fresh join label. */
static inline CSwitch c_switch(CValue v) {
    /* skip starts NULL: no case label exists until the first c_case.
     * done is a fresh join label. id is assigned below. */
    CSwitch s = { v, NULL, c_new_label() };
    s.entry = c_clobber_save();
    s.id = c_block_push(c_sw_stack, &c_sw_sp);
    return s;
}
/* Open a case for constant `k`; on mismatch jump past this case's body. Each
 * case body is entered from the compare chain, so it starts from the switch's
 * entry clobber state. */
static inline void c_case(CSwitch* s, const char* k) {
    if (s->skip) write_label(s->skip);
    s->skip = c_new_label();
    write_if_not_equal_then_jump_to(s->skip, c_cell(s->v), literal(k, s->v.tag));
    c_clobber_restore(s->entry);
    c_path_dead = false;
}

/* A case whose constant is a number. The driver passes the number. */
static inline void c_case_n(CSwitch* s, long long k) {
    c_case(s, c_dec(k));
}
/* Jump out of the switch (skip remaining cases). */
static inline void c_break(CSwitch* s) { write_jmp(s->done); c_path_dead = true; }
/* The fall-through (default) arm: land here when no case matched. */
static inline void c_default(CSwitch* s) {
    if (s->skip) { write_label(s->skip); s->skip = NULL; }
    c_clobber_restore(s->entry);
    c_path_dead = false;
}
/* Close the switch: emit any pending skip target and the join label. Reaching
 * past the switch is treated (conservatively) as the entry state. */
static inline void c_end_switch(CSwitch* s) {
    c_block_pop(c_sw_stack, &c_sw_sp, s->id, "c_end_switch does not close the most recent open switch");
    if (s->skip) write_label(s->skip);
    c_clobber_restore(s->entry);
    c_path_dead = false;
    write_label(s->done);
}

/* ======================================================================== *
 *  Calls  (arguments marshalled by asm_funcs; result bound to a CValue)
 * ======================================================================== */

/*
 * Bind a just-completed call's return register as a CValue. The underlying
 * write_call/write_extern_call already recorded the return register in the local
 * dictionary, so this only wraps it. A void callee yields a value with loc==NULL,
 * which the caller discards.
 */
static inline CValue c_bind_return(const AsmExtern* ext, CType type) {
    CValue v;
    v.name = ext->return_reg.name;
    v.type = type;
    v.loc  = ext->return_reg.name;
    v.tag  = ext->return_reg.type;
    /* reg is an index into the 16-entry pool, or -1 when this CValue does not
     * own a pool entry. The return register is the ABI's (rax, or eax for a
     * narrower result). The caller has not allocated it. c_dup and c_into are
     * what put a value in the pool. spill_slot -1 means it has no frame slot
     * either. live is false for a void callee, whose return name is NULL, so
     * a later read of this value fails instead of emitting a move from a
     * register that was never written. */
    v.reg  = -1;
    v.live = (ext->return_reg.name != NULL);
    v.spill_slot = -1;
    v.id = 0;
    v.disposed = false;
    v.is_const = false;
    return v;
}

/*
 * c_call_ext / c_call_proc - Emit a call and evaluate to a CValue holding the
 * result. Arguments are asm operand cells (immediates via literal(), globals /
 * offsets, or c_cell(local)), exactly as the underlying call expects. The first
 * parameter is the C type to give the returned value.
 *
 * Before the call, every named value sitting in a register the callee may
 * clobber is copied to a frame slot and restored to that same register
 * afterwards. The CValue still names the register. A value that cannot
 * be saved (spill off, or no frame) is poisoned, and a later read is a
 * generation error. The result is copied out of the return register before
 * that restore, so a live value that was in rax is put back and the result
 * lives in a different register.
 *
 * (c_call_ext keeps the full extern scaffolding of write_extern_call, including
 * stack arguments at qword ptr [rsp+40] and the slots after it. c_call_proc
 * uses write_call, which moves register arguments and does not store a stack
 * argument. A c_begin_function with spilling on names its fifth parameter
 * qword ptr [rbp+48], and write_extern_call rejects that spelling.)
 */
/* These are functions. C has no overloads, so the four shapes (result or void,
 * Windows x64 frame or private convention) are four functions. The return type
 * stays the first parameter, so existing call sites keep their argument order.
 * The variadic tail is the operand cells.
 *
 * They are noinline, and va_start is the first statement. MSVC's va_start
 * reads the home slots of the real call frame. Inlining a variadic function
 * whose first parameter is a 16-byte CType drops those slots: the x64
 * convention passes that struct as a pointer, while the va_arg macro in
 * host.h still steps one 8-byte slot. A call before va_start can move the
 * frame the macro reads. */
static inline CValue c_finish_call(const AsmExtern* ext, CType type, int hold) {
    unsigned mask = c_call_mask(ext);
    if (hold >= 0 && ext->return_reg.name) {
        const char* loc = c_reg_name(hold, type.size);
        c_materialize(loc, ext->return_reg.name, ext->return_reg.type);
        c_reg_owner_name[hold] = ext->return_reg.type;
        c_reg_size[hold] = type.size;
        c_reg_clobbered_by[hold] = NULL;
    }
    c_restore_across(ext->name, mask, hold);
    CValue v = c_bind_return(ext, type);
    if (hold < 0) return v;
    v.loc = c_reg_name(hold, type.size);
    v.reg = hold;
    v.live = true;
    v.name = ext->return_reg.type;
    v.tag = ext->return_reg.type;
    v.spill_slot = -1;
    v.id = 0;
    return c_track(v, 1);
}
/* Call the Windows import ext, saving live registers across the call, and bind
 * the result as ret_ctype. The ... tail is the operand cells. */
__declspec(noinline) static CValue c_call_ext(CType ret_ctype, const AsmExtern* ext, ...) {
    va_list args;
    va_start(args, ext);
    int hold = ext->return_reg.name ? c_hold_reg() : -1;
    /* c_save_across copies every named live GPR that this callee may
     * clobber into a fresh frame slot. The CValue keeps its register name.
     * c_restore_across, inside c_finish_call, moves those slots back and
     * releases them. The result is copied into `hold` first, so a restore
     * of rax does not wipe the value just returned. A value with no owner
     * name and no canonical slot is not saved; a later read is the clobber
     * error in c_cell. */
    c_save_across(c_call_mask(ext));
    write_extern_callv(ext, args);
    va_end(args);
    return c_finish_call(ext, ret_ctype, hold);
}
/* Call the private procedure proc, saving live registers across the call, and
 * bind the result as ret_ctype. The ... tail is the operand cells. */
__declspec(noinline) static CValue c_call_proc(CType ret_ctype, const AsmExtern* proc, ...) {
    va_list args;
    va_start(args, proc);
    int hold = proc->return_reg.name ? c_hold_reg() : -1;
    /* Same save as c_call_ext: named live GPRs in the callee's clobber
     * mask are copied to the frame for the duration of this private call. */
    c_save_across(c_call_mask(proc));
    write_callv(proc, args);
    va_end(args);
    return c_finish_call(proc, ret_ctype, hold);
}
/* Call the Windows import ext, saving live registers across the call and
 * restoring them after. The ... tail is the operand cells. */
__declspec(noinline) static void c_call_ext_void(const AsmExtern* ext, ...) {
    va_list args;
    va_start(args, ext);
    /* Save named live GPRs the import may clobber. There is no result to
     * hold, so c_restore_across writes every saved register back. */
    c_save_across(c_call_mask(ext));
    write_extern_callv(ext, args);
    va_end(args);
    c_restore_across(ext->name, c_call_mask(ext), -1);
}
/* Call the private procedure proc, saving live registers across the call and
 * restoring them after. The ... tail is the operand cells. */
__declspec(noinline) static void c_call_proc_void(const AsmExtern* proc, ...) {
    va_list args;
    va_start(args, proc);
    /* Save named live GPRs this private callee may clobber, then restore
     * them after the call. No result is kept. */
    c_save_across(c_call_mask(proc));
    write_callv(proc, args);
    va_end(args);
    c_restore_across(proc->name, c_call_mask(proc), -1);
}

/*
 * c_call - call an internal procedure described by a CFunc, ignoring any result.
 *
 * Hides both the raw write_call emitter and the CFunc's internal `.ext`
 * descriptor from generators. Variadic (works with zero or more argument cells), so
 * it covers both nullary procedures (c_call(&run)) and those taking arguments
 * (c_call(&render, c_addr("circle", "this"))). For a procedure whose result you
 * need as a CValue, use the c_call_proc macro with &fn->ext instead.
 */
static inline void c_call(CFunc* f, ...) {
    va_list args;
    va_start(args, f);
    /* Save named live GPRs f may clobber. The result, if any, is ignored,
     * so the restore writes every saved register back. */
    c_save_across(c_call_mask(&f->ext));
    write_callv(&f->ext, args);
    va_end(args);
    c_restore_across(f->ext.name, c_call_mask(&f->ext), -1);
}

/* ======================================================================== *
 *  Structs  (field offsets computed automatically)
 * ======================================================================== */

#define C_STRUCT_MAX_FIELDS 32
#define C_STRUCT_MAX_ENTRIES 64   /* fields + inserted padding */

/* One declared struct field: a name (blank for the base alias), an initializer
 * expression (e.g. "0", "offset WndProc", "?"), a type tag (used with the asm
 * layer, mirroring set_memory_layout_entry), and its C type (for size and
 * alignment only). */
typedef struct {
    const char* name;
    const char* value;
    const char* tag;
    CType       type;
    int         align_mode; /* -1 = use the struct mode; otherwise an AlignMode */
} CStructFieldSpec;

/* A struct being described. `pad_value` is the initializer used for inserted
 * alignment padding ("0" in an initialised .data section, "?" in .data?). */
typedef struct {
    const char*      base_name;
    const char*      pad_value;
    CStructFieldSpec fields[C_STRUCT_MAX_FIELDS];
    int              nfield;
    AlignMode        align; /* captured from align_get_default() at begin */
} CStruct;

/* The emitted struct: field name -> sized memory operand (padding excluded). */
typedef struct {
    const char*  names[C_STRUCT_MAX_ENTRIES];
    AsmDataField refs[C_STRUCT_MAX_ENTRIES];
    int          count;
} CStructRef;

/* Begin describing a struct whose base label is `base_name`. `pad_value` is the
 * initializer for auto-inserted alignment padding: "0" in a .data section, "?"
 * in an uninitialised .data? section. */
static inline CStruct c_struct_begin(const char* base_name, const char* pad_value) {
    CStruct s;
    s.base_name = base_name;
    s.pad_value = pad_value;
    s.nfield = 0;
    s.align = align_get_default();
    return s;
}

/* Set the alignment of struct s to mode. */
static inline void c_struct_set_align(CStruct* s, AlignMode mode) { s->align = mode; }

/* Append a field. Layout uses the struct's AlignMode (see align_pad):
 * ALIGN_NONE packs tight and any padding must be an explicit blank field;
 * ALIGN_C is natural alignment capped at 8, which is what Win32 interop
 * structs need and what makes aligned 1/2/4/8-byte accesses atomic on x64;
 * ALIGN_CACHE and ALIGN_PAGE also refuse to straddle a 64-byte line or a
 * 4096-byte page. A split line is not atomic. A split page can fault in
 * the middle of one access. Both cost extra bytes and avoid those stalls.
 *
 * A blank name on the first field means that field's label is the struct
 * base name passed to c_struct_begin. Later code takes the address of the
 * struct through that label (c_struct_base / "offset wndClass").
 */
static inline void c_struct_field(CStruct* s, const char* name,
                                  const char* value, const char* tag, CType type) {
    crash_if(s->nfield >= C_STRUCT_MAX_FIELDS, "c_struct_field: too many fields");
    s->fields[s->nfield].name  = name;
    s->fields[s->nfield].value = value;
    s->fields[s->nfield].tag   = tag;
    s->fields[s->nfield].type  = type;
    s->fields[s->nfield].align_mode = -1;
    s->nfield++;
}

/* Append field name to struct s with initializer value, tag, and type, then
 * set its alignment to mode. */
static inline void c_struct_field_mode(CStruct* s, const char* name,
                                       const char* value, const char* tag,
                                       CType type, AlignMode mode) {
    c_struct_field(s, name, value, tag, type);
    s->fields[s->nfield - 1].align_mode = (int)mode;
}

/*
 * Finalise the struct: compute offsets (inserting padding fields where a member
 * would otherwise be mis-aligned), emit it into the current data section via the
 * asm memory-layout machinery, and return per-field references. c_field then
 * resolves a member by name to its sized memory operand.
 */
static inline CStructRef c_struct_emit(CStruct* s) {
    /* Build layout entries with padding inserted for alignment. */
    struct { const char* name; const char* value; const char* tag; int off; int size; }
        entries[C_STRUCT_MAX_ENTRIES];
    int ne = 0;
    int off = 0;
    int pad_seq = 0;

    for (int i = 0; i < s->nfield; i++) {
        int size = s->fields[i].type.size;
        AlignMode mode = s->fields[i].align_mode < 0 ? s->align
                         : (AlignMode)s->fields[i].align_mode;
        int pad = align_pad(off, size, mode);
        if (pad != 0) {
            /* A pad bigger than 8 bytes is one db N dup entry. Splitting a
             * page slide into 4-byte pieces overflows C_STRUCT_MAX_ENTRIES
             * before the field is reached. Smaller pads stay 4/2/1 pieces. */
            if (pad > 8) {
                if (ne >= C_STRUCT_MAX_ENTRIES) { fprintf(stderr, "c_struct_emit: overflow\n"); exit(1); }
                entries[ne].name  = "";
                entries[ne].value = s->pad_value;
                entries[ne].tag   = "pad";
                entries[ne].off   = off;
                entries[ne].size  = pad;
                off += pad;
                ne++;
                (void)pad_seq;
            } else {
                while (pad > 0) {
                    int chunk = pad >= 4 ? 4 : (pad >= 2 ? 2 : 1);
                    if (ne >= C_STRUCT_MAX_ENTRIES) { fprintf(stderr, "c_struct_emit: overflow\n"); exit(1); }
                    entries[ne].name  = "";
                    entries[ne].value = s->pad_value;
                    entries[ne].tag   = "pad";
                    entries[ne].off   = off;
                    entries[ne].size  = chunk;
                    off += chunk;
                    pad -= chunk;
                    ne++;
                    (void)pad_seq;
                }
            }
        }
        if (ne >= C_STRUCT_MAX_ENTRIES) { fprintf(stderr, "c_struct_emit: overflow\n"); exit(1); }
        entries[ne].name  = (i == 0 && !s->fields[i].name[0]) ? s->base_name : s->fields[i].name;
        entries[ne].value = s->fields[i].value;
        entries[ne].tag   = s->fields[i].tag;
        entries[ne].off   = off;
        entries[ne].size  = size;
        off += size;
        ne++;
    }

    AsmMemoryLayout* layout = create_memory_layout(ne, s->base_name);
    for (int i = 0; i < ne; i++) {
        set_memory_layout_entry(i, entries[i].name, entries[i].value,
                                entries[i].tag, entries[i].off, entries[i].size);
    }
    AsmMemoryLayoutRefs refs = write_data_memory_layout(layout);

    CStructRef out;
    out.count = refs.count;
    for (int i = 0; i < refs.count && i < C_STRUCT_MAX_ENTRIES; i++) {
        out.names[i] = entries[i].name;
        out.refs[i]  = refs.fields[i];
    }
    return out;
}

/* Resolve a struct member to a memory-resident CValue by field name.
 * fname selects the member of r, and type is the cell's C type. */
static inline CValue c_field(CStructRef* r, const char* fname, CType type) {
    for (int i = 0; i < r->count; i++) {
        if (r->names[i] && r->names[i][0] && strcmp(r->names[i], fname) == 0) {
            return c_wrap(r->refs[i], type);
        }
    }
    char buf[180];
    snprintf(buf, sizeof buf, "c_field: no field \"%s\"\n", fname);
    host_fail(buf);
}

/*
 * The struct's base label as a bare operand for "offset".
 *
 * base_name, stored by c_struct_begin, is the MASM label of byte 0. When the
 * first field's name is blank, c_struct_emit stores that base name in names[0],
 * so the label and the first field are the same address. "offset wndClass" is
 * the pointer an API wants. "qword ptr wndClass" would load the first qword,
 * which is a different value. The tag argument is what the dictionary checks
 * when that address is passed to a call. c_field is the other lookup: one
 * named member, as a sized memory operand.
 */
static inline AsmDataField c_struct_base(CStructRef* r, const char* tag) {
    return c_addr(r->names[0], tag);
}

/* Remember the output path and emit option casemap:none. */
static const char* c_prog_file;

static inline void c_prog_begin(const char* path) {
    c_prog_file = path;
    write_set_case_sensitive_identifiers();
}

/* Close the file. Returns what write_asm_file returns. */
static inline int c_prog_end(void) {
    write_end("");
    return write_asm_file(c_prog_file);
}

