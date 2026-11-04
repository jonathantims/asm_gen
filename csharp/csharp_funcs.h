
/*
 * csharp_funcs.h - A C#-flavoured code-generation layer over c_funcs.h / asm_funcs.h.
 *
 * Adds the managed C# object model: every object begins with a MethodTable
 * pointer (the CLR's per-type descriptor), virtual/interface calls dispatch
 * through it, value types (`struct`) are copied while reference types (`class`)
 * are shared, and value types cross into `object` by BOXING (a heap header + the
 * copied value) and back by UNBOXING with a runtime type check. `using` blocks
 * call Dispose() deterministically at scope exit.
 *
 * SELF-CONTAINED: duplicates a small console prelude rather than sharing one.
 *
 * C# defers many checks to run time. Unlike the Rust/C++ headers (which reject
 * bad programs at generation time), the headline C# check here - an invalid
 * unbox or downcast raising InvalidCastException - is a runtime branch on the
 * object's type word, exactly as the CLR does.
 *
 * See csharp_interface.md for the full narrative.
 */


/* ---- memory operand helper: "qword ptr [base+off]" ---- */
/* Arena copy of "qword ptr [base]", or "qword ptr [base+off]" when off is nonzero. */
static inline const char* cs_mem(const char* base, int off) {
    char buf[48];
    if (off == 0) snprintf(buf, sizeof(buf), "qword ptr [%s]", base);
    else          snprintf(buf, sizeof(buf), "qword ptr [%s+%d]", base, off);
    return c_strdup_arena(buf);
}

/* ======================================================================== *
 *  Console runtime  (self-contained)
 * ======================================================================== */

typedef struct {
    AsmExtern get_std_handle;
    AsmExtern write_file;
    AsmExtern exit_process;
} CsConsole;

typedef struct { AsmDataField off; AsmDataField len; } CsStr;

/* Declare GetStdHandle, WriteFile, and ExitProcess, and return that console. */
static inline CsConsole cs_console_declare(void) {
    CsConsole c;
    static const char* gsh[] = { "nStdHandle" };
    c.get_std_handle = c_extern("GetStdHandle", gsh, 1, C_PTR, "handle");
    static const char* wf[] = { "handle", "lpBuffer", "nBytes", "lpWritten", "lpOverlapped" };
    c.write_file = c_extern("WriteFile", wf, 5, C_INT, "ok");
    static const char* ep[] = { "exit_code" };
    c.exit_process = c_extern("ExitProcess", ep, 1, C_PTR, NULL);
    return c;
}

/* Emit the uninitialised written cell that WriteFile's lpWritten points at. */
static inline void cs_console_globals(void) {
    c_ensure_bss();
    write_unitialised_data("written", "?");
}

/* Define string name from text_literal and return its offset and length cells. */
static inline CsStr cs_string(const char* name, const char* text_literal) {
    c_ensure_data();
    char offname[64], lenname[64], lenexpr[80];
    snprintf(offname, sizeof(offname), "off_%s", name);
    snprintf(lenname, sizeof(lenname), "len_%s", name);
    AsmDataField d = write_initialised_data(name, text_literal, "lpBuffer");
    CsStr s;
    s.off = write_offset_constant(c_strdup_arena(offname), d, "lpBuffer");
    snprintf(lenexpr, sizeof(lenexpr), "$ - %s", name);
    s.len = write_constant(c_strdup_arena(lenname), c_strdup_arena(lenexpr), "nBytes");
    return s;
}

static inline CsStr cs_line(const char* name, const char* text) {
    return cs_string(name, c_db_text(text, 2, 0));
}

/* Emit print from the externs in c (GetStdHandle, then WriteFile) and return it. */
static inline AsmExtern cs_console_print(CsConsole* c) {
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

/* Print string s through print. c_call_proc_void saves named live registers
 * across the call. */
static inline void cs_puts(AsmExtern* print, CsStr s) {
    c_call_proc_void(print, s.off, s.len);
}
/* Call ExitProcess through c with the immediate code. */
static inline void cs_exit(CsConsole* c, const char* code) {
    c_call_ext_void(&c->exit_process, literal(code, "exit_code"));
}

/* ======================================================================== *
 *  Indirect calls / aligned frames / MethodTable dispatch
 * ======================================================================== */

/* Reserve 40 bytes of shadow space. */
static inline void cs_frame_open(void)  { write_sub((AsmDataField){"rsp","rsp",NULL}, literal("40","frame")); }
/* Release 40 bytes of shadow space. */
static inline void cs_frame_close(void) { write_add((AsmDataField){"rsp","rsp",NULL}, literal("40","frame")); }

/* Emit an indirect call through register reg. */
static inline void cs_call_reg(const char* reg) {
    AsmExtern ind = { reg, NULL, 0, asm_void };
    write_call(&ind);
}

/* ======================================================================== *
 *  Classes: definition builder, MethodTables, virtual dispatch
 *
 *  A class is described between cs_class_begin(name) and cs_class_end(): declare
 *  data fields (cs_field) and members - ordinary (cs_method_begin) or virtual
 *  (cs_virtual_begin, which occupies the next MethodTable slot). cs_class_end
 *  bakes "Class$vtable dq <virtual methods...>" - the CLR's per-type
 *  MethodTable - into .data and returns a handle used to create instances and
 *  make (implicit-`this`) calls. The class builder writes both the table and
 *  the slot numbers.
 * ======================================================================== */

/* Fixed tables. The caps can be raised or made dynamic; 80 is enough for now.
 * Generation state is single-threaded. */
#define CS_MAX_CLASSES 80
#define CS_MAX_MEMBERS 80

/* Payload starts after the collector's two-word header (descriptor, meta). */
#define CS_HDR 16
#ifdef GC_SELECTED
typedef char cs_hdr_matches_gc[(CS_HDR == GC_HDR) ? 1 : -1];
#endif

typedef enum { CS_NONVIRTUAL, CS_VIRTUAL } CsMemberKind;

typedef struct {
    char         name[40];                        /* class name, e.g. "Dog"   */
    CFunc        method[CS_MAX_MEMBERS];           /* stable storage: bodies    */
    char         methodName[CS_MAX_MEMBERS][40];
    int          slot[CS_MAX_MEMBERS];             /* MethodTable slot, or -1   */
    int          nmethods;
    int          nslots;
    char         fieldName[CS_MAX_MEMBERS][40];    /* fields after the MT ptr   */
    int          nfields;
    bool         isInterface;                      /* abstract: no MT emitted   */
    const char*  vtsym[CS_MAX_MEMBERS];            /* inherited slot symbol, or NULL */
#ifdef GC_SELECTED
    GcType       gc;                               /* instance descriptor       */
    bool         has_gc;
#endif
} CsClassRec;

static CsClassRec  cs_classes[CS_MAX_CLASSES];
static int         cs_nclass = 0;
static CsClassRec* cs_cur = NULL;
static int         cs_cur_method = -1;

typedef struct { CsClassRec* rec; } CsClass;

/* A constructed instance: the GC pointer plus its class record. Returned by
 * cs_construct. `ptr` is the object base (collector header at offset 0). */
typedef struct { CValue ptr; CsClassRec* rec; } CsObject;

/* '$' joins cls and member. Method names collide across classes, so this is
 * the one place names are concatenated: Class$method, Class$vtable. */
static inline const char* cs_mangle(const char* cls, const char* member) {
    crash_if(!cls || !member, "csharp: missing name");
    crash_if(strchr(cls, '$') != NULL || strchr(member, '$') != NULL,
             "csharp: '$' is reserved for mangling");
    char buf[96];
    int n = snprintf(buf, sizeof buf, "%s$%s", cls, member);
    crash_if(n < 0 || n >= (int)sizeof buf, "csharp: mangled name too long");
    return c_strdup_arena(buf);
}

/* Copy name into dst, which holds cap bytes. An empty name, '$', or
 * overflow stops generation. */
static inline void cs_copy_name(char* dst, int cap, const char* name) {
    crash_if(!name || !name[0], "csharp: empty name");
    crash_if(strchr(name, '$') != NULL, "csharp: '$' is reserved for mangling");
    int n = snprintf(dst, (size_t)cap, "%s", name);
    crash_if(n < 0 || n >= cap, "csharp: name too long");
}

/* A register-resident obj is used as-is. A memory root (compacting
 * collector slot, or a spilled `this`) is reloaded into scratch. */
static inline const char* cs_ptr(CValue obj, const char* scratch) {
    obj = c_resolve(obj);
    crash_if(!obj.loc, "csharp: missing object pointer");
    if (obj.reg >= 0) return obj.loc;
    write_replace_mov(scratch, (AsmDataField){ obj.loc, obj.tag ? obj.tag : "obj", NULL });
    return scratch;
}

/* Stop generation with msg, prefixed by "csharp_funcs: ". */
static inline void cs_fail(const char* msg) {
    char buf[256];
    snprintf(buf, sizeof buf, "csharp_funcs: %s", msg);
    host_fail(buf);
}

/* Open a member `Class$method` with an implicit `this` in rcx (param 0).
 * Records its kind/slot and returns a stable CFunc* for the body (close with
 * cs_method_end). c_begin_function tracks `this`. A call saves that register
 * and puts it back. If the allocator runs out of registers it evicts the
 * tracked copy, and c_param reads that copy, so the caller does not register
 * a stable address. */
static inline CFunc* cs_member_open(const char* method, CsMemberKind kind) {
    if (!cs_cur) cs_fail("member declared outside a class");
    if (cs_cur->nmethods >= CS_MAX_MEMBERS) cs_fail("too many members");
    int i = cs_cur->nmethods++;
    cs_copy_name(cs_cur->methodName[i], (int)sizeof cs_cur->methodName[i], method);
    cs_cur->slot[i] = (kind == CS_VIRTUAL) ? cs_cur->nslots++ : -1;
    const CParam p[] = { {"this", C_PTR} };
    cs_cur->method[i] = c_begin_function(cs_mangle(cs_cur->name, method), p, 1, C_PTR, NULL);
    cs_cur_method = i;
    return &cs_cur->method[i];
}

/* Open member name as an ordinary (non-virtual) method. */
static inline CFunc* cs_method_begin(const char* name)  { return cs_member_open(name, CS_NONVIRTUAL); }

/* Open member name as a virtual method. A slot copied by cs_extends is
 * reused, so an override keeps the base slot number. A new name takes the
 * next slot. */
static inline CFunc* cs_virtual_begin(const char* name) {
    if (cs_cur) {
        for (int i = 0; i < cs_cur->nmethods; i++) {
            if (cs_cur->slot[i] >= 0 && strcmp(cs_cur->methodName[i], name) == 0) {
                const CParam p[] = { {"this", C_PTR} };
                cs_cur->method[i] = c_begin_function(cs_mangle(cs_cur->name, name), p, 1, C_PTR, NULL);
                cs_cur->vtsym[i] = NULL;
                cs_cur_method = i;
                return &cs_cur->method[i];
            }
        }
    }
    return cs_member_open(name, CS_VIRTUAL);
}

/* Close the open member: emit endp and clear the current-method index. */
static inline void cs_method_end(void) {
    if (!cs_cur || cs_cur_method < 0) cs_fail("cs_method_end without an open member");
    c_end_function(&cs_cur->method[cs_cur_method]);
    cs_cur_method = -1;
}

/* The receiver of the member being defined (implicit `this`). */
static inline CValue cs_this(void) {
    if (!cs_cur || cs_cur_method < 0) cs_fail("cs_this outside a member body");
    return c_param(&cs_cur->method[cs_cur_method], 0);
}

/* Open class name and make it the current class being built. */
static inline void cs_class_begin(const char* name) {
    if (cs_nclass >= CS_MAX_CLASSES) cs_fail("too many classes");
    cs_cur = &cs_classes[cs_nclass++];
    memset(cs_cur, 0, sizeof *cs_cur);
    cs_copy_name(cs_cur->name, (int)sizeof cs_cur->name, name);
}

/* Copy base's virtual slots onto the current class. An override reuses the
 * slot; a slot that is not overridden keeps the base method symbol. */
static inline void cs_extends(CsClass* base) {
    if (!cs_cur || !base || !base->rec) cs_fail("cs_extends outside a class");
    CsClassRec* b = base->rec;
    for (int i = 0; i < b->nmethods; i++) {
        if (b->slot[i] < 0) continue;
        if (cs_cur->nmethods >= CS_MAX_MEMBERS) cs_fail("too many members");
        int n = cs_cur->nmethods++;
        cs_copy_name(cs_cur->methodName[n], (int)sizeof cs_cur->methodName[n], b->methodName[i]);
        cs_cur->slot[n] = b->slot[i];
        cs_cur->vtsym[n] = cs_mangle(b->name, b->methodName[i]);
        if (b->slot[i] + 1 > cs_cur->nslots) cs_cur->nslots = b->slot[i] + 1;
    }
}

/* Mark the class abstract: declares virtual slots (cs_virtual_decl) but emits no
 * MethodTable - an interface that only fixes slot numbering. */
static inline void cs_class_interface(void) {
    if (!cs_cur) cs_fail("cs_class_interface outside a class");
    cs_cur->isInterface = true;
}

/* Add virtual member name on the current class at the next MethodTable slot.
 * The procedure symbol stays unset. */
static inline void cs_virtual_decl(const char* name) {
    if (!cs_cur) cs_fail("cs_virtual_decl outside a class");
    if (cs_cur->nmethods >= CS_MAX_MEMBERS) cs_fail("too many members");
    int i = cs_cur->nmethods++;
    cs_copy_name(cs_cur->methodName[i], (int)sizeof cs_cur->methodName[i], name);
    cs_cur->slot[i] = cs_cur->nslots++;
    cs_cur->method[i].ext.name = NULL;
}

/* Declare field name on the current class. Each field is one qword. */
static inline void cs_field(const char* name) {
    if (!cs_cur) cs_fail("cs_field outside a class");
    if (cs_cur->nfields >= CS_MAX_MEMBERS) cs_fail("too many fields");
    cs_copy_name(cs_cur->fieldName[cs_cur->nfields], 40, name);
    cs_cur->nfields++;
}

/* MethodTable symbol of class r ("Class$vtable"). */
static inline const char* cs_mt_sym(CsClassRec* r) {
    return cs_mangle(r->name, "vtable");
}

/* Mangled symbol for method i of class r. */
static inline const char* cs_method_sym(CsClassRec* r, int i) {
    return cs_mangle(r->name, r->methodName[i]);
}

/* Finish the class: bake its MethodTable into .data (one qword per virtual slot)
 * and, when the collector headers are visible, its instance descriptor. .data
 * from within .code is safe (MASM merges segments). */
static inline CsClass cs_class_end(void) {
    if (!cs_cur) cs_fail("cs_class_end without an open class");
    CsClassRec* r = cs_cur;
    if (!r->isInterface) {
        write_data();
            if (r->nslots > 0) {
                const char* mt = cs_mt_sym(r);
                for (int s = 0; s < r->nslots; s++) {
                    const char* msym = NULL;
                    for (int i = 0; i < r->nmethods; i++) {
                        if (r->slot[i] == s) {
                            msym = r->vtsym[i] ? r->vtsym[i] : cs_method_sym(r, i);
                            break;
                        }
                    }
                    if (!msym) cs_fail("MethodTable slot has no method");
                    if (s == 0) c_ptr_data(mt, msym, "methodtable");
                    else        write_dq("", c_offset(msym));
                }
            }
#ifdef GC_SELECTED
            {
                int size = CS_HDR + (r->nslots > 0 ? 8 : 0) + r->nfields * 8;
                r->gc = gc_typedesc(r->name, size, NULL, 0);
                r->has_gc = true;
            }
#endif
        write_code();
    }
    cs_cur = NULL;
    cs_cur_method = -1;
    CsClass h; h.rec = r; return h;
}

/* ---- Object layout ----------------------------------------------------- */

/* Does the class carry a MethodTable pointer? Concrete classes with at least
 * one virtual slot do; plain classes and interfaces do not. On the heap the
 * pointer sits at CS_HDR, the first payload qword. */
static inline bool cs_has_mt(CsClassRec* r) {
    return !r->isInterface && r->nslots > 0;
}

/* Byte offset where fields of r begin: CS_HDR, plus 8 when r has a MethodTable. */
static inline int cs_fields_base(CsClassRec* r) {
    return CS_HDR + (cs_has_mt(r) ? 8 : 0);
}

/* Byte offset of field `name`: fields follow the MethodTable pointer (if any)
 * in declaration order, one qword each, after the collector header. */
static inline int cs_field_offset(CsClassRec* r, const char* name) {
    int base = cs_fields_base(r);
    for (int i = 0; i < r->nfields; i++)
        if (strcmp(r->fieldName[i], name) == 0) return base + i * 8;
    cs_fail("cs_field_offset: no such field");
    return 0;
}

/* Immediate store of imm at byte off of obj, with tag.
 * `mov r/m64, imm32` covers small constants (0, type ids). */
static inline void cs_store_imm(CValue obj, int off, const char* imm, const char* tag) {
    const char* p = cs_ptr(obj, "rdx");
    write_replace_mov(cs_mem(p, off), (AsmDataField){ imm, tag, NULL });
}

/* Store symbol at byte off of obj, with tag. A symbol address is wider
 * than an imm32 store to memory, so stage it in rax. */
static inline void cs_store_symbol(CValue obj, int off, const char* symbol, const char* tag) {
    const char* p = cs_ptr(obj, "rdx");
    write_replace_mov("rax", (AsmDataField){ c_offset(symbol), tag, NULL });
    write_replace_mov(cs_mem(p, off), (AsmDataField){ "rax", tag, NULL });
}

#ifdef GC_SELECTED
/*
 * cs_construct - allocate a reference-type instance on the GC heap and build
 * it in that memory. Call from inside a function, after gc_start. gc_new is
 * the same allocator the collector tour uses. The MethodTable pointer (when
 * the class has one) is written at CS_HDR; fields are zeroed. A constructor,
 * if the class models one, is an ordinary member called on the handle.
 * Fields are scalar words, so the descriptor lists no traced pointers.
 */
static inline CsObject cs_construct(GcHeap* heap, CsClass* cls, const char* name) {
    CsClassRec* r = cls->rec;
    crash_if(!r || !r->has_gc, "csharp: class has no GC descriptor");
    crash_if(strchr(name, '$') != NULL, "csharp: '$' is reserved for mangling");
    CValue obj = gc_new(heap, &r->gc, name);
    if (cs_has_mt(r)) cs_store_symbol(obj, CS_HDR, cs_mt_sym(r), "methodtable");
    for (int i = 0; i < r->nfields; i++)
        cs_store_imm(obj, cs_fields_base(r) + i * 8, "0", r->fieldName[i]);
    CsObject o; o.ptr = obj; o.rec = r; return o;
}
#endif

/* ---- Member-field access ----------------------------------------------- */

/* `this->name` inside a member body, wrapped as type. Reloads `this` if a
 * call spilled it. */
static inline CValue cs_this_field(const char* name, CType type) {
    if (!cs_cur || cs_cur_method < 0) cs_fail("cs_this_field outside a member body");
    CValue self = cs_this();
    int off = cs_field_offset(cs_cur, name);
    const char* p = cs_ptr(self, "rax");
    AsmDataField f = { cs_mem(p, off), name, NULL };
    return c_wrap(f, type);
}

/* `obj->name` of object o from outside, through the instance pointer, wrapped as type. */
static inline CValue cs_field_ref(CsObject* o, const char* name, CType type) {
    int off = cs_field_offset(o->rec, name);
    const char* p = cs_ptr(o->ptr, "rax");
    AsmDataField f = { cs_mem(p, off), name, NULL };
    return c_wrap(f, type);
}

/* Store an immediate into a (memory-resident) field cell. */
static inline void cs_field_store_imm(CValue field, const char* imm) {
    write_replace_mov(field.loc, (AsmDataField){ imm, field.tag, NULL });
}

/* ---- Calls with an implicit `this` -------------------------------------- */

/* Operand the call checks as "this". A register-resident object keeps the
 * tag gc_emit_new recorded ("obj", or the value's own name after a reload).
 * treat_as crosses that tag to the parameter tag without relabelling the
 * register: the bits are still the object. An untracked location is already
 * acceptable to assert_type, so it is handed over as "this" directly. */
static inline AsmDataField cs_this_arg(CValue obj) {
    const char* p = cs_ptr(obj, "r10");
    const char* have = asm_type_lookup(p);
    if (have == NULL) {
        AsmDataField a = { p, "this", NULL };
        return a;
    }
    AsmDataField raw = { p, have, NULL };
    return treat_as(raw, have, "this");
}

/* Static (non-virtual) call of `method` on object `obj`: supplies the receiver
 * as the implicit `this` argument. */
static inline void cs_send(CFunc* method, CsObject obj) {
    c_call(method, cs_this_arg(obj.ptr));
}

/*
 * cs_vsend - virtual/interface call of `method` on `recv`, through the object's
 * MethodTable. Resolves the slot via `iface` (the class/interface that fixes the
 * numbering), loads the MethodTable from [this+CS_HDR], the method from
 * [mt+slot*8], and CALLs it with `this` in rcx. The concrete override is
 * chosen by the object's type: runtime polymorphism.
 */
static inline void cs_vsend(CValue recv, CsClass* iface, const char* method) {
    int slot = -1;
    for (int i = 0; i < iface->rec->nmethods; i++) {
        if (iface->rec->slot[i] >= 0 &&
            strcmp(iface->rec->methodName[i], method) == 0) { slot = iface->rec->slot[i]; break; }
    }
    if (slot < 0) cs_fail("cs_vsend: no such virtual method");
    const char* p = cs_ptr(recv, "rcx");
    if (strcmp(p, "rcx") != 0) write_replace_mov("rcx", (AsmDataField){ p, "this", NULL });
    write_replace_mov("rax", (AsmDataField){ cs_mem("rcx", CS_HDR), "methodtable", NULL });
    write_replace_mov("rax", (AsmDataField){ cs_mem("rax", slot * 8), "method", NULL });
    cs_frame_open();
    cs_call_reg("rax");
    cs_frame_close();
}

/* ======================================================================== *
 *  using / IDisposable
 *
 *  cs_using_end emits Dispose in reverse order of cs_using_own, on the path
 *  that reaches it. That is the deterministic half of a C# using block.
 *  A CLR using is a try/finally, so Dispose also runs on return and on a
 *  thrown exception, and a finalizer is the backup if the user never
 *  disposed. This layer does not emit a finally, a finalizer, or
 *  SuppressFinalize. The object's memory stays the collector's job. Dispose
 *  is an ordinary method for a resource the collector does not know about.
 * ======================================================================== */

#define CS_USING_MAX 16
typedef struct {
    const AsmExtern* disposers[CS_USING_MAX];
    CValue           objs[CS_USING_MAX];
    int              n;
} CsUsing;

/* Start an empty using scope. Dispose calls are added with cs_using_own. */
static inline CsUsing cs_using_begin(void) { CsUsing u; u.n = 0; return u; }
/* Register obj on u so its Dispose() runs at scope end. dispose is the
 * Dispose procedure (its `.ext` is taken internally). */
static inline void cs_using_own(CsUsing* u, const CFunc* dispose, CsObject obj) {
    crash_if(u->n >= CS_USING_MAX, "csharp: too many usings");
    u->disposers[u->n] = &dispose->ext;
    u->objs[u->n] = obj.ptr;
    u->n++;
}
/* Emit Dispose for each object registered on u, in reverse order. */
static inline void cs_using_end(CsUsing* u) {
    for (int i = u->n - 1; i >= 0; i--)
        c_call_proc_void(u->disposers[i], cs_this_arg(u->objs[i]));
}

/* ======================================================================== *
 *  Runtime type check (the unbox / InvalidCastException path)
 * ======================================================================== */

/*
 * cs_check_type - the one runtime cast. Loads the type word at [obj+off] and
 * branches to the else arm when it is not `expected`. Unbox and downcast both
 * come through here; there is no second copy of the compare. `expected` is an
 * assemble-time operand (a type-handle equate, or "offset Class$vtable").
 * A 64-bit address is staged in rdx: cmp does not take an imm64.
 *
 * Use exactly like c_if_*_else: the then-arm is the match.
 */
static inline CElse cs_check_type(CValue obj, int off, const char* expected) {
    CElse c;
    memset(&c, 0, sizeof c);
    c.else_label = c_new_label();
    c.done_label = c_new_label();
    const char* p = cs_ptr(obj, "rcx");
    write_replace_mov("rax", (AsmDataField){ cs_mem(p, off), "typehandle", NULL });
    write_replace_mov("rdx", (AsmDataField){ expected, "typehandle", NULL });
    write_if_not_equal_then_jump_to(c.else_label,
        (AsmDataField){"rax", "typehandle", NULL},
        (AsmDataField){"rdx", "typehandle", NULL});
    c.entry = c_clobber_save();
    c.id = c_block_push(c_else_stack, &c_else_sp);
    return c;
}

/* Unbox box. The box's first payload qword is its type handle, checked
 * against expected_type. */
static inline CElse cs_unbox_begin(CValue box, const char* expected_type) {
    return cs_check_type(box, CS_HDR, expected_type);
}

/* Downcast obj to target. The MethodTable pointer is the class's runtime type. */
static inline CElse cs_downcast_begin(CValue obj, CsClass* target) {
    crash_if(!target || !cs_has_mt(target->rec), "csharp: downcast target has no MethodTable");
    return cs_check_type(obj, CS_HDR, c_offset(cs_mt_sym(target->rec)));
}

/* ======================================================================== *
 *  CLI baked-ins (type handles, the Node descriptor, heap boxes)
 * ======================================================================== */

#ifdef GC_SELECTED
/* Type-handle equates plus the Node and Box GC descriptors.
 * Call from the initialised data section. */
typedef struct { GcType node; GcType box; } CsCli;

/* Emit Int32_type and String_type plus the Node and Box GC descriptors.
 * Call from the initialised data section. */
static inline CsCli cs_cli_init(void) {
    write_constant("Int32_type", "1", "typehandle");
    write_constant("String_type", "2", "typehandle");
    CsCli cli;
    GcTypeDesc nodeDesc = gc_type_begin("Node");
        gc_field_ptr(&nodeDesc, "next");
        gc_field_word(&nodeDesc, "value");
    cli.node = gc_type_end(&nodeDesc);
    GcTypeDesc boxDesc = gc_type_begin("Box");
        gc_field_word(&boxDesc, "typehandle");
        gc_field_word(&boxDesc, "value");
    cli.box = gc_type_end(&boxDesc);
    return cli;
}

/* A boxed value named name is a heap object of boxType on heap: type_handle
 * at CS_HDR, payload value at CS_HDR+8. Call from inside a function, after
 * gc_start. */
static inline CValue cs_box(GcHeap* heap, const GcType* boxType,
                            const char* type_handle, long long value,
                            const char* name) {
    crash_if(!name, "csharp: missing name");
    crash_if(strchr(name, '$') != NULL, "csharp: '$' is reserved for mangling");
    CValue obj = gc_new(heap, boxType, name);
    cs_store_imm(obj, CS_HDR, type_handle, "typehandle");
    cs_store_imm(obj, CS_HDR + 8, c_dec(value), "boxed");
    return obj;
}

/* The boxed payload as a value. The header size stays in this file. */
static inline CValue cs_box_payload(CValue box) {
    const char* p = cs_ptr(box, "rax");
    AsmDataField cell = { cs_mem(p, CS_HDR + 8), "boxed", NULL };
    return c_dup("boxed", C_LONG, c_wrap(cell, C_LONG));
}
#endif

/* ======================================================================== *
 *  Initialisation
 * ======================================================================== */

/*
 * cs_init - call once at the top of main. Selects case-sensitive identifiers
 * (the CLR is case-sensitive and Win32 externs are mixed-case) and resets the
 * class-builder state so a fresh program starts from an empty class table.
 */
static inline void cs_init(void) {
    write_set_case_sensitive_identifiers();
    cs_nclass = 0;
    cs_cur = NULL;
    cs_cur_method = -1;
}

