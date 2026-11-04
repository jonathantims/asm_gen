
/*
 * js_funcs.h - A JavaScript-flavoured code-generation layer over c_funcs.h.
 *
 * Models three defining JS ideas, all at RUN time:
 *   - Closures over a fresh environment: makeCounter() takes the next cell
 *     from a small pool. tick(env) mutates that cell, so two counters do not
 *     share a count.
 *   - Prototype-chain property lookup: walk the object's own sparse list,
 *     then [obj+0] (__proto__). A miss yields 0 (undefined). The lookup
 *     returns the function pointer. It does not call it.
 *   - Truthiness of a tagged word. 0 is null/undefined. Low bits 01 are a
 *     number (the value is the word shifted right by 2; numeric 0 is falsy).
 *     Low bits 10 are a string (clear the low bits, and a first byte of 0 is
 *     the empty string, falsy). Any other nonzero word is an object, truthy.
 *
 * Object layout: [obj+0] = __proto__ (0 ends the chain), [obj+8] = head of
 * the own property list. A property node is three qwords: next, name id,
 * value. js_set prepends a node from a static pool, so a property can appear
 * at run time. Class methods are a static list baked at js_class_end; an
 * instance starts with [+8] pointing at that list, and a later js_set shadows
 * it by prepending. Fields declared with js_field still sit after the 16-byte
 * header. That is the accepted limit: the list does not distinguish a data
 * property from a method, and there is no this-rebinding.
 *
 * Names are mangled Class$method. '$' is reserved, same as the other layers.
 * See js_interface.md.
 */


/* ======================================================================== *
 *  Console runtime  (self-contained)
 * ======================================================================== */

typedef struct {
    AsmExtern get_std_handle;
    AsmExtern write_file;
    AsmExtern exit_process;
} JsConsole;

typedef struct { AsmDataField off; AsmDataField len; } JsStr;

/* Declare GetStdHandle, WriteFile, and ExitProcess for the JavaScript console. */
static inline JsConsole js_console_declare(void) {
    JsConsole c;
    static const char* gsh[] = { "nStdHandle" };
    c.get_std_handle = c_extern("GetStdHandle", gsh, 1, C_PTR, "handle");
    static const char* wf[] = { "handle", "lpBuffer", "nBytes", "lpWritten", "lpOverlapped" };
    c.write_file = c_extern("WriteFile", wf, 5, C_INT, "ok");
    static const char* ep[] = { "exit_code" };
    c.exit_process = c_extern("ExitProcess", ep, 1, C_PTR, NULL);
    return c;
}

/* Reserve the WriteFile byte-count global in uninitialised data. */
static inline void js_console_globals(void) {
    c_ensure_bss();
    write_unitialised_data("written", "?");
}

/* Define string name from text_literal and return its address and length cells. */
static inline JsStr js_string(const char* name, const char* text_literal) {
    c_ensure_data();
    char offname[64], lenname[64], lenexpr[80];
    snprintf(offname, sizeof(offname), "off_%s", name);
    snprintf(lenname, sizeof(lenname), "len_%s", name);
    AsmDataField d = write_initialised_data(name, text_literal, "lpBuffer");
    JsStr s;
    s.off = write_offset_constant(c_strdup_arena(offname), d, "lpBuffer");
    snprintf(lenexpr, sizeof(lenexpr), "$ - %s", name);
    s.len = write_constant(c_strdup_arena(lenname), c_strdup_arena(lenexpr), "nBytes");
    return s;
}

/* A C string plus CR LF. The db quoting stays in c_db_text. */
static inline JsStr js_line(const char* name, const char* text) {
    return js_string(name, c_db_text(text, 2, 0));
}

/* Emit print, which writes a buffer using the console externs in c. */
static inline AsmExtern js_console_print(JsConsole* c) {
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

/* c_call_proc_void, not write_call. A print sits between live values
 * (the two counters, for one). write_call leaves those registers to the
 * callee, and the next use of them is then a bad pointer. */
static inline void js_puts(AsmExtern* print, JsStr s) {
    c_call_proc_void(print, s.off, s.len);
}
/* Exit the generated program through console c with status code. */
static inline void js_exit(JsConsole* c, const char* code) {
    c_call_ext_void(&c->exit_process, literal(code, "exit_code"));
}

/* ======================================================================== *
 *  Objects, names, mangling
 * ======================================================================== */

/* Memory operand for a qword at base+off. off 0 omits the displacement.
 * Used for the object header (prototype at 0, property list at 8) and for
 * fields and property nodes. */
static inline const char* js_mem(const char* base, int off) {
    return asm_mem("qword", base, off);
}

/* Stop generation, reporting msg with a js prefix. */
static inline void js_fail(const char* msg) {
    char buf[256];
    snprintf(buf, sizeof buf, "js: %s", msg);
    host_fail(buf);
}

#define JS_MAX_CLASSES 80
#define JS_MAX_MEMBERS 80
#define JS_MAX_NAMES   80
#define JS_PROP_POOL   16

/* Interned property names. The id is the index, stored in the property node. */
static char js_names[JS_MAX_NAMES][40];
static int  js_nname = 0;

/* Intern property name and return its id, adding one when the name is new. */
static inline int js_name_id(const char* name) {
    for (int i = 0; i < js_nname; i++)
        if (strcmp(js_names[i], name) == 0) return i;
    if (js_nname >= JS_MAX_NAMES) js_fail("too many distinct property names");
    snprintf(js_names[js_nname], 40, "%s", name);
    return js_nname++;
}

/* Mangle cls and member as Class$method. '$' is not a JavaScript identifier
 * character, so a real name cannot collide with a mangled one. One helper for
 * method begin and class end. */
static inline const char* js_mangle(const char* cls, const char* member) {
    if (cls == NULL || member == NULL || strchr(cls, '$') || strchr(member, '$'))
        js_fail("'$' is reserved for mangling");
    char buf[96];
    int n = snprintf(buf, sizeof buf, "%s$%s", cls, member);
    if (n < 0 || n >= (int)sizeof buf) js_fail("mangled name too long");
    return c_strdup_arena(buf);
}

typedef struct {
    char  name[40];
    CFunc method[JS_MAX_MEMBERS];
    char  methodName[JS_MAX_MEMBERS][40];
    int   name_id[JS_MAX_MEMBERS];
    int   nmethods;
    char  fieldName[JS_MAX_MEMBERS][40];
    int   nfields;
    const char* prop_head; /* symbol of the static property list, or NULL */
} JsClassRec;

static JsClassRec  js_classes[JS_MAX_CLASSES];
static int         js_nclass = 0;
static JsClassRec* js_cur = NULL;
static int         js_cur_method = -1;

typedef struct { JsClassRec* rec; } JsClass;

/* A constructed object: its data symbol plus its class record. */
typedef struct { const char* sym; JsClassRec* rec; } JsObject;

/* Open free function name with an implicit this parameter. */
static inline CFunc js_function_begin(const char* name) {
    const CParam p[] = { {"this", C_PTR} };
    return c_begin_function(name, p, 1, C_PTR, NULL);
}
/* Close function f. */
static inline void js_function_end(CFunc* f) { c_end_function(f); }

/* Open class name and select it as the current class. */
static inline void js_class_begin(const char* name) {
    if (js_nclass >= JS_MAX_CLASSES) js_fail("too many classes");
    if (name == NULL || strchr(name, '$')) js_fail("'$' is reserved for mangling");
    js_cur = &js_classes[js_nclass++];
    memset(js_cur, 0, sizeof *js_cur);
    snprintf(js_cur->name, sizeof js_cur->name, "%s", name);
}

/* Open Class$method with an implicit receiver (`this` = parameter 0). */
static inline CFunc* js_method_begin(const char* method) {
    if (!js_cur) js_fail("method declared outside a class");
    if (js_cur->nmethods >= JS_MAX_MEMBERS) js_fail("too many methods");
    int i = js_cur->nmethods++;
    snprintf(js_cur->methodName[i], 40, "%s", method);
    js_cur->name_id[i] = js_name_id(method);
    const char* mangled = js_mangle(js_cur->name, method);
    const CParam p[] = { {"this", C_PTR} };
    js_cur->method[i] = c_begin_function(mangled, p, 1, C_PTR, NULL);
    js_cur_method = i;
    return &js_cur->method[i];
}

/* Close the open method and clear the current method index. */
static inline void js_method_end(void) {
    if (!js_cur || js_cur_method < 0) js_fail("js_method_end without an open method");
    c_end_function(&js_cur->method[js_cur_method]);
    js_cur_method = -1;
}

/* Return the open method's receiver, its parameter 0. */
static inline CValue js_this(void) {
    if (!js_cur || js_cur_method < 0) js_fail("js_this outside a method body");
    return c_param(&js_cur->method[js_cur_method], 0);
}

/* Record data field name after the 16-byte header. */
static inline void js_field(const char* name) {
    if (!js_cur) js_fail("js_field outside a class");
    if (js_cur->nfields >= JS_MAX_MEMBERS) js_fail("too many fields");
    snprintf(js_cur->fieldName[js_cur->nfields++], 40, "%s", name);
}

/* Byte offset of field name in class record r, after the 16-byte header. */
static inline int js_field_offset(JsClassRec* r, const char* name) {
    for (int i = 0; i < r->nfields; i++)
        if (strcmp(r->fieldName[i], name) == 0) return 16 + i * 8;
    js_fail("js_field_offset: no such field");
    return 0;
}

/* Bake the method list: each node is next, name id, function pointer.
 * The first declared method is the head. Call while the code section is open;
 * the nodes are switched into .data and the section returns to .code. */
static inline JsClass js_class_end(void) {
    if (!js_cur) js_fail("js_class_end without an open class");
    JsClassRec* r = js_cur;
    const char* next = NULL;
    write_data();
    for (int i = r->nmethods - 1; i >= 0; i--) {
        char sym[64];
        snprintf(sym, sizeof sym, "%s_n%d", r->name, i);
        const char* node = c_strdup_arena(sym);
        char idb[16];
        snprintf(idb, sizeof idb, "%d", r->name_id[i]);
        if (next) c_ptr_data(node, next, "next");
        else      c_word_data(node, "0", "next");
        write_dq("", c_strdup_arena(idb));
        write_dq("", c_offset(js_mangle(r->name, r->methodName[i])));
        next = node;
    }
    write_code();
    r->prop_head = next;
    js_cur = NULL;
    js_cur_method = -1;
    JsClass h; h.rec = r; return h;
}

/* Build instance name of cls with prototype proto: [+0] __proto__, [+8]
 * property-list head, then one zero qword per field. */
static inline JsObject js_construct(JsClass* cls, const char* name, const char* proto) {
    JsClassRec* r = cls->rec;
    char tname[64];
    snprintf(tname, sizeof tname, "%s_t", name);
    const char* list = c_strdup_arena(tname);
    /* Low bits 00 are the object tag. A dq after a string is not
     * otherwise aligned, and truthiness would read it as a string. */
    write_data();
        write_align("8");
        if (proto) c_ptr_data(name, proto, "obj");
        else       c_word_data(name, "0", "obj");
        if (r->prop_head) c_ptr_data(list, r->prop_head, "list");
        else              c_word_data(list, "0", "list");
        for (int i = 0; i < r->nfields; i++) write_dq("", "0");
    write_code();
    JsObject o; o.sym = name; o.rec = r; return o;
}

/* this.name of type inside the open method. The address is this's current cell
 * plus the field offset, so a spill of this is visible here. */
static inline CValue js_this_field(const char* name, CType type) {
    if (!js_cur || js_cur_method < 0) js_fail("js_this_field outside a method body");
    CValue self = c_resolve(js_this());
    int off = js_field_offset(js_cur, name);
    AsmDataField f = { js_mem(self.loc, off), name, NULL };
    return c_wrap(f, type);
}

/* Field name of type on object o from outside a method, as a qword in the
 * object's data symbol. */
static inline CValue js_field_ref(JsObject* o, const char* name, CType type) {
    int off = js_field_offset(o->rec, name);
    AsmDataField f = { asm_mem("qword", o->sym, off), name, NULL };
    return c_wrap(f, type);
}

/* Store immediate imm into memory field. */
static inline void js_field_store_imm(CValue field, const char* imm) {
    write_replace_mov(field.loc, (AsmDataField){ imm, field.tag, NULL });
}

/* Add immediate imm to memory field in place. */
static inline void js_field_add_imm(CValue field, long long n) {
    write_add((AsmDataField){ field.loc, field.tag, NULL }, literal(c_dec(n), "one"));
}

/* Call fn with recv as this. */
static inline void js_send(CFunc* fn, JsObject recv) {
    c_call(fn, c_addr(recv.sym, "this"));
}

/* Property-node pool and the two string literals the truthiness tour reads.
 * Call from write_data(). Closures are not in this pool: makeCounter in
 * js_main.c allocates each cell from the collector. */
static inline void js_data(void) {
    c_ensure_data();
    write_dq("js_nodes", asm_dup(JS_PROP_POOL * 3, "0"));
    c_word_data("js_node_i", "0", "n");
    write_align("4");
    write_db("js_empty", "0");
    write_align("4");
    write_db("js_hi", "\"x\", 0");
}

/* js_set(obj, name_id, value) prepends a node. Call once from write_code(),
 * before any js_set(). The pool is JS_PROP_POOL nodes; past that, ud2. */
static AsmExtern js_set_fn;
static int js_runtime_ready = 0;

/* Emit the js_set procedure: prepend a pool node, or ud2 when the pool is full. */
static inline void js_emit_runtime(void) {
    static const AsmDataField pr[] = {
        {"rcx","obj",NULL}, {"rdx","id",NULL}, {"r8","value",NULL}
    };
    js_set_fn = write_proc("js_set", pr, 3, asm_void);
        const char* ok = c_new_label();
        write_replace_mov("rax", (AsmDataField){ "js_node_i", "n", NULL });
        write_cmp((AsmDataField){"rax","n",NULL}, literal(asm_i(JS_PROP_POOL), "n"));
        write_jb(ok);
        write_ud2();
        write_label(ok);
        write_replace_mov("r10", (AsmDataField){ "rax", "n", NULL });
        write_inc(asm_mem("qword", "js_node_i", 0));
        write_imul3("r10", "r10", "24");
        write_lea("r11", "js_nodes");
        write_add((AsmDataField){"r11","node",NULL}, (AsmDataField){"r10","off",NULL});
        write_replace_mov("rax", (AsmDataField){ asm_mem("qword", "rcx", 8), "list", NULL });
        write_replace_mov(asm_mem("qword", "r11", 0), (AsmDataField){ "rax", "next", NULL });
        write_replace_mov(asm_mem("qword", "r11", 8), (AsmDataField){ "rdx", "id", NULL });
        write_replace_mov(asm_mem("qword", "r11", 16), (AsmDataField){ "r8", "value", NULL });
        write_replace_mov(asm_mem("qword", "rcx", 8), (AsmDataField){ "r11", "list", NULL });
        write_ret();
    write_endp(&js_set_fn);
    js_runtime_ready = 1;
}

/* Prepend property name with value onto obj via the emitted js_set procedure. */
static inline void js_set(CValue obj, const char* name, CValue value) {
    if (!js_runtime_ready) js_fail("js_set before js_emit_runtime");
    c_call_proc_void(&js_set_fn,
        treat_as(c_cell(obj), obj.tag, "obj"),
        literal(asm_i(js_name_id(name)), "id"),
        treat_as(c_cell(value), value.tag, "value"));
}

/*
 * js_lookup - walk own list, then __proto__, and return the value word in a
 * register (0 when the name is missing). Does not call it. rbx is the walker;
 * the frame prologue already saved it. r10 is the node cursor, r11 the result.
 */
static inline CValue js_lookup(CValue self_obj, const char* name) {
    const char* idlit = asm_i(js_name_id(name));
    const char* loop = c_new_label();
    const char* scan = c_new_label();
    const char* up = c_new_label();
    const char* hit = c_new_label();
    const char* done = c_new_label();

    self_obj = c_resolve(self_obj);
    /* rbx walks objects. It is callee-saved, so the push keeps the caller's
     * value. r10 is the node. r11 is the value, or 0 when the name is missing. */
    write_push("rbx");
    write_replace_mov("rbx", (AsmDataField){ self_obj.loc, "obj", NULL });
    write_xor((AsmDataField){"r11","r11",NULL}, (AsmDataField){"r11","r11",NULL});
    write_label(loop);
        write_cmp((AsmDataField){"rbx","obj",NULL}, literal("0","obj"));
        write_je(done);
        write_replace_mov("r10", (AsmDataField){ js_mem("rbx", 8), "list", NULL });
        write_label(scan);
            write_cmp((AsmDataField){"r10","node",NULL}, literal("0","node"));
            write_je(up);
            write_cmp((AsmDataField){ js_mem("r10", 8), "id", NULL }, literal(idlit, "id"));
            write_je(hit);
            write_replace_mov("r10", (AsmDataField){ js_mem("r10", 0), "next", NULL });
            write_jmp(scan);
        write_label(up);
            write_replace_mov("rbx", (AsmDataField){ js_mem("rbx", 0), "proto", NULL });
            write_jmp(loop);
        write_label(hit);
            write_replace_mov("r11", (AsmDataField){ js_mem("r10", 16), "fn", NULL });
    write_label(done);
    write_pop("rbx");

    int idx = c_alloc_reg();
    const char* loc = c_reg_name(idx, 8);
    c_materialize(loc, "r11", "fn");
    c_reg_owner_name[idx] = "fn";
    c_reg_size[idx] = 8;
    CValue v = { "fn", C_PTR, loc, "fn", idx, true, -1 };
    return v;
}

/* call fn(this), passing receiver self. fn.loc is the register that holds the function pointer. */
static inline void js_call_fn(CValue fn, CValue self) {
    fn = c_resolve(fn);
    self = c_resolve(self);
    AsmDataField arg;
    memset(&arg, 0, sizeof arg);
    arg.name = "rcx";
    arg.type = "this";
    arg.is_register = 1;
    AsmExtern ind;
    memset(&ind, 0, sizeof ind);
    ind.name = fn.loc;
    ind.params = &arg;
    ind.param_count = 1;
    ind.return_reg = asm_void;
    write_call(&ind, treat_as(c_cell(self), self.tag, "this"));
}

/* Tagged number name: (n << 2) | 1. Numeric 0 is the word 1, which is falsy. */
static inline CValue js_number(const char* name, int n) {
    return c_local_imm(name, C_PTR, asm_i((n << 2) | 1));
}

/* Address of symbol in register name. Low bits stay 0, so this is an object. */
static inline CValue js_word_lea(const char* name, const char* symbol) {
    CValue v = c_local_imm(name, C_PTR, "0");
    write_lea(v.loc, symbol);
    return v;
}

/* String value name: the address of symbol with low bits 10. */
static inline CValue js_string_val(const char* name, const char* symbol) {
    CValue v = js_word_lea(name, symbol);
    write_or(v.loc, "2");
    return v;
}

/*
 * Truthiness. THEN when the word is truthy, ELSE when it is falsy.
 *   0            null / undefined
 *   low bits 01  number, falsy when the number is 0
 *   low bits 10  string, falsy when the first byte is 0
 *   other nonzero object (low bits 00), truthy
 * Clobbers r10 and r11. The caller's register is left as it was.
 */
static inline CElse js_if_truthy_else(CValue value) {
    CElse c;
    memset(&c, 0, sizeof c);
    c.else_label = c_new_label();
    c.done_label = c_new_label();
    const char* as_num = c_new_label();
    const char* as_str = c_new_label();
    const char* truthy = c_new_label();

    value = c_resolve(value);
    write_replace_mov("r11", (AsmDataField){ value.loc, "jsval", NULL });
    write_test("r11", "r11");
    write_jz(c.else_label);
    write_replace_mov("r10", (AsmDataField){ "r11", "jsval", NULL });
    write_and("r10", "3");
    write_cmp((AsmDataField){"r10","tag",NULL}, literal("1","tag"));
    write_je(as_num);
    write_cmp((AsmDataField){"r10","tag",NULL}, literal("2","tag"));
    write_je(as_str);
    write_jmp(truthy);

    write_label(as_num);
        write_shr("r11", "2");
        write_test("r11", "r11");
        write_jz(c.else_label);
        write_jmp(truthy);
    write_label(as_str);
        write_and("r11", "-4");
        write_cmp((AsmDataField){ asm_mem("byte", "r11", 0), "ch", NULL },
                  literal("0", "ch"));
        write_je(c.else_label);
    write_label(truthy);
    c.entry = c_clobber_save();
    c.id = c_block_push(c_else_stack, &c_else_sp);
    return c;
}

/* Leaf environment: 16-byte collector header plus an 8-byte count at
 * offset 16. No pointer fields, so a compacting collector has nothing
 * inside the cell to rewrite. Emit this from write_data(), before
 * makeCounter in js_main.c. */
static inline GcType js_env_type(void) {
    return gc_typedesc("Env", 24, NULL, 0);
}

/* The count field of a makeCounter environment. Offset 16 is the first
 * payload byte after the collector header. The address is recomputed from
 * the value's current location. */
static inline CValue js_counter_field(CValue env) {
    env = c_resolve(env);
    const char* base = gc_load_ptr(env, "rax");
    AsmDataField f = { asm_mem("qword", base, 16), "count", NULL };
    return c_wrap(f, C_PTR);
}

#ifdef GC_SWITCH
/* Allocate one counter environment and zero its count. The result is
 * tracked, so later calls see it move or spill. The source included gc.h. */
static inline CValue js_counter_new(GcHeap* h, GcType* env) {
    CValue obj = gc_new(h, env, "env");
    obj = c_track(obj, 0);
    js_field_store_imm(js_counter_field(obj), "0");
    return obj;
}
#endif

/*
 * js_init - once at the top of main. Case-sensitive identifiers, and an empty
 * class list and name table.
 */
static inline void js_init(void) {
    write_set_case_sensitive_identifiers();
    js_nclass = 0;
    js_nname = 0;
    js_cur = NULL;
    js_cur_method = -1;
    js_runtime_ready = 0;
}
