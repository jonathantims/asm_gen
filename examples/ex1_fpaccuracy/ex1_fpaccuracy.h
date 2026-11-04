/*
 * ex1_fpaccuracy.h - generation-time floating-point checks.
 *
 * A value is a named memory slot, not a pinned xmm accumulator. Each operation
 * loads the slot into an xmm scratch, computes, and stores it back, so the
 * next operation is free to use any slot and the xmm registers are not live
 * across the operation. c_funcs.h allocates general-purpose registers; it does
 * not allocate xmm. Keeping the values in memory is the spill.
 *
 * Three modes, chosen before any code is emitted:
 *   FP_PLAIN   the single-precision operation only. Zero footprint.
 *   FP_SHADOW  the same operation mirrored on a double-precision slot.
 *   FP_TREE    an expression tree in generator memory. fp_eval walks it at the
 *              precision selected with fp_set_precision (53-bit double in this
 *              library). The walk's result is the oracle the check compares
 *              against. A wider evaluator would walk the same nodes.
 *
 * The trap label is fp_trap and is emitted only when the mode is not plain.
 */


typedef enum { FP_PLAIN = 0, FP_SHADOW = 1, FP_TREE = 2 } FpMode;
typedef enum { FP_LIT = 0, FP_ADD = 1, FP_MUL = 2, FP_DIV = 3, FP_REP = 4 } FpOp;

static FpMode fp_mode = FP_PLAIN;
static int    fp_prec_bits = 53;

/* Select floating-point mode m before emission. m is FP_PLAIN, FP_SHADOW, or FP_TREE. */
static inline void fp_set_mode(FpMode m) { fp_mode = m; }
/* Return 1 when the selected mode will emit a shadow or tree check. */
static inline int  fp_active(void) { return fp_mode != FP_PLAIN; }
/* Record tree-walk precision bits. bits above 53 stops the generator. */
static inline void fp_set_precision(int bits) {
    crash_if(bits > 53, "fp: this evaluator is double; the tree is ready for a wider one");
    fp_prec_bits = bits;
}

typedef struct {
    const char* real;
    const char* shadow;
    int         node;
} FpVal;

/* Name the single-precision slot real and the double-precision slot shadow.
 * The expression-tree node starts unset. */
static inline FpVal fp_val(const char* real, const char* shadow) {
    FpVal v;
    v.real = real;
    v.shadow = shadow;
    v.node = -1;
    return v;
}

#define FP_MAX_NODES 32
typedef struct { int op, a, b, reps; double lit; } FpNode;
static FpNode fp_nodes[FP_MAX_NODES];
static int    fp_nnodes = 0;

/* Append a literal node holding v and return its index in the tree. */
static inline int fp_lit(double v) {
    crash_if(fp_nnodes >= FP_MAX_NODES, "fp: expression tree is full");
    int i = fp_nnodes++;
    fp_nodes[i].op = FP_LIT;
    fp_nodes[i].lit = v;
    fp_nodes[i].a = fp_nodes[i].b = -1;
    fp_nodes[i].reps = 0;
    return i;
}
/* Append a binary node: operator op on child nodes a and b. Returns its index. */
static inline int fp_bin(int op, int a, int b) {
    crash_if(fp_nnodes >= FP_MAX_NODES, "fp: expression tree is full");
    int i = fp_nnodes++;
    fp_nodes[i].op = op;
    fp_nodes[i].a = a;
    fp_nodes[i].b = b;
    fp_nodes[i].reps = 0;
    fp_nodes[i].lit = 0;
    return i;
}
/* Append a repeated-add node that adds step to acc, count times. Returns its index. */
static inline int fp_rep(int acc, int step, int count) {
    crash_if(fp_nnodes >= FP_MAX_NODES, "fp: expression tree is full");
    int i = fp_nnodes++;
    fp_nodes[i].op = FP_REP;
    fp_nodes[i].a = acc;
    fp_nodes[i].b = step;
    fp_nodes[i].reps = count;
    fp_nodes[i].lit = 0;
    return i;
}

/* Evaluate expression node n in double precision and return that value. */
static double fp_eval(int n) {
    crash_if(n < 0 || n >= fp_nnodes, "fp: bad expression node");
    (void)fp_prec_bits;
    FpNode* nd = &fp_nodes[n];
    if (nd->op == FP_LIT) return nd->lit;
    if (nd->op == FP_ADD) return fp_eval(nd->a) + fp_eval(nd->b);
    if (nd->op == FP_MUL) return fp_eval(nd->a) * fp_eval(nd->b);
    if (nd->op == FP_DIV) return fp_eval(nd->a) / fp_eval(nd->b);
    double v = fp_eval(nd->a);
    double s = fp_eval(nd->b);
    for (int i = 0; i < nd->reps; i++) v += s;
    return v;
}

typedef struct {
    ExConsole*  con;
    AsmExtern*  print;
    const char* trap_label;
    const char* real_slot;
    bool        active;
} FpCtx;

/* Emit value v as a zero real4. In shadow mode, also emit the shadow slot
 * of v as a zero real8. */
static inline void fp_emit_var(FpVal* v) {
    write_real4_def(v->real, "0.0");
    if (fp_mode == FP_SHADOW) write_real8_def(v->shadow, "0.0");
}
/* Emit the real slot of v from decimal. In shadow mode, also emit the shadow
 * slot of v from the same decimal. */
static inline void fp_emit_const(FpVal* v, const char* decimal) {
    write_real4_def(v->real, decimal);
    if (fp_mode == FP_SHADOW) write_real8_def(v->shadow, decimal);
}

/* Constants the check needs. Call in write_data(), only useful when active. */
static inline void fp_emit_check_data(void) {
    if (!fp_active()) return;
    c_ensure_data();
    write_dq("fp_absmask", "7FFFFFFFFFFFFFFFh");
    write_real8_def("fp_scale", "1000000.0");
}
/* Emit the uninitialised fp_errval qword when a check is active. */
static inline void fp_emit_scratch(void) {
    if (!fp_active()) return;
    c_ensure_bss();
    write_unitialised_data("fp_errval", "?");
}

/* Build the check context. real names the slot a later compare reads.
 * When active, emit fp_trap: realign the stack, print msg with print, and
 * exit 1 on con. */
static inline FpCtx fp_begin(ExConsole* con, AsmExtern* print, ExStr msg, FpVal* real) {
    FpCtx c;
    c.con = con;
    c.print = print;
    c.trap_label = "fp_trap";
    c.real_slot = real->real;
    c.active = fp_active();
    if (c.active) {
        write_jmp("fp_after_trap");
        write_label("fp_trap");
            write_and("rsp", "-16");
            write_sub((AsmDataField){ "rsp", "sp", NULL },
                      (AsmDataField){ "8", "pad", NULL });
            ex_puts(print, msg);
            ex_exit(con, 1);
        write_label("fp_after_trap");
    }
    return c;
}

/* dst = dst <op> src, single precision. Shadow mode mirrors it in double.
 * ss/sd select the scalar operation. The memory forms are the real4 and
 * real8 symbols; xmm0 and xmm1 carry the running value. */
static inline void fp_emit_binop(const char* ss, const char* sd, FpVal* dst, FpVal* src) {
    write_movss("xmm0", dst->real);
    if (strcmp(ss, "addss") == 0) write_addss("xmm0", src->real);
    else if (strcmp(ss, "mulss") == 0) write_mulss("xmm0", src->real);
    else write_divss("xmm0", src->real);
    write_movss(dst->real, "xmm0");
    if (fp_mode == FP_SHADOW) {
        write_movsd_sse("xmm1", dst->shadow);
        if (strcmp(sd, "addsd") == 0) write_addsd("xmm1", src->shadow);
        else if (strcmp(sd, "mulsd") == 0) write_mulsd("xmm1", src->shadow);
        else write_divsd("xmm1", src->shadow);
        write_movsd_sse(dst->shadow, "xmm1");
    }
}
/* Add src into dst in single precision. Shadow mode mirrors the add in double. */
static inline void fp_emit_add(FpVal* dst, FpVal* src) { fp_emit_binop("addss", "addsd", dst, src); }
/* Multiply dst by src in single precision. Shadow mode mirrors the product in double. */
static inline void fp_emit_mul(FpVal* dst, FpVal* src) { fp_emit_binop("mulss", "mulsd", dst, src); }
/* Divide dst by src in single precision. Shadow mode mirrors the quotient in double. */
static inline void fp_emit_div(FpVal* dst, FpVal* src) { fp_emit_binop("divss", "divsd", dst, src); }

/* Runtime loop: dst += src, `count` times. r8 is only the trip count. The
 * tree node for this loop is fp_rep, recorded by the generator, not here: the
 * generator emits the loop once, and the processor runs it `count` times. */
static inline void fp_emit_repeat_add(FpVal* dst, FpVal* src, int count) {
    char nbuf[32];
    snprintf(nbuf, sizeof nbuf, "%d", count);
    const char* nlit = c_strdup_arena(nbuf);
    write_replace_mov("r8", (AsmDataField){ "0", "i", NULL });
    write_label("fp_loop");
        write_cmp((AsmDataField){"r8","i",NULL}, literal(nlit, "n"));
        write_jge("fp_done");
        fp_emit_add(dst, src);
        write_inc("r8");
        write_jmp("fp_loop");
    write_label("fp_done");
}

/*
 * When active, |oracle - real| * 1e6 is printed and the trap fires if that
 * integer exceeds 1000 (an absolute error above 0.001). `oracle` is the
 * shadow slot (FP_SHADOW) or the baked tree result (FP_TREE). The error is
 * pulled into fp_errval before the print, because print clobbers xmm.
 */
static inline void fp_check(FpCtx* c, AsmExtern* pu64, ExStr label, ExStr nl,
                            FpVal* got) {
    if (!c->active) return;
    const char* oracle = (fp_mode == FP_TREE) ? "fp_ref" : got->shadow;
    write_movss("xmm0", c->real_slot);
    write_cvtss2sd("xmm5", "xmm0");
    write_movsd_sse("xmm4", oracle);
    write_subsd("xmm4", "xmm5");
    write_movsd_sse("xmm6", "fp_absmask");
    write_andpd("xmm4", "xmm6");
    write_mulsd("xmm4", "fp_scale");
    write_cvttsd2si("rax", "xmm4");
    write_set_mov((AsmDataField){ "fp_errval", "?", NULL }, (AsmDataField){ "rax", "err", NULL });
    ex_puts(c->print, label);
    write_replace_mov("rcx", (AsmDataField){ "fp_errval", "value", NULL });
    write_call_symbol(pu64->name);
    ex_puts(c->print, nl);
    write_cmp((AsmDataField){"fp_errval","err",NULL}, literal("1000", "thresh"));
    write_ja(c->trap_label);
}

/* Twelve fractional digits. Host snprintf has no %g. */
static inline const char* fp_decimal(double v) {
    int neg = 0;
    if (v < 0) { neg = 1; v = -v; }
    unsigned long long ip = (unsigned long long)v;
    double frac = v - (double)ip;
    unsigned long long scale = 1000000000000ULL;
    unsigned long long fp = (unsigned long long)(frac * (double)scale + 0.5);
    if (fp >= scale) { ip++; fp = 0; }
    char digits[16];
    for (int i = 11; i >= 0; i--) {
        digits[i] = (char)('0' + (int)(fp % 10));
        fp /= 10;
    }
    digits[12] = 0;
    char ref[80];
    if (neg) snprintf(ref, sizeof ref, "-%llu.%s", ip, digits);
    else     snprintf(ref, sizeof ref, "%llu.%s", ip, digits);
    return c_strdup_arena(ref);
}

static inline const char* fp_shadow_name(const char* real) {
    char buf[80];
    snprintf(buf, sizeof buf, "%s8", real);
    return c_strdup_arena(buf);
}

/* A named slot. The shadow symbol is the real name plus "8". */
static inline FpVal fp_slot(const char* name) {
    FpVal v;
    v.real = name;
    v.shadow = fp_shadow_name(name);
    v.node = -1;
    return v;
}

/* The accumulator starts at zero. Tree mode records that literal. */
static inline FpVal fp_acc(const char* name) {
    FpVal v = fp_slot(name);
    if (fp_mode == FP_TREE) v.node = fp_lit(0.0);
    return v;
}

static const char* fp_dec_name[8];
static const char* fp_dec_text[8];
static int fp_ndec = 0;

/* A constant slot. decimal is the assembler text. lit is the tree value. */
static inline FpVal fp_const(const char* name, const char* decimal, double lit) {
    FpVal v = fp_slot(name);
    if (fp_mode == FP_TREE) v.node = fp_lit(lit);
    crash_if(fp_ndec >= 8, "fp: too many constants");
    fp_dec_name[fp_ndec] = name;
    fp_dec_text[fp_ndec] = decimal;
    fp_ndec++;
    return v;
}

typedef struct {
    int kind; /* 0 repeat-add, 1 div, 2 mul */
    FpVal* dst;
    FpVal* src;
    int count;
} FpStep;
static FpStep fp_steps[8];
static int fp_nsteps = 0;
static FpVal* fp_tracked[8];
static int fp_ntracked = 0;
static FpVal* fp_result = NULL;

static inline void fp_seen(FpVal* v) {
    for (int i = 0; i < fp_ntracked; i++)
        if (fp_tracked[i] == v) return;
    crash_if(fp_ntracked >= 8, "fp: too many values");
    fp_tracked[fp_ntracked++] = v;
}

static inline void fp_repeat(FpVal* dst, FpVal* src, int count) {
    crash_if(fp_nsteps >= 8, "fp: too many steps");
    fp_steps[fp_nsteps].kind = 0;
    fp_steps[fp_nsteps].dst = dst;
    fp_steps[fp_nsteps].src = src;
    fp_steps[fp_nsteps].count = count;
    fp_nsteps++;
    fp_seen(dst);
    fp_seen(src);
    if (fp_mode == FP_TREE) dst->node = fp_rep(dst->node, src->node, count);
    fp_result = dst;
}

static inline void fp_divide(FpVal* dst, FpVal* src) {
    crash_if(fp_nsteps >= 8, "fp: too many steps");
    fp_steps[fp_nsteps].kind = 1;
    fp_steps[fp_nsteps].dst = dst;
    fp_steps[fp_nsteps].src = src;
    fp_steps[fp_nsteps].count = 0;
    fp_nsteps++;
    fp_seen(dst);
    fp_seen(src);
    if (fp_mode == FP_TREE) dst->node = fp_bin(FP_DIV, dst->node, src->node);
    fp_result = dst;
}

static inline void fp_multiply(FpVal* dst, FpVal* src) {
    crash_if(fp_nsteps >= 8, "fp: too many steps");
    fp_steps[fp_nsteps].kind = 2;
    fp_steps[fp_nsteps].dst = dst;
    fp_steps[fp_nsteps].src = src;
    fp_steps[fp_nsteps].count = 0;
    fp_nsteps++;
    fp_seen(dst);
    fp_seen(src);
    if (fp_mode == FP_TREE) dst->node = fp_bin(FP_MUL, dst->node, src->node);
    fp_result = dst;
}

/* Emit every tracked slot, the check constants, and the tree oracle. */
static inline void fp_emit_recorded(void) {
    c_ensure_data();
    for (int i = 0; i < fp_ntracked; i++) {
        FpVal* v = fp_tracked[i];
        const char* dec = NULL;
        for (int k = 0; k < fp_ndec; k++)
            if (strcmp(fp_dec_name[k], v->real) == 0) dec = fp_dec_text[k];
        if (dec) fp_emit_const(v, dec);
        else     fp_emit_var(v);
    }
    fp_emit_check_data();
    if (fp_mode == FP_TREE && fp_result)
        write_real8_def("fp_ref", fp_decimal(fp_eval(fp_result->node)));
}

/* Emit the recorded float program. Shadow mode mirrors it. Tree mode
 * already recorded the nodes; this is the single-precision run. */
static inline void fp_emit_program(void) {
    for (int i = 0; i < fp_nsteps; i++) {
        if (fp_steps[i].kind == 0)
            fp_emit_repeat_add(fp_steps[i].dst, fp_steps[i].src, fp_steps[i].count);
        else if (fp_steps[i].kind == 1)
            fp_emit_div(fp_steps[i].dst, fp_steps[i].src);
        else
            fp_emit_mul(fp_steps[i].dst, fp_steps[i].src);
    }
}
