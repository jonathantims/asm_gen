/*
 * ex5_layout.c - generator for the representation-selection example.
 *
 *   EX_ENABLE defined   -> rep_use_parallel(): one array per field. The
 *     parallel-only token is the dense label Monster_hp.
 *
 *   EX_ENABLE undefined -> rep_use_hierarchy(): contiguous objects
 *     (Monster_data, Weapon_data, Pet_data). Monster_hp is absent.
 *
 * The generator names fields and objects. It does not mention a representation.
 * Both builds print the same sums and exit 0.
 *
 *   hp, atk     required, single
 *   mark        multiple (add; several values on one monster)
 *   damage      required, single child, one weapon per monster
 *   loyalty     optional, single child (-1 = that monster has no pet)
 *
 * Rows: monsters (30, 5), (50, 9), (20, 3); weapons 2, 4, 1; pets 3, absent, 5.
 * Sums: hp 100, atk 17, damage 7, loyalty 8, mark 5.
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex5_layout.h"

/* Print one sum and exit 1 when it is not expect.
 *   fn     a generated sum procedure; its result is the sum.
 *   expect the decimal the sum must equal.
 *   label  the text printed before the number.
 *   print  the console writer used by ex_puts.
 *   pu64   print_u64. It takes one value tagged "value" and writes
 *          the unsigned decimal with no newline. The call keeps the sum,
 *          so the sum does not need a second copy.
 *   nl     the newline printed after the number.
 *   con    the console, used if the sum is wrong.
 */
static void show_sum(AsmExtern* fn, long long expect, ExStr label,
                     AsmExtern* print, AsmExtern* pu64, ExStr nl, ExConsole* con) {
    CValue sum = c_call_proc(C_PTR, fn);
    ex_puts(print, label);
    ex_print_val(pu64, sum);
    ex_puts(print, nl);
    CElse miss = c_if_ne_else(sum, c_imm_n(expect, C_PTR, "sum"));
        ex_exit(con, 1);
    c_else(&miss);
    c_end_if_else(&miss);
    c_free(&sum);
}

/* Generate the representation-layout example into ex5_output.asm.
 * Both representations are built to print the same field sums and exit 0.
 * A sum or loyalty check that misses its expected value exits 1. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex5_output.asm");
#ifdef EX_ENABLE
    rep_use_parallel();
#endif
    ExConsole con = ex_console_declare();

    /* hp and atk are required and single: every monster has both.
     * mark is multiple: a monster may carry several, and add is the writer.
     * Missing mark values are not stored.
     *
     * rep_begin names the class. rep_field(class, name, optional, multiple):
     * optional is 0 when every object has a value and 1 when a negative
     * value means "absent". multiple is 0 for one slot (set_dobject_field)
     * and 1 for a list (add_dobject_field). */
    RepClass monster = rep_begin("Monster");
    RepField* hp   = rep_field(&monster, "hp",   0, 0);
    RepField* atk  = rep_field(&monster, "atk",  0, 0);
    RepField* mark = rep_field(&monster, "mark", 0, 1);

    /* damage is a required single field of a required single child.
     * One weapon row lines up with one monster row. */
    RepClass weapon = rep_begin("Weapon");
    RepField* damage = rep_field(&weapon, "damage", 0, 0);
    rep_child(&monster, &weapon, 0, 0);

    /* loyalty is an optional single field of an optional single child.
     * A negative value means that monster has no pet. Parallel storage omits
     * it and keeps the parent-row index; object storage keeps the -1 in the
     * object so a crawl can see the absence. */
    RepClass pet = rep_begin("Pet");
    RepField* loyalty = rep_field(&pet, "loyalty", 1, 0);
    rep_child(&monster, &pet, 1, 0);

    /* First monster: 30 hit points, attack 5, two marks (1 and 4).
     * Its weapon does 2 damage. Its pet's loyalty is 3. */
    RepObject m0 = new_dobject(&monster);
    set_dobject_field(m0, hp, 30);
    set_dobject_field(m0, atk, 5);
    add_dobject_field(m0, mark, 1);
    add_dobject_field(m0, mark, 4);
    RepObject w0 = new_dobject(&weapon);
    set_dobject_field(w0, damage, 2);
    set_child(m0, w0);
    RepObject p0 = new_dobject(&pet);
    set_dobject_field(p0, loyalty, 3);
    set_child(m0, p0);

    /* Second monster: 50 hit points, attack 9, no marks.
     * Its weapon does 4 damage. No pet is attached, so that row is absent. */
    RepObject m1 = new_dobject(&monster);
    set_dobject_field(m1, hp, 50);
    set_dobject_field(m1, atk, 9);
    RepObject w1 = new_dobject(&weapon);
    set_dobject_field(w1, damage, 4);
    set_child(m1, w1);

    /* Third monster: 20 hit points, attack 3. Weapon damage 1. Loyalty 5. */
    RepObject m2 = new_dobject(&monster);
    set_dobject_field(m2, hp, 20);
    set_dobject_field(m2, atk, 3);
    RepObject w2 = new_dobject(&weapon);
    set_dobject_field(w2, damage, 1);
    set_child(m2, w2);
    RepObject p2 = new_dobject(&pet);
    set_dobject_field(p2, loyalty, 5);
    set_child(m2, p2);

        ExStr sHp = ex_line("s_hp", "sum hp  = ", 0, 0);
        ExStr sAt = ex_line("s_at", "sum atk = ", 0, 0);
        ExStr sDm = ex_line("s_dm", "sum dmg = ", 0, 0);
        ExStr sLy = ex_line("s_ly", "sum loy = ", 0, 0);
        ExStr sMk = ex_line("s_mk", "sum mrk = ", 0, 0);
        ExStr sNl = ex_line("s_nl", "", 2, 0);
        rep_emit_all(&monster);
        ex_console_globals();
        AsmExtern print = ex_console_print(&con);
        AsmExtern pu64  = ex_print_u64_def(&print);

        /* A sum and an indexed load. A filter or a join would be the same
         * walk with a different body; this is the proof of concept. */
        AsmExtern sum_hp  = rep_gen_sum(hp);
        AsmExtern sum_atk = rep_gen_sum(atk);
        AsmExtern sum_dmg = rep_gen_sum(damage);
        AsmExtern sum_loy = rep_gen_sum(loyalty);
        AsmExtern sum_mrk = rep_gen_sum(mark);
        AsmExtern at_loy  = rep_gen_at(loyalty);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            show_sum(&sum_hp,  100, sHp, &print, &pu64, sNl, &con);
            show_sum(&sum_atk, 17,  sAt, &print, &pu64, sNl, &con);
            show_sum(&sum_dmg, 7,   sDm, &print, &pu64, sNl, &con);
            show_sum(&sum_loy, 8,   sLy, &print, &pu64, sNl, &con);
            show_sum(&sum_mrk, 5,   sMk, &print, &pu64, sNl, &con);
            /* Row 0 has loyalty 3. Row 1 is absent in both representations. */
            rep_expect(&at_loy, "0", "3", &con);
            rep_expect(&at_loy, "1", "-1", &con);

            /* The same walk, with the body written here. Each row yields the
             * two field values RepGen provides. 35 + 59 + 23 = 117. Object
             * storage reaches those fields by row and slot; parallel storage
             * indexes the two arrays with the same row. */
            CValue pair = c_local_imm("pair", C_PTR, "0");
            RepGen pairg = begin_rep_gen_block(hp, atk);
                c_add(&pair, pairg.a);
                c_add(&pair, pairg.b);
            end_rep_gen_block(&pairg);
            CElse pair_miss = c_if_ne_else(pair, c_imm("117", C_PTR, "pair"));
                ex_exit(&con, 1);
            c_else(&pair_miss);
            c_end_if_else(&pair_miss);
            c_free(&pair);

            /* Not a sum: keep the loyalty of row 0. Both representations
             * produce 3. A missing pet is -1 and is not row 0. */
            CValue loy0 = c_local_imm("loy0", C_PTR, "-1");
            RepGen loyg = begin_rep_gen_block1(loyalty);
                CElse skip = c_if_ne_else(loyg.row, c_imm("0", C_PTR, "row"));
                c_else(&skip);
                    c_assign(&loy0, loyg.a);
                c_end_if_else(&skip);
            end_rep_gen_block(&loyg);
            CElse loy_miss = c_if_ne_else(loy0, c_imm("3", C_PTR, "loyalty"));
                ex_exit(&con, 1);
            c_else(&loy_miss);
            c_end_if_else(&loy_miss);
            c_free(&loy0);

            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
