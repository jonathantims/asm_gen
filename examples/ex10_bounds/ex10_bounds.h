/*
 * ex10_bounds.h - generation-time array/pointer bounds checking.
 *
 * A "fat pointer" bundles a base address with the element size and element
 * count. When the generation-time flag is on, every indexed access emits a
 * range check (`cmp index,count` + unsigned `jae trap`, which also catches
 * negative indices) before the load. When off, the fat pointer degrades to a
 * bare base pointer and the access is a plain `mov` with no check - zero
 * footprint, exactly what an unchecked C array access compiles to.
 *
 * The check itself is one compare and one conditional jump. The generator around
 * it uses c_funcs.h.
 */


static bool bounds_enabled = false;
/* Turn this example's bounds checking on before emission. */
static inline void bounds_enable(void)  { bounds_enabled = true; }
/* Turn this example's bounds checking off before emission. */
static inline void bounds_disable(void) { bounds_enabled = false; }

/* A generation-time fat pointer: base symbol + element size + element count. */
typedef struct { const char* base; int elem_size; int count; } FatPtr;
/* Pack base, elem_size, and count into a fat pointer. */
static inline FatPtr fatptr(const char* base, int elem_size, int count) {
    FatPtr f = { base, elem_size, count }; return f;
}

typedef struct { const char* trap_label; bool active; } BoundsCtx;

/* Emit the trap handler once (only when enabled): print msg via print
 * and exit 1 through con. */
static inline BoundsCtx bounds_begin(ExConsole* con, AsmExtern* print, ExStr msg) {
    BoundsCtx c = { "bounds_trap", bounds_enabled };
    if (bounds_enabled) {
        write_jmp("bounds_after_trap");
        write_label("bounds_trap");
            ex_puts(print, msg);
            ex_exit(con, 1);
        write_label("bounds_after_trap");
    }
    return c;
}

/*
 * bounds_load32 - load one 4-byte element of fp as a variable named elem.
 * c is the bounds context (its trap is taken when the check is on). fp is
 * the fat pointer. index_imm is a decimal index literal. When checking is
 * on, the index is compared with fp.count first and an out-of-range index
 * takes the trap. The scaled load stays in this function: c_funcs.h has no
 * indexed memory operand. The caller widens elem when a wider value is
 * required. The returned variable owns its storage, so the caller frees it.
 */
static inline CValue bounds_load32(BoundsCtx* c, FatPtr fp, int index) {
    const char* index_imm = c_dec(index);
    write_replace_mov("rcx", (AsmDataField){ index_imm, "index", NULL });
    if (c->active) {
        char cnt[24];
        snprintf(cnt, sizeof cnt, "%d", fp.count);
        write_cmp((AsmDataField){"rcx","index",NULL}, literal(c_strdup_arena(cnt), "count"));
        write_jae(c->trap_label);        /* index >= count (or negative) -> trap */
    }
    /* Load the base address RIP-relative (a bare `[sym + rcx*n]` operand would
     * force a 32-bit absolute relocation that fails to link on x64), then index
     * off registers only. */
    write_lea("rdx", fp.base);
    write_replace_mov("eax", (AsmDataField){ asm_mem_index("dword", "rdx", "rcx", fp.elem_size, 0), "elem", NULL });
    int idx = c_reg_index("eax");
    c_regs[idx].in_use = true;
    c_reg_owner_name[idx] = "elem";
    c_reg_size[idx] = 4;
    c_reg_revalidate("eax");
    CValue elem = { "elem", C_INT, "eax", "elem", idx, true, -1 };
    return c_track(elem, 1);
}
