
/*
 * asm_funcs.h - MASM emitter. Source files include host.h before this header.
 * host.h supplies the C runtime surface, crash_if, and the alignment helpers.
 * This header includes nothing.
 *
 * Generation state (the output buffer, the type dictionaries, the bump arena,
 * the indent counter) is process-global. The generator is single-threaded.
 * Run separate processes when you want concurrent generation. Threading this
 * state later is ordinary work; it is not required for the programs we emit.
 *
 * Helpers are added as experience demands. write_rep_movsb is the pattern for
 * an instruction whose registers are implicit: the wrapper takes those
 * registers as cells and checks them. write_init_mov, write_set_mov, and
 * write_replace_mov are the pattern for one mnemonic (mov) with three
 * different checking rules. A new check belongs next to the helper that needs
 * it, not in a second language layer.
 */

/*
 * DLL-call spelling used in generated assembly.
 *
 * The default matches Visual C object files:
 *
 *   extern __imp_ExitProcess:qword
 *   call qword ptr [__imp_ExitProcess]
 *
 * An import library resolves __imp_ExitProcess directly to the DLL's import
 * address table (IAT) slot. Calling through that slot avoids the extra
 * ExitProcess trampoline that link.exe would otherwise synthesize for
 * "call ExitProcess".
 *
 * Set ASM_USE_VISUAL_C_DLL_CALLS to 0 before including this header (or define
 * it on the compiler command line) for linkers/import libraries that expose
 * only conventional MASM procedure symbols:
 *
 *   extern ExitProcess:proc
 *   call ExitProcess
 *
 * The option affects DLL symbols created by write_extern/write_common_extern
 * only. Procedures created by write_proc and indirect register calls retain
 * their existing spelling.
 */
#ifndef ASM_USE_VISUAL_C_DLL_CALLS
#define ASM_USE_VISUAL_C_DLL_CALLS 1
#endif

#if ASM_USE_VISUAL_C_DLL_CALLS != 0 && ASM_USE_VISUAL_C_DLL_CALLS != 1
#error ASM_USE_VISUAL_C_DLL_CALLS must be 0 or 1
#endif

#define ASM_OUTPUT_MAX 1048576
static const char* asm_output[ASM_OUTPUT_MAX];
static int asm_output_cursor = 0;

/* Append `token` to the generated assembly. The pointer is stored as given, so the bytes must outlive emission. */
static inline void write(const char* token) {
    asm_output[asm_output_cursor++] = token;
}

static int asm_indent_level = 0;

typedef struct {
    const char* value;
    const char* type;
} TypedValue;

typedef struct {
    const char* name;           /* where the value lives / is written */
    const char* type;           /* the location's real / declared type tag */
    const char* use_as;         /* tag it has been asserted usable as; NULL means
                                 * "same as type". Only treat_as sets this to
                                 * differ from type. Type validation checks it. */
    int         is_register;    /* nonzero when `name` is a register. write_extern
                                 * and write_proc set this on the copy they keep.
                                 * A 3-field initializer leaves it 0. */
} AsmDataField;

/*
 * The tag a cell is validated against: its asserted use_as when set, otherwise
 * its declared type. The two are equal for any cell not produced by treat_as
 * (whose literals leave use_as NULL), so ordinary code is unaffected.
 */
static inline const char* asm_field_use_as(AsmDataField f) {
    return f.use_as ? f.use_as : f.type;
}

/*
 * literal - Construct a cell for a bare immediate operand.
 *
 * Immediates (numeric or hex constants such as "0", "80h", "123") are not stored
 * in any tracked location, so they carry no dictionary entry. literal(value, tag)
 * pairs the immediate with the logical type expected where it is used (e.g.
 * literal("0", "lpName")), which reads better than a raw compound literal and
 * marks the operand as an untracked immediate rather than a named location. The
 * tag is still checked against a call's declared parameter type.
 */
static inline AsmDataField literal(const char* value, const char* tag) {
    AsmDataField out = { value, tag };
    return out;
}

/*
 * asm_void - the empty-cell sentinel.
 *
 * A zero-initialised AsmDataField (all fields NULL) meaning "no cell". A
 * procedure or AsmExtern that returns nothing passes asm_void as its return_reg
 * (recognised internally by return_reg.name == NULL). Prefer this named sentinel
 * over a bare (AsmDataField){0} at call sites.
 */
static const AsmDataField asm_void = {0};

/* A call contract: the symbol or register to call, one cell per parameter,
 * the register the result comes back in, whether the symbol is a DLL import,
 * and the GPR mask the callee may overwrite.
 *
 * write_extern fills one in for an import. write_proc fills one in for a
 * procedure this file defines. An indirect call (cpp_call_reg) fills one in
 * whose name is a register. write_call and write_extern_call both consume
 * the same struct. "Extern" was the first of those uses. The name of the
 * struct is AsmCall. AsmExtern is a typedef of AsmCall so every existing
 * parameter and local still compiles. The typedef does not mean the target
 * is imported. is_dll_import is the field that means that. */
typedef struct {
    const char* name;
    const AsmDataField* params;  /* one (register, type) cell per parameter */
    int param_count;
    AsmDataField return_reg;  /* (register, type) the result comes back in; {0} when none */
    int is_dll_import;         /* nonzero only for write_extern declarations */
    unsigned volatile_mask;    /* GPRs the callee may overwrite; 0 = not declared */
} AsmCall;
typedef AsmCall AsmExtern;

/* Windows x64 GPR classes, as bit i of the c_regs / asm_gpr_index index.
 * Volatile (caller-saved): rax, rcx, rdx, r8-r11.
 * Preserved (callee-saved), excluding rsp: rbx, rbp, rsi, rdi, r12-r15. */
#define ASM_WIN64_VOLATILE  0x0F07u
#define ASM_WIN64_PRESERVED 0xF0E8u

/* Defined with the bump arena, below. Call sites above that point see this. */
static inline void* asm_bump_alloc(size_t size);
static inline AsmDataField* asm_copy_call_params(const AsmDataField* params, int count);

/*
 * The number of leading parameters passed in registers is implied by params:
 * a parameter travels in a register when its cell names a register, and on the
 * stack when its cell names a memory slot (a "[...]" reference such as
 * "qword ptr [rsp+32]") or has no location at all. The register count is the
 * length of the initial run of register-named cells.
 */
static inline int asm_gpr_index(const char* name);
/* True only for a GPR name (rax, eax, r15, ...). An immediate such as "0"
 * and a memory operand such as "qword ptr [rsp+40]" are not registers. */
static inline int asm_name_is_register(const char* name) {
    return asm_gpr_index(name) >= 0;
}

/* Leading register parameters. is_register is set when the call descriptor is
 * built (write_extern / write_proc copy the array into the arena). */
static inline int asm_extern_reg_count(const AsmExtern* ext) {
    int n = 0;
    while (n < ext->param_count && ext->params != NULL && ext->params[n].is_register)
        n++;
    return n;
}

/*
 * Type-validation dictionaries.
 *
 * Two maps from a location (a register, global, or stack slot — the string
 * carried in AsmDataField.name) to the logical type tag living there
 * (AsmDataField.type). They exist only at generation time to catch type-tag
 * mismatches; they emit no assembly.
 *
 * - The LOCAL dictionary (asm_type_dict) tracks per-procedure state: registers,
 *   parameters, and other scratch locations. write_proc clears it and seeds it
 *   with the procedure's parameter locations; write_endp clears it. Within a
 *   body, write_replace_mov overwrites an entry (a location is repurposed) and a
 *   call records its return register.
 * - The GLOBAL dictionary (asm_global_dict) tracks data globals declared with
 *   write_unitialised_data / write_initialised_data / write_data_memory_layout.
 *   It is never cleared and its entries are read-only once created: a global's
 *   declared type is fixed for the whole program (a global that legitimately
 *   holds different types over time is given a combined tag like "x or y" and
 *   any narrower check is a runtime concern). Compile-time constants (write_equ /
 *   write_constant) are NOT registered: they are immediates reused at each site
 *   with that site's tag, not typed storage.
 *
 * assert_type / write_set_mov / write_init_mov consult BOTH via asm_type_lookup.
 */
#define ASM_TYPE_DICT_MAX 1024
#define ASM_GLOBAL_DICT_MAX 128

typedef struct {
    const char* location;
    const char* tag;
} AsmTypeEntry;

/* A linear array is the structure that fits these sizes. Lookup walks the
 * live entries, at most 1024 local or 128 global. The local cap is that
 * large because write_replace_mov records every destination, including
 * each fresh frame-slot string a call save uses. A tree pays for a node and
 * for pointer chasing on every insert and every lookup. A hash table pays
 * for the hash and for a bucket array that is mostly empty at these key
 * counts. The scan is shorter than either, and generation spends its time
 * writing text.
 * The tables are process-global, matching the single-threaded generator.
 * Overflow aborts with the cap in the message. A larger unit raises the
 * constant. The arrays stay. */
static AsmTypeEntry asm_type_dict[ASM_TYPE_DICT_MAX];
static int asm_type_dict_count = 0;

/* A disposed location is dead until this procedure ends or a later
 * write_init_mov / write_replace_mov establishes a new value there.
 * A read or an in-place update is a generation error. The C layer drops
 * the register at the same time, so spill and call save/restore skip it. */
#define ASM_DISPOSED_MAX 64
static const char* asm_disposed_loc[ASM_DISPOSED_MAX];
static int asm_disposed_count = 0;

/* Drop every disposed-location mark so the list is empty. */
static inline void asm_disposed_clear(void) {
    asm_disposed_count = 0;
}

/* Return 1 when `location` is on the disposed list. A null `location` returns 0. */
static inline int asm_is_disposed(const char* location) {
    if (location == NULL) return 0;
    for (int i = 0; i < asm_disposed_count; i++) {
        if (asm_disposed_loc[i] != NULL &&
            strcmp(asm_disposed_loc[i], location) == 0)
            return 1;
    }
    return 0;
}

/* The location is being given a new value. The disposed mark belongs to
 * the previous value, not to the register name. */
static inline void asm_disposed_forget(const char* location) {
    if (location == NULL) return;
    int w = 0;
    for (int i = 0; i < asm_disposed_count; i++) {
        if (asm_disposed_loc[i] != NULL &&
            strcmp(asm_disposed_loc[i], location) == 0)
            continue;
        asm_disposed_loc[w++] = asm_disposed_loc[i];
    }
    asm_disposed_count = w;
}

static AsmTypeEntry asm_global_dict[ASM_GLOBAL_DICT_MAX];
static int asm_global_dict_count = 0;

/* The return register (location + tag) of the procedure currently being
 * generated, captured by write_proc so write_ret can assert the value being
 * returned matches it. {0} (NULL name) when outside any procedure. */
static AsmDataField asm_current_return_reg = {0};

/* Empty the local type dictionary and the disposed-location list. */
static inline void asm_type_dict_clear(void) {
    asm_type_dict_count = 0;
    asm_disposed_clear();
}

/* Returns the tag recorded in the local dictionary at location, or NULL. */
static inline const char* asm_type_dict_get(const char* location) {
    if (location == NULL) return NULL;
    for (int i = 0; i < asm_type_dict_count; i++) {
        if (strcmp(asm_type_dict[i].location, location) == 0) {
            return asm_type_dict[i].tag;
        }
    }
    return NULL;
}

/* Upsert into the local dictionary: record (or overwrite) the tag at location. */
static inline void asm_type_dict_set(const char* location, const char* tag) {
    if (location == NULL) return;
    for (int i = 0; i < asm_type_dict_count; i++) {
        if (strcmp(asm_type_dict[i].location, location) == 0) {
            asm_type_dict[i].tag = tag;
            return;
        }
    }
    if (asm_type_dict_count >= ASM_TYPE_DICT_MAX) {
        fprintf(stderr, "error: type dictionary overflow (max %d entries)\n", ASM_TYPE_DICT_MAX);
        exit(1);
    }
    asm_type_dict[asm_type_dict_count].location = location;
    asm_type_dict[asm_type_dict_count].tag = tag;
    asm_type_dict_count++;
}

/*
 * Relabel a cell: keep the same location but give it a new type tag. Use this
 * where a register keeps the value it already holds but takes on a new logical
 * meaning (typically after an in-place op like write_add/write_sub) so the
 * location literal is carried over from the existing cell rather than repeated.
 * For example, AsmDataField rdi_dst = change_tag(rdi_pView, "dst"). The result is
 * a plain cell (use_as follows the new type), not a treat_as reinterpretation.
 *
 * Because a register's meaning is what the local dictionary tracks, relabelling a
 * currently-tracked location also updates its local entry so later validation
 * (assert_type, write_set_mov) sees the new meaning. Untracked locations
 * (immediates) and read-only data globals are left alone.
 */
static inline AsmDataField change_tag(AsmDataField cell, const char* type) {
    if (asm_type_dict_get(cell.name) != NULL) {
        asm_type_dict_set(cell.name, type);
    }
    cell.type = type;
    cell.use_as = NULL;
    return cell;
}

/* Returns the tag recorded in the global dictionary at location, or NULL. */
static inline const char* asm_global_dict_get(const char* location) {
    if (location == NULL) return NULL;
    for (int i = 0; i < asm_global_dict_count; i++) {
        if (strcmp(asm_global_dict[i].location, location) == 0) {
            return asm_global_dict[i].tag;
        }
    }
    return NULL;
}

/* Register a data global's type. Entries are write-once: re-declaring the same
 * location with a different tag is an error; an identical re-declaration is a
 * harmless no-op. */
static inline void asm_global_dict_add(const char* location, const char* tag) {
    if (location == NULL) return;
    const char* have = asm_global_dict_get(location);
    if (have != NULL) {
        if (tag != NULL && strcmp(have, tag) != 0) {
            fprintf(stderr, "error: global \"%s\" already declared as \"%s\", cannot redeclare as \"%s\"\n",
                    location, have, tag);
            exit(1);
        }
        return;
    }
    if (asm_global_dict_count >= ASM_GLOBAL_DICT_MAX) {
        fprintf(stderr, "error: global type dictionary overflow (max %d entries)\n", ASM_GLOBAL_DICT_MAX);
        exit(1);
    }
    asm_global_dict[asm_global_dict_count].location = location;
    asm_global_dict[asm_global_dict_count].tag = tag;
    asm_global_dict_count++;
}

/* Combined lookup: the local (per-procedure) dictionary shadows the global one.
 * Registers/params never share a name with data globals, so order is immaterial
 * in practice, but locals are checked first. */
static inline const char* asm_type_lookup(const char* location) {
    const char* t = asm_type_dict_get(location);
    if (t != NULL) return t;
    return asm_global_dict_get(location);
}


/*
 * write() stores the pointers it is given without copying, so every emitted
 * token must have static storage. The only operands computed at runtime are
 * write_call's frame size and its stack-argument memory references, and both
 * are always small multiples of 8. Rather than format strings into temporary
 * buffers, we pre-generate the full set of legal tokens here as compile-time
 * constants. ASM_MAX_STACK_ARGS bounds both tables; out-of-range requests abort
 * loudly via the lookup helpers below.
 */
#define ASM_MAX_STACK_ARGS 16

/*
 * Decimal frame-size tokens, indexed by frame/8. These are the only sizes
 * write_extern_call will emit, whether or not you care about data alignment.
 *
 * A Windows x64 call frame is 32 bytes of home space, plus 8 bytes for each
 * stack argument, plus 0 or 8 bytes so RSP is 16-byte aligned at the CALL.
 * With at most 16 stack arguments the legal sizes are the multiples of 8 from
 * 0 through 168 (32 + 16*8 + 8). The table is that list, as static strings,
 * because write() stores the pointer it is given and will not format a new
 * one. An out-of-range frame aborts in asm_frame_token.
 *
 * Caring about alignment does not add a row. Struct and data alignment is
 * align_pad in host.h (ALIGN_NONE, ALIGN_C, ALIGN_CACHE, ALIGN_PAGE). Those
 * modes insert bytes between fields. They never change the call-frame sizes
 * in this table, and turning them off does not make other frame sizes legal.
 */
static const char* const asm_frame_size_tokens[] = {
    "0",   "8",   "16",  "24",  "32",  "40",  "48",  "56",
    "64",  "72",  "80",  "88",  "96",  "104", "112", "120",
    "128", "136", "144", "152", "160", "168"
};

/*
 * Full "qword ptr [rsp+N]" stack-slot references, indexed by (N-32)/8.
 * Windows x64 places the first stack argument at [rsp+32] (after the 32-byte
 * home area) and steps by 8, through [rsp+152] when 16 stack arguments are
 * in use. The table is that sequence, as static strings. An offset outside
 * it aborts in asm_stack_slot_token.
 *
 * Other memory offsets remain legal in the rest of the file. A spill slot
 * is [rbp-8] and downward. A struct field is whatever align_pad computed.
 * Caring about alignment, or turning alignment off, does not add a stack
 * argument slot and does not remove one. This table is only the slots
 * write_extern_call emits for arguments past the fourth.
 */
static const char* const asm_stack_slot_tokens[] = {
    "qword ptr [rsp+32]",  "qword ptr [rsp+40]",  "qword ptr [rsp+48]",
    "qword ptr [rsp+56]",  "qword ptr [rsp+64]",  "qword ptr [rsp+72]",
    "qword ptr [rsp+80]",  "qword ptr [rsp+88]",  "qword ptr [rsp+96]",
    "qword ptr [rsp+104]", "qword ptr [rsp+112]", "qword ptr [rsp+120]",
    "qword ptr [rsp+128]", "qword ptr [rsp+136]", "qword ptr [rsp+144]",
    "qword ptr [rsp+152]"
};

/* Prebuilt decimal text for a call frame of `frame` bytes. `frame` must be a listed multiple of 8, or generation stops. */
static inline const char* asm_frame_token(int frame) {
    int idx = frame / 8;
    if (frame < 0 || frame % 8 != 0 ||
        idx >= (int)(sizeof(asm_frame_size_tokens) / sizeof(asm_frame_size_tokens[0]))) {
        fprintf(stderr, "asm: frame size %d out of range (max %d stack args)\n",
                frame, ASM_MAX_STACK_ARGS);
        exit(1);
    }
    return asm_frame_size_tokens[idx];
}

/* Prebuilt "qword ptr [rsp+N]" text for stack-argument offset `off`. An `off` outside that table stops generation. */
static inline const char* asm_stack_slot_token(int off) {
    int idx = (off - 32) / 8;
    if (off < 32 || off % 8 != 0 ||
        idx >= (int)(sizeof(asm_stack_slot_tokens) / sizeof(asm_stack_slot_tokens[0]))) {
        fprintf(stderr, "asm: stack slot offset %d out of range (max %d stack args)\n",
                off, ASM_MAX_STACK_ARGS);
        exit(1);
    }
    return asm_stack_slot_tokens[idx];
}

/* Emit four spaces once for each current indent level. */
static inline void write_indent(void) {
    for (int i = 0; i < asm_indent_level; i++) write("    ");
}

/* Emit one indented line whose text is `a`, followed by a newline. */
static inline void write_line(const char* a) {
    write_indent();
    write(a);
    write("\n");
}

/*
 * write_label - Emit a jump target label (e.g. "lw_size_ok:").
 *
 * A label sits one indent level out from the code it introduces, so this
 * dedents, emits "<name>:" at that outer level, then re-indents so the
 * following body lines line up again. Pass just the bare name; the trailing
 * colon is added here. Returns `name` so a backward branch (a loop) can reuse
 * the same handle for its `write_jmp` target instead of repeating the literal.
 */
static inline const char* write_label(const char* name) {
    asm_indent_level--;
    write_indent();
    write(name);
    write(":");
    write("\n");
    asm_indent_level++;
    return name;
}

/*
 * assert_type - Assert the expected logical type of a location.
 *
 * Asserts that, at this point in the generated code, loc.name (e.g. "rdx",
 * "rax", a global, a constant, or a stack slot) currently holds a value of
 * loc's *real* type (loc.type). It consults the type-validation dictionaries —
 * which record the actual tag stored at each location — and aborts generation if
 * the location is tracked with a different tag. This is a dictionary-integrity
 * check, so it uses the real type, not any use_as reinterpretation: a cell whose
 * use_as differs from its real type (produced by treat_as) still asserts against
 * its real type here, while the reinterpretation is honoured only where the value
 * is matched against an expected tag (a call argument). Locations that are not
 * tracked (immediates, ad-hoc operands) pass silently — there is nothing recorded
 * to contradict. It emits no assembly. Use it immediately after a call to confirm
 * the type left in the return register, or anywhere a value's tag needs to be
 * re-verified.
 */
/* Format a validation failure from `fmt` and the `...` values, then stop. Tests arm host_fail_env to catch it. */
static inline void asm_fail_fmt(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    host_fail(buf);
}

/* Mark `location` disposed. A later read (assert_type) or in-place store
 * (write_set_mov) aborts. write_init_mov and write_replace_mov clear the
 * mark because they establish a new value in that location. */
static inline void asm_dispose_location(const char* location) {
    if (location == NULL || location[0] == 0) return;
    if (asm_is_disposed(location)) return;
    if (asm_disposed_count >= ASM_DISPOSED_MAX) {
        asm_fail_fmt("error: too many disposed locations (max %d)", ASM_DISPOSED_MAX);
    }
    asm_disposed_loc[asm_disposed_count++] = location;
}

/* Stop when `loc` names a disposed cell, or a tracked tag other than the cell's declared type. */
static inline void assert_type(AsmDataField loc) {
    if (loc.name != NULL && asm_is_disposed(loc.name)) {
        asm_fail_fmt("error: use of disposed location \"%s\"", loc.name);
    }
    const char* have = asm_type_lookup(loc.name);
    if (have != NULL && loc.type != NULL && strcmp(have, loc.type) != 0) {
        asm_fail_fmt("error: type mismatch at \"%s\": expected \"%s\", but it holds \"%s\"",
                     loc.name, loc.type, have);
    }
}

/*
 * treat_as - Reinterpret a value's tag for a specific use, with a checked bridge.
 *
 * A value often needs to be used where a different tag is expected — most often a
 * constant, e.g. LOG_SIZE (tag "size") passed as a "dwMaxLow" argument. Rather
 * than silently relabelling it, treat_as(x, existing_tag, as_tag) asserts that x
 * really carries existing_tag — both as its declared type and, via assert_type,
 * against the type dictionaries — and returns a cell that validation will treat
 * as as_tag. The returned cell keeps x's location and real type but carries
 * use_as = as_tag, so downstream checks (call arguments, moves) compare against
 * as_tag while the original type is preserved for the record. This is the only
 * sanctioned way to cross tag boundaries, and it documents (and checks) the
 * assumption at the point it is made.
 */
static inline AsmDataField treat_as(AsmDataField x, const char* existing_tag, const char* as_tag) {
    if (x.type == NULL || existing_tag == NULL || strcmp(x.type, existing_tag) != 0) {
        asm_fail_fmt("error: treat_as: \"%s\" has tag \"%s\", not \"%s\"",
                     x.name ? x.name : "(null)",
                     x.type ? x.type : "(none)",
                     existing_tag ? existing_tag : "(null)");
    }
    assert_type(x);
    AsmDataField out = x;
    out.use_as = as_tag;
    return out;
}

/* Emit an indented line of `a`, then a space and `b` when `b` has text. */
static inline void write_line2(const char* a, const char* b) {
    write_indent();
    write(a);
    if (b[0]) { write(" "); write(b); }
    write("\n");
}

/* Emit an indented line of mnemonic `a`, then operand `b` and operand `c` when each has text. */
static inline void write_line3(const char* a, const char* b, const char* c) {
    write_indent();
    write(a);
    if (b[0]) { write(" "); write(b); }
    if (c[0]) { write(", "); write(c); }
    write("\n");
}

/* Emit an indented line of mnemonic `a`, then operands `b`, `c`, and `d` when each has text. */
static inline void write_line4(const char* a, const char* b, const char* c, const char* d) {
    write_indent();
    write(a);
    if (b[0]) { write(" "); write(b); }
    if (c[0]) { write(", "); write(c); }
    if (d[0]) { write(", "); write(d); }
    write("\n");
}

/*
 * write_line_def - Emit a name-first definition line: "name directive operand".
 *
 * MASM data definitions and EQU/= take the symbol name first, followed by the
 * directive and the initializer (e.g. "msg db 'Hi',0", "SIZE equ 100h"). Unlike
 * write_line3 this inserts no comma between the directive and the operand, since
 * the operand itself may be a comma-separated initializer list.
 */
static inline void write_line_def(const char* name, const char* dir, const char* src) {
    write_indent();
    write(name);
    if (dir[0]) { write(" "); write(dir); }
    if (src[0]) { write(" "); write(src); }
    write("\n");
}

/* Copy s into the bump arena. Callers build operand text here so a later
 * write() still has the bytes after the generator's stack frame is gone. */
static inline char* asm_strdup(const char* s) {
    size_t n = 0;
    if (!s) s = "";
    while (s[n]) n++;
    char* d = (char*)asm_bump_alloc(n + 1);
    memcpy(d, s, n + 1);
    return d;
}

/* Decimal integer `v`, arena-backed. */
static inline const char* asm_i(int v) {
    char buf[16];
    snprintf(buf, sizeof buf, "%d", v);
    return asm_strdup(buf);
}

/* "qword ptr [base]" or "qword ptr [base+disp]" / "qword ptr [base-disp]".
 * width is the MASM size keyword: byte, word, dword, qword.
 * The sign is spelled out: host snprintf has no %+ flag. */
static inline const char* asm_mem(const char* width, const char* base, int disp) {
    char buf[96];
    if (disp == 0) snprintf(buf, sizeof buf, "%s ptr [%s]", width, base);
    else if (disp > 0) snprintf(buf, sizeof buf, "%s ptr [%s+%d]", width, base, disp);
    else snprintf(buf, sizeof buf, "%s ptr [%s%d]", width, base, disp);
    return asm_strdup(buf);
}

/* "`width` ptr [`base`+`index`*`scale`]" or with displacement `disp`. `scale` is 1, 2, 4, or 8. */
static inline const char* asm_mem_index(const char* width, const char* base,
                                       const char* index, int scale, int disp) {
    char buf[128];
    if (disp == 0)
        snprintf(buf, sizeof buf, "%s ptr [%s+%s*%d]", width, base, index, scale);
    else if (disp > 0)
        snprintf(buf, sizeof buf, "%s ptr [%s+%s*%d+%d]", width, base, index, scale, disp);
    else
        snprintf(buf, sizeof buf, "%s ptr [%s+%s*%d%d]", width, base, index, scale, disp);
    return asm_strdup(buf);
}

/* "`count` dup (`init`)" for DB/DW/DD/DQ. */
static inline const char* asm_dup(int count, const char* init) {
    char buf[80];
    snprintf(buf, sizeof buf, "%d dup (%s)", count, init);
    return asm_strdup(buf);
}

/* rsp % 16. Windows x64 enters a function at 8 (the call pushed a return
 * address). write_proc resets the tracker. Balanced sub/add pairs in
 * write_extern_call update it. MSVC's % truncates toward zero, so the pad
 * uses a non-negative remainder. Asm-level pushes are not auto-tracked:
 * the asm demos keep the entry remainder of 8, and their call frames stay
 * exactly the sizes they emitted before the tracker existed. */
static int asm_rsp_mod16 = 8;

/* Remainder of `x` modulo 16, in the range 0..15. A negative remainder is shifted up by 16. */
static inline int asm_mod16(int x) {
    int m = x % 16;
    if (m < 0) m += 16;
    return m;
}
/* Record rsp as 8 mod 16, the alignment Windows x64 has on function entry. */
static inline void asm_reset_rsp_align(void) { asm_rsp_mod16 = 8; }
/* Record a drop of rsp by `bytes` in the running alignment. */
static inline void asm_note_rsp_sub(int bytes) {
    asm_rsp_mod16 = asm_mod16(asm_rsp_mod16 - bytes);
}
/* Record a rise of rsp by `bytes` in the running alignment. */
static inline void asm_note_rsp_add(int bytes) {
    asm_rsp_mod16 = asm_mod16(asm_rsp_mod16 + bytes);
}

/* Smallest reservation >= min_bytes such that (rsp_mod - frame) % 16 == 0,
 * which is what CALL requires. When rsp_mod is 8 and min_bytes is 32, the
 * result is 40: the historical shadow-plus-pad frame. */
static inline int asm_call_frame_size(int min_bytes) {
    int frame = min_bytes;
    int have = asm_mod16(frame);
    int need = asm_rsp_mod16;
    if (have != need) frame += asm_mod16(need - have);
    return frame;
}

/* Non-temporal stores. Default off. asm_nontemporal_begin/end nest.
 * While the depth is positive, write_mov of a source with no '[' into a
 * destination that contains '[' emits movnti. movnti hints that the line
 * should not stay cached, which is what a streaming fill wants. The default
 * stays a normal mov because the next instruction usually reads the value
 * again. Language wrappers do not wrap the switch. A generator that wants the
 * measurement calls these two around the moves. Vector streaming (movntdq)
 * is a different opcode; this switch does not emit it. */
static int asm_nontemporal_depth = 0;
/* Enter one nested region where qualifying stores emit movnti. */
static inline void asm_nontemporal_begin(void) { asm_nontemporal_depth++; }
/* Leave one nested movnti region. Depth already at zero stays at zero. */
static inline void asm_nontemporal_end(void) {
    if (asm_nontemporal_depth > 0) asm_nontemporal_depth--;
}

/* Optional code emitted immediately after every CALL, from write_callv_target.
 * A null hook emits nothing. ex11 used to delay a watched callee from this
 * hook. It now wraps the function itself (causal_func_begin / causal_func_end)
 * so the delay, the record, and the trial live in that example's library.
 * The hook remains for a generator that wants a note after every call. */
typedef void (*AsmAfterCallHook)(const char* callee);
static AsmAfterCallHook asm_after_call_hook = 0;
/* Install `hook` to run after every CALL. A null `hook` emits nothing extra. */
static inline void asm_set_after_call_hook(AsmAfterCallHook hook) {
    asm_after_call_hook = hook;
}

/* Non-volatile write check. The mask is 0 unless the procedure opted in
 * (c_begin_function does, for the Windows x64 preserved set). A write to a
 * preserved register that has not been saved is a generation error. */
static unsigned asm_proc_preserved = 0;
static unsigned asm_proc_saved = 0;

/* Map GPR spelling `name` to its index 0..15, folding al/ax/eax onto rax
 * and the same for the other registers. A null or empty `name`, or any
 * spelling outside that table, returns -1. */
static inline int asm_gpr_index(const char* name) {
    static const char* names[] = {
        "al","ax","eax","rax", "cl","cx","ecx","rcx",
        "dl","dx","edx","rdx", "bl","bx","ebx","rbx",
        "spl","sp","esp","rsp", "bpl","bp","ebp","rbp",
        "sil","si","esi","rsi", "dil","di","edi","rdi",
        "r8b","r8w","r8d","r8", "r9b","r9w","r9d","r9",
        "r10b","r10w","r10d","r10", "r11b","r11w","r11d","r11",
        "r12b","r12w","r12d","r12", "r13b","r13w","r13d","r13",
        "r14b","r14w","r14d","r14", "r15b","r15w","r15d","r15"
    };
    if (name == NULL || name[0] == 0) return -1;
    if (strchr(name, '[') || strchr(name, ' ')) return -1;
    for (int i = 0; i < 64; i++) {
        if (strcmp(name, names[i]) == 0) return i / 4;
    }
    return -1;
}

/* Stop when GPR `idx` is in the preserved mask and has yet to be saved.
 * Any other `idx` leaves the masks alone. */
static inline void asm_note_write_reg(int idx) {
    if (asm_proc_preserved == 0 || idx < 0 || idx > 15) return;
    unsigned bit = 1u << idx;
    if ((asm_proc_preserved & bit) && !(asm_proc_saved & bit)) {
        crash_if(1, "write to a non-volatile register this procedure has not saved");
    }
}

/* Set the preserved-GPR mask for the open procedure to `mask`. */
static inline void asm_set_preserved(unsigned mask) { asm_proc_preserved = mask; }
/* Union `mask` into the preserved GPRs this procedure has already saved. */
static inline void asm_mark_saved(unsigned mask) { asm_proc_saved |= mask; }

/* Assembly form of commit_stack_pages. The generated image is linked
 * without the C runtime, so nothing provides __chkstk. Each step drops
 * 2500 bytes, which is less than a page, and touches both ends of that
 * chunk. The guard page is therefore committed rather than skipped. rsp
 * is left where it was. Clobbers r10 and r11, which are caller-saved.
 *
 * 420 steps is 420 * 2500 = 1,050,000 bytes, just over 1 MiB. The link
 * reserve is 2 MiB (/STACK:2097152), so the walk stays on the stack.
 * A new thread does not inherit the pages main committed, so a thread
 * entry calls this too. */
#define ASM_STACK_PROBE_STEPS 420
static int asm_commit_seq = 0;
/* Walk `steps` chunks of 2500 bytes down from rsp, leaving rsp unchanged.
 * A `steps` count below 1 is raised to 1. */
static inline void write_commit_stack(int steps) {
    if (steps < 1) steps = 1;
    char* nbuf = (char*)asm_bump_alloc(16);
    char* lbuf = (char*)asm_bump_alloc(40);
    snprintf(nbuf, 16, "%d", steps);
    snprintf(lbuf, 40, "stack_commit_%d", asm_commit_seq++);
    write_line3(ASM_MOV, "r10", "rsp");
    write_line3(ASM_MOV, "r11", nbuf);
    write_label(lbuf);
    write_line3(ASM_SUB, "r10", "2500");
    write_line3(ASM_MOV, "byte ptr [r10]", "0");
    write_line3(ASM_MOV, "byte ptr [r10+2499]", "0");
    write_line2(ASM_DEC, "r11");
    write_line2(ASM_JNZ, lbuf);
}


/*
 * asm.h
 * Exhaustive MASM x86/x64 register, instruction, directive, and keyword string table.
 * Each symbol exposes the lowercase MASM spelling for a documented token.
 */

/*
 * ============================================================================
 * 1. General Purpose Registers
 * ============================================================================
 */

/*
 * AL - accumulator family
 * AL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the AX/EAX/RAX family for integer return values, arithmetic
 * accumulators, and implicit multiply/divide operands
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov al, 0
 */
static inline void write_al(void) {
    write(ASM_AL);
}

/*
 * AH - accumulator family
 * AH is a 8-bit register operand recognized by MASM.
 * Typical use: the high byte of the low 16 bits of AX/EAX/RAX
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: AH, BH, CH, and DH cannot be encoded in instructions that require a REX prefix in
 * 64-bit mode.
 *
 * Example:
 *   mov ah, 0
 */
static inline void write_ah(void) {
    write(ASM_AH);
}

/*
 * AX - accumulator family
 * AX is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit arithmetic, addressing, or ABI-specific temporary storage related to
 * integer return values, arithmetic accumulators, and implicit multiply/divide operands
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ax, 0
 */
static inline void write_ax(void) {
    write(ASM_AX);
}

/*
 * EAX - accumulator family
 * EAX is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit arithmetic, argument passing, and zero-extending writes into the parent
 * 64-bit register in x64 mode
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov eax, 0
 */
static inline void write_eax(void) {
    write(ASM_EAX);
}

/*
 * RAX - accumulator family
 * RAX is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer operations and address calculations for integer return
 * values, arithmetic accumulators, and implicit multiply/divide operands
 * Relationship: RAX contains EAX, AX, AL, and AH. Writing EAX in 64-bit mode zero-extends into
 * RAX; writing AX, AL, or AH updates only the low portion.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rax, 0
 */
static inline void write_rax(void) {
    write(ASM_RAX);
}

/*
 * BL - base register family
 * BL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the BX/EBX/RBX family for base-address calculations and callee-
 * saved temporaries in common calling conventions
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov bl, 0
 */
static inline void write_bl(void) {
    write(ASM_BL);
}

/*
 * BH - base register family
 * BH is a 8-bit register operand recognized by MASM.
 * Typical use: the high byte of the low 16 bits of BX/EBX/RBX
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: AH, BH, CH, and DH cannot be encoded in instructions that require a REX prefix in
 * 64-bit mode.
 *
 * Example:
 *   mov bh, 0
 */
static inline void write_bh(void) {
    write(ASM_BH);
}

/*
 * BX - base register family
 * BX is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit arithmetic, addressing, or ABI-specific temporary storage related to base-
 * address calculations and callee-saved temporaries in common calling conventions
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov bx, 0
 */
static inline void write_bx(void) {
    write(ASM_BX);
}

/*
 * EBX - base register family
 * EBX is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit arithmetic, argument passing, and zero-extending writes into the parent
 * 64-bit register in x64 mode
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ebx, 0
 */
static inline void write_ebx(void) {
    write(ASM_EBX);
}

/*
 * RBX - base register family
 * RBX is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer operations and address calculations for base-address
 * calculations and callee-saved temporaries in common calling conventions
 * Relationship: RBX contains EBX, BX, BL, and BH. Writing EBX in 64-bit mode zero-extends into
 * RBX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rbx, 0
 */
static inline void write_rbx(void) {
    write(ASM_RBX);
}

/*
 * CL - count register family
 * CL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the CX/ECX/RCX family for loop counts, REP-prefixed string
 * counts, shifts that use CL, and volatile argument passing on many ABIs
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov cl, 0
 */
static inline void write_cl(void) {
    write(ASM_CL);
}

/*
 * CH - count register family
 * CH is a 8-bit register operand recognized by MASM.
 * Typical use: the high byte of the low 16 bits of CX/ECX/RCX
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: AH, BH, CH, and DH cannot be encoded in instructions that require a REX prefix in
 * 64-bit mode.
 *
 * Example:
 *   mov ch, 0
 */
static inline void write_ch(void) {
    write(ASM_CH);
}

/*
 * CX - count register family
 * CX is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit arithmetic, addressing, or ABI-specific temporary storage related to loop
 * counts, REP-prefixed string counts, shifts that use CL, and volatile argument passing on many
 * ABIs
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov cx, 0
 */
static inline void write_cx(void) {
    write(ASM_CX);
}

/*
 * ECX - count register family
 * ECX is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit arithmetic, argument passing, and zero-extending writes into the parent
 * 64-bit register in x64 mode
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ecx, 0
 */
static inline void write_ecx(void) {
    write(ASM_ECX);
}

/*
 * RCX - count register family
 * RCX is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer operations and address calculations for loop counts,
 * REP-prefixed string counts, shifts that use CL, and volatile argument passing on many ABIs
 * Relationship: RCX contains ECX, CX, CL, and CH. Writing ECX in 64-bit mode zero-extends into
 * RCX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rcx, 0
 */
static inline void write_rcx(void) {
    write(ASM_RCX);
}

/*
 * DL - data register family
 * DL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the DX/EDX/RDX family for high halves of multiply/divide results,
 * I/O port addressing, and volatile argument passing
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov dl, 0
 */
static inline void write_dl(void) {
    write(ASM_DL);
}

/*
 * DH - data register family
 * DH is a 8-bit register operand recognized by MASM.
 * Typical use: the high byte of the low 16 bits of DX/EDX/RDX
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: AH, BH, CH, and DH cannot be encoded in instructions that require a REX prefix in
 * 64-bit mode.
 *
 * Example:
 *   mov dh, 0
 */
static inline void write_dh(void) {
    write(ASM_DH);
}

/*
 * DX - data register family
 * DX is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit arithmetic, addressing, or ABI-specific temporary storage related to high
 * halves of multiply/divide results, I/O port addressing, and volatile argument passing
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov dx, 0
 */
static inline void write_dx(void) {
    write(ASM_DX);
}

/*
 * EDX - data register family
 * EDX is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit arithmetic, argument passing, and zero-extending writes into the parent
 * 64-bit register in x64 mode
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov edx, 0
 */
static inline void write_edx(void) {
    write(ASM_EDX);
}

/*
 * RDX - data register family
 * RDX is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer operations and address calculations for high halves of
 * multiply/divide results, I/O port addressing, and volatile argument passing
 * Relationship: RDX contains EDX, DX, DL, and DH. Writing EDX in 64-bit mode zero-extends into
 * RDX.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rdx, 0
 */
static inline void write_rdx(void) {
    write(ASM_RDX);
}

/*
 * SPL - stack-pointer family
 * SPL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the SP/ESP/RSP family for stack growth, PUSH/POP/CALL/RET, local
 * storage, and stack-based addressing
 * Relationship: RSP contains ESP, SP, and SPL. Writing ESP in 64-bit mode zero-extends into RSP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: SPL requires a REX prefix in 64-bit mode.
 *
 * Example:
 *   mov spl, 0
 */
static inline void write_spl(void) {
    write(ASM_SPL);
}

/*
 * SP - stack-pointer family
 * SP is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit forms of stack growth, PUSH/POP/CALL/RET, local storage, and stack-based
 * addressing
 * Relationship: RSP contains ESP, SP, and SPL. Writing ESP in 64-bit mode zero-extends into RSP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov sp, 0
 */
static inline void write_sp(void) {
    write(ASM_SP);
}

/*
 * ESP - stack-pointer family
 * ESP is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit forms of stack growth, PUSH/POP/CALL/RET, local storage, and stack-based
 * addressing; writes zero-extend into the 64-bit parent in long mode
 * Relationship: RSP contains ESP, SP, and SPL. Writing ESP in 64-bit mode zero-extends into RSP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov esp, 0
 */
static inline void write_esp(void) {
    write(ASM_ESP);
}

/*
 * RSP - stack-pointer family
 * RSP is a 64-bit register operand recognized by MASM.
 * Typical use: 64-bit forms of stack growth, PUSH/POP/CALL/RET, local storage, and stack-based
 * addressing
 * Relationship: RSP contains ESP, SP, and SPL. Writing ESP in 64-bit mode zero-extends into RSP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rsp, 0
 */
static inline void write_rsp(void) {
    write(ASM_RSP);
}

/*
 * BPL - frame-pointer family
 * BPL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the BP/EBP/RBP family for stack-frame anchoring, local variable
 * addressing, and nonvolatile storage in many calling conventions
 * Relationship: RBP contains EBP, BP, and BPL. Writing EBP in 64-bit mode zero-extends into RBP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: BPL requires a REX prefix in 64-bit mode.
 *
 * Example:
 *   mov bpl, 0
 */
static inline void write_bpl(void) {
    write(ASM_BPL);
}

/*
 * BP - frame-pointer family
 * BP is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit forms of stack-frame anchoring, local variable addressing, and nonvolatile
 * storage in many calling conventions
 * Relationship: RBP contains EBP, BP, and BPL. Writing EBP in 64-bit mode zero-extends into RBP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov bp, 0
 */
static inline void write_bp(void) {
    write(ASM_BP);
}

/*
 * EBP - frame-pointer family
 * EBP is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit forms of stack-frame anchoring, local variable addressing, and nonvolatile
 * storage in many calling conventions; writes zero-extend into the 64-bit parent in long mode
 * Relationship: RBP contains EBP, BP, and BPL. Writing EBP in 64-bit mode zero-extends into RBP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ebp, 0
 */
static inline void write_ebp(void) {
    write(ASM_EBP);
}

/*
 * RBP - frame-pointer family
 * RBP is a 64-bit register operand recognized by MASM.
 * Typical use: 64-bit forms of stack-frame anchoring, local variable addressing, and nonvolatile
 * storage in many calling conventions
 * Relationship: RBP contains EBP, BP, and BPL. Writing EBP in 64-bit mode zero-extends into RBP.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rbp, 0
 */
static inline void write_rbp(void) {
    write(ASM_RBP);
}

/*
 * SIL - source-index family
 * SIL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the SI/ESI/RSI family for source addresses for string
 * instructions and general pointer arithmetic
 * Relationship: RSI contains ESI, SI, and SIL. Writing ESI in 64-bit mode zero-extends into RSI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: SIL requires a REX prefix in 64-bit mode.
 *
 * Example:
 *   mov sil, 0
 */
static inline void write_sil(void) {
    write(ASM_SIL);
}

/*
 * SI - source-index family
 * SI is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit forms of source addresses for string instructions and general pointer
 * arithmetic
 * Relationship: RSI contains ESI, SI, and SIL. Writing ESI in 64-bit mode zero-extends into RSI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov si, 0
 */
static inline void write_si(void) {
    write(ASM_SI);
}

/*
 * ESI - source-index family
 * ESI is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit forms of source addresses for string instructions and general pointer
 * arithmetic; writes zero-extend into the 64-bit parent in long mode
 * Relationship: RSI contains ESI, SI, and SIL. Writing ESI in 64-bit mode zero-extends into RSI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov esi, 0
 */
static inline void write_esi(void) {
    write(ASM_ESI);
}

/*
 * RSI - source-index family
 * RSI is a 64-bit register operand recognized by MASM.
 * Typical use: 64-bit forms of source addresses for string instructions and general pointer
 * arithmetic
 * Relationship: RSI contains ESI, SI, and SIL. Writing ESI in 64-bit mode zero-extends into RSI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rsi, 0
 */
static inline void write_rsi(void) {
    write(ASM_RSI);
}

/*
 * DIL - destination-index family
 * DIL is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the DI/EDI/RDI family for destination addresses for string
 * instructions and general pointer arithmetic
 * Relationship: RDI contains EDI, DI, and DIL. Writing EDI in 64-bit mode zero-extends into RDI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DIL requires a REX prefix in 64-bit mode.
 *
 * Example:
 *   mov dil, 0
 */
static inline void write_dil(void) {
    write(ASM_DIL);
}

/*
 * DI - destination-index family
 * DI is a 16-bit register operand recognized by MASM.
 * Typical use: 16-bit forms of destination addresses for string instructions and general pointer
 * arithmetic
 * Relationship: RDI contains EDI, DI, and DIL. Writing EDI in 64-bit mode zero-extends into RDI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov di, 0
 */
static inline void write_di(void) {
    write(ASM_DI);
}

/*
 * EDI - destination-index family
 * EDI is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit forms of destination addresses for string instructions and general pointer
 * arithmetic; writes zero-extend into the 64-bit parent in long mode
 * Relationship: RDI contains EDI, DI, and DIL. Writing EDI in 64-bit mode zero-extends into RDI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov edi, 0
 */
static inline void write_edi(void) {
    write(ASM_EDI);
}

/*
 * RDI - destination-index family
 * RDI is a 64-bit register operand recognized by MASM.
 * Typical use: 64-bit forms of destination addresses for string instructions and general pointer
 * arithmetic
 * Relationship: RDI contains EDI, DI, and DIL. Writing EDI in 64-bit mode zero-extends into RDI.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov rdi, 0
 */
static inline void write_rdi(void) {
    write(ASM_RDI);
}

/*
 * R8B - extended general-purpose register family R8
 * R8B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R8 register for general integer temporaries; on
 * Win64, R8 and R9 also carry the third and fourth integer or pointer arguments
 * Relationship: R8 contains R8D, R8W, and R8B. Writing R8D in 64-bit mode zero-extends into R8.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r8b, 0
 */
static inline void write_r8b(void) {
    write(ASM_R8B);
}

/*
 * R8W - extended general-purpose register family R8
 * R8W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R8 for address and integer work
 * Relationship: R8 contains R8D, R8W, and R8B. Writing R8D in 64-bit mode zero-extends into R8.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r8w, 0
 */
static inline void write_r8w(void) {
    write(ASM_R8W);
}

/*
 * R8D - extended general-purpose register family R8
 * R8D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R8; writes zero-extend into the parent register
 * Relationship: R8 contains R8D, R8W, and R8B. Writing R8D in 64-bit mode zero-extends into R8.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r8d, 0
 */
static inline void write_r8d(void) {
    write(ASM_R8D);
}

/*
 * R8 - extended general-purpose register family R8
 * R8 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R8
 * Relationship: R8 contains R8D, R8W, and R8B. Writing R8D in 64-bit mode zero-extends into R8.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r8, 0
 */
static inline void write_r8(void) {
    write(ASM_R8);
}

/*
 * R9B - extended general-purpose register family R9
 * R9B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R9 register for general integer temporaries; on
 * Win64, R8 and R9 also carry the third and fourth integer or pointer arguments
 * Relationship: R9 contains R9D, R9W, and R9B. Writing R9D in 64-bit mode zero-extends into R9.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r9b, 0
 */
static inline void write_r9b(void) {
    write(ASM_R9B);
}

/*
 * R9W - extended general-purpose register family R9
 * R9W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R9 for address and integer work
 * Relationship: R9 contains R9D, R9W, and R9B. Writing R9D in 64-bit mode zero-extends into R9.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r9w, 0
 */
static inline void write_r9w(void) {
    write(ASM_R9W);
}

/*
 * R9D - extended general-purpose register family R9
 * R9D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R9; writes zero-extend into the parent register
 * Relationship: R9 contains R9D, R9W, and R9B. Writing R9D in 64-bit mode zero-extends into R9.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r9d, 0
 */
static inline void write_r9d(void) {
    write(ASM_R9D);
}

/*
 * R9 - extended general-purpose register family R9
 * R9 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R9
 * Relationship: R9 contains R9D, R9W, and R9B. Writing R9D in 64-bit mode zero-extends into R9.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r9, 0
 */
static inline void write_r9(void) {
    write(ASM_R9);
}

/*
 * R10B - extended general-purpose register family R10
 * R10B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R10 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R10 contains R10D, R10W, and R10B. Writing R10D in 64-bit mode zero-extends into
 * R10.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r10b, 0
 */
static inline void write_r10b(void) {
    write(ASM_R10B);
}

/*
 * R10W - extended general-purpose register family R10
 * R10W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R10 for address and integer work
 * Relationship: R10 contains R10D, R10W, and R10B. Writing R10D in 64-bit mode zero-extends into
 * R10.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r10w, 0
 */
static inline void write_r10w(void) {
    write(ASM_R10W);
}

/*
 * R10D - extended general-purpose register family R10
 * R10D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R10; writes zero-extend into the parent register
 * Relationship: R10 contains R10D, R10W, and R10B. Writing R10D in 64-bit mode zero-extends into
 * R10.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r10d, 0
 */
static inline void write_r10d(void) {
    write(ASM_R10D);
}

/*
 * R10 - extended general-purpose register family R10
 * R10 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R10
 * Relationship: R10 contains R10D, R10W, and R10B. Writing R10D in 64-bit mode zero-extends into
 * R10.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r10, 0
 */
static inline void write_r10(void) {
    write(ASM_R10);
}

/*
 * R11B - extended general-purpose register family R11
 * R11B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R11 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R11 contains R11D, R11W, and R11B. Writing R11D in 64-bit mode zero-extends into
 * R11.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r11b, 0
 */
static inline void write_r11b(void) {
    write(ASM_R11B);
}

/*
 * R11W - extended general-purpose register family R11
 * R11W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R11 for address and integer work
 * Relationship: R11 contains R11D, R11W, and R11B. Writing R11D in 64-bit mode zero-extends into
 * R11.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r11w, 0
 */
static inline void write_r11w(void) {
    write(ASM_R11W);
}

/*
 * R11D - extended general-purpose register family R11
 * R11D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R11; writes zero-extend into the parent register
 * Relationship: R11 contains R11D, R11W, and R11B. Writing R11D in 64-bit mode zero-extends into
 * R11.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r11d, 0
 */
static inline void write_r11d(void) {
    write(ASM_R11D);
}

/*
 * R11 - extended general-purpose register family R11
 * R11 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R11
 * Relationship: R11 contains R11D, R11W, and R11B. Writing R11D in 64-bit mode zero-extends into
 * R11.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r11, 0
 */
static inline void write_r11(void) {
    write(ASM_R11);
}

/*
 * R12B - extended general-purpose register family R12
 * R12B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R12 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R12 contains R12D, R12W, and R12B. Writing R12D in 64-bit mode zero-extends into
 * R12.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r12b, 0
 */
static inline void write_r12b(void) {
    write(ASM_R12B);
}

/*
 * R12W - extended general-purpose register family R12
 * R12W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R12 for address and integer work
 * Relationship: R12 contains R12D, R12W, and R12B. Writing R12D in 64-bit mode zero-extends into
 * R12.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r12w, 0
 */
static inline void write_r12w(void) {
    write(ASM_R12W);
}

/*
 * R12D - extended general-purpose register family R12
 * R12D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R12; writes zero-extend into the parent register
 * Relationship: R12 contains R12D, R12W, and R12B. Writing R12D in 64-bit mode zero-extends into
 * R12.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r12d, 0
 */
static inline void write_r12d(void) {
    write(ASM_R12D);
}

/*
 * R12 - extended general-purpose register family R12
 * R12 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R12
 * Relationship: R12 contains R12D, R12W, and R12B. Writing R12D in 64-bit mode zero-extends into
 * R12.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r12, 0
 */
static inline void write_r12(void) {
    write(ASM_R12);
}

/*
 * R13B - extended general-purpose register family R13
 * R13B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R13 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R13 contains R13D, R13W, and R13B. Writing R13D in 64-bit mode zero-extends into
 * R13.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r13b, 0
 */
static inline void write_r13b(void) {
    write(ASM_R13B);
}

/*
 * R13W - extended general-purpose register family R13
 * R13W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R13 for address and integer work
 * Relationship: R13 contains R13D, R13W, and R13B. Writing R13D in 64-bit mode zero-extends into
 * R13.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r13w, 0
 */
static inline void write_r13w(void) {
    write(ASM_R13W);
}

/*
 * R13D - extended general-purpose register family R13
 * R13D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R13; writes zero-extend into the parent register
 * Relationship: R13 contains R13D, R13W, and R13B. Writing R13D in 64-bit mode zero-extends into
 * R13.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r13d, 0
 */
static inline void write_r13d(void) {
    write(ASM_R13D);
}

/*
 * R13 - extended general-purpose register family R13
 * R13 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R13
 * Relationship: R13 contains R13D, R13W, and R13B. Writing R13D in 64-bit mode zero-extends into
 * R13.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r13, 0
 */
static inline void write_r13(void) {
    write(ASM_R13);
}

/*
 * R14B - extended general-purpose register family R14
 * R14B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R14 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R14 contains R14D, R14W, and R14B. Writing R14D in 64-bit mode zero-extends into
 * R14.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r14b, 0
 */
static inline void write_r14b(void) {
    write(ASM_R14B);
}

/*
 * R14W - extended general-purpose register family R14
 * R14W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R14 for address and integer work
 * Relationship: R14 contains R14D, R14W, and R14B. Writing R14D in 64-bit mode zero-extends into
 * R14.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r14w, 0
 */
static inline void write_r14w(void) {
    write(ASM_R14W);
}

/*
 * R14D - extended general-purpose register family R14
 * R14D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R14; writes zero-extend into the parent register
 * Relationship: R14 contains R14D, R14W, and R14B. Writing R14D in 64-bit mode zero-extends into
 * R14.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r14d, 0
 */
static inline void write_r14d(void) {
    write(ASM_R14D);
}

/*
 * R14 - extended general-purpose register family R14
 * R14 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R14
 * Relationship: R14 contains R14D, R14W, and R14B. Writing R14D in 64-bit mode zero-extends into
 * R14.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r14, 0
 */
static inline void write_r14(void) {
    write(ASM_R14);
}

/*
 * R15B - extended general-purpose register family R15
 * R15B is a 8-bit register operand recognized by MASM.
 * Typical use: the low byte of the extended R15 register for general integer temporaries; many
 * calling conventions treat these registers as volatile scratch registers, while R12-R15 are
 * commonly nonvolatile
 * Relationship: R15 contains R15D, R15W, and R15B. Writing R15D in 64-bit mode zero-extends into
 * R15.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r15b, 0
 */
static inline void write_r15b(void) {
    write(ASM_R15B);
}

/*
 * R15W - extended general-purpose register family R15
 * R15W is a 16-bit register operand recognized by MASM.
 * Typical use: the low 16 bits of R15 for address and integer work
 * Relationship: R15 contains R15D, R15W, and R15B. Writing R15D in 64-bit mode zero-extends into
 * R15.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r15w, 0
 */
static inline void write_r15w(void) {
    write(ASM_R15W);
}

/*
 * R15D - extended general-purpose register family R15
 * R15D is a 32-bit register operand recognized by MASM.
 * Typical use: 32-bit operations on R15; writes zero-extend into the parent register
 * Relationship: R15 contains R15D, R15W, and R15B. Writing R15D in 64-bit mode zero-extends into
 * R15.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r15d, 0
 */
static inline void write_r15d(void) {
    write(ASM_R15D);
}

/*
 * R15 - extended general-purpose register family R15
 * R15 is a 64-bit register operand recognized by MASM.
 * Typical use: full-width 64-bit integer and pointer operations using R15
 * Relationship: R15 contains R15D, R15W, and R15B. Writing R15D in 64-bit mode zero-extends into
 * R15.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov r15, 0
 */
static inline void write_r15(void) {
    write(ASM_R15);
}

/*
 * ============================================================================
 * 2. Segment Registers
 * ============================================================================
 */

/*
 * CS - Code Segment
 * CS is a 16-bit register operand recognized by MASM.
 * Typical use: selects the current code segment in segmented modes and pairs with RIP/EIP/IP for
 * instruction fetch
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CS is generally changed by far control transfers, privilege transitions, or system
 * mechanisms rather than by ordinary arithmetic code.
 *
 * Example:
 *   mov cs, 0
 */
static inline void write_cs(void) {
    write(ASM_CS);
}

/*
 * DS - Data Segment
 * DS is a 16-bit register operand recognized by MASM.
 * Typical use: provides the default data segment for many legacy memory references
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: In flat 32-bit or 64-bit models, DS usually holds a flat selector and is less visible
 * in source code.
 *
 * Example:
 *   mov ds, 0
 */
static inline void write_ds(void) {
    write(ASM_DS);
}

/*
 * ES - Extra Segment
 * ES is a 16-bit register operand recognized by MASM.
 * Typical use: historically supplies the destination segment for string operations such as MOVS,
 * STOS, SCAS, and CMPS
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Many string forms implicitly use ES for the destination in legacy modes.
 *
 * Example:
 *   mov es, 0
 */
static inline void write_es(void) {
    write(ASM_ES);
}

/*
 * FS - FS Segment
 * FS is a 16-bit register operand recognized by MASM.
 * Typical use: often points at thread-local or processor-local data structures
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: On Windows, FS is commonly used for the TEB in 32-bit mode.
 *
 * Example:
 *   mov fs, 0
 */
static inline void write_fs(void) {
    write(ASM_FS);
}

/*
 * GS - GS Segment
 * GS is a 16-bit register operand recognized by MASM.
 * Typical use: often points at thread-local or processor-local data structures in long mode
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: On Windows x64, GS commonly references the TEB/PEB chain and kernel swapgs state.
 *
 * Example:
 *   mov gs, 0
 */
static inline void write_gs(void) {
    write(ASM_GS);
}

/*
 * SS - Stack Segment
 * SS is a 16-bit register operand recognized by MASM.
 * Typical use: defines the stack segment used by stack references in segmented execution modes
 * Relationship: Segment registers are separate from the general-purpose register file and hold
 * selectors rather than ordinary integer values.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Changing SS has ordering and interrupt-shadow rules on x86 and is generally privileged
 * or carefully controlled.
 *
 * Example:
 *   mov ss, 0
 */
static inline void write_ss(void) {
    write(ASM_SS);
}

/*
 * ============================================================================
 * 3. Special Registers
 * ============================================================================
 */

/*
 * RIP - Instruction Pointer
 * RIP is a 64-bit register operand recognized by MASM.
 * Typical use: holds the address of the next instruction in 64-bit mode and supports RIP-
 * relative addressing for position-independent code
 * Relationship: RIP is updated implicitly by sequential execution, branches, calls, returns,
 * interrupts, and exceptions.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: You cannot encode RIP as a general destination register; it is referenced implicitly or
 * through RIP-relative memory operands.
 *
 * Example:
 *   mov rip, 0
 */
static inline void write_rip(void) {
    write(ASM_RIP);
}

/*
 * EIP - Instruction Pointer
 * EIP is a 32-bit register operand recognized by MASM.
 * Typical use: holds the next instruction address in 32-bit modes
 * Relationship: EIP is modified by control-transfer instructions rather than ordinary MOV
 * instructions.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov eip, 0
 */
static inline void write_eip(void) {
    write(ASM_EIP);
}

/*
 * IP - Instruction Pointer
 * IP is a 16-bit register operand recognized by MASM.
 * Typical use: holds the next instruction offset in 16-bit modes
 * Relationship: IP pairs with CS in segmented addressing.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov ip, 0
 */
static inline void write_ip(void) {
    write(ASM_IP);
}

/*
 * RFLAGS - Flags Register
 * RFLAGS is a 64-bit register operand recognized by MASM.
 * Typical use: contains status flags, control flags, and system flags for 64-bit execution
 * Relationship: RFLAGS extends EFLAGS/FLAGS; arithmetic instructions update status bits such as
 * CF, ZF, SF, OF, AF, and PF.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Access is usually via PUSHFQ/POPFQ, LAHF/SAHF, or privileged mechanisms rather than
 * direct MOV.
 *
 * Example:
 *   mov rflags, 0
 */
static inline void write_rflags(void) {
    write(ASM_RFLAGS);
}

/*
 * EFLAGS - Flags Register
 * EFLAGS is a 32-bit register operand recognized by MASM.
 * Typical use: contains status and control flags in 32-bit modes
 * Relationship: EFLAGS is the low 32 bits of RFLAGS.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov eflags, 0
 */
static inline void write_eflags(void) {
    write(ASM_EFLAGS);
}

/*
 * FLAGS - Flags Register
 * FLAGS is a 16-bit register operand recognized by MASM.
 * Typical use: contains the architectural low 16 bits of the flags register in 16-bit code
 * Relationship: FLAGS is the low 16-bit subset of EFLAGS/RFLAGS.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 *
 * Example:
 *   mov flags, 0
 */
static inline void write_flags(void) {
    write(ASM_FLAGS);
}

/*
 * CR0 - Control Register
 * CR0 is a 64-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR0 enables and reports major processor modes such as protected mode, paging, task
 * switching state, and cache behavior.
 *
 * Example:
 *   mov cr0, 0
 */
static inline void write_cr0(void) {
    write(ASM_CR0);
}

/*
 * CR1 - Control Register
 * CR1 is a 32-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR1 is reserved on modern x86 CPUs and is not used as a general architectural control
 * register.
 *
 * Example:
 *   mov cr1, 0
 */
static inline void write_cr1(void) {
    write(ASM_CR1);
}

/*
 * CR2 - Control Register
 * CR2 is a 64-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR2 receives the linear address that caused the most recent page fault.
 *
 * Example:
 *   mov cr2, 0
 */
static inline void write_cr2(void) {
    write(ASM_CR2);
}

/*
 * CR3 - Control Register
 * CR3 is a 64-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR3 holds the page-table root for paging hardware; on PCID-capable CPUs it also carries
 * process-context identifiers.
 *
 * Example:
 *   mov cr3, 0
 */
static inline void write_cr3(void) {
    write(ASM_CR3);
}

/*
 * CR4 - Control Register
 * CR4 is a 64-bit register operand recognized by MASM.
 * Typical use: processor control state used by operating systems, hypervisors, and low-level
 * runtime startup code
 * Relationship: Control registers are separate from general registers and are accessed only by
 * privileged MOV-to/from-control-register forms.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: CR4 enables optional architectural features such as PAE, PGE, OSFXSR, SMEP, SMAP, and
 * more.
 *
 * Example:
 *   mov cr4, 0
 */
static inline void write_cr4(void) {
    write(ASM_CR4);
}

/*
 * DR0 - Debug Register
 * DR0 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR0 holds a linear address for one of the hardware breakpoint slots.
 *
 * Example:
 *   mov dr0, 0
 */
static inline void write_dr0(void) {
    write(ASM_DR0);
}

/*
 * DR1 - Debug Register
 * DR1 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR1 holds a linear address for one of the hardware breakpoint slots.
 *
 * Example:
 *   mov dr1, 0
 */
static inline void write_dr1(void) {
    write(ASM_DR1);
}

/*
 * DR2 - Debug Register
 * DR2 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR2 holds a linear address for one of the hardware breakpoint slots.
 *
 * Example:
 *   mov dr2, 0
 */
static inline void write_dr2(void) {
    write(ASM_DR2);
}

/*
 * DR3 - Debug Register
 * DR3 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR3 holds a linear address for one of the hardware breakpoint slots.
 *
 * Example:
 *   mov dr3, 0
 */
static inline void write_dr3(void) {
    write(ASM_DR3);
}

/*
 * DR4 - Debug Register
 * DR4 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR4 is obsolete and may alias DR6 on old processors when debug extensions are disabled;
 * modern software should avoid it.
 *
 * Example:
 *   mov dr4, 0
 */
static inline void write_dr4(void) {
    write(ASM_DR4);
}

/*
 * DR5 - Debug Register
 * DR5 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR5 is obsolete and may alias DR7 on old processors when debug extensions are disabled;
 * modern software should avoid it.
 *
 * Example:
 *   mov dr5, 0
 */
static inline void write_dr5(void) {
    write(ASM_DR5);
}

/*
 * DR6 - Debug Register
 * DR6 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR6 reports debug-status results such as which breakpoint fired.
 *
 * Example:
 *   mov dr6, 0
 */
static inline void write_dr6(void) {
    write(ASM_DR6);
}

/*
 * DR7 - Debug Register
 * DR7 is a 64-bit register operand recognized by MASM.
 * Typical use: hardware breakpoints, watchpoints, and debug status/control in privileged or
 * debugger-aware code
 * Relationship: Debug registers are not general arithmetic registers; they are read and written
 * with MOV DRx forms under privilege checks.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: DR7 controls breakpoint enables, trigger types, and sizes for the hardware debug
 * registers.
 *
 * Example:
 *   mov dr7, 0
 */
static inline void write_dr7(void) {
    write(ASM_DR7);
}

/*
 * ============================================================================
 * 4. FPU Registers
 * ============================================================================
 */

/*
 * ST0 - x87 floating-point stack register
 * ST0 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(0) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   mov st(0), 0
 */
static inline void write_st0(void) {
    write(ASM_ST0);
}

/*
 * ST1 - x87 floating-point stack register
 * ST1 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(1) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   mov st(1), 0
 */
static inline void write_st1(void) {
    write(ASM_ST1);
}

/*
 * ST2 - x87 floating-point stack register
 * ST2 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(2) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   mov st(2), 0
 */
static inline void write_st2(void) {
    write(ASM_ST2);
}

/*
 * ST3 - x87 floating-point stack register
 * ST3 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(3) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   mov st(3), 0
 */
static inline void write_st3(void) {
    write(ASM_ST3);
}

/*
 * ST4 - x87 floating-point stack register
 * ST4 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(4) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   mov st(4), 0
 */
static inline void write_st4(void) {
    write(ASM_ST4);
}

/*
 * ST5 - x87 floating-point stack register
 * ST5 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(5) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   mov st(5), 0
 */
static inline void write_st5(void) {
    write(ASM_ST5);
}

/*
 * ST6 - x87 floating-point stack register
 * ST6 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(6) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   mov st(6), 0
 */
static inline void write_st6(void) {
    write(ASM_ST6);
}

/*
 * ST7 - x87 floating-point stack register
 * ST7 is a 80-bit register operand recognized by MASM.
 * Typical use: extended-precision floating-point values, transcendental results, and x87 stack-
 * based arithmetic
 * Relationship: ST(7) is one logical element of the eight-entry x87 register stack. ST(0) is the
 * top of stack; pushes and pops rename which physical register is seen as each ST(i).
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Most x87 instructions address ST(0) implicitly and optionally another ST(i) operand.
 *
 * Example:
 *   mov st(7), 0
 */
static inline void write_st7(void) {
    write(ASM_ST7);
}

/*
 * ============================================================================
 * 5. SIMD Registers
 * ============================================================================
 */

/*
 * XMM0 - SIMD vector register
 * XMM0 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM0 is the low 128-bit lane of YMM0. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm0, 0
 */
static inline void write_xmm0(void) {
    write(ASM_XMM0);
}

/*
 * XMM1 - SIMD vector register
 * XMM1 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM1 is the low 128-bit lane of YMM1. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm1, 0
 */
static inline void write_xmm1(void) {
    write(ASM_XMM1);
}

/*
 * XMM2 - SIMD vector register
 * XMM2 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM2 is the low 128-bit lane of YMM2. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm2, 0
 */
static inline void write_xmm2(void) {
    write(ASM_XMM2);
}

/*
 * XMM3 - SIMD vector register
 * XMM3 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM3 is the low 128-bit lane of YMM3. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm3, 0
 */
static inline void write_xmm3(void) {
    write(ASM_XMM3);
}

/*
 * XMM4 - SIMD vector register
 * XMM4 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM4 is the low 128-bit lane of YMM4. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm4, 0
 */
static inline void write_xmm4(void) {
    write(ASM_XMM4);
}

/*
 * XMM5 - SIMD vector register
 * XMM5 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM5 is the low 128-bit lane of YMM5. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm5, 0
 */
static inline void write_xmm5(void) {
    write(ASM_XMM5);
}

/*
 * XMM6 - SIMD vector register
 * XMM6 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM6 is the low 128-bit lane of YMM6. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm6, 0
 */
static inline void write_xmm6(void) {
    write(ASM_XMM6);
}

/*
 * XMM7 - SIMD vector register
 * XMM7 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM7 is the low 128-bit lane of YMM7. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm7, 0
 */
static inline void write_xmm7(void) {
    write(ASM_XMM7);
}

/*
 * XMM8 - SIMD vector register
 * XMM8 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM8 is the low 128-bit lane of YMM8. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm8, 0
 */
static inline void write_xmm8(void) {
    write(ASM_XMM8);
}

/*
 * XMM9 - SIMD vector register
 * XMM9 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM9 is the low 128-bit lane of YMM9. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm9, 0
 */
static inline void write_xmm9(void) {
    write(ASM_XMM9);
}

/*
 * XMM10 - SIMD vector register
 * XMM10 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM10 is the low 128-bit lane of YMM10. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm10, 0
 */
static inline void write_xmm10(void) {
    write(ASM_XMM10);
}

/*
 * XMM11 - SIMD vector register
 * XMM11 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM11 is the low 128-bit lane of YMM11. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm11, 0
 */
static inline void write_xmm11(void) {
    write(ASM_XMM11);
}

/*
 * XMM12 - SIMD vector register
 * XMM12 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM12 is the low 128-bit lane of YMM12. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm12, 0
 */
static inline void write_xmm12(void) {
    write(ASM_XMM12);
}

/*
 * XMM13 - SIMD vector register
 * XMM13 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM13 is the low 128-bit lane of YMM13. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm13, 0
 */
static inline void write_xmm13(void) {
    write(ASM_XMM13);
}

/*
 * XMM14 - SIMD vector register
 * XMM14 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM14 is the low 128-bit lane of YMM14. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm14, 0
 */
static inline void write_xmm14(void) {
    write(ASM_XMM14);
}

/*
 * XMM15 - SIMD vector register
 * XMM15 is a 128-bit register operand recognized by MASM.
 * Typical use: packed single-precision, double-precision, integer, and scalar SSE/SSE2 operands
 * Relationship: XMM15 is the low 128-bit lane of YMM15. Legacy SSE uses only the XMM width; VEX-
 * encoded forms may preserve or clear upper state depending on the instruction.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Used by SSE, SSE2, AES, PCLMULQDQ, and AVX scalar/vector instructions.
 *
 * Example:
 *   mov xmm15, 0
 */
static inline void write_xmm15(void) {
    write(ASM_XMM15);
}

/*
 * YMM0 - AVX vector register
 * YMM0 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM0 extends XMM0; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm0, 0
 */
static inline void write_ymm0(void) {
    write(ASM_YMM0);
}

/*
 * YMM1 - AVX vector register
 * YMM1 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM1 extends XMM1; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm1, 0
 */
static inline void write_ymm1(void) {
    write(ASM_YMM1);
}

/*
 * YMM2 - AVX vector register
 * YMM2 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM2 extends XMM2; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm2, 0
 */
static inline void write_ymm2(void) {
    write(ASM_YMM2);
}

/*
 * YMM3 - AVX vector register
 * YMM3 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM3 extends XMM3; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm3, 0
 */
static inline void write_ymm3(void) {
    write(ASM_YMM3);
}

/*
 * YMM4 - AVX vector register
 * YMM4 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM4 extends XMM4; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm4, 0
 */
static inline void write_ymm4(void) {
    write(ASM_YMM4);
}

/*
 * YMM5 - AVX vector register
 * YMM5 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM5 extends XMM5; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm5, 0
 */
static inline void write_ymm5(void) {
    write(ASM_YMM5);
}

/*
 * YMM6 - AVX vector register
 * YMM6 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM6 extends XMM6; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm6, 0
 */
static inline void write_ymm6(void) {
    write(ASM_YMM6);
}

/*
 * YMM7 - AVX vector register
 * YMM7 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM7 extends XMM7; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm7, 0
 */
static inline void write_ymm7(void) {
    write(ASM_YMM7);
}

/*
 * YMM8 - AVX vector register
 * YMM8 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM8 extends XMM8; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm8, 0
 */
static inline void write_ymm8(void) {
    write(ASM_YMM8);
}

/*
 * YMM9 - AVX vector register
 * YMM9 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM9 extends XMM9; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm9, 0
 */
static inline void write_ymm9(void) {
    write(ASM_YMM9);
}

/*
 * YMM10 - AVX vector register
 * YMM10 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM10 extends XMM10; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm10, 0
 */
static inline void write_ymm10(void) {
    write(ASM_YMM10);
}

/*
 * YMM11 - AVX vector register
 * YMM11 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM11 extends XMM11; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm11, 0
 */
static inline void write_ymm11(void) {
    write(ASM_YMM11);
}

/*
 * YMM12 - AVX vector register
 * YMM12 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM12 extends XMM12; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm12, 0
 */
static inline void write_ymm12(void) {
    write(ASM_YMM12);
}

/*
 * YMM13 - AVX vector register
 * YMM13 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM13 extends XMM13; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm13, 0
 */
static inline void write_ymm13(void) {
    write(ASM_YMM13);
}

/*
 * YMM14 - AVX vector register
 * YMM14 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM14 extends XMM14; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm14, 0
 */
static inline void write_ymm14(void) {
    write(ASM_YMM14);
}

/*
 * YMM15 - AVX vector register
 * YMM15 is a 256-bit register operand recognized by MASM.
 * Typical use: packed 256-bit AVX operands with two 128-bit lanes or a full 256-bit
 * integer/floating vector
 * Relationship: YMM15 extends XMM15; the low 128 bits are the corresponding XMM register view.
 * Syntax: use the register name directly as an operand.
 * Flags affected: None directly. Any flags changes come from the instruction that reads or
 * writes the register.
 * Notes: Mixing legacy SSE with AVX code can require VZEROUPPER or VZEROALL to avoid transition
 * penalties.
 *
 * Example:
 *   mov ymm15, 0
 */
static inline void write_ymm15(void) {
    write(ASM_YMM15);
}

/*
 * ============================================================================
 * 6. Data Movement Instructions
 * ============================================================================
 */

/*
 * MOV - Move Data
 * Copies the source operand into the destination operand without memory-to-memory transfer and
 * without altering status flags.
 *
 * Syntax: MOV dest, src
 * Operands: destination register or memory, source register, memory, immediate, or certain
 * special-register forms
 * Flags affected: None
 * Notes: Operands must have compatible sizes. Segment, control, and debug-register forms have
 * architectural restrictions. MASM selects the proper encoding from operand types.
 *
 * Example:
 *   mov eax, 42
 *   mov [rbp-4], eax
 *
 * The trailing `src_type` argument tags the logical meaning/type of the source
 * input (typically the variable name), mirroring how write_call pairs a type
 * with each argument. The destination is write-only, so it carries no tag. The
 * tag is currently recorded only for readability and is otherwise unused.
 */
static inline void write_mov(const char* dest, const char* src, const char* src_type) {
    /* src_type names the logical meaning of the source for the reader. MOV
     * itself does not consult the type dictionary. write_init_mov,
     * write_set_mov, and write_replace_mov do. The cast keeps the parameter
     * without an unused-parameter warning. */
    (void)src_type;
    int g = asm_gpr_index(dest);
    if (g >= 0) asm_note_write_reg(g);
    if (asm_nontemporal_depth > 0 && dest && strchr(dest, '[') &&
        src && !strchr(src, '[')) {
        write_line3("movnti", dest, src);
        return;
    }
    write_line3(ASM_MOV, dest, src);
}

/*
 * write_set_mov - MOV that updates a location while keeping its logical meaning.
 *
 * Use this when the destination continues to hold the same conceptual value, it
 * is merely being recomputed/updated (e.g. clamping len to LOG_SIZE: the
 * destination is still "len" both before and after). Because the location's
 * meaning is preserved, both operands carry a type tag and they are typically
 * the same logical name. Both operands are passed as whole AsmDataField cells so
 * call sites read as write_set_mov(dest, src).
 *
 * Validation: the destination must already be a tracked location whose recorded
 * tag equals dest.type; a mismatch or an untracked destination aborts. The first
 * store to a fresh location must use write_init_mov instead. The source is also
 * validated with assert_type(src): if src names a tracked location, its recorded
 * tag must match src.type (untracked sources such as immediates pass silently).
 */
static inline void write_set_mov(AsmDataField dest, AsmDataField src) {
    if (dest.name != NULL && asm_is_disposed(dest.name)) {
        asm_fail_fmt("error: write_set_mov of disposed location \"%s\"", dest.name);
    }
    const char* have = asm_type_lookup(dest.name);
    if (have == NULL) {
        asm_fail_fmt("error: write_set_mov to untracked location \"%s\"; use write_init_mov for the first store",
                     dest.name);
    }
    if (dest.type != NULL && strcmp(have, dest.type) != 0) {
        asm_fail_fmt("error: write_set_mov to \"%s\": location holds \"%s\", not \"%s\"",
                     dest.name, have, dest.type);
    }
    assert_type(src);
    write_mov(dest.name, src.name, src.type);
}

/*
 * write_replace_mov - MOV that repurposes a location to hold something new.
 *
 * Use this when the destination is being reused to store a completely different
 * logical value than it held before (e.g. overwriting a scratch register with an
 * unrelated quantity, or reusing a register to hold a new temporary). The
 * destination is given as a bare location string (its old contents are
 * discarded, so it carries no incoming tag) and src is a whole AsmDataField
 * cell. After the move the destination holds src's value, so this returns the
 * destination as a cell {dest, src.type} — letting the call site both emit the
 * move and name the resulting cell in one line, e.g.
 * AsmDataField rsi_src = write_replace_mov("rsi", rcx_src). Only src's tag
 * documents the value being moved, exactly like write_mov.
 *
 * Validation: this repurposes the destination, so it unconditionally overwrites
 * the destination's entry in the local type dictionary with src's tag — later
 * reads of the destination will validate against this new meaning. Repurposing a
 * data global is forbidden (globals are read-only), so a global destination
 * aborts.
 */
static inline AsmDataField write_replace_mov(const char* dest, AsmDataField src) {
    if (asm_global_dict_get(dest) != NULL) {
        asm_fail_fmt("error: write_replace_mov cannot repurpose data global \"%s\" (globals are read-only)",
                     dest);
    }
    if (src.name != NULL && asm_is_disposed(src.name)) {
        asm_fail_fmt("error: use of disposed location \"%s\"", src.name);
    }
    /* dest is being repurposed, so the previous value's disposed mark
     * does not apply to the value this move establishes. */
    asm_disposed_forget(dest);
    const char* moved = src.type;
    write_mov(dest, src.name, src.type);
    asm_type_dict_set(dest, moved);
    AsmDataField out = {dest, moved};
    return out;
}

/*
 * write_init_mov - MOV that initializes a previously unused location.
 *
 * Completes the trio of semantic moves alongside write_set_mov (update, meaning
 * preserved) and write_replace_mov (repurpose an already-live location). Use it
 * for the first store to a location that is not yet tracked — typically the
 * initial store into a global once its value has been computed. The operands are
 * given plainly as write_init_mov(location, value, tag): value is the source
 * expression moved in, and tag is the logical type the location takes on.
 *
 * The location must be untracked in either dictionary; initializing an already
 * live local (use write_set_mov / write_replace_mov instead) or a data global
 * (which is pre-declared and updated with write_set_mov) is an error. It records
 * the local entry dict[location] = tag and returns the resulting cell
 * {location, tag}, so the move and the naming of the now-live cell fold into one
 * line, e.g. AsmDataField t = write_init_mov("rbx", rax_val.name, "count").
 */
static inline AsmDataField write_init_mov(const char* location, const char* value, const char* tag) {
    const char* have = asm_type_lookup(location);
    if (have != NULL) {
        asm_fail_fmt("error: write_init_mov to \"%s\": location already holds \"%s\"; use write_set_mov or write_replace_mov",
                     location, have);
    }
    asm_disposed_forget(location);
    write_mov(location, value, tag);
    asm_type_dict_set(location, tag);
    AsmDataField out = {location, tag};
    return out;
}

/*
 * MOVSX - Move with Sign Extension
 * Copies a smaller signed source into a larger destination and sign-extends the value to fill
 * the destination width.
 *
 * Syntax: MOVSX reg, r/m8|r/m16
 * Operands: destination register larger than the source; source register or memory
 * Flags affected: None
 * Notes: Useful when promoting signed byte or word values before arithmetic or comparisons.
 *
 * Example:
 *   movsx eax, byte ptr [rsi]
 *   movsx rdx, word ptr [rbx]
 */
static inline void write_movsx(const char* dest, const char* src) {
    write_line3(ASM_MOVSX, dest, src);
}

/*
 * MOVZX - Move with Zero Extension
 * Copies a smaller unsigned source into a larger destination and fills the new high bits with
 * zero.
 *
 * Syntax: MOVZX reg, r/m8|r/m16
 * Operands: destination register larger than the source; source register or memory
 * Flags affected: None
 * Notes: Use MOVZX when the source should be treated as an unsigned byte or word.
 *
 * Example:
 *   movzx eax, byte ptr [rsi]
 *   movzx r8d, word ptr [rbx]
 */
static inline void write_movzx(const char* dest, const char* src) {
    write_line3(ASM_MOVZX, dest, src);
}

/*
 * MOVSXD - Move with Sign Extension to 64 Bits
 * Sign-extends a 32-bit source into a 64-bit destination register.
 *
 * Syntax: MOVSXD reg64, r/m32
 * Operands: 64-bit destination register; 32-bit register or memory source
 * Flags affected: None
 * Notes: Primarily used in 64-bit code when converting signed 32-bit integers to 64-bit values.
 *
 * Example:
 *   movsxd rax, dword ptr [rcx]
 *   movsxd r10, eax
 */
static inline void write_movsxd(const char* dest, const char* src) {
    write_line3(ASM_MOVSXD, dest, src);
}

/*
 * LEA - Load Effective Address
 * Computes the address implied by a memory operand and writes that address into a register
 * without reading memory.
 *
 * Syntax: LEA reg, mem
 * Operands: destination general-purpose register; source memory-address expression
 * Flags affected: None
 * Notes: LEA is widely used for pointer arithmetic, indexing, and strength-reduced arithmetic
 * because only address calculation occurs.
 *
 * Example:
 *   lea rax, [rbx+rcx*4+16]
 *   lea edx, [eax+eax*2]
 */
static inline void write_lea(const char* dest, const char* src) {
    write_line3(ASM_LEA, dest, src);
}

/*
 * XCHG - Exchange Operands
 * Atomically exchanges the values of two operands. When one operand is memory, the processor
 * locks the exchange with respect to other agents.
 *
 * Syntax: XCHG op1, op2
 * Operands: register/register or register/memory operands of the same size
 * Flags affected: None
 * Notes: There is no memory-to-memory form. XCHG with AX/EAX/RAX and another register has
 * special short encodings.
 *
 * Example:
 *   xchg eax, edx
 *   xchg rax, [rsp]
 */
static inline void write_xchg(const char* dest, const char* src) {
    write_line3(ASM_XCHG, dest, src);
}

/*
 * BSWAP - Byte Swap
 * Reverses the byte order of a 32-bit or 64-bit register, converting between little-endian and
 * big-endian byte layouts.
 *
 * Syntax: BSWAP reg32|reg64
 * Operands: single 32-bit or 64-bit general-purpose register
 * Flags affected: None
 * Notes: There is no 8-bit or 16-bit form. Often used in serialization, checksums, and protocol
 * parsing.
 *
 * Example:
 *   bswap eax
 *   bswap rdx
 */
static inline void write_bswap(const char* operand) {
    write_line2(ASM_BSWAP, operand);
}

/*
 * CMOVA - Conditional Move if Above / unsigned greater
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 and ZF=0 is true in the current flags state.
 *
 * Syntax: CMOVA reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmova eax, ecx
 */
static inline void write_cmova(const char* dest, const char* src) {
    write_line3(ASM_CMOVA, dest, src);
}

/*
 * CMOVAE - Conditional Move if Above or Equal / unsigned >=
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 is true in the current flags state.
 *
 * Syntax: CMOVAE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovae eax, ecx
 */
static inline void write_cmovae(const char* dest, const char* src) {
    write_line3(ASM_CMOVAE, dest, src);
}

/*
 * CMOVB - Conditional Move if Below / unsigned <
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 is true in the current flags state.
 *
 * Syntax: CMOVB reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovb eax, ecx
 */
static inline void write_cmovb(const char* dest, const char* src) {
    write_line3(ASM_CMOVB, dest, src);
}

/*
 * CMOVBE - Conditional Move if Below or Equal / unsigned <=
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 or ZF=1 is true in the current flags state.
 *
 * Syntax: CMOVBE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovbe eax, ecx
 */
static inline void write_cmovbe(const char* dest, const char* src) {
    write_line3(ASM_CMOVBE, dest, src);
}

/*
 * CMOVC - Conditional Move if Carry set
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 is true in the current flags state.
 *
 * Syntax: CMOVC reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovc eax, ecx
 */
static inline void write_cmovc(const char* dest, const char* src) {
    write_line3(ASM_CMOVC, dest, src);
}

/*
 * CMOVE - Conditional Move if Equal / zero
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=1 is true in the current flags state.
 *
 * Syntax: CMOVE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmove eax, ecx
 */
static inline void write_cmove(const char* dest, const char* src) {
    write_line3(ASM_CMOVE, dest, src);
}

/*
 * CMOVG - Conditional Move if Greater / signed >
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=0 and SF=OF is true in the current flags state.
 *
 * Syntax: CMOVG reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovg eax, ecx
 */
static inline void write_cmovg(const char* dest, const char* src) {
    write_line3(ASM_CMOVG, dest, src);
}

/*
 * CMOVGE - Conditional Move if Greater or Equal / signed >=
 * Reads the source operand and copies it into the destination register only if the condition
 * SF=OF is true in the current flags state.
 *
 * Syntax: CMOVGE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovge eax, ecx
 */
static inline void write_cmovge(const char* dest, const char* src) {
    write_line3(ASM_CMOVGE, dest, src);
}

/*
 * CMOVL - Conditional Move if Less / signed <
 * Reads the source operand and copies it into the destination register only if the condition
 * SF!=OF is true in the current flags state.
 *
 * Syntax: CMOVL reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovl eax, ecx
 */
static inline void write_cmovl(const char* dest, const char* src) {
    write_line3(ASM_CMOVL, dest, src);
}

/*
 * CMOVLE - Conditional Move if Less or Equal / signed <=
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=1 or SF!=OF is true in the current flags state.
 *
 * Syntax: CMOVLE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovle eax, ecx
 */
static inline void write_cmovle(const char* dest, const char* src) {
    write_line3(ASM_CMOVLE, dest, src);
}

/*
 * CMOVNA - Conditional Move if Not Above / unsigned <=
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 or ZF=1 is true in the current flags state.
 *
 * Syntax: CMOVNA reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovna eax, ecx
 */
static inline void write_cmovna(const char* dest, const char* src) {
    write_line3(ASM_CMOVNA, dest, src);
}

/*
 * CMOVNAE - Conditional Move if Not Above or Equal / unsigned <
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=1 is true in the current flags state.
 *
 * Syntax: CMOVNAE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnae eax, ecx
 */
static inline void write_cmovnae(const char* dest, const char* src) {
    write_line3(ASM_CMOVNAE, dest, src);
}

/*
 * CMOVNB - Conditional Move if Not Below / unsigned >=
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 is true in the current flags state.
 *
 * Syntax: CMOVNB reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnb eax, ecx
 */
static inline void write_cmovnb(const char* dest, const char* src) {
    write_line3(ASM_CMOVNB, dest, src);
}

/*
 * CMOVNBE - Conditional Move if Not Below or Equal / unsigned >
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 and ZF=0 is true in the current flags state.
 *
 * Syntax: CMOVNBE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnbe eax, ecx
 */
static inline void write_cmovnbe(const char* dest, const char* src) {
    write_line3(ASM_CMOVNBE, dest, src);
}

/*
 * CMOVNC - Conditional Move if No carry
 * Reads the source operand and copies it into the destination register only if the condition
 * CF=0 is true in the current flags state.
 *
 * Syntax: CMOVNC reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnc eax, ecx
 */
static inline void write_cmovnc(const char* dest, const char* src) {
    write_line3(ASM_CMOVNC, dest, src);
}

/*
 * CMOVNE - Conditional Move if Not Equal / nonzero
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=0 is true in the current flags state.
 *
 * Syntax: CMOVNE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovne eax, ecx
 */
static inline void write_cmovne(const char* dest, const char* src) {
    write_line3(ASM_CMOVNE, dest, src);
}

/*
 * CMOVNG - Conditional Move if Not Greater / signed <=
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=1 or SF!=OF is true in the current flags state.
 *
 * Syntax: CMOVNG reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovng eax, ecx
 */
static inline void write_cmovng(const char* dest, const char* src) {
    write_line3(ASM_CMOVNG, dest, src);
}

/*
 * CMOVNGE - Conditional Move if Not Greater or Equal / signed <
 * Reads the source operand and copies it into the destination register only if the condition
 * SF!=OF is true in the current flags state.
 *
 * Syntax: CMOVNGE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnge eax, ecx
 */
static inline void write_cmovnge(const char* dest, const char* src) {
    write_line3(ASM_CMOVNGE, dest, src);
}

/*
 * CMOVNL - Conditional Move if Not Less / signed >=
 * Reads the source operand and copies it into the destination register only if the condition
 * SF=OF is true in the current flags state.
 *
 * Syntax: CMOVNL reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnl eax, ecx
 */
static inline void write_cmovnl(const char* dest, const char* src) {
    write_line3(ASM_CMOVNL, dest, src);
}

/*
 * CMOVNLE - Conditional Move if Not Less or Equal / signed >
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=0 and SF=OF is true in the current flags state.
 *
 * Syntax: CMOVNLE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnle eax, ecx
 */
static inline void write_cmovnle(const char* dest, const char* src) {
    write_line3(ASM_CMOVNLE, dest, src);
}

/*
 * CMOVNO - Conditional Move if No overflow
 * Reads the source operand and copies it into the destination register only if the condition
 * OF=0 is true in the current flags state.
 *
 * Syntax: CMOVNO reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovno eax, ecx
 */
static inline void write_cmovno(const char* dest, const char* src) {
    write_line3(ASM_CMOVNO, dest, src);
}

/*
 * CMOVNP - Conditional Move if Not parity / parity odd
 * Reads the source operand and copies it into the destination register only if the condition
 * PF=0 is true in the current flags state.
 *
 * Syntax: CMOVNP reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnp eax, ecx
 */
static inline void write_cmovnp(const char* dest, const char* src) {
    write_line3(ASM_CMOVNP, dest, src);
}

/*
 * CMOVNS - Conditional Move if Not sign
 * Reads the source operand and copies it into the destination register only if the condition
 * SF=0 is true in the current flags state.
 *
 * Syntax: CMOVNS reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovns eax, ecx
 */
static inline void write_cmovns(const char* dest, const char* src) {
    write_line3(ASM_CMOVNS, dest, src);
}

/*
 * CMOVNZ - Conditional Move if Not zero
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=0 is true in the current flags state.
 *
 * Syntax: CMOVNZ reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovnz eax, ecx
 */
static inline void write_cmovnz(const char* dest, const char* src) {
    write_line3(ASM_CMOVNZ, dest, src);
}

/*
 * CMOVO - Conditional Move if Overflow
 * Reads the source operand and copies it into the destination register only if the condition
 * OF=1 is true in the current flags state.
 *
 * Syntax: CMOVO reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovo eax, ecx
 */
static inline void write_cmovo(const char* dest, const char* src) {
    write_line3(ASM_CMOVO, dest, src);
}

/*
 * CMOVP - Conditional Move if Parity / parity even
 * Reads the source operand and copies it into the destination register only if the condition
 * PF=1 is true in the current flags state.
 *
 * Syntax: CMOVP reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovp eax, ecx
 */
static inline void write_cmovp(const char* dest, const char* src) {
    write_line3(ASM_CMOVP, dest, src);
}

/*
 * CMOVPE - Conditional Move if Parity even
 * Reads the source operand and copies it into the destination register only if the condition
 * PF=1 is true in the current flags state.
 *
 * Syntax: CMOVPE reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovpe eax, ecx
 */
static inline void write_cmovpe(const char* dest, const char* src) {
    write_line3(ASM_CMOVPE, dest, src);
}

/*
 * CMOVPO - Conditional Move if Parity odd
 * Reads the source operand and copies it into the destination register only if the condition
 * PF=0 is true in the current flags state.
 *
 * Syntax: CMOVPO reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovpo eax, ecx
 */
static inline void write_cmovpo(const char* dest, const char* src) {
    write_line3(ASM_CMOVPO, dest, src);
}

/*
 * CMOVS - Conditional Move if Sign
 * Reads the source operand and copies it into the destination register only if the condition
 * SF=1 is true in the current flags state.
 *
 * Syntax: CMOVS reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovs eax, ecx
 */
static inline void write_cmovs(const char* dest, const char* src) {
    write_line3(ASM_CMOVS, dest, src);
}

/*
 * CMOVZ - Conditional Move if Zero
 * Reads the source operand and copies it into the destination register only if the condition
 * ZF=1 is true in the current flags state.
 *
 * Syntax: CMOVZ reg, r/m
 * Operands: destination general-purpose register; source register or memory of the same size
 * Flags affected: None
 * Notes: CMOVcc avoids a branch but still depends on flags set by a previous compare or
 * arithmetic instruction. The source may be read even if the move is not committed
 * architecturally.
 *
 * Example:
 *   cmp eax, ebx
 *   cmovz eax, ecx
 */
static inline void write_cmovz(const char* dest, const char* src) {
    write_line3(ASM_CMOVZ, dest, src);
}

/*
 * PUSH - Push onto Stack
 * Decrements the stack pointer and stores the operand value on the stack.
 *
 * Syntax: PUSH src
 * Operands: register, memory, immediate, or segment register depending on mode
 * Flags affected: None
 * Notes: Operand size controls the amount pushed. In 64-bit mode, most pushes are 8 bytes and
 * adjust RSP accordingly.
 *
 * Example:
 *   push rbp
 *   push 16
 */
static inline void write_push(const char* operand) {
    write_line2(ASM_PUSH, operand);
}

/*
 * POP - Pop from Stack
 * Loads the value at the top of the stack into the destination operand and then increments the
 * stack pointer.
 *
 * Syntax: POP dest
 * Operands: register, memory, or certain segment registers depending on mode
 * Flags affected: None
 * Notes: The destination size controls how much stack space is removed. POP to CS is not allowed
 * with ordinary POP.
 *
 * Example:
 *   pop rbp
 *   pop qword ptr [rsp]
 */
static inline void write_pop(const char* operand) {
    write_line2(ASM_POP, operand);
}

/*
 * Spill frame.
 *
 * One utility for every calling convention. The preserved mask is the
 * callee-saved set (rsp and rbp are never pushed here: rbp is the frame
 * pointer, rsp is the stack). asm_frame_prologue saves that set and opens a
 * packed slot area. asm_frame_epilogue restores it. A write to a preserved
 * register that was not in the mask still fails in asm_note_write_reg.
 * Restoring the mask at the epilogue puts those registers back, so the
 * writes are not a clobber the caller can observe.
 *
 * Slots are packed to the value's size. A qword does not force the next
 * byte into the following qword. Alignment is the requested power of two,
 * capped by the frame alignment. The default frame is 16-byte aligned,
 * because rbp is. Alignments of 64 and 4096 pass a larger `align` and the
 * prologue aligns a base in r12; slots are then [r12+n]. r12 is reserved
 * for that base until the epilogue pops the saved copy.
 *
 * 16-byte frames address slots as [rbp-n]. n is the distance of the slot's
 * lowest byte from rbp, so the address is aligned when n is.
 */
#define ASM_FRAME_CAP 4096
static unsigned char asm_frame_used[ASM_FRAME_CAP + 8];
static int asm_frame_cap = 256;
static int asm_frame_base_align = 16;
static int asm_frame_use_base = 0;
static int asm_frame_active = 0;
static const char* asm_frame_pushed[16];
static int asm_frame_npush = 0;

/* A GPR is a general-purpose register: one of the sixteen integer registers
 * rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8..r15. The index is the bit
 * number used by preserved masks and by c_regs. rsp is 4 and rbp is 5.
 * xmm registers are not GPRs. The byte/word/dword spellings (al, eax, r8d)
 * name the same GPR; asm_gpr_index folds those spellings onto this index. */
static const char* const asm_gpr_qword[16] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
};

/* Push rbx, rsi, rdi, r12..r15, low index to high. These are the Windows
 * x64 callee-saved GPRs apart from rbp (the frame pointer) and rsp.
 * A conservative collector uses this so a pointer that lives only in a
 * register becomes a word on the stack it is about to scan. Returns the
 * number of pushes. asm_pop_preserved_win64 pops that many in reverse. */
static inline int asm_push_preserved_win64(void) {
    static const int idx[] = { 3, 6, 7, 12, 13, 14, 15 };
    for (int i = 0; i < 7; i++) {
        write_push(asm_gpr_qword[idx[i]]);
        asm_note_rsp_sub(8);
    }
    return 7;
}
/* Pop rbx, rsi, rdi, and r12..r15 in reverse push order, recording each
 * 8-byte rise in rsp. */
static inline void asm_pop_preserved_win64(void) {
    static const int idx[] = { 3, 6, 7, 12, 13, 14, 15 };
    for (int i = 6; i >= 0; i--) {
        write_pop(asm_gpr_qword[idx[i]]);
        asm_note_rsp_add(8);
    }
}

static const char* const asm_frame_rbp_lea[17] = {
    "[rbp]",     "[rbp-8]",   "[rbp-16]",  "[rbp-24]",
    "[rbp-32]",  "[rbp-40]",  "[rbp-48]",  "[rbp-56]",
    "[rbp-64]",  "[rbp-72]",  "[rbp-80]",  "[rbp-88]",
    "[rbp-96]",  "[rbp-104]", "[rbp-112]", "[rbp-120]",
    "[rbp-128]"
};

/* Clear the spill-frame occupancy map so every reserved byte is free. */
static inline void asm_frame_clear(void) {
    memset(asm_frame_used, 0, (size_t)asm_frame_cap + 1);
}

/* Reserve `size` bytes at alignment `align` inside the open frame.
 *
 * The frame is a bitmap, one byte of asm_frame_used per byte of the
 * reservation. A set bit is already holding a spill or a local.
 *
 * Two addressings:
 *   - rbp frame (align 16). Slots grow downward. The key `lo` is the
 *     distance from rbp to the slot's lowest byte, so the operand is
 *     [rbp-lo] and the slot occupies bytes [lo-size+1, lo]. `lo` itself
 *     is a multiple of `align`, which makes [rbp-lo] aligned when rbp is.
 *   - r12 frame (align 64 or 4096). Slots grow upward from the aligned
 *     base. `lo` is the displacement, the slot occupies [lo, lo+size),
 *     and the operand is [r12+lo].
 *
 * The first fit wins. The returned key is what asm_frame_release expects.
 * Generation aborts when the reservation cannot hold the slot. */
static inline int asm_frame_take(int size, int align) {
    if (size < 1) size = 1;
    if (align < 1) align = 1;
    crash_if(align > asm_frame_base_align,
             "asm_frame_slot: alignment wider than this frame");
    crash_if(size != 1 && size != 2 && size != 4 && size != 8,
             "asm_frame_slot: size must be 1, 2, 4, or 8");
    if (!asm_frame_use_base) {
        for (int lo = align; lo <= asm_frame_cap; lo += align) {
            if (lo < size) continue;
            int begin = lo - size + 1;
            int ok = 1;
            for (int d = begin; d <= lo; d++) if (asm_frame_used[d]) { ok = 0; break; }
            if (!ok) continue;
            for (int d = begin; d <= lo; d++) asm_frame_used[d] = 1;
            return lo;
        }
    } else {
        for (int off = 0; off + size <= asm_frame_cap; off += align) {
            int ok = 1;
            for (int d = off; d < off + size; d++) if (asm_frame_used[d]) { ok = 0; break; }
            if (!ok) continue;
            for (int d = off; d < off + size; d++) asm_frame_used[d] = 1;
            return off;
        }
    }
    crash_if(1, "asm_frame_slot: frame is full");
    return 0;
}

/* Give a slot back. `lo` and `size` are the pair asm_frame_take returned.
 * The bytes are cleared in the same direction take marked them: downward
 * from lo on an rbp frame, upward from lo on an r12 frame. A later take
 * may reuse them. Releasing a key that was not taken clears whatever bits
 * fall in that range; callers pass the key they were given. */
static inline void asm_frame_release(int lo, int size) {
    if (lo < 0 || size < 1) return;
    if (!asm_frame_use_base) {
        int begin = lo - size + 1;
        if (begin < 0) begin = 0;
        for (int d = begin; d <= lo && d <= ASM_FRAME_CAP; d++) asm_frame_used[d] = 0;
    } else {
        for (int d = lo; d < lo + size && d <= ASM_FRAME_CAP; d++) asm_frame_used[d] = 0;
    }
}

/* Operand text for a key that is already reserved. size selects the MASM
 * width (byte, word, dword, qword). The string is bump-allocated because
 * write() keeps the pointer. An r12 frame uses [r12] or [r12+lo]. An rbp
 * frame uses [rbp-lo]. */
static inline const char* asm_frame_operand(int lo, int size) {
    const char* w = "qword";
    if (size == 1) w = "byte";
    else if (size == 2) w = "word";
    else if (size == 4) w = "dword";
    char buf[72];
    if (asm_frame_use_base) {
        if (lo == 0) snprintf(buf, sizeof buf, "%s ptr [r12]", w);
        else snprintf(buf, sizeof buf, "%s ptr [r12+%d]", w, lo);
    } else {
        snprintf(buf, sizeof buf, "%s ptr [rbp-%d]", w, lo);
    }
    size_t n = 0;
    while (buf[n]) n++;
    char* s = (char*)asm_bump_alloc(n + 1);
    memcpy(s, buf, n + 1);
    return s;
}

/* Take a slot of `size` bytes aligned to `align` and return its operand.
 * When `lo_out` is set it receives the key, so the caller can release the
 * slot later. */
static inline const char* asm_frame_slot_at(int size, int align, int* lo_out) {
    int lo = asm_frame_take(size, align);
    if (lo_out) *lo_out = lo;
    return asm_frame_operand(lo, size);
}

/* Take a slot of `size` bytes aligned to `align` and return its operand,
 * discarding the key. Use this when the slot lives until the function
 * returns and nothing has to release it. */
static inline const char* asm_frame_slot(int size, int align) {
    return asm_frame_slot_at(size, align, NULL);
}

/* Open a frame. `preserved` is the callee-saved GPR mask (bit = asm_gpr_index).
 * `align` is 16, 64, or 4096. */
static inline void asm_frame_prologue(unsigned preserved, int align) {
    if (align < 16) align = 16;
    asm_frame_base_align = align;
    asm_frame_use_base = align > 16;
    asm_frame_cap = (align >= 4096) ? 4096 : 256;
    asm_frame_clear();
    asm_frame_npush = 0;
    asm_frame_active = 1;
    asm_set_preserved(preserved);
    asm_mark_saved(preserved);

    write_push("rbp");
    asm_note_rsp_sub(8);
    write_line3(ASM_MOV, "rbp", "rsp");

    if (!asm_frame_use_base) {
        write_line3(ASM_SUB, "rsp", "256");
        asm_note_rsp_sub(256);
    }
    for (int idx = 0; idx < 16; idx++) {
        if (idx == 4 || idx == 5) continue;
        if ((preserved & (1u << idx)) == 0) continue;
        crash_if(asm_frame_npush >= 16, "asm_frame_prologue: too many preserved registers");
        write_push(asm_gpr_qword[idx]);
        asm_note_rsp_sub(8);
        asm_frame_pushed[asm_frame_npush++] = asm_gpr_qword[idx];
    }
    if (asm_frame_use_base) {
        write_line3(ASM_AND, "rsp", align >= 4096 ? "-4096" : "-64");
        asm_rsp_mod16 = 0;
        write_line3(ASM_SUB, "rsp", align >= 4096 ? "4096" : "256");
        asm_note_rsp_sub(align >= 4096 ? 4096 : 256);
        write_line3(ASM_MOV, "r12", "rsp");
    }
}

/* Inverse of asm_frame_prologue, not counting the pushes of rbp that the
 * prologue did. Every return path emits this. The frame stays open so a
 * later return in the same function emits the same sequence again.
 *
 * Aligned frames (r12): lea rsp back to the first push, which drops the
 * and-alignment and the reservation in one instruction, then pop the
 * preserved GPRs in reverse order, then pop rbp.
 * 16-byte frames: pop the preserved GPRs (they sit below the 256-byte gap),
 * then mov rsp, rbp (drops the gap), then pop rbp.
 *
 * rsp's alignment tracker is restored to the value it had on entry, because
 * the epilogue is emitted once per return and the tracker's running total
 * must not drift if a second return emits the sequence again. */
static inline void asm_frame_epilogue(void) {
    if (!asm_frame_active) return;
    /* Every return path emits this sequence. The frame stays open until the
     * procedure ends, so a later c_ret in the same function emits it again. */
    int saved = asm_rsp_mod16;
    if (asm_frame_use_base) {
        crash_if(asm_frame_npush < 0 || asm_frame_npush > 16, "asm_frame_epilogue: bad push count");
        write_line3(ASM_LEA, "rsp", asm_frame_rbp_lea[asm_frame_npush]);
    }
    for (int i = asm_frame_npush - 1; i >= 0; i--) {
        write_pop(asm_frame_pushed[i]);
        asm_note_rsp_add(8);
    }
    if (!asm_frame_use_base)
        write_line3(ASM_MOV, "rsp", "rbp");
    write_pop("rbp");
    asm_note_rsp_add(8);
    asm_rsp_mod16 = saved;
}

/* The procedure has ended. Further returns must not emit another epilogue,
 * and asm_frame_slot must not be asked for a slot until the next prologue.
 * The bitmap is left as it is; the next prologue clears it. */
static inline void asm_frame_end(void) {
    asm_frame_active = 0;
}

/*
 * PUSHA - Push All General Registers
 * Pushes the legacy 16-bit general-purpose registers in a fixed order beginning with AX and
 * including the original SP value.
 *
 * Syntax: PUSHA
 * Operands: implicit 16-bit register set
 * Flags affected: None
 * Notes: PUSHA is available only in 16-bit operand-size form and is invalid in 64-bit mode. The
 * 32-bit alias is PUSHAD, which MASM may also accept separately.
 *
 * Example:
 *   pusha
 */
static inline void write_pusha(void) {
    write_line(ASM_PUSHA);
}

/*
 * POPA - Pop All General Registers
 * Restores the legacy 16-bit general-purpose registers from the stack in the reverse order of
 * PUSHA while discarding the saved SP slot.
 *
 * Syntax: POPA
 * Operands: implicit 16-bit register set
 * Flags affected: None
 * Notes: POPA is invalid in 64-bit mode. The 32-bit alias is POPAD in assemblers that expose it.
 *
 * Example:
 *   popa
 */
static inline void write_popa(void) {
    write_line(ASM_POPA);
}

/*
 * PUSHF - Push Flags
 * Pushes the current FLAGS register image onto the stack using the active operand size.
 *
 * Syntax: PUSHF
 * Operands: implicit flags register
 * Flags affected: None
 * Notes: Use PUSHFD in 32-bit code or PUSHFQ in 64-bit code when you want an explicit width
 * mnemonic.
 *
 * Example:
 *   pushf
 */
static inline void write_pushf(void) {
    write_line(ASM_PUSHF);
}

/*
 * POPF - Pop Flags
 * Restores flags bits from the top of the stack into FLAGS/EFLAGS/RFLAGS according to the active
 * operand size and privilege rules.
 *
 * Syntax: POPF
 * Operands: implicit flags register
 * Flags affected: Restores architectural flags bits from memory on the stack
 * Notes: Not all bits are writable at all privilege levels. Interrupt enable and IOPL changes
 * are restricted.
 *
 * Example:
 *   popf
 */
static inline void write_popf(void) {
    write_line(ASM_POPF);
}

/*
 * PUSHFD - Push EFLAGS
 * Pushes the 32-bit EFLAGS image onto the stack.
 *
 * Syntax: PUSHFD
 * Operands: implicit EFLAGS register
 * Flags affected: None
 * Notes: Primarily used in 32-bit code to save status and control flags explicitly as a dword.
 *
 * Example:
 *   pushfd
 */
static inline void write_pushfd(void) {
    write_line(ASM_PUSHFD);
}

/*
 * POPFD - Pop EFLAGS
 * Restores the 32-bit EFLAGS image from the stack subject to privilege restrictions.
 *
 * Syntax: POPFD
 * Operands: implicit EFLAGS register
 * Flags affected: Restores writable EFLAGS bits from the stack
 * Notes: Some flag bits are ignored or protected depending on CPL and mode.
 *
 * Example:
 *   popfd
 */
static inline void write_popfd(void) {
    write_line(ASM_POPFD);
}

/*
 * PUSHFQ - Push RFLAGS
 * Pushes the 64-bit RFLAGS image onto the stack in long mode.
 *
 * Syntax: PUSHFQ
 * Operands: implicit RFLAGS register
 * Flags affected: None
 * Notes: Used by low-level 64-bit code to preserve the full visible flags state.
 *
 * Example:
 *   pushfq
 */
static inline void write_pushfq(void) {
    write_line(ASM_PUSHFQ);
}

/*
 * POPFQ - Pop RFLAGS
 * Restores writable bits of RFLAGS from the stack in long mode.
 *
 * Syntax: POPFQ
 * Operands: implicit RFLAGS register
 * Flags affected: Restores writable RFLAGS bits from the stack
 * Notes: Privilege restrictions still apply to IF, IOPL, and system-control bits.
 *
 * Example:
 *   popfq
 */
static inline void write_popfq(void) {
    write_line(ASM_POPFQ);
}

/*
 * CBW - Convert Byte to Word
 * Sign-extends AL into AX.
 *
 * Syntax: CBW
 * Operands: implicit AL -> AX
 * Flags affected: None
 * Notes: This instruction prepares a signed 8-bit value in AL for 16-bit arithmetic or division.
 *
 * Example:
 *   cbw
 */
static inline void write_cbw(void) {
    write_line(ASM_CBW);
}

/*
 * CWD - Convert Word to Doubleword
 * Sign-extends AX into DX:AX.
 *
 * Syntax: CWD
 * Operands: implicit AX -> DX:AX
 * Flags affected: None
 * Notes: Commonly used before signed 16-bit IDIV.
 *
 * Example:
 *   cwd
 */
static inline void write_cwd(void) {
    write_line(ASM_CWD);
}

/*
 * CWDE - Convert Word to Doubleword Extended
 * Sign-extends AX into EAX.
 *
 * Syntax: CWDE
 * Operands: implicit AX -> EAX
 * Flags affected: None
 * Notes: Promotes a signed 16-bit value to 32 bits without using a separate source operand.
 *
 * Example:
 *   cwde
 */
static inline void write_cwde(void) {
    write_line(ASM_CWDE);
}

/*
 * CDQ - Convert Doubleword to Quadword
 * Sign-extends EAX into EDX:EAX.
 *
 * Syntax: CDQ
 * Operands: implicit EAX -> EDX:EAX
 * Flags affected: None
 * Notes: Typically used before signed 32-bit IDIV.
 *
 * Example:
 *   cdq
 */
static inline void write_cdq(void) {
    write_line(ASM_CDQ);
}

/*
 * CDQE - Convert Doubleword to Quadword Extended
 * Sign-extends EAX into RAX in 64-bit mode.
 *
 * Syntax: CDQE
 * Operands: implicit EAX -> RAX
 * Flags affected: None
 * Notes: Useful when a 32-bit signed result must be promoted to a 64-bit pointer or arithmetic
 * value.
 *
 * Example:
 *   cdqe
 */
static inline void write_cdqe(void) {
    write_line(ASM_CDQE);
}

/*
 * CQO - Convert Quadword to Octoword
 * Sign-extends RAX into RDX:RAX.
 *
 * Syntax: CQO
 * Operands: implicit RAX -> RDX:RAX
 * Flags affected: None
 * Notes: Typically used before signed 64-bit IDIV.
 *
 * Example:
 *   cqo
 */
static inline void write_cqo(void) {
    write_line(ASM_CQO);
}

/*
 * MOVS - Move String Data
 * Move String Data copies an element from [DS:(E|R)SI] to [ES:(E|R)DI] and then advances or
 * decrements the pointers by the element size.
 *
 * Syntax: MOVS
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   rep movsb
 */
static inline void write_movs(const char* dest, const char* src) {
    write_line3(ASM_MOVS, dest, src);
}

/*
 * MOVSB - Move Byte String Data
 * Move Byte String Data copies one byte from [DS:(E|R)SI] to [ES:(E|R)DI] and updates SI/DI,
 * ESI/EDI, or RSI/RDI according to DF.
 *
 * Syntax: MOVSB
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   movsb
 */
/* Implicit-register check shared by the string ops. The cell's location
 * must be the register the instruction actually reads or writes. */
static inline void asm_require_reg(AsmDataField cell, const char* reg) {
    crash_if(cell.name == NULL || strcmp(cell.name, reg) != 0,
             "string op: operand is not in the register the instruction uses");
}

/* Emit movsb after requiring `dst` in rdi and `src` in rsi. One byte is
 * copied from [rsi] to [rdi]. */
static inline void write_movsb(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    write_line(ASM_MOVSB);
}

/*
 * write_rep_movsb - Emit "rep movsb", a block byte-copy whose operands are all
 * implicit. It copies rcx bytes from [rsi] to [rdi] (forward when DF is clear),
 * decrementing rcx to 0 and advancing rsi/rdi past the copied region. None of
 * those registers appear in the mnemonic, so this wrapper names them via cells:
 * the destination, source, and count, each an AsmDataField carrying its location
 * and type tag. The tags are currently recorded only for readability and are
 * otherwise unused.
 */
static inline void write_rep_movsb(AsmDataField dst, AsmDataField src,
                                   AsmDataField count) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    asm_require_reg(count, "rcx");
    write_line2(ASM_REP, ASM_MOVSB);
}

/*
 * MOVSW - Move Word String Data
 * Move Word String Data copies one 16-bit element between the implicit string pointers and
 * updates them by 2.
 *
 * Syntax: MOVSW
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   movsw
 */
static inline void write_movsw(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    write_line(ASM_MOVSW);
}

/*
 * MOVSD - Move Doubleword String Data
 * Move Doubleword String Data copies one 32-bit element between the implicit string pointers and
 * updates them by 4.
 *
 * Syntax: MOVSD
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   movsd
 */
static inline void write_movsd(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    write_line(ASM_MOVSD);
}

/*
 * MOVSQ - Move Quadword String Data
 * Move Quadword String Data copies one 64-bit element between RSI and RDI in 64-bit mode and
 * updates them by 8.
 *
 * Syntax: MOVSQ
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   rep movsq
 */
static inline void write_movsq(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    write_line(ASM_MOVSQ);
}

/*
 * LODS - Load String Data
 * Load String Data loads an element from [DS:(E|R)SI] into AL/AX/EAX/RAX and then updates the
 * source pointer according to DF.
 *
 * Syntax: LODS
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lodsd
 */
static inline void write_lods(const char* dest, const char* src) {
    write_line3(ASM_LODS, dest, src);
}

/*
 * LODSB - Load Byte String Data
 * Load Byte String Data loads one byte into AL from [DS:(E|R)SI] and adjusts the source pointer
 * by 1.
 *
 * Syntax: LODSB
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lodsb
 */
static inline void write_lodsb(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "al");
    asm_require_reg(src, "rsi");
    write_line(ASM_LODSB);
}

/*
 * LODSW - Load Word String Data
 * Load Word String Data loads one 16-bit element into AX and adjusts the source pointer by 2.
 *
 * Syntax: LODSW
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lodsw
 */
static inline void write_lodsw(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "ax");
    asm_require_reg(src, "rsi");
    write_line(ASM_LODSW);
}

/*
 * LODSD - Load Doubleword String Data
 * Load Doubleword String Data loads one 32-bit element into EAX and adjusts the source pointer
 * by 4.
 *
 * Syntax: LODSD
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lodsd
 */
static inline void write_lodsd(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "eax");
    asm_require_reg(src, "rsi");
    write_line(ASM_LODSD);
}

/*
 * LODSQ - Load Quadword String Data
 * Load Quadword String Data loads one 64-bit element into RAX and adjusts RSI by 8.
 *
 * Syntax: LODSQ
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   lodsq
 */
static inline void write_lodsq(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rax");
    asm_require_reg(src, "rsi");
    write_line(ASM_LODSQ);
}

/*
 * STOS - Store String Data
 * Store String Data stores AL/AX/EAX/RAX to [ES:(E|R)DI] and then updates the destination
 * pointer according to DF.
 *
 * Syntax: STOS
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   stosd
 */
static inline void write_stos(const char* dest, const char* src) {
    write_line3(ASM_STOS, dest, src);
}

/*
 * STOSB - Store Byte String Data
 * Store Byte String Data stores AL to [ES:(E|R)DI] and adjusts the destination pointer by 1.
 *
 * Syntax: STOSB
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   rep stosb
 */
static inline void write_stosb(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "al");
    write_line(ASM_STOSB);
}

/*
 * STOSW - Store Word String Data
 * Store Word String Data stores AX and adjusts the destination pointer by 2.
 *
 * Syntax: STOSW
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   stosw
 */
static inline void write_stosw(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "ax");
    write_line(ASM_STOSW);
}

/*
 * STOSD - Store Doubleword String Data
 * Store Doubleword String Data stores EAX and adjusts the destination pointer by 4.
 *
 * Syntax: STOSD
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   rep stosd
 */
static inline void write_stosd(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "eax");
    write_line(ASM_STOSD);
}

/*
 * STOSQ - Store Quadword String Data
 * Store Quadword String Data stores RAX and adjusts RDI by 8.
 *
 * Syntax: STOSQ
 * Operands: implicit source and/or destination registers; optional REP prefix for repetition
 * Flags affected: String compare/move/store forms do not directly change EFLAGS except for
 * compare-class instructions; MOVS/LODS/STOS themselves do not update status flags.
 * Notes: Direction Flag controls whether pointers increment or decrement. Segment defaults are
 * built into the instruction form and are part of the architecture.
 *
 * Example:
 *   rep stosq
 */
static inline void write_stosq(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rax");
    write_line(ASM_STOSQ);
}

/*
 * ============================================================================
 * 7. Arithmetic Instructions
 * ============================================================================
 */

/*
 * ADD - Add
 * Adds the source operand to the destination operand and stores the sum in the destination.
 *
 * Syntax: ADD dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the result
 * Notes: There is no memory-to-memory form. ADD is the fundamental integer addition primitive.
 *
 * Example:
 *   add eax, 1
 *
 * Both operands are inputs (the destination is read-modify-written), so each is
 * an AsmDataField carrying its location and type tag. The tags are currently
 * recorded only for readability and are otherwise unused.
 */
static inline void write_add(AsmDataField dest, AsmDataField src) {
    write_line3(ASM_ADD, dest.name, src.name);
}

/*
 * ADC - Add with Carry
 * Adds the source operand and the current carry flag to the destination operand.
 *
 * Syntax: ADC dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the result
 * Notes: ADC is used for multi-precision addition because it incorporates CF from a previous
 * lower-word add.
 *
 * Example:
 *   adc eax, 1
 */
static inline void write_adc(const char* dest, const char* src) {
    write_line3(ASM_ADC, dest, src);
}

/*
 * SUB - Subtract
 * Subtracts the source operand from the destination operand and stores the difference in the
 * destination.
 *
 * Syntax: SUB dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the result
 * Notes: SUB performs unsigned borrow reporting through CF and signed overflow reporting through
 * OF.
 *
 * Example:
 *   sub eax, 1
 *
 * Both operands are inputs (the destination is read-modify-written), so each is
 * an AsmDataField carrying its location and type tag. The tags are currently
 * recorded only for readability and are otherwise unused.
 */
static inline void write_sub(AsmDataField dest, AsmDataField src) {
    write_line3(ASM_SUB, dest.name, src.name);
}

/*
 * SBB - Subtract with Borrow
 * Subtracts the source operand and the current carry flag from the destination operand.
 *
 * Syntax: SBB dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the result
 * Notes: SBB is the subtraction counterpart to ADC for multi-precision arithmetic.
 *
 * Example:
 *   sbb eax, 1
 */
static inline void write_sbb(const char* dest, const char* src) {
    write_line3(ASM_SBB, dest, src);
}

/*
 * INC - Increment
 * Adds 1 to the destination operand.
 *
 * Syntax: INC dest
 * Operands: register or memory operand
 * Flags affected: OF, SF, ZF, AF, and PF are updated; CF is not changed
 * Notes: INC is convenient when carry preservation matters, but ADD reg, 1 is sometimes
 * preferred for uniform flag behavior.
 *
 * Example:
 *   inc eax, 1
 */
static inline void write_inc(const char* operand) {
    write_line2(ASM_INC, operand);
}

/*
 * DEC - Decrement
 * Subtracts 1 from the destination operand.
 *
 * Syntax: DEC dest
 * Operands: register or memory operand
 * Flags affected: OF, SF, ZF, AF, and PF are updated; CF is not changed
 * Notes: DEC preserves CF, which distinguishes it from SUB dest, 1.
 *
 * Example:
 *   dec eax, 1
 */
static inline void write_dec(const char* operand) {
    write_line2(ASM_DEC, operand);
}

/*
 * NEG - Two's-Complement Negate
 * Replaces the destination operand with its arithmetic negation (0 - dest).
 *
 * Syntax: NEG dest
 * Operands: register or memory operand
 * Flags affected: OF, SF, ZF, AF, PF, and CF are updated; CF becomes 0 only when the original
 * operand was zero
 * Notes: NEG is useful for sign inversion and absolute-value sequences.
 *
 * Example:
 *   neg eax, 1
 */
static inline void write_neg(const char* operand) {
    write_line2(ASM_NEG, operand);
}

/*
 * CMP - Compare
 * Subtracts the source operand from the destination operand for flags only and discards the
 * arithmetic result.
 *
 * Syntax: CMP left, right
 * Operands: register or memory left operand; register, memory, or immediate right operand
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set as if SUB had executed
 * Notes: CMP feeds Jcc, SETcc, CMOVcc, LOOP conditions, and other flag-dependent logic.
 *
 * Example:
 *   cmp eax, 1
 *
 * Both operands are inputs to the comparison, so each is an AsmDataField
 * carrying its location and type tag. The tags are currently recorded only for
 * readability and are otherwise unused.
 */
static inline void write_cmp(AsmDataField dest, AsmDataField src) {
    write_line3(ASM_CMP, dest.name, src.name);
}

/*
 * XADD - Exchange and Add
 * Exchanges the source and destination operands, then stores the sum into the destination.
 *
 * Syntax: XADD dest, src
 * Operands: register/register or register/memory operands of the same size
 * Flags affected: OF, SF, ZF, AF, CF, and PF are set according to the addition result
 * Notes: With a LOCK prefix and a memory destination, XADD is useful for atomic counters and
 * fetch-add patterns.
 *
 * Example:
 *   xadd eax, 1
 */
static inline void write_xadd(const char* dest, const char* src) {
    write_line3(ASM_XADD, dest, src);
}

/*
 * MUL - Unsigned Multiply
 * Multiplies the implicit accumulator by the source operand and stores the full-width product in
 * a register pair or in the wider accumulator register.
 *
 * Syntax: MUL src
 * Operands: source register or memory; implicit AL/AX/EAX/RAX accumulator
 * Flags affected: CF and OF are set when the upper half of the product is nonzero; other status
 * flags are undefined
 * Notes: Result destinations are AX for byte multiply, DX:AX for word, EDX:EAX for dword, and
 * RDX:RAX for qword.
 *
 * Example:
 *   mul ecx
 *   mul qword ptr [rsi]
 */
static inline void write_mul(const char* operand) {
    write_line2(ASM_MUL, operand);
}

/*
 * IMUL - Signed Multiply
 * Performs signed multiplication in one-, two-, or three-operand forms.
 *
 * Syntax: IMUL src | IMUL reg, r/m | IMUL reg, r/m, imm
 * Operands: one-operand implicit-accumulator form or explicit register destination forms
 * Flags affected: CF and OF indicate whether the signed product was truncated in the explicit
 * forms; other flags are undefined or architecturally insignificant
 * Notes: The one-operand form writes a double-width product like MUL. Two- and three-operand
 * forms produce a truncated product in the destination register.
 *
 * Example:
 *   imul ecx
 *   imul eax, edx, 10
 */
static inline void write_imul(const char* operand) {
    write_line2(ASM_IMUL, operand);
}

/*
 * IMUL - Signed Multiply
 * Performs signed multiplication in one-, two-, or three-operand forms.
 *
 * Syntax: IMUL src | IMUL reg, r/m | IMUL reg, r/m, imm
 * Operands: one-operand implicit-accumulator form or explicit register destination forms
 * Flags affected: CF and OF indicate whether the signed product was truncated in the explicit
 * forms; other flags are undefined or architecturally insignificant
 * Notes: The one-operand form writes a double-width product like MUL. Two- and three-operand
 * forms produce a truncated product in the destination register.
 *
 * Example:
 *   imul ecx
 *   imul eax, edx, 10
 */
static inline void write_imul2(const char* dest, const char* src) {
    write_line3(ASM_IMUL, dest, src);
}

/*
 * IMUL - Signed Multiply
 * Performs signed multiplication in one-, two-, or three-operand forms.
 *
 * Syntax: IMUL src | IMUL reg, r/m | IMUL reg, r/m, imm
 * Operands: one-operand implicit-accumulator form or explicit register destination forms
 * Flags affected: CF and OF indicate whether the signed product was truncated in the explicit
 * forms; other flags are undefined or architecturally insignificant
 * Notes: The one-operand form writes a double-width product like MUL. Two- and three-operand
 * forms produce a truncated product in the destination register.
 *
 * Example:
 *   imul ecx
 *   imul eax, edx, 10
 */
static inline void write_imul3(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_IMUL, dest, src, imm);
}

/*
 * DIV - Unsigned Divide
 * Divides the implicit dividend in AX, DX:AX, EDX:EAX, or RDX:RAX by the source operand.
 *
 * Syntax: DIV src
 * Operands: source register or memory; implicit dividend and quotient/remainder registers
 * Flags affected: Undefined
 * Notes: Quotient and remainder widths depend on operand size. Division by zero or quotient
 * overflow raises a divide error exception.
 *
 * Example:
 *   xor edx, edx
 *   div ecx
 */
static inline void write_div(const char* operand) {
    write_line2(ASM_DIV, operand);
}

/*
 * IDIV - Signed Divide
 * Performs signed division of the implicit dividend by the source operand.
 *
 * Syntax: IDIV src
 * Operands: source register or memory; implicit signed dividend in AX, DX:AX, EDX:EAX, or
 * RDX:RAX
 * Flags affected: Undefined
 * Notes: Use CWD, CDQ, or CQO beforehand to sign-extend the dividend. Divide error is raised on
 * division by zero or quotient overflow.
 *
 * Example:
 *   cdq
 *   idiv ecx
 */
static inline void write_idiv(const char* operand) {
    write_line2(ASM_IDIV, operand);
}

/*
 * ============================================================================
 * 8. Logical Instructions
 * ============================================================================
 */

/*
 * AND - Bitwise AND
 * Computes the bitwise logical AND of the operands and writes the result to the destination.
 *
 * Syntax: AND dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: ZF, SF, and PF reflect the result; CF and OF are cleared; AF is undefined
 * Notes: AND is often used for masking and alignment.
 *
 * Example:
 *   and eax, 0FFh
 */
static inline void write_and(const char* dest, const char* src) {
    write_line3(ASM_AND, dest, src);
}

/*
 * OR - Bitwise OR
 * Computes the bitwise logical inclusive OR of the operands and writes the result to the
 * destination.
 *
 * Syntax: OR dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: ZF, SF, and PF reflect the result; CF and OF are cleared; AF is undefined
 * Notes: OR is commonly used to combine bitfields or test for nonzero values.
 *
 * Example:
 *   or eax, 0FFh
 */
static inline void write_or(const char* dest, const char* src) {
    write_line3(ASM_OR, dest, src);
}

/*
 * XOR - Bitwise Exclusive OR
 * Computes the bitwise XOR of the operands and writes the result to the destination.
 *
 * Syntax: XOR dest, src
 * Operands: register or memory destination; register, memory, or immediate source
 * Flags affected: ZF, SF, and PF reflect the result; CF and OF are cleared; AF is undefined
 * Notes: XOR reg, reg is a classic zeroing idiom.
 *
 * Example:
 *   xor eax, 0FFh
 */
/*
 * write_xor - XOR two operands given as whole AsmDataField cells.
 *
 * Callers pass typed locations directly, e.g. zeroing a proc's return register
 * with write_xor(wnd_proc.return_reg, wnd_proc.return_reg), so the register is not
 * re-spelled as a raw string. Only the operand names drive the emitted
 * instruction; the tags are documentary.
 */
static inline void write_xor(AsmDataField dest, AsmDataField src) {
    write_line3(ASM_XOR, dest.name, src.name);
}

/*
 * NOT - Bitwise NOT
 * Inverts every bit in the destination operand.
 *
 * Syntax: NOT dest
 * Operands: register or memory operand
 * Flags affected: None
 * Notes: NOT is a pure bitwise complement operation and does not interact with flags.
 *
 * Example:
 *   not eax, 0FFh
 */
static inline void write_not(const char* operand) {
    write_line2(ASM_NOT, operand);
}

/*
 * TEST - Logical Compare
 * Computes a bitwise AND for flags only and discards the result.
 *
 * Syntax: TEST left, right
 * Operands: register or memory left operand; register, memory, or immediate right operand
 * Flags affected: ZF, SF, and PF reflect the temporary result; CF and OF are cleared; AF is
 * undefined
 * Notes: TEST is commonly used to probe bits, check sign, or check for zero without modifying
 * the original operand.
 *
 * Example:
 *   test eax, 0FFh
 */
static inline void write_test(const char* dest, const char* src) {
    write_line3(ASM_TEST, dest, src);
}

/*
 * ============================================================================
 * 9. Shift and Rotate Instructions
 * ============================================================================
 */

/*
 * SHL - Shift Logical Left
 * Shifts the destination left, filling low bits with zero.
 *
 * Syntax: SHL dest, count
 * Operands: register or memory destination; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is defined for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: SHL is synonymous with SAL for integer operands and is commonly used for multiplication
 * by powers of two.
 *
 * Example:
 *   shl eax, 1
 */
static inline void write_shl(const char* dest, const char* src) {
    write_line3(ASM_SHL, dest, src);
}

/*
 * SHR - Shift Logical Right
 * Shifts the destination right, filling high bits with zero.
 *
 * Syntax: SHR dest, count
 * Operands: register or memory destination; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is defined for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: Use SHR for unsigned division by powers of two.
 *
 * Example:
 *   shr eax, 1
 */
static inline void write_shr(const char* dest, const char* src) {
    write_line3(ASM_SHR, dest, src);
}

/*
 * SAL - Shift Arithmetic Left
 * Alias of SHL for left shifts on integer operands.
 *
 * Syntax: SAL dest, count
 * Operands: register or memory destination; immediate count or CL
 * Flags affected: Same as SHL
 * Notes: MASM accepts SAL and SHL interchangeably for left shifts.
 *
 * Example:
 *   sal eax, 1
 */
static inline void write_sal(const char* dest, const char* src) {
    write_line3(ASM_SAL, dest, src);
}

/*
 * SAR - Shift Arithmetic Right
 * Shifts the destination right while replicating the original sign bit into the vacated high
 * bits.
 *
 * Syntax: SAR dest, count
 * Operands: register or memory destination; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is cleared for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: SAR preserves signedness and is commonly used for signed division by powers of two with
 * rounding toward negative infinity.
 *
 * Example:
 *   sar eax, 1
 */
static inline void write_sar(const char* dest, const char* src) {
    write_line3(ASM_SAR, dest, src);
}

/*
 * ROL - Rotate Left
 * Rotates bits left through the operand without involving the carry flag as part of the data
 * path.
 *
 * Syntax: ROL dest, count
 * Operands: register or memory operand; immediate count or CL
 * Flags affected: CF receives the last bit rotated out; OF is defined only for count 1
 * Notes: Rotates are useful for hashes, checksums, and bitfield permutations.
 *
 * Example:
 *   rol eax, 1
 */
static inline void write_rol(const char* dest, const char* src) {
    write_line3(ASM_ROL, dest, src);
}

/*
 * ROR - Rotate Right
 * Rotates bits right through the operand without involving the carry flag as part of the data
 * path.
 *
 * Syntax: ROR dest, count
 * Operands: register or memory operand; immediate count or CL
 * Flags affected: CF receives the last bit rotated out; OF is defined only for count 1
 * Notes: Common in crypto, hashing, and bitfield manipulation.
 *
 * Example:
 *   ror eax, 1
 */
static inline void write_ror(const char* dest, const char* src) {
    write_line3(ASM_ROR, dest, src);
}

/*
 * RCL - Rotate through Carry Left
 * Rotates the destination left through CF, treating carry as an extra bit.
 *
 * Syntax: RCL dest, count
 * Operands: register or memory operand; immediate count or CL
 * Flags affected: CF is part of the rotation path; OF is defined only for count 1
 * Notes: RCL is useful for multi-precision shifts and rotate operations across multiple words.
 *
 * Example:
 *   rcl eax, 1
 */
static inline void write_rcl(const char* dest, const char* src) {
    write_line3(ASM_RCL, dest, src);
}

/*
 * RCR - Rotate through Carry Right
 * Rotates the destination right through CF, treating carry as an extra bit.
 *
 * Syntax: RCR dest, count
 * Operands: register or memory operand; immediate count or CL
 * Flags affected: CF is part of the rotation path; OF is defined only for count 1
 * Notes: RCR complements RCL for multi-word arithmetic.
 *
 * Example:
 *   rcr eax, 1
 */
static inline void write_rcr(const char* dest, const char* src) {
    write_line3(ASM_RCR, dest, src);
}

/*
 * SHLD - Double-Precision Shift Left
 * Shifts the destination left while shifting bits in from a second source operand.
 *
 * Syntax: SHLD dest, src, count
 * Operands: destination register or memory; source register; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is defined for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: Useful for multi-word bitfield operations.
 *
 * Example:
 *   shld eax, 1
 */
static inline void write_shld(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_SHLD, dest, src, imm);
}

/*
 * SHRD - Double-Precision Shift Right
 * Shifts the destination right while shifting bits in from a second source operand.
 *
 * Syntax: SHRD dest, src, count
 * Operands: destination register or memory; source register; immediate count or CL
 * Flags affected: CF receives the last bit shifted out; OF is defined for count 1; SF, ZF, and
 * PF reflect the result when count is nonzero; AF is undefined
 * Notes: Useful for multi-word bitfield operations and funnel-shift style sequences.
 *
 * Example:
 *   shrd eax, 1
 */
static inline void write_shrd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_SHRD, dest, src, imm);
}

/*
 * ============================================================================
 * 10. Bit Manipulation
 * ============================================================================
 */

/*
 * BT - Bit Test
 * Copies the selected bit from the source operand into CF.
 *
 * Syntax: BT base, index
 * Operands: register or memory bit base; register or immediate bit index
 * Flags affected: CF receives the selected bit; other flags are unchanged
 * Notes: BT is commonly used to probe bitmaps and capability masks.
 *
 * Example:
 *   bt eax, ebx
 */
static inline void write_bt(const char* dest, const char* src) {
    write_line3(ASM_BT, dest, src);
}

/*
 * BTS - Bit Test and Set
 * Copies the selected bit into CF and then sets that bit in the destination.
 *
 * Syntax: BTS base, index
 * Operands: register or memory bit base; register or immediate bit index
 * Flags affected: CF receives the original bit value; other flags are unchanged
 * Notes: With LOCK on memory, BTS can atomically claim a bit.
 *
 * Example:
 *   bts eax, ebx
 */
static inline void write_bts(const char* dest, const char* src) {
    write_line3(ASM_BTS, dest, src);
}

/*
 * BTR - Bit Test and Reset
 * Copies the selected bit into CF and then clears that bit in the destination.
 *
 * Syntax: BTR base, index
 * Operands: register or memory bit base; register or immediate bit index
 * Flags affected: CF receives the original bit value; other flags are unchanged
 * Notes: Useful for bitmap allocators and flag words.
 *
 * Example:
 *   btr eax, ebx
 */
static inline void write_btr(const char* dest, const char* src) {
    write_line3(ASM_BTR, dest, src);
}

/*
 * BTC - Bit Test and Complement
 * Copies the selected bit into CF and then toggles that bit in the destination.
 *
 * Syntax: BTC base, index
 * Operands: register or memory bit base; register or immediate bit index
 * Flags affected: CF receives the original bit value; other flags are unchanged
 * Notes: With LOCK on memory, BTC can atomically flip a shared bit.
 *
 * Example:
 *   btc eax, ebx
 */
static inline void write_btc(const char* dest, const char* src) {
    write_line3(ASM_BTC, dest, src);
}

/*
 * BSF - Bit Scan Forward
 * Searches for the least-significant set bit and returns its index.
 *
 * Syntax: BSF reg, src
 * Operands: destination register; source register or memory
 * Flags affected: ZF=1 if the source is zero, otherwise ZF=0 and destination receives the bit
 * index; other flags are undefined
 * Notes: The destination is undefined when the source is zero.
 *
 * Example:
 *   bsf eax, ebx
 */
static inline void write_bsf(const char* dest, const char* src) {
    write_line3(ASM_BSF, dest, src);
}

/*
 * BSR - Bit Scan Reverse
 * Searches for the most-significant set bit and returns its index.
 *
 * Syntax: BSR reg, src
 * Operands: destination register; source register or memory
 * Flags affected: ZF=1 if the source is zero, otherwise ZF=0 and destination receives the bit
 * index; other flags are undefined
 * Notes: The destination is undefined when the source is zero.
 *
 * Example:
 *   bsr eax, ebx
 */
static inline void write_bsr(const char* dest, const char* src) {
    write_line3(ASM_BSR, dest, src);
}

/*
 * POPCNT - Population Count
 * Counts the number of set bits in the source operand and writes that count to the destination
 * register.
 *
 * Syntax: POPCNT reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: ZF=1 if the source is zero; CF, OF, SF, AF, and PF are cleared
 * Notes: Requires POPCNT support and is useful for bitset cardinality and rank operations.
 *
 * Example:
 *   popcnt eax, ebx
 */
static inline void write_popcnt(const char* dest, const char* src) {
    write_line3(ASM_POPCNT, dest, src);
}

/*
 * LZCNT - Leading Zero Count
 * Counts the number of leading zero bits in the source operand.
 *
 * Syntax: LZCNT reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: ZF is set if the result is zero; CF is set if the source is zero; other
 * arithmetic flags are cleared or undefined by the architecture
 * Notes: Requires ABM/LZCNT support and differs from BSR on zero inputs.
 *
 * Example:
 *   lzcnt eax, ebx
 */
static inline void write_lzcnt(const char* dest, const char* src) {
    write_line3(ASM_LZCNT, dest, src);
}

/*
 * TZCNT - Trailing Zero Count
 * Counts the number of trailing zero bits in the source operand.
 *
 * Syntax: TZCNT reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: ZF is set if the result is zero; CF is set if the source is zero; other
 * arithmetic flags are cleared or undefined by the architecture
 * Notes: Requires BMI1 support and differs from BSF on zero inputs.
 *
 * Example:
 *   tzcnt eax, ebx
 */
static inline void write_tzcnt(const char* dest, const char* src) {
    write_line3(ASM_TZCNT, dest, src);
}

/*
 * ANDN - Bitwise AND NOT
 * Computes the bitwise AND of the inverted first source and the second source and writes the
 * result to the destination.
 *
 * Syntax: ANDN reg, src1, src2
 * Operands: destination register; two register or register/memory sources
 * Flags affected: ZF and SF reflect the result; CF and OF are cleared; AF and PF are undefined
 * Notes: BMI1 instruction useful for mask clearing without an explicit NOT.
 *
 * Example:
 *   andn eax, ebx
 */
static inline void write_andn(const char* dest, const char* src) {
    write_line3(ASM_ANDN, dest, src);
}

/*
 * BEXTR - Bit Field Extract
 * Extracts a contiguous bit field from the source operand based on a start and length control
 * value.
 *
 * Syntax: BEXTR reg, src, control
 * Operands: destination register; source register or memory; control register or immediate form
 * depending on encoding
 * Flags affected: ZF=1 when the extracted result is zero; CF and OF are cleared; other flags are
 * undefined
 * Notes: BMI1 and TBM variants exist; MASM usually follows the Intel BMI1 form.
 *
 * Example:
 *   bextr eax, ebx
 */
static inline void write_bextr(const char* dest, const char* src) {
    write_line3(ASM_BEXTR, dest, src);
}

/*
 * BLSI - Extract Lowest Set Bit
 * Copies only the least-significant set bit of the source to the destination and clears all
 * other bits.
 *
 * Syntax: BLSI reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: CF=1 if the source was nonzero; ZF reflects whether the result is zero; OF is
 * cleared; SF reflects the result
 * Notes: BMI1 instruction useful for low-bit isolation.
 *
 * Example:
 *   blsi eax, ebx
 */
static inline void write_blsi(const char* dest, const char* src) {
    write_line3(ASM_BLSI, dest, src);
}

/*
 * BLSMSK - Mask from Lowest Set Bit
 * Generates a mask of ones from bit 0 through the least-significant set bit of the source.
 *
 * Syntax: BLSMSK reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: CF=1 if the source was nonzero; ZF reflects whether the result is zero; OF is
 * cleared; SF reflects the result
 * Notes: BMI1 instruction often paired with bit scanning and sparse-set algorithms.
 *
 * Example:
 *   blsmsk eax, ebx
 */
static inline void write_blsmsk(const char* dest, const char* src) {
    write_line3(ASM_BLSMSK, dest, src);
}

/*
 * BLSR - Reset Lowest Set Bit
 * Copies the source to the destination while clearing only the least-significant set bit.
 *
 * Syntax: BLSR reg, r/m
 * Operands: destination register; source register or memory
 * Flags affected: CF=1 if the source was zero; ZF reflects whether the result is zero; OF is
 * cleared; SF reflects the result
 * Notes: BMI1 instruction useful for iterating through set bits.
 *
 * Example:
 *   blsr eax, ebx
 */
static inline void write_blsr(const char* dest, const char* src) {
    write_line3(ASM_BLSR, dest, src);
}

/*
 * BZHI - Zero High Bits
 * Copies the low part of the source operand up to the index count and clears all higher bits.
 *
 * Syntax: BZHI reg, src, index
 * Operands: destination register; source register or memory; index register
 * Flags affected: ZF and SF reflect the result; CF reports whether the index exceeded the
 * operand width; OF is cleared
 * Notes: BMI2 instruction useful for mask generation without an explicit shift and subtract.
 *
 * Example:
 *   bzhi eax, ebx
 */
static inline void write_bzhi(const char* dest, const char* src) {
    write_line3(ASM_BZHI, dest, src);
}

/*
 * PDEP - Parallel Bit Deposit
 * Deposits contiguous low bits from one source into positions selected by a mask operand.
 *
 * Syntax: PDEP reg, src, mask
 * Operands: destination register; source register; mask register or memory
 * Flags affected: None
 * Notes: BMI2 instruction used for Morton codes, sparse bit packing, and table-free
 * permutations.
 *
 * Example:
 *   pdep eax, ebx
 */
static inline void write_pdep(const char* dest, const char* src) {
    write_line3(ASM_PDEP, dest, src);
}

/*
 * PEXT - Parallel Bit Extract
 * Extracts bits selected by a mask operand and packs them contiguously into the destination.
 *
 * Syntax: PEXT reg, src, mask
 * Operands: destination register; source register; mask register or memory
 * Flags affected: None
 * Notes: BMI2 instruction used for dense extraction from sparse bitfields.
 *
 * Example:
 *   pext eax, ebx
 */
static inline void write_pext(const char* dest, const char* src) {
    write_line3(ASM_PEXT, dest, src);
}

/*
 * ============================================================================
 * 11. Control Flow
 * ============================================================================
 */

/*
 * JMP - Unconditional Jump
 * Transfers control to the target address without saving a return address.
 *
 * Syntax: JMP target
 * Operands: relative label, register, or memory indirect target depending on form
 * Flags affected: None
 * Notes: Short, near, and far forms exist. MASM chooses the shortest legal encoding for label-
 * based relative jumps unless directed otherwise.
 *
 * Example:
 *   jmp done
 *   jmp qword ptr [rax]
 */
static inline void write_jmp(const char* operand) {
    write_line2(ASM_JMP, operand);
}

/*
 * write_extern_call - Emit a CALL with x64 Windows calling convention scaffolding.
 *
 * This function is intended for interop calls to external/Windows API functions
 * (e.g. ExitProcess, WriteFile, printf). It wraps write_call (which performs the
 * argument validation, the register-argument moves, and the CALL common to every
 * call) with the additional setup the x64 Windows convention requires:
 *
 *   sub rsp, 40         ; allocate 32 bytes shadow space + 8 bytes alignment
 *   mov rcx, <arg0>     ; first parameter (if any)      } emitted by write_call
 *   mov rdx, <arg1>     ; second parameter (if any)     }
 *   call <target>       ;                                }
 *   add rsp, 40         ; reclaim the allocated space
 *
 * (Arguments beyond the four register slots are written to [rsp+32], [rsp+40], …
 * before the CALL; see the body.)
 *
 * The 40 bytes break down as:
 *   - 32 bytes of shadow space (aka home space): the callee is allowed to spill
 *     the four register parameters (rcx, rdx, r8, r9) here. This is required
 *     even if the callee takes fewer than four arguments, and is what makes
 *     variadic functions work (spilled args become contiguous with stack args).
 *   - 8 bytes of padding: the previous call instruction pushed an 8-byte return
 *     address, misaligning RSP. This extra 8 bytes restores 16-byte alignment,
 *     which the convention requires at the point of every call.
 *
 * The AsmExtern struct (returned by write_extern) carries the function name,
 * the register array, and the parameter count so that write_extern_call can
 * automatically emit the correct mov instructions for each argument.
 *
 * For your own internal functions, you do NOT need to follow this convention.
 * The x64 calling convention is a software contract for interop — the call and
 * ret instructions themselves are purely mechanical (push return address / pop
 * and jump back) and enforce nothing about registers, shadow space, or stack
 * alignment. Between your own functions you are free to:
 *   - Pass arguments in any registers you choose
 *   - Skip shadow space allocation entirely
 *   - Ignore 16-byte stack alignment
 *   - Use call/ret purely for the jump-and-return mechanism
 *
 * For such internal calls use write_call directly (below): it marshals the
 * register arguments and emits the CALL, but adds none of the x64 frame. The
 * only rule is: if a custom internal function itself calls an external/Windows
 * function at some point, it must set up the x64 convention (via
 * write_extern_call) at that specific call site.
 *
 * Alternatively, for code blocks that don't need to be reusable from multiple
 * call sites, labels and jmp can be used to avoid call/ret overhead entirely.
 * The trade-off is that jmp has no built-in return mechanism — the caller must
 * either duplicate the block or manually store a return address in a register.
 *
 * To consume the result, read the extern's return_reg cell (the register/type
 * the result comes back in) after the call — e.g.
 * write_extern_call(&create_file, ...);
 * AsmDataField rax_hFile = create_file.return_reg;
 * write_set_mov(hFile, rax_hFile); — instead of re-spelling the return
 * register/type literally.
 */
#define ASM_CALL_MAX_PARAMS 16

/*
 * write_callv is the shared core of write_call (defined below). write_extern_call
 * lays down the x64 frame and then delegates the register-argument moves and the
 * CALL itself to this routine, so the "place args + call" logic lives in one place.
 */
static inline const char* write_callv_target(const AsmExtern* ext, va_list args,
                                             int through_import_address);
static inline const char* write_callv(const AsmExtern* ext, va_list args);

/* Windows x64 CALL of `ext`: shadow frame, stack slots, then the register
 * moves. `args` is the va_list of AsmDataField arguments. */
static inline void write_extern_callv(const AsmExtern* ext, va_list args) {
    va_list args_for_call;
    va_copy(args_for_call, args);

    /*
     * Snapshot the arguments so the stack parameters (5th+) can be placed in the
     * reserved frame below. The register parameters and the CALL itself are then
     * emitted by write_call's core (write_callv) from the copied va_list.
     */
    TypedValue tvs[ASM_CALL_MAX_PARAMS];
    for (int i = 0; i < ext->param_count; i++) {
        AsmDataField a = va_arg(args, AsmDataField);
        tvs[i].value = a.name;
        tvs[i].type  = a.type;
    }
    /*
     * Stack-frame layout at the point of CALL (x64 Windows ABI):
     *
     *   rsp+0  .. rsp+31 : 32-byte shadow (home) space for rcx/rdx/r8/r9
     *   rsp+32 .. rsp+.. : 5th, 6th, ... stack arguments (contiguous)
     *   [optional 8-byte pad above the args to keep rsp 16-byte aligned]
     *
     * The caller is assumed 16-byte aligned at this call site (rsp ≡ 8 mod 16
     * here, immediately after this frame's return-address push). The whole frame
     * is reserved with a single SUB so that the stack arguments land at exactly
     * [rsp+32] onward, and the total reservation is forced to ≡ 8 (mod 16) so
     * that rsp is a multiple of 16 when CALL executes.
     */
    int reg_count = asm_extern_reg_count(ext);
    int stack_args = ext->param_count - reg_count;
    if (stack_args < 0) stack_args = 0;

    int frame = asm_call_frame_size(32 + stack_args * 8);

    const char* frame_tok = asm_frame_token(frame);
    write_line3(ASM_SUB, ASM_RSP, frame_tok);
    asm_note_rsp_sub(frame);

    /*
     * Place the 5th+ arguments. Routing each value through the volatile r11
     * scratch register lets any operand (64-bit immediate, register, or memory)
     * be stored, which a direct mov-immediate-to-memory cannot always encode.
     * r11 is not an argument register, so this never clobbers a value we still
     * need. The destination slot is taken from the parameter's declared location
     * (params[i].name); we cross-check it against the slot the frame reserves so
     * a mistaken declaration is caught rather than silently miscompiled.
     *
     * rsp is biased down by 8 across just these stores so that each write targets
     * the very expression the callee will later read it from. CALL pushes an
     * 8-byte return address before the callee runs, so a value the callee reads
     * at [rsp+40] would otherwise have to be written 8 bytes lower (at [rsp+32])
     * by the caller. Dropping rsp by 8 for the stores makes the write operand
     * identical to the read operand ([rsp+40], [rsp+48], ...); the 8 is added
     * back before CALL so the return address and frame teardown land exactly
     * where the ABI frame expects them (rsp is 16-byte aligned at the CALL).
     */
    if (stack_args > 0) {
        write_line3(ASM_SUB, ASM_RSP, "8");
        asm_note_rsp_sub(8);
    }
    for (int i = reg_count; i < ext->param_count; i++) {
        int off = 40 + (i - reg_count) * 8;
        const char* slot = ext->params ? ext->params[i].name : NULL;
        const char* expect = asm_stack_slot_token(off);
        if (slot == NULL || strcmp(slot, expect) != 0) {
            asm_fail_fmt("error: %s stack argument %d: expected location \"%s\", got \"%s\"",
                         ext->name, i, expect, slot ? slot : "(null)");
        }
        write_line3(ASM_MOV, "r11", tvs[i].value);
        write_line3(ASM_MOV, slot, "r11");
    }
    if (stack_args > 0) {
        write_line3(ASM_ADD, ASM_RSP, "8");
        asm_note_rsp_add(8);
    }

    /* Register parameters + the configured imported-symbol CALL spelling. */
    write_callv_target(ext, args_for_call,
                       ASM_USE_VISUAL_C_DLL_CALLS && ext->is_dll_import);
    va_end(args_for_call);

    /* Reclaim the entire frame (shadow + stack args + alignment pad). */
    write_line3(ASM_ADD, ASM_RSP, frame_tok);
    asm_note_rsp_add(frame);
}

/* Windows x64 CALL of `ext`, with shadow space and stack arguments. The `...`
 * arguments are AsmDataField cells, one per parameter of `ext`. */
static inline void write_extern_call(const AsmExtern* ext, ...) {
    va_list args;
    va_start(args, ext);
    write_extern_callv(ext, args);
    va_end(args);
}

/*
 * write_call - Emit a CALL with argument marshalling for a private convention.
 *
 * This is the minimal "logical" call: everything that ANY call needs, with
 * nothing tied to a specific ABI. Given an AsmExtern describing the callee's
 * convention (its register list, parameter count/types, and how many parameters
 * travel in registers), it:
 *
 *   1. validates each supplied argument cell's type against the declared parameter type,
 *   2. emits a mov to load each register parameter into its register, and
 *   3. emits the CALL:
 *
 *   mov rcx, <arg0>     ; first register parameter (if any)
 *   mov rdx, <arg1>     ; second register parameter (if any)
 *   ...
 *   call <target>
 *
 * It does NOT reserve shadow space, align the stack, or place stack arguments —
 * those belong to a specific calling convention. For interop with
 * external/Windows API functions use write_extern_call, which wraps this with
 * the x64 Windows calling-convention scaffolding.
 *
 * Use write_call directly for calls between your own internal routines that
 * follow a private register-based convention. ext is typically the AsmExtern
 * returned by write_proc. To consume the result, read ext->return_reg (the cell
 * the result comes back in) after the call, instead of repeating the return
 * register/type literals.
 *
 * Each argument after ext is a single AsmDataField cell {value, type}; pass a
 * named cell (e.g. msgStartedLen) or a compound literal for an immediate (e.g.
 * (AsmDataField){"123", "exit_code"}). The cells are read in order and validated
 * against the declared parameter types.
 */

/*
 * Move src[i] into dest[i] without destroying a source a later move still
 * reads. A GPR here is one of the sixteen integer registers named by
 * asm_gpr_qword (rax through r15). xmm registers are not part of this
 * shuffle; call arguments that travel in xmm are not passed through here.
 *
 * Left to right is wrong when the buffer is still in rcx and the handle
 * has to occupy rcx:
 *   mov rcx, rax
 *   mov rdx, rcx      ; rcx no longer holds the buffer
 * A destination that no remaining source uses can be written now. That
 * move is retired and the search repeats. A cycle (rcx and rdx swapping,
 * or a longer loop of GPRs) has no such move: every destination is still
 * somebody's source. The cycle is broken by parking pending[0]'s source
 * in a GPR that is neither a remaining source nor a remaining destination
 * (rsp and rbp are never candidates; they are the stack and the frame).
 * The parked register becomes that move's source, which frees the GPR the
 * value just left, and the search finds a move on the next pass.
 *
 * Identical source and destination pairs are skipped. The function aborts
 * if the shuffle does not finish (a bug) or if every GPR is in the cycle
 * (there is then no scratch register).
 */
static inline void asm_emit_arg_moves(const char** dest, const char** src, int n) {
    int pending[ASM_CALL_MAX_PARAMS];
    int np = 0;
    for (int i = 0; i < n; i++) {
        if (dest[i] != NULL && src[i] != NULL && strcmp(dest[i], src[i]) == 0)
            continue;
        pending[np++] = i;
    }
    int steps = 0;
    while (np > 0) {
        crash_if(steps++ > n * 3, "asm: argument shuffle did not finish");
        int picked = -1;
        for (int p = 0; p < np; p++) {
            int d = asm_gpr_index(dest[pending[p]]);
            int blocked = 0;
            if (d >= 0) {
                for (int q = 0; q < np; q++) {
                    if (q == p) continue;
                    if (asm_gpr_index(src[pending[q]]) == d) { blocked = 1; break; }
                }
            }
            if (!blocked) { picked = p; break; }
        }
        if (picked >= 0) {
            int i = pending[picked];
            write_line3(ASM_MOV, dest[i], src[i]);
            pending[picked] = pending[np - 1];
            np--;
            continue;
        }
        int scratch = -1;
        for (int cand = 0; cand < 16; cand++) {
            if (cand == 4 || cand == 5) continue; /* rsp, rbp */
            int used = 0;
            for (int p = 0; p < np; p++) {
                if (asm_gpr_index(dest[pending[p]]) == cand) used = 1;
                if (asm_gpr_index(src[pending[p]]) == cand) used = 1;
            }
            if (!used) { scratch = cand; break; }
        }
        crash_if(scratch < 0, "asm: no free register to break an argument cycle");
        write_line3(ASM_MOV, asm_gpr_qword[scratch], src[pending[0]]);
        src[pending[0]] = asm_gpr_qword[scratch];
    }
}

/* Check `args` against `ext`, move the register parameters, and CALL.
 * Nonzero `through_import_address` calls the __imp_ slot. Returns the callee name. */
static inline const char* write_callv_target(const AsmExtern* ext, va_list args,
                                             int through_import_address) {
    /* ext is an AsmCall (the typedef AsmExtern is the same struct). It is the
     * contract for this call: name, parameter cells, return register, import
     * flag, volatile mask. The name "extern" is the historical one, from
     * write_extern. The same descriptor describes an internal proc and a
     * call through a register. is_dll_import is what marks an import. */
    TypedValue tvs[ASM_CALL_MAX_PARAMS];
    for (int i = 0; i < ext->param_count; i++) {
        AsmDataField a = va_arg(args, AsmDataField);
        tvs[i].value = a.name;
        tvs[i].type  = asm_field_use_as(a);
        if (strcmp(tvs[i].type, ext->params[i].type) != 0) {
            asm_fail_fmt("error: %s argument %d: expected type \"%s\", got \"%s\"",
                         ext->name, i, ext->params[i].type, tvs[i].type);
        }
        /* Validate the input against what the dictionary knows lives there. */
        assert_type(a);
    }
    /* Place the register parameters. Any stack parameters are the specific
     * calling convention's responsibility and are set up before this runs.
     * The moves are ordered so a source that still lives in an argument
     * register is read before that register is overwritten. */
    int reg_count = asm_extern_reg_count(ext);
    {
        const char* dests[ASM_CALL_MAX_PARAMS];
        const char* srcs[ASM_CALL_MAX_PARAMS];
        int nmov = 0;
        for (int i = 0; i < reg_count && i < ext->param_count; i++) {
            dests[nmov] = ext->params[i].name;
            srcs[nmov] = tvs[i].value;
            nmov++;
        }
        asm_emit_arg_moves(dests, srcs, nmov);
    }
    if (through_import_address) {
        for (int i = 0; i < asm_indent_level; i++) {
            write("    ");
        }
        write(ASM_CALL);
        write(" qword ptr [__imp_");
        write(ext->name);
        write("]\n");
    } else {
        write_line2(ASM_CALL, ext->name);
    }
    /* The call leaves its result in the return register with a known tag; record
     * it so subsequent assert_type / write_set_mov calls can validate it. */
    if (ext->return_reg.name != NULL) {
        asm_type_dict_set(ext->return_reg.name, ext->return_reg.type);
    }
    if (ext->volatile_mask && asm_proc_preserved) {
        unsigned conflict = ext->volatile_mask & asm_proc_preserved & ~asm_proc_saved;
        crash_if(conflict != 0,
                 "call clobbers a non-volatile register this procedure has not saved");
    }
    if (asm_after_call_hook) asm_after_call_hook(ext->name);
    return ext->name;
}

/* Check `args`, move the register parameters, and CALL `ext`. A DLL `ext`
 * uses the __imp_ slot when the Visual C call spelling is on. Returns the callee name.
 *
 * ext is the callee descriptor. args is the operand list, one cell per parameter.
 * A private call does not build a Windows frame, but the callee is still entered
 * with rsp 8 mod 16. That is the remainder write_proc records. A direct call
 * from a body already at 8 mod 16 would enter at 0, and a Windows import inside
 * that callee would then fault. lea closes the gap and does not change flags.
 * The pad is removed after the call, so the caller's rsp is unchanged. */
static inline const char* write_callv(const AsmExtern* ext, va_list args) {
    int pad = asm_call_frame_size(0);
    if (pad) {
        char tmp[32];
        int n = snprintf(tmp, sizeof tmp, "[rsp-%d]", pad);
        if (n < 0 || n >= (int)sizeof tmp) n = (int)sizeof tmp - 1;
        char* down = (char*)asm_bump_alloc((size_t)n + 1);
        memcpy(down, tmp, (size_t)n + 1);
        write_lea("rsp", down);
        asm_note_rsp_sub(pad);
    }
    const char* name = write_callv_target(ext, args,
                              ASM_USE_VISUAL_C_DLL_CALLS && ext->is_dll_import);
    if (pad) {
        char tmp[32];
        int n = snprintf(tmp, sizeof tmp, "[rsp+%d]", pad);
        if (n < 0 || n >= (int)sizeof tmp) n = (int)sizeof tmp - 1;
        char* up = (char*)asm_bump_alloc((size_t)n + 1);
        memcpy(up, tmp, (size_t)n + 1);
        write_lea("rsp", up);
        asm_note_rsp_add(pad);
    }
    return name;
}

/* Documented at the comment block above write_extern_call / write_callv_target.
 * Instruction wrappers earlier in this file each carry their own comment. */
/* call `name` after the arguments are already in place. Prefer write_call
 * when the callee has a descriptor. */
static inline void write_call_symbol(const char* name) {
    write_line2(ASM_CALL, name);
}

/* CALL `ext` after placing its register arguments. The `...` arguments are
 * AsmDataField cells in parameter order. */
static inline void write_call(const AsmExtern* ext, ...) {
    va_list args;
    va_start(args, ext);
    write_callv(ext, args);
    va_end(args);
}

/*
 * RET - Return from Procedure
 * Pops the saved return address from the stack and transfers control back to the caller.
 *
 * Syntax: RET [imm16]
 * Operands: implicit stack top with optional immediate stack adjustment
 * Flags affected: None
 * Notes: The optional immediate releases stack arguments in callee-clean conventions.
 *
 * Example:
 *   ret
 *   ret 16
 */
static inline void write_ret(void) {
    /* Confirm the value left in the return register matches the procedure's
     * declared return type (no-op outside a procedure / for void returns). */
    assert_type(asm_current_return_reg);
    write_line(ASM_RET);
}

/*
 * RETN - Near Return
 * Returns to a near caller within the current code segment.
 *
 * Syntax: RETN [imm16]
 * Operands: implicit stack top with optional immediate stack adjustment
 * Flags affected: None
 * Notes: RETN is an explicit near-return spelling equivalent to the ordinary near RET form.
 *
 * Example:
 *   retn
 */
static inline void write_retn(void) {
    write_line(ASM_RETN);
}

/*
 * RETF - Far Return
 * Returns to a far caller by popping IP/EIP/RIP and CS, optionally with stack adjustment.
 *
 * Syntax: RETF [imm16]
 * Operands: implicit far return frame on the stack
 * Flags affected: None
 * Notes: Used in segmented code, call gates, and some system transitions rather than ordinary
 * flat-model applications.
 *
 * Example:
 *   retf
 */
static inline void write_retf(void) {
    write_line(ASM_RETF);
}

/*
 * JA - Jump if Above / unsigned greater
 * Transfers control to the target label only when the condition CF=0 and ZF=0 is true in the
 * current status flags.
 *
 * Syntax: JA label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   ja target
 */
static inline void write_ja(const char* operand) {
    write_line2(ASM_JA, operand);
}

/*
 * JAE - Jump if Above or Equal / unsigned >=
 * Transfers control to the target label only when the condition CF=0 is true in the current
 * status flags.
 *
 * Syntax: JAE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jae target
 */
static inline void write_jae(const char* operand) {
    write_line2(ASM_JAE, operand);
}

/*
 * JB - Jump if Below / unsigned <
 * Transfers control to the target label only when the condition CF=1 is true in the current
 * status flags.
 *
 * Syntax: JB label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jb target
 */
static inline void write_jb(const char* operand) {
    write_line2(ASM_JB, operand);
}

/*
 * JBE - Jump if Below or Equal / unsigned <=
 * Transfers control to the target label only when the condition CF=1 or ZF=1 is true in the
 * current status flags.
 *
 * Syntax: JBE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jbe target
 */
static inline void write_jbe(const char* operand) {
    write_line2(ASM_JBE, operand);
}

/*
 * write_if_unsigned_less_than_or_equal_then_jump_to - Combined CMP + JBE.
 *
 * Emits an unsigned comparison of dest against src followed by a JBE, so control
 * transfers to `label` exactly when dest <= src (unsigned). Both compared
 * operands carry a type tag (see write_cmp); the branch reads the flags the CMP
 * sets. Folding the compare and the branch into one call keeps the condition and
 * its operands together at the call site. Returns `label` so the matching
 * `write_label` can reuse the same handle instead of repeating the literal.
 */
static inline const char* write_if_unsigned_less_than_or_equal_then_jump_to(
        const char* label,
        AsmDataField dest, AsmDataField src) {
    write_cmp(dest, src);
    write_jbe(label);
    return label;
}

/*
 * write_if_unsigned_less_than_then_jump_to - Combined CMP + JB.
 *
 * Emits an unsigned comparison of dest against src followed by a JB, so control
 * transfers to `label` exactly when dest < src (unsigned). Both compared
 * operands carry a type tag (see write_cmp); the branch reads the flags the CMP
 * sets. Folding the compare and the branch into one call keeps the condition and
 * its operands together at the call site. Returns `label` so the matching
 * `write_label` can reuse the same handle instead of repeating the literal.
 */
static inline const char* write_if_unsigned_less_than_then_jump_to(
        const char* label,
        AsmDataField dest, AsmDataField src) {
    write_cmp(dest, src);
    write_jb(label);
    return label;
}

/*
 * JC - Jump if Carry set
 * Transfers control to the target label only when the condition CF=1 is true in the current
 * status flags.
 *
 * Syntax: JC label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jc target
 */
static inline void write_jc(const char* operand) {
    write_line2(ASM_JC, operand);
}

/*
 * JE - Jump if Equal / zero
 * Transfers control to the target label only when the condition ZF=1 is true in the current
 * status flags.
 *
 * Syntax: JE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   je target
 */
static inline void write_je(const char* operand) {
    write_line2(ASM_JE, operand);
}

/*
 * JG - Jump if Greater / signed >
 * Transfers control to the target label only when the condition ZF=0 and SF=OF is true in the
 * current status flags.
 *
 * Syntax: JG label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jg target
 */
static inline void write_jg(const char* operand) {
    write_line2(ASM_JG, operand);
}

/*
 * JGE - Jump if Greater or Equal / signed >=
 * Transfers control to the target label only when the condition SF=OF is true in the current
 * status flags.
 *
 * Syntax: JGE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jge target
 */
static inline void write_jge(const char* operand) {
    write_line2(ASM_JGE, operand);
}

/*
 * JL - Jump if Less / signed <
 * Transfers control to the target label only when the condition SF!=OF is true in the current
 * status flags.
 *
 * Syntax: JL label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jl target
 */
static inline void write_jl(const char* operand) {
    write_line2(ASM_JL, operand);
}

/*
 * JLE - Jump if Less or Equal / signed <=
 * Transfers control to the target label only when the condition ZF=1 or SF!=OF is true in the
 * current status flags.
 *
 * Syntax: JLE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jle target
 */
static inline void write_jle(const char* operand) {
    write_line2(ASM_JLE, operand);
}

/*
 * JNA - Jump if Not Above / unsigned <=
 * Transfers control to the target label only when the condition CF=1 or ZF=1 is true in the
 * current status flags.
 *
 * Syntax: JNA label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jna target
 */
static inline void write_jna(const char* operand) {
    write_line2(ASM_JNA, operand);
}

/*
 * JNAE - Jump if Not Above or Equal / unsigned <
 * Transfers control to the target label only when the condition CF=1 is true in the current
 * status flags.
 *
 * Syntax: JNAE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnae target
 */
static inline void write_jnae(const char* operand) {
    write_line2(ASM_JNAE, operand);
}

/*
 * JNB - Jump if Not Below / unsigned >=
 * Transfers control to the target label only when the condition CF=0 is true in the current
 * status flags.
 *
 * Syntax: JNB label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnb target
 */
static inline void write_jnb(const char* operand) {
    write_line2(ASM_JNB, operand);
}

/*
 * JNBE - Jump if Not Below or Equal / unsigned >
 * Transfers control to the target label only when the condition CF=0 and ZF=0 is true in the
 * current status flags.
 *
 * Syntax: JNBE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnbe target
 */
static inline void write_jnbe(const char* operand) {
    write_line2(ASM_JNBE, operand);
}

/*
 * JNC - Jump if No carry
 * Transfers control to the target label only when the condition CF=0 is true in the current
 * status flags.
 *
 * Syntax: JNC label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnc target
 */
static inline void write_jnc(const char* operand) {
    write_line2(ASM_JNC, operand);
}

/*
 * JNE - Jump if Not Equal / nonzero
 * Transfers control to the target label only when the condition ZF=0 is true in the current
 * status flags.
 *
 * Syntax: JNE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jne target
 */
static inline void write_jne(const char* operand) {
    write_line2(ASM_JNE, operand);
}

/*
 * write_if_not_equal_then_jump_to - Combined CMP + JNE.
 *
 * Emits a comparison of dest against src followed by a JNE, so control transfers
 * to `label` exactly when dest != src. Both compared operands carry a type tag
 * (see write_cmp); the branch reads the flags the CMP sets. Folding the compare
 * and the branch into one call keeps the condition and its operands together at
 * the call site. Returns `label` so the matching `write_label` can reuse the
 * same handle instead of repeating the literal.
 */
static inline const char* write_if_not_equal_then_jump_to(
        const char* label,
        AsmDataField dest, AsmDataField src) {
    write_cmp(dest, src);
    write_jne(label);
    return label;
}

/*
 * write_if_signed_less_than_or_equal_then_jump_to - Combined CMP + JLE.
 *
 * Emits a signed comparison of dest against src followed by a JLE, so control
 * transfers to `label` exactly when dest <= src (signed). Both compared operands
 * carry a type tag (see write_cmp); the branch reads the flags the CMP sets.
 * Folding the compare and the branch into one call keeps the condition and its
 * operands together at the call site. Returns `label` so the matching
 * `write_label` can reuse the same handle instead of repeating the literal.
 */
static inline const char* write_if_signed_less_than_or_equal_then_jump_to(
        const char* label,
        AsmDataField dest, AsmDataField src) {
    write_cmp(dest, src);
    write_jle(label);
    return label;
}

/*
 * write_if_unsigned_greater_than_or_equal_then_jump_to - Combined CMP + JAE.
 *
 * Emits an unsigned comparison of dest against src followed by a JAE, so control
 * transfers to label exactly when dest >= src (unsigned). Both compared
 * operands carry a type tag (see write_cmp). Returns label so the matching
 * write_label can reuse the same handle.
 */
static inline const char* write_if_unsigned_greater_than_or_equal_then_jump_to(
        const char* label,
        AsmDataField dest, AsmDataField src) {
    write_cmp(dest, src);
    write_jae(label);
    return label;
}

/*
 * write_if_signed_greater_than_or_equal_then_jump_to - Combined CMP + JGE.
 *
 * Emits a signed comparison of dest against src followed by a JGE, so control
 * transfers to label exactly when dest >= src (signed). Both compared
 * operands carry a type tag (see write_cmp). Returns label so the matching
 * write_label can reuse the same handle.
 */
static inline const char* write_if_signed_greater_than_or_equal_then_jump_to(
        const char* label,
        AsmDataField dest, AsmDataField src) {
    write_cmp(dest, src);
    write_jge(label);
    return label;
}

/*
 * write_if_equal_then_jump_to - Combined CMP + JE.
 *
 * Emits a comparison of dest against src followed by a JE, so control transfers
 * to label exactly when dest == src. Both compared operands carry a type tag
 * (see write_cmp). Returns label so the matching write_label can reuse the
 * same handle.
 */
static inline const char* write_if_equal_then_jump_to(
        const char* label,
        AsmDataField dest, AsmDataField src) {
    write_cmp(dest, src);
    write_je(label);
    return label;
}

/*
 * JNG - Jump if Not Greater / signed <=
 * Transfers control to the target label only when the condition ZF=1 or SF!=OF is true in the
 * current status flags.
 *
 * Syntax: JNG label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jng target
 */
static inline void write_jng(const char* operand) {
    write_line2(ASM_JNG, operand);
}

/*
 * JNGE - Jump if Not Greater or Equal / signed <
 * Transfers control to the target label only when the condition SF!=OF is true in the current
 * status flags.
 *
 * Syntax: JNGE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnge target
 */
static inline void write_jnge(const char* operand) {
    write_line2(ASM_JNGE, operand);
}

/*
 * JNL - Jump if Not Less / signed >=
 * Transfers control to the target label only when the condition SF=OF is true in the current
 * status flags.
 *
 * Syntax: JNL label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnl target
 */
static inline void write_jnl(const char* operand) {
    write_line2(ASM_JNL, operand);
}

/*
 * JNLE - Jump if Not Less or Equal / signed >
 * Transfers control to the target label only when the condition ZF=0 and SF=OF is true in the
 * current status flags.
 *
 * Syntax: JNLE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnle target
 */
static inline void write_jnle(const char* operand) {
    write_line2(ASM_JNLE, operand);
}

/*
 * JNO - Jump if No overflow
 * Transfers control to the target label only when the condition OF=0 is true in the current
 * status flags.
 *
 * Syntax: JNO label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jno target
 */
static inline void write_jno(const char* operand) {
    write_line2(ASM_JNO, operand);
}

/*
 * JNP - Jump if Not parity / parity odd
 * Transfers control to the target label only when the condition PF=0 is true in the current
 * status flags.
 *
 * Syntax: JNP label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnp target
 */
static inline void write_jnp(const char* operand) {
    write_line2(ASM_JNP, operand);
}

/*
 * JNS - Jump if Not sign
 * Transfers control to the target label only when the condition SF=0 is true in the current
 * status flags.
 *
 * Syntax: JNS label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jns target
 */
static inline void write_jns(const char* operand) {
    write_line2(ASM_JNS, operand);
}

/*
 * JNZ - Jump if Not zero
 * Transfers control to the target label only when the condition ZF=0 is true in the current
 * status flags.
 *
 * Syntax: JNZ label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jnz target
 */
static inline void write_jnz(const char* operand) {
    write_line2(ASM_JNZ, operand);
}

/*
 * JO - Jump if Overflow
 * Transfers control to the target label only when the condition OF=1 is true in the current
 * status flags.
 *
 * Syntax: JO label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jo target
 */
static inline void write_jo(const char* operand) {
    write_line2(ASM_JO, operand);
}

/*
 * JP - Jump if Parity / parity even
 * Transfers control to the target label only when the condition PF=1 is true in the current
 * status flags.
 *
 * Syntax: JP label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jp target
 */
static inline void write_jp(const char* operand) {
    write_line2(ASM_JP, operand);
}

/*
 * JPE - Jump if Parity even
 * Transfers control to the target label only when the condition PF=1 is true in the current
 * status flags.
 *
 * Syntax: JPE label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jpe target
 */
static inline void write_jpe(const char* operand) {
    write_line2(ASM_JPE, operand);
}

/*
 * JPO - Jump if Parity odd
 * Transfers control to the target label only when the condition PF=0 is true in the current
 * status flags.
 *
 * Syntax: JPO label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jpo target
 */
static inline void write_jpo(const char* operand) {
    write_line2(ASM_JPO, operand);
}

/*
 * JS - Jump if Sign
 * Transfers control to the target label only when the condition SF=1 is true in the current
 * status flags.
 *
 * Syntax: JS label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   js target
 */
static inline void write_js(const char* operand) {
    write_line2(ASM_JS, operand);
}

/*
 * JZ - Jump if Zero
 * Transfers control to the target label only when the condition ZF=1 is true in the current
 * status flags.
 *
 * Syntax: JZ label
 * Operands: short or near relative target selected by the assembler
 * Flags affected: None
 * Notes: Jcc instructions read flags set by earlier CMP, TEST, arithmetic, or logical
 * instructions. They do not modify flags themselves.
 *
 * Example:
 *   cmp eax, ebx
 *   jz target
 */
static inline void write_jz(const char* operand) {
    write_line2(ASM_JZ, operand);
}

/*
 * JECXZ - Jump if ECX is Zero
 * Transfers control to the target if ECX equals zero.
 *
 * Syntax: JECXZ label
 * Operands: short relative label
 * Flags affected: None
 * Notes: Useful for loop setup in 32-bit code because it tests the count register directly
 * rather than the flags register.
 *
 * Example:
 *   jecxz empty
 */
static inline void write_jecxz(const char* operand) {
    write_line2(ASM_JECXZ, operand);
}

/*
 * JRCXZ - Jump if RCX is Zero
 * Transfers control to the target if RCX equals zero.
 *
 * Syntax: JRCXZ label
 * Operands: short relative label
 * Flags affected: None
 * Notes: 64-bit counterpart to JECXZ for loop setup and guard logic.
 *
 * Example:
 *   jrcxz empty
 */
static inline void write_jrcxz(const char* operand) {
    write_line2(ASM_JRCXZ, operand);
}

/*
 * LOOP - Counted Loop
 * Counted Loop decrements CX, ECX, or RCX according to address size and jumps when the count is
 * not zero.
 *
 * Syntax: LOOP label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loop next_item
 */
static inline void write_loop(const char* operand) {
    write_line2(ASM_LOOP, operand);
}

/*
 * LOOPE - Loop While Equal
 * Loop While Equal decrements the count register and jumps while the count is not zero and ZF=1.
 *
 * Syntax: LOOPE label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loope next_item
 */
static inline void write_loope(const char* operand) {
    write_line2(ASM_LOOPE, operand);
}

/*
 * LOOPNE - Loop While Not Equal
 * Loop While Not Equal decrements the count register and jumps while the count is not zero and
 * ZF=0.
 *
 * Syntax: LOOPNE label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loopne next_item
 */
static inline void write_loopne(const char* operand) {
    write_line2(ASM_LOOPNE, operand);
}

/*
 * LOOPZ - Loop While Zero
 * Loop While Zero alias of LOOPE that decrements the count and jumps while ZF=1 and the count is
 * nonzero.
 *
 * Syntax: LOOPZ label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loopz next_item
 */
static inline void write_loopz(const char* operand) {
    write_line2(ASM_LOOPZ, operand);
}

/*
 * LOOPNZ - Loop While Not Zero
 * Loop While Not Zero alias of LOOPNE that decrements the count and jumps while ZF=0 and the
 * count is nonzero.
 *
 * Syntax: LOOPNZ label
 * Operands: short relative label; implicit count register
 * Flags affected: None
 * Notes: LOOP-family instructions do not alter status flags; they only read ZF in the
 * conditional variants and always update the implicit count register.
 *
 * Example:
 *   loopnz next_item
 */
static inline void write_loopnz(const char* operand) {
    write_line2(ASM_LOOPNZ, operand);
}

/*
 * ============================================================================
 * 12. String Instructions
 * ============================================================================
 */

/*
 * CMPS - Compare String Data
 * Compare String Data compares memory elements at the implicit source and destination pointers,
 * sets flags as if a subtraction occurred, and advances or decrements the pointers according to
 * DF.
 *
 * Syntax: CMPS
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe cmps
 */
static inline void write_cmps(const char* dest, const char* src) {
    write_line3(ASM_CMPS, dest, src);
}

/*
 * CMPSB - Compare Byte String Data
 * Compare Byte String Data compares bytes at [DS:(E|R)SI] and [ES:(E|R)DI].
 *
 * Syntax: CMPSB
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe cmpsb
 */
static inline void write_cmpsb(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    write_line(ASM_CMPSB);
}

/*
 * CMPSW - Compare Word String Data
 * Compare Word String Data compares 16-bit elements and updates the string pointers by 2.
 *
 * Syntax: CMPSW
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe cmpsw
 */
static inline void write_cmpsw(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    write_line(ASM_CMPSW);
}

/*
 * CMPSD - Compare Doubleword String Data
 * Compare Doubleword String Data compares 32-bit elements and updates the string pointers by 4.
 *
 * Syntax: CMPSD
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe cmpsd
 */
static inline void write_cmpsd(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    write_line(ASM_CMPSD);
}

/*
 * CMPSQ - Compare Quadword String Data
 * Compare Quadword String Data compares 64-bit elements and updates RSI/RDI by 8.
 *
 * Syntax: CMPSQ
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe cmpsq
 */
static inline void write_cmpsq(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rsi");
    write_line(ASM_CMPSQ);
}

/*
 * SCAS - Scan String Data
 * Scan String Data compares AL/AX/EAX/RAX against the memory element at [ES:(E|R)DI], sets
 * flags, and updates the destination pointer according to DF.
 *
 * Syntax: SCAS
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe scas
 */
static inline void write_scas(const char* dest, const char* src) {
    write_line3(ASM_SCAS, dest, src);
}

/*
 * SCASB - Scan Byte String Data
 * Scan Byte String Data compares AL against the destination byte at [ES:(E|R)DI].
 *
 * Syntax: SCASB
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe scasb
 */
static inline void write_scasb(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "al");
    write_line(ASM_SCASB);
}

/*
 * SCASW - Scan Word String Data
 * Scan Word String Data compares AX against the destination word and updates the pointer by 2.
 *
 * Syntax: SCASW
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe scasw
 */
static inline void write_scasw(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "ax");
    write_line(ASM_SCASW);
}

/*
 * SCASD - Scan Doubleword String Data
 * Scan Doubleword String Data compares EAX against the destination dword and updates the pointer
 * by 4.
 *
 * Syntax: SCASD
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe scasd
 */
static inline void write_scasd(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "eax");
    write_line(ASM_SCASD);
}

/*
 * SCASQ - Scan Quadword String Data
 * Scan Quadword String Data compares RAX against the destination qword and updates RDI by 8.
 *
 * Syntax: SCASQ
 * Operands: implicit source/destination pointer registers and accumulator for SCAS
 * Flags affected: OF, SF, ZF, AF, CF, and PF are updated as if a subtraction had been performed
 * Notes: REP, REPE/REPZ, and REPNE/REPNZ commonly prefix these instructions to compare or scan
 * entire buffers.
 *
 * Example:
 *   repe scasq
 */
static inline void write_scasq(AsmDataField dst, AsmDataField src) {
    asm_require_reg(dst, "rdi");
    asm_require_reg(src, "rax");
    write_line(ASM_SCASQ);
}

/*
 * REP - Repeat Prefix
 * Repeat Prefix repeats the following string instruction while RCX/ECX/CX is nonzero.
 *
 * Syntax: REP string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   rep movsb
 */
static inline void write_rep(void) {
    write_line(ASM_REP);
}

/*
 * REPE - Repeat While Equal Prefix
 * Repeat While Equal Prefix repeats the following CMPS or SCAS instruction while the count is
 * nonzero and ZF=1.
 *
 * Syntax: REPE string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   repe movsb
 */
static inline void write_repe(void) {
    write_line(ASM_REPE);
}

/*
 * REPNE - Repeat While Not Equal Prefix
 * Repeat While Not Equal Prefix repeats the following CMPS or SCAS instruction while the count
 * is nonzero and ZF=0.
 *
 * Syntax: REPNE string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   repne movsb
 */
static inline void write_repne(void) {
    write_line(ASM_REPNE);
}

/*
 * REPZ - Repeat While Zero Prefix
 * Repeat While Zero Prefix alias of REPE.
 *
 * Syntax: REPZ string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   repz movsb
 */
static inline void write_repz(void) {
    write_line(ASM_REPZ);
}

/*
 * REPNZ - Repeat While Not Zero Prefix
 * Repeat While Not Zero Prefix alias of REPNE.
 *
 * Syntax: REPNZ string_instruction
 * Operands: repeat prefix plus a string instruction such as MOVSB, STOSQ, CMPSB, or SCASB
 * Flags affected: The prefix itself does not define new flags semantics; the repeated
 * instruction determines any flag updates
 * Notes: The count register is decremented each iteration. Some instructions accept only REP,
 * while compare/scan instructions can also use REPE/REPNE variants.
 *
 * Example:
 *   repnz movsb
 */
static inline void write_repnz(void) {
    write_line(ASM_REPNZ);
}

/*
 * ============================================================================
 * 13. Flag Instructions
 * ============================================================================
 */

/*
 * STC - Set Carry Flag
 * Sets CF to 1.
 *
 * Syntax: STC
 * Operands: implicit flags register
 * Flags affected: CF is set; all other flags are unchanged
 * Notes: Used before ADC, RCL, multi-precision arithmetic, or software carry generation.
 *
 * Example:
 *   stc
 */
static inline void write_stc(void) {
    write_line(ASM_STC);
}

/*
 * CLC - Clear Carry Flag
 * Clears CF to 0.
 *
 * Syntax: CLC
 * Operands: implicit flags register
 * Flags affected: CF is cleared; all other flags are unchanged
 * Notes: Often used before ADC or to normalize carry state.
 *
 * Example:
 *   clc
 */
static inline void write_clc(void) {
    write_line(ASM_CLC);
}

/*
 * CMC - Complement Carry Flag
 * Inverts CF.
 *
 * Syntax: CMC
 * Operands: implicit flags register
 * Flags affected: CF is toggled; all other flags are unchanged
 * Notes: Useful in multi-precision arithmetic and bit-serial algorithms.
 *
 * Example:
 *   cmc
 */
static inline void write_cmc(void) {
    write_line(ASM_CMC);
}

/*
 * STD - Set Direction Flag
 * Sets DF so string instructions auto-decrement their pointer registers.
 *
 * Syntax: STD
 * Operands: implicit flags register
 * Flags affected: DF is set; all other flags are unchanged
 * Notes: Low-level code must usually restore DF to the expected ABI state before returning.
 *
 * Example:
 *   std
 */
static inline void write_std(void) {
    write_line(ASM_STD);
}

/*
 * CLD - Clear Direction Flag
 * Clears DF so string instructions auto-increment their pointer registers.
 *
 * Syntax: CLD
 * Operands: implicit flags register
 * Flags affected: DF is cleared; all other flags are unchanged
 * Notes: Most modern runtimes assume DF=0 during normal execution.
 *
 * Example:
 *   cld
 */
static inline void write_cld(void) {
    write_line(ASM_CLD);
}

/*
 * STI - Set Interrupt Flag
 * Sets IF to enable maskable interrupts after the architecturally defined interrupt shadow.
 *
 * Syntax: STI
 * Operands: implicit flags register
 * Flags affected: IF is set subject to privilege checks
 * Notes: Typically privileged or used only in low-level system code.
 *
 * Example:
 *   sti
 */
static inline void write_sti(void) {
    write_line(ASM_STI);
}

/*
 * CLI - Clear Interrupt Flag
 * Clears IF to disable maskable interrupts.
 *
 * Syntax: CLI
 * Operands: implicit flags register
 * Flags affected: IF is cleared subject to privilege checks
 * Notes: Typically privileged or used only in low-level system code.
 *
 * Example:
 *   cli
 */
static inline void write_cli(void) {
    write_line(ASM_CLI);
}

/*
 * LAHF - Load AH from Flags
 * Copies selected status flags into AH.
 *
 * Syntax: LAHF
 * Operands: implicit FLAGS bits -> AH
 * Flags affected: None; AH receives SF:ZF:0:AF:0:PF:1:CF
 * Notes: Useful for compact flag saving in user mode when supported.
 *
 * Example:
 *   lahf
 */
static inline void write_lahf(void) {
    write_line(ASM_LAHF);
}

/*
 * SAHF - Store AH into Flags
 * Loads selected status flags from AH.
 *
 * Syntax: SAHF
 * Operands: implicit AH -> FLAGS bits
 * Flags affected: SF, ZF, AF, PF, and CF are loaded from AH; other flags are unchanged
 * Notes: Useful for restoring a compact saved flag subset.
 *
 * Example:
 *   sahf
 */
static inline void write_sahf(void) {
    write_line(ASM_SAHF);
}

/*
 * SETA - Set Byte if Above / unsigned greater
 * Writes 1 to the destination byte if the condition CF=0 and ZF=0 is true; otherwise writes 0.
 *
 * Syntax: SETA r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   seta al
 */
static inline void write_seta(const char* operand) {
    write_line2(ASM_SETA, operand);
}

/*
 * SETAE - Set Byte if Above or Equal / unsigned >=
 * Writes 1 to the destination byte if the condition CF=0 is true; otherwise writes 0.
 *
 * Syntax: SETAE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setae al
 */
static inline void write_setae(const char* operand) {
    write_line2(ASM_SETAE, operand);
}

/*
 * SETB - Set Byte if Below / unsigned <
 * Writes 1 to the destination byte if the condition CF=1 is true; otherwise writes 0.
 *
 * Syntax: SETB r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setb al
 */
static inline void write_setb(const char* operand) {
    write_line2(ASM_SETB, operand);
}

/*
 * SETBE - Set Byte if Below or Equal / unsigned <=
 * Writes 1 to the destination byte if the condition CF=1 or ZF=1 is true; otherwise writes 0.
 *
 * Syntax: SETBE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setbe al
 */
static inline void write_setbe(const char* operand) {
    write_line2(ASM_SETBE, operand);
}

/*
 * SETC - Set Byte if Carry set
 * Writes 1 to the destination byte if the condition CF=1 is true; otherwise writes 0.
 *
 * Syntax: SETC r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setc al
 */
static inline void write_setc(const char* operand) {
    write_line2(ASM_SETC, operand);
}

/*
 * SETE - Set Byte if Equal / zero
 * Writes 1 to the destination byte if the condition ZF=1 is true; otherwise writes 0.
 *
 * Syntax: SETE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   sete al
 */
static inline void write_sete(const char* operand) {
    write_line2(ASM_SETE, operand);
}

/*
 * SETG - Set Byte if Greater / signed >
 * Writes 1 to the destination byte if the condition ZF=0 and SF=OF is true; otherwise writes 0.
 *
 * Syntax: SETG r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setg al
 */
static inline void write_setg(const char* operand) {
    write_line2(ASM_SETG, operand);
}

/*
 * SETGE - Set Byte if Greater or Equal / signed >=
 * Writes 1 to the destination byte if the condition SF=OF is true; otherwise writes 0.
 *
 * Syntax: SETGE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setge al
 */
static inline void write_setge(const char* operand) {
    write_line2(ASM_SETGE, operand);
}

/*
 * SETL - Set Byte if Less / signed <
 * Writes 1 to the destination byte if the condition SF!=OF is true; otherwise writes 0.
 *
 * Syntax: SETL r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setl al
 */
static inline void write_setl(const char* operand) {
    write_line2(ASM_SETL, operand);
}

/*
 * SETLE - Set Byte if Less or Equal / signed <=
 * Writes 1 to the destination byte if the condition ZF=1 or SF!=OF is true; otherwise writes 0.
 *
 * Syntax: SETLE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setle al
 */
static inline void write_setle(const char* operand) {
    write_line2(ASM_SETLE, operand);
}

/*
 * SETNA - Set Byte if Not Above / unsigned <=
 * Writes 1 to the destination byte if the condition CF=1 or ZF=1 is true; otherwise writes 0.
 *
 * Syntax: SETNA r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setna al
 */
static inline void write_setna(const char* operand) {
    write_line2(ASM_SETNA, operand);
}

/*
 * SETNAE - Set Byte if Not Above or Equal / unsigned <
 * Writes 1 to the destination byte if the condition CF=1 is true; otherwise writes 0.
 *
 * Syntax: SETNAE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnae al
 */
static inline void write_setnae(const char* operand) {
    write_line2(ASM_SETNAE, operand);
}

/*
 * SETNB - Set Byte if Not Below / unsigned >=
 * Writes 1 to the destination byte if the condition CF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNB r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnb al
 */
static inline void write_setnb(const char* operand) {
    write_line2(ASM_SETNB, operand);
}

/*
 * SETNBE - Set Byte if Not Below or Equal / unsigned >
 * Writes 1 to the destination byte if the condition CF=0 and ZF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNBE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnbe al
 */
static inline void write_setnbe(const char* operand) {
    write_line2(ASM_SETNBE, operand);
}

/*
 * SETNC - Set Byte if No carry
 * Writes 1 to the destination byte if the condition CF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNC r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnc al
 */
static inline void write_setnc(const char* operand) {
    write_line2(ASM_SETNC, operand);
}

/*
 * SETNE - Set Byte if Not Equal / nonzero
 * Writes 1 to the destination byte if the condition ZF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setne al
 */
static inline void write_setne(const char* operand) {
    write_line2(ASM_SETNE, operand);
}

/*
 * SETNG - Set Byte if Not Greater / signed <=
 * Writes 1 to the destination byte if the condition ZF=1 or SF!=OF is true; otherwise writes 0.
 *
 * Syntax: SETNG r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setng al
 */
static inline void write_setng(const char* operand) {
    write_line2(ASM_SETNG, operand);
}

/*
 * SETNGE - Set Byte if Not Greater or Equal / signed <
 * Writes 1 to the destination byte if the condition SF!=OF is true; otherwise writes 0.
 *
 * Syntax: SETNGE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnge al
 */
static inline void write_setnge(const char* operand) {
    write_line2(ASM_SETNGE, operand);
}

/*
 * SETNL - Set Byte if Not Less / signed >=
 * Writes 1 to the destination byte if the condition SF=OF is true; otherwise writes 0.
 *
 * Syntax: SETNL r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnl al
 */
static inline void write_setnl(const char* operand) {
    write_line2(ASM_SETNL, operand);
}

/*
 * SETNLE - Set Byte if Not Less or Equal / signed >
 * Writes 1 to the destination byte if the condition ZF=0 and SF=OF is true; otherwise writes 0.
 *
 * Syntax: SETNLE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnle al
 */
static inline void write_setnle(const char* operand) {
    write_line2(ASM_SETNLE, operand);
}

/*
 * SETNO - Set Byte if No overflow
 * Writes 1 to the destination byte if the condition OF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNO r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setno al
 */
static inline void write_setno(const char* operand) {
    write_line2(ASM_SETNO, operand);
}

/*
 * SETNP - Set Byte if Not parity / parity odd
 * Writes 1 to the destination byte if the condition PF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNP r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnp al
 */
static inline void write_setnp(const char* operand) {
    write_line2(ASM_SETNP, operand);
}

/*
 * SETNS - Set Byte if Not sign
 * Writes 1 to the destination byte if the condition SF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNS r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setns al
 */
static inline void write_setns(const char* operand) {
    write_line2(ASM_SETNS, operand);
}

/*
 * SETNZ - Set Byte if Not zero
 * Writes 1 to the destination byte if the condition ZF=0 is true; otherwise writes 0.
 *
 * Syntax: SETNZ r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setnz al
 */
static inline void write_setnz(const char* operand) {
    write_line2(ASM_SETNZ, operand);
}

/*
 * SETO - Set Byte if Overflow
 * Writes 1 to the destination byte if the condition OF=1 is true; otherwise writes 0.
 *
 * Syntax: SETO r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   seto al
 */
static inline void write_seto(const char* operand) {
    write_line2(ASM_SETO, operand);
}

/*
 * SETP - Set Byte if Parity / parity even
 * Writes 1 to the destination byte if the condition PF=1 is true; otherwise writes 0.
 *
 * Syntax: SETP r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setp al
 */
static inline void write_setp(const char* operand) {
    write_line2(ASM_SETP, operand);
}

/*
 * SETPE - Set Byte if Parity even
 * Writes 1 to the destination byte if the condition PF=1 is true; otherwise writes 0.
 *
 * Syntax: SETPE r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setpe al
 */
static inline void write_setpe(const char* operand) {
    write_line2(ASM_SETPE, operand);
}

/*
 * SETPO - Set Byte if Parity odd
 * Writes 1 to the destination byte if the condition PF=0 is true; otherwise writes 0.
 *
 * Syntax: SETPO r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setpo al
 */
static inline void write_setpo(const char* operand) {
    write_line2(ASM_SETPO, operand);
}

/*
 * SETS - Set Byte if Sign
 * Writes 1 to the destination byte if the condition SF=1 is true; otherwise writes 0.
 *
 * Syntax: SETS r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   sets al
 */
static inline void write_sets(const char* operand) {
    write_line2(ASM_SETS, operand);
}

/*
 * SETZ - Set Byte if Zero
 * Writes 1 to the destination byte if the condition ZF=1 is true; otherwise writes 0.
 *
 * Syntax: SETZ r/m8
 * Operands: 8-bit register or 8-bit memory destination
 * Flags affected: None
 * Notes: SETcc reads the current flags but does not alter them. The destination is always
 * exactly one byte wide.
 *
 * Example:
 *   cmp eax, ebx
 *   setz al
 */
static inline void write_setz(const char* operand) {
    write_line2(ASM_SETZ, operand);
}

/*
 * ============================================================================
 * 14. Stack Frame
 * ============================================================================
 */

/*
 * ENTER - Build Stack Frame
 * Creates a procedure stack frame by pushing the old frame pointer, establishing a new frame
 * pointer, and optionally allocating stack space and nested frame links.
 *
 * Syntax: ENTER imm16, imm8
 * Operands: frame size immediate and nesting-level immediate
 * Flags affected: None
 * Notes: ENTER is semantically rich but slower and less common than the manual PUSH/MOV/SUB
 * prologue sequence in modern code.
 *
 * Example:
 *   enter 32, 0
 */
static inline void write_enter(const char* dest, const char* src) {
    write_line3(ASM_ENTER, dest, src);
}

/*
 * LEAVE - Destroy Stack Frame
 * Restores the previous frame pointer and stack pointer, effectively reversing a standard ENTER
 * or PUSH/MOV frame setup.
 *
 * Syntax: LEAVE
 * Operands: implicit RBP/EBP/BP and stack pointer
 * Flags affected: None
 * Notes: LEAVE is equivalent to MOV SP, BP followed by POP BP in the active operand size.
 *
 * Example:
 *   leave
 */
static inline void write_leave(void) {
    write_line(ASM_LEAVE);
}

/*
 * ============================================================================
 * 15. I/O Instructions
 * ============================================================================
 */

/*
 * IN - Input from Port
 * Reads data from an I/O port into AL, AX, or EAX.
 *
 * Syntax: IN accumulator, imm8|DX
 * Operands: implicit accumulator and immediate port or DX port register
 * Flags affected: None
 * Notes: Port I/O is privileged or platform-specific on many systems.
 *
 * Example:
 *   in al, dx
 */
static inline void write_in(const char* dest, const char* src) {
    write_line3(ASM_IN, dest, src);
}

/*
 * OUT - Output to Port
 * Writes AL, AX, or EAX to an I/O port.
 *
 * Syntax: OUT imm8|DX, accumulator
 * Operands: implicit accumulator and immediate port or DX port register
 * Flags affected: None
 * Notes: Port I/O is privileged or platform-specific on many systems.
 *
 * Example:
 *   out dx, al
 */
static inline void write_out(const char* dest, const char* src) {
    write_line3(ASM_OUT, dest, src);
}

/*
 * INS - Input String from Port
 * Reads from the port in DX to the memory location at ES:(E|R)DI and updates the pointer
 * according to DF.
 *
 * Syntax: INS
 * Operands: implicit DX port and ES:(E|R)DI destination
 * Flags affected: None
 * Notes: String-port input may be repeated with REP.
 *
 * Example:
 *   rep insd
 */
static inline void write_ins(const char* dest, const char* src) {
    write_line3(ASM_INS, dest, src);
}

/*
 * INSB - Input Byte String from Port
 * Reads one byte from the DX port into ES:(E|R)DI.
 *
 * Syntax: INSB
 * Operands: implicit DX port and destination pointer
 * Flags affected: None
 * Notes: Often used with REP for device buffers.
 *
 * Example:
 *   insb
 */
static inline void write_insb(void) {
    write_line(ASM_INSB);
}

/*
 * INSW - Input Word String from Port
 * Reads one word from the DX port into ES:(E|R)DI.
 *
 * Syntax: INSW
 * Operands: implicit DX port and destination pointer
 * Flags affected: None
 * Notes: Pointer movement is 2 bytes per iteration.
 *
 * Example:
 *   insw
 */
static inline void write_insw(void) {
    write_line(ASM_INSW);
}

/*
 * INSD - Input Doubleword String from Port
 * Reads one dword from the DX port into ES:(E|R)DI.
 *
 * Syntax: INSD
 * Operands: implicit DX port and destination pointer
 * Flags affected: None
 * Notes: Pointer movement is 4 bytes per iteration.
 *
 * Example:
 *   insd
 */
static inline void write_insd(void) {
    write_line(ASM_INSD);
}

/*
 * OUTS - Output String to Port
 * Writes the memory element at DS:(E|R)SI to the port in DX and updates the source pointer
 * according to DF.
 *
 * Syntax: OUTS
 * Operands: implicit source pointer and DX port
 * Flags affected: None
 * Notes: May be repeated with REP for block output.
 *
 * Example:
 *   rep outsw
 */
static inline void write_outs(const char* dest, const char* src) {
    write_line3(ASM_OUTS, dest, src);
}

/*
 * OUTSB - Output Byte String to Port
 * Writes one byte from DS:(E|R)SI to the DX port.
 *
 * Syntax: OUTSB
 * Operands: implicit source pointer and DX port
 * Flags affected: None
 * Notes: Pointer movement is 1 byte per iteration.
 *
 * Example:
 *   outsb
 */
static inline void write_outsb(void) {
    write_line(ASM_OUTSB);
}

/*
 * OUTSW - Output Word String to Port
 * Writes one word from DS:(E|R)SI to the DX port.
 *
 * Syntax: OUTSW
 * Operands: implicit source pointer and DX port
 * Flags affected: None
 * Notes: Pointer movement is 2 bytes per iteration.
 *
 * Example:
 *   outsw
 */
static inline void write_outsw(void) {
    write_line(ASM_OUTSW);
}

/*
 * OUTSD - Output Doubleword String to Port
 * Writes one dword from DS:(E|R)SI to the DX port.
 *
 * Syntax: OUTSD
 * Operands: implicit source pointer and DX port
 * Flags affected: None
 * Notes: Pointer movement is 4 bytes per iteration.
 *
 * Example:
 *   outsd
 */
static inline void write_outsd(void) {
    write_line(ASM_OUTSD);
}

/*
 * ============================================================================
 * 16. System Instructions
 * ============================================================================
 */

/*
 * SYSCALL - Fast System Call
 * Enters the operating system using the 64-bit fast system-call mechanism.
 *
 * Syntax: SYSCALL
 * Operands: implicit model-specific registers and calling-convention registers
 * Flags affected: RFLAGS is masked by IA32_FMASK; RCX receives the return RIP and R11 receives
 * the original RFLAGS image
 * Notes: Available only when enabled by the OS and CPU. Register conventions are ABI-specific.
 *
 * Example:
 *   syscall
 */
static inline void write_syscall(void) {
    write_line(ASM_SYSCALL);
}

/*
 * SYSRET - Return from Fast System Call
 * Returns from a SYSCALL transition back to user mode using state prepared by the kernel.
 *
 * Syntax: SYSRET
 * Operands: implicit RCX return address and R11 flags image
 * Flags affected: Restores selected flags and transfers control
 * Notes: Typically privileged and tightly coupled to the operating system's syscall ABI.
 *
 * Example:
 *   sysret
 */
static inline void write_sysret(void) {
    write_line(ASM_SYSRET);
}

/*
 * SYSENTER - Fast System Enter
 * Performs a fast transition to system mode using SYSENTER MSR configuration.
 *
 * Syntax: SYSENTER
 * Operands: implicit CS/EIP/ESP-style machine state via MSRs
 * Flags affected: None architecturally visible in the usual programming model
 * Notes: Common in older 32-bit kernels; ABI conventions are OS-defined.
 *
 * Example:
 *   sysenter
 */
static inline void write_sysenter(void) {
    write_line(ASM_SYSENTER);
}

/*
 * SYSEXIT - Fast System Exit
 * Returns from a SYSENTER-style system call to less-privileged code.
 *
 * Syntax: SYSEXIT
 * Operands: implicit return state from registers and MSRs
 * Flags affected: None architecturally visible in the usual programming model
 * Notes: Primarily used by kernels that implement the SYSENTER/SYSEXIT ABI.
 *
 * Example:
 *   sysexit
 */
static inline void write_sysexit(void) {
    write_line(ASM_SYSEXIT);
}

/*
 * INT - Software Interrupt
 * Invokes an interrupt or trap handler through the IDT using the supplied vector number.
 *
 * Syntax: INT imm8
 * Operands: 8-bit interrupt vector immediate
 * Flags affected: None directly; the processor pushes flags and state as part of exception
 * delivery
 * Notes: Used for BIOS, DOS, debugging, or software interrupt APIs in legacy environments.
 *
 * Example:
 *   int
 */
static inline void write_int(const char* operand) {
    write_line2(ASM_INT, operand);
}

/*
 * INT3 - Breakpoint Interrupt
 * Generates breakpoint interrupt vector 3 with a dedicated one-byte encoding favored by
 * debuggers.
 *
 * Syntax: INT3
 * Operands: implicit breakpoint vector
 * Flags affected: None directly
 * Notes: Commonly patched into code for breakpoints.
 *
 * Example:
 *   int3
 */
static inline void write_int3(void) {
    write_line(ASM_INT3);
}

/*
 * INTO - Interrupt on Overflow
 * Triggers interrupt vector 4 if OF is set.
 *
 * Syntax: INTO
 * Operands: implicit OF test
 * Flags affected: None
 * Notes: Legacy instruction not available in 64-bit mode.
 *
 * Example:
 *   into
 */
static inline void write_into(void) {
    write_line(ASM_INTO);
}

/*
 * IRET - Interrupt Return
 * Returns from an interrupt or exception handler by restoring IP/EIP/RIP, CS, and FLAGS state
 * from the stack.
 *
 * Syntax: IRET
 * Operands: implicit interrupt frame on the stack
 * Flags affected: Restores flags from the interrupt frame
 * Notes: Used in 16-bit and size-dependent contexts; IRETD and IRETQ make width explicit.
 *
 * Example:
 *   iret
 */
static inline void write_iret(void) {
    write_line(ASM_IRET);
}

/*
 * IRETD - Interrupt Return Doubleword
 * Returns from a 32-bit interrupt frame.
 *
 * Syntax: IRETD
 * Operands: implicit 32-bit interrupt frame
 * Flags affected: Restores EFLAGS from the stack frame
 * Notes: Used by 32-bit handlers and system code.
 *
 * Example:
 *   iretd
 */
static inline void write_iretd(void) {
    write_line(ASM_IRETD);
}

/*
 * IRETQ - Interrupt Return Quadword
 * Returns from a 64-bit interrupt or exception frame.
 *
 * Syntax: IRETQ
 * Operands: implicit 64-bit interrupt frame
 * Flags affected: Restores RFLAGS from the stack frame
 * Notes: Used by 64-bit kernels, hypervisors, and low-level handlers.
 *
 * Example:
 *   iretq
 */
static inline void write_iretq(void) {
    write_line(ASM_IRETQ);
}

/*
 * HLT - Halt Processor
 * Stops instruction execution until an interrupt, NMI, SMI, INIT, or reset event occurs.
 *
 * Syntax: HLT
 * Operands: implicit processor state
 * Flags affected: None
 * Notes: Privileged on most systems and typically used only by kernels or firmware idle loops.
 *
 * Example:
 *   hlt
 */
static inline void write_hlt(void) {
    write_line(ASM_HLT);
}

/*
 * NOP - No Operation
 * Consumes instruction bytes and execution resources without changing the architectural state
 * used by general code.
 *
 * Syntax: NOP
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Used for alignment, patch space, timing, and placeholder instructions.
 *
 * Example:
 *   nop
 */
static inline void write_nop(void) {
    write_line(ASM_NOP);
}

/*
 * PAUSE - Spin-Wait Hint
 * Provides a hint to the processor that the code is in a spin-wait loop.
 *
 * Syntax: PAUSE
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Improves power and performance behavior in busy-wait loops and reduces contention
 * penalties.
 *
 * Example:
 *   pause
 */
static inline void write_pause(void) {
    write_line(ASM_PAUSE);
}

/*
 * UD2 - Undefined Instruction Trap
 * Intentionally triggers an invalid-opcode exception.
 *
 * Syntax: UD2
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Useful for guaranteed traps, unreachable markers, and sanitizer/debug instrumentation.
 *
 * Example:
 *   ud2
 */
static inline void write_ud2(void) {
    write_line(ASM_UD2);
}

/*
 * CPUID - Query CPU Identification
 * Returns processor identification and feature information based on the requested leaf values in
 * EAX/ECX.
 *
 * Syntax: CPUID
 * Operands: implicit EAX/ECX inputs and EAX/EBX/ECX/EDX outputs
 * Flags affected: None
 * Notes: Serializing instruction often used for feature detection and vendor strings.
 *
 * Example:
 *   cpuid
 */
static inline void write_cpuid(void) {
    write_line(ASM_CPUID);
}

/*
 * RDTSC - Read Time-Stamp Counter
 * Reads the time-stamp counter into EDX:EAX or RDX:RAX.
 *
 * Syntax: RDTSC
 * Operands: implicit counter destination registers
 * Flags affected: None
 * Notes: Not fully serializing by itself; LFENCE or CPUID may be used around it depending on the
 * required ordering.
 *
 * Example:
 *   rdtsc
 */
static inline void write_rdtsc(void) {
    write_line(ASM_RDTSC);
}

/* Read the time-stamp counter into a GPR as one 64-bit value.
 * rdtsc writes the low half in eax and the high half in edx. This shifts
 * edx up and ors it into rax. dest may be rax, in which case rax already
 * holds the counter. Any other dest is a qword GPR; rax and rdx are
 * clobbered either way. The counter is not serialising. */
static inline void asm_read_tsc(const char* dest) {
    write_rdtsc();
    write_shl("rdx", "32");
    write_or("rax", "rdx");
    if (dest != NULL && strcmp(dest, "rax") != 0)
        write_replace_mov(dest, (AsmDataField){ "rax", "tsc", NULL });
}

/*
 * RDTSCP - Read Time-Stamp Counter and Processor ID
 * Reads the time-stamp counter and auxiliary processor ID in a partially serializing manner.
 *
 * Syntax: RDTSCP
 * Operands: implicit EDX:EAX/RDX:RAX result and ECX auxiliary output
 * Flags affected: None
 * Notes: Useful for higher-quality timing and core identification than bare RDTSC.
 *
 * Example:
 *   rdtscp
 */
static inline void write_rdtscp(void) {
    write_line(ASM_RDTSCP);
}

/*
 * RDMSR - Read Model-Specific Register
 * Reads the MSR selected by ECX into EDX:EAX.
 *
 * Syntax: RDMSR
 * Operands: implicit ECX selector and EDX:EAX result
 * Flags affected: None
 * Notes: Privileged instruction used by kernels and hypervisors.
 *
 * Example:
 *   rdmsr
 */
static inline void write_rdmsr(void) {
    write_line(ASM_RDMSR);
}

/*
 * WRMSR - Write Model-Specific Register
 * Writes EDX:EAX to the MSR selected by ECX.
 *
 * Syntax: WRMSR
 * Operands: implicit ECX selector and EDX:EAX value
 * Flags affected: None
 * Notes: Privileged and potentially dangerous because MSRs control key processor behavior.
 *
 * Example:
 *   wrmsr
 */
static inline void write_wrmsr(void) {
    write_line(ASM_WRMSR);
}

/*
 * RDPMC - Read Performance-Monitor Counter
 * Reads a performance-monitoring counter selected by ECX.
 *
 * Syntax: RDPMC
 * Operands: implicit ECX selector and EDX:EAX result
 * Flags affected: None
 * Notes: Availability in user mode depends on CR4.PCE and platform policy.
 *
 * Example:
 *   rdpmc
 */
static inline void write_rdpmc(void) {
    write_line(ASM_RDPMC);
}

/*
 * XGETBV - Read Extended Control Register
 * Reads XCR state selected by ECX into EDX:EAX.
 *
 * Syntax: XGETBV
 * Operands: implicit ECX selector and EDX:EAX result
 * Flags affected: None
 * Notes: Used to query OS-enabled extended state such as SSE and AVX support.
 *
 * Example:
 *   xgetbv
 */
static inline void write_xgetbv(void) {
    write_line(ASM_XGETBV);
}

/*
 * XSETBV - Write Extended Control Register
 * Writes EDX:EAX into the XCR selected by ECX.
 *
 * Syntax: XSETBV
 * Operands: implicit ECX selector and EDX:EAX value
 * Flags affected: None
 * Notes: Privileged or otherwise restricted; used by kernels to enable XSAVE-managed state.
 *
 * Example:
 *   xsetbv
 */
static inline void write_xsetbv(void) {
    write_line(ASM_XSETBV);
}

/*
 * INVD - Invalidate Caches without Writeback
 * Invalidates internal caches without first writing back modified lines.
 *
 * Syntax: INVD
 * Operands: implicit cache hierarchy state
 * Flags affected: None
 * Notes: Privileged and typically used only in firmware or extreme low-level diagnostics.
 *
 * Example:
 *   invd
 */
static inline void write_invd(void) {
    write_line(ASM_INVD);
}

/*
 * WBINVD - Write Back and Invalidate Cache
 * Writes back modified cache lines and invalidates internal caches.
 *
 * Syntax: WBINVD
 * Operands: implicit cache hierarchy state
 * Flags affected: None
 * Notes: Privileged and used for cache management in firmware, kernels, or platform code.
 *
 * Example:
 *   wbinvd
 */
static inline void write_wbinvd(void) {
    write_line(ASM_WBINVD);
}

/*
 * INVLPG - Invalidate TLB Entry
 * Invalidates the translation lookaside buffer entry for the specified linear address.
 *
 * Syntax: INVLPG mem
 * Operands: memory operand supplying the linear address
 * Flags affected: None
 * Notes: Privileged instruction commonly used after page-table updates.
 *
 * Example:
 *   invlpg
 */
static inline void write_invlpg(const char* operand) {
    write_line2(ASM_INVLPG, operand);
}

/*
 * CLFLUSH - Cache Line Flush
 * Writes back and invalidates the cache line containing the specified memory operand from all
 * cache levels coherent with the current core.
 *
 * Syntax: CLFLUSH mem
 * Operands: memory operand naming the cache line
 * Flags affected: None
 * Notes: Used in persistence, cache-control, and some side-channel research code.
 *
 * Example:
 *   clflush
 */
static inline void write_clflush(const char* operand) {
    write_line2(ASM_CLFLUSH, operand);
}

/*
 * CLFLUSHOPT - Optimized Cache Line Flush
 * Like CLFLUSH but permits more implementation flexibility and batching.
 *
 * Syntax: CLFLUSHOPT mem
 * Operands: memory operand naming the cache line
 * Flags affected: None
 * Notes: Often paired with SFENCE for ordering when used for persistent memory.
 *
 * Example:
 *   clflushopt
 */
static inline void write_clflushopt(const char* operand) {
    write_line2(ASM_CLFLUSHOPT, operand);
}

/*
 * LFENCE - Load Fence
 * Prevents later loads from being reordered before earlier loads.
 *
 * Syntax: LFENCE
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Used for memory ordering and as a lightweight serialization primitive around timing
 * code on some CPUs.
 *
 * Example:
 *   lfence
 */
static inline void write_lfence(void) {
    write_line(ASM_LFENCE);
}

/*
 * SFENCE - Store Fence
 * Prevents later stores from being reordered before earlier stores.
 *
 * Syntax: SFENCE
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Important for non-temporal stores and persistence ordering.
 *
 * Example:
 *   sfence
 */
static inline void write_sfence(void) {
    write_line(ASM_SFENCE);
}

/*
 * MFENCE - Memory Fence
 * Prevents later memory operations from being reordered before earlier ones.
 *
 * Syntax: MFENCE
 * Operands: no explicit operands
 * Flags affected: None
 * Notes: Full memory-order fence for load/store ordering.
 *
 * Example:
 *   mfence
 */
static inline void write_mfence(void) {
    write_line(ASM_MFENCE);
}

/*
 * PREFETCH - Prefetch Data
 * Hints that the addressed memory should be fetched into cache before it is needed.
 *
 * Syntax: PREFETCH mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Assemblers often map this generic spelling to a locality-specific hint if supported.
 *
 * Example:
 *   prefetch
 */
static inline void write_prefetch(const char* operand) {
    write_line2(ASM_PREFETCH, operand);
}

/*
 * PREFETCHW - Prefetch for Write
 * Hints that the addressed memory will be written soon.
 *
 * Syntax: PREFETCHW mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Often used before lock-free or write-heavy data structure updates.
 *
 * Example:
 *   prefetchw
 */
static inline void write_prefetchw(const char* operand) {
    write_line2(ASM_PREFETCHW, operand);
}

/*
 * PREFETCHNTA - Non-Temporal Prefetch
 * Prefetches data with a non-temporal locality hint.
 *
 * Syntax: PREFETCHNTA mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Useful for streaming accesses that should not heavily pollute caches.
 *
 * Example:
 *   prefetchnta
 */
static inline void write_prefetchnta(const char* operand) {
    write_line2(ASM_PREFETCHNTA, operand);
}

/*
 * PREFETCHT0 - Temporal Prefetch T0
 * Prefetches data with high temporal locality, favoring close cache levels.
 *
 * Syntax: PREFETCHT0 mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Appropriate for data expected to be reused soon.
 *
 * Example:
 *   prefetcht0
 */
static inline void write_prefetcht0(const char* operand) {
    write_line2(ASM_PREFETCHT0, operand);
}

/*
 * PREFETCHT1 - Temporal Prefetch T1
 * Prefetches data with a moderate temporal-locality hint.
 *
 * Syntax: PREFETCHT1 mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Hint behavior is microarchitecture dependent.
 *
 * Example:
 *   prefetcht1
 */
static inline void write_prefetcht1(const char* operand) {
    write_line2(ASM_PREFETCHT1, operand);
}

/*
 * PREFETCHT2 - Temporal Prefetch T2
 * Prefetches data with a more distant temporal-locality hint.
 *
 * Syntax: PREFETCHT2 mem
 * Operands: memory operand
 * Flags affected: None
 * Notes: Hint behavior is microarchitecture dependent.
 *
 * Example:
 *   prefetcht2
 */
static inline void write_prefetcht2(const char* operand) {
    write_line2(ASM_PREFETCHT2, operand);
}

/*
 * CLTS - Clear Task-Switched Flag
 * Clears CR0.TS so x87/SSE instructions can execute without a device-not-available fault.
 *
 * Syntax: CLTS
 * Operands: implicit CR0 state
 * Flags affected: None
 * Notes: Privileged and used by kernels that lazily manage floating-point state.
 *
 * Example:
 *   clts
 */
static inline void write_clts(void) {
    write_line(ASM_CLTS);
}

/*
 * LGDT - Load Global Descriptor Table Register
 * Loads the GDTR from a memory operand.
 *
 * Syntax: LGDT mem
 * Operands: memory operand containing a descriptor-table pseudo-descriptor
 * Flags affected: None
 * Notes: Privileged setup instruction for protected mode and virtualization environments.
 *
 * Example:
 *   lgdt
 */
static inline void write_lgdt(const char* operand) {
    write_line2(ASM_LGDT, operand);
}

/*
 * SGDT - Store Global Descriptor Table Register
 * Stores the current GDTR to memory.
 *
 * Syntax: SGDT mem
 * Operands: memory destination operand
 * Flags affected: None
 * Notes: Used in low-level diagnostics, hypervisors, and system software.
 *
 * Example:
 *   sgdt
 */
static inline void write_sgdt(const char* operand) {
    write_line2(ASM_SGDT, operand);
}

/*
 * LIDT - Load Interrupt Descriptor Table Register
 * Loads the IDTR from a memory operand.
 *
 * Syntax: LIDT mem
 * Operands: memory operand containing an IDT pseudo-descriptor
 * Flags affected: None
 * Notes: Privileged setup instruction for interrupt dispatch.
 *
 * Example:
 *   lidt
 */
static inline void write_lidt(const char* operand) {
    write_line2(ASM_LIDT, operand);
}

/*
 * SIDT - Store Interrupt Descriptor Table Register
 * Stores the current IDTR to memory.
 *
 * Syntax: SIDT mem
 * Operands: memory destination operand
 * Flags affected: None
 * Notes: Used for diagnostics and virtualization-related code.
 *
 * Example:
 *   sidt
 */
static inline void write_sidt(const char* operand) {
    write_line2(ASM_SIDT, operand);
}

/*
 * LLDT - Load Local Descriptor Table Register
 * Loads the LDTR selector from a register or memory operand.
 *
 * Syntax: LLDT r/m16
 * Operands: 16-bit register or memory selector
 * Flags affected: None
 * Notes: Privileged instruction for segmented protected-mode task environments.
 *
 * Example:
 *   lldt
 */
static inline void write_lldt(const char* operand) {
    write_line2(ASM_LLDT, operand);
}

/*
 * SLDT - Store Local Descriptor Table Register
 * Stores the LDTR selector into a register or memory operand.
 *
 * Syntax: SLDT r/m16
 * Operands: 16-bit register or memory destination
 * Flags affected: None
 * Notes: Mostly relevant to system software or emulators.
 *
 * Example:
 *   sldt
 */
static inline void write_sldt(const char* operand) {
    write_line2(ASM_SLDT, operand);
}

/*
 * LTR - Load Task Register
 * Loads the task register with a TSS selector.
 *
 * Syntax: LTR r/m16
 * Operands: 16-bit register or memory selector
 * Flags affected: None
 * Notes: Privileged protected-mode setup instruction.
 *
 * Example:
 *   ltr
 */
static inline void write_ltr(const char* operand) {
    write_line2(ASM_LTR, operand);
}

/*
 * STR - Store Task Register
 * Stores the task-register selector to a register or memory operand.
 *
 * Syntax: STR r/m16
 * Operands: 16-bit register or memory destination
 * Flags affected: None
 * Notes: Mostly relevant to low-level system software.
 *
 * Example:
 *   str
 */
static inline void write_str(const char* operand) {
    write_line2(ASM_STR, operand);
}

/*
 * LMSW - Load Machine Status Word
 * Loads selected low CR0 bits from the source operand.
 *
 * Syntax: LMSW r/m16
 * Operands: 16-bit register or memory source
 * Flags affected: None
 * Notes: Historic protected-mode enabling instruction with limited CR0 coverage.
 *
 * Example:
 *   lmsw
 */
static inline void write_lmsw(const char* operand) {
    write_line2(ASM_LMSW, operand);
}

/*
 * SMSW - Store Machine Status Word
 * Stores the machine-status word derived from CR0 into the destination operand.
 *
 * Syntax: SMSW r/m16|r32|r64
 * Operands: register or memory destination
 * Flags affected: None
 * Notes: Useful mainly to low-level software investigating control state.
 *
 * Example:
 *   smsw
 */
static inline void write_smsw(const char* operand) {
    write_line2(ASM_SMSW, operand);
}

/*
 * SWAPGS - Swap Kernel and User GS Base
 * Exchanges the current GS base with the kernel GS-base MSR.
 *
 * Syntax: SWAPGS
 * Operands: implicit GS base state
 * Flags affected: None
 * Notes: 64-bit privileged instruction commonly used on kernel entry/exit paths.
 *
 * Example:
 *   swapgs
 */
static inline void write_swapgs(void) {
    write_line(ASM_SWAPGS);
}

/*
 * XSAVE - Save Extended Processor State
 * Stores x87, SSE, AVX, and other enabled extended state components to an XSAVE area according
 * to XCR0 and EDX:EAX masks.
 *
 * Syntax: XSAVE mem
 * Operands: memory destination and implicit EDX:EAX component mask
 * Flags affected: May update modified/compacted tracking state; does not use normal arithmetic
 * flags
 * Notes: Requires OS-managed XSAVE support and a correctly aligned save area.
 *
 * Example:
 *   xsave
 */
static inline void write_xsave(const char* operand) {
    write_line2(ASM_XSAVE, operand);
}

/*
 * XRSTOR - Restore Extended Processor State
 * Restores enabled processor state components from an XSAVE area.
 *
 * Syntax: XRSTOR mem
 * Operands: memory source and implicit EDX:EAX component mask
 * Flags affected: May update internal state-tracking metadata; no ordinary arithmetic flags
 * semantics
 * Notes: Used with XSAVE-family save areas.
 *
 * Example:
 *   xrstor
 */
static inline void write_xrstor(const char* operand) {
    write_line2(ASM_XRSTOR, operand);
}

/*
 * XSAVEOPT - Optimized Save Extended State
 * Like XSAVE but may skip components that are already in memory and unchanged.
 *
 * Syntax: XSAVEOPT mem
 * Operands: memory destination and implicit EDX:EAX component mask
 * Flags affected: No ordinary arithmetic flags semantics
 * Notes: Used when CPU support allows more efficient save behavior than XSAVE.
 *
 * Example:
 *   xsaveopt
 */
static inline void write_xsaveopt(const char* operand) {
    write_line2(ASM_XSAVEOPT, operand);
}

/*
 * ============================================================================
 * 17. CRC and AES Instructions
 * ============================================================================
 */

/*
 * CRC32 - CRC-32 Accumulate
 * Computes a CRC-32C update from the destination accumulator and the source operand.
 *
 * Syntax: CRC32 reg, r/m8|r/m16|r/m32|r/m64
 * Operands: destination register accumulator and source register or memory operand
 * Flags affected: OF, SF, ZF, AF, CF, and PF are undefined
 * Notes: Used for fast checksums with the SSE4.2 CRC polynomial.
 *
 * Example:
 *   crc32 eax, byte ptr [rsi]
 */
static inline void write_crc32(const char* dest, const char* src) {
    write_line3(ASM_CRC32, dest, src);
}

/*
 * AESENC - AES Round Encrypt
 * Performs one standard AES encryption round on the state using the round key operand.
 *
 * Syntax: AESENC xmm1, xmm2/m128
 * Operands: destination/source XMM state and round-key operand
 * Flags affected: None
 * Notes: AES-NI instruction operating on 128-bit AES states.
 *
 * Example:
 *   aesenc xmm0, xmm1
 */
static inline void write_aesenc(const char* dest, const char* src) {
    write_line3(ASM_AESENC, dest, src);
}

/*
 * AESENCLAST - AES Final Encrypt Round
 * Performs the final AES encryption round without MixColumns.
 *
 * Syntax: AESENCLAST xmm1, xmm2/m128
 * Operands: destination/source XMM state and round-key operand
 * Flags affected: None
 * Notes: Use after the regular AESENC rounds.
 *
 * Example:
 *   aesenclast xmm0, xmm1
 */
static inline void write_aesenclast(const char* dest, const char* src) {
    write_line3(ASM_AESENCLAST, dest, src);
}

/*
 * AESDEC - AES Round Decrypt
 * Performs one standard AES decryption round on the state using the round key operand.
 *
 * Syntax: AESDEC xmm1, xmm2/m128
 * Operands: destination/source XMM state and round-key operand
 * Flags affected: None
 * Notes: AES-NI instruction for decryption rounds.
 *
 * Example:
 *   aesdec xmm0, xmm1
 */
static inline void write_aesdec(const char* dest, const char* src) {
    write_line3(ASM_AESDEC, dest, src);
}

/*
 * AESDECLAST - AES Final Decrypt Round
 * Performs the final AES decryption round without InvMixColumns.
 *
 * Syntax: AESDECLAST xmm1, xmm2/m128
 * Operands: destination/source XMM state and round-key operand
 * Flags affected: None
 * Notes: Use after the regular AESDEC rounds.
 *
 * Example:
 *   aesdeclast xmm0, xmm1
 */
static inline void write_aesdeclast(const char* dest, const char* src) {
    write_line3(ASM_AESDECLAST, dest, src);
}

/*
 * AESIMC - AES Inverse MixColumns
 * Applies the inverse MixColumns transform used to prepare decryption round keys.
 *
 * Syntax: AESIMC xmm1, xmm2/m128
 * Operands: destination XMM register and source XMM/memory operand
 * Flags affected: None
 * Notes: Common in AES key-schedule setup for decryption.
 *
 * Example:
 *   aesimc xmm2, xmm1
 */
static inline void write_aesimc(const char* dest, const char* src) {
    write_line3(ASM_AESIMC, dest, src);
}

/*
 * AESKEYGENASSIST - AES Key Generation Assist
 * Generates round-key material to assist software AES key expansion.
 *
 * Syntax: AESKEYGENASSIST xmm1, xmm2/m128, imm8
 * Operands: destination XMM register, source XMM/memory operand, and round constant immediate
 * Flags affected: None
 * Notes: Used with shifts, shuffles, and XORs to build round keys.
 *
 * Example:
 *   aeskeygenassist xmm1, xmm0, 1
 */
static inline void write_aeskeygenassist(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_AESKEYGENASSIST, dest, src, imm);
}

/*
 * PCLMULQDQ - Carry-Less Multiply
 * Performs polynomial carry-less multiplication on selected 64-bit lanes from two 128-bit
 * operands.
 *
 * Syntax: PCLMULQDQ xmm1, xmm2/m128, imm8
 * Operands: destination/source XMM register, source XMM/memory operand, and lane-select
 * immediate
 * Flags affected: None
 * Notes: Useful for GCM, CRC, and other binary-field algorithms.
 *
 * Example:
 *   pclmulqdq xmm0, xmm1, 0
 */
static inline void write_pclmulqdq(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_PCLMULQDQ, dest, src, imm);
}

/*
 * ============================================================================
 * 18. FPU Instructions
 * ============================================================================
 */

/*
 * FLD - Load Floating-Point Value
 * Pushes a floating-point value onto the x87 register stack.
 *
 * Syntax: FLD m32fp|m64fp|m80fp|ST(i)
 * Operands: memory source or another x87 register
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: The stack top is decremented and the new value becomes ST(0).
 *
 * Example:
 *   fld qword ptr [value]
 */
static inline void write_fld(const char* operand) {
    write_line2(ASM_FLD, operand);
}

/*
 * FST - Store Floating-Point Value
 * Stores ST(0) to the destination without popping the x87 stack.
 *
 * Syntax: FST m32fp|m64fp|ST(i)
 * Operands: memory or x87 register destination
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Useful when the top-of-stack value must remain available for later x87 operations.
 *
 * Example:
 *   fst qword ptr [result]
 */
static inline void write_fst(const char* operand) {
    write_line2(ASM_FST, operand);
}

/*
 * FSTP - Store Floating-Point Value and Pop
 * Stores ST(0) to the destination and then pops the x87 stack.
 *
 * Syntax: FSTP m32fp|m64fp|m80fp|ST(i)
 * Operands: memory or x87 register destination
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: After the store, the x87 stack pointer advances and the next register becomes ST(0).
 *
 * Example:
 *   fstp qword ptr [result]
 */
static inline void write_fstp(const char* operand) {
    write_line2(ASM_FSTP, operand);
}

/*
 * FILD - Load Integer to x87
 * Pushes a signed integer from memory onto the x87 stack after converting it to extended
 * precision.
 *
 * Syntax: FILD m16int|m32int|m64int
 * Operands: integer memory source
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Useful for integer-to-floating conversion into x87 code paths.
 *
 * Example:
 *   fild dword ptr [count]
 */
static inline void write_fild(const char* operand) {
    write_line2(ASM_FILD, operand);
}

/*
 * FIST - Store Integer from x87
 * Converts ST(0) to an integer and stores it without popping the x87 stack.
 *
 * Syntax: FIST m16int|m32int
 * Operands: integer memory destination
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Rounding mode comes from the x87 control word.
 *
 * Example:
 *   fist dword ptr [count]
 */
static inline void write_fist(const char* operand) {
    write_line2(ASM_FIST, operand);
}

/*
 * FISTP - Store Integer from x87 and Pop
 * Converts ST(0) to an integer, stores it, and pops the x87 stack.
 *
 * Syntax: FISTP m16int|m32int|m64int
 * Operands: integer memory destination
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Use when the converted value is final and the stack element should be consumed.
 *
 * Example:
 *   fistp qword ptr [count]
 */
static inline void write_fistp(const char* operand) {
    write_line2(ASM_FISTP, operand);
}

/*
 * FXCH - Exchange x87 Registers
 * Exchanges ST(0) with another x87 stack register.
 *
 * Syntax: FXCH [ST(i)]
 * Operands: optional x87 register operand, default ST(1)
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Reorders the logical x87 stack without moving actual data in memory.
 *
 * Example:
 *   fxch st(1)
 */
static inline void write_fxch(const char* operand) {
    write_line2(ASM_FXCH, operand);
}

/*
 * FADD - Floating-Point Add
 * Adds the source operand to ST(0) or adds ST(0) to an x87 destination.
 *
 * Syntax: FADD m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fadd st(1)
 */
static inline void write_fadd(const char* operand) {
    write_line2(ASM_FADD, operand);
}

/*
 * FADDP - Floating-Point Add and Pop
 * Adds ST(0) to the destination x87 register and then pops the stack.
 *
 * Syntax: FADDP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   faddp st(1), st(0)
 */
static inline void write_faddp(const char* operand) {
    write_line2(ASM_FADDP, operand);
}

/*
 * FSUB - Floating-Point Subtract
 * Subtracts the source operand from the destination x87 operand.
 *
 * Syntax: FSUB m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fsub st(1)
 */
static inline void write_fsub(const char* operand) {
    write_line2(ASM_FSUB, operand);
}

/*
 * FSUBP - Floating-Point Subtract and Pop
 * Subtracts ST(0) from the destination x87 register and pops the stack.
 *
 * Syntax: FSUBP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fsubp st(1), st(0)
 */
static inline void write_fsubp(const char* operand) {
    write_line2(ASM_FSUBP, operand);
}

/*
 * FSUBR - Floating-Point Reverse Subtract
 * Subtracts the destination from the source order opposite to FSUB.
 *
 * Syntax: FSUBR m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fsubr st(1)
 */
static inline void write_fsubr(const char* operand) {
    write_line2(ASM_FSUBR, operand);
}

/*
 * FSUBRP - Floating-Point Reverse Subtract and Pop
 * Performs reverse subtraction and pops the x87 stack.
 *
 * Syntax: FSUBRP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fsubrp st(1), st(0)
 */
static inline void write_fsubrp(const char* operand) {
    write_line2(ASM_FSUBRP, operand);
}

/*
 * FMUL - Floating-Point Multiply
 * Multiplies x87 operands and stores the product in the destination.
 *
 * Syntax: FMUL m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fmul st(1)
 */
static inline void write_fmul(const char* operand) {
    write_line2(ASM_FMUL, operand);
}

/*
 * FMULP - Floating-Point Multiply and Pop
 * Multiplies the destination x87 register by ST(0) and then pops the stack.
 *
 * Syntax: FMULP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fmulp st(1), st(0)
 */
static inline void write_fmulp(const char* operand) {
    write_line2(ASM_FMULP, operand);
}

/*
 * FDIV - Floating-Point Divide
 * Divides the destination by the source in the default x87 operand order.
 *
 * Syntax: FDIV m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fdiv st(1)
 */
static inline void write_fdiv(const char* operand) {
    write_line2(ASM_FDIV, operand);
}

/*
 * FDIVP - Floating-Point Divide and Pop
 * Divides the destination x87 register by ST(0) and pops the stack.
 *
 * Syntax: FDIVP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fdivp st(1), st(0)
 */
static inline void write_fdivp(const char* operand) {
    write_line2(ASM_FDIVP, operand);
}

/*
 * FDIVR - Floating-Point Reverse Divide
 * Divides in the reverse operand order relative to FDIV.
 *
 * Syntax: FDIVR m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fdivr st(1)
 */
static inline void write_fdivr(const char* operand) {
    write_line2(ASM_FDIVR, operand);
}

/*
 * FDIVRP - Floating-Point Reverse Divide and Pop
 * Performs reverse division and pops the x87 stack.
 *
 * Syntax: FDIVRP m32fp|m64fp|ST(i) or register-stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: x87 arithmetic uses the current control-word precision, rounding, and exception-mask
 * settings.
 *
 * Example:
 *   fdivrp st(1), st(0)
 */
static inline void write_fdivrp(const char* operand) {
    write_line2(ASM_FDIVRP, operand);
}

/*
 * FCHS - Change Sign
 * Negates the sign of ST(0) without changing its magnitude.
 *
 * Syntax: FCHS
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fchs
 */
static inline void write_fchs(void) {
    write_line(ASM_FCHS);
}

/*
 * FABS - Absolute Value
 * Clears the sign bit of ST(0).
 *
 * Syntax: FABS
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fabs
 */
static inline void write_fabs(void) {
    write_line(ASM_FABS);
}

/*
 * FSQRT - Square Root
 * Replaces ST(0) with its square root.
 *
 * Syntax: FSQRT
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fsqrt
 */
static inline void write_fsqrt(void) {
    write_line(ASM_FSQRT);
}

/*
 * FPREM - Partial Remainder
 * Computes a partial remainder useful for argument reduction.
 *
 * Syntax: FPREM
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fprem
 */
static inline void write_fprem(void) {
    write_line(ASM_FPREM);
}

/*
 * FPREM1 - IEEE Partial Remainder
 * Computes an IEEE-style partial remainder.
 *
 * Syntax: FPREM1
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fprem1
 */
static inline void write_fprem1(void) {
    write_line(ASM_FPREM1);
}

/*
 * FRNDINT - Round to Integer
 * Rounds ST(0) to an integral value using the current rounding mode.
 *
 * Syntax: FRNDINT
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   frndint
 */
static inline void write_frndint(void) {
    write_line(ASM_FRNDINT);
}

/*
 * FSIN - Sine
 * Replaces ST(0) with its sine.
 *
 * Syntax: FSIN
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fsin
 */
static inline void write_fsin(void) {
    write_line(ASM_FSIN);
}

/*
 * FCOS - Cosine
 * Replaces ST(0) with its cosine.
 *
 * Syntax: FCOS
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fcos
 */
static inline void write_fcos(void) {
    write_line(ASM_FCOS);
}

/*
 * FSINCOS - Sine and Cosine
 * Computes sine and cosine, leaving one result in ST(0) and pushing the other.
 *
 * Syntax: FSINCOS
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fsincos
 */
static inline void write_fsincos(void) {
    write_line(ASM_FSINCOS);
}

/*
 * FPTAN - Partial Tangent
 * Computes tangent and pushes 1.0 as defined by the x87 architecture.
 *
 * Syntax: FPTAN
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fptan
 */
static inline void write_fptan(void) {
    write_line(ASM_FPTAN);
}

/*
 * FPATAN - Arctangent
 * Computes arctangent from ST(1) and ST(0), stores the result, and pops the stack.
 *
 * Syntax: FPATAN
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fpatan
 */
static inline void write_fpatan(void) {
    write_line(ASM_FPATAN);
}

/*
 * F2XM1 - 2^x Minus 1
 * Computes 2 raised to ST(0) minus 1 for a restricted input range.
 *
 * Syntax: F2XM1
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   f2xm1
 */
static inline void write_f2xm1(void) {
    write_line(ASM_F2XM1);
}

/*
 * FYL2X - y * log2(x)
 * Computes ST(1) * log2(ST(0)), stores the result, and pops the stack.
 *
 * Syntax: FYL2X
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fyl2x
 */
static inline void write_fyl2x(void) {
    write_line(ASM_FYL2X);
}

/*
 * FYL2XP1 - y * log2(x+1)
 * Computes ST(1) * log2(ST(0)+1) and pops the stack.
 *
 * Syntax: FYL2XP1
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fyl2xp1
 */
static inline void write_fyl2xp1(void) {
    write_line(ASM_FYL2XP1);
}

/*
 * FSCALE - Scale by Power of Two
 * Scales ST(0) by 2 raised to the integer part of ST(1).
 *
 * Syntax: FSCALE
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fscale
 */
static inline void write_fscale(void) {
    write_line(ASM_FSCALE);
}

/*
 * FXTRACT - Extract Exponent and Significand
 * Splits ST(0) into significand and exponent values on the x87 stack.
 *
 * Syntax: FXTRACT
 * Operands: implicit x87 stack operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions are controlled by x87 precision, rounding, and exception state and
 * often have restricted valid input ranges.
 *
 * Example:
 *   fxtract
 */
static inline void write_fxtract(void) {
    write_line(ASM_FXTRACT);
}

/*
 * FCOM - Floating-Point Compare
 * Compares ST(0) with the source operand and updates x87 condition codes.
 *
 * Syntax: FCOM m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS.
 *
 * Example:
 *   fcom st(1)
 */
static inline void write_fcom(const char* operand) {
    write_line2(ASM_FCOM, operand);
}

/*
 * FCOMP - Floating-Point Compare and Pop
 * Compares and then pops one x87 stack element.
 *
 * Syntax: FCOMP m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS.
 *
 * Example:
 *   fcomp st(1)
 */
static inline void write_fcomp(const char* operand) {
    write_line2(ASM_FCOMP, operand);
}

/*
 * FCOMPP - Floating-Point Compare and Pop Twice
 * Compares ST(0) and ST(1) and pops both stack elements.
 *
 * Syntax: FCOMPP m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS.
 *
 * Example:
 *   fcompp
 */
static inline void write_fcompp(void) {
    write_line(ASM_FCOMPP);
}

/*
 * FUCOM - Floating-Point Unordered Compare
 * Performs an unordered compare that treats NaNs according to the FUCOM rules.
 *
 * Syntax: FUCOM m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS.
 *
 * Example:
 *   fucom st(1)
 */
static inline void write_fucom(const char* operand) {
    write_line2(ASM_FUCOM, operand);
}

/*
 * FUCOMP - Floating-Point Unordered Compare and Pop
 * Unordered compare followed by one pop.
 *
 * Syntax: FUCOMP m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS.
 *
 * Example:
 *   fucomp st(1)
 */
static inline void write_fucomp(const char* operand) {
    write_line2(ASM_FUCOMP, operand);
}

/*
 * FUCOMPP - Floating-Point Unordered Compare and Pop Twice
 * Unordered compare followed by popping both operands.
 *
 * Syntax: FUCOMPP m32fp|m64fp|ST(i) or stack form
 * Operands: x87 register-stack operands and certain memory operands
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: Results appear in the x87 status word condition codes C0, C2, and C3 rather than
 * directly in EFLAGS.
 *
 * Example:
 *   fucompp
 */
static inline void write_fucompp(void) {
    write_line(ASM_FUCOMPP);
}

/*
 * FCOMI - Compare and Set EFLAGS
 * Compares ST(0) with ST(i) and writes the result into ZF, PF, and CF in EFLAGS.
 *
 * Syntax: FCOMI ST(i)
 * Operands: ST(0) and another x87 register
 * Flags affected: ZF, PF, and CF are updated to encode the compare result; OF, SF, and AF are
 * cleared
 * Notes: These are the x87 compare instructions that directly update integer condition codes for
 * easy Jcc/SETcc use.
 *
 * Example:
 *   fcomi st(1)
 */
static inline void write_fcomi(const char* operand) {
    write_line2(ASM_FCOMI, operand);
}

/*
 * FCOMIP - Compare, Set EFLAGS, and Pop
 * Like FCOMI but pops the x87 stack once after the compare.
 *
 * Syntax: FCOMIP ST(i)
 * Operands: ST(0) and another x87 register
 * Flags affected: ZF, PF, and CF are updated to encode the compare result; OF, SF, and AF are
 * cleared
 * Notes: These are the x87 compare instructions that directly update integer condition codes for
 * easy Jcc/SETcc use.
 *
 * Example:
 *   fcomip st(1)
 */
static inline void write_fcomip(const char* operand) {
    write_line2(ASM_FCOMIP, operand);
}

/*
 * FUCOMI - Unordered Compare and Set EFLAGS
 * Performs an unordered compare and writes ZF, PF, and CF in EFLAGS.
 *
 * Syntax: FUCOMI ST(i)
 * Operands: ST(0) and another x87 register
 * Flags affected: ZF, PF, and CF are updated to encode the compare result; OF, SF, and AF are
 * cleared
 * Notes: These are the x87 compare instructions that directly update integer condition codes for
 * easy Jcc/SETcc use.
 *
 * Example:
 *   fucomi st(1)
 */
static inline void write_fucomi(const char* operand) {
    write_line2(ASM_FUCOMI, operand);
}

/*
 * FUCOMIP - Unordered Compare, Set EFLAGS, and Pop
 * Unordered compare with EFLAGS result and one stack pop.
 *
 * Syntax: FUCOMIP ST(i)
 * Operands: ST(0) and another x87 register
 * Flags affected: ZF, PF, and CF are updated to encode the compare result; OF, SF, and AF are
 * cleared
 * Notes: These are the x87 compare instructions that directly update integer condition codes for
 * easy Jcc/SETcc use.
 *
 * Example:
 *   fucomip st(1)
 */
static inline void write_fucomip(const char* operand) {
    write_line2(ASM_FUCOMIP, operand);
}

/*
 * FTST - Test against Zero
 * Compares ST(0) with 0.0 and updates x87 condition codes.
 *
 * Syntax: FTST ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   ftst
 */
static inline void write_ftst(void) {
    write_line(ASM_FTST);
}

/*
 * FXAM - Examine Value
 * Classifies the value in ST(0) and updates x87 condition codes.
 *
 * Syntax: FXAM ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fxam
 */
static inline void write_fxam(void) {
    write_line(ASM_FXAM);
}

/*
 * FLDZ - Load +0.0
 * Pushes the constant 0.0 onto the x87 stack.
 *
 * Syntax: FLDZ ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldz
 */
static inline void write_fldz(void) {
    write_line(ASM_FLDZ);
}

/*
 * FLD1 - Load +1.0
 * Pushes the constant 1.0 onto the x87 stack.
 *
 * Syntax: FLD1 ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fld1
 */
static inline void write_fld1(void) {
    write_line(ASM_FLD1);
}

/*
 * FLDPI - Load pi
 * Pushes the constant pi onto the x87 stack.
 *
 * Syntax: FLDPI ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldpi
 */
static inline void write_fldpi(void) {
    write_line(ASM_FLDPI);
}

/*
 * FLDL2E - Load log2(e)
 * Pushes the constant log2(e) onto the x87 stack.
 *
 * Syntax: FLDL2E ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldl2e
 */
static inline void write_fldl2e(void) {
    write_line(ASM_FLDL2E);
}

/*
 * FLDL2T - Load log2(10)
 * Pushes the constant log2(10) onto the x87 stack.
 *
 * Syntax: FLDL2T ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldl2t
 */
static inline void write_fldl2t(void) {
    write_line(ASM_FLDL2T);
}

/*
 * FLDLG2 - Load log10(2)
 * Pushes the constant log10(2) onto the x87 stack.
 *
 * Syntax: FLDLG2 ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldlg2
 */
static inline void write_fldlg2(void) {
    write_line(ASM_FLDLG2);
}

/*
 * FLDLN2 - Load ln(2)
 * Pushes the constant ln(2) onto the x87 stack.
 *
 * Syntax: FLDLN2 ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldln2
 */
static inline void write_fldln2(void) {
    write_line(ASM_FLDLN2);
}

/*
 * FINIT - Initialize x87 Unit
 * Waits for pending exceptions and resets the x87 FPU environment to its default state.
 *
 * Syntax: FINIT ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   finit
 */
static inline void write_finit(void) {
    write_line(ASM_FINIT);
}

/*
 * FNINIT - Initialize x87 Unit without Wait
 * Resets the x87 environment without the implicit FWAIT step.
 *
 * Syntax: FNINIT ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fninit
 */
static inline void write_fninit(void) {
    write_line(ASM_FNINIT);
}

/*
 * FLDCW - Load x87 Control Word
 * Loads the x87 control word from memory.
 *
 * Syntax: FLDCW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fldcw word ptr [cw]
 */
static inline void write_fldcw(const char* operand) {
    write_line2(ASM_FLDCW, operand);
}

/*
 * FSTCW - Store x87 Control Word
 * Stores the x87 control word to memory with the wait form.
 *
 * Syntax: FSTCW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fstcw word ptr [cw]
 */
static inline void write_fstcw(const char* operand) {
    write_line2(ASM_FSTCW, operand);
}

/*
 * FNSTCW - Store x87 Control Word without Wait
 * Stores the x87 control word to memory without an implicit FWAIT.
 *
 * Syntax: FNSTCW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnstcw word ptr [cw]
 */
static inline void write_fnstcw(const char* operand) {
    write_line2(ASM_FNSTCW, operand);
}

/*
 * FSTSW - Store x87 Status Word
 * Stores the x87 status word with wait semantics.
 *
 * Syntax: FSTSW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fstsw ax
 */
static inline void write_fstsw(const char* operand) {
    write_line2(ASM_FSTSW, operand);
}

/*
 * FNSTSW - Store x87 Status Word without Wait
 * Stores the x87 status word without an implicit FWAIT.
 *
 * Syntax: FNSTSW ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnstsw ax
 */
static inline void write_fnstsw(const char* operand) {
    write_line2(ASM_FNSTSW, operand);
}

/*
 * FCLEX - Clear x87 Exceptions
 * Clears pending x87 exception flags with wait semantics.
 *
 * Syntax: FCLEX ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fclex
 */
static inline void write_fclex(void) {
    write_line(ASM_FCLEX);
}

/*
 * FNCLEX - Clear x87 Exceptions without Wait
 * Clears pending x87 exception flags without an implicit FWAIT.
 *
 * Syntax: FNCLEX ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnclex
 */
static inline void write_fnclex(void) {
    write_line(ASM_FNCLEX);
}

/*
 * FWAIT - Wait for x87
 * Waits until the x87 unit is ready and checks for pending unmasked x87 exceptions.
 *
 * Syntax: FWAIT ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fwait
 */
static inline void write_fwait(void) {
    write_line(ASM_FWAIT);
}

/*
 * WAIT - Wait Alias
 * Alias for FWAIT.
 *
 * Syntax: WAIT ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   wait
 */
static inline void write_wait(void) {
    write_line(ASM_WAIT);
}

/*
 * FNOP - x87 No Operation
 * Consumes an x87 instruction slot without changing meaningful x87 data state.
 *
 * Syntax: FNOP ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnop
 */
static inline void write_fnop(void) {
    write_line(ASM_FNOP);
}

/*
 * FDECSTP - Decrement x87 Stack Pointer
 * Adjusts the x87 TOP pointer downward, effectively rotating the logical stack view.
 *
 * Syntax: FDECSTP ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fdecstp
 */
static inline void write_fdecstp(void) {
    write_line(ASM_FDECSTP);
}

/*
 * FINCSTP - Increment x87 Stack Pointer
 * Adjusts the x87 TOP pointer upward, effectively rotating the logical stack view.
 *
 * Syntax: FINCSTP ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fincstp
 */
static inline void write_fincstp(void) {
    write_line(ASM_FINCSTP);
}

/*
 * FFREE - Free x87 Register
 * Marks the specified x87 register stack entry as empty.
 *
 * Syntax: FFREE ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   ffree st(1)
 */
static inline void write_ffree(const char* operand) {
    write_line2(ASM_FFREE, operand);
}

/*
 * FSAVE - Save x87 Environment and Registers
 * Saves the x87 environment and register stack to memory, then initializes the FPU.
 *
 * Syntax: FSAVE ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fsave [mem]
 */
static inline void write_fsave(const char* operand) {
    write_line2(ASM_FSAVE, operand);
}

/*
 * FNSAVE - Save x87 State without Wait
 * Saves x87 state without an implicit FWAIT and then initializes the FPU.
 *
 * Syntax: FNSAVE ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fnsave [mem]
 */
static inline void write_fnsave(const char* operand) {
    write_line2(ASM_FNSAVE, operand);
}

/*
 * FRSTOR - Restore x87 Environment and Registers
 * Restores the x87 environment and register stack from memory.
 *
 * Syntax: FRSTOR ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   frstor [mem]
 */
static inline void write_frstor(const char* operand) {
    write_line2(ASM_FRSTOR, operand);
}

/*
 * FXSAVE - Save FPU/MMX/XMM State
 * Stores the architectural floating-point, MMX, and XMM state to an FXSAVE area.
 *
 * Syntax: FXSAVE ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fxsave [mem]
 */
static inline void write_fxsave(const char* operand) {
    write_line2(ASM_FXSAVE, operand);
}

/*
 * FXRSTOR - Restore FPU/MMX/XMM State
 * Restores architectural floating-point, MMX, and XMM state from an FXRSTOR area.
 *
 * Syntax: FXRSTOR ...
 * Operands: implicit x87 state or a memory/control-word operand depending on the form
 * Flags affected: EFLAGS are usually unaffected; x87 status-word condition codes, exceptions,
 * and stack-top state may change
 * Notes: These instructions manage x87 constants, environment, exception state, or context
 * save/restore behavior rather than ordinary arithmetic.
 *
 * Example:
 *   fxrstor [mem]
 */
static inline void write_fxrstor(const char* operand) {
    write_line2(ASM_FXRSTOR, operand);
}

/*
 * ============================================================================
 * 19. SSE/SSE2 Instructions
 * ============================================================================
 */

/*
 * MOVAPS - Move Aligned Packed Single
 * Moves a 128-bit packed single-precision vector between XMM registers or aligned memory.
 *
 * Syntax: MOVAPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movaps xmm0, xmmword ptr [rax]
 */
static inline void write_movaps(const char* dest, const char* src) {
    write_line3(ASM_MOVAPS, dest, src);
}

/*
 * MOVUPS - Move Unaligned Packed Single
 * Moves a 128-bit packed single-precision vector without requiring aligned memory.
 *
 * Syntax: MOVUPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movups xmm0, xmmword ptr [rax]
 */
static inline void write_movups(const char* dest, const char* src) {
    write_line3(ASM_MOVUPS, dest, src);
}

/*
 * MOVSS - Move Scalar Single
 * Moves the low scalar single-precision element between registers or memory.
 *
 * Syntax: MOVSS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movss xmm0, dword ptr [rax]
 */
static inline void write_movss(const char* dest, const char* src) {
    write_line3(ASM_MOVSS, dest, src);
}

/* Same move of `src` into `dest`, and record `tag` on an xmm destination. A memory
 * destination is left out of the dictionary, because that operand text is an unstable location. */
static inline void write_movss_tag(const char* dest, const char* src, const char* tag) {
    write_movss(dest, src);
    if (dest && dest[0] == 'x' && tag)
        asm_type_dict_set(dest, tag);
}

/*
 * MOVAPD - Move Aligned Packed Double
 * Moves a 128-bit packed double-precision vector between XMM registers or aligned memory.
 *
 * Syntax: MOVAPD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movapd xmm0, xmmword ptr [rax]
 */
static inline void write_movapd(const char* dest, const char* src) {
    write_line3(ASM_MOVAPD, dest, src);
}

/*
 * MOVUPD - Move Unaligned Packed Double
 * Moves a 128-bit packed double-precision vector without requiring aligned memory.
 *
 * Syntax: MOVUPD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movupd xmm0, xmmword ptr [rax]
 */
static inline void write_movupd(const char* dest, const char* src) {
    write_line3(ASM_MOVUPD, dest, src);
}

/*
 * MOVSD_SSE - Move Scalar Double (SSE2)
 * Moves the low scalar double-precision element between XMM registers or memory; this is the
 * SSE2 scalar move, not the string instruction of the same mnemonic.
 *
 * Syntax: MOVSD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movsd xmm0, qword ptr [rax]
 */
static inline void write_movsd_sse(const char* dest, const char* src) {
    write_line3(ASM_MOVSD_SSE, dest, src);
}

/*
 * MOVDQA - Move Aligned Integer Vector
 * Moves 128-bit integer data between XMM registers or aligned memory.
 *
 * Syntax: MOVDQA ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movdqa xmm0, xmmword ptr [rax]
 */
static inline void write_movdqa(const char* dest, const char* src) {
    write_line3(ASM_MOVDQA, dest, src);
}

/*
 * MOVDQU - Move Unaligned Integer Vector
 * Moves 128-bit integer data between XMM registers or unaligned memory.
 *
 * Syntax: MOVDQU ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movdqu xmm0, xmmword ptr [rax]
 */
static inline void write_movdqu(const char* dest, const char* src) {
    write_line3(ASM_MOVDQU, dest, src);
}

/*
 * MOVLPS - Move Low Packed Singles
 * Moves the low 64 bits of packed single data between an XMM register and memory.
 *
 * Syntax: MOVLPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movlps xmm0, qword ptr [rax]
 */
static inline void write_movlps(const char* dest, const char* src) {
    write_line3(ASM_MOVLPS, dest, src);
}

/*
 * MOVHPS - Move High Packed Singles
 * Moves the high 64 bits of packed single data between an XMM register and memory.
 *
 * Syntax: MOVHPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movhps xmm0, qword ptr [rax]
 */
static inline void write_movhps(const char* dest, const char* src) {
    write_line3(ASM_MOVHPS, dest, src);
}

/*
 * MOVLPD - Move Low Packed Double
 * Moves the low 64 bits of a packed double vector between an XMM register and memory.
 *
 * Syntax: MOVLPD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movlpd xmm0, qword ptr [rax]
 */
static inline void write_movlpd(const char* dest, const char* src) {
    write_line3(ASM_MOVLPD, dest, src);
}

/*
 * MOVHPD - Move High Packed Double
 * Moves the high 64 bits of a packed double vector between an XMM register and memory.
 *
 * Syntax: MOVHPD ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movhpd xmm0, qword ptr [rax]
 */
static inline void write_movhpd(const char* dest, const char* src) {
    write_line3(ASM_MOVHPD, dest, src);
}

/*
 * MOVLHPS - Move Low to High Packed Singles
 * Copies the low 64 bits of one XMM register into the high 64 bits of another.
 *
 * Syntax: MOVLHPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movlhps xmm0, xmm1
 */
static inline void write_movlhps(const char* dest, const char* src) {
    write_line3(ASM_MOVLHPS, dest, src);
}

/*
 * MOVHLPS - Move High to Low Packed Singles
 * Copies the high 64 bits of one XMM register into the low 64 bits of another.
 *
 * Syntax: MOVHLPS ...
 * Operands: XMM registers and/or 64-bit or 128-bit memory operands as dictated by the specific
 * form
 * Flags affected: None
 * Notes: These are data-movement instructions; alignment requirements apply to the aligned forms
 * only.
 *
 * Example:
 *   movhlps xmm0, xmm1
 */
static inline void write_movhlps(const char* dest, const char* src) {
    write_line3(ASM_MOVHLPS, dest, src);
}

/*
 * ADDPS - Add Packed Singles
 * Adds four packed single-precision elements.
 *
 * Syntax: addps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   addps xmm0, xmm1
 */
static inline void write_addps(const char* dest, const char* src) {
    write_line3(ASM_ADDPS, dest, src);
}

/*
 * ADDPD - Add Packed Doubles
 * Adds two packed double-precision elements.
 *
 * Syntax: addpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   addpd xmm0, xmm1
 */
static inline void write_addpd(const char* dest, const char* src) {
    write_line3(ASM_ADDPD, dest, src);
}

/*
 * ADDSS - Add Scalar Single
 * Adds the low scalar single-precision elements and leaves upper lanes unchanged in legacy SSE
 * encoding.
 *
 * Syntax: addss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   addss xmm0, xmm1
 */
static inline void write_addss(const char* dest, const char* src) {
    write_line3(ASM_ADDSS, dest, src);
}

/*
 * ADDSD - Add Scalar Double
 * Adds the low scalar double-precision elements and leaves upper lanes unchanged in legacy SSE2
 * encoding.
 *
 * Syntax: addsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   addsd xmm0, xmm1
 */
static inline void write_addsd(const char* dest, const char* src) {
    write_line3(ASM_ADDSD, dest, src);
}

/*
 * SUBPS - Subtract Packed Singles
 * Subtracts packed single-precision elements.
 *
 * Syntax: subps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   subps xmm0, xmm1
 */
static inline void write_subps(const char* dest, const char* src) {
    write_line3(ASM_SUBPS, dest, src);
}

/*
 * SUBPD - Subtract Packed Doubles
 * Subtracts packed double-precision elements.
 *
 * Syntax: subpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   subpd xmm0, xmm1
 */
static inline void write_subpd(const char* dest, const char* src) {
    write_line3(ASM_SUBPD, dest, src);
}

/*
 * SUBSS - Subtract Scalar Single
 * Subtracts the low scalar single-precision elements.
 *
 * Syntax: subss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   subss xmm0, xmm1
 */
static inline void write_subss(const char* dest, const char* src) {
    write_line3(ASM_SUBSS, dest, src);
}

/*
 * SUBSD - Subtract Scalar Double
 * Subtracts the low scalar double-precision elements.
 *
 * Syntax: subsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   subsd xmm0, xmm1
 */
static inline void write_subsd(const char* dest, const char* src) {
    write_line3(ASM_SUBSD, dest, src);
}

/*
 * MULPS - Multiply Packed Singles
 * Multiplies packed single-precision elements.
 *
 * Syntax: mulps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   mulps xmm0, xmm1
 */
static inline void write_mulps(const char* dest, const char* src) {
    write_line3(ASM_MULPS, dest, src);
}

/*
 * MULPD - Multiply Packed Doubles
 * Multiplies packed double-precision elements.
 *
 * Syntax: mulpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   mulpd xmm0, xmm1
 */
static inline void write_mulpd(const char* dest, const char* src) {
    write_line3(ASM_MULPD, dest, src);
}

/*
 * MULSS - Multiply Scalar Single
 * Multiplies the low scalar single-precision elements.
 *
 * Syntax: mulss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   mulss xmm0, xmm1
 */
static inline void write_mulss(const char* dest, const char* src) {
    write_line3(ASM_MULSS, dest, src);
}

/*
 * MULSD - Multiply Scalar Double
 * Multiplies the low scalar double-precision elements.
 *
 * Syntax: mulsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   mulsd xmm0, xmm1
 */
static inline void write_mulsd(const char* dest, const char* src) {
    write_line3(ASM_MULSD, dest, src);
}

/*
 * DIVPS - Divide Packed Singles
 * Divides packed single-precision elements.
 *
 * Syntax: divps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   divps xmm0, xmm1
 */
static inline void write_divps(const char* dest, const char* src) {
    write_line3(ASM_DIVPS, dest, src);
}

/*
 * DIVPD - Divide Packed Doubles
 * Divides packed double-precision elements.
 *
 * Syntax: divpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   divpd xmm0, xmm1
 */
static inline void write_divpd(const char* dest, const char* src) {
    write_line3(ASM_DIVPD, dest, src);
}

/*
 * DIVSS - Divide Scalar Single
 * Divides the low scalar single-precision elements.
 *
 * Syntax: divss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   divss xmm0, xmm1
 */
static inline void write_divss(const char* dest, const char* src) {
    write_line3(ASM_DIVSS, dest, src);
}

/*
 * DIVSD - Divide Scalar Double
 * Divides the low scalar double-precision elements.
 *
 * Syntax: divsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   divsd xmm0, xmm1
 */
static inline void write_divsd(const char* dest, const char* src) {
    write_line3(ASM_DIVSD, dest, src);
}

/*
 * SQRTPS - Square Root Packed Singles
 * Computes packed single-precision square roots.
 *
 * Syntax: sqrtps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   sqrtps xmm0, xmm1
 */
static inline void write_sqrtps(const char* dest, const char* src) {
    write_line3(ASM_SQRTPS, dest, src);
}

/*
 * SQRTPD - Square Root Packed Doubles
 * Computes packed double-precision square roots.
 *
 * Syntax: sqrtpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   sqrtpd xmm0, xmm1
 */
static inline void write_sqrtpd(const char* dest, const char* src) {
    write_line3(ASM_SQRTPD, dest, src);
}

/*
 * SQRTSS - Square Root Scalar Single
 * Computes the square root of the low scalar single-precision element.
 *
 * Syntax: sqrtss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   sqrtss xmm0, xmm1
 */
static inline void write_sqrtss(const char* dest, const char* src) {
    write_line3(ASM_SQRTSS, dest, src);
}

/*
 * SQRTSD - Square Root Scalar Double
 * Computes the square root of the low scalar double-precision element.
 *
 * Syntax: sqrtsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   sqrtsd xmm0, xmm1
 */
static inline void write_sqrtsd(const char* dest, const char* src) {
    write_line3(ASM_SQRTSD, dest, src);
}

/*
 * MAXPS - Packed Maximum Singles
 * Selects the greater packed single-precision element from each lane.
 *
 * Syntax: maxps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   maxps xmm0, xmm1
 */
static inline void write_maxps(const char* dest, const char* src) {
    write_line3(ASM_MAXPS, dest, src);
}

/*
 * MAXPD - Packed Maximum Doubles
 * Selects the greater packed double-precision element from each lane.
 *
 * Syntax: maxpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   maxpd xmm0, xmm1
 */
static inline void write_maxpd(const char* dest, const char* src) {
    write_line3(ASM_MAXPD, dest, src);
}

/*
 * MAXSS - Scalar Maximum Single
 * Selects the greater low scalar single-precision element.
 *
 * Syntax: maxss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   maxss xmm0, xmm1
 */
static inline void write_maxss(const char* dest, const char* src) {
    write_line3(ASM_MAXSS, dest, src);
}

/*
 * MAXSD - Scalar Maximum Double
 * Selects the greater low scalar double-precision element.
 *
 * Syntax: maxsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   maxsd xmm0, xmm1
 */
static inline void write_maxsd(const char* dest, const char* src) {
    write_line3(ASM_MAXSD, dest, src);
}

/*
 * MINPS - Packed Minimum Singles
 * Selects the smaller packed single-precision element from each lane.
 *
 * Syntax: minps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   minps xmm0, xmm1
 */
static inline void write_minps(const char* dest, const char* src) {
    write_line3(ASM_MINPS, dest, src);
}

/*
 * MINPD - Packed Minimum Doubles
 * Selects the smaller packed double-precision element from each lane.
 *
 * Syntax: minpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   minpd xmm0, xmm1
 */
static inline void write_minpd(const char* dest, const char* src) {
    write_line3(ASM_MINPD, dest, src);
}

/*
 * MINSS - Scalar Minimum Single
 * Selects the smaller low scalar single-precision element.
 *
 * Syntax: minss xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   minss xmm0, xmm1
 */
static inline void write_minss(const char* dest, const char* src) {
    write_line3(ASM_MINSS, dest, src);
}

/*
 * MINSD - Scalar Minimum Double
 * Selects the smaller low scalar double-precision element.
 *
 * Syntax: minsd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These arithmetic operations do not update integer EFLAGS. Floating-point exceptions and
 * MXCSR state govern exceptional behavior.
 *
 * Example:
 *   minsd xmm0, xmm1
 */
static inline void write_minsd(const char* dest, const char* src) {
    write_line3(ASM_MINSD, dest, src);
}

/*
 * CMPPS - Compare Packed Singles
 * Compares packed single-precision elements and writes an all-ones/all-zeros mask result to each
 * lane.
 *
 * Syntax: cmpps xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and comparison-immediate selector
 * Flags affected: None
 * Notes: The immediate selects the comparison predicate; the result is a vector mask rather than
 * EFLAGS bits.
 *
 * Example:
 *   cmpps xmm0, xmm1, 0
 */
static inline void write_cmpps(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_CMPPS, dest, src, imm);
}

/*
 * CMPPD - Compare Packed Doubles
 * Compares packed double-precision elements and writes mask results per lane.
 *
 * Syntax: cmppd xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and comparison-immediate selector
 * Flags affected: None
 * Notes: The immediate selects the comparison predicate; the result is a vector mask rather than
 * EFLAGS bits.
 *
 * Example:
 *   cmppd xmm0, xmm1, 0
 */
static inline void write_cmppd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_CMPPD, dest, src, imm);
}

/*
 * CMPSS - Compare Scalar Single
 * Compares the low scalar single-precision elements and writes a mask result to the low lane.
 *
 * Syntax: cmpss xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and comparison-immediate selector
 * Flags affected: None
 * Notes: The immediate selects the comparison predicate; the result is a vector mask rather than
 * EFLAGS bits.
 *
 * Example:
 *   cmpss xmm0, xmm1, 0
 */
static inline void write_cmpss(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_CMPSS, dest, src, imm);
}

/*
 * CMPSD_SSE - Compare Scalar Double (SSE2)
 * Compares the low scalar double-precision elements and writes a mask result to the low lane;
 * this is the SSE compare, not the string CMPSD instruction.
 *
 * Syntax: cmpsd xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source, XMM or memory source, and comparison-immediate selector
 * Flags affected: None
 * Notes: The immediate selects the comparison predicate; the result is a vector mask rather than
 * EFLAGS bits.
 *
 * Example:
 *   cmpsd xmm0, xmm1, 0
 */
static inline void write_cmpsd_sse(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_CMPSD_SSE, dest, src, imm);
}

/*
 * COMISS - Compare Ordered Scalar Single and Set EFLAGS
 * Performs an ordered compare of low scalar single-precision values and writes ZF, PF, and CF in
 * EFLAGS.
 *
 * Syntax: comiss xmm1, xmm2/m64
 * Operands: XMM register operands or one XMM plus memory source
 * Flags affected: ZF, PF, and CF are updated; OF, SF, and AF are cleared
 * Notes: These compare instructions are the SIMD equivalents that feed ordinary Jcc/SETcc
 * integer control flow.
 *
 * Example:
 *   comiss xmm0, xmm1
 */
static inline void write_comiss(const char* dest, const char* src) {
    write_line3(ASM_COMISS, dest, src);
}

/*
 * COMISD - Compare Ordered Scalar Double and Set EFLAGS
 * Performs an ordered compare of low scalar double-precision values and writes ZF, PF, and CF in
 * EFLAGS.
 *
 * Syntax: comisd xmm1, xmm2/m64
 * Operands: XMM register operands or one XMM plus memory source
 * Flags affected: ZF, PF, and CF are updated; OF, SF, and AF are cleared
 * Notes: These compare instructions are the SIMD equivalents that feed ordinary Jcc/SETcc
 * integer control flow.
 *
 * Example:
 *   comisd xmm0, xmm1
 */
static inline void write_comisd(const char* dest, const char* src) {
    write_line3(ASM_COMISD, dest, src);
}

/*
 * UCOMISS - Compare Unordered Scalar Single and Set EFLAGS
 * Performs an unordered compare of low scalar single-precision values and writes ZF, PF, and CF
 * in EFLAGS.
 *
 * Syntax: ucomiss xmm1, xmm2/m64
 * Operands: XMM register operands or one XMM plus memory source
 * Flags affected: ZF, PF, and CF are updated; OF, SF, and AF are cleared
 * Notes: These compare instructions are the SIMD equivalents that feed ordinary Jcc/SETcc
 * integer control flow.
 *
 * Example:
 *   ucomiss xmm0, xmm1
 */
static inline void write_ucomiss(const char* dest, const char* src) {
    write_line3(ASM_UCOMISS, dest, src);
}

/*
 * UCOMISD - Compare Unordered Scalar Double and Set EFLAGS
 * Performs an unordered compare of low scalar double-precision values and writes ZF, PF, and CF
 * in EFLAGS.
 *
 * Syntax: ucomisd xmm1, xmm2/m64
 * Operands: XMM register operands or one XMM plus memory source
 * Flags affected: ZF, PF, and CF are updated; OF, SF, and AF are cleared
 * Notes: These compare instructions are the SIMD equivalents that feed ordinary Jcc/SETcc
 * integer control flow.
 *
 * Example:
 *   ucomisd xmm0, xmm1
 */
static inline void write_ucomisd(const char* dest, const char* src) {
    write_line3(ASM_UCOMISD, dest, src);
}

/*
 * ANDPS - Bitwise AND Packed Singles
 * Bitwise AND of packed single-precision bit patterns.
 *
 * Syntax: andps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   andps xmm0, xmm1
 */
static inline void write_andps(const char* dest, const char* src) {
    write_line3(ASM_ANDPS, dest, src);
}

/*
 * ANDPD - Bitwise AND Packed Doubles
 * Bitwise AND of packed double-precision bit patterns.
 *
 * Syntax: andpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   andpd xmm0, xmm1
 */
static inline void write_andpd(const char* dest, const char* src) {
    write_line3(ASM_ANDPD, dest, src);
}

/*
 * ORPS - Bitwise OR Packed Singles
 * Bitwise OR of packed single-precision bit patterns.
 *
 * Syntax: orps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   orps xmm0, xmm1
 */
static inline void write_orps(const char* dest, const char* src) {
    write_line3(ASM_ORPS, dest, src);
}

/*
 * ORPD - Bitwise OR Packed Doubles
 * Bitwise OR of packed double-precision bit patterns.
 *
 * Syntax: orpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   orpd xmm0, xmm1
 */
static inline void write_orpd(const char* dest, const char* src) {
    write_line3(ASM_ORPD, dest, src);
}

/*
 * XORPS - Bitwise XOR Packed Singles
 * Bitwise XOR of packed single-precision bit patterns.
 *
 * Syntax: xorps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   xorps xmm0, xmm1
 */
static inline void write_xorps(const char* dest, const char* src) {
    write_line3(ASM_XORPS, dest, src);
}

/*
 * XORPD - Bitwise XOR Packed Doubles
 * Bitwise XOR of packed double-precision bit patterns.
 *
 * Syntax: xorpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   xorpd xmm0, xmm1
 */
static inline void write_xorpd(const char* dest, const char* src) {
    write_line3(ASM_XORPD, dest, src);
}

/*
 * ANDNPS - Bitwise AND NOT Packed Singles
 * Bitwise AND of the inverted destination and the source for packed single data.
 *
 * Syntax: andnps xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   andnps xmm0, xmm1
 */
static inline void write_andnps(const char* dest, const char* src) {
    write_line3(ASM_ANDNPS, dest, src);
}

/*
 * ANDNPD - Bitwise AND NOT Packed Doubles
 * Bitwise AND of the inverted destination and the source for packed double data.
 *
 * Syntax: andnpd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source
 * Flags affected: None
 * Notes: These operate on raw bit patterns and are often used for masks, sign-bit control, and
 * blends.
 *
 * Example:
 *   andnpd xmm0, xmm1
 */
static inline void write_andnpd(const char* dest, const char* src) {
    write_line3(ASM_ANDNPD, dest, src);
}

/*
 * SHUFPS - Shuffle Packed Singles
 * Reorders packed single elements selected by an immediate control byte.
 *
 * Syntax: shufps xmm1, xmm2/m128, imm8
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   shufps xmm0, xmm1, 00011011b
 */
static inline void write_shufps(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_SHUFPS, dest, src, imm);
}

/*
 * SHUFPD - Shuffle Packed Doubles
 * Reorders packed double elements selected by an immediate control byte.
 *
 * Syntax: shufpd xmm1, xmm2/m128, imm8
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   shufpd xmm0, xmm1, 01b
 */
static inline void write_shufpd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_SHUFPD, dest, src, imm);
}

/*
 * UNPCKLPS - Unpack Low Packed Singles
 * Interleaves the low packed single elements from two operands.
 *
 * Syntax: unpcklps xmm1, xmm2/m128
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   unpcklps xmm0, xmm1
 */
static inline void write_unpcklps(const char* dest, const char* src) {
    write_line3(ASM_UNPCKLPS, dest, src);
}

/*
 * UNPCKHPS - Unpack High Packed Singles
 * Interleaves the high packed single elements from two operands.
 *
 * Syntax: unpckhps xmm1, xmm2/m128
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   unpckhps xmm0, xmm1
 */
static inline void write_unpckhps(const char* dest, const char* src) {
    write_line3(ASM_UNPCKHPS, dest, src);
}

/*
 * UNPCKLPD - Unpack Low Packed Doubles
 * Interleaves the low packed double elements from two operands.
 *
 * Syntax: unpcklpd xmm1, xmm2/m128
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   unpcklpd xmm0, xmm1
 */
static inline void write_unpcklpd(const char* dest, const char* src) {
    write_line3(ASM_UNPCKLPD, dest, src);
}

/*
 * UNPCKHPD - Unpack High Packed Doubles
 * Interleaves the high packed double elements from two operands.
 *
 * Syntax: unpckhpd xmm1, xmm2/m128
 * Operands: XMM registers and optional immediate control for shuffle forms
 * Flags affected: None
 * Notes: Used to permute, interleave, or replicate vector lanes without changing integer EFLAGS.
 *
 * Example:
 *   unpckhpd xmm0, xmm1
 */
static inline void write_unpckhpd(const char* dest, const char* src) {
    write_line3(ASM_UNPCKHPD, dest, src);
}

/*
 * CVTSI2SS - Convert Integer to Scalar Single
 * Converts a signed integer to the low scalar single-precision element.
 *
 * Syntax: cvtsi2ss dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtsi2ss xmm0, eax
 */
static inline void write_cvtsi2ss(const char* dest, const char* src) {
    write_line3(ASM_CVTSI2SS, dest, src);
}

/*
 * CVTSI2SD - Convert Integer to Scalar Double
 * Converts a signed integer to the low scalar double-precision element.
 *
 * Syntax: cvtsi2sd dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtsi2sd xmm0, eax
 */
static inline void write_cvtsi2sd(const char* dest, const char* src) {
    write_line3(ASM_CVTSI2SD, dest, src);
}

/*
 * CVTSS2SI - Convert Scalar Single to Integer
 * Converts the low scalar single-precision element to a signed integer using current rounding
 * mode.
 *
 * Syntax: cvtss2si dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtss2si eax, xmm0
 */
static inline void write_cvtss2si(const char* dest, const char* src) {
    write_line3(ASM_CVTSS2SI, dest, src);
}

/*
 * CVTSD2SI - Convert Scalar Double to Integer
 * Converts the low scalar double-precision element to a signed integer using current rounding
 * mode.
 *
 * Syntax: cvtsd2si dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtsd2si eax, xmm0
 */
static inline void write_cvtsd2si(const char* dest, const char* src) {
    write_line3(ASM_CVTSD2SI, dest, src);
}

/*
 * CVTTSS2SI - Convert Scalar Single to Integer with Truncation
 * Converts with truncation toward zero regardless of MXCSR rounding mode.
 *
 * Syntax: cvttss2si dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvttss2si eax, xmm0
 */
static inline void write_cvttss2si(const char* dest, const char* src) {
    write_line3(ASM_CVTTSS2SI, dest, src);
}

/*
 * CVTTSD2SI - Convert Scalar Double to Integer with Truncation
 * Converts with truncation toward zero regardless of MXCSR rounding mode.
 *
 * Syntax: cvttsd2si dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvttsd2si eax, xmm0
 */
static inline void write_cvttsd2si(const char* dest, const char* src) {
    write_line3(ASM_CVTTSD2SI, dest, src);
}

/*
 * CVTSS2SD - Convert Scalar Single to Scalar Double
 * Widens the low scalar single-precision element to scalar double-precision.
 *
 * Syntax: cvtss2sd dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtss2sd xmm0, xmm1
 */
static inline void write_cvtss2sd(const char* dest, const char* src) {
    write_line3(ASM_CVTSS2SD, dest, src);
}

/*
 * CVTSD2SS - Convert Scalar Double to Scalar Single
 * Narrows the low scalar double-precision element to scalar single-precision.
 *
 * Syntax: cvtsd2ss dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtsd2ss xmm0, xmm1
 */
static inline void write_cvtsd2ss(const char* dest, const char* src) {
    write_line3(ASM_CVTSD2SS, dest, src);
}

/*
 * CVTPS2PD - Convert Packed Singles to Packed Doubles
 * Widens packed single-precision values to packed doubles.
 *
 * Syntax: cvtps2pd dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtps2pd xmm0, xmm1
 */
static inline void write_cvtps2pd(const char* dest, const char* src) {
    write_line3(ASM_CVTPS2PD, dest, src);
}

/*
 * CVTPD2PS - Convert Packed Doubles to Packed Singles
 * Narrows packed double-precision values to packed singles.
 *
 * Syntax: cvtpd2ps dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtpd2ps xmm0, xmm1
 */
static inline void write_cvtpd2ps(const char* dest, const char* src) {
    write_line3(ASM_CVTPD2PS, dest, src);
}

/*
 * CVTPS2DQ - Convert Packed Singles to Packed Integers
 * Converts packed singles to signed dword integers using current rounding mode.
 *
 * Syntax: cvtps2dq dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtps2dq xmm0, xmm1
 */
static inline void write_cvtps2dq(const char* dest, const char* src) {
    write_line3(ASM_CVTPS2DQ, dest, src);
}

/*
 * CVTDQ2PS - Convert Packed Integers to Packed Singles
 * Converts packed signed dword integers to packed single-precision values.
 *
 * Syntax: cvtdq2ps dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtdq2ps xmm0, xmm1
 */
static inline void write_cvtdq2ps(const char* dest, const char* src) {
    write_line3(ASM_CVTDQ2PS, dest, src);
}

/*
 * CVTPD2DQ - Convert Packed Doubles to Packed Integers
 * Converts packed doubles to signed dword integers.
 *
 * Syntax: cvtpd2dq dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtpd2dq xmm0, xmm1
 */
static inline void write_cvtpd2dq(const char* dest, const char* src) {
    write_line3(ASM_CVTPD2DQ, dest, src);
}

/*
 * CVTDQ2PD - Convert Packed Integers to Packed Doubles
 * Converts packed signed dword integers to packed doubles.
 *
 * Syntax: cvtdq2pd dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvtdq2pd xmm0, xmm1
 */
static inline void write_cvtdq2pd(const char* dest, const char* src) {
    write_line3(ASM_CVTDQ2PD, dest, src);
}

/*
 * CVTTPD2DQ - Convert Packed Doubles to Packed Integers with Truncation
 * Converts packed doubles to signed dword integers using truncation.
 *
 * Syntax: cvttpd2dq dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvttpd2dq xmm0, xmm1
 */
static inline void write_cvttpd2dq(const char* dest, const char* src) {
    write_line3(ASM_CVTTPD2DQ, dest, src);
}

/*
 * CVTTPS2DQ - Convert Packed Singles to Packed Integers with Truncation
 * Converts packed singles to signed dword integers using truncation.
 *
 * Syntax: cvttps2dq dst, src
 * Operands: GPR/XMM destination and XMM/register/memory source according to the specific
 * conversion
 * Flags affected: None
 * Notes: Conversion instructions use MXCSR rounding mode except the CVTT* truncation forms, and
 * they raise SIMD floating-point exceptions rather than changing integer flags.
 *
 * Example:
 *   cvttps2dq xmm0, xmm1
 */
static inline void write_cvttps2dq(const char* dest, const char* src) {
    write_line3(ASM_CVTTPS2DQ, dest, src);
}

/*
 * PAND - Packed Bitwise AND
 * Performs bitwise AND on packed integer data.
 *
 * Syntax: pand xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pand xmm0, xmm1
 */
static inline void write_pand(const char* dest, const char* src) {
    write_line3(ASM_PAND, dest, src);
}

/*
 * PANDN - Packed Bitwise AND NOT
 * Performs bitwise AND of the inverted destination and the source on packed integer data.
 *
 * Syntax: pandn xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pandn xmm0, xmm1
 */
static inline void write_pandn(const char* dest, const char* src) {
    write_line3(ASM_PANDN, dest, src);
}

/*
 * POR - Packed Bitwise OR
 * Performs bitwise OR on packed integer data.
 *
 * Syntax: por xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   por xmm0, xmm1
 */
static inline void write_por(const char* dest, const char* src) {
    write_line3(ASM_POR, dest, src);
}

/*
 * PXOR - Packed Bitwise XOR
 * Performs bitwise XOR on packed integer data.
 *
 * Syntax: pxor xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pxor xmm0, xmm1
 */
static inline void write_pxor(const char* dest, const char* src) {
    write_line3(ASM_PXOR, dest, src);
}

/*
 * PADDB - Packed Add Bytes
 * Adds packed 8-bit integer lanes.
 *
 * Syntax: paddb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   paddb xmm0, xmm1
 */
static inline void write_paddb(const char* dest, const char* src) {
    write_line3(ASM_PADDB, dest, src);
}

/*
 * PADDW - Packed Add Words
 * Adds packed 16-bit integer lanes.
 *
 * Syntax: paddw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   paddw xmm0, xmm1
 */
static inline void write_paddw(const char* dest, const char* src) {
    write_line3(ASM_PADDW, dest, src);
}

/*
 * PADDD - Packed Add Doublewords
 * Adds packed 32-bit integer lanes.
 *
 * Syntax: paddd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   paddd xmm0, xmm1
 */
static inline void write_paddd(const char* dest, const char* src) {
    write_line3(ASM_PADDD, dest, src);
}

/*
 * PADDQ - Packed Add Quadwords
 * Adds packed 64-bit integer lanes.
 *
 * Syntax: paddq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   paddq xmm0, xmm1
 */
static inline void write_paddq(const char* dest, const char* src) {
    write_line3(ASM_PADDQ, dest, src);
}

/*
 * PSUBB - Packed Subtract Bytes
 * Subtracts packed 8-bit integer lanes.
 *
 * Syntax: psubb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psubb xmm0, xmm1
 */
static inline void write_psubb(const char* dest, const char* src) {
    write_line3(ASM_PSUBB, dest, src);
}

/*
 * PSUBW - Packed Subtract Words
 * Subtracts packed 16-bit integer lanes.
 *
 * Syntax: psubw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psubw xmm0, xmm1
 */
static inline void write_psubw(const char* dest, const char* src) {
    write_line3(ASM_PSUBW, dest, src);
}

/*
 * PSUBD - Packed Subtract Doublewords
 * Subtracts packed 32-bit integer lanes.
 *
 * Syntax: psubd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psubd xmm0, xmm1
 */
static inline void write_psubd(const char* dest, const char* src) {
    write_line3(ASM_PSUBD, dest, src);
}

/*
 * PSUBQ - Packed Subtract Quadwords
 * Subtracts packed 64-bit integer lanes.
 *
 * Syntax: psubq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psubq xmm0, xmm1
 */
static inline void write_psubq(const char* dest, const char* src) {
    write_line3(ASM_PSUBQ, dest, src);
}

/*
 * PMULLW - Packed Multiply Low Words
 * Multiplies packed signed 16-bit integers and keeps the low 16 bits of each product.
 *
 * Syntax: pmullw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pmullw xmm0, xmm1
 */
static inline void write_pmullw(const char* dest, const char* src) {
    write_line3(ASM_PMULLW, dest, src);
}

/*
 * PMULLD - Packed Multiply Doublewords
 * Multiplies packed signed 32-bit integers and keeps the low 32 bits of each product.
 *
 * Syntax: pmulld xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pmulld xmm0, xmm1
 */
static inline void write_pmulld(const char* dest, const char* src) {
    write_line3(ASM_PMULLD, dest, src);
}

/*
 * PMULHW - Packed Multiply High Words
 * Multiplies packed signed 16-bit integers and keeps the high 16 bits of each product.
 *
 * Syntax: pmulhw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pmulhw xmm0, xmm1
 */
static inline void write_pmulhw(const char* dest, const char* src) {
    write_line3(ASM_PMULHW, dest, src);
}

/*
 * PMULUDQ - Packed Multiply Unsigned Doubleword to Quadword
 * Multiplies selected unsigned 32-bit lanes producing 64-bit products.
 *
 * Syntax: pmuludq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pmuludq xmm0, xmm1
 */
static inline void write_pmuludq(const char* dest, const char* src) {
    write_line3(ASM_PMULUDQ, dest, src);
}

/*
 * PSLLW - Packed Shift Left Logical Words
 * Shifts packed 16-bit lanes left logically.
 *
 * Syntax: psllw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psllw xmm0, xmm1
 */
static inline void write_psllw(const char* dest, const char* src) {
    write_line3(ASM_PSLLW, dest, src);
}

/*
 * PSLLD - Packed Shift Left Logical Doublewords
 * Shifts packed 32-bit lanes left logically.
 *
 * Syntax: pslld xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pslld xmm0, xmm1
 */
static inline void write_pslld(const char* dest, const char* src) {
    write_line3(ASM_PSLLD, dest, src);
}

/*
 * PSLLQ - Packed Shift Left Logical Quadwords
 * Shifts packed 64-bit lanes left logically.
 *
 * Syntax: psllq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psllq xmm0, xmm1
 */
static inline void write_psllq(const char* dest, const char* src) {
    write_line3(ASM_PSLLQ, dest, src);
}

/*
 * PSRLW - Packed Shift Right Logical Words
 * Shifts packed 16-bit lanes right logically.
 *
 * Syntax: psrlw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psrlw xmm0, xmm1
 */
static inline void write_psrlw(const char* dest, const char* src) {
    write_line3(ASM_PSRLW, dest, src);
}

/*
 * PSRLD - Packed Shift Right Logical Doublewords
 * Shifts packed 32-bit lanes right logically.
 *
 * Syntax: psrld xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psrld xmm0, xmm1
 */
static inline void write_psrld(const char* dest, const char* src) {
    write_line3(ASM_PSRLD, dest, src);
}

/*
 * PSRLQ - Packed Shift Right Logical Quadwords
 * Shifts packed 64-bit lanes right logically.
 *
 * Syntax: psrlq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psrlq xmm0, xmm1
 */
static inline void write_psrlq(const char* dest, const char* src) {
    write_line3(ASM_PSRLQ, dest, src);
}

/*
 * PSRAW - Packed Shift Right Arithmetic Words
 * Shifts packed signed 16-bit lanes right arithmetically.
 *
 * Syntax: psraw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psraw xmm0, xmm1
 */
static inline void write_psraw(const char* dest, const char* src) {
    write_line3(ASM_PSRAW, dest, src);
}

/*
 * PSRAD - Packed Shift Right Arithmetic Doublewords
 * Shifts packed signed 32-bit lanes right arithmetically.
 *
 * Syntax: psrad xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   psrad xmm0, xmm1
 */
static inline void write_psrad(const char* dest, const char* src) {
    write_line3(ASM_PSRAD, dest, src);
}

/*
 * PCMPEQB - Packed Compare Equal Bytes
 * Compares packed byte lanes for equality and produces all-ones or all-zeros masks.
 *
 * Syntax: pcmpeqb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpeqb xmm0, xmm1
 */
static inline void write_pcmpeqb(const char* dest, const char* src) {
    write_line3(ASM_PCMPEQB, dest, src);
}

/*
 * PCMPEQW - Packed Compare Equal Words
 * Compares packed word lanes for equality and produces masks.
 *
 * Syntax: pcmpeqw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpeqw xmm0, xmm1
 */
static inline void write_pcmpeqw(const char* dest, const char* src) {
    write_line3(ASM_PCMPEQW, dest, src);
}

/*
 * PCMPEQD - Packed Compare Equal Doublewords
 * Compares packed dword lanes for equality and produces masks.
 *
 * Syntax: pcmpeqd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpeqd xmm0, xmm1
 */
static inline void write_pcmpeqd(const char* dest, const char* src) {
    write_line3(ASM_PCMPEQD, dest, src);
}

/*
 * PCMPGTB - Packed Compare Greater Than Bytes
 * Performs signed packed byte greater-than comparisons.
 *
 * Syntax: pcmpgtb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpgtb xmm0, xmm1
 */
static inline void write_pcmpgtb(const char* dest, const char* src) {
    write_line3(ASM_PCMPGTB, dest, src);
}

/*
 * PCMPGTW - Packed Compare Greater Than Words
 * Performs signed packed word greater-than comparisons.
 *
 * Syntax: pcmpgtw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpgtw xmm0, xmm1
 */
static inline void write_pcmpgtw(const char* dest, const char* src) {
    write_line3(ASM_PCMPGTW, dest, src);
}

/*
 * PCMPGTD - Packed Compare Greater Than Doublewords
 * Performs signed packed dword greater-than comparisons.
 *
 * Syntax: pcmpgtd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pcmpgtd xmm0, xmm1
 */
static inline void write_pcmpgtd(const char* dest, const char* src) {
    write_line3(ASM_PCMPGTD, dest, src);
}

/*
 * PACKSSWB - Pack with Signed Saturation Word to Byte
 * Packs signed word lanes to bytes using signed saturation.
 *
 * Syntax: packsswb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   packsswb xmm0, xmm1
 */
static inline void write_packsswb(const char* dest, const char* src) {
    write_line3(ASM_PACKSSWB, dest, src);
}

/*
 * PACKSSDW - Pack with Signed Saturation Doubleword to Word
 * Packs signed dword lanes to words using signed saturation.
 *
 * Syntax: packssdw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   packssdw xmm0, xmm1
 */
static inline void write_packssdw(const char* dest, const char* src) {
    write_line3(ASM_PACKSSDW, dest, src);
}

/*
 * PACKUSWB - Pack with Unsigned Saturation Word to Byte
 * Packs signed or unsigned word lanes to bytes using unsigned saturation.
 *
 * Syntax: packuswb xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   packuswb xmm0, xmm1
 */
static inline void write_packuswb(const char* dest, const char* src) {
    write_line3(ASM_PACKUSWB, dest, src);
}

/*
 * PUNPCKLBW - Unpack Low Bytes
 * Interleaves the low packed byte lanes from two operands.
 *
 * Syntax: punpcklbw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpcklbw xmm0, xmm1
 */
static inline void write_punpcklbw(const char* dest, const char* src) {
    write_line3(ASM_PUNPCKLBW, dest, src);
}

/*
 * PUNPCKLWD - Unpack Low Words
 * Interleaves the low packed word lanes from two operands.
 *
 * Syntax: punpcklwd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpcklwd xmm0, xmm1
 */
static inline void write_punpcklwd(const char* dest, const char* src) {
    write_line3(ASM_PUNPCKLWD, dest, src);
}

/*
 * PUNPCKLDQ - Unpack Low Doublewords
 * Interleaves the low packed dword lanes from two operands.
 *
 * Syntax: punpckldq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckldq xmm0, xmm1
 */
static inline void write_punpckldq(const char* dest, const char* src) {
    write_line3(ASM_PUNPCKLDQ, dest, src);
}

/*
 * PUNPCKLQDQ - Unpack Low Quadwords
 * Interleaves the low qword lanes from two XMM operands.
 *
 * Syntax: punpcklqdq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpcklqdq xmm0, xmm1
 */
static inline void write_punpcklqdq(const char* dest, const char* src) {
    write_line3(ASM_PUNPCKLQDQ, dest, src);
}

/*
 * PUNPCKHBW - Unpack High Bytes
 * Interleaves the high packed byte lanes from two operands.
 *
 * Syntax: punpckhbw xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckhbw xmm0, xmm1
 */
static inline void write_punpckhbw(const char* dest, const char* src) {
    write_line3(ASM_PUNPCKHBW, dest, src);
}

/*
 * PUNPCKHWD - Unpack High Words
 * Interleaves the high packed word lanes from two operands.
 *
 * Syntax: punpckhwd xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckhwd xmm0, xmm1
 */
static inline void write_punpckhwd(const char* dest, const char* src) {
    write_line3(ASM_PUNPCKHWD, dest, src);
}

/*
 * PUNPCKHDQ - Unpack High Doublewords
 * Interleaves the high packed dword lanes from two operands.
 *
 * Syntax: punpckhdq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckhdq xmm0, xmm1
 */
static inline void write_punpckhdq(const char* dest, const char* src) {
    write_line3(ASM_PUNPCKHDQ, dest, src);
}

/*
 * PUNPCKHQDQ - Unpack High Quadwords
 * Interleaves the high qword lanes from two XMM operands.
 *
 * Syntax: punpckhqdq xmm1, xmm2/m128
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   punpckhqdq xmm0, xmm1
 */
static inline void write_punpckhqdq(const char* dest, const char* src) {
    write_line3(ASM_PUNPCKHQDQ, dest, src);
}

/*
 * PSHUFD - Shuffle Packed Doublewords
 * Reorders packed dword lanes based on an immediate control.
 *
 * Syntax: pshufd xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pshufd xmm0, xmm1
 */
static inline void write_pshufd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_PSHUFD, dest, src, imm);
}

/*
 * PSHUFHW - Shuffle High Words
 * Reorders the high packed word lanes of an XMM register based on an immediate control.
 *
 * Syntax: pshufhw xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pshufhw xmm0, xmm1
 */
static inline void write_pshufhw(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_PSHUFHW, dest, src, imm);
}

/*
 * PSHUFLW - Shuffle Low Words
 * Reorders the low packed word lanes of an XMM register based on an immediate control.
 *
 * Syntax: pshuflw xmm1, xmm2/m128, imm8
 * Operands: XMM destination/source and XMM or memory source, with optional immediate
 * count/control
 * Flags affected: None
 * Notes: These packed integer SSE2 instructions operate lane-wise and do not modify integer
 * EFLAGS.
 *
 * Example:
 *   pshuflw xmm0, xmm1
 */
static inline void write_pshuflw(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_PSHUFLW, dest, src, imm);
}

/*
 * MOVQ_SSE - Move Quadword (SSE/MMX-style)
 * Moves a 64-bit quantity between XMM registers, MMX registers, and/or memory depending on the
 * encoding. This symbol names the SIMD move form rather than any unrelated mnemonic reuse.
 *
 * Syntax: movq xmm0, qword ptr [rax]
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   movq xmm0, qword ptr [rax]
 */
static inline void write_movq_sse(const char* dest, const char* src) {
    write_line3(ASM_MOVQ_SSE, dest, src);
}

/*
 * MOVD_SSE - Move Doubleword (SSE/MMX-style)
 * Moves a 32-bit integer value between a general-purpose register, memory, and an XMM register.
 * This symbol names the SIMD move form rather than any unrelated mnemonic reuse.
 *
 * Syntax: movd xmm0, eax
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   movd xmm0, eax
 */
static inline void write_movd_sse(const char* dest, const char* src) {
    write_line3(ASM_MOVD_SSE, dest, src);
}

/*
 * PEXTRW - Extract Word
 * Extracts a selected 16-bit word from an XMM/MMX register into a general-purpose register or
 * memory.
 *
 * Syntax: pextrw eax, xmm0, 2
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   pextrw eax, xmm0, 2
 */
static inline void write_pextrw(const char* dest, const char* src) {
    write_line3(ASM_PEXTRW, dest, src);
}

/*
 * PINSRW - Insert Word
 * Inserts a 16-bit word from a general-purpose register or memory into a selected lane of an
 * XMM/MMX register.
 *
 * Syntax: pinsrw xmm0, eax, 2
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   pinsrw xmm0, eax, 2
 */
static inline void write_pinsrw(const char* dest, const char* src) {
    write_line3(ASM_PINSRW, dest, src);
}

/*
 * PMOVMSKB - Move Byte Mask
 * Collects the most-significant bit of each byte lane into a general-purpose register bitmask.
 *
 * Syntax: pmovmskb eax, xmm0
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   pmovmskb eax, xmm0
 */
static inline void write_pmovmskb(const char* dest, const char* src) {
    write_line3(ASM_PMOVMSKB, dest, src);
}

/*
 * MASKMOVDQU - Masked Store Bytes
 * Stores selected bytes from an XMM register to memory using the sign bits of a mask XMM
 * register.
 *
 * Syntax: maskmovdqu xmm0, xmm1
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   maskmovdqu xmm0, xmm1
 */
static inline void write_maskmovdqu(const char* dest, const char* src) {
    write_line3(ASM_MASKMOVDQU, dest, src);
}

/*
 * LDMXCSR - Load MXCSR
 * Loads the SIMD floating-point control/status register from memory.
 *
 * Syntax: ldmxcsr dword ptr [mxcsr]
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   ldmxcsr dword ptr [mxcsr]
 */
static inline void write_ldmxcsr(const char* operand) {
    write_line2(ASM_LDMXCSR, operand);
}

/*
 * STMXCSR - Store MXCSR
 * Stores the SIMD floating-point control/status register to memory.
 *
 * Syntax: stmxcsr dword ptr [mxcsr]
 * Operands: GPR, XMM, MMX, and/or memory operands according to the specific form
 * Flags affected: None
 * Notes: These instructions move or manage SIMD state without using ordinary integer status
 * flags.
 *
 * Example:
 *   stmxcsr dword ptr [mxcsr]
 */
static inline void write_stmxcsr(const char* operand) {
    write_line2(ASM_STMXCSR, operand);
}

/*
 * ============================================================================
 * 20. AVX Instructions
 * ============================================================================
 */

/*
 * VADDPS - Vector Add Packed Singles
 * Adds packed single-precision lanes in the three-operand AVX form.
 *
 * Syntax: vaddps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vaddps ymm0, ymm1, ymm2
 */
static inline void write_vaddps(const char* dest, const char* src) {
    write_line3(ASM_VADDPS, dest, src);
}

/*
 * VADDPD - Vector Add Packed Doubles
 * Adds packed double-precision lanes in the three-operand AVX form.
 *
 * Syntax: vaddpd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vaddpd ymm0, ymm1, ymm2
 */
static inline void write_vaddpd(const char* dest, const char* src) {
    write_line3(ASM_VADDPD, dest, src);
}

/*
 * VSUBPS - Vector Subtract Packed Singles
 * Subtracts packed single-precision lanes in the three-operand AVX form.
 *
 * Syntax: vsubps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vsubps ymm0, ymm1, ymm2
 */
static inline void write_vsubps(const char* dest, const char* src) {
    write_line3(ASM_VSUBPS, dest, src);
}

/*
 * VSUBPD - Vector Subtract Packed Doubles
 * Subtracts packed double-precision lanes in the three-operand AVX form.
 *
 * Syntax: vsubpd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vsubpd ymm0, ymm1, ymm2
 */
static inline void write_vsubpd(const char* dest, const char* src) {
    write_line3(ASM_VSUBPD, dest, src);
}

/*
 * VMULPS - Vector Multiply Packed Singles
 * Multiplies packed single-precision lanes in the three-operand AVX form.
 *
 * Syntax: vmulps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmulps ymm0, ymm1, ymm2
 */
static inline void write_vmulps(const char* dest, const char* src) {
    write_line3(ASM_VMULPS, dest, src);
}

/*
 * VMULPD - Vector Multiply Packed Doubles
 * Multiplies packed double-precision lanes in the three-operand AVX form.
 *
 * Syntax: vmulpd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmulpd ymm0, ymm1, ymm2
 */
static inline void write_vmulpd(const char* dest, const char* src) {
    write_line3(ASM_VMULPD, dest, src);
}

/*
 * VDIVPS - Vector Divide Packed Singles
 * Divides packed single-precision lanes in the three-operand AVX form.
 *
 * Syntax: vdivps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vdivps ymm0, ymm1, ymm2
 */
static inline void write_vdivps(const char* dest, const char* src) {
    write_line3(ASM_VDIVPS, dest, src);
}

/*
 * VDIVPD - Vector Divide Packed Doubles
 * Divides packed double-precision lanes in the three-operand AVX form.
 *
 * Syntax: vdivpd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vdivpd ymm0, ymm1, ymm2
 */
static inline void write_vdivpd(const char* dest, const char* src) {
    write_line3(ASM_VDIVPD, dest, src);
}

/*
 * VMOVAPS - Vector Move Aligned Packed Singles
 * Moves aligned packed single data using AVX encodings.
 *
 * Syntax: vmovaps ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovaps ymm0, ymm1, ymm2
 */
static inline void write_vmovaps(const char* dest, const char* src) {
    write_line3(ASM_VMOVAPS, dest, src);
}

/*
 * VMOVAPD - Vector Move Aligned Packed Doubles
 * Moves aligned packed double data using AVX encodings.
 *
 * Syntax: vmovapd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovapd ymm0, ymm1, ymm2
 */
static inline void write_vmovapd(const char* dest, const char* src) {
    write_line3(ASM_VMOVAPD, dest, src);
}

/*
 * VMOVUPS - Vector Move Unaligned Packed Singles
 * Moves unaligned packed single data using AVX encodings.
 *
 * Syntax: vmovups ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovups ymm0, ymm1, ymm2
 */
static inline void write_vmovups(const char* dest, const char* src) {
    write_line3(ASM_VMOVUPS, dest, src);
}

/*
 * VMOVUPD - Vector Move Unaligned Packed Doubles
 * Moves unaligned packed double data using AVX encodings.
 *
 * Syntax: vmovupd ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovupd ymm0, ymm1, ymm2
 */
static inline void write_vmovupd(const char* dest, const char* src) {
    write_line3(ASM_VMOVUPD, dest, src);
}

/*
 * VMOVDQA - Vector Move Aligned Integer Data
 * Moves aligned integer vector data using AVX encodings.
 *
 * Syntax: vmovdqa ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovdqa ymm0, ymm1, ymm2
 */
static inline void write_vmovdqa(const char* dest, const char* src) {
    write_line3(ASM_VMOVDQA, dest, src);
}

/*
 * VMOVDQU - Vector Move Unaligned Integer Data
 * Moves unaligned integer vector data using AVX encodings.
 *
 * Syntax: vmovdqu ymm1, ymm2, ymm3/m256
 * Operands: three-operand AVX vector registers and/or memory operand as appropriate
 * Flags affected: None
 * Notes: AVX instructions use VEX encoding, avoid destructive two-operand overwrites, and
 * generally do not update integer EFLAGS.
 *
 * Example:
 *   vmovdqu ymm0, ymm1, ymm2
 */
static inline void write_vmovdqu(const char* dest, const char* src) {
    write_line3(ASM_VMOVDQU, dest, src);
}

/*
 * VBROADCASTSS - Broadcast Scalar Single
 * Loads one scalar single-precision value and broadcasts it to all vector lanes.
 *
 * Syntax: vbroadcastss ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vbroadcastss ymm0, dword ptr [rax]
 */
static inline void write_vbroadcastss(const char* dest, const char* src) {
    write_line3(ASM_VBROADCASTSS, dest, src);
}

/*
 * VBROADCASTSD - Broadcast Scalar Double
 * Loads one scalar double-precision value and broadcasts it to all vector lanes.
 *
 * Syntax: vbroadcastsd ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vbroadcastsd ymm0, qword ptr [rax]
 */
static inline void write_vbroadcastsd(const char* dest, const char* src) {
    write_line3(ASM_VBROADCASTSD, dest, src);
}

/*
 * VBROADCASTF128 - Broadcast 128-bit Lane
 * Loads one 128-bit lane from memory and replicates it across a YMM destination.
 *
 * Syntax: vbroadcastf128 ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vbroadcastf128 ymm0, xmmword ptr [rax]
 */
static inline void write_vbroadcastf128(const char* dest, const char* src) {
    write_line3(ASM_VBROADCASTF128, dest, src);
}

/*
 * VPERM2F128 - Permute 128-bit Lanes
 * Builds a new YMM register from selected 128-bit lanes of two YMM sources under immediate
 * control.
 *
 * Syntax: vperm2f128 ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vperm2f128 ymm0, ymm1, ymm2, 00100000b
 */
static inline void write_vperm2f128(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VPERM2F128, dest, src, imm);
}

/*
 * VINSERTF128 - Insert 128-bit Lane
 * Inserts a 128-bit XMM source or memory lane into a selected half of a YMM destination.
 *
 * Syntax: vinsertf128 ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vinsertf128 ymm0, ymm1, xmm2, 1
 */
static inline void write_vinsertf128(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VINSERTF128, dest, src, imm);
}

/*
 * VEXTRACTF128 - Extract 128-bit Lane
 * Extracts a selected 128-bit lane from a YMM source into an XMM register or memory.
 *
 * Syntax: vextractf128 ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vextractf128 xmm0, ymm1, 1
 */
static inline void write_vextractf128(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VEXTRACTF128, dest, src, imm);
}

/*
 * VZEROALL - Zero All XMM/YMM State
 * Clears all architectural XMM/YMM registers to zero.
 *
 * Syntax: vzeroall ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vzeroall
 */
static inline void write_vzeroall(void) {
    write_line(ASM_VZEROALL);
}

/*
 * VZEROUPPER - Zero Upper Halves
 * Clears the upper 128 bits of all YMM registers to avoid AVX/SSE transition penalties.
 *
 * Syntax: vzeroupper ...
 * Operands: vector registers, memory operands, and immediates according to the specific
 * instruction
 * Flags affected: None
 * Notes: These AVX instructions manage lane movement, broadcast, or state cleanup and are common
 * in optimized vector code.
 *
 * Example:
 *   vzeroupper
 */
static inline void write_vzeroupper(void) {
    write_line(ASM_VZEROUPPER);
}

/*
 * VFMADD132PS - Fused Multiply-Add Packed Singles 132
 * Computes a fused multiply-add on packed single-precision lanes using the 132 operand ordering.
 *
 * Syntax: vfmadd132ps dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd132ps ymm0, ymm1, ymm2
 */
static inline void write_vfmadd132ps(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD132PS, dest, src, imm);
}

/*
 * VFMADD213PS - Fused Multiply-Add Packed Singles 213
 * Computes a fused multiply-add on packed single-precision lanes using the 213 operand ordering.
 *
 * Syntax: vfmadd213ps dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd213ps ymm0, ymm1, ymm2
 */
static inline void write_vfmadd213ps(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD213PS, dest, src, imm);
}

/*
 * VFMADD231PS - Fused Multiply-Add Packed Singles 231
 * Computes a fused multiply-add on packed single-precision lanes using the 231 operand ordering.
 *
 * Syntax: vfmadd231ps dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd231ps ymm0, ymm1, ymm2
 */
static inline void write_vfmadd231ps(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD231PS, dest, src, imm);
}

/*
 * VFMADD132PD - Fused Multiply-Add Packed Doubles 132
 * Computes a fused multiply-add on packed double-precision lanes using the 132 operand ordering.
 *
 * Syntax: vfmadd132pd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd132pd ymm0, ymm1, ymm2
 */
static inline void write_vfmadd132pd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD132PD, dest, src, imm);
}

/*
 * VFMADD213PD - Fused Multiply-Add Packed Doubles 213
 * Computes a fused multiply-add on packed double-precision lanes using the 213 operand ordering.
 *
 * Syntax: vfmadd213pd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd213pd ymm0, ymm1, ymm2
 */
static inline void write_vfmadd213pd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD213PD, dest, src, imm);
}

/*
 * VFMADD231PD - Fused Multiply-Add Packed Doubles 231
 * Computes a fused multiply-add on packed double-precision lanes using the 231 operand ordering.
 *
 * Syntax: vfmadd231pd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd231pd ymm0, ymm1, ymm2
 */
static inline void write_vfmadd231pd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD231PD, dest, src, imm);
}

/*
 * VFMADD132SS - Fused Multiply-Add Scalar Single 132
 * Computes a fused multiply-add on the low scalar single-precision lane using the 132 operand
 * ordering.
 *
 * Syntax: vfmadd132ss dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd132ss ymm0, ymm1, ymm2
 */
static inline void write_vfmadd132ss(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD132SS, dest, src, imm);
}

/*
 * VFMADD213SS - Fused Multiply-Add Scalar Single 213
 * Computes a fused multiply-add on the low scalar single-precision lane using the 213 operand
 * ordering.
 *
 * Syntax: vfmadd213ss dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd213ss ymm0, ymm1, ymm2
 */
static inline void write_vfmadd213ss(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD213SS, dest, src, imm);
}

/*
 * VFMADD231SS - Fused Multiply-Add Scalar Single 231
 * Computes a fused multiply-add on the low scalar single-precision lane using the 231 operand
 * ordering.
 *
 * Syntax: vfmadd231ss dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd231ss ymm0, ymm1, ymm2
 */
static inline void write_vfmadd231ss(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD231SS, dest, src, imm);
}

/*
 * VFMADD132SD - Fused Multiply-Add Scalar Double 132
 * Computes a fused multiply-add on the low scalar double-precision lane using the 132 operand
 * ordering.
 *
 * Syntax: vfmadd132sd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd132sd ymm0, ymm1, ymm2
 */
static inline void write_vfmadd132sd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD132SD, dest, src, imm);
}

/*
 * VFMADD213SD - Fused Multiply-Add Scalar Double 213
 * Computes a fused multiply-add on the low scalar double-precision lane using the 213 operand
 * ordering.
 *
 * Syntax: vfmadd213sd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd213sd ymm0, ymm1, ymm2
 */
static inline void write_vfmadd213sd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD213SD, dest, src, imm);
}

/*
 * VFMADD231SD - Fused Multiply-Add Scalar Double 231
 * Computes a fused multiply-add on the low scalar double-precision lane using the 231 operand
 * ordering.
 *
 * Syntax: vfmadd231sd dst, src1, src2
 * Operands: vector destination plus two vector/memory sources
 * Flags affected: None
 * Notes: FMA instructions perform multiplication and addition with a single final rounding step,
 * improving both speed and numerical precision relative to separate MUL+ADD sequences.
 *
 * Example:
 *   vfmadd231sd ymm0, ymm1, ymm2
 */
static inline void write_vfmadd231sd(const char* dest, const char* src, const char* imm) {
    write_line4(ASM_VFMADD231SD, dest, src, imm);
}

/*
 * ============================================================================
 * 21. MASM Directives and Keywords
 * ============================================================================
 */

/*
 * MODEL - .MODEL Directive
 * Selects the memory model, language convention, and stack distance assumptions for traditional
 * 16-bit or 32-bit MASM programs.
 *
 * Syntax: .MODEL model [, language] [, stacktype]
 * Common usage: Used near the top of a source file before segment declarations and procedure
 * definitions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .model flat, stdcall
 */
static inline void write_model(const char* dest, const char* src) {
    write_line3(ASM_MODEL, dest, src);
}

/*
 * STACK - .STACK Directive
 * Declares the stack segment and optionally its size in simplified segment models.
 *
 * Syntax: .STACK [size]
 * Common usage: Common in small sample programs that rely on MASM's simplified directives.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .stack 4096
 */
static inline void write_stack(const char* operand) {
    write_line2(ASM_STACK, operand);
}

/*
 * DATA - .DATA Directive
 * Begins the initialized data segment in simplified segment syntax.
 *
 * Syntax: .DATA
 * Common usage: Place DB/DW/DD/DQ definitions here for writable initialized data.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .data
 */
/* 0 none, 1 .data, 2 .data?, 3 .code, 4 .const. A repeated directive for
 * the section already open is skipped. MASM merges repeated sections, and
 * a second copy of the same directive changes nothing a reader uses.
 * asm_proc_depth counts open PROC bodies. A data helper that would reopen
 * .data from inside a procedure must not: the bytes would land in the
 * procedure. write_proc / write_endp maintain the depth. */
static int asm_sec = 0;
static int asm_proc_depth = 0;

static inline void write_data(void) {
    if (asm_sec == 1) return;
    write_line(ASM_DATA);
    asm_sec = 1;
}

/*
 * DATAQ - .DATA? Directive
 * Begins the uninitialized data segment used for BSS-style storage.
 *
 * Syntax: .DATA?
 * Common usage: Variables placed here reserve space without contributing initialized bytes to
 * the image file.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .data?
 */
static inline void write_dataq(void) {
    if (asm_sec == 2) return;
    write_line(ASM_DATAQ);
    asm_sec = 2;
}

/*
 * CONST - .CONST Directive
 * Begins a read-only or constant data segment in simplified segment syntax.
 *
 * Syntax: .CONST
 * Common usage: Use for lookup tables, literal scalars, and other immutable data.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .const
 */
static inline void write_const(void) {
    if (asm_sec == 4) return;
    write_line(ASM_CONST);
    asm_sec = 4;
}

/*
 * CODE - .CODE Directive
 * Begins the code segment in simplified segment syntax.
 *
 * Syntax: .CODE
 * Common usage: Executable instructions, PROC definitions, and labels are commonly placed here.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .code
 */
static inline void write_code(void) {
    if (asm_sec == 3) return;
    write_line(ASM_CODE);
    asm_sec = 3;
}

/*
 * STARTUP - .STARTUP Directive
 * Emits simplified-language startup code and defines the program entry point in older MASM
 * program models.
 *
 * Syntax: .STARTUP
 * Common usage: Typically paired with .EXIT in small examples rather than hand-written CRT
 * startup code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .startup
 */
static inline void write_startup(void) {
    write_line(ASM_STARTUP);
}

/*
 * EXIT - .EXIT Directive
 * Emits simplified-language program termination code, optionally with a return code.
 *
 * Syntax: .EXIT [expression]
 * Common usage: Used in educational or tiny-model samples instead of explicit OS API calls.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .exit 0
 */
static inline void write_exit(void) {
    write_line(ASM_EXIT);
}

/*
 * IF - .IF High-Level Directive
 * Starts a MASM high-level conditional block whose condition is evaluated at run time and
 * lowered into compare/jump code.
 *
 * Syntax: .IF condition
 * Common usage: Pairs with .ELSE/.ELSEIF/.ENDIF to write structured assembly control flow.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .if eax == 0
 */
static inline void write_if(const char* dest, const char* src) {
    write_line3(ASM_IF, dest, src);
}

/*
 * ELSE - .ELSE High-Level Directive
 * Begins the alternative block of a preceding .IF or .ELSEIF.
 *
 * Syntax: .ELSE
 * Common usage: Used in MASM high-level run-time flow blocks.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .else
 */
static inline void write_else(void) {
    write_line(ASM_ELSE);
}

/*
 * ELSEIF - .ELSEIF High-Level Directive
 * Adds another conditional branch to a MASM high-level .IF block.
 *
 * Syntax: .ELSEIF condition
 * Common usage: MASM expands it into additional compare/jump logic.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .elseif eax > 0
 */
static inline void write_elseif(const char* dest, const char* src) {
    write_line3(ASM_ELSEIF, dest, src);
}

/*
 * ENDIF - .ENDIF High-Level Directive
 * Terminates a MASM high-level .IF block.
 *
 * Syntax: .ENDIF
 * Common usage: Closes .IF/.ELSEIF/.ELSE structures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .endif
 */
static inline void write_endif(void) {
    write_line(ASM_ENDIF);
}

/*
 * WHILE - .WHILE High-Level Directive
 * Starts a MASM high-level loop that evaluates a run-time condition before each iteration.
 *
 * Syntax: .WHILE condition
 * Common usage: Pairs with .ENDW for structured loops.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .while ecx != 0
 */
static inline void write_while(const char* dest, const char* src) {
    write_line3(ASM_WHILE, dest, src);
}

/*
 * ENDW - .ENDW High-Level Directive
 * Terminates a MASM high-level .WHILE block.
 *
 * Syntax: .ENDW
 * Common usage: Closes a structured .WHILE loop.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .endw
 */
static inline void write_endw(void) {
    write_line(ASM_ENDW);
}

/*
 * REPEAT - .REPEAT High-Level Directive
 * Starts a MASM high-level repeat-until loop body.
 *
 * Syntax: .REPEAT
 * Common usage: Pairs with .UNTIL to test the condition at the end of the loop.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .repeat
 */
static inline void write_repeat(const char* dest, const char* src) {
    write_line3(ASM_REPEAT, dest, src);
}

/*
 * UNTIL - .UNTIL High-Level Directive
 * Ends a .REPEAT block and provides the terminating run-time condition.
 *
 * Syntax: .UNTIL condition
 * Common usage: Used for do-while style loops.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .until eax == 0
 */
static inline void write_until(const char* dest, const char* src) {
    write_line3(ASM_UNTIL, dest, src);
}

/*
 * BREAK - .BREAK High-Level Directive
 * Exits the nearest MASM high-level loop immediately.
 *
 * Syntax: .BREAK [ .IF condition ]
 * Common usage: Used inside .WHILE or .REPEAT blocks for structured early exits.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .break .if ecx == 0
 */
static inline void write_break(void) {
    write_line(ASM_BREAK);
}

/*
 * CONTINUE - .CONTINUE High-Level Directive
 * Skips to the next iteration of the nearest MASM high-level loop.
 *
 * Syntax: .CONTINUE [ .IF condition ]
 * Common usage: Used inside .WHILE or .REPEAT blocks for structured loop control.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .continue .if eax < 0
 */
static inline void write_continue(void) {
    write_line(ASM_CONTINUE);
}

/*
 * 386 - .386 Directive
 * Enables 80386 instruction set syntax and operand forms.
 *
 * Syntax: .386
 * Common usage: Placed near the top of the file to declare the minimum required CPU.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .386
 */
static inline void write_386(void) {
    write_line(ASM_386);
}

/*
 * 486 - .486 Directive
 * Enables 80486 instruction set syntax and scheduling assumptions.
 *
 * Syntax: .486
 * Common usage: Declares a 486-or-later target.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .486
 */
static inline void write_486(void) {
    write_line(ASM_486);
}

/*
 * 586 - .586 Directive
 * Enables Pentium-era instructions.
 *
 * Syntax: .586
 * Common usage: Declares a 586-or-later target.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .586
 */
static inline void write_586(void) {
    write_line(ASM_586);
}

/*
 * 686 - .686 Directive
 * Enables Pentium Pro and later baseline instructions.
 *
 * Syntax: .686
 * Common usage: Often used for 32-bit flat-model code that expects conditional moves and other
 * newer instructions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .686
 */
static inline void write_686(void) {
    write_line(ASM_686);
}

/*
 * MMX - .MMX Directive
 * Enables MMX instruction syntax.
 *
 * Syntax: .MMX
 * Common usage: Placed before MMX instruction use if the selected CPU level does not already
 * imply support.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .mmx
 */
static inline void write_mmx(void) {
    write_line(ASM_MMX);
}

/*
 * XMM - .XMM Directive
 * Enables SSE/XMM instruction syntax.
 *
 * Syntax: .XMM
 * Common usage: Required by some MASM configurations before using XMM registers and SSE
 * mnemonics.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .xmm
 */
static inline void write_xmm(void) {
    write_line(ASM_XMM);
}

/*
 * OPTION - OPTION Directive
 * Sets assembler options such as case mapping, proc visibility rules, language defaults, and
 * expression behavior.
 *
 * Syntax: OPTION option-list
 * Common usage: Used to control MASM parsing and code-generation policy for the current module.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   option casemap:none
 */
static inline void write_option(const char* operand) {
    write_line2(ASM_OPTION, operand);
}

/*
 * write_set_case_sensitive_identifiers - Emit "option casemap:none".
 *
 * Wraps write_option to make MASM treat identifiers as case-sensitive and keep
 * symbol names exactly as written, so mixed-case externals (e.g. ExitProcess,
 * CreateFileA) resolve correctly at link time.
 */
static inline void write_set_case_sensitive_identifiers(void) {
    write_option("casemap:none");
}

/*
 * INCLUDE - INCLUDE Directive
 * Textually includes another source file at assembly time.
 *
 * Syntax: INCLUDE filename
 * Common usage: Used for shared macro files, prototypes, and constants.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   include windows.inc
 */
static inline void write_include(const char* operand) {
    write_line2(ASM_INCLUDE, operand);
}

/*
 * INCLUDELIB - INCLUDELIB Directive
 * Adds a library name to the linker input list from within source code.
 *
 * Syntax: INCLUDELIB libraryname
 * Common usage: Convenient for samples that want self-contained linker dependencies.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   includelib kernel32.lib
 */
static inline void write_includelib(const char* operand) {
    write_line2(ASM_INCLUDELIB, operand);
}

/*
 * EXTERN - EXTERN Directive
 * Declares an imported function using the configured DLL-call spelling.
 *
 * Default syntax: EXTERN __imp_symbol:qword
 * Fallback syntax: EXTERN symbol:type
 * Common usage: The default calls directly through the DLL import-address table;
 * define ASM_USE_VISUAL_C_DLL_CALLS=0 for conventional MASM procedure symbols.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   extern __imp_printf:qword
 */
/*
 * write_extern - Emit the configured DLL EXTERN declaration and record its
 * calling-convention metadata for later use by write_extern_call.
 *
 * out: caller-owned AsmExtern that this fills in. The caller owns its storage
 *   and must keep it alive for as long as it is used (i.e. through all matching
 *   write_call sites); pass its address to write_call and write_extern_call.
 *
 * params: array of (location, type) cells, one per parameter — params[i].name is
 *   where the i-th argument is placed and params[i].type is its declared type.
 *   A register name (e.g. "rcx") means the argument travels in that register; a
 *   memory slot (a "[...]" reference such as "qword ptr [rsp+32]") means it is
 *   written to that caller-side stack slot. The number of register parameters is
 *   derived from this array (the leading run of register-named cells). The
 *   AsmExtern retains this pointer as-is (it is NOT copied), so the array must
 *   remain valid for as long as the AsmExtern is used. Do not pass a
 *   temporary/automatic array (such as a compound literal) that goes out of scope
 *   before then; use storage with a matching lifetime.
 *
 * return_reg: (register, type) cell describing where the result is returned, or
 *   a zeroed cell ({0}) when the routine has no result. Unlike params, this cell
 *   is copied by value into the AsmExtern, so it need not outlive it.
 */
static inline void write_extern(AsmExtern* out, const char* name, const char* type,
                                const AsmDataField* params,
                                int param_count,
                                AsmDataField return_reg) {
    for (int i = 0; i < asm_indent_level; i++) {
        write("    ");
    }
    write(ASM_EXTERN);
#if ASM_USE_VISUAL_C_DLL_CALLS
    write(" __imp_");
    write(name);
    write(":qword\n");
#else
    write(" ");
    write(name);
    write(":");
    write(type);
    write("\n");
#endif
    out->name = name;
    out->params = asm_copy_call_params(params, param_count);
    out->param_count = param_count;
    out->return_reg = return_reg;
    out->is_dll_import = 1;
    out->volatile_mask = ASM_WIN64_VOLATILE;
}

/*
 * write_common_extern - Convenience wrapper for common x64 Windows integer parameter functions.
 * 
 * Declares an external function with x64 Windows integer calling convention defaults:
 *   - type: "proc"
 *   - register parameters: the first four params (those whose cell carries a
 *     register name); any beyond the fourth are stack parameters
 *   - return_reg: supplied by the caller (the register/type the result comes back
 *     in, e.g. {"rax", "hFile"} for a HANDLE, or a zeroed cell {0} for a void
 *     function)
 *
 * The caller supplies params as an array of (location, type) cells using the x64
 * Windows registers rcx, rdx, r8, r9 for the first four parameters and the
 * caller-side stack slots "qword ptr [rsp+32]", "[rsp+40]", ... (in order) for
 * any parameters beyond the fourth.
 *
 * params follows the same lifetime contract as write_extern: pass a named array
 * (or NULL when param_count is 0) that outlives every matching write_call, not a
 * temporary/compound-literal array. return_reg is copied by value, so a compound
 * literal (e.g. (AsmDataField){"rax", "hFile"}) is fine.
 *
 * Returns the AsmExtern describing the function by value; store it in a caller-
 * owned local (which outlives every matching write_call) and pass its address to
 * write_extern_call / write_call. After each call the result can be consumed via
 * ext->return_reg.
 *
 * Example:
 *   static const AsmDataField params[] = {{"rcx", "exit_code"}};
 *   AsmExtern exit_process = write_common_extern("ExitProcess", params, 1, (AsmDataField){0});
 */
static inline AsmExtern write_common_extern(const char* name,
                                            const AsmDataField* params,
                                            int param_count,
                                            AsmDataField return_reg) {
    AsmExtern out;
    write_extern(&out, name, ASM_PROC, params, param_count, return_reg);
    return out;
}

/*
 * asm_win64_params - standard Windows x64 slots for an ordered tag list.
 *
 * Parameter 0..3 land in rcx, rdx, r8, r9. Later parameters land in
 * qword ptr [rsp+40], [rsp+48], ... which is the slot write_extern_call
 * both stores and the callee reads (it biases rsp by 8). The array is
 * copied by write_extern, so the caller does not keep it. A private
 * convention such as log_write still passes its own AsmDataField list
 * and does not call this.
 */
static inline AsmDataField* asm_win64_params(const char** tags, int count) {
    static const char* regs[4] = { "rcx", "rdx", "r8", "r9" };
    AsmDataField* p = (AsmDataField*)asm_bump_alloc(sizeof(AsmDataField) * (size_t)count);
    for (int i = 0; i < count; i++) {
        if (i < 4) {
            p[i].name = regs[i];
        } else {
            char buf[40];
            snprintf(buf, sizeof buf, "qword ptr [rsp+%d]", 40 + (i - 4) * 8);
            p[i].name = asm_strdup(buf);
        }
        p[i].type = tags[i];
        p[i].use_as = NULL;
    }
    return p;
}

/*
 * EXTERNDEF - EXTERNDEF Directive
 * Declares a symbol that may be defined locally or resolved externally.
 *
 * Syntax: EXTERNDEF symbol:type
 * Common usage: Useful in headers shared by defining and consuming modules.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   externdef gValue:dword
 */
static inline void write_externdef(const char* operand) {
    write_line2(ASM_EXTERNDEF, operand);
}

/*
 * PUBLIC - PUBLIC Directive
 * Exports a symbol from the current module so the linker can resolve it from other modules.
 *
 * Syntax: PUBLIC symbol[, symbol]...
 * Common usage: Applied to procedures, labels, and data that should have external linkage.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   public MyProc
 */
static inline void write_public(const char* operand) {
    write_line2(ASM_PUBLIC, operand);
}

/*
 * PROTO - PROTO Directive
 * Declares a procedure prototype with parameter types and calling convention metadata.
 *
 * Syntax: name PROTO [params]
 * Common usage: Used with INVOKE and for documentation of external/internal procedures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MessageBoxA PROTO :QWORD, :QWORD, :QWORD, :DWORD
 */
static inline void write_proto(const char* operand) {
    write_line2(ASM_PROTO, operand);
}

/*
 * INVOKE - INVOKE Directive
 * Calls a procedure using its prototype so MASM can marshal arguments according to the procedure
 * declaration.
 *
 * Syntax: INVOKE procname, arg1, arg2, ...
 * Common usage: Useful in 32-bit MASM with high-level procedure declarations and calling
 * conventions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   invoke ExitProcess, 0
 */
static inline void write_invoke(const char* operand) {
    write_line2(ASM_INVOKE, operand);
}

/*
 * PROC - PROC Directive
 * Begins a procedure definition and associates calling convention, visibility, parameter, and
 * local-variable metadata with a label.
 *
 * Syntax: name PROC [attributes] [USES reglist] [, params]
 * Common usage: Pairs with ENDP and can emit prologue/epilogue code depending on options and
 * model.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyProc PROC USES ebx esi, value:DWORD
 */
/*
 * write_proc - Begin a procedure definition and record its calling-convention
 * metadata for later use by write_call.
 *
 * Emits "name proc" and returns an AsmExtern describing how the procedure is
 * called: a params array of (register, type) cells, one per parameter, plus the
 * return register. Each params[i].name is the register a parameter arrives in
 * and params[i].type is its declared type, so write_call validates and places
 * each argument from this single array (the count of register parameters is
 * derived from the array). Because the cells already record the incoming
 * register types, no assert_type calls are needed after write_proc. Store the
 * returned value in a caller-owned local and pass its address to write_call at
 * each call site and to write_endp to close the procedure. This mirrors
 * write_extern, except the symbol is defined here rather than imported, so no
 * x64 ABI scaffolding is implied — the recorded convention can be any private
 * register-based convention you choose.
 *
 * The returned AsmExtern is caller-owned: keep the local it is stored in alive
 * for as long as it is used (through all matching write_call sites and the
 * write_endp).
 *
 * params follows the same lifetime contract as write_extern's params: pass a
 * named array (or NULL when param_count is 0) that outlives every matching
 * write_call, not a temporary/compound-literal array.
 *
 * return_reg: (register, type) cell for the result, or a zeroed cell ({0}) when
 *   the procedure returns nothing. Copied by value into the AsmExtern.
 *
 * Example:
 *   AsmDataField rcx_src = {"rcx", "src"};
 *   AsmDataField rdx_len = {"rdx", "len"};
 *   const AsmDataField params[] = {rcx_src, rdx_len};
 *   AsmExtern p = write_proc("log_write", params, 2, (AsmDataField){0});
 *   ...
 *   write_endp(&p);
 */
static inline AsmExtern write_proc(const char* name,
                                   const AsmDataField* params,
                                   int param_count,
                                   AsmDataField return_reg) {
    if (asm_sec != 3) write_code();
    asm_proc_depth++;
    write_line2(name, ASM_PROC);
    asm_indent_level++;
    asm_reset_rsp_align();
    asm_proc_preserved = 0;
    asm_proc_saved = 0;
    /* Start type validation fresh for this body, seeded with the parameter
     * locations (both register and stack slots) and their declared types. */
    asm_type_dict_clear();
    asm_current_return_reg = return_reg;
    AsmDataField* owned = asm_copy_call_params(params, param_count);
    for (int i = 0; i < param_count; i++) {
        if (owned != NULL && owned[i].name != NULL) {
            asm_type_dict_set(owned[i].name, owned[i].type);
        }
    }
    AsmExtern out;
    out.name = name;
    out.params = owned;
    out.param_count = param_count;
    out.return_reg = return_reg;
    out.is_dll_import = 0;
    out.volatile_mask = 0;
    /* The process entry has no __chkstk. Commit the stack before the body
     * runs. Other procedures sit inside that committed region, or they are
     * thread entries and commit for themselves. */
    if (name != NULL && strcmp(name, "main") == 0)
        write_commit_stack(ASM_STACK_PROBE_STEPS);
    return out;
}

/*
 * ENDP - ENDP Directive
 * Terminates a procedure definition started with PROC.
 *
 * Syntax: name ENDP
 * Common usage: Used to close the lexical scope of a MASM procedure.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyProc ENDP
 */
static inline void write_endp(const AsmExtern* proc) {
    asm_indent_level--;
    if (asm_proc_depth > 0) asm_proc_depth--;
    write_line2(proc->name, ASM_ENDP);
    /* The procedure's locations go out of scope; drop their tracked types. */
    asm_type_dict_clear();
    asm_current_return_reg = asm_void;
}

/*
 * MACRO - MACRO Directive
 * Begins a macro definition with named parameters and optional defaults.
 *
 * Syntax: name MACRO [params]
 * Common usage: Pairs with ENDM to define reusable source expansions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyMacro MACRO arg
 */
static inline void write_macro(const char* operand) {
    write_line2(ASM_MACRO, operand);
}

/*
 * ENDM - ENDM Directive
 * Terminates a macro definition started with MACRO.
 *
 * Syntax: ENDM
 * Common usage: Closes the macro body.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ENDM
 */
static inline void write_endm(void) {
    write_line(ASM_ENDM);
}

/*
 * LOCAL - LOCAL Directive
 * Declares local macro variables inside a MACRO or stack-based local variables inside a PROC
 * depending on context.
 *
 * Syntax: LOCAL name[:type] [, name[:type]]...
 * Common usage: In procedures it reserves stack storage; in macros it creates unique text
 * symbols.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   LOCAL buffer[64]:BYTE
 */
static inline void write_local(const char* operand) {
    write_line2(ASM_LOCAL, operand);
}

/*
 * EQU - EQU Directive
 * Assigns a constant value or text replacement to a symbol.
 *
 * Syntax: name EQU expression
 * Common usage: Creates symbolic constants used throughout a module.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   BUF_SIZE EQU 256
 */
static inline void write_equ(const char* dest, const char* src) {
    write_line_def(dest, ASM_EQU, src);
}

/*
 * write_constant - Define an assemble-time constant and record its tag.
 *
 * Wraps write_equ to emit "<name> equ <expr>" (no memory is allocated; the
 * assembler substitutes the value wherever the symbol appears) and returns an
 * AsmDataField holding the symbol's name and its type tag, so later references
 * can use out.name as the location and out.type as the type tag instead of
 * repeating the literals. For example,
 * AsmDataField started_len = write_constant("msgStartedLen", "$ - msgStarted",
 * "len") emits "msgStartedLen equ $ - msgStarted" and lets callers write
 * write_call(..., started_len).
 */
static inline AsmDataField write_constant(const char* name,
                                          const char* expr, const char* type) {
    write_equ(name, expr);
    asm_global_dict_add(name, type);
    AsmDataField out = {name, type};
    return out;
}

/*
 * write_offset_constant - Define a constant equal to the address of a previously
 * declared symbol, e.g. "offset_logName equ offset logName", and record its tag.
 *
 * Unlike write_constant("offset_logName", "offset logName", ...), the address
 * expression is not passed as a pre-built string: the "offset " directive is a
 * static token and the operand is taken straight from field.name, so no memory
 * is allocated for a combined "offset <name>" string and the reference stays
 * tied to the original declaration (reusing the AsmDataField returned by, say,
 * write_initialised_data rather than repeating its label literal).
 *
 * field.name must be a bare label (as returned by write_initialised_data /
 * write_unitialised_data / write_constant), not a sized memory operand such as
 * "qword ptr foo" (an "offset qword ptr foo" would be invalid).
 *
 * Returns an AsmDataField holding the constant's symbol name and type tag for
 * later reuse (as an operand / call argument), like write_constant.
 */
static inline AsmDataField write_offset_constant(const char* name,
                                                 AsmDataField field,
                                                 const char* type) {
    for (int i = 0; i < asm_indent_level; i++) {
        write("    ");
    }
    write(name);
    write(" ");
    write(ASM_EQU);
    write(" offset ");
    write(field.name);
    write("\n");
    asm_global_dict_add(name, type);
    AsmDataField out = {name, type};
    return out;
}

/*
 * TEXTEQU - TEXTEQU Directive
 * Assigns a literal text substitution to a symbol.
 *
 * Syntax: name TEXTEQU <text>
 * Common usage: Useful in macros and token-level metaprogramming.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   CRLF TEXTEQU <13,10>
 */
static inline void write_textequ(const char* dest, const char* src) {
    write_line3(ASM_TEXTEQU, dest, src);
}

/*
 * CATSTR - CATSTR Operator
 * Concatenates text strings during assembly-time macro processing.
 *
 * Syntax: symbol CATSTR text1, text2, ...
 * Common usage: Used to build identifiers or string literals in macros.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   name CATSTR <pre>, <fix>
 */
static inline void write_catstr(const char* dest, const char* src) {
    write_line3(ASM_CATSTR, dest, src);
}

/*
 * SUBSTR - SUBSTR Operator
 * Extracts a substring from a text macro string during assembly-time processing.
 *
 * Syntax: symbol SUBSTR text, position [, length]
 * Common usage: Used in advanced macro metaprogramming.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   piece SUBSTR <abcdef>, 2, 3
 */
static inline void write_substr(const char* dest, const char* src) {
    write_line3(ASM_SUBSTR, dest, src);
}

/*
 * SIZESTR - SIZESTR Operator
 * Returns the length of a text string at assembly time.
 *
 * Syntax: SIZESTR text
 * Common usage: Useful for macro-time validation and code generation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   IF SIZESTR(<abc>) EQ 3
 */
static inline void write_sizestr(const char* dest, const char* src) {
    write_line3(ASM_SIZESTR, dest, src);
}

/*
 * INSTR - INSTR Operator
 * Searches for one string inside another during assembly-time text processing.
 *
 * Syntax: INSTR [start,] text, substr
 * Common usage: Used in macros for token inspection and conditional expansion.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   pos = INSTR(1, <abcdef>, <cd>)
 */
static inline void write_instr(const char* dest, const char* src) {
    write_line3(ASM_INSTR, dest, src);
}

/*
 * SEGMENT - SEGMENT Directive
 * Begins an explicit segment definition.
 *
 * Syntax: name SEGMENT [attributes]
 * Common usage: Used instead of simplified .CODE/.DATA style in more manual MASM source layouts.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   _TEXT SEGMENT PARA PUBLIC 'CODE'
 */
static inline void write_segment(const char* operand) {
    write_line2(ASM_SEGMENT, operand);
}

/*
 * ENDS - ENDS Directive
 * Terminates a segment definition started with SEGMENT.
 *
 * Syntax: name ENDS
 * Common usage: Closes an explicit segment block.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   _TEXT ENDS
 */
static inline void write_ends(void) {
    write_line(ASM_ENDS);
}

/*
 * ASSUME - ASSUME Directive
 * Informs MASM which segment registers or type assumptions apply to given symbols and registers.
 *
 * Syntax: ASSUME register:segmentOrType [, ...]
 * Common usage: Affects assembler checking and default segment selection but does not emit
 * runtime code by itself.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   assume cs:_TEXT, ds:_DATA
 */
static inline void write_assume(const char* operand) {
    write_line2(ASM_ASSUME, operand);
}

/*
 * ORG - ORG Directive
 * Sets the current location counter within the current segment or structure.
 *
 * Syntax: ORG expression
 * Common usage: Used in binary layouts, boot sectors, structures, and manually controlled memory
 * maps.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   org 7C00h
 */
static inline void write_org(const char* operand) {
    write_line2(ASM_ORG, operand);
}

/*
 * ALIGN - ALIGN Directive
 * Advances the location counter to the requested power-of-two boundary, emitting fill bytes as
 * needed.
 *
 * Syntax: ALIGN n
 * Common usage: Used for code and data alignment to improve performance or satisfy ABI
 * requirements.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   align 16
 */
static inline void write_align(const char* operand) {
    write_line2(ASM_ALIGN, operand);
}

/*
 * EVEN - EVEN Directive
 * Aligns the location counter to an even address boundary.
 *
 * Syntax: EVEN
 * Common usage: Historical shorthand for ALIGN 2.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   even
 */
static inline void write_even(void) {
    write_line(ASM_EVEN);
}

/*
 * END - END Directive
 * Marks the logical end of the source module and may specify the entry label.
 *
 * Syntax: END [entryLabel]
 * Common usage: Required to terminate a MASM source file cleanly.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   end main
 */
static inline void write_end(const char* operand) {
    write_line2(ASM_END, operand);
}

/*
 * Flush the accumulated assembly (every token passed to write()) to `filename`.
 * Each *_main.c ends by calling this instead of repeating the same open / loop /
 * close boilerplate. Prints a success or failure line and returns a process exit
 * code (0 on success, 1 on failure) so a generator can `return write_asm_file(name)`.
 */
static inline int write_asm_file(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("Failed to open %s for writing\n", filename);
        return 1;
    }
    for (int i = 0; i < asm_output_cursor; i++) {
        fprintf(f, "%s", asm_output[i]);
    }
    fclose(f);
    printf("Generated %s\n", filename);
    return 0;
}

/*
 * TITLE - TITLE Directive
 * Supplies a title string for listings and diagnostics.
 *
 * Syntax: TITLE text
 * Common usage: Affects assembler output metadata rather than machine code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   title Sample Program
 */
static inline void write_title(const char* operand) {
    write_line2(ASM_TITLE, operand);
}

/*
 * NAME_DIR - NAME Directive
 * Supplies a module name for object-module metadata in older toolchains.
 *
 * Syntax: NAME modulename
 * Common usage: Mostly historical but still recognized by MASM.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   name mymodule
 */
static inline void write_name_dir(const char* operand) {
    write_line2(ASM_NAME_DIR, operand);
}

/*
 * PAGE - PAGE Directive
 * Controls page width and length in generated listing files.
 *
 * Syntax: PAGE [length][, width]
 * Common usage: Affects listings only, not emitted machine code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   page 60, 132
 */
static inline void write_page(const char* operand) {
    write_line2(ASM_PAGE, operand);
}

/*
 * Line comment. Emitted as "; text", not the COMMENT directive.
 *
 * COMMENT takes its first character as a delimiter and hides every following
 * line until that character appears again. A sentence such as
 * "mark it, then walk" therefore deletes the next mov. A semicolon comment
 * ends at the newline, so the next instruction is assembled.
 */
static inline void write_comment(const char* operand) {
    write_line2(";", operand);
}

/*
 * ECHO_DIR - ECHO Directive
 * Emits a message to the assembler output during assembly.
 *
 * Syntax: ECHO text
 * Common usage: Useful for macro debugging and conditional-build diagnostics.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   echo Building debug configuration
 */
static inline void write_echo_dir(const char* operand) {
    write_line2(ASM_ECHO_DIR, operand);
}

/*
 * ERR - .ERR Directive
 * Forces the assembler to emit an error unconditionally.
 *
 * Syntax: .ERR [text]
 * Common usage: Used in macro-time assertions and unsupported-configuration checks.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .err <unsupported configuration>
 */
static inline void write_err(const char* operand) {
    write_line2(ASM_ERR, operand);
}

/*
 * ERRE - .ERRE Directive
 * Emits an error if an expression is true or nonzero.
 *
 * Syntax: .ERRE expression
 * Common usage: Used for compile-time assertions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .erre SIZEOF table EQ 0
 */
static inline void write_erre(const char* operand) {
    write_line2(ASM_ERRE, operand);
}

/*
 * ERRNZ - .ERRNZ Directive
 * Emits an error if an expression is not zero.
 *
 * Syntax: .ERRNZ expression
 * Common usage: Common for structure-size and offset validation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errnz ($ - start) - 16
 */
static inline void write_errnz(const char* operand) {
    write_line2(ASM_ERRNZ, operand);
}

/*
 * ERRNDEF - .ERRNDEF Directive
 * Emits an error if the named symbol is not defined.
 *
 * Syntax: .ERRNDEF symbol
 * Common usage: Used for include guards and dependency checks in assembly-time logic.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errndef MyRequiredSymbol
 */
static inline void write_errndef(const char* operand) {
    write_line2(ASM_ERRNDEF, operand);
}

/*
 * ERRDEF - .ERRDEF Directive
 * Emits an error if the named symbol is defined.
 *
 * Syntax: .ERRDEF symbol
 * Common usage: Useful for collision detection and single-definition enforcement.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errdef OldMacro
 */
static inline void write_errdef(const char* operand) {
    write_line2(ASM_ERRDEF, operand);
}

/*
 * ERRB - .ERRB Directive
 * Emits an error if a text argument is blank.
 *
 * Syntax: .ERRB <text>
 * Common usage: Used in macro parameter validation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errb <&arg>
 */
static inline void write_errb(const char* operand) {
    write_line2(ASM_ERRB, operand);
}

/*
 * ERRNB - .ERRNB Directive
 * Emits an error if a text argument is not blank.
 *
 * Syntax: .ERRNB <text>
 * Common usage: Used in mutually exclusive macro forms.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errnb <&arg>
 */
static inline void write_errnb(const char* operand) {
    write_line2(ASM_ERRNB, operand);
}

/*
 * ERRDIF - .ERRDIF Directive
 * Emits an error if two text arguments differ.
 *
 * Syntax: .ERRDIF <text1>, <text2>
 * Common usage: Used in macro-time string validation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .errdif <eax>, <&reg>
 */
static inline void write_errdif(const char* operand) {
    write_line2(ASM_ERRDIF, operand);
}

/*
 * ERRIDN - .ERRIDN Directive
 * Emits an error if two text arguments are identical.
 *
 * Syntax: .ERRIDN <text1>, <text2>
 * Common usage: Used to reject forbidden macro arguments.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .erridn <esp>, <&reg>
 */
static inline void write_erridn(const char* operand) {
    write_line2(ASM_ERRIDN, operand);
}

/*
 * IF_DIR - IF Directive
 * Begins an assembly-time conditional block evaluated by the assembler rather than by emitted
 * runtime code.
 *
 * Syntax: IF expression
 * Common usage: Pairs with ELSE/ELSEIF/ENDIF for conditional assembly.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   if DEBUG
 */
static inline void write_if_dir(const char* dest, const char* src) {
    write_line3(ASM_IF_DIR, dest, src);
}

/*
 * IFE - IFE Directive
 * Begins an assembly-time conditional block taken when the expression equals zero.
 *
 * Syntax: IFE expression
 * Common usage: Used for inverse conditional assembly tests.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ife DEBUG
 */
static inline void write_ife(const char* dest, const char* src) {
    write_line3(ASM_IFE, dest, src);
}

/*
 * IFDEF - IFDEF Directive
 * Begins an assembly-time block if the named symbol is defined.
 *
 * Syntax: IFDEF symbol
 * Common usage: Common in include guards and feature toggles.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifdef UNICODE
 */
static inline void write_ifdef(const char* operand) {
    write_line2(ASM_IFDEF, operand);
}

/*
 * IFNDEF - IFNDEF Directive
 * Begins an assembly-time block if the named symbol is not defined.
 *
 * Syntax: IFNDEF symbol
 * Common usage: Common in include guards and default-definition logic.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifndef MY_HEADER_GUARD
 */
static inline void write_ifndef(const char* operand) {
    write_line2(ASM_IFNDEF, operand);
}

/*
 * IFB - IFB Directive
 * Begins an assembly-time block if a text argument is blank.
 *
 * Syntax: IFB <text>
 * Common usage: Used in macro parameter processing.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifb <&arg>
 */
static inline void write_ifb(const char* operand) {
    write_line2(ASM_IFB, operand);
}

/*
 * IFNB - IFNB Directive
 * Begins an assembly-time block if a text argument is not blank.
 *
 * Syntax: IFNB <text>
 * Common usage: Used in macro parameter processing.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifnb <&arg>
 */
static inline void write_ifnb(const char* operand) {
    write_line2(ASM_IFNB, operand);
}

/*
 * IFDIF - IFDIF Directive
 * Begins an assembly-time block if two text arguments differ.
 *
 * Syntax: IFDIF <text1>, <text2>
 * Common usage: Useful in macro-time string comparisons.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifdif <eax>, <&reg>
 */
static inline void write_ifdif(const char* dest, const char* src) {
    write_line3(ASM_IFDIF, dest, src);
}

/*
 * IFIDN - IFIDN Directive
 * Begins an assembly-time block if two text arguments are identical.
 *
 * Syntax: IFIDN <text1>, <text2>
 * Common usage: Useful in macro-time string comparisons and overload-like dispatch.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   ifidn <eax>, <&reg>
 */
static inline void write_ifidn(const char* dest, const char* src) {
    write_line3(ASM_IFIDN, dest, src);
}

/*
 * ELSE_DIR - ELSE Directive
 * Assembly-time alternative branch used with IF/IFE/IFDEF and related directives.
 *
 * Syntax: ELSE
 * Common usage: Used in conditional assembly, distinct from the run-time .ELSE directive.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   else
 */
static inline void write_else_dir(const char* operand) {
    write_line2(ASM_ELSE_DIR, operand);
}

/*
 * ELSEIF_DIR - ELSEIF Directive
 * Assembly-time else-if branch used with IF and related directives.
 *
 * Syntax: ELSEIF expression
 * Common usage: Lets conditional assembly choose among multiple cases.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   elseif RELEASE
 */
static inline void write_elseif_dir(const char* dest, const char* src) {
    write_line3(ASM_ELSEIF_DIR, dest, src);
}

/*
 * ENDIF_DIR - ENDIF Directive
 * Terminates an assembly-time conditional block.
 *
 * Syntax: ENDIF
 * Common usage: Closes IF/IFE/IFDEF and related directives.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   endif
 */
static inline void write_endif_dir(const char* operand) {
    write_line2(ASM_ENDIF_DIR, operand);
}

/*
 * REPEAT_DIR - REPEAT Directive
 * Repeats a block of source text a fixed number of times at assembly time.
 *
 * Syntax: REPEAT count
 * Common usage: Pairs with ENDM to generate repeated code or data definitions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   repeat 4
 */
static inline void write_repeat_dir(const char* dest, const char* src) {
    write_line3(ASM_REPEAT_DIR, dest, src);
}

/*
 * FOR - FOR Directive
 * Iterates over a comma-separated argument list during macro expansion.
 *
 * Syntax: FOR param, <list>
 * Common usage: Useful for macro-generated boilerplate and table definitions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   for reg, <eax, ebx, ecx>
 */
static inline void write_for(const char* dest, const char* src) {
    write_line3(ASM_FOR, dest, src);
}

/*
 * FORC - FORC Directive
 * Iterates over the characters of a text string during macro expansion.
 *
 * Syntax: FORC ch, <text>
 * Common usage: Useful for text processing macros.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   forc ch, <ABC>
 */
static inline void write_forc(const char* dest, const char* src) {
    write_line3(ASM_FORC, dest, src);
}

/*
 * WHILE_DIR - WHILE Directive
 * Repeats a block of source text while an assembly-time expression remains true.
 *
 * Syntax: WHILE expression
 * Common usage: Pairs with ENDM for macro-time loops.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   while index LT 10
 */
static inline void write_while_dir(const char* dest, const char* src) {
    write_line3(ASM_WHILE_DIR, dest, src);
}

/*
 * EXITM - EXITM Directive
 * Terminates macro expansion immediately and optionally returns text.
 *
 * Syntax: EXITM [<text>]
 * Common usage: Used for macro-time control flow and early returns.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   exitm <eax>
 */
static inline void write_exitm(void) {
    write_line(ASM_EXITM);
}

/*
 * GOTO - GOTO Directive
 * Transfers assembly-time control to a macro label within the current macro body.
 *
 * Syntax: GOTO label
 * Common usage: Used sparingly in complex macros that need nonstructured expansion flow.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   goto retry
 */
static inline void write_goto(const char* operand) {
    write_line2(ASM_GOTO, operand);
}

/*
 * STRUCT - STRUCT Directive
 * Begins a structure type definition with named fields and offsets.
 *
 * Syntax: name STRUCT [alignment]
 * Common usage: Pairs with ENDS or an implicit terminator depending on style to define aggregate
 * layouts.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   POINT STRUCT
 */
static inline void write_struct(const char* operand) {
    write_line2(ASM_STRUCT, operand);
}

/*
 * UNION - UNION Directive
 * Begins a union type definition whose members overlap at offset zero.
 *
 * Syntax: name UNION
 * Common usage: Used for type punning or alternate views of the same storage.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   VALUE UNION
 */
static inline void write_union(const char* operand) {
    write_line2(ASM_UNION, operand);
}

/*
 * TYPEDEF - TYPEDEF Directive
 * Creates a type alias for another MASM type expression.
 *
 * Syntax: name TYPEDEF type
 * Common usage: Used to simplify prototypes and data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   PBYTE TYPEDEF PTR BYTE
 */
static inline void write_typedef(const char* operand) {
    write_line2(ASM_TYPEDEF, operand);
}

/*
 * RECORD - RECORD Directive
 * Defines a packed bitfield record type with named fields and widths.
 *
 * Syntax: name RECORD field:width [, field:width]...
 * Common usage: Useful for symbolic masks and packed control values.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   FLAGS RECORD carry:1, zero:1, sign:1
 */
static inline void write_record(const char* dest, const char* src) {
    write_line3(ASM_RECORD, dest, src);
}

/*
 * LABEL - LABEL Directive
 * Associates a type with the current location counter without allocating storage.
 *
 * Syntax: name LABEL type
 * Common usage: Useful for giving multiple typed views to the same address.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value LABEL DWORD
 */
static inline void write_label_directive(const char* operand) {
    write_line2(ASM_LABEL, operand);
}

/*
 * DB - Define Byte
 * Allocates and optionally initializes one or more byte-sized objects.
 *
 * Syntax: name DB initializer[, initializer]...
 * Common usage: Used for strings, raw bytes, and byte arrays.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   msg db 'Hi',0
 */
static inline void write_db(const char* dest, const char* src) {
    write_line_def(dest, ASM_DB, src);
}

/*
 * DW - Define Word
 * Allocates and optionally initializes 16-bit objects.
 *
 * Syntax: name DW initializer[, initializer]...
 * Common usage: Used for 16-bit integers and pointer components.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value dw 1234h
 */
static inline void write_dw(const char* dest, const char* src) {
    write_line_def(dest, ASM_DW, src);
}

/*
 * DD - Define Doubleword
 * Allocates and optionally initializes 32-bit objects.
 *
 * Syntax: name DD initializer[, initializer]...
 * Common usage: Used for 32-bit integers, addresses, and single-precision floats.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value dd 12345678h
 */
static inline void write_dd(const char* dest, const char* src) {
    write_line_def(dest, ASM_DD, src);
}

/*
 * DF - Define Far Pointer / Fword
 * Allocates and optionally initializes a 48-bit far pointer or six-byte object depending on
 * context.
 *
 * Syntax: name DF initializer[, initializer]...
 * Common usage: Used primarily in segmented or descriptor-related code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   fp df 0
 */
static inline void write_df(const char* dest, const char* src) {
    write_line_def(dest, ASM_DF, src);
}

/*
 * DQ - Define Quadword
 * Allocates and optionally initializes 64-bit objects.
 *
 * Syntax: name DQ initializer[, initializer]...
 * Common usage: Used for 64-bit integers, pointers, and double-precision constants.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value dq 123456789ABCDEF0h
 */
static inline void write_dq(const char* dest, const char* src) {
    write_line_def(dest, ASM_DQ, src);
}

/*
 * AsmMemoryLayout - Describes the in-memory layout of a data structure as a set
 * of parallel arrays, one entry per field (in order, low address to high):
 *   - tag:       a documentary label describing the whole layout object (e.g.
 *                the struct type it represents).
 *   - names[i]:  the assembly label placed on the field, or "" for an anonymous
 *                field (padding or an interior member that is never referenced by
 *                name). Only the first field of a struct usually carries the
 *                struct's own label; the rest are typically "".
 *   - values[i]: the field's initializer expression (e.g. "0", "offset WndProc").
 *   - tags[i]:   the field's logical type tag (documentary, mirroring AsmDataField
 *                tags elsewhere), e.g. "lpfnWndProc" or "hInstance".
 *   - offsets[i]:the field's byte offset from the start of the layout. Offsets are
 *                explicit so fields may overlap (a union member shares the offset of
 *                the field it aliases); a field whose byte range lies within storage
 *                that was already emitted is written as a MASM `label` alias rather
 *                than re-reserving storage.
 *   - sizes[i]:  the field's size in bytes; selects the define directive
 *                (1 -> db, 2 -> dw, 4 -> dd, 8 -> dq).
 *   - count:     number of fields (length of each parallel array).
 *
 * Build one with create_memory_layout + set_memory_layout_entry, then emit it
 * with write_data_memory_layout.
 */
typedef struct {
    const char*  tag;
    const char** names;
    const char** values;
    const char** tags;
    int*         offsets;
    int*         sizes;
    int          count;
} AsmMemoryLayout;

/*
 * The layout most recently produced by create_memory_layout. set_memory_layout_entry
 * fills entries into this "layout under construction", so callers can populate it
 * with a running index (int i = 0; set_memory_layout_entry(i++, ...)) without
 * threading the layout pointer through every call.
 */
static AsmMemoryLayout* asm_current_layout = NULL;

/*
 * Global bump-allocation arena. create_memory_layout carves the layout struct and
 * its parallel arrays out of this fixed buffer instead of calling malloc. The
 * buffer size is hard-coded; this is a one-shot generator, so allocations are
 * never individually freed. asm_bump_alloc hands out packed, unaligned chunks
 * (x64 tolerates unaligned scalar access) and aborts loudly if the arena is
 * exhausted.
 */
/* Bump arena. A generation run is a short-lived process whose allocations are
 * almost all strings that must live until the process exits. Freeing them
 * individually would not make the run faster or smaller in any way that
 * matters, so the arena is never freed. 64 KiB is enough for the demos; the
 * allocator aborts if it is exhausted rather than growing silently.
 */
#define ASM_BUMP_ARENA_SIZE (64 * 1024)
static unsigned char asm_bump_arena[ASM_BUMP_ARENA_SIZE];
static size_t asm_bump_offset = 0;

/* Reserve `size` bytes from the process-lifetime bump arena. Running past the arena stops generation. */
static inline void* asm_bump_alloc(size_t size) {
    if (asm_bump_offset + size > ASM_BUMP_ARENA_SIZE) {
        char buf[180];
        snprintf(buf, sizeof buf,
                "asm_bump_alloc: out of arena memory (need %zu, %zu of %d used)\n",
                size, asm_bump_offset, ASM_BUMP_ARENA_SIZE);
        host_fail(buf);
    }
    void* ptr = &asm_bump_arena[asm_bump_offset];
    asm_bump_offset += size;
    return ptr;
}

/* Copy a parameter list into the arena and set is_register. Callers may pass
 * a temporary array; the AsmCall keeps the copy. An is_register the caller
 * already set is preserved; otherwise the name is classified (no '[' and no
 * space means a register). */
static inline AsmDataField* asm_copy_call_params(const AsmDataField* params, int count) {
    if (count <= 0 || params == NULL) return NULL;
    AsmDataField* copy = (AsmDataField*)asm_bump_alloc(sizeof(AsmDataField) * (size_t)count);
    for (int i = 0; i < count; i++) {
        copy[i] = params[i];
        if (!copy[i].is_register)
            copy[i].is_register = asm_name_is_register(copy[i].name);
    }
    return copy;
}

/*
 * create_memory_layout - Allocate an AsmMemoryLayout with room for count fields.
 *
 * tag is a documentary label for the whole object. The returned layout becomes
 * the current layout that set_memory_layout_entry writes into; populate its
 * fields with set_memory_layout_entry, then pass it to write_data_memory_layout.
 * Storage comes from the global bump arena and is intentionally never freed: this
 * is a one-shot generator that exits when done.
 */
static inline AsmMemoryLayout* create_memory_layout(int count, const char* tag) {
    AsmMemoryLayout* layout = (AsmMemoryLayout*)asm_bump_alloc(sizeof(AsmMemoryLayout));
    layout->tag = tag;
    layout->count = count;
    layout->names  = (const char**)asm_bump_alloc(sizeof(const char*) * (size_t)count);
    layout->values = (const char**)asm_bump_alloc(sizeof(const char*) * (size_t)count);
    layout->tags   = (const char**)asm_bump_alloc(sizeof(const char*) * (size_t)count);
    layout->offsets = (int*)asm_bump_alloc(sizeof(int) * (size_t)count);
    layout->sizes  = (int*)asm_bump_alloc(sizeof(int) * (size_t)count);
    asm_current_layout = layout;
    return layout;
}

/*
 * set_memory_layout_entry - Fill one field of the current layout.
 *
 * index is the field position (0-based); name/value/tag are that field's label,
 * initializer, and type tag; offset is its byte offset from the start of the
 * layout and size is its size in bytes. Explicit offsets let fields overlap (e.g.
 * a union member reusing an earlier field's offset). Call once per field, e.g.
 *   int i = 0;
 *   set_memory_layout_entry(i++, "wndClass", "0", "style", 0, 4);
 *   set_memory_layout_entry(i++, "", "offset WndProc", "lpfnWndProc", 8, 8);
 * An out-of-range index aborts loudly.
 */
static inline void set_memory_layout_entry(int index, const char* name,
                                           const char* value, const char* tag,
                                           int offset, int size) {
    if (asm_current_layout == NULL || index < 0 || index >= asm_current_layout->count) {
        fprintf(stderr,
                "set_memory_layout_entry: index %d out of range (count %d)\n",
                index, asm_current_layout ? asm_current_layout->count : 0);
        exit(1);
    }
    asm_current_layout->names[index]   = name;
    asm_current_layout->values[index]  = value;
    asm_current_layout->tags[index]    = tag;
    asm_current_layout->offsets[index] = offset;
    asm_current_layout->sizes[index]   = size;
}

/*
 * AsmMemoryLayoutRefs - Per-field references produced by write_data_memory_layout.
 *
 * fields[i] is an AsmDataField that names field i as a sized memory operand, e.g.
 * {"qword ptr wcInstance", "hInstance"}, so callers can reference struct members
 * (write_set_mov, write_mov, ...) without re-typing the operand literal. The size
 * selects the operand's pointer keyword (byte/word/dword/qword). A padding or
 * anonymous field (blank name) yields an empty AsmDataField ({0}), since it has no
 * label to reference. Storage comes from the bump arena and outlives the call.
 */
typedef struct {
    const AsmDataField* fields;
    int                 count;
} AsmMemoryLayoutRefs;

/* Pointer-size keyword for a field of byte size, or NULL when size is not positive.
 * size 1, 2, 4, and 8 select byte, word, dword, and qword. A larger multiple of
 * 8 is also qword, so a named label can be read as its first qword. Any other
 * positive size is byte, which is what an anonymous pad emitted as db N dup needs. */
static inline const char* asm_ptr_keyword(int size) {
    switch (size) {
        case 1: return "byte";
        case 2: return "word";
        case 4: return "dword";
        case 8: return "qword";
        default:
            if (size >= 8 && (size % 8) == 0) return "qword";
            if (size > 0) return "byte";
            return NULL;
    }
}

/*
 * write_data_memory_layout - Emit a data-segment structure from an AsmMemoryLayout.
 *
 * Walks the fields in order, tracking the current fill position, and emits one
 * define directive per field (db/dw/dd/dq chosen from sizes[i]) so the layout is
 * described once as data rather than a hand-ordered series of write_dd/write_dq
 * calls. Padding is expressed the same way it would be by hand: a field with a
 * blank name ("") and the desired size/value.
 *
 * Fields carry explicit offsets, so overlapping (union) members are supported: a
 * group of fields sharing the same offset is written as MASM `label` aliases for
 * all but the last, which reserves the storage. List the aliases before the
 * storage field so the labels land at the shared offset before it is filled. Only
 * shared-start overlaps are supported; gaps and partial (mid-field) overlaps abort
 * loudly. A non-positive size aborts. Any other size is one define.
 *
 * Returns an AsmMemoryLayoutRefs whose fields[i] references field i as a sized
 * memory operand (e.g. {"qword ptr wcInstance", "hInstance"}), so later code can
 * reference struct members through it instead of repeating operand literals.
 */
static inline AsmMemoryLayoutRefs write_data_memory_layout(const AsmMemoryLayout* layout) {
    AsmDataField* refs =
        (AsmDataField*)asm_bump_alloc(sizeof(AsmDataField) * (size_t)layout->count);
    int pos = 0;
    for (int i = 0; i < layout->count; i++) {
        const char* name = layout->names[i];
        const char* value = layout->values[i];
        int off = layout->offsets[i];
        int size = layout->sizes[i];
        const char* keyword = asm_ptr_keyword(size);
        if (keyword == NULL) {
            fprintf(stderr,
                    "write_data_memory_layout: unsupported field size %d for field \"%s\"\n",
                    size, name[0] ? name : "(padding)");
            exit(1);
        }

        /* Build the reference operand for this field (e.g. "qword ptr wcInstance"). */
        if (name[0]) {
            size_t len = strlen(keyword) + strlen(" ptr ") + strlen(name) + 1;
            char* operand = (char*)asm_bump_alloc(len);
            snprintf(operand, len, "%s ptr %s", keyword, name);
            refs[i].name = operand;
            refs[i].type = layout->tags[i];
            refs[i].use_as = NULL;
            /* Register the field as a read-only data global under the same sized
             * operand callers will reference it by. */
            asm_global_dict_add(operand, layout->tags[i]);
        } else {
            refs[i] = asm_void;
        }

        /* A field sharing its offset with the next one is an alias for the same
         * storage: emit it as a zero-size label rather than reserving bytes. */
        int is_alias = (i + 1 < layout->count && layout->offsets[i + 1] == off);
        if (off == pos && is_alias) {
            if (name[0]) {
                write_line_def(name, ASM_LABEL, keyword);
            }
        } else if (off == pos) {
            if (size == 1) write_db(name, value);
            else if (size == 2) write_dw(name, value);
            else if (size == 4) write_dd(name, value);
            else if (size == 8) write_dq(name, value);
            else {
                /* A page-sized member, or the pad that slides it there, is one
                 * db N dup line. Splitting it into 4-byte pieces would blow
                 * past C_STRUCT_MAX_ENTRIES before the field was reached. */
                /* write() keeps the pointer, so the dup text has to outlive
                 * this call. A stack buffer would be overwritten before ml64. */
                char tmp[80];
                int n = snprintf(tmp, sizeof tmp, "%d dup(%s)", size, value);
                if (n < 0 || n >= (int)sizeof tmp) n = (int)sizeof tmp - 1;
                char* spec = (char*)asm_bump_alloc((size_t)n + 1);
                memcpy(spec, tmp, (size_t)n + 1);
                write_db(name, spec);
            }
            pos += size;
        } else {
            char buf[420];
            snprintf(buf, sizeof buf,
                    "write_data_memory_layout: field \"%s\" at offset %d does not line up "
                    "with fill position %d (gaps and partial overlaps are unsupported; list "
                    "union aliases before the storage field that shares their offset)\n",
                    name[0] ? name : "(padding)", off, pos);
            host_fail(buf);
        }
    }
    AsmMemoryLayoutRefs result = { refs, layout->count };
    return result;
}

/*
 * write_unitialised_data - Reserve one uninitialised qword and record its tag.
 *
 * Emits "<name> dq ?" (the "?" is supplied here, so callers never repeat it) and
 * returns an AsmDataField holding the field's name and its type tag, so later
 * references can use out.name as the location and out.type as the type tag
 * instead of repeating the literals. For example,
 * AsmDataField log_pos = write_unitialised_data("logPos", "head") emits
 * "logPos dq ?" and lets callers write write_set_mov(log_pos, ...) /
 * assert_type((AsmDataField){"rax", log_pos.type}).
 */
static inline AsmDataField write_unitialised_data(const char* name, const char* type) {
    write_dq(name, "?");
    asm_global_dict_add(name, type);
    AsmDataField out = {name, type};
    return out;
}

/*
 * write_initialised_data - Define one initialised byte field and record its tag.
 *
 * Follows write_unitialised_data, but emits "<name> db <value>" (the caller
 * supplies the initial value) and returns an AsmDataField holding the field's
 * name (out.name) and its type tag (out.type), so later references use those
 * instead of repeating the literals. Take a field's address with a separate
 * write_constant, e.g.
 * AsmDataField off = write_constant("offset_msgStarted", "offset msgStarted", "src").
 */
static inline AsmDataField write_initialised_data(const char* name,
                                                  const char* value, const char* type) {
    write_db(name, value);
    asm_global_dict_add(name, type);
    AsmDataField out = {name, type};
    return out;
}

/*
 * DT - Define Ten Bytes
 * Allocates and optionally initializes 10-byte objects.
 *
 * Syntax: name DT initializer[, initializer]...
 * Common usage: Often used for x87 80-bit extended-precision constants.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value dt 1.0
 */
static inline void write_dt(const char* dest, const char* src) {
    write_line_def(dest, ASM_DT, src);
}

/*
 * BYTE_KW - BYTE Keyword
 * Type keyword naming an unsigned 8-bit object or memory operand size.
 *
 * Syntax: BYTE PTR operand | name BYTE initializer
 * Common usage: Used in declarations and explicit operand-size overrides.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov al, BYTE PTR [rax]
 */
static inline void write_byte_kw(const char* operand) {
    write_line2(ASM_BYTE_KW, operand);
}

/*
 * SBYTE - SBYTE Keyword
 * Type keyword naming a signed 8-bit object.
 *
 * Syntax: name SBYTE initializer
 * Common usage: Used in typed data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value sbyte -1
 */
static inline void write_sbyte(const char* operand) {
    write_line2(ASM_SBYTE, operand);
}

/*
 * WORD_KW - WORD Keyword
 * Type keyword naming a 16-bit object or operand size.
 *
 * Syntax: WORD PTR operand | name WORD initializer
 * Common usage: Used in declarations and explicit operand-size overrides.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ax, WORD PTR [rax]
 */
static inline void write_word_kw(const char* operand) {
    write_line2(ASM_WORD_KW, operand);
}

/*
 * SWORD - SWORD Keyword
 * Type keyword naming a signed 16-bit object.
 *
 * Syntax: name SWORD initializer
 * Common usage: Used in typed data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value sword -2
 */
static inline void write_sword(const char* operand) {
    write_line2(ASM_SWORD, operand);
}

/*
 * DWORD_KW - DWORD Keyword
 * Type keyword naming a 32-bit object or operand size.
 *
 * Syntax: DWORD PTR operand | name DWORD initializer
 * Common usage: Used in declarations and explicit operand-size overrides.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov eax, DWORD PTR [rax]
 */
static inline void write_dword_kw(const char* operand) {
    write_line2(ASM_DWORD_KW, operand);
}

/*
 * SDWORD - SDWORD Keyword
 * Type keyword naming a signed 32-bit object.
 *
 * Syntax: name SDWORD initializer
 * Common usage: Used in typed data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value sdword -3
 */
static inline void write_sdword(const char* operand) {
    write_line2(ASM_SDWORD, operand);
}

/*
 * FWORD_KW - FWORD Keyword
 * Type keyword naming a 48-bit object, often a far pointer or descriptor fragment.
 *
 * Syntax: FWORD PTR operand
 * Common usage: Used mainly in segmented and system-level code.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   les bx, FWORD PTR ptr48
 */
static inline void write_fword_kw(const char* operand) {
    write_line2(ASM_FWORD_KW, operand);
}

/*
 * QWORD_KW - QWORD Keyword
 * Type keyword naming a 64-bit object or operand size.
 *
 * Syntax: QWORD PTR operand | name QWORD initializer
 * Common usage: Used for 64-bit integers, pointers, and explicit operand sizing.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov rax, QWORD PTR [rsp]
 */
static inline void write_qword_kw(const char* operand) {
    write_line2(ASM_QWORD_KW, operand);
}

/*
 * SQWORD - SQWORD Keyword
 * Type keyword naming a signed 64-bit object.
 *
 * Syntax: name SQWORD initializer
 * Common usage: Used in typed data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value sqword -4
 */
static inline void write_sqword(const char* operand) {
    write_line2(ASM_SQWORD, operand);
}

/*
 * TBYTE_KW - TBYTE Keyword
 * Type keyword naming a 10-byte object, usually x87 extended precision.
 *
 * Syntax: TBYTE PTR operand | name TBYTE initializer
 * Common usage: Used for 80-bit floating-point storage.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   fld TBYTE PTR [value]
 */
static inline void write_tbyte_kw(const char* operand) {
    write_line2(ASM_TBYTE_KW, operand);
}

/*
 * REAL4 - REAL4 Keyword
 * Type keyword naming a 32-bit IEEE single-precision floating-point object.
 *
 * Syntax: name REAL4 initializer
 * Common usage: Used for scalar floating data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value real4 1.0
 */
static inline void write_real4(const char* operand) {
    write_line2(ASM_REAL4, operand);
}

/* "name real4 decimal". decimal is the initializer ("1.0", "-0.5"). */
static inline void write_real4_def(const char* name, const char* decimal) {
    write_line_def(name, ASM_REAL4, decimal);
}

/* "name real8 decimal". */
static inline void write_real8_def(const char* name, const char* decimal) {
    write_line_def(name, ASM_REAL8, decimal);
}

/*
 * REAL8 - REAL8 Keyword
 * Type keyword naming a 64-bit IEEE double-precision floating-point object.
 *
 * Syntax: name REAL8 initializer
 * Common usage: Used for scalar floating data declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value real8 1.0
 */
static inline void write_real8(const char* operand) {
    write_line2(ASM_REAL8, operand);
}

/*
 * REAL10 - REAL10 Keyword
 * Type keyword naming an 80-bit extended-precision floating-point object.
 *
 * Syntax: name REAL10 initializer
 * Common usage: Used for x87 extended values.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   value real10 1.0
 */
static inline void write_real10(const char* operand) {
    write_line2(ASM_REAL10, operand);
}

/*
 * XMMWORD_KW - XMMWORD Keyword
 * Type keyword naming a 128-bit memory operand or object size.
 *
 * Syntax: XMMWORD PTR operand
 * Common usage: Used with SSE loads, stores, and vector declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   movdqa xmm0, XMMWORD PTR [rax]
 */
static inline void write_xmmword_kw(const char* operand) {
    write_line2(ASM_XMMWORD_KW, operand);
}

/*
 * YMMWORD_KW - YMMWORD Keyword
 * Type keyword naming a 256-bit memory operand or object size.
 *
 * Syntax: YMMWORD PTR operand
 * Common usage: Used with AVX loads, stores, and vector declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   vmovaps ymm0, YMMWORD PTR [rax]
 */
static inline void write_ymmword_kw(const char* operand) {
    write_line2(ASM_YMMWORD_KW, operand);
}

/*
 * NEAR - NEAR Keyword
 * Type or distance keyword specifying a near code or pointer reference within the current code
 * segment.
 *
 * Syntax: NEAR PTR symbol
 * Common usage: Used in prototypes, labels, and explicit distance overrides.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   call NEAR PTR proc
 */
static inline void write_near(const char* operand) {
    write_line2(ASM_NEAR, operand);
}

/*
 * FAR - FAR Keyword
 * Type or distance keyword specifying a far code or pointer reference that includes a segment
 * component.
 *
 * Syntax: FAR PTR symbol
 * Common usage: Mainly relevant in segmented code and some low-level system interfaces.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   jmp FAR PTR target
 */
static inline void write_far(const char* operand) {
    write_line2(ASM_FAR, operand);
}

/*
 * PTR - PTR Operator
 * Overrides or supplies an explicit type for a memory operand or symbol reference.
 *
 * Syntax: type PTR expression
 * Common usage: Commonly used when MASM cannot infer operand size from context.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov eax, DWORD PTR [rax]
 */
static inline void write_ptr(const char* operand) {
    write_line2(ASM_PTR, operand);
}

/*
 * OFFSET - OFFSET Operator
 * Computes the assembly-time offset/address of a symbol within its segment or flat address
 * space.
 *
 * Syntax: OFFSET symbol
 * Common usage: Used to load addresses into registers or pass pointers to procedures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov rax, OFFSET msg
 */
static inline void write_offset(const char* operand) {
    write_line2(ASM_OFFSET, operand);
}

/*
 * ADDR - ADDR Operator
 * Passes the address of an argument to INVOKE or high-level MASM procedure machinery.
 *
 * Syntax: ADDR symbol
 * Common usage: Common with INVOKE and typed prototypes.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   invoke SomeProc, ADDR buffer
 */
static inline void write_addr(const char* operand) {
    write_line2(ASM_ADDR, operand);
}

/*
 * SEG - SEG Operator
 * Computes the segment part of a symbol address at assembly time.
 *
 * Syntax: SEG symbol
 * Common usage: Relevant primarily in segmented-memory programming.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ax, SEG variable
 */
static inline void write_seg(const char* operand) {
    write_line2(ASM_SEG, operand);
}

/*
 * TYPE_OP - TYPE Operator
 * Returns the size or type value associated with a symbol or type expression.
 *
 * Syntax: TYPE symbol
 * Common usage: Used in macros and generic code to reason about data sizes.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, TYPE DWORD
 */
static inline void write_type_op(const char* operand) {
    write_line2(ASM_TYPE_OP, operand);
}

/*
 * SIZE_OP - SIZE Operator
 * Returns the size in bytes of a single data item for a symbol or type.
 *
 * Syntax: SIZE symbol
 * Common usage: Used in generic macros and declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   IF SIZE buffer EQ 16
 */
static inline void write_size_op(const char* operand) {
    write_line2(ASM_SIZE_OP, operand);
}

/*
 * SIZEOF_OP - SIZEOF Operator
 * Returns the total size in bytes of a variable, array, or type instance.
 *
 * Syntax: SIZEOF symbol
 * Common usage: Useful for buffer lengths and structure copies.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, SIZEOF buffer
 */
static inline void write_sizeof_op(const char* operand) {
    write_line2(ASM_SIZEOF_OP, operand);
}

/*
 * LENGTH_OP - LENGTH Operator
 * Returns the number of initializers in the first dimension of a data item in older MASM
 * semantics.
 *
 * Syntax: LENGTH symbol
 * Common usage: Mostly superseded by LENGTHOF but still recognized.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, LENGTH table
 */
static inline void write_length_op(const char* operand) {
    write_line2(ASM_LENGTH_OP, operand);
}

/*
 * LENGTHOF_OP - LENGTHOF Operator
 * Returns the total element count of a variable or array.
 *
 * Syntax: LENGTHOF symbol
 * Common usage: Common for loop counts over static arrays.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, LENGTHOF table
 */
static inline void write_lengthof_op(const char* operand) {
    write_line2(ASM_LENGTHOF_OP, operand);
}

/*
 * WIDTH_OP - WIDTH Operator
 * Returns the width in bits of a RECORD field or type.
 *
 * Syntax: WIDTH fieldOrType
 * Common usage: Used with RECORD definitions and masks.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ecx, WIDTH carry
 */
static inline void write_width_op(const char* operand) {
    write_line2(ASM_WIDTH_OP, operand);
}

/*
 * MASK_OP - MASK Operator
 * Returns a bit mask for a RECORD field.
 *
 * Syntax: MASK field
 * Common usage: Used to extract or set packed record fields symbolically.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov eax, MASK carry
 */
static inline void write_mask_op(const char* operand) {
    write_line2(ASM_MASK_OP, operand);
}

/*
 * THIS - THIS Operator
 * Associates the current location with a specific type without allocating additional storage.
 *
 * Syntax: THIS type
 * Common usage: Used to create typed aliases at the current location counter.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   label THIS DWORD
 */
static inline void write_this(const char* operand) {
    write_line2(ASM_THIS, operand);
}

/*
 * DUP_KW - DUP Operator
 * Repeats an initializer or nested initializer pattern in data declarations.
 *
 * Syntax: count DUP(initializer)
 * Common usage: Used to reserve or initialize arrays succinctly.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   buffer db 64 DUP(0)
 */
static inline void write_dup_kw(const char* operand) {
    write_line2(ASM_DUP_KW, operand);
}

/*
 * FLAT - FLAT Keyword
 * Names the flat address space or memory model used by modern 32-bit MASM programs.
 *
 * Syntax: FLAT
 * Common usage: Appears in .MODEL, ASSUME, and type expressions.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .model flat, stdcall
 */
static inline void write_flat(const char* operand) {
    write_line2(ASM_FLAT, operand);
}

/*
 * STDCALL - STDCALL Keyword
 * Calling-convention keyword for callee-clean parameter passing on the stack in 32-bit Windows-
 * style code.
 *
 * Syntax: STDCALL
 * Common usage: Used in .MODEL, PROC, and PROTO declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .model flat, stdcall
 */
static inline void write_stdcall(const char* operand) {
    write_line2(ASM_STDCALL, operand);
}

/*
 * C_CALL - C Calling-Convention Keyword
 * Calling-convention keyword for C-style caller-clean calls.
 *
 * Syntax: C
 * Common usage: Used in .MODEL, PROC, and PROTO declarations.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   printf PROTO C :PTR BYTE
 */
static inline void write_c_call(const char* operand) {
    write_line2(ASM_C_CALL, operand);
}

/*
 * SYSCALL_CALL - SYSCALL Calling-Convention Keyword
 * MASM calling-convention keyword spelling used in prototype metadata, distinct from the CPU
 * instruction with the same mnemonic.
 *
 * Syntax: SYSCALL
 * Common usage: Used where MASM wants a language/calling-convention name rather than an opcode.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyApi PROTO SYSCALL :QWORD
 */
static inline void write_syscall_call(const char* operand) {
    write_line2(ASM_SYSCALL_CALL, operand);
}

/*
 * PASCAL - PASCAL Keyword
 * Calling-convention keyword for Pascal-style parameter ordering and callee cleanup.
 *
 * Syntax: PASCAL
 * Common usage: Used in legacy Windows and educational examples.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   .model small, pascal
 */
static inline void write_pascal(const char* operand) {
    write_line2(ASM_PASCAL, operand);
}

/*
 * BASIC - BASIC Keyword
 * Calling-convention keyword for BASIC-compatible interfaces in older MASM toolchains.
 *
 * Syntax: BASIC
 * Common usage: Historical interop feature for legacy environments.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   SomeProc PROTO BASIC :WORD
 */
static inline void write_basic(const char* operand) {
    write_line2(ASM_BASIC, operand);
}

/*
 * FORTRAN - FORTRAN Keyword
 * Calling-convention keyword for FORTRAN-compatible interfaces in older MASM toolchains.
 *
 * Syntax: FORTRAN
 * Common usage: Historical interop feature for legacy codebases.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   SomeProc PROTO FORTRAN :WORD
 */
static inline void write_fortran(const char* operand) {
    write_line2(ASM_FORTRAN, operand);
}

/*
 * FASTCALL - FASTCALL Keyword
 * Calling-convention keyword for register-assisted parameter passing.
 *
 * Syntax: FASTCALL
 * Common usage: Used in prototypes and procedures where MASM should apply fastcall naming and
 * argument rules.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyProc PROTO FASTCALL :DWORD, :DWORD
 */
static inline void write_fastcall(const char* operand) {
    write_line2(ASM_FASTCALL, operand);
}

/*
 * USES - USES Keyword
 * PROC attribute that tells MASM to save and restore listed registers in the generated
 * prologue/epilogue.
 *
 * Syntax: PROC USES reglist
 * Common usage: Convenient for preserving nonvolatile registers automatically in MASM
 * procedures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   MyProc PROC USES ebx esi edi
 */
static inline void write_uses(const char* operand) {
    write_line2(ASM_USES, operand);
}

/*
 * VARARG - VARARG Keyword
 * Marks the end of fixed parameters in a procedure prototype to indicate additional unnamed
 * arguments may follow.
 *
 * Syntax: name PROTO arg1:type, arg2:type, VARARG
 * Common usage: Used for printf-style procedures.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   printf PROTO C :PTR BYTE, :VARARG
 */
static inline void write_vararg(const char* operand) {
    write_line2(ASM_VARARG, operand);
}

/*
 * HIGH - HIGH Operator
 * Returns the high byte of an expression at assembly time.
 *
 * Syntax: HIGH expression
 * Common usage: Useful in low-level constant decomposition.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov al, HIGH wordValue
 */
static inline void write_high(const char* operand) {
    write_line2(ASM_HIGH, operand);
}

/*
 * LOW - LOW Operator
 * Returns the low byte of an expression at assembly time.
 *
 * Syntax: LOW expression
 * Common usage: Useful in low-level constant decomposition.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov al, LOW wordValue
 */
static inline void write_low(const char* operand) {
    write_line2(ASM_LOW, operand);
}

/*
 * HIGHWORD - HIGHWORD Operator
 * Returns the high 16 bits of a 32-bit expression at assembly time.
 *
 * Syntax: HIGHWORD expression
 * Common usage: Used when splitting larger constants.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ax, HIGHWORD value
 */
static inline void write_highword(const char* operand) {
    write_line2(ASM_HIGHWORD, operand);
}

/*
 * LOWWORD - LOWWORD Operator
 * Returns the low 16 bits of a 32-bit expression at assembly time.
 *
 * Syntax: LOWWORD expression
 * Common usage: Used when splitting larger constants.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   mov ax, LOWWORD value
 */
static inline void write_lowword(const char* operand) {
    write_line2(ASM_LOWWORD, operand);
}

/*
 * OPATTR - OPATTR Operator
 * Returns an attribute bitmask describing a symbol or operand's properties.
 *
 * Syntax: OPATTR operand
 * Common usage: Used in sophisticated macros for overload-like dispatch and validation.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   IF OPATTR(arg) AND 10h
 */
static inline void write_opattr(const char* operand) {
    write_line2(ASM_OPATTR, operand);
}

/*
 * SHORT - SHORT Keyword
 * Forces a short relative jump or treats an expression as a short-distance target.
 *
 * Syntax: JMP SHORT label
 * Common usage: Used when the programmer wants an 8-bit relative branch encoding or explicit
 * distance override.
 * Flags affected: None. Directives and MASM operators are handled by the assembler rather than
 * by the CPU.
 *
 * Example:
 *   jmp short again
 */
static inline void write_short(const char* operand) {
    write_line2(ASM_SHORT, operand);
}

/*
 * asm_string - bytes, address, and length of one initialised string.
 *
 * db_text is the MASM byte list ("\"hello\",13,10" or "\"path\",0").
 * addr is "offset_<name>" and len is "<name>Len" equal to `$ - <name>`.
 * A caller that wants the length idiom under a different name still calls
 * write_constant("$ - label") itself. An unused length equate is harmless.
 */
typedef struct {
    AsmDataField bytes;
    AsmDataField addr;
    AsmDataField len;
} AsmString;

static inline AsmString asm_string(const char* name, const char* db_text,
                                   const char* tag) {
    AsmString s;
    char offname[96], lenname[96], expr[96];
    s.bytes = write_initialised_data(name, db_text, tag);
    snprintf(offname, sizeof offname, "offset_%s", name);
    snprintf(lenname, sizeof lenname, "%sLen", name);
    snprintf(expr, sizeof expr, "$ - %s", name);
    s.addr = write_offset_constant(asm_strdup(offname), s.bytes, tag);
    s.len = write_constant(asm_strdup(lenname), asm_strdup(expr), "len");
    return s;
}

/*
 * asm_layout - a field list that accumulates byte offsets.
 *
 * asm_layout_field appends the next field at the running offset.
 * asm_layout_at pins a field at an explicit offset, which is how a Win32
 * struct keeps hInstance at @24, and how MSG overlaps its base alias with
 * hwnd at offset 0. The running offset becomes the end of the furthest field.
 */
typedef struct {
    AsmMemoryLayout* layout;
    int index;
    int offset;
} AsmLayout;

static inline AsmLayout asm_layout_begin(const char* tag, int count) {
    AsmLayout L;
    L.layout = create_memory_layout(count, tag);
    L.index = 0;
    L.offset = 0;
    return L;
}

static inline void asm_layout_field(AsmLayout* L, const char* name,
                                    const char* value, const char* tag,
                                    int size) {
    set_memory_layout_entry(L->index++, name, value, tag, L->offset, size);
    L->offset += size;
}

static inline void asm_layout_at(AsmLayout* L, const char* name,
                                 const char* value, const char* tag,
                                 int offset, int size) {
    set_memory_layout_entry(L->index++, name, value, tag, offset, size);
    if (offset + size > L->offset) L->offset = offset + size;
}

