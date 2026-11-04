/*
 * ex12_quantisation.h - the quantisation library used by ex12_quantisation.c.
 *
 * An operation is an expression tree of inputs, literals, and add,
 * subtract, multiply, and divide. It always ends as a tuple of one or
 * more floats. The application decides what those floats mean. This
 * header stores the tree, fills the grid when asked, and emits the call.
 * It has no skill, no game, and no mean of its own.
 *
 * The application works in this order:
 *
 *   q_dimension        once per input, before the operation. The index,
 *                      the bucket width, and how many buckets.
 *   q_use_grid         optional. Call it before q_operation_end to emit
 *                      the lookup. Leave it out and the same tree is an
 *                      ordinary float function.
 *   q_operation_begin  names the plain function and opens the tree.
 *   q_input, q_lit,    one node each. q_out appends one float of the
 *   q_add, q_sub,      result tuple. The order of the q_out calls is the
 *   q_mul, q_div,      order of the tuple.
 *   q_out
 *   q_operation_end    closes the tree. With the grid on, this is where
 *                      the output array and the lookup are computed.
 *   q_value            one float global. The driver keeps the QValue.
 *   q_emit             constants or tables, then the plain procedure.
 *                      The grid build emits no procedure. Call it once
 *                      after q_operation_end, before q_invoke.
 *   q_invoke           runs the operation. inputs has one QValue per
 *                      axis. outputs has one QValue per float of the tuple.
 *
 * Grid off. q_emit writes one real4 for each literal in the tree
 * and a procedure of the name passed to q_operation_begin.
 * Input i arrives in xmm<i>. Each node is computed into a dword stack
 * slot. Float k of the tuple comes back in xmm<k>. q_invoke loads the
 * memory operands into those xmm registers, calls the procedure, and
 * stores the tuple back to the output operands. No grid symbol is emitted.
 *
 * Grid on. q_operation_end runs the tree once for every combination of
 * buckets and stores the tuples in a single array, q_update_out. A second
 * array, q_update_map, maps the row-major index of an input combination
 * to a slot of that array. The map in this library is the identity: slot
 * k holds the tuple for combination k. The map is still a separate table
 * so a later builder can point several input combinations at one stored
 * tuple. q_emit writes no procedure. q_invoke turns each input into
 * a bucket index, folds the indices into the row-major key, reads the
 * map, and loads the tuple from q_update_out. The symbols that contain
 * q_update exist only in this build. The example's disabled build checks
 * that those symbols are absent.
 *
 * An axis is uniform and starts at zero. Bucket i covers
 * [i * granularity, (i + 1) * granularity). The value used while the
 * table is built is the midpoint, (i + 0.5) * granularity. The library
 * checks that this midpoint quantises back to bucket i. At a call, an
 * input is scaled by 1/granularity, truncated toward zero, and clamped
 * into [0, count). A negative input is bucket 0. An input past the last
 * bucket is the last bucket.
 *
 * Sampling the domain at random and keeping the partition whose stored
 * outputs stay closest to the real operation would give a non-uniform
 * grid. The lookup is the table that choice would rewrite. Midpoints are
 * enough to show the idea.
 *
 * Limits, all checked while the generator runs: 64 nodes, 4 inputs,
 * 4 floats in a tuple, 16 buckets on one axis, 256 input combinations.
 */

#define Q_MAX_NODES   64
#define Q_MAX_DIMS    4
#define Q_MAX_OUT     4
#define Q_MAX_BUCKETS 16
#define Q_MAX_PAIRS   256

/* An index into q_nodes. The application threads these through the
 * arithmetic and hands the tuple members to q_out. */
typedef int QVal;

typedef enum {
    Q_INPUT = 0,
    Q_LIT   = 1,
    Q_ADD   = 2,
    Q_SUB   = 3,
    Q_MUL   = 4,
    Q_DIV   = 5
} QKind;

/* One node. a and b are child indices for the arithmetic. dim is the
 * input axis for Q_INPUT. lit is the constant for Q_LIT. The unused
 * fields stay -1 or 0. */
typedef struct {
    QKind  op;
    int    a, b;
    int    dim;
    double lit;
} QNode;

typedef struct {
    double granularity;
    int    count;
} QDim;

/* The handle q_invoke runs. name is the plain procedure's symbol.
 * n_in is how many axes were declared. n_out is the width of the tuple.
 * The application keeps this by value and passes its address. */
typedef struct {
    const char* name;
    int         n_in;
    int         n_out;
    int         ready;
} QOperation;

static QNode  q_nodes[Q_MAX_NODES];
static int    q_n = 0;
static QDim   q_dims[Q_MAX_DIMS];
static int    q_n_dim = 0;
static int    q_outs[Q_MAX_OUT];
static int    q_n_out = 0;
static int    q_grid = 0;
static int    q_building = 0;
static int    q_ready = 0;
static int    q_data_emitted = 0;
static int    q_code_emitted = 0;
static const char* q_name = NULL;

static double q_samples[Q_MAX_PAIRS][Q_MAX_OUT];
static int    q_map[Q_MAX_PAIRS];
static int    q_n_pairs = 0;

static const char q_out_name[] = "q_update_out";
static const char q_map_name[] = "q_update_map";

/* Format text into the generator arena. The host snprintf has no %f, so a
 * float goes through q_number first and arrives here as a decimal string.
 * Callers use the result as a symbol, an immediate, or an operand. The
 * buffer is 160 bytes; a longer expansion is a generation error. */
static inline const char* q_fmt(const char* fmt, ...) {
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    crash_if(n < 0 || n >= (int)sizeof buf, "quant: text too long");
    return c_strdup_arena(buf);
}

/* Decimal for a MASM real4 of v. Six fractional digits. The sign is included. */
static inline const char* q_number(double v) {
    int neg = 0;
    if (v < 0) { neg = 1; v = -v; }
    crash_if(v != v, "quant: cannot format a non-number");
    crash_if(v > 1.0e9, "quant: value is too large to emit");
    unsigned long long ip = (unsigned long long)v;
    double frac = v - (double)ip;
    unsigned long long fp = (unsigned long long)(frac * 1000000.0 + 0.5);
    if (fp >= 1000000ULL) { ip++; fp = 0; }
    char digits[8];
    for (int i = 5; i >= 0; i--) {
        digits[i] = (char)('0' + (int)(fp % 10));
        fp /= 10;
    }
    digits[6] = 0;
    if (neg) return q_fmt("-%llu.%s", ip, digits);
    return q_fmt("%llu.%s", ip, digits);
}

/* Select the lookup. Call it before q_operation_end. Until this runs
 * the operation is a plain float function, which is the zero-footprint
 * build. */
static inline void q_use_grid(void) {
    crash_if(q_ready, "quant: turn the grid on before the operation ends");
    q_grid = 1;
}

/*
 * One input axis. `dimension` is 0, then 1, and so on. `granularity` is
 * the bucket width. `range` is how many buckets. Bucket i covers
 * [i * granularity, (i + 1) * granularity). Call before q_operation_begin.
 */
static inline void q_dimension(int dimension, double granularity, int range) {
    crash_if(q_building || q_ready, "quant: dimensions before the operation");
    crash_if(dimension != q_n_dim, "quant: dimensions are numbered from zero");
    crash_if(q_n_dim >= Q_MAX_DIMS, "quant: too many dimensions");
    crash_if(!(granularity > 0.0), "quant: granularity must be positive");
    crash_if(range < 1 || range > Q_MAX_BUCKETS, "quant: range out of bounds");
    q_dims[q_n_dim].granularity = granularity;
    q_dims[q_n_dim].count = range;
    q_n_dim++;
}

/* Open the tree. name becomes the procedure symbol when the grid is
 * off, and it is unused in the emitted program when the grid is on.
 * Every q_dimension for this operation has already been called. */
static inline void q_operation_begin(const char* name) {
    crash_if(q_building || q_ready, "quant: an operation is already recorded");
    crash_if(name == NULL || name[0] == 0, "quant: the function needs a name");
    crash_if(q_n_dim < 1, "quant: declare a dimension first");
    q_building = 1;
    q_name = name;
    q_n = 0;
    q_n_out = 0;
}

/* Read input axis `dimension`. The same axis may be read more than once.
 * Every declared axis has to appear in at least one q_input, which
 * q_operation_end checks. */
static inline QVal q_input(int dimension) {
    crash_if(!q_building, "quant: input outside an operation");
    crash_if(dimension < 0 || dimension >= q_n_dim, "quant: input dimension");
    crash_if(q_n >= Q_MAX_NODES, "quant: expression is too large");
    int i = q_n++;
    q_nodes[i].op = Q_INPUT;
    q_nodes[i].a = q_nodes[i].b = -1;
    q_nodes[i].dim = dimension;
    q_nodes[i].lit = 0;
    return i;
}

/* A constant node for v. The number is formatted into the asm when the data
 * is emitted. The generator's snprintf has no %f, so q_number writes
 * the decimal by hand. */
static inline QVal q_lit(double v) {
    crash_if(!q_building, "quant: literal outside an operation");
    crash_if(q_n >= Q_MAX_NODES, "quant: expression is too large");
    int i = q_n++;
    q_nodes[i].op = Q_LIT;
    q_nodes[i].a = q_nodes[i].b = -1;
    q_nodes[i].dim = -1;
    q_nodes[i].lit = v;
    return i;
}

/* Append a binary node for op whose children are a and b, and return its index. */
static inline QVal q_bin(QKind op, QVal a, QVal b) {
    crash_if(!q_building, "quant: arithmetic outside an operation");
    crash_if(a < 0 || a >= q_n || b < 0 || b >= q_n, "quant: bad operand");
    crash_if(q_n >= Q_MAX_NODES, "quant: expression is too large");
    int i = q_n++;
    q_nodes[i].op = op;
    q_nodes[i].a = a;
    q_nodes[i].b = b;
    q_nodes[i].dim = -1;
    q_nodes[i].lit = 0;
    return i;
}

/* The four arithmetic nodes. Each appends one node whose children are
 * a and b, and returns the new node's index. */
static inline QVal q_add(QVal a, QVal b) { return q_bin(Q_ADD, a, b); }
/* Append a subtract node with children a and b and return its index. */
static inline QVal q_sub(QVal a, QVal b) { return q_bin(Q_SUB, a, b); }
/* Append a multiply node with children a and b and return its index. */
static inline QVal q_mul(QVal a, QVal b) { return q_bin(Q_MUL, a, b); }
/* Append a divide node with children a and b and return its index. */
static inline QVal q_div(QVal a, QVal b) { return q_bin(Q_DIV, a, b); }

/* Append one float v to the result tuple, left to right. The first q_out
 * is xmm0 (or the first real4 of a stored tuple). The second is xmm1,
 * and so on, up to Q_MAX_OUT. */
static inline void q_out(QVal v) {
    crash_if(!q_building, "quant: output outside an operation");
    crash_if(v < 0 || v >= q_n, "quant: bad output");
    crash_if(q_n_out >= Q_MAX_OUT, "quant: tuple is too wide");
    q_outs[q_n_out++] = v;
}

/* Run one node on a concrete input vector. This is the generator
 * evaluating the tree while it builds the table. The emitted program
 * does the same arithmetic only when the grid is off. */
static double q_eval(int node, const double* in) {
    crash_if(node < 0 || node >= q_n, "quant: bad expression node");
    QNode* nd = &q_nodes[node];
    if (nd->op == Q_INPUT) return in[nd->dim];
    if (nd->op == Q_LIT) return nd->lit;
    double x = q_eval(nd->a, in);
    double y = q_eval(nd->b, in);
    if (nd->op == Q_ADD) return x + y;
    if (nd->op == Q_SUB) return x - y;
    if (nd->op == Q_MUL) return x * y;
    crash_if(y == 0.0, "quant: division by zero on the grid");
    return x / y;
}

/* Representative of bucket on axis dim. The comment on q_fill is why a midpoint
 * is the sample, and what a non-uniform search would change instead. */
static double q_midpoint(int dim, int bucket) {
    return ((double)bucket + 0.5) * q_dims[dim].granularity;
}

/* Row-major index of one combination. Axis 0 is the slow index.
 * Two axes of sizes n0 and n1 give key = idx0 * n1 + idx1. q_invoke
 * folds the runtime indices with the same multiply-add. */
static int q_pair_key(const int* idx) {
    int key = idx[0];
    for (int d = 1; d < q_n_dim; d++)
        key = key * q_dims[d].count + idx[d];
    return key;
}

/*
 * The library is not fixed at two inputs and two outputs. Q_MAX_DIMS axes
 * and Q_MAX_OUT tuple floats are the caps, and every loop below walks
 * op->n_in and op->n_out. The application in ex12_quantisation.c happens
 * to use two and two. A different tree with one input, or a tuple of four
 * floats, uses the same emitters.
 *
 * The grid is not the faster shape for this toy. The plain function is a
 * straight-line run of scalar adds, subtracts, multiplies, and divides,
 * one per arithmetic node. The lookup, per call, is a sign test, a
 * multiply by 1/granularity, a truncate, a clamp, a row-major fold, a
 * load from the map, and a load of each tuple float. That is more work
 * than a handful of float operations. The grid pays off when one
 * evaluation of the real function is expensive enough that those integer
 * steps and two loads beat it.
 *
 * q_emit_data times three generator-side loops of 200000 evaluations and
 * writes the cycle counts into the data section as a comment. "formula"
 * is one pass over the nodes, the same order the plain function emits.
 * "lookup" is the index, the fold, the map, and the load. "tree" is the
 * recursive q_eval walk, which is how the generator fills the table and
 * is not what the processor runs. The formula loop is still a C loop, so
 * it is an upper bound on the straight-line code. One run of the three
 * loops counted formula 94295114, lookup 31474322, and tree 193467315
 * cycles. The lookup loop was the shorter of the first two, because the
 * formula loop walks the nodes in C. Those counts are also written above
 * q_emit_data.
 *
 * One output tuple per bucket combination, evaluated at the midpoint of
 * each bucket. The lookup entry for that combination is the tuple's slot.
 *
 * Drawing inputs at random and keeping the partition whose stored outputs
 * stay closest to the real operation would give a non-uniform grid. The
 * lookup table is the part that would change. Midpoints are enough to
 * show the idea.
 */
static void q_fill(int dim, double* in, int* idx, int* pair) {
    if (dim == q_n_dim) {
        int k = *pair;
        crash_if(q_pair_key(idx) != k, "quant: input pair index is not row-major");
        crash_if(k >= Q_MAX_PAIRS, "quant: too many bucket pairs");
        for (int o = 0; o < q_n_out; o++) {
            double v = q_eval(q_outs[o], in);
            crash_if(v != v, "quant: output is not a number");
            q_samples[k][o] = v;
        }
        q_map[k] = k;
        (*pair)++;
        return;
    }
    for (int b = 0; b < q_dims[dim].count; b++) {
        double mid = q_midpoint(dim, b);
        int back = (int)(mid / q_dims[dim].granularity);
        crash_if(back != b, "quant: midpoint left its bucket");
        in[dim] = mid;
        idx[dim] = b;
        q_fill(dim + 1, in, idx, pair);
    }
}

/* Every declared axis has to be read by at least one Q_INPUT. An axis
 * that never appears would still take an xmm register and a table
 * dimension, and the lookup would index a value the tree ignores. */
static void q_check_axes(void) {
    int used[Q_MAX_DIMS];
    for (int d = 0; d < q_n_dim; d++) used[d] = 0;
    for (int i = 0; i < q_n; i++)
        if (q_nodes[i].op == Q_INPUT) used[q_nodes[i].dim] = 1;
    for (int d = 0; d < q_n_dim; d++)
        crash_if(!used[d], "quant: dimension is not an input");
}

/* Size the table, then fill it. The product of the bucket counts has to
 * fit in Q_MAX_PAIRS before q_fill walks the axes. q_fill checks that
 * each combination's row-major key matches the slot it writes, and that
 * the midpoint of bucket i quantises back to i. The map written here is
 * the identity. q_map[k] is the slot whose tuple is combination k. */
static void q_build_grid(void) {
    int pairs = 1;
    for (int d = 0; d < q_n_dim; d++) {
        crash_if(pairs > Q_MAX_PAIRS / q_dims[d].count, "quant: too many bucket pairs");
        pairs *= q_dims[d].count;
    }
    double in[Q_MAX_DIMS];
    int idx[Q_MAX_DIMS];
    int pair = 0;
    q_fill(0, in, idx, &pair);
    crash_if(pair != pairs, "quant: grid size does not match the axes");
    for (int k = 0; k < pair; k++)
        crash_if(q_map[k] < 0 || q_map[k] >= pair, "quant: lookup left the output array");
    q_n_pairs = pair;
}

/* Close the tree and return the handle. With the grid on, the output
 * array and the lookup exist in generator memory after this returns.
 * q_emit_data is what writes them into the asm. */
static inline QOperation q_operation_end(void) {
    crash_if(!q_building, "quant: q_operation_end without begin");
    crash_if(q_n_out < 1, "quant: an operation ends in a tuple of floats");
    q_check_axes();
    q_building = 0;
    q_ready = 1;
    if (q_grid) q_build_grid();
    QOperation op;
    op.name = q_name;
    op.n_in = q_n_dim;
    op.n_out = q_n_out;
    op.ready = 1;
    return op;
}

/* Stack slot of one node in the plain function. Four bytes each, node 0
 * at [rsp], node i at [rsp+4*i]. q_emit_code subtracts a 16-byte-aligned
 * frame before any of these are used, and adds it back before the ret.
 * The grid path does not use these slots. */
static const char* q_slot(int node) {
    if (node == 0) return "dword ptr [rsp]";
    return q_fmt("dword ptr [rsp+%d]", node * 4);
}

/* Symbol of literal node `node` in the plain function's data. qk0, qk1,
 * and so on. Only literal nodes get a symbol. The grid path does not
 * emit these; it emits the filled table instead. */
static const char* q_lit_name(int node) {
    return q_fmt("qk%d", node);
}

/* One forward pass, children before parents. This is the work the plain
 * function emits: each node once, into a slot. The recursive q_eval is
 * only the generator's table filler. */
static double q_eval_linear(const double* in) {
    double slot[Q_MAX_NODES];
    for (int i = 0; i < q_n; i++) {
        QNode* nd = &q_nodes[i];
        if (nd->op == Q_INPUT) { slot[i] = in[nd->dim]; continue; }
        if (nd->op == Q_LIT) { slot[i] = nd->lit; continue; }
        double x = slot[nd->a];
        double y = slot[nd->b];
        if (nd->op == Q_ADD) slot[i] = x + y;
        else if (nd->op == Q_SUB) slot[i] = x - y;
        else if (nd->op == Q_MUL) slot[i] = x * y;
        else {
            crash_if(y == 0.0, "quant: division by zero");
            slot[i] = x / y;
        }
    }
    double s = 0;
    for (int o = 0; o < q_n_out; o++) s += slot[q_outs[o]];
    return s;
}

/* Compiler intrinsic. intrin.h is not included: it would pull in the
 * headers this repo declares itself. */
#pragma intrinsic(__rdtsc)
unsigned long long __rdtsc(void);

/* Keeps the timed loops live. The value is never emitted. */
static volatile double q_bench_sink;

/* 200000 evaluations of the one-pass formula, the lookup, and the
 * recursive walk. Fills the table in generator memory when the grid is
 * off, and does not emit that table. The comment it writes has no grid
 * symbol in it, so the disabled asm can carry the counts. */
static void q_emit_bench(void) {
    if (q_n_pairs == 0) q_build_grid();
    enum { N = 200000 };
    double sink = 0;
    unsigned long long t0 = __rdtsc();
    for (int i = 0; i < N; i++) {
        double in[Q_MAX_DIMS];
        for (int d = 0; d < q_n_dim; d++)
            in[d] = q_midpoint(d, (i + d) % q_dims[d].count);
        sink += q_eval_linear(in);
    }
    unsigned long long t1 = __rdtsc();
    unsigned long long t2 = __rdtsc();
    for (int i = 0; i < N; i++) {
        int idx[Q_MAX_DIMS];
        for (int d = 0; d < q_n_dim; d++) {
            double x = q_midpoint(d, (i + d) % q_dims[d].count);
            int b = (int)(x * (1.0 / q_dims[d].granularity));
            if (b < 0) b = 0;
            if (b >= q_dims[d].count) b = q_dims[d].count - 1;
            idx[d] = b;
        }
        int slot = q_map[q_pair_key(idx)];
        for (int o = 0; o < q_n_out; o++) sink += q_samples[slot][o];
    }
    unsigned long long t3 = __rdtsc();
    unsigned long long t4 = __rdtsc();
    for (int i = 0; i < N; i++) {
        double in[Q_MAX_DIMS];
        for (int d = 0; d < q_n_dim; d++)
            in[d] = q_midpoint(d, (i + d) % q_dims[d].count);
        for (int o = 0; o < q_n_out; o++) sink += q_eval(q_outs[o], in);
    }
    unsigned long long t5 = __rdtsc();
    q_bench_sink = sink;
    char note[160];
    int n = snprintf(note, sizeof note,
                     "! quant bench formula=%llu lookup=%llu tree=%llu !",
                     t1 - t0, t3 - t2, t5 - t4);
    crash_if(n < 0 || n >= (int)sizeof note, "quant: bench text too long");
    /* write() keeps the pointer and flushes it later, so the text has to
     * outlive this frame. */
    write_comment(asm_strdup(note));
}

/*
 * Body of q_emit, after q_operation_end. Opens .data.
 *
 * Grid off still emits data. movss needs a memory operand, so each
 * literal becomes one real4 (`qkN`). Emitting nothing here would leave
 * the plain function with nowhere to load its constants. What stays
 * absent is the grid: no output array, no index scratch, no reciprocal,
 * no map. q_emit_code is the function that emits no procedure when the
 * grid is on.
 *
 * Grid on: the output array is every tuple, one real4 per float,
 * combination 0 first. q_idx is scratch for the bucket indices of one
 * call. q_zero is the sign test. q_invN is 1/granularity for axis N.
 * The map is eight dwords to a line. Those symbols are how the disabled
 * build is checked, so this comment and the bench line must not name
 * them.
 *
 * The bench line is written on both builds. It is a MASM comment, not
 * code. One run counted formula 94295114, lookup 31474322, and tree
 * 193467315 cycles.
 */
static int q_scale_emitted = 0;
static int q_nvals = 0;

/* A float global the operation reads or writes. The name is the label. */
typedef struct { const char* name; } QValue;

/* One initialised real4. The driver holds the value and does not repeat
 * the label. Opens .data. */
static inline QValue q_value(double initial) {
    c_ensure_data();
    const char* name = q_fmt("qv%d", q_nvals);
    q_nvals++;
    c_real4(name, q_number(initial));
    QValue v;
    v.name = name;
    return v;
}

/* scale100 and half, the printer's constants. Call from the data section. */
static inline void q_emit_scale(void) {
    if (q_scale_emitted) return;
    q_scale_emitted = 1;
    c_ensure_data();
    write_real4_def("scale100", "100.0");
    write_real4_def("half", "0.5");
}

static inline void q_emit_data(QOperation* op) {
    crash_if(op == NULL || !op->ready, "quant: emit before q_operation_end");
    crash_if(q_data_emitted, "quant: data already emitted");
    q_data_emitted = 1;
    c_ensure_data();
    q_emit_scale();
    q_emit_bench();
    if (!q_grid) {
        for (int i = 0; i < q_n; i++) {
            if (q_nodes[i].op != Q_LIT) continue;
            write_real4_def(q_lit_name(i), q_number(q_nodes[i].lit));
        }
        return;
    }
    int first = 1;
    for (int k = 0; k < q_n_pairs; k++) {
        for (int o = 0; o < op->n_out; o++) {
            const char* dec = q_number(q_samples[k][o]);
            if (first) { write_real4_def(q_out_name, dec); first = 0; }
            else write_real4(dec);
        }
    }
    char idxinit[64];
    int p = 0;
    idxinit[0] = 0;
    for (int d = 0; d < op->n_in; d++) {
        p += snprintf(idxinit + p, sizeof idxinit - (size_t)p, "%s0", d ? ", " : "");
        crash_if(p < 0 || p >= (int)sizeof idxinit, "quant: index text too long");
    }
    write_dd("q_idx", c_strdup_arena(idxinit));
    write_real4_def("q_zero", "0.0");
    for (int d = 0; d < op->n_in; d++) {
        double inv = 1.0 / q_dims[d].granularity;
        write_real4_def(q_fmt("q_inv%d", d), q_number(inv));
    }
    first = 1;
    int k = 0;
    while (k < q_n_pairs) {
        char line[160];
        int n = 0;
        line[0] = 0;
        int col = 0;
        while (k < q_n_pairs && col < 8) {
            n += snprintf(line + n, sizeof line - (size_t)n, "%s%d", col ? ", " : "", q_map[k]);
            crash_if(n < 0 || n >= (int)sizeof line, "quant: lookup text too long");
            k++;
            col++;
        }
        const char* text = c_strdup_arena(line);
        if (first) { write_dd(q_map_name, text); first = 0; }
        else write_dd("", text);
    }
}

/* One node of the plain function, other than an input. A literal is
 * loaded from its real4 and stored to the node's slot. An arithmetic
 * node loads child a into xmm0 (tagged "term"), applies child b, and
 * stores xmm0 back. Child b is a memory operand, which addss/subss/
 * mulss/divss accept, so it is not copied into a second xmm. The slot
 * store is a memory destination and is not entered in the type
 * dictionary. Inputs are placed by q_emit_code before this runs. */
static void q_emit_node(int node) {
    QNode* nd = &q_nodes[node];
    if (nd->op == Q_LIT) {
        write_movss_tag("xmm0", q_lit_name(node), "lit");
        write_movss(q_slot(node), "xmm0");
        return;
    }
    write_movss_tag("xmm0", q_slot(nd->a), "term");
    const char* src = q_slot(nd->b);
    if (nd->op == Q_ADD) write_addss("xmm0", src);
    else if (nd->op == Q_SUB) write_subss("xmm0", src);
    else if (nd->op == Q_MUL) write_mulss("xmm0", src);
    else if (nd->op == Q_DIV) write_divss("xmm0", src);
    else crash_if(1, "quant: not an arithmetic node");
    write_movss(q_slot(node), "xmm0");
}

/*
 * Body of q_emit, after the data and before q_invoke. write_proc opens .code.
 *
 * Grid off: the procedure named by q_operation_begin. The frame is one
 * dword per node, rounded up to 16. Input d arrives in xmm<d>, tagged
 * in<d>, and is stored to that input's slot. Every other node is then
 * evaluated in index order, which is dependency order because a child
 * is allocated before its parent. Tuple float o is loaded into xmm<o>,
 * tagged out<o>. The frame is released before the ret. The procedure
 * does not call, so the frame is only the slots.
 *
 * Grid on: nothing. q_invoke emits the lookup inline.
 */
static inline void q_emit_code(QOperation* op) {
    crash_if(op == NULL || !op->ready, "quant: emit before q_operation_end");
    crash_if(!q_data_emitted, "quant: emit data before code");
    crash_if(q_code_emitted, "quant: code already emitted");
    q_code_emitted = 1;
    if (q_grid) return;
    int bytes = (q_n * 4 + 15) & ~15;
    const char* nbytes = q_fmt("%d", bytes);
    AsmExtern proc = write_proc(op->name, NULL, 0, asm_void);
        write_sub((AsmDataField){ "rsp", "sp", NULL },
                  (AsmDataField){ nbytes, "bytes", NULL });
        asm_note_rsp_sub(bytes);
        for (int i = 0; i < q_n; i++) {
            if (q_nodes[i].op != Q_INPUT) continue;
            const char* xm = q_fmt("xmm%d", q_nodes[i].dim);
            asm_type_dict_set(xm, q_fmt("in%d", q_nodes[i].dim));
            write_movss(q_slot(i), xm);
        }
        for (int i = 0; i < q_n; i++) {
            if (q_nodes[i].op == Q_INPUT) continue;
            q_emit_node(i);
        }
        for (int o = 0; o < op->n_out; o++)
            write_movss_tag(q_fmt("xmm%d", o), q_slot(q_outs[o]), q_fmt("out%d", o));
        write_add((AsmDataField){ "rsp", "sp", NULL },
                  (AsmDataField){ nbytes, "bytes", NULL });
        asm_note_rsp_add(bytes);
        write_ret();
    write_endp(&proc);
}

/* Data and, when the grid is off, the plain procedure. One call. */
static inline void q_emit(QOperation* op) {
    q_emit_data(op);
    q_emit_code(op);
}

/*
 * Bucket index of one input, left in eax, tagged "idx".
 *
 * xmm0 is tagged "axis" while it holds the input. comiss against q_zero
 * and jb catches a negative input and a NaN (both set CF). Otherwise the
 * input is multiplied by q_inv<dim> (1/granularity, six decimal digits)
 * and cvttss2si truncates toward zero. An index greater than or equal to
 * the bucket count becomes the last bucket.
 *
 * cvttss2si's overflow result is the signed integer 0x80000000. That
 * value is negative, so the signed jl against the count does not clamp
 * it. The demo's inputs are small finite means, inside the range where
 * the convert does not overflow.
 */
static void q_emit_index(int dim, const char* mem) {
    const char* neg = c_new_label();
    const char* done = c_new_label();
    write_movss_tag("xmm0", mem, "axis");
    write_comiss("xmm0", "q_zero");
    write_jb(neg);
    write_mulss("xmm0", q_fmt("q_inv%d", dim));
    write_cvttss2si("eax", "xmm0");
    asm_type_dict_set("eax", "idx");
    write_cmp((AsmDataField){"eax", "idx", NULL},
              literal(q_fmt("%d", q_dims[dim].count), "range"));
    write_jl(done);
    write_replace_mov("eax", (AsmDataField){ q_fmt("%d", q_dims[dim].count - 1), "idx", NULL });
    write_jmp(done);
    write_label(neg);
    write_replace_mov("eax", (AsmDataField){ "0", "idx", NULL });
    write_label(done);
}

/*
 * Run the operation. `inputs` has n_in float globals. `outputs` has n_out,
 * in tuple order. Clobbers xmm0 and, on the grid path, eax and r8. Call
 * it when no live C value still needs those registers.
 *
 * Grid off. Input d is loaded into xmm<d> and tagged in<d>. The plain
 * procedure is called by name, with no argument marshalling: the xmm
 * registers are the arguments. Tuple float o comes back in xmm<o> and is
 * stored to output o. The store is a memory destination, so the tag is
 * not entered for that operand text.
 *
 * Grid on. There is no call. q_emit_index writes each bucket into q_idx.
 * The key starts as idx0. Each later axis does key = key * count[d] + idx[d],
 * the same row-major order q_pair_key used when the table was built. The
 * key loads one dword from the map. That slot times the tuple width in
 * bytes is the address of the tuple. Each float is loaded into xmm0,
 * tagged out<o>, and stored to the matching output.
 */
static inline void q_invoke(QOperation* op, const QValue* inputs, const QValue* outputs) {
    crash_if(op == NULL || !op->ready, "quant: invoke before q_operation_end");
    crash_if(!q_code_emitted, "quant: emit code before invoke");
    crash_if(inputs == NULL || outputs == NULL, "quant: invoke needs operands");
    if (!q_grid) {
        for (int d = 0; d < op->n_in; d++)
            write_movss_tag(q_fmt("xmm%d", d), inputs[d].name, q_fmt("in%d", d));
        write_call_symbol(op->name);
        for (int o = 0; o < op->n_out; o++)
            write_movss(outputs[o].name, q_fmt("xmm%d", o));
        return;
    }
    for (int d = 0; d < op->n_in; d++) {
        q_emit_index(d, inputs[d].name);
        write_replace_mov(asm_mem("dword", "q_idx", d * 4), (AsmDataField){ "eax", "idx", NULL });
    }
    write_replace_mov("eax", (AsmDataField){ asm_mem("dword", "q_idx", 0), "idx", NULL });
    for (int d = 1; d < op->n_in; d++) {
        write_imul2("eax", q_fmt("%d", q_dims[d].count));
        write_add((AsmDataField){ "eax", "idx", NULL },
                  (AsmDataField){ asm_mem("dword", "q_idx", d * 4), "idx", NULL });
    }
    write_lea("r8", q_map_name);
    write_replace_mov("eax", (AsmDataField){ asm_mem_index("dword", "r8", "rax", 4, 0), "slot", NULL });
    write_imul2("eax", q_fmt("%d", op->n_out * 4));
    write_lea("r8", q_out_name);
    for (int o = 0; o < op->n_out; o++) {
        write_movss_tag("xmm0", asm_mem_index("dword", "r8", "rax", 1, o * 4),
                        q_fmt("out%d", o));
        write_movss(outputs[o].name, "xmm0");
    }
}

/* Print slot as an integer hundredth. slot is the float global. printer
 * is the procedure that prints the integer; the scaled value is already
 * that procedure's argument, so the call is by name. The demo's values
 * are positive, so adding one half before the conversion rounds to
 * nearest. A variance of 0.8 prints as 80. scale100 and half are 4-byte
 * floats in the data section. The float arithmetic stays here: c_funcs.h
 * has no float operations. */
static inline void q_print_x100(QValue slot, AsmExtern* printer) {
    if (!q_scale_emitted && asm_proc_depth == 0) q_emit_scale();
    write_movss_tag("xmm0", slot.name, "scaled");
    write_mulss("xmm0", "scale100");
    write_addss("xmm0", "half");
    write_cvttss2si("rcx", "xmm0");
    asm_type_dict_set("rcx", "value");
    write_call_symbol(printer->name);
}
