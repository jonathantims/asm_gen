/*
 * ex4_contracts.h - generation-time code contracts (design-by-contract).
 *
 * THE IDEA (shared by every example in this folder)
 * -------------------------------------------------
 * A "debug feature" is compiled into the *generator*, not the shipped program.
 * A single generation-time flag decides whether contract checks are woven into
 * the emitted assembly. When the flag is off, the contract helpers emit nothing
 * at all - so the feature has ZERO FOOTPRINT in the released binary. When on,
 * each contract becomes a runtime comparison that diverts to a trap handler on
 * violation.
 *
 * Four kinds of contract are supported, all reusing the same emission:
 *   - REQUIRE   (pre-condition):   checked on entry to an operation.
 *   - ENSURE    (post-condition):  checked before an operation returns.
 *   - CLASS INVARIANT:             a property that must always hold on an
 *                                  object's state (checked at chosen points).
 *   - VARIABLE INVARIANT:          a property that must always hold for a value.
 * The four differ only in intent and in the message printed on violation; the
 * generated check is identical, which is exactly the point - a contract is just
 * an assertion tagged with when/why it is expected to hold.
 *
 * A contract compares a value cell against a bound using a signed relation
 * (ct_lt/le/gt/ge/eq/ne). When enabled we emit `cmp value,bound` followed by the
 * *negated* conditional jump to the trap (e.g. REQUIRE value>=bound violates when
 * value<bound, so we emit `jl trap`).
 */


/* The generation-time flag. Off by default => zero footprint. */
static bool contracts_enabled = false;
/* Turn this example's contract checks on before emission. Takes no parameters. */
static inline void contracts_enable(void)  { contracts_enabled = true; }
/* Turn this example's contract checks off before emission. Takes no parameters. */
static inline void contracts_disable(void) { contracts_enabled = false; }

/* Signed relation a contract asserts must hold between a value and a bound. */
typedef enum { CT_LT, CT_LE, CT_GT, CT_GE, CT_EQ, CT_NE } CtRel;

/* Context: what the woven trap handler needs, plus whether we are active. */
typedef struct {
    ExConsole*  con;
    AsmExtern*  print;
    const char* trap_label;
    bool        active;
} CtCtx;

/* Emit the two scratch globals the shared trap uses to receive the specific
 * violation message (address + length). Call inside write_dataq(). */
static inline void ct_globals(void) {
    c_ensure_bss();
    write_unitialised_data("ct_msg", "?");   /* message address */
    write_unitialised_data("ct_len", "?");   /* message length  */
    /* Entry snapshot for a post-condition. Only a checked build reads it. */
    if (contracts_enabled) write_unitialised_data("ct_entry", "?");
}

/*
 * ct_begin - if enabled, emit the single shared trap handler: it prints a fixed
 * banner ("[CONTRACT] violated: ") followed by the specific message the failing
 * check left in the ct_msg/ct_len globals, then exits 1. One handler thus serves
 * every call site. Returns an (in)active context.
 *
 * The handler is reached by a conditional jump from inside a procedure, so the
 * stack is at an arbitrary alignment; since it never returns we simply realign
 * (`and rsp,-16` then `sub rsp,8`) to the standard in-body state the console
 * helpers expect before making any call.
 */
static inline CtCtx ct_begin(ExConsole* con, AsmExtern* print, ExStr banner) {
    CtCtx c = { con, print, "ct_trap", contracts_enabled };
    if (contracts_enabled) {
        write_jmp("ct_after_trap");        /* skip handler in straight-line flow */
        write_label("ct_trap");
            write_and("rsp", "-16");         /* realign (we never return) */
            write_sub((AsmDataField){ "rsp", "rsp", NULL }, literal("8", "frame"));
            ex_puts(print, banner);              /* fixed banner */
            write_replace_mov("rcx", (AsmDataField){ "ct_msg", "lpBuffer", NULL });   /* the specific message */
            write_replace_mov("edx", (AsmDataField){ "dword ptr ct_len", "nBytes", NULL });
            write_call_symbol(print->name);
            ex_exit(con, 1);
        write_label("ct_after_trap");
    }
    return c;
}

/* Emit the negated conditional jump: jump to trap when the asserted relation is
 * VIOLATED. rel is the relation that must hold; we jump on its negation. */
static inline void ct_jump_if_violated(CtCtx* c, CtRel rel) {
    switch (rel) {
        case CT_LT: write_jge(c->trap_label); break;  /* want <  ; violate if >= */
        case CT_LE: write_jg (c->trap_label); break;  /* want <= ; violate if >  */
        case CT_GT: write_jle(c->trap_label); break;  /* want >  ; violate if <= */
        case CT_GE: write_jl (c->trap_label); break;  /* want >= ; violate if <  */
        case CT_EQ: write_jne(c->trap_label); break;  /* want == ; violate if != */
        case CT_NE: write_je (c->trap_label); break;  /* want != ; violate if == */
    }
}

/*
 * ct_check - the one primitive behind all four contract kinds. When enabled,
 * emit `cmp value,bound`, stash the violation message into the ct_msg/ct_len
 * globals, and jump to the trap if the relation does not hold. When disabled,
 * emit nothing.
 *
 * `value` is a register/memory cell holding the quantity (must not be rax, which
 * is used here as scratch); `bound` a decimal literal; `msg` the console string
 * printed after the banner on violation. The message address is materialised
 * RIP-relative via `lea` (a bare offset immediate to memory would need a 32-bit
 * absolute relocation that fails to link on x64).
 */
/* lea overwrites rax. If an earlier move left a tag on rax, retag it
 * before the set, or write_set_mov rejects the source. ct_msg and ct_len
 * are globals registered as "?", so the destination type is "?", not the
 * Win32 parameter name carried by the source. */
static inline void ct_stash_msg(ExStr msg) {
    write_lea("rax", msg.off.name + 4); /* skip "off_" prefix */
    change_tag((AsmDataField){ "rax", "lpBuffer", NULL }, "lpBuffer");
    write_set_mov((AsmDataField){ "ct_msg", "?", NULL },
                  (AsmDataField){ "rax", "lpBuffer", NULL });
    write_set_mov((AsmDataField){ "ct_len", "?", NULL },
                  (AsmDataField){ msg.len.name, "nBytes", NULL });
}

/* When c is active, compare value with bound under rel, stash msg, and jump
 * to the trap when rel is violated. */
static inline void ct_check(CtCtx* c, AsmDataField value, CtRel rel,
                            const char* bound, ExStr msg) {
    if (!c->active) return;
    write_cmp(value, literal(bound, "bound"));
    ct_stash_msg(msg);
    ct_jump_if_violated(c, rel);
}

/* The four named forms. They differ only in intent; each takes its own message
 * so the trap output explains which kind of contract failed. */
/* Pre-condition on a value. The relation is the one that must hold, so
 * ct_require(ctx, amount, CT_GT, "0", msg) reads as amount > 0.
 * c is the context, v the value, r the relation, b the bound, and m the message. */
static inline void ct_require(CtCtx* c, CValue v, CtRel r, const char* b, ExStr m) {
    ct_check(c, c_cell(v), r, b, m);
}
/* Pre-condition that v is greater than bound b. c is the context and m the message. */
static inline void ct_require_gt(CtCtx* c, CValue v, const char* b, ExStr m) {
    ct_require(c, v, CT_GT, b, m);
}
/* Post-condition on cell v. c is the context, r the relation, b the bound, m the message. */
static inline void ct_ensure(CtCtx* c, AsmDataField v, CtRel r, const char* b, ExStr m) {
    ct_check(c, v, r, b, m);   /* post-condition */
}
/* Class invariant on cell v. c is the context, r the relation, b the bound, m the message. */
static inline void ct_class_invariant(CtCtx* c, AsmDataField v, CtRel r, const char* b, ExStr m) {
    ct_check(c, v, r, b, m);   /* object invariant */
}
/* Variable invariant on cell v. c is the context, r the relation, b the bound, m the message. */
static inline void ct_var_invariant(CtCtx* c, AsmDataField v, CtRel r, const char* b, ExStr m) {
    ct_check(c, v, r, b, m);   /* variable invariant */
}

/* Compare two cells. Same trap path as ct_check. c is the context, value the
 * left cell, rel the relation that must hold, and msg the violation text.
 * other is a register or a global captured at function entry, so a
 * post-condition can name the entry value rather than a literal. */
static inline void ct_check_vs(CtCtx* c, AsmDataField value, CtRel rel,
                               AsmDataField other, ExStr msg) {
    if (!c->active) return;
    write_cmp(value, other);
    ct_stash_msg(msg);
    ct_jump_if_violated(c, rel);
}

/* A post-condition does not compare at the call. It snapshots entry_now
 * through rax into slot (mov cannot be memory,memory) and records
 * {slot, rel, msg}. The next ct_method_begin calls ct_posts_reset, which
 * drops a leftover record. At method close, ct_emit_posts compares the
 * final cell with the saved global and jumps to ct_trap on the negated
 * relation (CT_GE becomes jl). A disabled context records nothing and
 * emits nothing. slot for the Account demo is the global ct_entry,
 * registered as "?". */
#define CT_MAX_POST 4
typedef struct {
    const char* slot;
    CtRel       rel;
    ExStr       msg;
} CtPostRec;
static CtPostRec ct_posts[CT_MAX_POST];
static int       ct_npost = 0;
static const char* ct_post_field = NULL;

/* A post-condition belongs to one function. It is a nested check of that
 * function's entry state, recorded while the function is open and emitted
 * when the function closes. ct_posts_reset runs at the start of each wrapped
 * function so the new function does not emit the previous function's posts. */
static inline void ct_posts_reset(void) { ct_npost = 0; }

/* When c is active, copy entry_now through rax into slot and record rel and msg. */
static inline void ct_postcondition(CtCtx* c, AsmDataField entry_now,
                                    const char* slot, CtRel rel, ExStr msg) {
    if (!c->active) return;
    crash_if(ct_npost >= CT_MAX_POST, "too many post-conditions");
    /* The field is a memory operand and the slot is a global. mov cannot
     * take two memory operands, so the snapshot stops in rax. rax is the
     * scratch ct_stash_msg also uses for the message address. The compare
     * itself is ct_emit_posts, at the close, not this recording. */
    write_replace_mov("rax", (AsmDataField){ entry_now.name, "entry", NULL });
    write_set_mov((AsmDataField){ slot, "?", NULL },
                  (AsmDataField){ "rax", "entry", NULL });
    ct_posts[ct_npost].slot = slot;
    ct_posts[ct_npost].rel = rel;
    ct_posts[ct_npost].msg = msg;
    ct_npost++;
    if (entry_now.type) ct_post_field = entry_now.type;
}

/* Emit each recorded post-condition against final_value using c, then clear the list. */
static inline void ct_emit_posts(CtCtx* c, AsmDataField final_value) {
    for (int i = 0; i < ct_npost; i++) {
        AsmDataField saved = { ct_posts[i].slot, "entry", NULL };
        ct_check_vs(c, final_value, ct_posts[i].rel, saved, ct_posts[i].msg);
    }
    ct_npost = 0;
}

/* The class wrappers. The generator describes the invariant and the
 * pre/post conditions. Method open and close insert the calls.
 * The invariant returns 0 when the object is valid and non-zero when it is
 * not. A wrapped close calls it and jumps to ct_trap on a non-zero result.
 * That call is emitted only while contracts are enabled, so a disabled build
 * has no ct_trap. ct_msg and ct_len are still emitted by ct_globals. */
static AsmExtern ct_inv;
static int ct_have_inv = 0;
static ExStr ct_inv_msg;

/* Open a cpp class called name whose later methods can carry contracts. */
static inline void cpp_class_with_contracts_begin(const char* name) {
    cpp_class_begin(name);
}

/* Open the class invariant method. The return tag is bad. */
static inline CFunc* cpp_class_with_contracts_invariant_begin(void) {
    return cpp_method_begin_ret("invariant", "bad");
}

/* Remember invariant procedure f and the violation message msg. */
static inline void ct_remember_invariant(CFunc* f, ExStr msg) {
    ct_inv = f->ext;
    ct_have_inv = 1;
    ct_inv_msg = msg;
}

/* 1 when v < 0, else 0. The compare reads v's cell. rax stays free so a
 * later c_ret_value can place the flag word there. ct_check's lea also
 * uses rax, but that lea is in the caller, after this value is returned. */
static inline CValue ct_is_negative(CValue v) {
    CValue bad = c_local_imm("bad", C_PTR, "0");
    write_cmp(c_cell(v), literal("0", "zero"));
    const char* ge = c_new_label();
    write_jge(ge);
    c_assign_imm(&bad, "1");
    write_label(ge);
    return bad;
}

/* When ct is active and an invariant was remembered, call it on th.
 * A non-zero result stashes the invariant message and jumps to the trap. */
static inline void ct_call_invariant(CtCtx* ct, AsmDataField th) {
    if (!ct->active || !ct_have_inv) return;
    write_call(&ct_inv, th);
    /* test before the lea: lea does not change flags, and it reuses rax. */
    write_test("rax", "rax");
    ct_stash_msg(ct_inv_msg);
    write_jnz(ct->trap_label);
}

/* Close cpp member f. The invariant call runs here through ct, then c_ret
 * and cpp_method_end. */
static inline void ct_member_end(CFunc* f, CtCtx* ct) {
    CValue self = cpp_this();
    ct_call_invariant(ct, c_cell(self));
    c_ret(f);
    cpp_method_end();
}

/* Open a method inside the current class. `extra` follows the implicit
 * `this`. Posts recorded for the previous function are dropped first.
 * The mangled name is Class$method, from cpp_method_begin_params. */
static inline CFunc* ct_method_begin(const char* name, const CParam* extra, int nextra) {
    ct_posts_reset();
    return cpp_method_begin_params(name, extra, nextra);
}

/* Close method f through ct. Order: invariant, c_ret, cpp_method_end.
 * ct_method_end emits posts before this. */
static inline void ct_close_method(CFunc* f, CtCtx* ct) {
    CValue self = cpp_this();
    ct_call_invariant(ct, c_cell(self));
    c_ret(f);
    cpp_method_end();
}

/* Load the recorded field, compare the posts, run the invariant, and close. */
static inline void ct_method_end(CFunc* f, CtCtx* ct) {
    if (ct->active && ct_npost > 0 && ct_post_field) {
        CValue bal = cpp_this_field(ct_post_field, C_PTR);
        CValue now = c_dup(ct_post_field, C_PTR, bal);
        ct_emit_posts(ct, c_cell(now));
        c_free(&now);
    }
    ct_close_method(f, ct);
}

/* Close a free function. this_cell is the receiver the invariant sees.
 * Posts are the caller's job, before this. */
static inline void ct_close_function(CFunc* f, CtCtx* ct, AsmDataField this_cell) {
    if (ct->active)
        ct_call_invariant(ct, this_cell);
    c_ret(f);
    c_end_function(f);
}

