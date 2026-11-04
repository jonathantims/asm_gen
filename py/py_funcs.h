
/*
 * py_funcs.h - A Python-flavoured code-generation layer over c_funcs.h / asm_funcs.h.
 *
 * Models three defining CPython ideas, all resolved at RUN time (Python performs
 * essentially no static checking - the teaching point versus Rust/C++):
 *   - Reference counting: every object begins with an 8-byte refcount; incref
 *     bumps it, decref drops it and deallocates (runs __del__) exactly when it
 *     reaches zero - a real runtime branch.
 *   - Duck typing / dynamic dispatch: a method is found by looking it up in the
 *     object's per-type method table at call time; any type exposing the slot
 *     works (no shared base class).
 *   - EAFP + runtime TypeError: the checked call looks before it uses a field
 *     and lets the caller handle a missing one; the required call attempts the
 *     use and, if the field is absent, prints TypeError and exits nonzero.
 *
 * Object layout (globals): [obj+0] = refcount, [obj+8] = pointer to type's
 * method table. Method table: [+0] slot0, [+8] slot1, ...  (0 = "no such attr").
 *
 * SELF-CONTAINED. See py_interface.md for the narrative.
 */


/* ======================================================================== *
 *  Console runtime  (self-contained)
 * ======================================================================== */

typedef struct {
    AsmExtern get_std_handle;
    AsmExtern write_file;
    AsmExtern exit_process;
} PyConsole;

typedef struct { AsmDataField off; AsmDataField len; } PyStr;

/* Declare GetStdHandle, WriteFile, and ExitProcess for the Python console. */
static inline PyConsole py_console_declare(void) {
    PyConsole c;
    static const char* gsh[] = { "nStdHandle" };
    c.get_std_handle = c_extern("GetStdHandle", gsh, 1, C_PTR, "handle");
    static const char* wf[] = { "handle", "lpBuffer", "nBytes", "lpWritten", "lpOverlapped" };
    c.write_file = c_extern("WriteFile", wf, 5, C_INT, "ok");
    static const char* ep[] = { "exit_code" };
    c.exit_process = c_extern("ExitProcess", ep, 1, C_PTR, NULL);
    return c;
}

/* Reserve the WriteFile byte-count global in uninitialised data. */
static inline void py_console_globals(void) {
    c_ensure_bss();
    write_unitialised_data("written", "?");
}

/* Define string name from a MASM db operand and return its address and length. */
static inline PyStr py_string(const char* name, const char* text_literal) {
    CStr s = c_string(name, text_literal, "lpBuffer", "nBytes");
    PyStr o;
    o.off = s.off;
    o.len = s.len;
    return o;
}

/* Address and length of a C string plus CRLF. */
static inline PyStr py_line(const char* name, const char* text) {
    return py_string(name, c_db_text(text, 2, 0));
}

/* Emit print, which writes a buffer using the console externs in c. */
static inline AsmExtern py_console_print(PyConsole* c) {
    static const AsmDataField pr[] = { {"rcx","lpBuffer",NULL}, {"edx","nBytes",NULL} };
    AsmExtern print = write_proc("print", pr, 2, asm_void);
        write_push("rsi");
        write_push("rdi");
        write_replace_mov("rsi", (AsmDataField){ "rcx", "lpBuffer", NULL });
        write_replace_mov("edi", (AsmDataField){ "edx", "nBytes", NULL });
        write_extern_call(&c->get_std_handle, literal("-11", "nStdHandle"));
        write_extern_call(&c->write_file,
            (AsmDataField){"rax","handle",NULL},
            (AsmDataField){"rsi","lpBuffer",NULL},
            (AsmDataField){"rdi","nBytes",NULL},
            literal("offset written","lpWritten"),
            literal("0","lpOverlapped"));
        write_pop("rdi");
        write_pop("rsi");
        write_ret();
    write_endp(&print);
    return print;
}

/* Call print with the address and length stored in s. */
static inline void py_puts(AsmExtern* print, PyStr s) { write_call(print, s.off, s.len); }
/* Exit the generated program through console c with status code. */
static inline void py_exit(PyConsole* c, const char* code) {
    c_call_ext_void(&c->exit_process, literal(code, "exit_code"));
}

/* ======================================================================== *
 *  Objects, memory operands, dispatch
 * ======================================================================== */

/* Qword memory operand at base plus byte displacement off. */
static inline const char* py_mem(const char* base, int off) {
    char buf[48];
    if (off == 0) snprintf(buf, sizeof(buf), "qword ptr [%s]", base);
    else          snprintf(buf, sizeof(buf), "qword ptr [%s+%d]", base, off);
    return c_strdup_arena(buf);
}

/* ======================================================================== *
 *  Classes / types: definition builder, per-type method tables
 *
 *  A Python `class` is described between py_class_begin(name) and py_class_end():
 *  declare its methods with py_method_begin (each with an implicit `self`).
 *  py_class_end bakes the type's method table "Name_table" into .data and returns
 *  a handle used to create instances. Every object stores a pointer to its type
 *  table at [self+8]; duck-typed dispatch resolves a method by looking it up
 *  there (py_dispatch), so unrelated types work as long as they expose the slot.
 *
 *  Attribute names are interned into fixed table slots shared by all types (so
 *  every table agrees "speak" is slot 0, "quack" is slot 1, ...). Tables are
 *  padded to PY_MAX_ATTRS slots, with 0 (null) for attributes a type lacks -
 *  which is exactly what EAFP probes for. This makes any attribute safe to look
 *  up on any object regardless of the order types were defined.
 * ======================================================================== */

#define PY_MAX_CLASSES 8
#define PY_MAX_MEMBERS 8
#define PY_MAX_ATTRS   4    /* table width: distinct attribute names, padded */

/* Stop generation with msg. */
static inline void py_fail(const char* msg) {
    char buf[256];
    snprintf(buf, sizeof buf, "py_funcs: %s", msg);
    host_fail(buf);
}

/* Global attribute-name -> slot interning table, shared by every type. */
static char py_attr_names[PY_MAX_ATTRS][40];
static int  py_nattr = 0;

/* Intern an attribute/method name, returning its fixed slot (adds it if new). */
static inline int py_attr(const char* name) {
    for (int i = 0; i < py_nattr; i++)
        if (strcmp(py_attr_names[i], name) == 0) return i;
    if (py_nattr >= PY_MAX_ATTRS) py_fail("too many distinct attributes");
    snprintf(py_attr_names[py_nattr], 40, "%s", name);
    return py_nattr++;
}

typedef struct {
    char  name[40];                          /* type name, e.g. "Dog"       */
    CFunc method[PY_MAX_MEMBERS];             /* stable storage: bodies       */
    char  methodName[PY_MAX_MEMBERS][40];
    int   slot[PY_MAX_MEMBERS];               /* interned attribute slot      */
    int   nmethods;
    char  fieldName[PY_MAX_MEMBERS][40];      /* instance fields after header */
    int   nfields;
} PyClassRec;

static PyClassRec  py_classes[PY_MAX_CLASSES];
static int         py_nclass = 0;
static PyClassRec* py_cur = NULL;
static int         py_cur_method = -1;

typedef struct { PyClassRec* rec; } PyClass;

/* A handle to a constructed instance: its data symbol plus its class record, so
 * call/field helpers resolve the type table and field offsets without the
 * generator repeating the object name. Returned by py_construct. */
typedef struct { const char* sym; PyClassRec* rec; } PyObject;

/* Open a method `Name_method` with an implicit receiver (`self` in rcx =
 * param 0). Interns the method name to its table slot and returns a stable
 * CFunc* for the body (close with py_method_end). */
static inline CFunc* py_method_begin(const char* method) {
    if (!py_cur) py_fail("method declared outside a class");
    if (py_cur->nmethods >= PY_MAX_MEMBERS) py_fail("too many methods");
    int i = py_cur->nmethods++;
    snprintf(py_cur->methodName[i], 40, "%s", method);
    py_cur->slot[i] = py_attr(method);
    char mangled[96];
    snprintf(mangled, sizeof mangled, "%s_%s", py_cur->name, method);
    const CParam p[] = { {"self", C_PTR} };
    py_cur->method[i] = c_begin_function(c_strdup_arena(mangled), p, 1, C_PTR, NULL);
    py_cur_method = i;
    return &py_cur->method[i];
}

/* Close the open method and clear the current method index. */
static inline void py_method_end(void) {
    if (!py_cur || py_cur_method < 0) py_fail("py_method_end without an open method");
    c_end_function(&py_cur->method[py_cur_method]);
    py_cur_method = -1;
}

/* The receiver of the method being defined (implicit `self`). */
static inline CValue py_self(void) {
    if (!py_cur || py_cur_method < 0) py_fail("py_self outside a method body");
    return c_param(&py_cur->method[py_cur_method], 0);
}

/* Open class name and select it as the current class. */
static inline void py_class_begin(const char* name) {
    if (py_nclass >= PY_MAX_CLASSES) py_fail("too many classes");
    py_cur = &py_classes[py_nclass++];
    memset(py_cur, 0, sizeof *py_cur);
    snprintf(py_cur->name, sizeof py_cur->name, "%s", name);
}

/* Declare instance field name. Fields follow the 16-byte header (refcount +
 * typetable pointer) in declaration order, one qword each. */
static inline void py_field(const char* name) {
    if (!py_cur) py_fail("py_field outside a class");
    if (py_cur->nfields >= PY_MAX_MEMBERS) py_fail("too many fields");
    snprintf(py_cur->fieldName[py_cur->nfields++], 40, "%s", name);
}

/* Byte offset of field `name`: after the [refcount][typetable] header. */
static inline int py_field_offset(PyClassRec* r, const char* name) {
    for (int i = 0; i < r->nfields; i++)
        if (strcmp(r->fieldName[i], name) == 0) return 16 + i * 8;
    py_fail("py_field_offset: no such field");
    return 0;
}

/* The type's method-table symbol ("Name_table"), arena-stored. */
static inline const char* py_table_sym(PyClassRec* r) {
    char buf[64];
    snprintf(buf, sizeof buf, "%s_table", r->name);
    return c_strdup_arena(buf);
}

/* Finish the type: bake its method table into .data (PY_MAX_ATTRS slots; each
 * holds the type's method for that attribute, or 0 if it lacks it). .data from
 * within .code is safe - MASM merges same-named segments. */
static inline PyClass py_class_end(void) {
    if (!py_cur) py_fail("py_class_end without an open class");
    PyClassRec* r = py_cur;
    const char* tbl = py_table_sym(r);
    write_data();
        for (int s = 0; s < PY_MAX_ATTRS; s++) {
            const char* msym = NULL;               /* method holding slot s */
            for (int i = 0; i < r->nmethods; i++) {
                if (r->slot[i] == s) {
                    char m[96];
                    snprintf(m, sizeof m, "%s_%s", r->name, r->methodName[i]);
                    msym = c_strdup_arena(m);
                    break;
                }
            }
            if (s == 0) {
                if (msym) c_ptr_data(tbl, msym, "method");
                else      c_word_data(tbl, "0", "method");
            } else {
                if (msym) write_dq("", c_offset(msym));
                else      write_dq("", "0");
            }
        }
    write_code();
    py_cur = NULL;
    py_cur_method = -1;
    PyClass h; h.rec = r; return h;
}

/*
 * py_construct - create an instance at data symbol `name`: bake its storage into
 * .data ([+0] refcount = 1, [+8] pointer to the type's method table, then one
 * zero-initialised qword per instance field) and return a PyObject handle used
 * for later calls. Replaces the old py_instance and additionally supports
 * per-instance fields.
 */
static inline PyObject py_construct(PyClass* cls, const char* name) {
    PyClassRec* r = cls->rec;
    char tname[64];
    snprintf(tname, sizeof tname, "%s_t", name);
    write_data();
        c_word_data(name, "1", "refcount");
        c_ptr_data(c_strdup_arena(tname), py_table_sym(r), "typetable");
        for (int i = 0; i < r->nfields; i++) write_dq("", "0");
    write_code();
    PyObject o; o.sym = name; o.rec = r; return o;
}

/* ---- Member-field access ----------------------------------------------- */

/* self.name of type inside a method body (self held in its parameter register). */
static inline CValue py_this_field(const char* name, CType type) {
    if (!py_cur || py_cur_method < 0) py_fail("py_this_field outside a method body");
    CValue self = c_resolve(py_self());
    int off = py_field_offset(py_cur, name);
    AsmDataField f = { py_mem(self.loc, off), name, NULL };
    return c_wrap(f, type);
}

/* Field name of type on object o from outside, addressing the object's global storage directly. */
static inline CValue py_field_ref(PyObject* o, const char* name, CType type) {
    int off = py_field_offset(o->rec, name);
    char buf[96];
    snprintf(buf, sizeof buf, "qword ptr %s+%d", o->sym, off);
    AsmDataField f = { c_strdup_arena(buf), name, NULL };
    return c_wrap(f, type);
}

/* Store an immediate into a (memory-resident) field cell. */
static inline void py_field_store_imm(CValue field, const char* imm) {
    write_replace_mov(field.loc, (AsmDataField){ imm, field.tag, NULL });
}

/* Add an immediate to a (memory-resident) field cell in place. */
static inline void py_field_add_imm(CValue field, long long n) {
    write_add((AsmDataField){ field.loc, field.tag, NULL }, literal(c_dec(n), "one"));
}

/* Subtract 40 bytes from rsp before an indirect call. */
static inline void py_frame_open(void)  { write_sub((AsmDataField){"rsp","rsp",NULL}, literal("40","frame")); }
/* Add 40 bytes back to rsp after an indirect call. */
static inline void py_frame_close(void) { write_add((AsmDataField){"rsp","rsp",NULL}, literal("40","frame")); }

/* Indirect-call the function pointer held in reg. */
static inline void py_call_reg(const char* reg) {
    AsmExtern ind = { reg, NULL, 0, asm_void };
    write_call(&ind);
}

/*
 * py_dispatch - duck-typed method call. Loads the type's method table from
 * [self+8], the slot pointer from [table + slot*8], and CALLs it with self still
 * in rcx. Any type exposing this slot works - no common base class required.
 * `self_obj` is the receiver value (normally the method's param 0).
 */
static inline void py_dispatch(CValue self_obj, int slot) {
    self_obj = c_resolve(self_obj);
    const char* self_reg = self_obj.loc;
    write_replace_mov("rax", (AsmDataField){ py_mem(self_reg, 8), "typetable", NULL });
    write_replace_mov("rax", (AsmDataField){ py_mem("rax", slot * 8), "method", NULL });
    py_frame_open();
    py_call_reg("rax");
    py_frame_close();
}

/*
 * py_dispatch_checked - look before using an attribute, as a two-armed
 * conditional. Loads the slot; if it is present the method is CALLed and the
 * THEN arm runs; if it is 0 (attribute absent) control diverts to the ELSE
 * arm. The caller handles the missing field. Use like c_if_*_else:
 *
 *     CElse miss = py_dispatch_checked(self, 1);
 *         <present: method was called>
 *     c_else(&miss);
 *         <absent: raise TypeError>
 *     c_end_if_else(&miss);
 */
static inline CElse py_dispatch_checked(CValue self_obj, int slot) {
    self_obj = c_resolve(self_obj);
    const char* self_reg = self_obj.loc;
    CElse c;
    memset(&c, 0, sizeof c);
    c.else_label = c_new_label();
    c.done_label = c_new_label();
    write_replace_mov("rax", (AsmDataField){ py_mem(self_reg, 8), "typetable", NULL });
    write_replace_mov("rax", (AsmDataField){ py_mem("rax", slot * 8), "method", NULL });
    write_cmp((AsmDataField){"rax","method",NULL}, literal("0","method"));
    write_je(c.else_label);            /* attribute absent -> else (raise) */
    c.entry = c_clobber_save();
    c.id = c_block_push(c_else_stack, &c_else_sp);
    py_frame_open();
    py_call_reg("rax");
    py_frame_close();
    return c;
}

/* ---- Calls with an implicit `self` -------------------------------------- *
 * These resolve an attribute by NAME (via the interned slot table) so the
 * caller never writes a magic slot number or the `self` tag. */

/* Plain call of free function fn on recv, passing it as self. */
static inline void py_send(CFunc* fn, PyObject recv) {
    c_call(fn, c_addr(recv.sym, "self"));
}

/* Duck-typed dispatch of attribute `method` on receiver `self_obj`. */
static inline void py_dispatch_name(CValue self_obj, const char* method) {
    py_dispatch(self_obj, py_attr(method));
}

/* Checked dispatch of attribute method on self_obj: looks before the use.
 * Present -> called + THEN arm; absent -> ELSE arm. See py_dispatch_checked. */
static inline CElse py_dispatch_checked_name(CValue self_obj, const char* method) {
    return py_dispatch_checked(self_obj, py_attr(method));
}

/*
 * py_dispatch_required_name - attempt attribute `method`. Present: CALL it.
 * Absent: print `err` (a TypeError line) and exit the generated program with
 * a nonzero status. Does not return a CElse; the caller does not handle it.
 */
static inline void py_dispatch_required_name(CValue self_obj, const char* method,
                                             AsmExtern* print, PyStr err,
                                             PyConsole* con) {
    int slot = py_attr(method);
    self_obj = c_resolve(self_obj);
    const char* self_reg = self_obj.loc;
    const char* miss = c_new_label();
    const char* done = c_new_label();
    write_replace_mov("rax", (AsmDataField){ py_mem(self_reg, 8), "typetable", NULL });
    write_replace_mov("rax", (AsmDataField){ py_mem("rax", slot * 8), "method", NULL });
    write_cmp((AsmDataField){"rax","method",NULL}, literal("0","method"));
    write_je(miss);
    py_frame_open();
    py_call_reg("rax");
    py_frame_close();
    write_jmp(done);
    write_label(miss);
        py_puts(print, err);
        py_exit(con, "1");
    write_label(done);
}

/* ======================================================================== *
 *  Reference counting
 * ======================================================================== */

/*
 * py_emit_refcount_ops - emit the py_incref / py_decref procedures. decref
 * decrements [self+0] and, when it reaches 0, prints `deallocated_msg` (models
 * running __del__ and freeing); otherwise prints `alive_msg`. Returns the two
 * procs via out-params so callers can invoke them.
 */
static inline void py_emit_refcount_ops(AsmExtern* print, PyStr alive_msg, PyStr freed_msg,
                                        AsmExtern* out_incref, AsmExtern* out_decref) {
    const CParam p[] = { {"self", C_PTR} };

    CFunc incref = c_begin_function("py_incref", p, 1, C_PTR, NULL);
        write_replace_mov("rax", (AsmDataField){ py_mem("rcx", 0), "refcount", NULL });
        write_add((AsmDataField){"rax","refcount",NULL}, literal("1","one"));
        write_replace_mov(py_mem("rcx", 0), (AsmDataField){ "rax", "refcount", NULL });
        c_ret(&incref);
    c_end_function(&incref);
    *out_incref = incref.ext;

    CFunc decref = c_begin_function("py_decref", p, 1, C_PTR, NULL);
        write_replace_mov("rax", (AsmDataField){ py_mem("rcx", 0), "refcount", NULL });
        write_sub((AsmDataField){"rax","refcount",NULL}, literal("1","one"));
        write_replace_mov(py_mem("rcx", 0), (AsmDataField){ "rax", "refcount", NULL });
        write_if_not_equal_then_jump_to("Lpy_alive",
            (AsmDataField){"rax","refcount",NULL}, literal("0","refcount"));
            py_puts(print, freed_msg);   /* rax == 0 -> deallocate */
            write_jmp("Lpy_decref_done");
        write_label("Lpy_alive");
            py_puts(print, alive_msg);
        write_label("Lpy_decref_done");
            c_ret(&decref);              /* one epilogue; the frame emits it once */
    c_end_function(&decref);
    *out_decref = decref.ext;
}

/* incref / decref on a constructed object. The procedures come from
 * py_emit_refcount_ops. The object is the same model as Dog and Cat. */
static inline void py_incref(AsmExtern* op, PyObject obj) {
    c_call_proc_void(op, c_addr(obj.sym, "self"));
}

static inline void py_decref(AsmExtern* op, PyObject obj) {
    c_call_proc_void(op, c_addr(obj.sym, "self"));
}

/* ======================================================================== *
 *  Initialisation
 * ======================================================================== */

/*
 * py_init - per-language setup a Python generator calls once, in place of poking
 * assembler options directly. It selects case-sensitive identifiers (Python is
 * case-sensitive and Win32 externs are mixed-case) and resets the class-builder
 * and attribute-intern state so a fresh program starts empty.
 */
static inline void py_init(void) {
    write_set_case_sensitive_identifiers();
    py_nclass = 0;
    py_nattr = 0;
    py_cur = NULL;
    py_cur_method = -1;
}

