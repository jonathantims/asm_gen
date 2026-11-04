/*
 * ex12_quantisation.c - one application of the quantisation library.
 *
 * ex12_quantisation.h is the library. It stores an expression tree and
 * emits that tree either as an ordinary float function or as a lookup of
 * precomputed tuples. This file is the application. It writes the skill
 * update, names the two axes, plays three games, and prints the result.
 * EX_ENABLE is the only switch, so the same source builds both programs.
 *
 *   EX_ENABLE defined   -> update_skills is a lookup. Each input becomes a
 *     bucket index, the pair of indices selects one precomputed
 *     (mean, variance), and that tuple is the result. Exit 0.
 *
 *   EX_ENABLE undefined -> update_skills is an ordinary float function.
 *     q_update is absent. Exit 0.
 *
 * A skill here is a mean and a variance. A game has a winner and a loser.
 * update_skills rewrites the winner only. The loser changes only when a
 * later call passes that player in as the winner, which is what the third
 * game does. The numbers printed are the means and the variances times
 * 100, so a mean of 17.25 is the line "mean A x100 = 1725".
 */
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "ex_console.h"
#include "ex12_quantisation.h"

/*
 * The winner's new (mean, variance).
 *
 * Input 0 is the winner's mean before the game. Input 1 is the opponent's
 * mean. The result is a tuple of two floats, in this order: the winner's
 * mean after the game, then the winner's variance. The opponent is not in
 * the tuple. Updating the opponent is a second q_invoke with the two
 * players exchanged.
 *
 *     gap  = other - prior
 *     mean = prior + 1 + gap / 4
 *     var  = 4 / (1 + gap * gap / 16)
 *
 * The mean steps up by one, and then by a quarter of the gap. A win
 * against a stronger opponent (a positive gap) moves the mean further. A
 * win against a weaker opponent (a negative gap) moves it by less than
 * one, and the mean falls when the gap is below -4. The variance is 4
 * when the two means are equal, and it falls toward 0 as the gap grows
 * in either direction. That is the shape of a gaussian skill update: a
 * surprising result moves the estimate more, and a large disagreement is
 * already explained by the gap,
 * so the variance shrinks. The tree uses only add, subtract, multiply, and
 * divide, which is the whole arithmetic the library records.
 *
 * q_operation_begin names the plain function. Each helper below appends
 * one node and returns that node's index. q_lit stores a constant. q_out
 * appends one float of the tuple, so the first q_out is the mean and the
 * second is the variance. q_operation_end checks that every declared axis
 * is actually an input, and, when the grid is on, evaluates this tree at
 * the midpoint of every bucket.
 *
 * The QOperation this returns is the handle q_invoke runs. Building it
 * records the tree in the library and does not emit instructions.
 * q_emit writes the constants, or the output array and the lookup, and
 * the procedure named here when the grid is off. The grid build writes
 * no procedure.
 */
static QOperation update_skills(void) {
    q_operation_begin("update_skills");
        QVal prior = q_input(0);
        QVal other = q_input(1);
        QVal gap   = q_sub(other, prior);
        QVal step  = q_div(gap, q_lit(4.0));
        QVal bump  = q_add(q_lit(1.0), step);
        QVal mean  = q_add(prior, bump);
        QVal gap2  = q_mul(gap, gap);
        QVal wide  = q_div(gap2, q_lit(16.0));
        QVal var   = q_div(q_lit(4.0), q_add(q_lit(1.0), wide));
        q_out(mean);
        q_out(var);
    return q_operation_end();
}

/*
 * One game, from the winner's side. The two inputs are the winner's mean
 * and the opponent's mean. The two outputs are stored back over the
 * winner's mean and the winner's variance. q_invoke is that call. With
 * the grid off it loads the two means, calls update_skills, and stores
 * the tuple. With the grid on it converts each mean to a bucket index,
 * joins the indices through the lookup, and loads the stored tuple.
 */
static void play(QOperation* update_operation, QValue winner, QValue loser,
                 QValue variance) {
    QValue inputs[] = { winner, loser };
    QValue outputs[] = { winner, variance };
    q_invoke(update_operation, inputs, outputs);
}

/* Build example 12, the quantised skill update, and write ex12_output.asm. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    c_prog_begin("ex12_output.asm");
    /*
     * The grid is off unless the application asks for it, and it has to
     * ask before q_operation_end. The axes are declared either way. On
     * the grid they size the table. Off, they are the two float arguments
     * the plain function takes.
     */
#ifdef EX_ENABLE
    q_use_grid();
#endif
    /*
     * Two axes, both declared before the operation is built. Axis 0 is
     * the winner's prior mean. Axis 1 is the opponent's mean. q_dimension
     * takes the input index, the bucket width, and how many buckets.
     * Width 8 and 4 buckets cover [0, 32) on each axis. Bucket i is the
     * half-open interval [8i, 8i + 8). The table is filled by running
     * update_skills at the midpoint of every bucket: 4, 12, 20, and 28.
     *
     * mu_a starts at 12 and mu_b at 20, which are both midpoints, so the
     * first game agrees with the plain function: gap 8, mean 15, variance
     * 0.8. A later mean that falls inside a bucket but is not its midpoint
     * is snapped to that bucket before the lookup. The second win for A
     * is the one that shows the snap. On the grid, A's mean stays the
     * tuple for (12, 20), which prints as 1500 and 80. The plain function
     * keeps the exact 17.25 and about 1.561, which print as 1725 and 156.
     * B then beats A. On the grid B's prior is still the midpoint 20 and
     * A's mean snaps back to 12, so B prints as 1900 and 80. The plain
     * function keeps B's exact mean, about 20.3125, and variance, about
     * 2.716, which print as 2031 and 272.
     *
     * Random samples of the domain could choose a non-uniform set of
     * buckets whose stored outputs stay closer to the real function. The
     * lookup is the table that choice would rewrite. Midpoints are enough
     * to show the idea.
     */
    q_dimension(0, 8.0, 4);
    q_dimension(1, 8.0, 4);
    QOperation update_operation = update_skills();
    ExConsole con = ex_console_declare();

    /*
     * mu_a and mu_b are the two means. var_a and var_b are the variances
     * the operation writes. q_emit appends scale100 and half, or the
     * output array and the lookup, and the plain procedure.
     */
    QValue mu_a = q_value(12.0);
    QValue mu_b = q_value(20.0);
    QValue var_a = q_value(0.0);
    QValue var_b = q_value(0.0);
    ExStr sA  = ex_line("s_a", "mean A x100 = ", 0, 0);
    ExStr sVa = ex_line("s_va", "var  A x100 = ", 0, 0);
    ExStr sB  = ex_line("s_b", "mean B x100 = ", 0, 0);
    ExStr sVb = ex_line("s_vb", "var  B x100 = ", 0, 0);
    ExStr sNl = ex_line("s_nl", "", 2, 0);
    q_emit(&update_operation);
    ex_console_globals();
    AsmExtern print = ex_console_print(&con);
    AsmExtern pu64  = ex_print_u64_def(&print);

        CFunc mainf = c_begin_function("main", NULL, 0, C_INT, "exit_code");
            /*
             * Three games. A beats B, A beats B again, then B beats A.
             * Each play rewrites only the winner. The four lines that
             * follow are A's mean, A's variance, B's mean, and B's
             * variance, each times 100.
             */
            play(&update_operation, mu_a, mu_b, var_a);
            play(&update_operation, mu_a, mu_b, var_a);
            play(&update_operation, mu_b, mu_a, var_b);
            ex_puts(&print, sA);
            q_print_x100(mu_a, &pu64);
            ex_puts(&print, sNl);
            ex_puts(&print, sVa);
            q_print_x100(var_a, &pu64);
            ex_puts(&print, sNl);
            ex_puts(&print, sB);
            q_print_x100(mu_b, &pu64);
            ex_puts(&print, sNl);
            ex_puts(&print, sVb);
            q_print_x100(var_b, &pu64);
            ex_puts(&print, sNl);
            ex_exit(&con, 0);
        c_end_function(&mainf);

    return c_prog_end();
}
