/*
 * gap_tests.h - basic checks for features the behavior program used to skip.
 *
 * Included from beh_tests.h. The procedures are emitted into the same
 * success program. main compares each result with beh_check. Generation
 * failures are armed later, after that program has been written out.
 */
static void beh_check(CValue* code, const char* id, CValue got, const char* imm);
static void beh_expect_fail(const char* needle, void (*fn)(void));

static AsmExtern gap_ext[80];
static CType gap_ty[80];
static const char* gap_exp[80];
static int gap_kind[80];
static CStructRef gap_none_ref, gap_cache_ref, gap_page_ref;
static CStr gap_hi_str;
static PyStr gap_py_msg;

static const AsmDataField gap_sixth_params[] = {
    { "rcx", "a", NULL }, { "rdx", "b", NULL },
    { "r8", "c", NULL }, { "r9", "d", NULL },
    { "qword ptr [rsp+40]", "e", NULL },
    { "qword ptr [rsp+48]", "f", NULL }
};

/* Remember ext as behavior exit id. type is the result width. expect is the
 * decimal text main compares against. kind is 0 for a nullary call, 1 for the
 * single argument 6, 2 for the arguments 10 20 30, and 3 for the six-argument
 * call whose last value is 77. */
static void gap_save(int id, AsmExtern ext, CType type, const char* expect, int kind) {
    gap_ext[id] = ext;
    gap_ty[id] = type;
    gap_exp[id] = expect;
    gap_kind[id] = kind;
}

/* Add the partial in eax into the checksum kept in r8d. */
static void gap_acc(void) {
    write_add((AsmDataField){ "r8d", "sum", NULL }, (AsmDataField){ "eax", "part", NULL });
}

/* Emit the data the gap procedures read: bytes, scalars, structs, a union,
 * and the language-object literals that are not tied to a constructor call. */
static void gap_emit_data(void) {
    write_align("8");
    c_word_data("gap_tab", "11", "word");
    c_word_data("gap_tab1", "22", "word");
    c_word_data("gap_tab2", "33", "word");
    write_align("4");
    write_dd("gap_x87", "11");
    write_db("gap_src", "4,5,6,7");
    write_db("gap_dst", "0,0,0,0");

    gap_hi_str = c_string("gap_hi", "\"hi\",0", "lp", "len");
    c_constant("gap_seven", "7", C_LONG, "seven");
    c_ptr_data("gap_ptr", "gap_src", "ptr");
    write_dataq();
        c_global("gap_word", C_LONG, "gapw");
    write_data();

    /* An 8-byte field at offset 57 needs 7 bytes of natural padding. Cache
     * and page modes add nothing more when that field does not straddle a
     * line or a page. A 4096-byte field at offset 1 is slid to the next page,
     * so its pad is more than those 7 bytes. */
    expect_true(align_pad(57, 8, ALIGN_CACHE) == 7);
    expect_true(align_pad(1, 8, ALIGN_PAGE) == 7);
    expect_true(align_pad(1, 4096, ALIGN_PAGE) > 8);
    expect_true(align_pad(1, 4096, ALIGN_PAGE) == 4095);

    {
        CStruct s = c_struct_begin("gap_none", "0");
        c_struct_set_align(&s, ALIGN_NONE);
        c_struct_field(&s, "", "1", "b", C_CHAR);
        c_struct_field(&s, "", "2", "b", C_CHAR);
        c_struct_field(&s, "gap_nq", "44", "q", C_LONG);
        gap_none_ref = c_struct_emit(&s);
    }
    {
        CStruct s = c_struct_begin("gap_cache", "0");
        c_struct_set_align(&s, ALIGN_CACHE);
        c_struct_field(&s, "", "0", "q", C_LONG);
        for (int i = 0; i < 6; i++) c_struct_field(&s, "", "0", "q", C_LONG);
        c_struct_field(&s, "", "0", "b", C_CHAR);
        c_struct_field(&s, "gap_cq", "99", "q", C_LONG);
        gap_cache_ref = c_struct_emit(&s);
    }
    {
        CStruct s = c_struct_begin("gap_page", "0");
        c_struct_set_align(&s, ALIGN_PAGE);
        c_struct_field(&s, "", "0", "b", C_CHAR);
        c_struct_field(&s, "gap_pq", "13", "q", C_LONG);
        gap_page_ref = c_struct_emit(&s);
    }
    /* A 4096-byte field at offset 1 slides to the next page. The pad is one
     * 4095-byte entry, and the field label is 4096 bytes past the base. */
    {
        CType pageblk = { "page", 4096, false };
        CStruct s = c_struct_begin("gap_slide", "0");
        c_struct_set_align(&s, ALIGN_PAGE);
        c_struct_field(&s, "", "0", "b", C_CHAR);
        c_struct_field(&s, "gap_slide_big", "0", "q", pageblk);
        (void)c_struct_emit(&s);
    }
    write_db("gap_wname", "\"gap_w.txt\",0");
    write_db("gap_wbytes", "\"wxyz\"");
    c_word_data("gap_wn", "0", "nwritten");
    {
        AsmMemoryLayout* u = create_memory_layout(2, "gap_u");
        set_memory_layout_entry(0, "gap_ub", "0", "ub", 0, 1);
        set_memory_layout_entry(1, "gap_ud", "120", "ud", 0, 4);
        write_data_memory_layout(u);
    }

    c_word_data("gap_dtors", "0", "drops");
    c_word_data("cs_uses", "0", "uses");
    c_word_data("gap_tw", "7", "typehandle");
    gap_py_msg = py_string("gap_pyq", "\"x\",0");

    write_align("8");
    js_data();
    /* The closure environment descriptor is three qwords. */
    {
        GcType env = js_env_type();
        expect_true(env.size == 24);
    }
}

/* One assembled form of each write_* helper whose parameters are a mnemonic
 * and const char* operands. The procedure jumps over the body. Register-token
 * writers, AsmDataField helpers, and forms ml64 rejects are omitted.
 * jecxz, jrcxz, and loop* are rel8, so they target the label that follows them. */
static void gap_emit_catalogue(void) {
    AsmExtern cat = write_proc("gap_catalogue", NULL, 0, asm_void);
    cat.volatile_mask = ASM_WIN64_VOLATILE;
    write_jmp("gap_cat_end");
#include "gap_catalogue.h"
    write_label("gap_cat_lbl");
    write_label("gap_cat_end");
    write_ret();
    write_endp(&cat);
}

/* Emit every gap procedure. clobber is the volatile-wiping callee. checked
 * and erred are the Rust Ok and Err callees. dropfn is the Rust Drop body.
 * Constructors that must run inside a live frame are emitted inside the
 * procedure that calls them. */
static void gap_emit_procs(CFunc* clobber, CFunc* checked, CFunc* erred, CFunc* dropfn) {
    /* c_and_imm keeps the low nibble of 255. */
    {
        CFunc f = c_begin_function("gap_and", NULL, 0, C_LONG, "v");
            CValue v = c_local_imm("v", C_LONG, "255");
            c_and_imm(&v, "15");
            c_ret_value(&f, v);
        c_end_function(&f);
        gap_save(21, f.ext, C_LONG, "15", 0);
    }
    /* c_rep_movsb copies 4,5,6,7. The third destination byte is 6. */
    {
        CFunc f = c_begin_function("gap_movsb", NULL, 0, C_LONG, "n");
            CValue dst = c_local_imm("dst", C_PTR, "0");
            CValue src = c_local_imm("src", C_PTR, "0");
            CValue n = c_local_imm("ncount", C_LONG, "4");
            write_lea(dst.loc, "gap_dst");
            write_lea(src.loc, "gap_src");
            c_rep_movsb(dst, src, n);
            CValue out = c_local_imm("n", C_LONG, "0");
            write_movzx("r11d", asm_mem("byte", dst.loc, 2));
            c_materialize(out.loc, "r11", "n");
            c_ret_value(&f, out);
        c_end_function(&f);
        gap_save(22, f.ext, C_LONG, "6", 0);
    }
    /* c_relabel changes the tag to fresh at generation time. The bits stay 9. */
    {
        CFunc f = c_begin_function("gap_relabel", NULL, 0, C_LONG, "fresh");
            CValue v = c_local_imm("old", C_LONG, "9");
            c_relabel(&v, "fresh");
            expect_true(v.tag != NULL && strcmp(v.tag, "fresh") == 0);
            c_ret_value(&f, v);
        c_end_function(&f);
        gap_save(23, f.ext, C_LONG, "9", 0);
    }
    /* c_goto jumps back to the label until the counter reaches 3. */
    {
        CFunc f = c_begin_function("gap_goto", NULL, 0, C_LONG, "i");
            CValue i = c_local_imm("i", C_LONG, "0");
            CLabel start = c_label();
                c_add(&i, c_imm("1", C_LONG, "i"));
                CElse eq = c_if_eq_else(i, c_imm("3", C_LONG, "i"));
                c_else(&eq);
                    c_goto(start);
                c_end_if_else(&eq);
            c_ret_value(&f, i);
        c_end_function(&f);
        gap_save(24, f.ext, C_LONG, "3", 0);
    }
    /* continue skips the add when i is 2. break stops once i reaches 4.
     * The sum is 1+3+4. */
    {
        CFunc f = c_begin_function("gap_loop", NULL, 0, C_LONG, "s");
            CValue i = c_local_imm("i", C_LONG, "0");
            CValue s = c_local_imm("s", C_LONG, "0");
            CLoop L = c_loop_begin();
                c_add(&i, c_imm("1", C_LONG, "i"));
                CIf sk = c_if_eq(i, c_imm("2", C_LONG, "i"));
                    c_loop_continue(&L);
                c_end_if(&sk);
                c_add(&s, i);
                CIf br = c_if_uge(i, c_imm("4", C_LONG, "i"));
                    c_loop_break(&L);
                c_end_if(&br);
            c_loop_end(&L);
            c_ret_value(&f, s);
        c_end_function(&f);
        gap_save(25, f.ext, C_LONG, "8", 0);
    }
    /* 1 is not 2, so the not-equal arm stores 1. */
    {
        CFunc f = c_begin_function("gap_ne", NULL, 0, C_LONG, "r");
            CValue a = c_local_imm("a", C_LONG, "1");
            CValue r = c_local_imm("r", C_LONG, "0");
            CElse e = c_if_ne_else(a, c_imm("2", C_LONG, "a"));
                c_assign_imm(&r, "1");
            c_else(&e);
                c_assign_imm(&r, "0");
            c_end_if_else(&e);
            c_ret_value(&f, r);
        c_end_function(&f);
        gap_save(26, f.ext, C_LONG, "1", 0);
    }
    /* A nested if stores 4, one loop trip adds 7, and switch case 2 adds 8. */
    {
        CFunc f = c_begin_function("gap_nest", NULL, 0, C_LONG, "acc");
            CValue acc = c_local_imm("acc", C_LONG, "0");
            CValue one = c_local_imm("one", C_LONG, "1");
            CValue two = c_local_imm("two", C_LONG, "2");
            CElse outer = c_if_eq_else(one, c_imm("1", C_LONG, "one"));
                CElse inner = c_if_ne_else(two, c_imm("3", C_LONG, "two"));
                    c_assign_imm(&acc, "4");
                c_else(&inner);
                    c_assign_imm(&acc, "0");
                c_end_if_else(&inner);
            c_else(&outer);
                c_assign_imm(&acc, "0");
            c_end_if_else(&outer);
            CValue i = c_local_imm("i", C_LONG, "0");
            CLoop L = c_loop_begin();
                c_add(&i, c_imm("1", C_LONG, "i"));
                c_add(&acc, c_imm("7", C_LONG, "acc"));
                CIf br = c_if_uge(i, c_imm("1", C_LONG, "i"));
                    c_loop_break(&L);
                c_end_if(&br);
            c_loop_end(&L);
            CValue sel = c_local_imm("sel", C_LONG, "2");
            CSwitch sw = c_switch(sel);
                c_case(&sw, "1");
                    c_assign_imm(&acc, "0");
                    c_break(&sw);
                c_case(&sw, "2");
                    c_add(&acc, c_imm("8", C_LONG, "acc"));
                    c_break(&sw);
            c_end_switch(&sw);
            c_ret_value(&f, acc);
        c_end_function(&f);
        gap_save(27, f.ext, C_LONG, "19", 0);
    }
    /* A procedure with no parameters returns the local it initialised. */
    {
        CFunc f = c_begin_function("gap_zero", NULL, 0, C_LONG, "z");
            CValue z = c_local_imm("z", C_LONG, "8");
            c_ret_value(&f, z);
        c_end_function(&f);
        gap_save(28, f.ext, C_LONG, "8", 0);
    }
    /* One parameter. main passes 6, and the procedure returns 7. */
    {
        const CParam p[] = { { "a", C_LONG } };
        CFunc f = c_begin_function("gap_one", p, 1, C_LONG, "z");
            CValue z = c_dup("z", C_LONG, c_param(&f, 0));
            c_add(&z, c_imm("1", C_LONG, "z"));
            c_ret_value(&f, z);
        c_end_function(&f);
        gap_save(29, f.ext, C_LONG, "7", 1);
    }
    /* Three parameters. main passes 10, 20, and 30, and the sum is 60. */
    {
        const CParam p[] = { { "a", C_LONG }, { "b", C_LONG }, { "c", C_LONG } };
        CFunc f = c_begin_function("gap_three", p, 3, C_LONG, "z");
            CValue z = c_dup("z", C_LONG, c_param(&f, 0));
            c_add(&z, c_param(&f, 1));
            c_add(&z, c_param(&f, 2));
            c_ret_value(&f, z);
        c_end_function(&f);
        gap_save(30, f.ext, C_LONG, "60", 2);
    }
    /* Six arguments, opened with write_proc. The last one is the stack slot
     * at [rbp+56] after the frame prologue, and that slot holds 77. */
    {
        AsmExtern sixth = write_proc("gap_sixth", gap_sixth_params, 6,
                                     (AsmDataField){ "rax", "f", NULL });
            asm_frame_prologue(ASM_WIN64_PRESERVED, 16);
            write_replace_mov("rax", (AsmDataField){ "qword ptr [rbp+56]", "f", NULL });
            asm_frame_epilogue();
            write_ret();
        write_endp(&sixth);
        asm_frame_end();
        gap_save(31, sixth, C_LONG, "77", 3);
    }
    /* SetLastError(231) then GetLastError returns that same code. */
    {
        const char* tags[] = { "dwErrCode" };
        AsmExtern setle = c_extern("SetLastError", tags, 1, C_INT, NULL);
        AsmExtern getle = c_extern("GetLastError", NULL, 0, C_INT, "err");
        CFunc f = c_begin_function("gap_gle", NULL, 0, C_INT, "err");
            c_call_ext_void(&setle, literal("231", "dwErrCode"));
            CValue err = c_call_ext(C_INT, &getle);
            c_ret_value(&f, err);
        c_end_function(&f);
        gap_save(32, f.ext, C_INT, "231", 0);
    }
    /* c_string's length of the bytes "hi",0 is 3. */
    {
        CFunc f = c_begin_function("gap_strlen", NULL, 0, C_LONG, "len");
            c_ret_value(&f, c_wrap(gap_hi_str.len, C_LONG));
        c_end_function(&f);
        gap_save(33, f.ext, C_LONG, "3", 0);
    }
    /* A store to the gap_word global is the value the procedure returns. */
    {
        CFunc f = c_begin_function("gap_glob", NULL, 0, C_LONG, "gapw");
            CValue w = c_wrap((AsmDataField){ "gap_word", "gapw", NULL }, C_LONG);
            c_assign_imm(&w, "19");
            c_ret_value(&f, w);
        c_end_function(&f);
        gap_save(34, f.ext, C_LONG, "19", 0);
    }
    /* The named constant gap_seven reads back as 7. */
    {
        CFunc f = c_begin_function("gap_const", NULL, 0, C_LONG, "seven");
            c_ret_value(&f, c_wrap((AsmDataField){ "gap_seven", "seven", NULL }, C_LONG));
        c_end_function(&f);
        gap_save(35, f.ext, C_LONG, "7", 0);
    }
    /* lea of gap_src compares equal to the pointer global initialised to
     * that label. */
    {
        CFunc f = c_begin_function("gap_peq", NULL, 0, C_LONG, "p");
            CValue p = c_local_imm("p", C_LONG, "0");
            write_lea(p.loc, "gap_src");
            CValue q = c_wrap((AsmDataField){ "gap_ptr", "ptr", NULL }, C_LONG);
            CElse e = c_if_eq_else(p, q);
                c_assign_imm(&p, "1");
            c_else(&e);
                c_assign_imm(&p, "0");
            c_end_if_else(&e);
            c_ret_value(&f, p);
        c_end_function(&f);
        gap_save(36, f.ext, C_LONG, "1", 0);
    }
    /* The qword in the ALIGN_NONE struct is stored at a packed offset and
     * reads back as 44. */
    {
        CFunc f = c_begin_function("gap_aln", NULL, 0, C_LONG, "q");
            c_ret_value(&f, c_field(&gap_none_ref, "gap_nq", C_LONG));
        c_end_function(&f);
        gap_save(37, f.ext, C_LONG, "44", 0);
    }
    /* The qword placed after a cache line of fields reads back as 99. */
    {
        CFunc f = c_begin_function("gap_alc", NULL, 0, C_LONG, "q");
            c_ret_value(&f, c_field(&gap_cache_ref, "gap_cq", C_LONG));
        c_end_function(&f);
        gap_save(38, f.ext, C_LONG, "99", 0);
    }
    /* The qword in the page-aligned struct reads back as 13. */
    {
        CFunc f = c_begin_function("gap_alp", NULL, 0, C_LONG, "q");
            c_ret_value(&f, c_field(&gap_page_ref, "gap_pq", C_LONG));
        c_end_function(&f);
        gap_save(39, f.ext, C_LONG, "13", 0);
    }
    /* The byte and the dword share offset 0. Both reads see 120, and the
     * sum is 240. */
    {
        CFunc f = c_begin_function("gap_union", NULL, 0, C_LONG, "sum");
            CValue s = c_local_imm("sum", C_LONG, "0");
            write_movzx("r8d", "byte ptr gap_ub");
            write_replace_mov("r9d", (AsmDataField){ "dword ptr gap_ud", "ud", NULL });
            write_add((AsmDataField){ "r8", "ub", NULL }, (AsmDataField){ "r9", "ud", NULL });
            c_materialize(s.loc, "r8", "sum");
            c_ret_value(&f, s);
        c_end_function(&f);
        gap_save(40, f.ext, C_LONG, "240", 0);
    }
    /* A char-sized local returns 65. */
    {
        CFunc f = c_begin_function("gap_char", NULL, 0, C_CHAR, "ch");
            CValue v = c_local_imm("ch", C_CHAR, "65");
            c_ret_value(&f, v);
        c_end_function(&f);
        gap_save(41, f.ext, C_CHAR, "65", 0);
    }
    /* A short-sized local returns 300. */
    {
        CFunc f = c_begin_function("gap_short", NULL, 0, C_SHORT, "sh");
            CValue v = c_local_imm("sh", C_SHORT, "300");
            c_ret_value(&f, v);
        c_end_function(&f);
        gap_save(42, f.ext, C_SHORT, "300", 0);
    }
    /* z is the fifth local, so it lives in r11. The char, the short, and
     * the int are zero-extended into that register and added: 3+20+100. */
    {
        CFunc f = c_begin_function("gap_mix", NULL, 0, C_LONG, "sum");
            CValue b = c_local_imm("b", C_CHAR, "3");
            CValue w = c_local_imm("w", C_SHORT, "20");
            CValue d = c_local_imm("d", C_INT, "100");
            CValue sum = c_local_imm("sum", C_LONG, "0");
            CValue z = c_local_imm("z", C_LONG, "0");
            write_movzx("r11d", b.loc);
            c_add(&sum, z);
            write_movzx("r11d", w.loc);
            c_add(&sum, z);
            write_replace_mov("r11d", (AsmDataField){ d.loc, "d", NULL });
            c_add(&sum, z);
            c_ret_value(&f, sum);
        c_end_function(&f);
        gap_save(43, f.ext, C_LONG, "123", 0);
    }
    /* The next frame is cache aligned. The stack slot is addressed from r12
     * and holds 64. */
    {
        c_set_frame_align(ALIGN_CACHE);
        CFunc f = c_begin_function("gap_alignc", NULL, 0, C_LONG, "v");
            expect_true(asm_frame_base_align == 64);
            CValue s = c_stack_local("slot", C_LONG, ALIGN_CACHE);
            expect_true(s.loc != NULL && strstr(s.loc, "r12") != NULL);
            c_materialize(s.loc, "64", "slot");
            c_ret_value(&f, s);
        c_end_function(&f);
        gap_save(44, f.ext, C_LONG, "64", 0);
    }
    /* The next frame is page aligned. The stack slot is addressed from r12
     * and holds 4096. */
    {
        c_set_frame_align(ALIGN_PAGE);
        CFunc f = c_begin_function("gap_alignp", NULL, 0, C_LONG, "v");
            expect_true(asm_frame_base_align == 4096);
            CValue s = c_stack_local("slot", C_LONG, ALIGN_PAGE);
            expect_true(s.loc != NULL && strstr(s.loc, "r12") != NULL);
            c_materialize(s.loc, "4096", "slot");
            c_ret_value(&f, s);
        c_end_function(&f);
        gap_save(45, f.ext, C_LONG, "4096", 0);
    }
    /* Preserving no registers marks rbx volatile. The local still returns 5. */
    {
        c_set_preserved(0);
        CFunc f = c_begin_function("gap_pres", NULL, 0, C_LONG, "v");
            expect_true((f.ext.volatile_mask & (1u << 3)) != 0);
            CValue v = c_local_imm("v", C_LONG, "5");
            c_ret_value(&f, v);
        c_end_function(&f);
        gap_save(46, f.ext, C_LONG, "5", 0);
    }
    /* a and b stay readable across two calls that wipe the volatile
     * registers. c is allocated after those calls. The sum is 10+20+30. */
    {
        CFunc f = c_begin_function("gap_spill3", NULL, 0, C_LONG, "a");
            CValue a = c_local_imm("a", C_LONG, "10");
            CValue b = c_local_imm("b", C_LONG, "20");
            c_call(clobber);
            CValue c = c_local_imm("c", C_LONG, "30");
            c_call(clobber);
            c_add(&a, b);
            c_add(&a, c);
            c_ret_value(&f, a);
        c_end_function(&f);
        gap_save(47, f.ext, C_LONG, "60", 0);
    }
    /* The taken arm calls the clobber and then stores 5. The join still
     * reads that 5. */
    {
        CFunc f = c_begin_function("gap_join", NULL, 0, C_LONG, "r");
            CValue flag = c_local_imm("flag", C_LONG, "1");
            CValue r = c_local_imm("r", C_LONG, "0");
            CIf open = c_if_eq(flag, c_imm("1", C_LONG, "flag"));
                c_call(clobber);
                c_assign_imm(&r, "5");
            c_end_if(&open);
            CElse other = c_if_eq_else(flag, c_imm("0", C_LONG, "flag"));
                c_assign_imm(&r, "9");
            c_else(&other);
            c_end_if_else(&other);
            c_ret_value(&f, r);
        c_end_function(&f);
        gap_save(48, f.ext, C_LONG, "5", 0);
    }
    /* Both arms write r10. The join keeps the later arm's tag, right, and
     * the bits are the 1 written by the taken arm. */
    {
        CFunc f = c_begin_function("gap_tags", NULL, 0, C_LONG, "right");
            CValue flag = c_local_imm("flag", C_LONG, "1");
            CValue out = c_local_imm("right", C_LONG, "0");
            CElse e = c_if_eq_else(flag, c_imm("1", C_LONG, "flag"));
                write_replace_mov("r10", (AsmDataField){ "1", "left", NULL });
            c_else(&e);
                write_replace_mov("r10", (AsmDataField){ "9", "right", NULL });
            c_end_if_else(&e);
            write_set_mov(c_cell(out), (AsmDataField){ "r10", "right", NULL });
            c_ret_value(&f, out);
        c_end_function(&f);
        gap_save(49, f.ext, C_LONG, "1", 0);
    }
    /* One checksum over shl, shr, sar, rol, ror, adc, sbb, div, idiv,
     * bsf, bsr, cmove, sete, paddd, fild/fistp, cmpsb, a displacement load,
     * a scaled-index load, and a negative displacement. The total is 147. */
    {
        AsmExtern ops = write_proc("gap_ops", NULL, 0, (AsmDataField){ "rax", "sum", NULL });
        ops.volatile_mask = ASM_WIN64_VOLATILE | (1u << 6) | (1u << 7);
            write_init_mov("r8d", "0", "sum");
            write_init_mov("eax", "100", "part");
            write_shl("eax", "1");
            write_shr("eax", "2");
            write_replace_mov("cl", (AsmDataField){ "1", "cnt", NULL });
            write_sar("eax", "cl");
            write_rol("eax", "1");
            write_ror("eax", "1");
            write_stc();
            write_adc("eax", "5");
            write_clc();
            write_sbb("eax", "1");
            gap_acc();
            write_xor((AsmDataField){ "edx", "z", NULL }, (AsmDataField){ "edx", "z", NULL });
            write_replace_mov("eax", (AsmDataField){ "100", "part", NULL });
            write_replace_mov("ecx", (AsmDataField){ "7", "div", NULL });
            write_div("ecx");
            gap_acc();
            write_replace_mov("eax", (AsmDataField){ "-20", "part", NULL });
            write_replace_mov("ecx", (AsmDataField){ "6", "div", NULL });
            write_cdq();
            write_idiv("ecx");
            gap_acc();
            write_replace_mov("eax", (AsmDataField){ "40", "part", NULL });
            write_bsf("ecx", "eax");
            write_bsr("r9d", "eax");
            write_add((AsmDataField){ "r8d", "sum", NULL }, (AsmDataField){ "ecx", "bit", NULL });
            write_add((AsmDataField){ "r8d", "sum", NULL }, (AsmDataField){ "r9d", "bit", NULL });
            write_replace_mov("eax", (AsmDataField){ "1", "part", NULL });
            write_replace_mov("ecx", (AsmDataField){ "9", "part", NULL });
            write_cmp((AsmDataField){ "eax", "part", NULL }, literal("1", "part"));
            write_cmove("ecx", "eax");
            write_add((AsmDataField){ "r8d", "sum", NULL }, (AsmDataField){ "ecx", "part", NULL });
            /* add clears ZF, so this sete adds 0. The later sete follows cmpsb. */
            write_sete("r9b");
            write_movzx("eax", "r9b");
            gap_acc();
            write_replace_mov("eax", (AsmDataField){ "4", "part", NULL });
            write_movd_sse("xmm0", "eax");
            write_movd_sse("xmm1", "eax");
            write_paddd("xmm0", "xmm1");
            write_movd_sse("eax", "xmm0");
            gap_acc();
            write_fild("dword ptr gap_x87");
            write_fistp("dword ptr gap_x87");
            write_replace_mov("eax", (AsmDataField){ "dword ptr gap_x87", "part", NULL });
            gap_acc();
            write_lea("rsi", "gap_src");
            write_lea("rdi", "gap_src");
            write_cmpsb((AsmDataField){ "rdi", "dst", NULL }, (AsmDataField){ "rsi", "src", NULL });
            write_sete("r9b");
            write_movzx("eax", "r9b");
            gap_acc();
            write_replace_mov("eax", (AsmDataField){ asm_mem("dword", "gap_tab", 8), "part", NULL });
            gap_acc();
            write_replace_mov("ecx", (AsmDataField){ "1", "idx", NULL });
            write_lea("r10", "gap_tab");
            write_replace_mov("eax", (AsmDataField){ asm_mem_index("dword", "r10", "rcx", 8, 8), "part", NULL });
            gap_acc();
            write_replace_mov("eax", (AsmDataField){ asm_mem("dword", "gap_tab2", -8), "part", NULL });
            gap_acc();
            write_replace_mov("rax", (AsmDataField){ "r8", "sum", NULL });
            write_ret();
        write_endp(&ops);
        gap_save(50, ops, C_LONG, "147", 0);
    }

    cpp_class_begin("Shape");
        cpp_class_interface();
        cpp_virtual_decl("draw");
    CppClass Shape = cpp_class_end();

    cpp_class_begin("Dot");
        cpp_implements(&Shape);
        cpp_field("n", C_LONG);
        cpp_field("m", C_LONG);
        CFunc* ctor = cpp_ctor_begin();
            CValue n0 = cpp_this_field("n", C_LONG);
            c_materialize(n0.loc, "4", "n");
            c_ret(ctor);
        cpp_method_end();
        CFunc* dtor = cpp_dtor_begin();
            write_add((AsmDataField){ asm_mem("qword", "gap_dtors", 0), "drops", NULL },
                      literal("1", "drops"));
            c_ret(dtor);
        cpp_method_end();
        CFunc* draw = cpp_virtual_begin("draw");
            CValue n1 = cpp_this_field("n", C_LONG);
            c_add(&n1, c_imm("11", C_LONG, "n"));
            c_ret(draw);
        cpp_method_end();
        const CParam kpar[] = { { "k", C_LONG } };
        CFunc* addk = cpp_method_begin_params("addk", kpar, 1);
            CValue m = cpp_this_field("m", C_LONG);
            c_materialize(m.loc, c_param(addk, 1).loc, "m");
            c_ret(addk);
        cpp_method_end();
    CppClass Dot = cpp_class_end();

    /* Virtual draw adds 11 to n, so 4 becomes 15. addk stores its argument
     * in m. The inner scope's Dot is destroyed before the outer one. */
    {
        CFunc f = c_begin_function("gap_cpp", NULL, 0, C_LONG, "n");
            CppScope outer = cpp_scope_begin();
                CppObject d1 = cpp_construct(&Dot, "gap_dot1");
                cpp_vsend_obj(d1, &Shape, "draw");
                c_call(addk, c_addr("gap_dot1", "this"), literal("6", "k"));
                CppScope inner = cpp_scope_begin();
                    cpp_construct(&Dot, "gap_dot2");
                cpp_scope_end(&inner);
            cpp_scope_end(&outer);
            c_ret_value(&f, cpp_field_ref(&d1, "n", C_LONG));
        c_end_function(&f);
        gap_save(51, f.ext, C_LONG, "15", 0);
    }
    /* m is the 6 that gap_cpp's addk stored. */
    {
        CFunc f = c_begin_function("gap_cpp_m", NULL, 0, C_LONG, "m");
            CppObject d1;
            d1.sym = "gap_dot1";
            d1.rec = Dot.rec;
            c_ret_value(&f, cpp_field_ref(&d1, "m", C_LONG));
        c_end_function(&f);
        gap_save(52, f.ext, C_LONG, "6", 0);
    }
    /* Both Dots ran their destructor, so the counter is 2. */
    {
        CFunc f = c_begin_function("gap_cpp_d", NULL, 0, C_LONG, "drops");
            c_ret_value(&f, c_wrap((AsmDataField){ "gap_dtors", "drops", NULL }, C_LONG));
        c_end_function(&f);
        gap_save(53, f.ext, C_LONG, "2", 0);
    }

    cs_class_begin("IPing");
        cs_class_interface();
        cs_virtual_decl("ping");
    CsClass IPing = cs_class_end();

    cs_class_begin("Btn");
        cs_field("n");
        CFunc* ping = cs_virtual_begin("ping");
            CValue cn = cs_this_field("n", C_LONG);
            c_add(&cn, c_imm("7", C_LONG, "n"));
            c_ret(ping);
        cs_method_end();
    CsClass Btn = cs_class_end();

    cs_class_begin("Use");
        CFunc* disp = cs_method_begin("Dispose");
            write_add((AsmDataField){ asm_mem("qword", "cs_uses", 0), "uses", NULL },
                      literal("1", "uses"));
            c_ret(disp);
        cs_method_end();
    CsClass Use = cs_class_end();

    write_data();
        write_dq("gap_btn", "0");
        write_dq("", "0");
        write_dq("", c_offset(cs_mt_sym(Btn.rec)));
        write_dq("", "0");
        write_dq("gap_use", "0");
        write_dq("gap_box", "0");
        write_dq("", "0");
        write_dq("", "1");
        write_dq("", "42");
    write_code();

    /* Interface ping on gap_btn. Its method table is Btn's, so ping adds 7
     * to n. */
    {
        CFunc f = c_begin_function("gap_cs_v", NULL, 0, C_LONG, "n");
            CValue recv = c_local_imm("btn", C_PTR, "0");
            write_lea(recv.loc, "gap_btn");
            cs_vsend(recv, &IPing, "ping");
            write_lea(recv.loc, "gap_btn");
            CsObject o;
            o.ptr = recv;
            o.rec = Btn.rec;
            c_ret_value(&f, cs_field_ref(&o, "n", C_LONG));
        c_end_function(&f);
        gap_save(54, f.ext, C_LONG, "7", 0);
    }
    /* Leaving the using calls Dispose once. */
    {
        CFunc f = c_begin_function("gap_cs_u", NULL, 0, C_LONG, "uses");
            CValue p = c_local_imm("use", C_PTR, "0");
            write_lea(p.loc, "gap_use");
            CsObject o;
            o.ptr = p;
            o.rec = Use.rec;
            CsUsing u = cs_using_begin();
            cs_using_own(&u, disp, o);
            cs_using_end(&u);
            c_ret_value(&f, c_wrap((AsmDataField){ "cs_uses", "uses", NULL }, C_LONG));
        c_end_function(&f);
        gap_save(55, f.ext, C_LONG, "1", 0);
    }
    /* The type word at offset 0 of gap_tw is 7, so the cast matches. */
    {
        CFunc f = c_begin_function("gap_cs_c", NULL, 0, C_LONG, "ok");
            CValue obj = c_local_imm("obj", C_PTR, "0");
            CValue ok = c_local_imm("ok", C_LONG, "0");
            write_lea(obj.loc, "gap_tw");
            CElse c = cs_check_type(obj, 0, "7");
                c_assign_imm(&ok, "1");
            c_else(&c);
                c_assign_imm(&ok, "0");
            c_end_if_else(&c);
            c_ret_value(&f, ok);
        c_end_function(&f);
        gap_save(56, f.ext, C_LONG, "1", 0);
    }
    /* gap_btn's method-table slot is Btn's table, so the downcast matches. */
    {
        CFunc f = c_begin_function("gap_cs_d", NULL, 0, C_LONG, "ok");
            CValue obj = c_local_imm("obj", C_PTR, "0");
            CValue ok = c_local_imm("ok", C_LONG, "0");
            write_lea(obj.loc, "gap_btn");
            CElse c = cs_downcast_begin(obj, &Btn);
                c_assign_imm(&ok, "1");
            c_else(&c);
                c_assign_imm(&ok, "0");
            c_end_if_else(&c);
            c_ret_value(&f, ok);
        c_end_function(&f);
        gap_save(57, f.ext, C_LONG, "1", 0);
    }
    /* The box's type word is 1. The payload qword after the header is 42. */
    {
        CFunc f = c_begin_function("gap_cs_b", NULL, 0, C_LONG, "pay");
            CValue box = c_local_imm("box", C_PTR, "0");
            CValue pay = c_local_imm("pay", C_LONG, "0");
            write_lea(box.loc, "gap_box");
            CElse c = cs_unbox_begin(box, "1");
                write_lea(box.loc, "gap_box");
                write_replace_mov(pay.loc, (AsmDataField){ cs_mem(box.loc, 24), "pay", NULL });
            c_else(&c);
                c_assign_imm(&pay, "0");
            c_end_if_else(&c);
            c_ret_value(&f, pay);
        c_end_function(&f);
        gap_save(58, f.ext, C_LONG, "42", 0);
    }

    py_class_begin("Pair");
        py_field("n");
        py_field("m");
        CFunc* py_add = py_method_begin("add");
            py_field_add_imm(py_this_field("n", C_LONG), 5);
            c_ret(py_add);
        py_method_end();
        CFunc* py_extra = py_method_begin("extra");
            py_field_store_imm(py_this_field("m", C_LONG), "8");
            c_ret(py_extra);
        py_method_end();
    PyClass Pair = py_class_end();
    PyObject pair = py_construct(&Pair, "pair");
    (void)pair;

    /* The quiet procedure swallows the refcount message. The four procedures
     * check a present method, a second method, a missing name, and the count. */
    {
        static const AsmDataField pr[] = { { "rcx", "lpBuffer", NULL }, { "edx", "nBytes", NULL } };
        AsmExtern quiet = write_proc("gap_py_quiet", pr, 2, asm_void);
            write_ret();
        write_endp(&quiet);
        AsmExtern incf, decf;
        py_emit_refcount_ops(&quiet, gap_py_msg, gap_py_msg, &incf, &decf);

        /* Dispatch of add adds 5 to n. */
        {
        CFunc fn = c_begin_function("gap_py_n", NULL, 0, C_LONG, "n");
            CValue self = c_into("rcx", C_PTR, c_imm("0", C_PTR, "self"), "self");
            write_lea("rcx", "pair");
            py_dispatch_name(self, "add");
            c_ret_value(&fn, py_field_ref(&pair, "n", C_LONG));
        c_end_function(&fn);
        gap_save(59, fn.ext, C_LONG, "5", 0);
        }

        /* Dispatch of extra stores 8 in m. */
        {
        CFunc fm = c_begin_function("gap_py_m", NULL, 0, C_LONG, "m");
            CValue self = c_into("rcx", C_PTR, c_imm("0", C_PTR, "self"), "self");
            write_lea("rcx", "pair");
            py_dispatch_name(self, "extra");
            c_ret_value(&fm, py_field_ref(&pair, "m", C_LONG));
        c_end_function(&fm);
        gap_save(60, fm.ext, C_LONG, "8", 0);
        }

        /* The name nope is absent, so the checked dispatch takes the else
         * arm and miss becomes 1. */
        {
        CFunc fmiss = c_begin_function("gap_py_miss", NULL, 0, C_LONG, "miss");
            CValue self = c_into("rcx", C_PTR, c_imm("0", C_PTR, "self"), "self");
            /* py_dispatch_checked rewrites rax. miss stays in the next register. */
            CValue hold = c_local_imm("hold", C_LONG, "0");
            CValue miss = c_local_imm("miss", C_LONG, "0");
            (void)hold;
            write_lea("rcx", "pair");
            CElse arm = py_dispatch_checked_name(self, "nope");
                c_assign_imm(&miss, "0");
            c_else(&arm);
                c_assign_imm(&miss, "1");
            c_end_if_else(&arm);
            c_ret_value(&fmiss, miss);
        c_end_function(&fmiss);
        gap_save(61, fmiss.ext, C_LONG, "1", 0);
        }

        /* One increment and two decrements leave the refcount at 0. */
        CFunc fr = c_begin_function("gap_py_ref", NULL, 0, C_LONG, "refcount");
            c_call_proc_void(&incf, c_addr("pair", "self"));
            c_call_proc_void(&decf, c_addr("pair", "self"));
            c_call_proc_void(&decf, c_addr("pair", "self"));
            c_ret_value(&fr, c_wrap((AsmDataField){ "pair", "refcount", NULL }, C_LONG));
        c_end_function(&fr);
        gap_save(62, fr.ext, C_LONG, "0", 0);
    }

    js_class_begin("Greeter");
        js_field("n");
        CFunc* greet = js_method_begin("greet");
            js_field_add_imm(js_this_field("n", C_LONG), 3);
            c_ret(greet);
        js_method_end();
    JsClass Greeter = js_class_end();
    js_class_begin("Kid");
        js_field("n");
    JsClass Kid = js_class_end();
    JsObject base = js_construct(&Greeter, "gap_base", NULL);
    JsObject kid = js_construct(&Kid, "gap_kid", "gap_base");
    (void)base;
    js_emit_runtime();

    /* kid finds greet on its prototype and the call adds 3 to the child's n. */
    {
        CFunc f = c_begin_function("gap_js_g", NULL, 0, C_LONG, "n");
            CValue obj = js_word_lea("kid", "gap_kid");
            CValue fn = js_lookup(obj, "greet");
            js_call_fn(fn, obj);
            c_ret_value(&f, js_field_ref(&kid, "n", C_LONG));
        c_end_function(&f);
        gap_save(63, f.ext, C_LONG, "3", 0);
    }
    /* A lookup of a name that is not on the object or its prototype is 0. */
    {
        CFunc f = c_begin_function("gap_js_m", NULL, 0, C_LONG, "fn");
            CValue obj = js_word_lea("kid", "gap_kid");
            c_ret_value(&f, js_lookup(obj, "missing"));
        c_end_function(&f);
        gap_save(64, f.ext, C_LONG, "0", 0);
    }
    /* js_set of extra is the value lookup returns. */
    {
        CFunc f = c_begin_function("gap_js_s", NULL, 0, C_LONG, "fn");
            CValue obj = js_word_lea("kid", "gap_kid");
            CValue val = c_local_imm("xv", C_LONG, "21");
            js_set(obj, "extra", val);
            c_ret_value(&f, js_lookup(obj, "extra"));
        c_end_function(&f);
        gap_save(65, f.ext, C_LONG, "21", 0);
    }
    /* Truthiness of 42, 0, an empty string, "hi", null, and an object.
     * The matching arms add 1, 2, 4, 8, 16, and 32. */
    {
        CFunc f = c_begin_function("gap_js_t", NULL, 0, C_LONG, "acc");
            CValue acc = c_local_imm("acc", C_LONG, "0");
            CValue n42 = js_number("n42", 42);
            CElse t1 = js_if_truthy_else(n42);
                c_add(&acc, c_imm("1", C_LONG, "acc"));
            c_else(&t1);
            c_end_if_else(&t1);
            c_free(&n42);
            CValue n0 = js_number("n0", 0);
            CElse t2 = js_if_truthy_else(n0);
            c_else(&t2);
                c_add(&acc, c_imm("2", C_LONG, "acc"));
            c_end_if_else(&t2);
            c_free(&n0);
            CValue empty = js_string_val("empty", "js_empty");
            CElse t3 = js_if_truthy_else(empty);
            c_else(&t3);
                c_add(&acc, c_imm("4", C_LONG, "acc"));
            c_end_if_else(&t3);
            c_free(&empty);
            CValue hi = js_string_val("hi", "js_hi");
            CElse t4 = js_if_truthy_else(hi);
                c_add(&acc, c_imm("8", C_LONG, "acc"));
            c_else(&t4);
            c_end_if_else(&t4);
            c_free(&hi);
            CValue nil = c_local_imm("nil", C_PTR, "0");
            CElse t5 = js_if_truthy_else(nil);
            c_else(&t5);
                c_add(&acc, c_imm("16", C_LONG, "acc"));
            c_end_if_else(&t5);
            c_free(&nil);
            CValue obj = js_word_lea("obj", "js_nodes");
            CElse t6 = js_if_truthy_else(obj);
                c_add(&acc, c_imm("32", C_LONG, "acc"));
            c_else(&t6);
            c_end_if_else(&t6);
            c_free(&obj);
            c_ret_value(&f, acc);
        c_end_function(&f);
        gap_save(66, f.ext, C_LONG, "63", 0);
    }

    /* Drop counts. The nested scope drops one binding. Two live bindings
     * both drop. Two moves drop nothing. The total is 1+2+0. */
    {
        CFunc f = c_begin_function("gap_rust", NULL, 0, C_LONG, "acc");
            CValue acc = c_stack_local("acc", C_LONG, ALIGN_C);
            c_materialize(acc.loc, "0", "acc");
            write_set_mov((AsmDataField){ "rust_drops", "drops", NULL }, literal("0", "drops"));
            /* The inner scope ends first and drops its one binding. */
            {
                RustScope outer = rust_scope_begin();
                    RustScope inner = rust_scope_begin();
                        RustVal a;
                        rust_let(&inner, &a, "na", "obj_a", dropfn);
                    rust_scope_end(&inner);
                rust_scope_end(&outer);
                CValue t = c_dup("t", C_LONG, c_wrap((AsmDataField){ "rust_drops", "drops", NULL }, C_LONG));
                c_add(&acc, t);
                c_free(&t);
                write_set_mov((AsmDataField){ "rust_drops", "drops", NULL }, literal("0", "drops"));
            }
            /* Both bindings are still owned at the end of the scope, so both
             * drop. rust_use checks that c is still live. */
            {
                RustScope s = rust_scope_begin();
                    RustVal b, c;
                    rust_let(&s, &b, "nb", "obj_a", dropfn);
                    rust_let(&s, &c, "nc", "obj_b", dropfn);
                    (void)rust_use(&c);
                rust_scope_end(&s);
                CValue t = c_dup("t", C_LONG, c_wrap((AsmDataField){ "rust_drops", "drops", NULL }, C_LONG));
                c_add(&acc, t);
                c_free(&t);
                write_set_mov((AsmDataField){ "rust_drops", "drops", NULL }, literal("0", "drops"));
            }
            /* Both bindings are moved out, so the scope end emits no Drop. */
            {
                RustScope s = rust_scope_begin();
                    RustVal b, c;
                    rust_let(&s, &b, "mb", "obj_a", dropfn);
                    rust_let(&s, &c, "mc", "obj_b", dropfn);
                    rust_move(&b);
                    (void)rust_use(&c);
                    rust_move(&c);
                rust_scope_end(&s);
                CValue t = c_dup("t", C_LONG, c_wrap((AsmDataField){ "rust_drops", "drops", NULL }, C_LONG));
                c_add(&acc, t);
                c_free(&t);
            }
            c_ret_value(&f, acc);
        c_end_function(&f);
        gap_save(67, f.ext, C_LONG, "3", 0);
    }
    /* Ok falls through. Err skips the store of 1, and the catch stores 9. */
    {
        CFunc f = c_begin_function("gap_rtry", NULL, 0, C_LONG, "r");
            CValue r = c_stack_local("r", C_LONG, ALIGN_C);
            c_materialize(r.loc, "0", "r");
            c_call(checked);
            RustCatch cat = rust_catch_begin();
                rust_try(&cat);
                c_call(erred);
                rust_try(&cat);
                write_set_mov(c_cell(r), literal("1", "r"));
            rust_catch(&cat);
                write_set_mov(c_cell(r), literal("9", "r"));
            rust_catch_end(&cat);
            c_ret_value(&f, r);
        c_end_function(&f);
        gap_save(68, f.ext, C_LONG, "9", 0);
    }

    /* One-armed ult, slt, and ne. Taken arms add 1, 4, 32, and 64.
     * The skipped arms would have added 2, 8, 16, and 128. The sum is 101.
     * Unsigned -1 is greater than 1, so that ult is skipped. Signed -1 is
     * less than 1, so that slt is taken. */
    {
        CFunc f = c_begin_function("gap_ifs", NULL, 0, C_LONG, "acc");
            CValue acc = c_local_imm("acc", C_LONG, "0");
            CValue one = c_local_imm("one", C_LONG, "1");
            CValue two = c_local_imm("two", C_LONG, "2");
            CValue neg = c_local_imm("neg", C_LONG, "-1");
            CIf u1 = c_if_ult(one, two);
                c_add(&acc, c_imm("1", C_LONG, "acc"));
            c_end_if(&u1);
            CIf u2 = c_if_ult(two, one);
                c_add(&acc, c_imm("2", C_LONG, "acc"));
            c_end_if(&u2);
            CIf s1 = c_if_slt(one, two);
                c_add(&acc, c_imm("4", C_LONG, "acc"));
            c_end_if(&s1);
            CIf s2 = c_if_slt(two, one);
                c_add(&acc, c_imm("8", C_LONG, "acc"));
            c_end_if(&s2);
            CIf u3 = c_if_ult(neg, one);
                c_add(&acc, c_imm("16", C_LONG, "acc"));
            c_end_if(&u3);
            CIf s3 = c_if_slt(neg, one);
                c_add(&acc, c_imm("32", C_LONG, "acc"));
            c_end_if(&s3);
            CIf n1 = c_if_ne(one, two);
                c_add(&acc, c_imm("64", C_LONG, "acc"));
            c_end_if(&n1);
            CIf n2 = c_if_ne(one, one);
                c_add(&acc, c_imm("128", C_LONG, "acc"));
            c_end_if(&n2);
            c_ret_value(&f, acc);
        c_end_function(&f);
        gap_save(69, f.ext, C_LONG, "101", 0);
    }
    /* gap_slide_big starts on the next page. The difference of the two
     * labels is 4096. */
    {
        AsmExtern slide = write_proc("gap_slidep", NULL, 0, (AsmDataField){ "rax", "off", NULL });
        slide.volatile_mask = ASM_WIN64_VOLATILE;
            write_lea("r8", "gap_slide_big");
            write_lea("rcx", "gap_slide");
            write_sub((AsmDataField){ "r8", "off", NULL }, (AsmDataField){ "rcx", "base", NULL });
            write_replace_mov("rax", (AsmDataField){ "r8", "off", NULL });
            write_ret();
        write_endp(&slide);
        gap_save(70, slide, C_LONG, "4096", 0);
    }
    /* Twenty values fill the fourteen registers and spill six at once.
     * Freeing three that are still in registers opens holes, and 101, 102,
     * and 103 reuse those holes. After the clobber the survivors sum to 495. */
    {
        CFunc f = c_begin_function("gap_spilln", NULL, 0, C_LONG, "acc");
            CValue vs[20];
            int i;
            for (i = 0; i < 20; i++) {
                char nb[16], ib[16];
                snprintf(nb, sizeof nb, "s%d", i);
                snprintf(ib, sizeof ib, "%d", i + 1);
                vs[i] = c_local_imm(c_strdup_arena(nb), C_LONG, c_strdup_arena(ib));
            }
            c_free(&vs[2]);
            c_free(&vs[5]);
            c_free(&vs[11]);
            CValue h0 = c_local_imm("h0", C_LONG, "101");
            CValue h1 = c_local_imm("h1", C_LONG, "102");
            CValue h2 = c_local_imm("h2", C_LONG, "103");
            c_call(clobber);
            CValue acc = c_local_imm("acc", C_LONG, "0");
            for (i = 0; i < 20; i++) {
                if (i == 2 || i == 5 || i == 11) continue;
                c_add(&acc, vs[i]);
            }
            c_add(&acc, h0);
            c_add(&acc, h1);
            c_add(&acc, h2);
            c_ret_value(&f, acc);
        c_end_function(&f);
        gap_save(71, f.ext, C_LONG, "495", 0);
    }
    /* CreateFileA takes seven arguments, so the last three travel on the
     * stack. WriteFile's overlapped pointer is the fifth argument and also
     * travels on the stack. A real handle that accepts four bytes returns 1. */
    {
        const char* cf_tags[] = {
            "lpFileName", "dwDesiredAccess", "dwShareMode", "lpSecurityAttributes",
            "dwCreationDisposition", "dwFlagsAndAttributes", "hTemplateFile"
        };
        const char* wf_tags[] = {
            "hFile", "lpBuffer", "nNumberOfBytesToWrite",
            "lpNumberOfBytesWritten", "lpOverlapped"
        };
        const char* cl_tags[] = { "hObject" };
        AsmExtern createf = c_extern("CreateFileA", cf_tags, 7, C_PTR, "hFile");
        AsmExtern writef = c_extern("WriteFile", wf_tags, 5, C_INT, "wrote");
        AsmExtern closef = c_extern("CloseHandle", cl_tags, 1, C_INT, NULL);
        CFunc f = c_begin_function("gap_wfile", NULL, 0, C_LONG, "ok");
            CValue h = c_call_ext(C_PTR, &createf,
                c_addr("gap_wname", "lpFileName"),
                literal("1073741824", "dwDesiredAccess"),
                literal("0", "dwShareMode"),
                literal("0", "lpSecurityAttributes"),
                literal("2", "dwCreationDisposition"),
                literal("128", "dwFlagsAndAttributes"),
                literal("0", "hTemplateFile"));
            CValue ok = c_local_imm("ok", C_LONG, "0");
            CIf good = c_if_ne(h, c_imm("-1", C_PTR, "hFile"));
                CValue wrote = c_call_ext(C_INT, &writef,
                    c_cell(h),
                    c_addr("gap_wbytes", "lpBuffer"),
                    literal("4", "nNumberOfBytesToWrite"),
                    c_addr("gap_wn", "lpNumberOfBytesWritten"),
                    literal("0", "lpOverlapped"));
                CIf wok = c_if_ne(wrote, c_imm("0", C_INT, "wrote"));
                    CValue nwr = c_wrap((AsmDataField){ "gap_wn", "nwritten", NULL }, C_LONG);
                    CIf n4 = c_if_eq(nwr, c_imm("4", C_LONG, "nwritten"));
                        c_assign_imm(&ok, "1");
                    c_end_if(&n4);
                c_end_if(&wok);
                c_call_ext_void(&closef, c_treat_as(h, "hFile", "hObject"));
            c_end_if(&good);
            c_ret_value(&f, ok);
        c_end_function(&f);
        gap_save(72, f.ext, C_LONG, "1", 0);
    }

    cpp_class_begin("Ink");
        cpp_class_interface();
        cpp_virtual_decl("mark");
    CppClass Ink = cpp_class_end();

    cpp_class_begin("Pen");
        cpp_implements(&Shape);
        cpp_implements(&Ink);
        cpp_field("n", C_LONG);
        cpp_field("k", C_LONG);
        CFunc* pen_ctor = cpp_ctor_begin();
            CValue pn = cpp_this_field("n", C_LONG);
            c_materialize(pn.loc, "1", "n");
            CValue pk = cpp_this_field("k", C_LONG);
            c_materialize(pk.loc, "2", "k");
            c_ret(pen_ctor);
        cpp_method_end();
        CFunc* pen_draw = cpp_virtual_begin("draw");
            CValue pd = cpp_this_field("n", C_LONG);
            c_add(&pd, c_imm("4", C_LONG, "n"));
            c_ret(pen_draw);
        cpp_method_end();
        CFunc* pen_mark = cpp_virtual_begin("mark");
            CValue pm = cpp_this_field("k", C_LONG);
            c_add(&pm, c_imm("5", C_LONG, "k"));
            c_ret(pen_mark);
        cpp_method_end();
    CppClass Pen = cpp_class_end();

    /* Shape.draw adds 4 to n. Ink.mark adds 5 to k. 1+4 and 2+5 sum to 12.
     * Pen has no destructor, so nothing is added to gap_dtors. */
    {
        CFunc f = c_begin_function("gap_cpp2", NULL, 0, C_LONG, "n");
            CppObject pen = cpp_construct(&Pen, "gap_pen");
            cpp_vsend_obj(pen, &Shape, "draw");
            cpp_vsend_obj(pen, &Ink, "mark");
            CValue n = c_dup("n", C_LONG, cpp_field_ref(&pen, "n", C_LONG));
            c_add(&n, cpp_field_ref(&pen, "k", C_LONG));
            c_ret_value(&f, n);
        c_end_function(&f);
        gap_save(73, f.ext, C_LONG, "12", 0);
    }

    cpp_class_begin("Stamp");
        cpp_field("n", C_LONG);
        CFunc* stamp_ctor = cpp_ctor_begin();
            CValue sn = cpp_this_field("n", C_LONG);
            c_materialize(sn.loc, "9", "n");
            c_ret(stamp_ctor);
        cpp_method_end();
    CppClass Stamp = cpp_class_end();

    /* cpp_stack_construct builds Stamp at [rsp]. The field is the first
     * qword because Stamp has no vptr. The copy is spilled before the scope
     * releases that reservation, and the return is 9. */
    {
        CFunc f = cpp_begin_function("gap_stamp", NULL, 0, C_LONG, "n");
            CppScope sc = cpp_scope_begin();
                cpp_stack_construct(&Stamp);
                CValue n = c_local_imm("n", C_LONG, "0");
                write_replace_mov(c_resolve(n).loc, (AsmDataField){ "qword ptr [rsp]", "n", NULL });
                c_spill(&n);
            cpp_scope_end(&sc);
            c_ret_value(&f, n);
        cpp_end_function(&f);
        gap_save(74, f.ext, C_LONG, "9", 0);
    }
    /* js_set's id argument is a register holding the interned name, not the
     * literal the js_set helper bakes in. The stored value is 17. */
    {
        int id = js_name_id("rtname");
        char idbuf[16];
        snprintf(idbuf, sizeof idbuf, "%d", id);
        const char* idlit = c_strdup_arena(idbuf);
        CFunc f = c_begin_function("gap_js_rt", NULL, 0, C_LONG, "fn");
            CValue obj = js_word_lea("kid", "gap_kid");
            CValue idn = c_local_imm("idn", C_LONG, "0");
            c_add(&idn, c_imm(idlit, C_LONG, "idn"));
            CValue val = c_local_imm("rv", C_LONG, "17");
            c_call_proc_void(&js_set_fn,
                treat_as(c_cell(obj), obj.tag, "obj"),
                treat_as(c_cell(idn), idn.tag, "id"),
                treat_as(c_cell(val), val.tag, "value"));
            c_ret_value(&f, js_lookup(obj, "rtname"));
        c_end_function(&f);
        gap_save(75, f.ext, C_LONG, "17", 0);
    }

    gap_emit_catalogue();
}

/* Call each saved procedure from main and beh_check it. code is main's exit word. */
static void gap_emit_checks(CValue* code) {
    for (int id = 21; id <= 75; id++) {
        char buf[8];
        CValue got;
        if (gap_exp[id] == NULL) continue;
        snprintf(buf, sizeof buf, "%d", id);
        if (gap_kind[id] == 1) {
            got = c_call_proc(gap_ty[id], &gap_ext[id], literal("6", "a"));
        } else if (gap_kind[id] == 2) {
            got = c_call_proc(gap_ty[id], &gap_ext[id],
                              literal("10", "a"), literal("20", "b"), literal("30", "c"));
        } else if (gap_kind[id] == 3) {
            got = c_call_ext(gap_ty[id], &gap_ext[id],
                             literal("1", "a"), literal("2", "b"), literal("3", "c"),
                             literal("4", "d"), literal("5", "e"), literal("77", "f"));
        } else {
            got = c_call_proc(gap_ty[id], &gap_ext[id]);
        }
        beh_check(code, asm_strdup(buf), got, gap_exp[id]);
    }
}

/* write_replace_mov of a data global is a generation error. */
static void fail_repurpose(void) {
    write_unitialised_data("gap_repurpose", "word");
    write_replace_mov("gap_repurpose", (AsmDataField){ "1", "word", NULL });
}

/* write_set_mov whose tag is not the tag the register holds. */
static void fail_set_tag(void) {
    AsmExtern p = write_proc("gap_set_bad", NULL, 0, asm_void);
    write_init_mov("rax", "1", "sum");
    write_set_mov((AsmDataField){ "rax", "other", NULL }, literal("2", "other"));
    write_endp(&p);
}

/* Spill off: a call inside one arm poisons the local at the join. */
static void fail_poison_join(void) {
    AsmExtern dummy = write_proc("gap_poison_j", NULL, 0, asm_void);
    dummy.volatile_mask = ASM_WIN64_VOLATILE;
    write_ret();
    write_endp(&dummy);
    c_spill_off();
    CFunc f = c_begin_function("gap_poison_user", NULL, 0, C_LONG, NULL);
        CValue n = c_local_imm("kept", C_LONG, "7");
        CValue flag = c_local_imm("flag", C_LONG, "1");
        CIf open = c_if_eq(flag, c_imm("1", C_LONG, "flag"));
            c_call_proc_void(&dummy);
        c_end_if(&open);
        (void)c_cell(n);
    c_end_function(&f);
}

/* The dictionary keeps the later arm's tag, so the earlier tag is rejected. */
static void fail_tag_join(void) {
    CFunc f = c_begin_function("gap_tag_bad", NULL, 0, C_LONG, "right");
        CValue flag = c_local_imm("flag", C_LONG, "1");
        CValue out = c_local_imm("right", C_LONG, "0");
        CElse e = c_if_eq_else(flag, c_imm("1", C_LONG, "flag"));
            write_replace_mov("r10", (AsmDataField){ "1", "left", NULL });
        c_else(&e);
            write_replace_mov("r10", (AsmDataField){ "9", "right", NULL });
        c_end_if_else(&e);
        write_set_mov(c_cell(out), (AsmDataField){ "r10", "left", NULL });
        c_ret_value(&f, out);
    c_end_function(&f);
}

/* One more attribute name than PY_MAX_ATTRS. The names are fresh so the
 * success program's intern table is what overflows. */
static void fail_py_attrs(void) {
    (void)py_attr("gap_a4");
    (void)py_attr("gap_a5");
}

/* The bump cursor is already at the end of the arena, so one more byte fails. */
static void fail_arena(void) {
    asm_bump_offset = ASM_BUMP_ARENA_SIZE;
    (void)asm_bump_alloc(1);
}

/* The only field starts at offset 8 while the fill cursor is still 0. */
static void fail_layout(void) {
    create_memory_layout(1, "gap_bad_layout");
    set_memory_layout_entry(0, "", "0", "pad", 8, 8);
    (void)write_data_memory_layout(asm_current_layout);
}

/* gap_none_ref has no member by this name. */
static void fail_c_field(void) {
    (void)c_field(&gap_none_ref, "no_such", C_LONG);
}

/* Generation failures that belong with the gap procedures. */
static void gap_expect_fails(void) {
    size_t saved_off = asm_bump_offset;
    AsmMemoryLayout* saved_layout = asm_current_layout;
    beh_expect_fail("cannot repurpose", fail_repurpose);
    beh_expect_fail("location holds", fail_set_tag);
    beh_expect_fail("clobbered by the call", fail_poison_join);
    beh_expect_fail("expected \"left\"", fail_tag_join);
    beh_expect_fail("too many distinct attributes", fail_py_attrs);
    beh_expect_fail("out of arena", fail_arena);
    asm_bump_offset = saved_off;
    beh_expect_fail("does not line up", fail_layout);
    asm_bump_offset = saved_off;
    asm_current_layout = saved_layout;
    beh_expect_fail("no field", fail_c_field);
}
