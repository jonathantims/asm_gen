/*
 * ex8_fuzzmock.h - the generic half of fuzzing under a mock.
 *
 * The function under test is one emitter parameterised by an InputSource.
 * That emitter lives in the example .c, next to the production and mock
 * call sites. The source includes ex7_fuzz.h before this header. This
 * header names the two sources and emits the global the mock source reads.
 */


/* How the adaptable function acquires its input. */
typedef enum { SRC_REAL, SRC_MOCK } InputSource;

/* Emit the mock-input global the SRC_MOCK path reads. dataq; only when testing. */
static inline void mock_emit_globals(void) {
    if (!testing_enabled) return;
    c_ensure_bss();
    write_unitialised_data("mock_input", "in");
}

/* Copy input in into the mock global. in is the value fuzz_next_input
 * returned. A move from one global to another is not a legal encoding,
 * so in is copied through a temporary variable first. */
static inline void mock_set(CValue in) {
    CValue tmp = c_dup("in", C_PTR, in);
    write_set_mov((AsmDataField){ "mock_input", "in", NULL }, c_cell(tmp));
    c_free(&tmp);
}

/* The mock global as a value. The header owns the cell. */
static inline CValue mock_get(void) {
    CValue slot = c_wrap((AsmDataField){ "mock_input", "in", NULL }, C_PTR);
    return c_dup("in", C_PTR, slot);
}
