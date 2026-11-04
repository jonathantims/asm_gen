/*
 * test.c - Unit test runner for the generated exe(s).
 *
 * Each command-line argument names an exe to test (default "output.exe"), so
 * build.bat can run the identical test suite against both output.exe (from
 * asm_main.c / asm_funcs.h) and c_output.exe (from c_main.c / c_funcs.h).
 *
 * Built and executed by build.bat after the exe is linked. The exe maps a
 * 1 MB log file, logs "started\n", then registers a window class, creates a
 * window, and runs a message loop until the window is closed. This runner:
 *   - launches output.exe WITHOUT waiting (it blocks in its message loop),
 *   - waits for the window to appear (proving CreateWindowExA succeeded),
 *   - posts WM_CLOSE to the window,
 *   - waits for the process to exit and checks the exit code is 123,
 *   - checks the window no longer exists,
 * and then re-checks the log file that was written before the window came up:
 *   - the log file was created,
 *   - it has the expected size (1 MB),
 *   - it begins with the expected record ("started\n"),
 *   - the remainder is zero-filled (a fresh circular buffer, no error records).
 *
 * Exits 0 if all checks pass, 1 otherwise, so build.bat fails on a regression.
 */

#include "host.h"

/* expect_true ends the process at this source line. A fuzz run that never
 * reaches a case leaves its coverage flag clear and fails the same way.
 * Named expect_true so it does not collide with crash_if(cond, msg). */
#define expect_true(cond) do { if (!(cond)) ExitProcess((UINT)__LINE__); } while (0)

#include "asm.h"
#include "asm_funcs.h"
#include "asm_tests.h"
#include "c_funcs.h"
#include "c_tests.h"
#include "cpp_funcs.h"
#include "cpp_tests.h"
#include "csharp_funcs.h"
#include "csharp_tests.h"
#include "gc_common.h"
#include "rust_funcs.h"
#include "py_funcs.h"
#include "js_funcs.h"
#include "fuzz_beh.h"
#include "gap_tests.h"
#include "beh_tests.h"

/* FindWindowA / PostMessageA live in user32, which cl does not link by default. */
#pragma comment(lib, "user32.lib")

#define LOG_PATH        "output.log"
#define LOG_SIZE        0x100000          /* 1 MB, must match LOG_SIZE in asm_main.c */
#define EXPECTED_EXIT   123
#define WINDOW_CLASS    "AsmGenWindow"    /* must match className in asm_main.c */
#define WINDOW_TIMEOUT  10000             /* ms to wait for the window to appear */
#define EXIT_TIMEOUT    10000             /* ms to wait for the process to exit */

/* The demo record written by output.exe: "started" followed by a newline. */
static const char EXPECTED_PREFIX[] = {'s', 't', 'a', 'r', 't', 'e', 'd', '\n'};
#define EXPECTED_PREFIX_LEN ((long)sizeof(EXPECTED_PREFIX))

/* Print PASS or FAIL for msg. Increment failures when cond is false. */
static void check(int* failures, int cond, const char* msg) {
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", msg);
    if (!cond) (*failures)++;
}

/*
 * Launch the given exe without waiting for it to exit (it blocks in a message
 * loop). The caller owns pi->hProcess / pi->hThread. Returns 1 on success.
 */
static int launch_output_exe(const char* exe, PROCESS_INFORMATION* pi) {
    STARTUPINFOA si;
    char cmd[MAX_PATH];

    strncpy(cmd, exe, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(pi, sizeof(*pi));

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, pi)) {
        printf("  [FAIL] CreateProcess(%s) failed: %lu\n",
               exe, (unsigned long)GetLastError());
        return 0;
    }
    return 1;
}

/* Poll FindWindowA for cls until the window appears or timeout_ms elapses. */
static HWND wait_for_window(const char* cls, DWORD timeout_ms) {
    DWORD start = GetTickCount();
    for (;;) {
        HWND h = FindWindowA(cls, NULL);
        if (h) return h;
        if (GetTickCount() - start > timeout_ms) return NULL;
        Sleep(20);
    }
}

/* Read an entire file at path into a freshly malloc'd buffer and set out_size.
 * Returns NULL on failure. */
static unsigned char* read_file(const char* path, long* out_size) {
    FILE* f = fopen(path, "rb");
    long size;
    unsigned char* buf;

    *out_size = 0;
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }

    buf = (unsigned char*)malloc((size_t)size);
    if (buf && fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        buf = NULL;
    }
    fclose(f);

    if (buf) *out_size = size;
    return buf;
}

/*
 * run_console_test - launch a console exe with stdout+stderr redirected to a
 * temp file, wait for it to exit, and verify (a) exit code 0 and (b) the marker
 * line appears in its output. Used for the language-wrapper demos (cpp/csharp/
 * rust/py/js), which print a feature tour and exit 0. Returns failure count.
 */
static int run_console_test(const char* exe, int nneedle, char** needles) {
    char outpath[MAX_PATH];
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    HANDLE hOut;
    DWORD exit_code = 1, wait_result;
    char cmd[MAX_PATH];
    unsigned char* buf;
    long size;
    int failures = 0;

    printf("Running %s console test...\n", exe);

    snprintf(outpath, sizeof(outpath), "%s.out.txt", exe);
    remove(outpath);

    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    hOut = CreateFileA(outpath, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("  [FAIL] could not create %s\n", outpath);
        return 1;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hOut;
    si.hStdError = hOut;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    ZeroMemory(&pi, sizeof(pi));

    strncpy(cmd, exe, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        printf("  [FAIL] CreateProcess(%s) failed: %lu\n",
               exe, (unsigned long)GetLastError());
        CloseHandle(hOut);
        return 1;
    }

    wait_result = WaitForSingleObject(pi.hProcess, EXIT_TIMEOUT);
    check(&failures, wait_result == WAIT_OBJECT_0, "process exited");
    if (wait_result == WAIT_OBJECT_0) {
        GetExitCodeProcess(pi.hProcess, &exit_code);
    } else {
        TerminateProcess(pi.hProcess, 1);
    }
    check(&failures, exit_code == 0, "exit code is 0");

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hOut);

    buf = read_file(outpath, &size);
    check(&failures, buf != NULL, "produced console output");
    if (buf) {
        /* NUL-terminate a copy so we can strstr for the marker line. */
        char* text = (char*)malloc((size_t)size + 1);
        if (text) {
            memcpy(text, buf, (size_t)size);
            text[size] = '\0';
            for (int i = 0; i < nneedle; i++) {
                char msg[160];
                snprintf(msg, sizeof msg, "output contains \"%s\"", needles[i]);
                check(&failures, strstr(text, needles[i]) != NULL, msg);
            }
            free(text);
        }
        free(buf);
    }

    if (failures == 0) printf("All %s console tests passed.\n", exe);
    else               printf("%d %s console test(s) failed.\n", failures, exe);
    return failures;
}

/* Run the window checks against exe. */
static int run_tests(const char* exe) {    PROCESS_INFORMATION pi;
    DWORD exit_code = 0;
    DWORD wait_result;
    HWND hwnd;
    unsigned char* buf;
    long size;
    int failures = 0;

    printf("Running %s test...\n", exe);

    /* Remove any stale log so we verify the exe creates it fresh. */
    remove(LOG_PATH);

    if (!launch_output_exe(exe, &pi)) {
        printf("1 test(s) failed.\n");
        return 1;
    }

    /* The window must appear, proving RegisterClassA + CreateWindowExA worked. */
    hwnd = wait_for_window(WINDOW_CLASS, WINDOW_TIMEOUT);
    check(&failures, hwnd != NULL, "window was created");

    if (hwnd) {
        /* Ask the window to close; DefWindowProc -> WM_DESTROY -> PostQuitMessage. */
        PostMessageA(hwnd, WM_CLOSE, 0, 0);
    } else {
        /* Never appeared: don't hang the build - force the process down. */
        TerminateProcess(pi.hProcess, 1);
    }

    wait_result = WaitForSingleObject(pi.hProcess, EXIT_TIMEOUT);
    check(&failures, wait_result == WAIT_OBJECT_0, "process exited after window closed");

    if (wait_result == WAIT_OBJECT_0) {
        GetExitCodeProcess(pi.hProcess, &exit_code);
    } else {
        TerminateProcess(pi.hProcess, 1);
    }
    check(&failures, exit_code == EXPECTED_EXIT, "exit code is 123");

    /* After the process exits its window is gone. */
    check(&failures, FindWindowA(WINDOW_CLASS, NULL) == NULL, "window destroyed after close");

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* The log was written before the window came up; re-check its contents. */
    buf = read_file(LOG_PATH, &size);
    check(&failures, buf != NULL, "log file was created");

    if (buf) {
        check(&failures, size == LOG_SIZE, "log file is 1 MB");

        check(&failures, size >= EXPECTED_PREFIX_LEN &&
                  memcmp(buf, EXPECTED_PREFIX, (size_t)EXPECTED_PREFIX_LEN) == 0,
              "log begins with \"started\\n\"");

        /* Everything after the demo record must be zero in a fresh ring
         * (no error records were logged, so the happy path stays clean). */
        int rest_zero = 1;
        for (long i = EXPECTED_PREFIX_LEN; i < size; i++) {
            if (buf[i] != 0) { rest_zero = 0; break; }
        }
        check(&failures, rest_zero, "remainder of log is zero-filled");

        free(buf);
    }

    if (failures == 0) {
        printf("All %s tests passed.\n", exe);
    } else {
        printf("%d %s test(s) failed.\n", failures, exe);
    }
    return failures;
}

/*
 * Each command-line argument is an exe path to test; with no arguments the
 * default "output.exe" is used. The tests are identical for every exe (both
 * output.exe and c_output.exe generate the same program), so build.bat runs
 * this once per exe. Returns non-zero if any exe fails a check.
 */
/*
 * Two modes:
 *   test.exe [exe ...]                 - window tests (default output.exe)
 *   test.exe --console <exe> <marker>  - console test: run <exe>, require exit 0
 *                                        and that <marker> appears in its output.
 * Returns non-zero if any check fails.
 */
int main(int argc, char** argv) {
    int total_failures = 0;
    asm_run_fuzz();
    c_run_fuzz();
    c_run_unit_tests();
    cpp_run_fuzz();
    csharp_run_fuzz();

    /* --console runs once per language demo. The behavior suite generates,
     * assembles, and executes a program; do that on the window run only. */
    if (!(argc >= 2 && strcmp(argv[1], "--console") == 0)) {
        beh_run_tests();
        if (argc >= 2 && strcmp(argv[1], "--behavior") == 0) return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "--console") == 0) {
        if (argc < 4) {
            printf("usage: test.exe --console <exe> <marker> [more markers]\n");
            return 2;
        }
        return run_console_test(argv[2], argc - 3, argv + 3) == 0 ? 0 : 1;
    }

    if (argc <= 1) {
        total_failures += run_tests("output.exe");
    } else {
        for (int i = 1; i < argc; i++) {
            total_failures += run_tests(argv[i]);
        }
    }

    return total_failures == 0 ? 0 : 1;
}
