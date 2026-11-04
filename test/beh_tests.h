/*
 * beh_tests.h - generate methods with the layer headers and run them.
 *
 * The success program is assembled and executed. Each check returns a value;
 * main ExitProcess's the first mismatch (the numbers are beh_why[]).
 *
 * Generation failures (a call whose volatile mask hits a preserved register
 * this procedure has not saved, a poisoned read when spill is off, a type
 * mismatch, a use-after-move) go through host_fail. Arming host_fail_env
 * longjmps back here instead of ending the test process. With the arm clear,
 * host_fail still exits, so a real generator stops the way it always has.
 *
 * Included from test/test.c after the asm, C, C++, and C# headers,
 * gc_common.h, the Rust, Python, and JavaScript headers, fuzz_beh.h,
 * and gap_tests.h.
 */

/* Exit codes of the success program. Index 0 is success. beh_check writes
 * the index of the first mismatch into the exit code and leaves later
 * checks alone. */
static const char* beh_why[] = {
    "ok",
    "spill keep (42 survives a call)",
    "spill pressure (15 live values)",
    "stack slot across a call",
    "c add",
    "asm add",
    "fifth stack argument",
    "cpp field add",
    "csharp field add",
    "python field add",
    "javascript field add",
    "rust Ok path",
    "rust Err path",
    "rust drop of the value still owned",
    "asm init mov",
    "fuzz 64-bit alu",
    "fuzz 32-bit alu",
    "fuzz loop sum",
    "fuzz compares",
    "fuzz switch",
    "fuzz struct qword",
    "c_and_imm",
    "c_rep_movsb",
    "c_relabel",
    "c_goto",
    "c_loop_continue",
    "c_if_ne_else",
    "nested if else loop switch",
    "zero-arg call",
    "one-arg call",
    "three-arg call",
    "sixth stack argument",
    "GetLastError",
    "c_string length",
    "c_global",
    "c_constant",
    "c_ptr_data",
    "ALIGN_NONE field",
    "ALIGN_CACHE field",
    "ALIGN_PAGE field",
    "union alias",
    "C_CHAR return",
    "C_SHORT return",
    "mixed widths",
    "cache-line frame",
    "page frame",
    "preserved mask 0",
    "interleaved spill",
    "if join value",
    "if join tag",
    "asm checksum",
    "cpp virtual and fields",
    "cpp second field",
    "cpp destructors",
    "csharp virtual",
    "csharp using",
    "csharp type check",
    "csharp downcast",
    "csharp unbox",
    "python dispatch",
    "python second field",
    "python missing attribute",
    "python refcount",
    "javascript prototype",
    "javascript missing name",
    "javascript set",
    "javascript truthiness",
    "rust scopes and moves",
    "rust try twice",
    "one-armed ult/slt/ne",
    "page-slide field",
    "many spills",
    "CreateFile stack args",
    "cpp second interface",
    "cpp stack construct",
    "js runtime property"
};

/* Zero a register with xor so the clobber callee does not depend on an immediate. */
static void beh_xor_reg(const char* reg) {
    AsmDataField r = { reg, "scratch", NULL };
    write_xor(r, r);
}

/* Calls the preserved-register check must accept. Dead at run time. */
static void beh_emit_legal_calls(void) {
    AsmExtern only_rax = write_proc("legal_callee", NULL, 0, asm_void);
    only_rax.volatile_mask = 1u << 0; /* rax, which this caller does not preserve */
    write_ret();
    write_endp(&only_rax);

    AsmExtern legal_caller = write_proc("legal_caller", NULL, 0, asm_void);
    asm_set_preserved(1u << 3); /* rbx is preserved and not saved */
    write_call(&only_rax);
    write_ret();
    write_endp(&legal_caller);

    AsmExtern quiet = write_proc("quiet_callee", NULL, 0, asm_void);
    write_ret();
    write_endp(&quiet);

    AsmExtern quiet_caller = write_proc("quiet_caller", NULL, 0, asm_void);
    asm_set_preserved(ASM_WIN64_PRESERVED);
    write_call(&quiet); /* volatile_mask 0: the check does not apply */
    write_ret();
    write_endp(&quiet_caller);

    AsmExtern saved_callee = write_proc("saved_callee", NULL, 0, asm_void);
    saved_callee.volatile_mask = 1u << 3; /* rbx */
    write_ret();
    write_endp(&saved_callee);

    AsmExtern saved_caller = write_proc("saved_caller", NULL, 0, asm_void);
    asm_set_preserved(1u << 3);
    asm_mark_saved(1u << 3);
    write_replace_mov("rbx", (AsmDataField){ "0", "tmp", NULL });
    write_call(&saved_callee);
    write_ret();
    write_endp(&saved_caller);

    AsmExtern typed = write_proc("typed_ok", NULL, 0, (AsmDataField){ "rax", "sum", NULL });
    write_init_mov("rax", "42", "sum");
    assert_type((AsmDataField){ "rax", "sum", NULL });
    write_ret();
    write_endp(&typed);
}

/* If code is still 0 and got is not imm, set code to id. Frees got. */
static void beh_check(CValue* code, const char* id, CValue got, const char* imm) {
    const char* tag = got.tag ? got.tag : "v";
    CIf open = c_if_eq(*code, c_imm("0", C_LONG, "code"));
        CElse arm = c_if_eq_else(got, c_imm(imm, got.type, tag));
        c_else(&arm);
            c_assign_imm(code, id);
        c_end_if_else(&arm);
    c_end_if(&open);
    c_free(&got);
}

/* Emit beh_output.asm: legal calls, spill, language field-adds, and the
 * fuzz procedures. main ExitProcess's the first mismatch. */
static int beh_emit(void) {
    static const char* ep_tags[] = { "exit_code" };
    static const AsmDataField add_params[] = {
        { "rcx", "a", NULL }, { "rdx", "b", NULL }
    };
    static const AsmDataField fifth_params[] = {
        { "rcx", "a", NULL }, { "rdx", "b", NULL },
        { "r8", "c", NULL }, { "r9", "d", NULL },
        { "qword ptr [rsp+40]", "e", NULL }
    };
    static const char* one_name[] = {
        "v0", "v1", "v2", "v3", "v4", "v5", "v6",
        "v7", "v8", "v9", "v10", "v11", "v12"
    };
    const CParam two[] = { { "a", C_LONG }, { "b", C_LONG } };
    const CParam self_p[] = { { "self", C_PTR } };

    write_set_case_sensitive_identifiers();
    AsmExtern exit_process = c_extern("ExitProcess", ep_tags, 1, C_PTR, NULL);

    write_data();
        c_word_data("rust_drops", "0", "drops");
        c_word_data("obj_a", "0", "self");
        c_word_data("obj_b", "0", "self");
        c_word_data("cs_obj", "0", "obj");
        write_dq("", "0");
        write_dq("", "0");
        fuzz_emit_struct_data();
        gap_emit_data();
    write_code();

    beh_emit_legal_calls();

    AsmExtern asm_add = write_proc("asm_add", add_params, 2, (AsmDataField){ "rax", "sum", NULL });
        write_replace_mov("rax", (AsmDataField){ "rcx", "sum", NULL });
        write_add((AsmDataField){ "rax", "sum", NULL }, (AsmDataField){ "rdx", "b", NULL });
        write_ret();
    write_endp(&asm_add);

    AsmExtern fifth = write_proc("fifth", fifth_params, 5, (AsmDataField){ "rax", "e", NULL });
        asm_frame_prologue(ASM_WIN64_PRESERVED, 16);
        write_replace_mov("rax", (AsmDataField){ "qword ptr [rbp+48]", "e", NULL });
        asm_frame_epilogue();
        write_ret();
    write_endp(&fifth);
    asm_frame_end();

    CFunc clobber = c_begin_function("spill_clobber", NULL, 0, C_LONG, NULL);
        beh_xor_reg("rax");
        beh_xor_reg("rcx");
        beh_xor_reg("rdx");
        beh_xor_reg("r8");
        beh_xor_reg("r9");
        beh_xor_reg("r10");
        beh_xor_reg("r11");
        c_ret(&clobber);
    c_end_function(&clobber);

    CFunc keep = c_begin_function("spill_keep", NULL, 0, C_LONG, "kept");
        CValue kept = c_local_imm("kept", C_LONG, "42");
        c_call(&clobber);
        c_ret_value(&keep, kept);
    c_end_function(&keep);

    CFunc press = c_begin_function("spill_press", NULL, 0, C_LONG, "sum");
        CValue sum = c_local_imm("sum", C_LONG, "0");
        CValue ones[13];
        for (int i = 0; i < 13; i++) ones[i] = c_local_imm(one_name[i], C_LONG, "1");
        CValue big = c_local_imm("big", C_LONG, "100");
        c_call(&clobber);
        for (int i = 0; i < 13; i++) c_add(&sum, ones[i]);
        c_add(&sum, big);
        c_ret_value(&press, sum);
    c_end_function(&press);

    CFunc slotf = c_begin_function("stack_keep", NULL, 0, C_LONG, "slot");
        CValue n = c_local_imm("kept", C_LONG, "20");
        CValue slot = c_stack_local("slot", C_LONG, ALIGN_C);
        c_materialize(slot.loc, "17", "slot");
        c_call(&clobber);
        c_add(&slot, n);
        c_ret_value(&slotf, slot);
    c_end_function(&slotf);

    CFunc add2 = c_begin_function("c_add2", two, 2, C_LONG, "sum");
        CValue s = c_dup("sum", C_LONG, c_param(&add2, 0));
        c_add(&s, c_param(&add2, 1));
        c_ret_value(&add2, s);
    c_end_function(&add2);

    CFunc dropfn = c_begin_function("rust_drop", self_p, 1, C_PTR, NULL);
        write_add((AsmDataField){ asm_mem("qword", "rust_drops", 0), "drops", NULL },
                  literal("1", "drops"));
        c_ret(&dropfn);
    c_end_function(&dropfn);

    CFunc checked = c_begin_function("rust_checked", NULL, 0, C_PTR, NULL);
        rust_ret_ok(&checked);
    c_end_function(&checked);

    CFunc erred = c_begin_function("rust_erred", NULL, 0, C_PTR, NULL);
        rust_ret_err(&erred);
    c_end_function(&erred);

    CFunc okf = c_begin_function("rust_ok_test", NULL, 0, C_LONG, "rok");
        CValue rok = c_stack_local("rok", C_LONG, ALIGN_C);
        c_materialize(rok.loc, "0", "rok");
        c_call(&checked);
        RustCatch okc = rust_catch_begin();
            rust_try(&okc);
            write_set_mov(c_cell(rok), literal("1", "rok"));
        rust_catch(&okc);
            write_set_mov(c_cell(rok), literal("2", "rok"));
        rust_catch_end(&okc);
        c_ret_value(&okf, rok);
    c_end_function(&okf);

    CFunc errf = c_begin_function("rust_err_test", NULL, 0, C_LONG, "rerr");
        CValue rerr = c_stack_local("rerr", C_LONG, ALIGN_C);
        c_materialize(rerr.loc, "0", "rerr");
        c_call(&erred);
        RustCatch errc = rust_catch_begin();
            rust_try(&errc);
            write_set_mov(c_cell(rerr), literal("1", "rerr"));
        rust_catch(&errc);
            write_set_mov(c_cell(rerr), literal("7", "rerr"));
        rust_catch_end(&errc);
        c_ret_value(&errf, rerr);
    c_end_function(&errf);

    CFunc own = c_begin_function("rust_own", NULL, 0, C_LONG, "drops");
        RustScope scope = rust_scope_begin();
        RustVal owned_a, owned_b;
        rust_let(&scope, &owned_a, "a", "obj_a", &dropfn);
        rust_let(&scope, &owned_b, "b", "obj_b", &dropfn);
        rust_move(&owned_b);
        rust_scope_end(&scope);
        c_ret_value(&own, c_wrap((AsmDataField){ "rust_drops", "drops", NULL }, C_LONG));
    c_end_function(&own);

    cpp_class_begin("Box");
        cpp_field("n", C_LONG);
        CFunc* add3 = cpp_method_begin("add3");
            CValue cn = cpp_this_field("n", C_LONG);
            c_add(&cn, c_imm("3", C_LONG, "n"));
            c_ret(add3);
        cpp_method_end();
    CppClass Box = cpp_class_end();

    cs_class_begin("Cnt");
        cs_field("n");
        CFunc* inc = cs_method_begin("inc");
            CValue csn = cs_this_field("n", C_LONG);
            c_add(&csn, c_imm("1", C_LONG, "n"));
            c_ret(inc);
        cs_method_end();
    cs_class_end();

    py_class_begin("Bag");
        py_field("n");
        CFunc* pyadd = py_method_begin("add");
            py_field_add_imm(py_this_field("n", C_LONG), 5);
            c_ret(pyadd);
        py_method_end();
    PyClass Bag = py_class_end();

    js_class_begin("Cell");
        js_field("n");
        CFunc* bump = js_method_begin("bump");
            js_field_add_imm(js_this_field("n", C_LONG), 9);
            c_ret(bump);
        js_method_end();
    JsClass Cell = js_class_end();

    static const char* alu64_names[] = {"fa", "fb", "fc", "fd", "fe", "ff"};
    static const char* alu32_names[] = {"ia", "ib", "ic", "id"};
    const char* exp_alu64 = 0;
    const char* exp_alu32 = 0;
    const char* exp_loop = 0;
    const char* exp_br = 0;
    const char* exp_sw = 0;
    CFunc falu64 = fuzz_emit_alu(&clobber, 64, "fuzz_alu64", "facc",
                                 alu64_names, 6, 24, &exp_alu64);
    CFunc falu32 = fuzz_emit_alu(&clobber, 32, "fuzz_alu32", "isum",
                                 alu32_names, 4, 16, &exp_alu32);
    CFunc floop = fuzz_emit_loop(&exp_loop);
    CFunc fbr = fuzz_emit_branch(&exp_br);
    CFunc fsw = fuzz_emit_switch(&exp_sw);
    CFunc fst = fuzz_emit_struct_fn();
    printf("  fuzz draws %u alu64 %s alu32 %s loop %s branch %s switch %s struct off %d\n",
           fuzz_pcg_draws, exp_alu64, exp_alu32, exp_loop, exp_br, exp_sw,
           fuzz_struct_off);

    gap_emit_procs(&clobber, &checked, &erred, &dropfn);

    CFunc mainf = c_begin_function("main", NULL, 0, C_LONG, NULL);
        CValue code = c_into("rbx", C_LONG, c_imm("0", C_LONG, "code"), "code");
        beh_check(&code, "1", c_call_proc(C_LONG, &keep.ext), "42");
        beh_check(&code, "2", c_call_proc(C_LONG, &press.ext), "113");
        beh_check(&code, "3", c_call_proc(C_LONG, &slotf.ext), "37");
        beh_check(&code, "4",
                  c_call_proc(C_LONG, &add2.ext, literal("20", "a"), literal("22", "b")),
                  "42");
        beh_check(&code, "5",
                  c_call_proc(C_LONG, &asm_add, literal("20", "a"), literal("22", "b")),
                  "42");
        beh_check(&code, "6",
                  c_call_ext(C_LONG, &fifth,
                             literal("1", "a"), literal("2", "b"), literal("3", "c"),
                             literal("4", "d"), literal("99", "e")),
                  "99");

        CppObject box = cpp_construct(&Box, "box");
        cpp_send(add3, box);
        beh_check(&code, "7", cpp_field_ref(&box, "n", C_LONG), "3");

        c_call(inc, c_addr("cs_obj", "this"));
        beh_check(&code, "8",
                  c_wrap((AsmDataField){ asm_mem("qword", "cs_obj", 16), "n", NULL }, C_LONG),
                  "1");

        PyObject bag = py_construct(&Bag, "bag");
        py_send(pyadd, bag);
        beh_check(&code, "9", py_field_ref(&bag, "n", C_LONG), "5");

        JsObject cell = js_construct(&Cell, "cell", NULL);
        js_send(bump, cell);
        beh_check(&code, "10", js_field_ref(&cell, "n", C_LONG), "9");

        beh_check(&code, "11", c_call_proc(C_LONG, &okf.ext), "1");
        beh_check(&code, "12", c_call_proc(C_LONG, &errf.ext), "7");
        beh_check(&code, "13", c_call_proc(C_LONG, &own.ext), "1");

        /* typed_ok's descriptor stayed inside beh_emit_legal_calls.
         * This call matches that procedure: no parameters, result in rax. */
        {
            AsmExtern typed_call = { "typed_ok", NULL, 0, { "rax", "sum", NULL }, 0, 0 };
            beh_check(&code, "14", c_call_proc(C_LONG, &typed_call), "42");
        }

        fuzz_eq_check(&code, "15", c_call_proc(C_LONG, &falu64.ext), exp_alu64, "exA", C_LONG);
        fuzz_eq_check(&code, "16", c_call_proc(C_INT, &falu32.ext), exp_alu32, "exI", C_INT);
        fuzz_eq_check(&code, "17", c_call_proc(C_LONG, &floop.ext), exp_loop, "exL", C_LONG);
        fuzz_eq_check(&code, "18", c_call_proc(C_LONG, &fbr.ext), exp_br, "exB", C_LONG);
        fuzz_eq_check(&code, "19", c_call_proc(C_LONG, &fsw.ext), exp_sw, "exW", C_LONG);
        fuzz_eq_check(&code, "20", c_call_proc(C_LONG, &fst.ext), "44", "exS", C_LONG);

        gap_emit_checks(&code);

        c_call_ext_void(&exit_process, c_treat_as(code, "code", "exit_code"));
    c_end_function(&mainf);
    write_end("");
    return write_asm_file("beh_output.asm");
}

/* First 2000 bytes of a tool log at path, for a failed ml64/link/run. */
static void beh_print_log(const char* path) {
    FILE* f = fopen(path, "rb");
    char buf[2001];
    size_t n;
    if (!f) return;
    n = fread(buf, 1, 2000, f);
    buf[n] = 0;
    fclose(f);
    printf("%s\n", buf);
}

/* Run cmd, capture stdout/stderr to log, wait up to 60s. Returns 1 if the
 * process was created. *code is the exit code, or 1 if the wait timed out. */
static int beh_exec(const char* cmd, const char* log, DWORD* code) {
    char line[512];
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    HANDLE h;
    DWORD wait;

    strncpy(line, cmd, sizeof line - 1);
    line[sizeof line - 1] = 0;
    ZeroMemory(&sa, sizeof sa);
    sa.nLength = sizeof sa;
    sa.bInheritHandle = TRUE;
    h = CreateFileA(log, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        printf("  [FAIL] could not create %s\n", log);
        return 0;
    }
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = h;
    si.hStdError = h;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    ZeroMemory(&pi, sizeof pi);
    if (!CreateProcessA(NULL, line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        printf("  [FAIL] CreateProcess(%s) %lu\n", cmd, (unsigned long)GetLastError());
        CloseHandle(h);
        return 0;
    }
    wait = WaitForSingleObject(pi.hProcess, 60000);
    *code = 1;
    if (wait == WAIT_OBJECT_0) GetExitCodeProcess(pi.hProcess, code);
    else TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(h);
    return 1;
}

/* Drop a generation that host_fail aborted. The asm buffer rewinds to
 * cursor. Dictionaries, the register pool, and the language cursors reset
 * so the next sample starts clean. asm_type_dict_clear also clears the
 * disposed list. */
static void beh_reset_failed(int cursor, int indent) {
    int i;
    host_fail_env = 0;
    asm_output_cursor = cursor;
    asm_indent_level = indent;
    asm_proc_preserved = 0;
    asm_proc_saved = 0;
    asm_frame_active = 0;
    asm_frame_npush = 0;
    asm_current_return_reg = asm_void;
    asm_type_dict_clear();
    c_pool_reset();
    c_frame_open = 0;
    c_path_dead = 0;
    c_regs[12].reserved = 0;
    c_spill_enabled = 1;
    c_if_sp = 0;
    c_else_sp = 0;
    c_sw_sp = 0;
    c_loop_sp = 0;
    c_call_nsave = 0;
    cpp_cur = 0;
    cpp_cur_method = -1;
    cpp_scope_depth = 0;
    for (i = 0; i < CPP_SCOPE_DEPTH; i++) cpp_scope_pool[i].open = 0;
    cs_cur = 0;
    cs_cur_method = -1;
    py_cur = 0;
    py_cur_method = -1;
    js_cur = 0;
    js_cur_method = -1;
}

/* Arm host_fail_env, run fn, and require the message to contain needle. */
static void beh_expect_fail(const char* needle, void (*fn)(void)) {
    HostJmpBuf env;
    int cursor = asm_output_cursor;
    int indent = asm_indent_level;
    host_fail_hit = 0;
    host_fail_msg[0] = 0;
    host_fail_env = &env;
    if (_setjmp(&env) == 0) {
        fn();
        host_fail_env = 0;
    }
    beh_reset_failed(cursor, indent);
    if (!host_fail_hit || strstr(host_fail_msg, needle) == NULL) {
        printf("  [FAIL] expected failure containing \"%s\"\n", needle);
        if (host_fail_hit) printf("         got: %s\n", host_fail_msg);
        else               printf("         generation did not fail\n");
        expect_true(0);
    }
    printf("  [PASS] rejects: %s\n", needle);
}

/* Proves the longjmp hook is armed. The needle is the crash_if text. */
static void fail_probe(void) { crash_if(1, "probe failure"); }

/* Callee clobbers rbx. Caller preserves rbx and does not save it. */
static void fail_volatile_call(void) {
    AsmExtern callee = write_proc("bad_callee", NULL, 0, asm_void);
    callee.volatile_mask = 1u << 3; /* rbx */
    write_ret();
    write_endp(&callee);
    AsmExtern caller = write_proc("bad_caller", NULL, 0, asm_void);
    asm_set_preserved(1u << 3);
    write_call(&callee);
    write_endp(&caller);
}

/* Write rbx inside a procedure that preserves it and has not saved it. */
static void fail_unsaved_write(void) {
    AsmExtern p = write_proc("bad_write", NULL, 0, asm_void);
    asm_set_preserved(1u << 3);
    write_replace_mov("rbx", (AsmDataField){ "0", "tmp", NULL });
    write_endp(&p);
}

/* assert_type against a tag the location does not hold. */
static void fail_type(void) {
    AsmExtern p = write_proc("bad_type", NULL, 0, asm_void);
    write_init_mov("rax", "1", "sum");
    assert_type((AsmDataField){ "rax", "other", NULL });
    write_endp(&p);
}

/* Call argument tag does not match the parameter tag. */
static void fail_arg(void) {
    static const AsmDataField ps[] = { { "rcx", "need", NULL } };
    AsmExtern p = write_proc("needs", ps, 1, asm_void);
    write_ret();
    write_endp(&p);
    AsmExtern caller = write_proc("needs_caller", NULL, 0, asm_void);
    write_call(&p, literal("1", "other"));
    write_endp(&caller);
}

/* treat_as claims a tag the location does not have. */
static void fail_treat(void) {
    AsmExtern p = write_proc("treat_host", NULL, 0, asm_void);
    write_init_mov("rax", "1", "sum");
    (void)treat_as((AsmDataField){ "rax", "sum", NULL }, "other", "exit_code");
    write_endp(&p);
}

/* Second write_init_mov of a location that is already tracked. */
static void fail_init_twice(void) {
    AsmExtern p = write_proc("init_bad", NULL, 0, asm_void);
    write_init_mov("rax", "1", "sum");
    write_init_mov("rax", "2", "sum");
    write_endp(&p);
}

/* write_set_mov requires a location that already has a tag. */
static void fail_set_untracked(void) {
    AsmExtern p = write_proc("set_bad", NULL, 0, asm_void);
    write_set_mov((AsmDataField){ "rax", "sum", NULL }, literal("1", "sum"));
    write_endp(&p);
}

/* Fifth argument spelled [rsp+32]. The Windows slot is [rsp+40]. */
static void fail_slot(void) {
    static const AsmDataField bad[] = {
        { "rcx", "a", NULL }, { "rdx", "b", NULL },
        { "r8", "c", NULL }, { "r9", "d", NULL },
        { "qword ptr [rsp+32]", "e", NULL }
    };
    AsmExtern p = write_proc("bad_slot", bad, 5, asm_void);
    write_ret();
    write_endp(&p);
    AsmExtern caller = write_proc("bad_slot_caller", NULL, 0, asm_void);
    write_extern_call(&p,
                      literal("1", "a"), literal("2", "b"), literal("3", "c"),
                      literal("4", "d"), literal("5", "e"));
    write_endp(&caller);
}

/* Spill off: the call does not save the local, and c_cell rejects the read. */
static void fail_poison(void) {
    AsmExtern dummy = write_proc("poison_callee", NULL, 0, asm_void);
    dummy.volatile_mask = ASM_WIN64_VOLATILE;
    write_ret();
    write_endp(&dummy);
    c_spill_off();
    CFunc f = c_begin_function("poison_user", NULL, 0, C_LONG, NULL);
    CValue n = c_local_imm("kept", C_LONG, "7");
    c_call_proc_void(&dummy);
    (void)c_cell(n);
    c_end_function(&f);
}

/* rust_use after rust_move. */
static void fail_rust_move(void) {
    CFunc dummy;
    RustScope s;
    RustVal v;
    memset(&dummy, 0, sizeof dummy);
    s = rust_scope_begin();
    rust_let(&s, &v, "box", "box_sym", &dummy);
    rust_move(&v);
    rust_use(&v);
}

/* Method-end with no method open. */
static void fail_cpp(void) { cpp_method_end(); }
/* Call cs_method_end outside an open C# method. */
static void fail_cs(void)  { cs_method_end(); }
/* Call py_method_end outside an open Python method. */
static void fail_py(void)  { py_method_end(); }
/* Call js_method_end outside an open JavaScript method. */
static void fail_js(void)  { js_method_end(); }

/* A later set of a value marked const is a generation error. */
static void fail_const_assign(void) {
    CFunc f = c_begin_function("const_user", NULL, 0, C_INT, NULL);
    CValue n = c_local_imm("n", C_INT, "1");
    c_mark_const(&n);
    c_assign_imm(&n, "2");
    c_end_function(&f);
}

/* A read of a disposed value is a generation error. */
static void fail_dispose_use(void) {
    CFunc f = c_begin_function("dispose_user", NULL, 0, C_INT, NULL);
    CValue n = c_local_imm("n", C_INT, "1");
    c_dispose(&n);
    (void)c_cell(n);
    c_end_function(&f);
}

/* Assemble and run the success program, then the generation-failure samples. */
static void beh_run_tests(void) {
    DWORD code = 1;
    int nwhy = (int)(sizeof beh_why / sizeof beh_why[0]);

    printf("Running behavior tests...\n");
    beh_expect_fail("probe failure", fail_probe);
    fuzz_host_checks();
    expect_true(beh_emit() == 0);

    expect_true(beh_exec("ml64 /nologo /c beh_output.asm", "beh_ml64.log", &code));
    if (code != 0) {
        printf("  [FAIL] ml64 exit %lu\n", (unsigned long)code);
        beh_print_log("beh_ml64.log");
        expect_true(0);
    }
    remove("beh_ml64.log");

    expect_true(beh_exec(
        "link /nologo /subsystem:console /entry:main /STACK:2097152 /OUT:beh_output.exe beh_output.obj kernel32.lib",
        "beh_link.log", &code));
    if (code != 0) {
        printf("  [FAIL] link exit %lu\n", (unsigned long)code);
        beh_print_log("beh_link.log");
        expect_true(0);
    }
    remove("beh_link.log");

    expect_true(beh_exec("beh_output.exe", "beh_run.log", &code));
    if (code != 0) {
        unsigned long u = (unsigned long)code;
        printf("  [FAIL] beh_output.exe exit %lu", u);
        if (code < (DWORD)nwhy) printf(" (%s)", beh_why[code]);
        printf("\n");
        beh_print_log("beh_run.log");
        expect_true(0);
    }
    remove("beh_run.log");
    printf("  [PASS] generated methods behaved (spill, calls, cpp, csharp, py, js, rust)\n");

    beh_expect_fail("call clobbers a non-volatile", fail_volatile_call);
    beh_expect_fail("write to a non-volatile", fail_unsaved_write);
    beh_expect_fail("type mismatch", fail_type);
    beh_expect_fail("expected type", fail_arg);
    beh_expect_fail("treat_as", fail_treat);
    beh_expect_fail("write_init_mov", fail_init_twice);
    beh_expect_fail("write_set_mov", fail_set_untracked);
    beh_expect_fail("expected location", fail_slot);
    beh_expect_fail("clobbered by the call", fail_poison);
    beh_expect_fail("use of moved value", fail_rust_move);
    beh_expect_fail("cpp_method_end", fail_cpp);
    beh_expect_fail("cs_method_end", fail_cs);
    beh_expect_fail("py_method_end", fail_py);
    beh_expect_fail("js_method_end", fail_js);
    beh_expect_fail("const", fail_const_assign);
    beh_expect_fail("disposed", fail_dispose_use);
    gap_expect_fails();
    fuzz_reject_samples();
    printf("Behavior tests passed.\n");
}
