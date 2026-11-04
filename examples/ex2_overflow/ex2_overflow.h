/*
 * ex2_overflow.h - generation-time integer-overflow checking.
 *
 * THE IDEA (shared by every example in this folder)
 * -------------------------------------------------
 * A "debug feature" is compiled into the *generator*, not the generated program.
 * A single generation-time flag decides whether extra machine code is woven into
 * the emitted assembly. When the flag is off, the wrappers emit exactly the same
 * instructions they always would - so the feature has ZERO FOOTPRINT in the
 * shipped binary. When the flag is on, each arithmetic step is followed by a
 * hardware overflow test that diverts to a trap handler.
 *
 * This file weaves an overflow check after signed integer add/sub/mul. x86 sets
 * the OF (overflow) flag on signed wraparound; the woven code is a single
 * `jo <trap>` immediately after the arithmetic instruction, so it is as cheap as
 * a check can be, and completely absent when disabled.
 */


/* The generation-time flag. Off by default => zero footprint. */
static bool ovf_enabled = false;
/* Turn this example's overflow checks on before emission. Takes no parameters. */
static inline void ovf_enable(void)  { ovf_enabled = true; }
/* Turn this example's overflow checks off before emission. Takes no parameters. */
static inline void ovf_disable(void) { ovf_enabled = false; }

/* Handle carrying the trap label the woven `jo` jumps to. Only meaningful when
 * the feature is enabled (otherwise no trap code is emitted at all). */
typedef struct { const char* trap_label; bool active; } OvfCtx;

/*
 * ovf_begin - if the feature is enabled, emit the trap handler once: it prints
 * `msg` and exits with code 1. Returns a context the checked-arithmetic helpers
 * consult. When disabled, emits nothing and returns an inactive context.
 *
 * The trap is a plain code label (not a called proc): the woven `jo` jumps into
 * it and it never returns, so there is no call/return overhead.
 */
/* Emit the trap and its message only when checks are on. Call between
 * procedures: the message is data, then the handler is code. */
static inline OvfCtx ovf_begin(ExConsole* con, AsmExtern* print, const char* trap_text) {
    OvfCtx c = { "ovf_trap", ovf_enabled };
    if (ovf_enabled) {
        write_data();
            ExStr msg = ex_line("s_trap", trap_text, 2, 0);
        write_code();
        write_jmp("ovf_after_trap");
        write_label("ovf_trap");
            ex_puts(print, msg);
            ex_exit(con, 1);
        write_label("ovf_after_trap");
    }
    return c;
}

/* Weave the check after an arithmetic instruction (no-op when disabled). */
static inline void ovf_check(OvfCtx* c) {
    if (c->active) write_jo(c->trap_label);
}

/* dst += src (signed), with a woven overflow check when enabled. dst and src are
 * register/memory cells; the ADD is emitted then, if enabled, `jo <trap>`. */
static inline void ovf_add(OvfCtx* c, AsmDataField dst, AsmDataField src) {
    write_add(dst, src);
    ovf_check(c);
}

/* dst -= src (signed), checked when enabled. */
static inline void ovf_sub(OvfCtx* c, AsmDataField dst, AsmDataField src) {
    write_sub(dst, src);
    ovf_check(c);
}

