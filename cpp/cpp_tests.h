/*
 * cpp_tests.h - generation-time checks for the C++ layer.
 * Included from test/test.c after cpp_funcs.h. The class caps are fixed tables; they can grow.
 */

// TODO: it really doesn't seem like this checks much of cpp_funcs.h. Ensure the tests here cover cpp_funcs.h.

/* Class-table caps and operand text. Host checks of c_funcs.h that do not
 * draw random numbers live in c_run_unit_tests (c/c_tests.h). */
static void cpp_run_fuzz(void) {
    expect_true(CPP_MAX_CLASSES >= 8);
    expect_true(CPP_MAX_MEMBERS >= 8);
    const char* m = cpp_mem("rcx", 0);
    expect_true(m && strcmp(m, "qword ptr [rcx]") == 0);
    m = cpp_mem("rcx", 16);
    expect_true(m && strcmp(m, "qword ptr [rcx+16]") == 0);
}
