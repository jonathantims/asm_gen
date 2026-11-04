/*
 * csharp_tests.h - generation-time checks for the C# layer.
 * Included from test/test.c after csharp_funcs.h. The class caps are fixed tables; they can grow.
 */

/* Generation-time check: class and member table caps are at least 8. */
static void csharp_run_fuzz(void) {
    expect_true(CS_MAX_CLASSES >= 8);
    expect_true(CS_MAX_MEMBERS >= 8);
}
