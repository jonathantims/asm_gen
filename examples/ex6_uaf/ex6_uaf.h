/*
 * ex6_uaf.h - generation-time use-after-free detection.
 *
 * THE IDEA (shared by every example in this folder)
 * -------------------------------------------------
 * A "debug feature" lives in the *generator*, gated by a single generation-time
 * flag. When off, the object helpers emit exactly the allocate / use / free code
 * they always would - ZERO FOOTPRINT. When on, every object is given a small
 * integer ID and a debug-only global "liveness" table records whether each ID is
 * currently in-use (1) or freed (0). Each *use* is preceded by a check that the
 * object's table entry is still in-use; a use of a freed object diverts to a
 * trap handler (prints + exits 1).
 *
 * The table is a plain byte array indexed by object ID. It is addressed
 * RIP-relative (`lea rax,uaf_table` then `[rax+id]`) so no 32-bit absolute
 * relocation is needed. Because IDs are assigned at generation time, each check
 * is a fixed `[rax+<constant>]`.
 */


/* The generation-time flag. Off by default => zero footprint. */
static bool uaf_enabled = false;
/* Turn this example's use-after-free checks on before emission. Takes no parameters. */
static inline void uaf_enable(void)  { uaf_enabled = true; }
/* Turn this example's use-after-free checks off before emission. Takes no parameters. */
static inline void uaf_disable(void) { uaf_enabled = false; }

/* How many object IDs the liveness table has room for (only sized/emitted when
 * the feature is enabled). */
#define UAF_TABLE_SIZE 64

/* A generation-time object handle: a data symbol holding the object's payload
 * plus the integer ID used to index the liveness table. */
typedef struct { const char* sym; int id; } UafObj;

typedef struct { const char* trap_label; bool active; } UafCtx;

/* Emit the liveness table (only meaningful when enabled). Call in write_dataq().
 * Marked ".data?"-style zero-initialised; entries are set to 1 on allocation. */
static inline void uaf_globals(void) {
    if (uaf_enabled) {
        c_ensure_bss();
        write_db("uaf_table", asm_dup(UAF_TABLE_SIZE, "0"));
    }
}

/* Emit the shared trap handler once (only when enabled): realigns the stack
 * (reached by a jump, never returns), prints msg through print, and exits 1
 * on con. */
static inline UafCtx uaf_begin(ExConsole* con, AsmExtern* print, ExStr msg) {
    UafCtx c = { "uaf_trap", uaf_enabled };
    if (uaf_enabled) {
        write_jmp("uaf_after_trap");
        write_label("uaf_trap");
            write_and("rsp", "-16");
            write_sub((AsmDataField){ "rsp", "rsp", NULL }, literal("8", "frame"));
            ex_puts(print, msg);
            ex_exit(con, 1);
        write_label("uaf_after_trap");
    }
    return c;
}

/* Set the liveness table entry for `id` to `val` (1 = in-use, 0 = freed). */
static inline void uaf_mark(int id, int val) {
    write_lea("rax", "uaf_table");
    write_replace_mov(asm_mem("byte", "rax", id), (AsmDataField){ val ? "1" : "0", "live", NULL });
}

/*
 * uaf_new - "allocate" an object: bind it to data symbol `sym` and id `id`, and
 * (when enabled) mark its table entry in-use. Returns the handle used by
 * uaf_use / uaf_free. When disabled this only records the symbol/id in C - no
 * code is emitted.
 */
static int uaf_next_id = 0;

/* Bind sym to the next liveness slot. The header numbers the table. */
static inline UafObj uaf_new(const char* sym) {
    int id = uaf_next_id++;
    UafObj o = { sym, id };
    if (uaf_enabled) uaf_mark(id, 1);
    return o;
}

/* uaf_free - "free" the object: when enabled, mark its table entry freed. */
static inline void uaf_free(UafObj o) {
    if (uaf_enabled) uaf_mark(o.id, 0);
}

/*
 * uaf_check - guard a use: when enabled, verify the object's table entry is
 * still in-use (== 1); if not, jump to the trap. No-op when disabled. Uses rax
 * as scratch. Callers use uaf_use, which calls this.
 */
static inline void uaf_check(UafCtx* c, UafObj o) {
    if (!c->active) return;
    write_lea("rax", "uaf_table");
    write_cmp((AsmDataField){ asm_mem("byte", "rax", o.id), "live", NULL },
              literal("1", "live"));
    write_jne(c->trap_label);        /* entry != in-use -> use-after-free */
}

/* uaf_use - read an object by printing it. uaf_check runs first, so the
 * print happens only when the liveness byte is 1. When the feature is off
 * the check emits nothing and this is a plain print of the payload qword. */
static inline void uaf_use(UafCtx* ctx, AsmExtern* print, AsmExtern* pu64,
                           ExStr label, ExStr nl, UafObj o) {
    uaf_check(ctx, o);
    ex_puts(print, label);
    c_call_proc_void(pu64, (AsmDataField){ o.sym, "value", NULL });
    ex_puts(print, nl);
}

/* uaf_store - write immediate imm as a qword through object o. The same check
 * as uaf_use runs first on ctx, so a store to a freed object traps when the
 * feature is on and is a plain store when it is off. */
static inline void uaf_store(UafCtx* ctx, UafObj o, const char* imm) {
    uaf_check(ctx, o);
    write_replace_mov(asm_mem("qword", o.sym, 0), (AsmDataField){ imm, "payload", NULL });
}

