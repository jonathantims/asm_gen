/*
 * ex5_layout.h - one class-hierarchy description, two generated representations.
 *
 * THE IDEA
 * --------
 * A generation-time flag selects the representation. The generator describes
 * the hierarchy once, through field handles and object handles, and does not
 * branch on the flag itself:
 *
 *   - OBJECT HIERARCHY (array of structures): each object is a contiguous
 *     block of its single fields. Object i is at base + i*stride.
 *   - PARALLEL ARRAYS (structure of arrays): one array per field.
 *
 * A field is either single or multiple, and either required or optional.
 *   - single + required: one value on every object. Parallel form is dense.
 *   - single + optional: a negative value means "absent". Parallel form omits
 *     that value and keeps an index array of the parent rows that have one.
 *     Random access bisects the index. Walking the stored values does not.
 *   - multiple: add_dobject_field appends a value. The values are a packed
 *     list plus the parent row of each one, in both representations, because
 *     a fixed object slot cannot hold a varying count. Object-mode summation
 *     crawls each object and scans that list. Parallel-mode summation walks
 *     the packed values only.
 *
 * Other operations (a filter, a join, a grouped reduction) are the same walk
 * with a different body. The sum and the indexed load are the proof of concept.
 *
 * All addressing is RIP-relative (`lea rdx, <label>` then `[rdx+index*8]`).
 * Counts are known while generating, so the loop bounds are immediates.
 */


/* false => object hierarchy (AoS); true => parallel arrays (SoA). */
static bool rep_parallel = false;
/* Select this example's parallel-array representation before emission. Takes no parameters. */
static inline void rep_use_parallel(void)  { rep_parallel = true;  }
/* Select this example's object-hierarchy representation before emission. Takes no parameters. */
static inline void rep_use_hierarchy(void) { rep_parallel = false; }

#define REP_MAX_FIELDS 8
#define REP_MAX_ROWS   16
#define REP_MAX_MULTI  32
#define REP_MAX_CHILDREN 4

typedef struct RepClass RepClass;

/* A handle returned by rep_field. It names one field of one class. */
typedef struct RepField {
    RepClass*   owner;
    const char* name;
    int         index;         /* index in the class field list */
    int         single_index;  /* slot among single fields, or -1 when multiple */
    int         optional;      /* negative value means absent */
    int         multiple;      /* writers use add_dobject_field */
} RepField;

/* One object. `row` is its index in the class. */
typedef struct RepObject {
    RepClass* cls;
    int       row;
} RepObject;

struct RepClass {
    const char* type_name;
    RepField    fields[REP_MAX_FIELDS];
    int         nfields;
    int         nsingle;
    long long   rows[REP_MAX_ROWS][REP_MAX_FIELDS];
    int         nrows;
    long long   multi_vals[REP_MAX_FIELDS][REP_MAX_MULTI];
    int         multi_row[REP_MAX_FIELDS][REP_MAX_MULTI];
    int         multi_n[REP_MAX_FIELDS];
    RepClass*   children[REP_MAX_CHILDREN];
    int         child_optional[REP_MAX_CHILDREN];
    int         child_multiple[REP_MAX_CHILDREN];
    int         child_link[REP_MAX_CHILDREN][REP_MAX_ROWS]; /* parent row -> child row, or -1 */
    int         child_bound[REP_MAX_CHILDREN];
    int         nchildren;
};

/* Format signed integer v as a decimal token stored in the arena. */
static inline const char* rep_num(long long v) {
    char buf[32];
    snprintf(buf, sizeof buf, "%lld", v);
    return c_strdup_arena(buf);
}

/* Join a and b with an underscore into one arena token. */
static inline const char* rep_join(const char* a, const char* b) {
    char buf[96];
    snprintf(buf, sizeof buf, "%s_%s", a, b);
    return c_strdup_arena(buf);
}

/* Start an empty class record named type_name. */
static inline RepClass rep_begin(const char* type_name) {
    RepClass c;
    memset(&c, 0, sizeof c);
    c.type_name = type_name;
    return c;
}

/* Declare field name on class c. optional: a negative value is absent.
 * multiple: writers use add_dobject_field. The returned pointer addresses
 * the field inside c. */
static inline RepField* rep_field(RepClass* c, const char* name,
                                  int optional, int multiple) {
    crash_if(c->nfields >= REP_MAX_FIELDS, "rep_field: too many fields");
    RepField* f = &c->fields[c->nfields];
    memset(f, 0, sizeof *f);
    f->owner = c;
    f->name = name;
    f->index = c->nfields;
    f->optional = optional;
    f->multiple = multiple;
    if (multiple) f->single_index = -1;
    else          f->single_index = c->nsingle++;
    c->nfields++;
    return f;
}

/* A child class, aligned with the parent by row. optional: a negative value
 * in the child means that parent has no child. multiple is recorded so the
 * description can say so; this proof of concept stores a varying count as a
 * multiple field (packed values + parent index), which is the same shape a
 * multiple child would use. */
static inline void rep_child(RepClass* parent, RepClass* child,
                             int optional, int multiple) {
    crash_if(parent->nchildren >= REP_MAX_CHILDREN, "rep: too many children");
    parent->children[parent->nchildren] = child;
    parent->child_optional[parent->nchildren] = optional;
    parent->child_multiple[parent->nchildren] = multiple;
    parent->child_bound[parent->nchildren] = 0;
    for (int r = 0; r < REP_MAX_ROWS; r++)
        parent->child_link[parent->nchildren][r] = -1;
    parent->nchildren++;
}

/* Attach child to parent. Emit reorders child rows into parent order and
 * synthesizes an absent row for an optional child that was not attached. */
static inline void set_child(RepObject parent, RepObject child) {
    int slot = -1;
    for (int i = 0; i < parent.cls->nchildren; i++)
        if (parent.cls->children[i] == child.cls) slot = i;
    crash_if(slot < 0, "set_child: object is not a child of this parent");
    crash_if(parent.cls->child_link[slot][parent.row] >= 0, "set_child: parent already has this child");
    parent.cls->child_link[slot][parent.row] = child.row;
    parent.cls->child_bound[slot] = 1;
}

/* A fresh object. Optional single fields start absent (-1). Required single
 * fields start at 0 until set_dobject_field writes them. */
static inline RepObject new_dobject(RepClass* c) {
    crash_if(c->nrows >= REP_MAX_ROWS, "new_dobject: too many objects");
    int row = c->nrows++;
    for (int f = 0; f < c->nfields; f++) {
        if (c->fields[f].multiple) continue;
        c->rows[row][c->fields[f].single_index] = c->fields[f].optional ? -1 : 0;
    }
    RepObject o;
    o.cls = c;
    o.row = row;
    return o;
}

/* Write value into single field f of object rec. A required f rejects a negative value. */
static inline void set_dobject_field(RepObject rec, RepField* f, long long value) {
    crash_if(f->multiple, "set_dobject_field: multiple field; use add_dobject_field");
    crash_if(rec.cls != f->owner, "set_dobject_field: field belongs to another class");
    crash_if(rec.row < 0 || rec.row >= rec.cls->nrows, "set_dobject_field: bad object");
    crash_if(!f->optional && value < 0, "set_dobject_field: a required field cannot be absent");
    rec.cls->rows[rec.row][f->single_index] = value;
}

/* Append value on multiple field f of object rec, recording rec's row.
 * A negative value is rejected. */
static inline void add_dobject_field(RepObject rec, RepField* f, long long value) {
    crash_if(!f->multiple, "add_dobject_field: single field; use set_dobject_field");
    crash_if(rec.cls != f->owner, "add_dobject_field: field belongs to another class");
    crash_if(value < 0, "add_dobject_field: an omitted value is not stored");
    int n = f->owner->multi_n[f->index];
    crash_if(n >= REP_MAX_MULTI, "add_dobject_field: too many values");
    f->owner->multi_vals[f->index][n] = value;
    f->owner->multi_row[f->index][n] = rec.row;
    f->owner->multi_n[f->index]++;
}

/* Packed values for a sparse or multiple field f, written into vals.
 * ats[i] is the parent row. Returns the number of values packed. */
static inline int rep_collect(RepField* f, long long* vals, long long* ats) {
    int n = 0;
    RepClass* c = f->owner;
    if (f->multiple) {
        for (int i = 0; i < c->multi_n[f->index]; i++) {
            vals[n] = c->multi_vals[f->index][i];
            if (ats) ats[n] = c->multi_row[f->index][i];
            n++;
        }
        return n;
    }
    for (int r = 0; r < c->nrows; r++) {
        long long v = c->rows[r][f->single_index];
        if (f->optional && rep_parallel && v < 0) continue;
        vals[n] = v;
        if (ats) ats[n] = r;
        n++;
    }
    return n;
}

/* Emit dq label from the n qwords in vals. An empty list emits a single 0. */
static inline void rep_emit_dq(const char* label, const long long* vals, int n) {
    char line[512];
    int k = 0;
    if (n <= 0) {
        snprintf(line, sizeof line, "0");
    } else {
        line[0] = 0;
        for (int i = 0; i < n; i++)
            k += snprintf(line + k, sizeof line - (size_t)k, "%s%lld",
                          i ? ", " : "", vals[i]);
    }
    write_dq(label, c_strdup_arena(line));
}

/* Bake storage for one class. AoS label is "<type>_data". A dense parallel
 * field is "<type>_<field>". A sparse or multiple field is that label plus
 * "<type>_<field>_at", the parent row of each stored value. */
static inline void rep_emit(RepClass* c) {
    c_ensure_data();
    if (!rep_parallel) {
        long long flat[REP_MAX_ROWS * REP_MAX_FIELDS];
        int n = 0;
        for (int r = 0; r < c->nrows; r++)
            for (int s = 0; s < c->nsingle; s++)
                flat[n++] = c->rows[r][s];
        if (c->nsingle > 0)
            rep_emit_dq(rep_join(c->type_name, "data"), flat, n);
    } else {
        for (int fi = 0; fi < c->nfields; fi++) {
            RepField* f = &c->fields[fi];
            if (f->multiple) continue;
            long long vals[REP_MAX_ROWS];
            long long ats[REP_MAX_ROWS];
            int n = rep_collect(f, vals, ats);
            const char* label = rep_join(c->type_name, f->name);
            rep_emit_dq(label, vals, n);
            if (f->optional)
                rep_emit_dq(rep_join(label, "at"), ats, n);
        }
    }
    for (int fi = 0; fi < c->nfields; fi++) {
        RepField* f = &c->fields[fi];
        if (!f->multiple) continue;
        long long vals[REP_MAX_MULTI];
        long long ats[REP_MAX_MULTI];
        int n = rep_collect(f, vals, ats);
        const char* label = rep_join(c->type_name, f->name);
        rep_emit_dq(label, vals, n);
        rep_emit_dq(rep_join(label, "at"), ats, n);
    }
}

/* Put each linked child into its parent's row. An unlinked optional child
 * becomes an absent row. Unlinked required children stop generation. */
static inline void rep_align_children(RepClass* parent) {
    for (int s = 0; s < parent->nchildren; s++) {
        if (!parent->child_bound[s]) continue;
        RepClass* ch = parent->children[s];
        int optional = parent->child_optional[s];
        long long newrows[REP_MAX_ROWS][REP_MAX_FIELDS];
        int old_to_new[REP_MAX_ROWS];
        for (int r = 0; r < REP_MAX_ROWS; r++) old_to_new[r] = -1;
        int nrows = parent->nrows;
        for (int pr = 0; pr < nrows; pr++) {
            int cr = parent->child_link[s][pr];
            if (cr < 0) {
                crash_if(!optional, "set_child: a required child is missing");
                for (int f = 0; f < ch->nfields; f++) {
                    if (ch->fields[f].multiple) continue;
                    newrows[pr][ch->fields[f].single_index] =
                        ch->fields[f].optional ? -1 : 0;
                }
            } else {
                crash_if(cr >= ch->nrows, "set_child: child row is out of range");
                for (int si = 0; si < ch->nsingle; si++)
                    newrows[pr][si] = ch->rows[cr][si];
                old_to_new[cr] = pr;
            }
        }
        for (int pr = 0; pr < nrows; pr++)
            for (int si = 0; si < ch->nsingle; si++)
                ch->rows[pr][si] = newrows[pr][si];
        for (int f = 0; f < ch->nfields; f++) {
            if (!ch->fields[f].multiple) continue;
            for (int i = 0; i < ch->multi_n[f]; i++) {
                int old = ch->multi_row[f][i];
                if (old >= 0 && old < REP_MAX_ROWS && old_to_new[old] >= 0)
                    ch->multi_row[f][i] = old_to_new[old];
            }
        }
        ch->nrows = nrows;
    }
}

/* Emit storage for class c and then for every child class. */
static inline void rep_emit_all(RepClass* c) {
    rep_align_children(c);
    rep_emit(c);
    for (int i = 0; i < c->nchildren; i++) rep_emit_all(c->children[i]);
}

/* Return 1 when field f is multiple, or optional while parallel arrays are selected. */
static inline int rep_is_sparse(RepField* f) {
    if (f->multiple) return 1;
    if (f->optional && rep_parallel) return 1;
    return 0;
}

/* Sum every present value of `f` into rax. Object mode crawls the objects.
 * Parallel mode rolls across that field's array. A sparse array has already
 * dropped the absent values, so the roll does not test them. */
static inline AsmExtern rep_gen_sum(RepField* f) {
    RepClass* c = f->owner;
    char nbuf[96];
    snprintf(nbuf, sizeof nbuf, "agg_%s_for_%s", c->type_name, f->name);
    AsmExtern fn = write_proc(c_strdup_arena(nbuf), NULL, 0,
                              (AsmDataField){ "rax", "sum", NULL });
    write_xor((AsmDataField){"rax","rax",NULL}, (AsmDataField){"rax","rax",NULL});
    write_xor((AsmDataField){"rcx","rcx",NULL}, (AsmDataField){"rcx","rcx",NULL});

    long long vals[REP_MAX_MULTI];
    long long ats[REP_MAX_MULTI];
    int packed = rep_is_sparse(f) ? rep_collect(f, vals, ats) : 0;

    const char* top = c_new_label();
    const char* done = c_new_label();

    if (f->multiple && !rep_parallel) {
        /* Crawl: for each object, scan the packed list for that parent row. */
        const char* scan = c_new_label();
        const char* nextent = c_new_label();
        const char* nextrow = c_new_label();
        const char* val_l = rep_join(c->type_name, f->name);
        const char* at_l = rep_join(val_l, "at");
        write_label(top);
            write_cmp((AsmDataField){"rcx","row",NULL}, literal(rep_num(c->nrows), "row"));
            write_jae(done);
            write_xor((AsmDataField){"r8","r8",NULL}, (AsmDataField){"r8","r8",NULL});
            write_label(scan);
                write_cmp((AsmDataField){"r8","i",NULL}, literal(rep_num(packed), "i"));
                write_jae(nextrow);
                write_lea("rdx", at_l);
                write_replace_mov("r9", (AsmDataField){ asm_mem_index("qword", "rdx", "r8", 8, 0), "row", NULL });
                write_cmp((AsmDataField){"r9","row",NULL}, (AsmDataField){"rcx","row",NULL});
                write_jne(nextent);
                write_lea("rdx", val_l);
                write_add((AsmDataField){"rax","sum",NULL},
                          (AsmDataField){ asm_mem_index("qword", "rdx", "r8", 8, 0), "v", NULL });
                write_label(nextent);
                write_inc("r8");
                write_jmp(scan);
            write_label(nextrow);
            write_inc("rcx");
            write_jmp(top);
    } else if (rep_parallel || f->multiple) {
        /* Dense parallel, or a sparse/multiple list walked in storage order. */
        const char* label = rep_join(c->type_name, f->name);
        int bound = rep_is_sparse(f) ? packed : c->nrows;
        write_lea("rdx", label);
        write_label(top);
            write_cmp((AsmDataField){"rcx","i",NULL}, literal(rep_num(bound), "i"));
            write_jae(done);
            write_add((AsmDataField){"rax","sum",NULL},
                      (AsmDataField){ asm_mem_index("qword", "rdx", "rcx", 8, 0), "v", NULL });
            write_inc("rcx");
            write_jmp(top);
    } else {
        /* Object, single field. Displacement is (row * nsingle + slot) * 8.
         * A negative value is an absent optional and is not added. */
        const char* label = rep_join(c->type_name, "data");
        const char* skip = c_new_label();
        write_lea("rdx", label);
        write_label(top);
            write_cmp((AsmDataField){"rcx","row",NULL}, literal(rep_num(c->nrows), "row"));
            write_jae(done);
            write_replace_mov("r8", (AsmDataField){ "rcx", "row", NULL });
            write_imul3("r8", "r8", asm_i(c->nsingle));
            write_add((AsmDataField){"r8","idx",NULL}, literal(rep_num(f->single_index), "slot"));
            write_replace_mov("r9", (AsmDataField){ asm_mem_index("qword", "rdx", "r8", 8, 0), f->name, NULL });
            write_test("r9", "r9");
            write_js(skip);
            write_add((AsmDataField){"rax","sum",NULL}, (AsmDataField){"r9","v",NULL});
            write_label(skip);
            write_inc("rcx");
            write_jmp(top);
    }

    write_label(done);
    write_ret();
    write_endp(&fn);
    return fn;
}

/* Load the value of object `row` (rcx) into rax. A missing optional or a
 * multiple field with no entry for that row returns -1. Sparse storage finds
 * the row by bisection on the index array. A dense field is one indexed load.
 * For a multiple field the load returns the first stored value of that row. */
static inline AsmExtern rep_gen_at(RepField* f) {
    RepClass* c = f->owner;
    char nbuf[96];
    snprintf(nbuf, sizeof nbuf, "at_%s_for_%s", c->type_name, f->name);
    static const AsmDataField rowp[] = { {"rcx", "row", NULL} };
    AsmExtern fn = write_proc(c_strdup_arena(nbuf), rowp, 1,
                              (AsmDataField){ "rax", "value", NULL });

    if (!rep_is_sparse(f) && rep_parallel) {
        const char* label = rep_join(c->type_name, f->name);
        write_lea("rdx", label);
        write_replace_mov("rax", (AsmDataField){ asm_mem_index("qword", "rdx", "rcx", 8, 0), "value", NULL });
        write_ret();
    } else if (!rep_is_sparse(f)) {
        const char* label = rep_join(c->type_name, "data");
        write_lea("rdx", label);
        write_imul3("r8", "rcx", asm_i(c->nsingle));
        write_add((AsmDataField){"r8","idx",NULL}, literal(rep_num(f->single_index), "slot"));
        write_replace_mov("rax", (AsmDataField){ asm_mem_index("qword", "rdx", "r8", 8, 0), "value", NULL });
        write_ret();
    } else {
        long long vals[REP_MAX_MULTI];
        long long ats[REP_MAX_MULTI];
        int packed = rep_collect(f, vals, ats);
        const char* val_l = rep_join(c->type_name, f->name);
        const char* at_l = rep_join(val_l, "at");
        const char* bin = c_new_label();
        const char* miss = c_new_label();
        const char* hit = c_new_label();
        const char* higher = c_new_label();
        write_xor((AsmDataField){"r8","r8",NULL}, (AsmDataField){"r8","r8",NULL});
        write_replace_mov("r9", (AsmDataField){ rep_num(packed), "n", NULL });
        write_lea("rdx", at_l);
        write_label(bin);
            write_cmp((AsmDataField){"r8","lo",NULL}, (AsmDataField){"r9","hi",NULL});
            write_jae(miss);
            write_replace_mov("r10", (AsmDataField){ "r8", "lo", NULL });
            write_add((AsmDataField){"r10","mid",NULL}, (AsmDataField){"r9","hi",NULL});
            write_shr("r10", "1");
            write_replace_mov("rax", (AsmDataField){ asm_mem_index("qword", "rdx", "r10", 8, 0), "at", NULL });
            write_cmp((AsmDataField){"rax","at",NULL}, (AsmDataField){"rcx","row",NULL});
            write_je(hit);
            write_jb(higher);
            write_replace_mov("r9", (AsmDataField){ "r10", "hi", NULL });
            write_jmp(bin);
        write_label(higher);
            write_inc("r10");
            write_replace_mov("r8", (AsmDataField){ "r10", "lo", NULL });
            write_jmp(bin);
        write_label(hit);
            write_lea("rdx", val_l);
            write_replace_mov("rax", (AsmDataField){ asm_mem_index("qword", "rdx", "r10", 8, 0), "value", NULL });
            write_ret();
        write_label(miss);
            write_replace_mov("rax", (AsmDataField){ "-1", "value", NULL });
            write_ret();
    }
    write_endp(&fn);
    return fn;
}

/* The open block borrows rcx (row), rdx and r8 (address scratch), r9 (first
 * value) and r10 (second value). They are marked in use so a c_local in the
 * body does not pick them, then released when the block closes. */
static int rep_hold_idx[] = { 1, 2, 8, 9, 10, 11 };
static int rep_hold_prev[6];
static int rep_hold_on_flag = 0;

/* Save the in-use flags of rcx, rdx, r8, r9, r10, and r11, then mark those
 * registers in use for the open representation walk. */
static inline void rep_hold_on(void) {
    for (int i = 0; i < 6; i++) {
        int idx = rep_hold_idx[i];
        rep_hold_prev[i] = c_regs[idx].in_use;
        c_regs[idx].in_use = true;
    }
    rep_hold_on_flag = 1;
}
/* If a hold is active, restore the saved in-use flags for rcx, rdx, r8, r9, r10, and r11. */
static inline void rep_hold_off(void) {
    if (!rep_hold_on_flag) return;
    for (int i = 0; i < 6; i++)
        c_regs[rep_hold_idx[i]].in_use = rep_hold_prev[i];
    rep_hold_on_flag = 0;
}

/* Load field f of the row in rcx into dest. Dense parallel is one indexed
 * load. Dense object storage is (row * stride + slot). A sparse field
 * bisects the index array and yields -1 when that row is absent. The
 * search uses r8, r10, and r11, so a second value that already lives in
 * r10 must not be a sparse load. */
static inline void rep_load_at_row(RepField* f, const char* dest) {
    RepClass* c = f->owner;
    if (!rep_is_sparse(f) && rep_parallel) {
        write_lea("rdx", rep_join(c->type_name, f->name));
        write_replace_mov(dest, (AsmDataField){ asm_mem_index("qword", "rdx", "rcx", 8, 0), f->name, NULL });
        return;
    }
    if (!rep_is_sparse(f)) {
        write_lea("rdx", rep_join(c->type_name, "data"));
        write_replace_mov("r8", (AsmDataField){ "rcx", "row", NULL });
        write_imul3("r8", "r8", asm_i(c->nsingle));
        write_add((AsmDataField){"r8","idx",NULL}, literal(rep_num(f->single_index), "slot"));
        write_replace_mov(dest, (AsmDataField){ asm_mem_index("qword", "rdx", "r8", 8, 0), f->name, NULL });
        return;
    }
    long long vals[REP_MAX_MULTI];
    long long ats[REP_MAX_MULTI];
    int packed = rep_collect(f, vals, ats);
    const char* val_l = rep_join(c->type_name, f->name);
    const char* at_l = rep_join(val_l, "at");
    const char* bin = c_new_label();
    const char* miss = c_new_label();
    const char* hit = c_new_label();
    const char* higher = c_new_label();
    const char* loaded = c_new_label();
    write_xor((AsmDataField){"r8","lo",NULL}, (AsmDataField){"r8","lo",NULL});
    write_replace_mov("r11", (AsmDataField){ rep_num(packed), "n", NULL });
    write_lea("rdx", at_l);
    write_label(bin);
        write_cmp((AsmDataField){"r8","lo",NULL}, (AsmDataField){"r11","hi",NULL});
        write_jae(miss);
        write_replace_mov("r10", (AsmDataField){ "r8", "mid", NULL });
        write_add((AsmDataField){"r10","mid",NULL}, (AsmDataField){"r11","hi",NULL});
        write_shr("r10", "1");
        write_replace_mov("r9", (AsmDataField){ asm_mem_index("qword", "rdx", "r10", 8, 0), "at", NULL });
        write_cmp((AsmDataField){"r9","at",NULL}, (AsmDataField){"rcx","row",NULL});
        write_je(hit);
        write_jb(higher);
        write_replace_mov("r11", (AsmDataField){ "r10", "hi", NULL });
        write_jmp(bin);
    write_label(higher);
        write_inc("r10");
        write_replace_mov("r8", (AsmDataField){ "r10", "lo", NULL });
        write_jmp(bin);
    write_label(hit);
        write_lea("rdx", val_l);
        write_replace_mov(dest, (AsmDataField){ asm_mem_index("qword", "rdx", "r10", 8, 0), f->name, NULL });
        write_jmp(loaded);
    write_label(miss);
        write_replace_mov(dest, (AsmDataField){ "-1", f->name, NULL });
    write_label(loaded);
}

/* One open walk. a is the value in r9. When b is set, b is the value in r10
 * of the same row. row is rcx. The body is emitted by the caller between
 * begin and end, so the operation is not baked into this header. */
typedef struct {
    const char* top;
    const char* done;
    CValue a;
    CValue b;
    CValue row;
    int has_b;
} RepGen;

/* Open a loop over one field, or over two dense fields of the same row
 * count (a child field is fine when each parent row has one child row).
 * On each iteration rcx is the row, r9 is field a, and r10 is field b
 * when b is not NULL. Close with end_rep_gen_block. */
static inline RepGen begin_rep_gen_block(RepField* a, RepField* b) {
    crash_if(a == NULL, "begin_rep_gen_block: a field is required");
    if (b) {
        crash_if(rep_is_sparse(a) || rep_is_sparse(b),
                 "begin_rep_gen_block: a pair has to be two dense fields");
        crash_if(a->owner->nrows != b->owner->nrows,
                 "begin_rep_gen_block: paired fields need the same row count");
    }
    RepGen g;
    memset(&g, 0, sizeof g);
    g.has_b = b != NULL;
    g.top = c_new_label();
    g.done = c_new_label();
    g.row = (CValue){ "row", C_PTR, "rcx", "row", 1, true, -1 };
    g.a = (CValue){ a->name, C_PTR, "r9", a->name, 9, true, -1 };
    if (b) g.b = (CValue){ b->name, C_PTR, "r10", b->name, 10, true, -1 };
    rep_hold_on();
    write_xor((AsmDataField){"rcx","row",NULL}, (AsmDataField){"rcx","row",NULL});
    write_label(g.top);
        write_cmp((AsmDataField){"rcx","row",NULL}, literal(rep_num(a->owner->nrows), "row"));
        write_jae(g.done);
        rep_load_at_row(a, "r9");
        if (b) rep_load_at_row(b, "r10");
    return g;
}

/* One field. Same loop as the two-field form. */
static inline RepGen begin_rep_gen_block1(RepField* a) {
    return begin_rep_gen_block(a, NULL);
}

/* Close walk g by incrementing rcx, jumping to its top, emitting its done
 * label, and releasing the held registers. */
static inline void end_rep_gen_block(RepGen* g) {
    write_inc("rcx");
    write_jmp(g->top);
    write_label(g->done);
    rep_hold_off();
}

/* Exit 1 through con unless at(row) equals expect. Call from inside a c function. */
static inline void rep_expect(AsmExtern* at, const char* row, const char* expect,
                              ExConsole* con) {
    CValue got = c_call_proc(C_PTR, at, literal(row, "row"));
    const char* ok = c_new_label();
    write_cmp(c_cell(got), literal(expect, "value"));
    write_je(ok);
    ex_exit(con, 1);
    write_label(ok);
    c_free(&got);
}
