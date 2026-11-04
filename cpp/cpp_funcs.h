/*
 * cpp_funcs.h - A C++-flavoured code-generation layer over c_funcs.h.
 *
 * Classes, virtual dispatch, RAII, operator calls, and monomorphised
 * functions. The feature-tour console (GetStdHandle / WriteFile / ExitProcess)
 * lives here with the class layer, the same split the other language headers use.
 * Heap allocation lives in gc/. Objects here are globals, a placement into
 * an existing global, or a stack reservation.
 *
 * Method symbols are Class$method. Two classes may both define draw, and
 * MASM's namespace is flat, so the generator joins them with '$'. '$' is a
 * legal MASM identifier character and is rejected in class, method, and field
 * names at this layer, so a mangled symbol cannot collide with one the user wrote.
 * The same helper builds Class$vtable.
 *
 * An interface fixes slot numbers. cpp_implements adopts that layout. Each
 * implemented interface gets its own vptr (the first at offset 0) and its own
 * vtable. cpp_vsend loads the vptr at [this+0], so the receiver must already
 * point at that interface's subobject. cpp_vsend_obj does the adjustment.
 *
 * See cpp_interface.md.
 */

#define CPP_SCOPE_MAX    16
#define CPP_SCOPE_DEPTH  8
/* Fixed tables are enough for a generation unit. Raise the constant, or replace
 * the arrays, if a unit needs more. Nothing here grows them at runtime. */
#define CPP_MAX_CLASSES  100
#define CPP_MAX_MEMBERS  80
#define CPP_MAX_IFACES   4

typedef enum {
    CPP_NONVIRTUAL,
    CPP_VIRTUAL,
    CPP_CTOR,
    CPP_DTOR
} CppMemberKind;

typedef struct CppClassRec CppClassRec;
struct CppClassRec {
    char          name[40];
    CFunc         method[CPP_MAX_MEMBERS];
    char          methodName[CPP_MAX_MEMBERS][40];
    CppMemberKind kind[CPP_MAX_MEMBERS];
    int           slot[CPP_MAX_MEMBERS];     /* index in that interface's vtable, or -1 */
    int           iface_of[CPP_MAX_MEMBERS]; /* which implemented interface, or 0 for an own vtable */
    int           nmethods;
    int           own_slots;                 /* virtuals with no interface */
    int           ctorIdx, dtorIdx;
    char          fieldName[CPP_MAX_MEMBERS][40];
    int           fieldSize[CPP_MAX_MEMBERS];
    int           fieldAlign[CPP_MAX_MEMBERS]; /* AlignMode, or -1 for align_get_default() */
    int           fieldOff[CPP_MAX_MEMBERS];
    int           nfields;
    int           next_off;                  /* next byte after the header and fields */
    int           laid_out;                  /* a field has been placed; header is frozen */
    bool          isInterface;
    bool          has_own;                   /* virtuals, and no cpp_implements */
    CppClassRec*  iface[CPP_MAX_IFACES];
    int           niface;
};

static CppClassRec  cpp_classes[CPP_MAX_CLASSES];
static int          cpp_nclass = 0;
static CppClassRec* cpp_cur = NULL;
static int          cpp_cur_method = -1;

typedef struct { CppClassRec* rec; } CppClass;

/* sym is the data label of a global instance. NULL means the object lives at
 * [rsp] until its scope releases the reservation. */
typedef struct { const char* sym; CppClassRec* rec; } CppObject;

typedef struct {
    const AsmExtern* dtors[CPP_SCOPE_MAX];
    const char*      objs[CPP_SCOPE_MAX];   /* global label, or NULL for [rsp] */
    int              release[CPP_SCOPE_MAX]; /* bytes added back to rsp after the dtor */
    int              n;
    bool             open;
} CppScopeData;

static CppScopeData cpp_scope_pool[CPP_SCOPE_DEPTH];
static int          cpp_scope_stack[CPP_SCOPE_DEPTH];
static int          cpp_scope_depth = 0;

typedef struct { int id; } CppScope;

/* Stop generation and report msg. */
static inline void cpp_fail(const char* msg) {
    crash_if(1, msg);
}

/* Reject a null or empty name, or a name that contains '$', which mangling reserves. */
static inline void cpp_check_ident(const char* name) {
    crash_if(name == NULL || name[0] == 0 || strchr(name, '$') != NULL,
             "cpp: '$' is reserved for mangling");
}

/* Join cls and member as Class$method, or Class$vtable when member is "vtable". */
static inline const char* cpp_mangle(const char* cls, const char* member) {
    cpp_check_ident(cls);
    cpp_check_ident(member);
    char buf[96];
    int n = snprintf(buf, sizeof buf, "%s$%s", cls, member);
    crash_if(n < 0 || n >= (int)sizeof buf, "cpp: mangled name too long");
    return c_strdup_arena(buf);
}

/* Bytes reserved for vptrs at the front of class record r. */
static inline int cpp_header_bytes(CppClassRec* r) {
    if (r->isInterface) return 0;
    if (r->niface > 0) return r->niface * 8;
    if (r->has_own) return 8;
    return 0;
}

/* Instance size in bytes for class record r, at least 8 when the object is empty. */
static inline int cpp_object_bytes(CppClassRec* r) {
    int header = cpp_header_bytes(r);
    int end = r->laid_out ? r->next_off : header;
    if (end < header) end = header;
    if (end == 0) end = 8;
    return end;
}

/* Symbol of vtable vi on class record r: the primary table, or a secondary interface table. */
static inline const char* cpp_vtable_name(CppClassRec* r, int vi) {
    if (vi == 0) return cpp_mangle(r->name, "vtable");
    crash_if(r->iface[vi] == NULL, "cpp: secondary vtable without an interface");
    char buf[96];
    int n = snprintf(buf, sizeof buf, "%s$%s$vtable", r->name, r->iface[vi]->name);
    crash_if(n < 0 || n >= (int)sizeof buf, "cpp: vtable name too long");
    return c_strdup_arena(buf);
}

/* Byte offset of iface inside obj, one vptr stride per implemented interface. */
static inline int cpp_iface_offset(CppClassRec* obj, CppClassRec* iface) {
    for (int i = 0; i < obj->niface; i++)
        if (obj->iface[i] == iface) return i * 8;
    cpp_fail("cpp_vsend_obj: class does not implement that interface");
    return 0;
}

/* "qword ptr [base]" / "qword ptr [base+off]". Kept in this shape: cpp_tests.h
 * checks these two spellings. */
static inline const char* cpp_mem(const char* base, int off) {
    char buf[48];
    if (off == 0) snprintf(buf, sizeof buf, "qword ptr [%s]", base);
    else          snprintf(buf, sizeof buf, "qword ptr [%s+%d]", base, off);
    return c_strdup_arena(buf);
}

/* Sized memory operand at base plus off, with the ptr keyword for size. */
static inline const char* cpp_mem_sized(const char* base, int off, int size) {
    const char* kw = size == 1 ? "byte ptr" : size == 2 ? "word ptr"
                   : size == 4 ? "dword ptr" : "qword ptr";
    crash_if(size != 1 && size != 2 && size != 4 && size != 8, "cpp: bad field size");
    char buf[64];
    if (off == 0) snprintf(buf, sizeof buf, "%s [%s]", kw, base);
    else          snprintf(buf, sizeof buf, "%s [%s+%d]", kw, base, off);
    return c_strdup_arena(buf);
}

/* Take a free scope slot, push it, and return its id. */
static inline int cpp_scope_push(void) {
    if (cpp_scope_depth >= CPP_SCOPE_DEPTH) cpp_fail("cpp: scope nesting too deep");
    int id = -1;
    for (int i = 0; i < CPP_SCOPE_DEPTH; i++) {
        if (!cpp_scope_pool[i].open) { id = i; break; }
    }
    if (id < 0) cpp_fail("cpp: no free scope slot");
    cpp_scope_pool[id].n = 0;
    cpp_scope_pool[id].open = true;
    cpp_scope_stack[cpp_scope_depth++] = id;
    return id;
}

/* Close scope id in LIFO order, running its destructors and releasing stack bytes. */
static inline void cpp_scope_pop(int id) {
    if (cpp_scope_depth == 0 || cpp_scope_stack[cpp_scope_depth - 1] != id)
        cpp_fail("cpp: scope closed out of LIFO order");
    CppScopeData* sc = &cpp_scope_pool[id];
    for (int i = sc->n - 1; i >= 0; i--) {
        if (sc->dtors[i] != NULL) {
            if (sc->objs[i] != NULL) {
                write_call(sc->dtors[i], c_addr(sc->objs[i], "this"));
            } else {
                write_lea("rcx", "[rsp]");
                asm_type_dict_set("rcx", "this");
                write_call(sc->dtors[i], (AsmDataField){ "rcx", "this", NULL });
            }
        }
        if (sc->release[i] != 0) {
            const char* tok = asm_frame_token(sc->release[i]);
            write_add((AsmDataField){ "rsp", "rsp", NULL }, literal(tok, "frame"));
            asm_note_rsp_add(sc->release[i]);
        }
    }
    sc->open = false;
    cpp_scope_depth--;
}

/* Open a nested scope and indent the generated body. */
static inline CppScope cpp_scope_begin(void) {
    CppScope s;
    s.id = cpp_scope_push();
    asm_indent_level++;
    return s;
}

/* Close scope s and outdent the generated body. */
static inline void cpp_scope_end(CppScope* s) {
    cpp_scope_pop(s->id);
    asm_indent_level--;
}

/* Open procedure name with params (nparam of them), return type ret,
 * and ret_tag, and push a scope for destructors. */
static inline CFunc cpp_begin_function(const char* name, const CParam* params,
                                       int nparam, CType ret, const char* ret_tag) {
    CFunc f = c_begin_function(name, params, nparam, ret, ret_tag);
    cpp_scope_push();
    return f;
}

/* Pop the function scope, run its destructors, and close procedure f. */
static inline void cpp_end_function(CFunc* f) {
    if (cpp_scope_depth == 0) cpp_fail("cpp_end_function without a function scope");
    cpp_scope_pop(cpp_scope_stack[cpp_scope_depth - 1]);
    c_end_function(f);
}

/* A Windows x64 CALL needs rsp 0 mod 16 and 32 bytes of home space, where
 * the callee may spill rcx, rdx, r8, and r9. On entry rsp is 8 mod 16, so
 * the smallest such reservation is 40. asm_call_frame_size returns that, or
 * 32 when an earlier adjustment already aligned rsp, so the frame is not
 * padded twice. write_call itself does not reserve it. */
static int cpp_frame_bytes = 0;

/* Reserve the Windows call frame and remember its size. */
static inline void cpp_frame_open(void) {
    int frame = asm_call_frame_size(32);
    cpp_frame_bytes = frame;
    const char* tok = asm_frame_token(frame);
    write_sub((AsmDataField){ "rsp", "rsp", NULL }, literal(tok, "frame"));
    asm_note_rsp_sub(frame);
}

/* Add the reserved call frame back onto rsp. */
static inline void cpp_frame_close(void) {
    const char* tok = asm_frame_token(cpp_frame_bytes);
    write_add((AsmDataField){ "rsp", "rsp", NULL }, literal(tok, "frame"));
    asm_note_rsp_add(cpp_frame_bytes);
}

/* Call the address held in reg. */
static inline void cpp_call_reg(const char* reg) {
    AsmExtern ind = { reg, NULL, 0, asm_void };
    write_call(&ind);
}

/* Start class name and make it the current class. */
static inline void cpp_class_begin(const char* name) {
    cpp_check_ident(name);
    if (cpp_nclass >= CPP_MAX_CLASSES) cpp_fail("cpp: too many classes");
    cpp_cur = &cpp_classes[cpp_nclass++];
    memset(cpp_cur, 0, sizeof *cpp_cur);
    snprintf(cpp_cur->name, sizeof cpp_cur->name, "%s", name);
    cpp_cur->ctorIdx = cpp_cur->dtorIdx = -1;
}

/* Mark the current class as an interface. */
static inline void cpp_class_interface(void) {
    if (!cpp_cur) cpp_fail("cpp_class_interface outside a class");
    cpp_cur->isInterface = true;
}

/* Declare virtual name on the current class and assign the next own slot. */
static inline void cpp_virtual_decl(const char* name) {
    cpp_check_ident(name);
    if (!cpp_cur) cpp_fail("cpp_virtual_decl outside a class");
    if (cpp_cur->nmethods >= CPP_MAX_MEMBERS) cpp_fail("cpp: too many members");
    int i = cpp_cur->nmethods++;
    snprintf(cpp_cur->methodName[i], sizeof cpp_cur->methodName[i], "%s", name);
    cpp_cur->kind[i] = CPP_VIRTUAL;
    cpp_cur->slot[i] = cpp_cur->own_slots++;
    cpp_cur->iface_of[i] = 0;
    cpp_cur->method[i].ext.name = NULL;
}

/* Adopt an interface's slot layout. This is how the generator learns the
 * relationship: the generator calls it inside the class, before any field.
 * There is no base-clause to parse. The call copies that interface's virtual
 * names and slot numbers (Shape's draw stays slot 0) and reserves the next
 * vptr, in call order. cpp_virtual_begin later binds the still-open slot of
 * the same name. Declare every implemented interface before any field. */
static inline void cpp_implements(CppClass* iface) {
    if (!cpp_cur) cpp_fail("cpp_implements outside a class");
    crash_if(iface == NULL || iface->rec == NULL || !iface->rec->isInterface,
             "cpp_implements: target is not an interface");
    crash_if(cpp_cur->laid_out, "cpp_implements: call it before declaring fields");
    crash_if(cpp_cur->has_own, "cpp_implements: own virtuals already declared");
    crash_if(cpp_cur->niface >= CPP_MAX_IFACES, "cpp_implements: too many interfaces");
    int vi = cpp_cur->niface++;
    CppClassRec* src = iface->rec;
    cpp_cur->iface[vi] = src;
    for (int j = 0; j < src->nmethods; j++) {
        if (src->kind[j] != CPP_VIRTUAL) continue;
        crash_if(cpp_cur->nmethods >= CPP_MAX_MEMBERS, "cpp: too many members");
        int i = cpp_cur->nmethods++;
        snprintf(cpp_cur->methodName[i], sizeof cpp_cur->methodName[i], "%s", src->methodName[j]);
        cpp_cur->kind[i] = CPP_VIRTUAL;
        cpp_cur->slot[i] = src->slot[j];
        cpp_cur->iface_of[i] = vi;
        cpp_cur->method[i].ext.name = NULL;
    }
}

/* Open the body of member i as Class$method with this in the first parameter. */
static inline CFunc* cpp_bind_method(int i) {
    const char* mangled = cpp_mangle(cpp_cur->name, cpp_cur->methodName[i]);
    const CParam p[] = { { "this", C_PTR } };
    cpp_cur->method[i] = cpp_begin_function(mangled, p, 1, C_PTR, NULL);
    cpp_cur_method = i;
    return &cpp_cur->method[i];
}

/* Open member method of kind on the current class, binding a free virtual slot when one matches. */
static inline CFunc* cpp_member_open(const char* method, CppMemberKind kind) {
    cpp_check_ident(method);
    if (!cpp_cur) cpp_fail("cpp: member declared outside a class");
    if (kind == CPP_VIRTUAL) {
        for (int i = 0; i < cpp_cur->nmethods; i++) {
            if (cpp_cur->kind[i] == CPP_VIRTUAL &&
                cpp_cur->method[i].ext.name == NULL &&
                strcmp(cpp_cur->methodName[i], method) == 0) {
                return cpp_bind_method(i);
            }
        }
        crash_if(cpp_cur->niface > 0, "cpp: virtual is not on an implemented interface");
        crash_if(cpp_cur->laid_out, "cpp: declare virtuals before fields");
        cpp_cur->has_own = true;
    }
    if (cpp_cur->nmethods >= CPP_MAX_MEMBERS) cpp_fail("cpp: too many members");
    int i = cpp_cur->nmethods++;
    snprintf(cpp_cur->methodName[i], sizeof cpp_cur->methodName[i], "%s", method);
    cpp_cur->kind[i] = kind;
    cpp_cur->slot[i] = -1;
    cpp_cur->iface_of[i] = 0;
    if (kind == CPP_VIRTUAL) cpp_cur->slot[i] = cpp_cur->own_slots++;
    if (kind == CPP_CTOR) cpp_cur->ctorIdx = i;
    if (kind == CPP_DTOR) cpp_cur->dtorIdx = i;
    return cpp_bind_method(i);
}

/* A non-virtual method with `this` plus `nextra` parameters. cpp_method_begin
 * only binds `this`. The extras follow `this` in declaration order, so
 * parameter 0 is still the receiver and parameter 1 is extra[0]. */
static inline CFunc* cpp_method_begin_params(const char* name, const CParam* extra, int nextra) {
    cpp_check_ident(name);
    if (!cpp_cur) cpp_fail("cpp: member declared outside a class");
    if (cpp_cur->nmethods >= CPP_MAX_MEMBERS) cpp_fail("cpp: too many members");
    crash_if(nextra < 0 || nextra > 8, "cpp: too many method params");
    int i = cpp_cur->nmethods++;
    snprintf(cpp_cur->methodName[i], sizeof cpp_cur->methodName[i], "%s", name);
    cpp_cur->kind[i] = CPP_NONVIRTUAL;
    cpp_cur->slot[i] = -1;
    cpp_cur->iface_of[i] = 0;
    CParam ps[9];
    ps[0] = (CParam){ "this", C_PTR };
    for (int k = 0; k < nextra; k++) ps[1 + k] = extra[k];
    const char* mangled = cpp_mangle(cpp_cur->name, cpp_cur->methodName[i]);
    cpp_cur->method[i] = cpp_begin_function(mangled, ps, 1 + nextra, C_PTR, NULL);
    cpp_cur_method = i;
    return &cpp_cur->method[i];
}

/* Like cpp_method_begin, but the procedure returns in rax tagged `ret_tag`.
 * A void method leaves the tag NULL. The invariant needs a tag so
 * c_ret_value can place its flag word and write_ret can check it. */
static inline CFunc* cpp_method_begin_ret(const char* name, const char* ret_tag) {
    cpp_check_ident(name);
    if (!cpp_cur) cpp_fail("cpp: member declared outside a class");
    if (cpp_cur->nmethods >= CPP_MAX_MEMBERS) cpp_fail("cpp: too many members");
    int i = cpp_cur->nmethods++;
    snprintf(cpp_cur->methodName[i], sizeof cpp_cur->methodName[i], "%s", name);
    cpp_cur->kind[i] = CPP_NONVIRTUAL;
    cpp_cur->slot[i] = -1;
    cpp_cur->iface_of[i] = 0;
    const CParam p[] = { { "this", C_PTR } };
    const char* mangled = cpp_mangle(cpp_cur->name, name);
    cpp_cur->method[i] = cpp_begin_function(mangled, p, 1, C_PTR, ret_tag);
    cpp_cur_method = i;
    return &cpp_cur->method[i];
}

/* `operator op` as a method of this and other. "+" is Vec2::operator+. */
static inline CFunc* cpp_operator_begin(const char* op) {
    if (!op || strcmp(op, "+") != 0) cpp_fail("cpp_operator_begin: only + is defined");
    const CParam extra[] = { { "other", C_PTR } };
    return cpp_method_begin_params("operator_add", extra, 1);
}

/* One procedure per concrete type. body is written once and receives the
 * type string, so the instantiation can use it. The symbol is base_type. */
typedef void (*CppMonoBody)(CFunc* fn, const char* type, void* user);
static inline CFunc cpp_instantiate(const char* base, const char* type,
                                    CppMonoBody body, void* user) {
    cpp_check_ident(base);
    cpp_check_ident(type);
    char buf[96];
    int n = snprintf(buf, sizeof buf, "%s_%s", base, type);
    crash_if(n < 0 || n >= (int)sizeof buf, "cpp_instantiate: name too long");
    CFunc fn = cpp_begin_function(c_strdup_arena(buf), NULL, 0, C_PTR, NULL);
    body(&fn, type, user);
    cpp_end_function(&fn);
    return fn;
}

/* Open an ordinary method member for name. */
static inline CFunc* cpp_method_begin(const char* name)  { return cpp_member_open(name, CPP_NONVIRTUAL); }
/* Open a virtual method member for name. */
static inline CFunc* cpp_virtual_begin(const char* name) { return cpp_member_open(name, CPP_VIRTUAL); }
/* Open a constructor member. */
static inline CFunc* cpp_ctor_begin(void)                { return cpp_member_open("ctor", CPP_CTOR); }
/* Open a destructor member. */
static inline CFunc* cpp_dtor_begin(void)                { return cpp_member_open("dtor", CPP_DTOR); }

/* Close the open member and drop the current method index. */
static inline void cpp_method_end(void) {
    if (!cpp_cur || cpp_cur_method < 0) cpp_fail("cpp_method_end without an open member");
    cpp_end_function(&cpp_cur->method[cpp_cur_method]);
    cpp_cur_method = -1;
}

/* The current member's receiver, parameter 0. */
static inline CValue cpp_this(void) {
    if (!cpp_cur || cpp_cur_method < 0) cpp_fail("cpp_this outside a member body");
    return c_param(&cpp_cur->method[cpp_cur_method], 0);
}

/* Place field name of type, padding with align_mode (the default when negative). */
static inline void cpp_place_field(const char* name, CType type, int align_mode) {
    cpp_check_ident(name);
    if (!cpp_cur) cpp_fail("cpp_field outside a class");
    if (cpp_cur->nfields >= CPP_MAX_MEMBERS) cpp_fail("cpp: too many fields");
    if (!cpp_cur->laid_out) {
        cpp_cur->next_off = cpp_header_bytes(cpp_cur);
        cpp_cur->laid_out = 1;
    }
    int size = type.size;
    crash_if(size != 1 && size != 2 && size != 4 && size != 8, "cpp_field: size must be 1, 2, 4, or 8");
    AlignMode mode = align_mode < 0 ? align_get_default() : (AlignMode)align_mode;
    int pad = align_pad(cpp_cur->next_off, size, mode);
    int i = cpp_cur->nfields++;
    snprintf(cpp_cur->fieldName[i], sizeof cpp_cur->fieldName[i], "%s", name);
    cpp_cur->fieldSize[i] = size;
    cpp_cur->fieldAlign[i] = align_mode;
    cpp_cur->fieldOff[i] = cpp_cur->next_off + pad;
    cpp_cur->next_off = cpp_cur->fieldOff[i] + size;
}

/* Place field name of type with the default alignment. */
static inline void cpp_field(const char* name, CType type) {
    cpp_place_field(name, type, -1);
}

/* Place field name of type using alignment mode. */
static inline void cpp_field_mode(const char* name, CType type, AlignMode mode) {
    cpp_place_field(name, type, (int)mode);
}

/* Finish the current class, emit any vtables, and return its handle. */
static inline CppClass cpp_class_end(void) {
    if (!cpp_cur) cpp_fail("cpp_class_end without an open class");
    CppClassRec* r = cpp_cur;
    if (!r->isInterface && (r->niface > 0 || r->has_own)) {
        int tables = r->niface > 0 ? r->niface : 1;
        for (int vi = 0; vi < tables; vi++) {
            int nslot = (r->niface > 0) ? r->iface[vi]->own_slots : r->own_slots;
            const char* vt = cpp_vtable_name(r, vi);
            write_data();
            int emitted = 0;
            for (int s = 0; s < nslot; s++) {
                const char* msym = NULL;
                for (int i = 0; i < r->nmethods; i++) {
                    int on_this = (r->niface > 0) ? (r->iface_of[i] == vi) : 1;
                    if (on_this && r->slot[i] == s) {
                        crash_if(r->method[i].ext.name == NULL, "cpp: vtable slot has no method body");
                        msym = r->method[i].ext.name;
                        break;
                    }
                }
                crash_if(msym == NULL, "cpp: vtable slot has no method");
                if (emitted == 0) c_ptr_data(vt, msym, "vptr");
                else              write_dq("", c_offset(msym));
                emitted++;
            }
            write_code();
        }
    }
    cpp_cur = NULL;
    cpp_cur_method = -1;
    CppClass h;
    h.rec = r;
    return h;
}

/* Whether class record r stores one or more vptrs. */
static inline bool cpp_has_vptr(CppClassRec* r) {
    return !r->isInterface && (r->niface > 0 || r->has_own);
}

/* Byte offset of field name in class record r. */
static inline int cpp_field_offset(CppClassRec* r, const char* name) {
    for (int i = 0; i < r->nfields; i++)
        if (strcmp(r->fieldName[i], name) == 0) return r->fieldOff[i];
    cpp_fail("cpp_field_offset: no such field");
    return 0;
}

/* Byte width of field name in class record r. */
static inline int cpp_field_size(CppClassRec* r, const char* name) {
    for (int i = 0; i < r->nfields; i++)
        if (strcmp(r->fieldName[i], name) == 0) return r->fieldSize[i];
    cpp_fail("cpp_field_size: no such field");
    return 0;
}

/* Emit label as a data directive of size holding value. */
static inline void cpp_emit_bytes(const char* label, int size, const char* value) {
    if (size == 8)      write_dq(label, value);
    else if (size == 4) write_dd(label, value);
    else if (size == 2) write_dw(label, value);
    else if (size == 1) write_db(label, value);
    else cpp_fail("cpp: cannot emit that field width");
}

/* Emit nbytes of zero padding in 4, 2, or 1 byte chunks. */
static inline void cpp_emit_pad(int nbytes) {
    while (nbytes > 0) {
        int chunk = nbytes >= 4 ? 4 : (nbytes >= 2 ? 2 : 1);
        cpp_emit_bytes("", chunk, "0");
        nbytes -= chunk;
    }
}

/* Zero-filled storage for a global. The symbol `name` is the instance label. */
static inline void cpp_emit_storage(CppClassRec* r, const char* name) {
    int header = cpp_header_bytes(r);
    write_data();
    if (header > 0) {
        int tables = r->niface > 0 ? r->niface : 1;
        c_ptr_data(name, cpp_vtable_name(r, 0), "vptr");
        for (int vi = 1; vi < tables; vi++)
            write_dq("", c_offset(cpp_vtable_name(r, vi)));
    }
    int cursor = header;
    if (header == 0 && r->nfields == 0) {
        c_word_data(name, "0", "this");
        cursor = 8;
    }
    for (int i = 0; i < r->nfields; i++) {
        int pad = r->fieldOff[i] - cursor;
        if (pad > 0) cpp_emit_pad(pad);
        const char* label = (header == 0 && i == 0) ? name : "";
        cpp_emit_bytes(label, r->fieldSize[i], "0");
        cursor = r->fieldOff[i] + r->fieldSize[i];
    }
    write_code();
    (void)cursor;
}

/* Whether scope sc already tracks an object labeled name. */
static inline int cpp_scope_holds(CppScopeData* sc, const char* name) {
    for (int i = 0; i < sc->n; i++)
        if (sc->objs[i] != NULL && strcmp(sc->objs[i], name) == 0) return 1;
    return 0;
}

/* Record the destructor of r for label name, restoring release bytes after it. */
static inline void cpp_note_dtor(CppClassRec* r, const char* name, int release) {
    if (r->dtorIdx < 0 && release == 0) return;
    if (cpp_scope_depth == 0) cpp_fail("cpp: destructor or stack object outside any scope");
    CppScopeData* sc = &cpp_scope_pool[cpp_scope_stack[cpp_scope_depth - 1]];
    if (name != NULL && cpp_scope_holds(sc, name)) return;
    if (sc->n >= CPP_SCOPE_MAX) cpp_fail("cpp: too many objects in scope");
    sc->dtors[sc->n] = r->dtorIdx >= 0 ? &r->method[r->dtorIdx].ext : NULL;
    sc->objs[sc->n] = name;
    sc->release[sc->n] = release;
    sc->n++;
}

/* Call the constructor of r with receiver th when the class has one. */
static inline void cpp_call_ctor(CppClassRec* r, AsmDataField th) {
    if (r->ctorIdx >= 0) write_call(&r->method[r->ctorIdx].ext, th);
}

/* Global instance. `name` is the MASM data label of this one object, the
 * symbol cpp_send and lea use as its address. It is not the class name and
 * not a constructor argument. Two instances need two labels. */
static inline CppObject cpp_construct(CppClass* cls, const char* name) {
    cpp_check_ident(name);
    CppClassRec* r = cls->rec;
    cpp_emit_storage(r, name);
    cpp_call_ctor(r, c_addr(name, "this"));
    cpp_note_dtor(r, name, 0);
    CppObject o;
    o.sym = name;
    o.rec = r;
    return o;
}

/* Storage only for cls under label name, so a later cpp_emplace can overwrite it.
 * Generated heap allocation lives in gc/. */
static inline void cpp_reserve(CppClass* cls, const char* name) {
    cpp_check_ident(name);
    cpp_emit_storage(cls->rec, name);
}

/* Placement of cls into existing global label name. A second emplace of the
 * same label runs the constructor again and leaves the one destructor already registered. */
static inline CppObject cpp_emplace(CppClass* cls, const char* name) {
    cpp_check_ident(name);
    CppClassRec* r = cls->rec;
    cpp_call_ctor(r, c_addr(name, "this"));
    cpp_note_dtor(r, name, 0);
    CppObject o;
    o.sym = name;
    o.rec = r;
    return o;
}

/* Subtract a tracker-sized frame for cls and construct at [rsp]. The enclosing
 * scope runs the destructor, then adds the reservation back. */
static inline CppObject cpp_stack_construct(CppClass* cls) {
    CppClassRec* r = cls->rec;
    int need = cpp_object_bytes(r);
    int frame = asm_call_frame_size(need);
    const char* tok = asm_frame_token(frame);
    write_sub((AsmDataField){ "rsp", "rsp", NULL }, literal(tok, "frame"));
    asm_note_rsp_sub(frame);
    write_lea("rcx", "[rsp]");
    asm_type_dict_set("rcx", "this");
    cpp_call_ctor(r, (AsmDataField){ "rcx", "this", NULL });
    cpp_note_dtor(r, NULL, frame);
    CppObject o;
    o.sym = NULL;
    o.rec = r;
    return o;
}

/* Field name of the current receiver, wrapped as type.
 * name is the field. type is the C type of the loaded cell.
 * A virtual call through a later interface has already added that
 * interface's stride to this. Field offsets are measured from the
 * start of the object, so the stride comes back off here. The first
 * interface and every non-virtual method sit at stride 0. */
static inline CValue cpp_this_field(const char* name, CType type) {
    if (!cpp_cur || cpp_cur_method < 0) cpp_fail("cpp_this_field outside a member body");
    CValue self = c_resolve(cpp_this());
    int off = cpp_field_offset(cpp_cur, name);
    if (cpp_cur->niface > 0)
        off -= cpp_cur->iface_of[cpp_cur_method] * 8;
    int size = cpp_field_size(cpp_cur, name);
    AsmDataField f = { cpp_mem_sized(self.loc, off, size), name, NULL };
    return c_wrap(f, type);
}

/* Field name on global object o, wrapped as type. */
static inline CValue cpp_field_ref(CppObject* o, const char* name, CType type) {
    crash_if(o->sym == NULL, "cpp_field_ref: stack object has no global label");
    int off = cpp_field_offset(o->rec, name);
    int size = cpp_field_size(o->rec, name);
    const char* kw = size == 1 ? "byte ptr" : size == 2 ? "word ptr"
                   : size == 4 ? "dword ptr" : "qword ptr";
    char buf[96];
    if (off) snprintf(buf, sizeof buf, "%s %s+%d", kw, o->sym, off);
    else     snprintf(buf, sizeof buf, "%s %s", kw, o->sym);
    AsmDataField f = { c_strdup_arena(buf), name, NULL };
    return c_wrap(f, type);
}

/* Call method with the address of global obj as this. */
static inline void cpp_send(CFunc* method, CppObject obj) {
    crash_if(obj.sym == NULL, "cpp_send: stack object is not a named global");
    c_call(method, c_addr(obj.sym, "this"));
}

/* Call method with this and one numeric argument. */
static inline void cpp_send_imm(CFunc* method, CppObject obj, long long n, const char* tag) {
    crash_if(obj.sym == NULL, "cpp_send_imm: stack object is not a named global");
    c_call(method, c_addr(obj.sym, "this"), c_cell(c_imm_n(n, C_PTR, tag)));
}

/* a + b. The class of a must have been given operator+ via cpp_operator_begin. */
static inline void cpp_add(CppObject a, CppObject b) {
    crash_if(a.sym == NULL || b.sym == NULL, "cpp_add: stack object is not a named global");
    crash_if(a.rec == NULL, "cpp_add: no class");
    for (int i = 0; i < a.rec->nmethods; i++) {
        if (strcmp(a.rec->methodName[i], "operator_add") == 0) {
            c_call(&a.rec->method[i], c_addr(a.sym, "this"), c_addr(b.sym, "other"));
            return;
        }
    }
    cpp_fail("cpp_add: class has no operator+");
}

/* Virtual call of method on iface through the vptr at recv. */
static inline void cpp_vsend(CValue recv, CppClass* iface, const char* method) {
    int slot = -1;
    CppClassRec* layout = iface->rec;
    for (int i = 0; i < layout->nmethods; i++) {
        if (layout->slot[i] >= 0 && strcmp(layout->methodName[i], method) == 0) {
            slot = layout->slot[i];
            break;
        }
    }
    if (slot < 0) cpp_fail("cpp_vsend: no such virtual method");
    recv = c_resolve(recv);
    const char* this_reg = recv.loc;
    write_replace_mov("rax", (AsmDataField){ cpp_mem(this_reg, 0), "vptr", NULL });
    write_replace_mov("rax", (AsmDataField){ cpp_mem("rax", slot * 8), "method", NULL });
    cpp_frame_open();
    cpp_call_reg("rax");
    cpp_frame_close();
}

/* Virtual call of method on global obj, adjusting this to the vptr of iface. */
static inline void cpp_vsend_obj(CppObject obj, CppClass* iface, const char* method) {
    crash_if(obj.sym == NULL, "cpp_vsend_obj: stack object has no global label");
    int off = cpp_iface_offset(obj.rec, iface->rec);
    write_replace_mov("rcx", (AsmDataField){ c_addr(obj.sym, "this").name, "this", NULL });
    if (off != 0) {
        write_add((AsmDataField){ "rcx", "this", NULL }, literal(asm_frame_token(off), "this"));
    }
    asm_type_dict_set("rcx", "this");
    CValue recv = { "this", C_PTR, "rcx", "this", c_reg_index("rcx"), true, -1 };
    cpp_vsend(recv, iface, method);
}

/* ======================================================================== *
 *  Console for the feature tour
 * ======================================================================== */

typedef struct {
    AsmExtern get_std_handle;
    AsmExtern write_file;
    AsmExtern exit_process;
} CppConsole;

typedef struct { AsmDataField off; AsmDataField len; } CppStr;

/* Declare GetStdHandle, WriteFile, and ExitProcess. */
static inline CppConsole cpp_console_declare(void) {
    CppConsole c;
    static const char* gsh[] = { "nStdHandle" };
    c.get_std_handle = c_extern("GetStdHandle", gsh, 1, C_PTR, "handle");
    static const char* wf[] = { "handle", "lpBuffer", "nBytes", "lpWritten", "lpOverlapped" };
    c.write_file = c_extern("WriteFile", wf, 5, C_INT, "ok");
    static const char* ep[] = { "exit_code" };
    c.exit_process = c_extern("ExitProcess", ep, 1, C_PTR, NULL);
    return c;
}

/* The cell WriteFile's lpWritten points at. */
static inline void cpp_console_globals(void) {
    c_ensure_bss();
    write_unitialised_data("written", "?");
}

/* Address and length of a MASM db operand. */
static inline CppStr cpp_string(const char* name, const char* text_literal) {
    CStr s = c_string(name, text_literal, "lpBuffer", "nBytes");
    CppStr o;
    o.off = s.off;
    o.len = s.len;
    return o;
}

/* Address and length of a C string. nl is 0, 1 (LF), or 2 (CRLF). */
static inline CppStr cpp_line(const char* name, const char* text) {
    return cpp_string(name, c_db_text(text, 2, 0));
}

/* print(buffer, length). The call saves values that GetStdHandle clobbers. */
static inline AsmExtern cpp_console_print(CppConsole* c) {
    const CParam params[] = { { "lpBuffer", C_PTR }, { "nBytes", C_PTR } };
    CFunc print = c_begin_function("print", params, 2, C_PTR, NULL);
        CValue buf = c_param(&print, 0);
        CValue len = c_param(&print, 1);
        CValue handle = c_call_ext(C_PTR, &c->get_std_handle, c_lit_n(-11, "nStdHandle"));
        c_call_ext_void(&c->write_file,
            c_cell(handle),
            c_cell(buf),
            c_cell(len),
            c_addr("written", "lpWritten"),
            c_lit_n(0, "lpOverlapped"));
        c_ret(&print);
    c_end_function(&print);
    return print.ext;
}

/* Call print with the address and length of s. */
static inline void cpp_puts(AsmExtern* print, CppStr s) {
    write_call(print, s.off, s.len);
}

/* Call ExitProcess with a numeric status. */
static inline void cpp_exit(CppConsole* c, long long code) {
    c_call_ext_void(&c->exit_process, c_lit_n(code, "exit_code"));
}

/* Reset class and scope tables, and set case-sensitive names with C alignment. */
static inline void cpp_init(void) {
    write_set_case_sensitive_identifiers();
    align_set_default(ALIGN_C);
    cpp_nclass = 0;
    cpp_cur = NULL;
    cpp_cur_method = -1;
    cpp_scope_depth = 0;
}
