/*
 * host.h - declarations and helpers this repo implements itself.
 *
 * Source files include this header once, before any header that uses it.
 *
 * The generators are short-lived, single-threaded processes. Splitting a
 * generation across processes is how concurrent runs are done today. The
 * static buffers below are therefore process-global on purpose.
 *
 * String formatting, file output, and process exit are implemented here so
 * the repo does not include stdio.h, stdlib.h, string.h, stdarg.h, stdbool.h,
 * or windows.h. The generators and the test harness link /NODEFAULTLIB.
 * mainCRTStartup commits the stack, then calls main. There is no C runtime
 * and no __chkstk.
 */

typedef unsigned long long size_t;
typedef long long          ptrdiff_t;
typedef int                bool;
#define true  1
#define false 0
#define NULL  ((void*)0)

typedef char* va_list;

/* MSVC x64 puts every variadic argument in an 8-byte slot. A value that fits
 * in that slot (size 1, 2, 4, or 8) is the slot. A larger value, including
 * AsmDataField, is passed as a pointer to the real object. __va_start is a
 * compiler intrinsic: it lowers to the address of the next stack slot and
 * does not come from the C runtime. */
void __cdecl __va_start(va_list*, ...);
#define va_start(ap, x) ((void)(__va_start(&(ap), (x))))
#define va_arg(ap, t)                                                          \
    ((sizeof(t) > 8 || (sizeof(t) & (sizeof(t) - 1)) != 0)                     \
        ? **(t**)((ap += 8) - 8)                                               \
        : *(t*)((ap += 8) - 8))
#define va_end(ap)      ((void)((ap) = (va_list)0))
#define va_copy(d, s)   ((void)((d) = (s)))

/* A FILE* is a Win32 HANDLE from fopen, or one of the stdout/stderr
 * sentinels below. The heap and the file calls are implemented further
 * down with HeapAlloc and CreateFileA. */
typedef struct HostFile FILE;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* The compiler emits a reference to this when a translation unit uses
 * floating point. Defining it here is the whole of that contract. */
int _fltused = 0;

/* ---- Win32, the small surface the generators and the test harness use -- */

typedef void*            HANDLE;
typedef void*            HWND;
typedef unsigned long    DWORD;
typedef unsigned short   WORD;
typedef unsigned char    BYTE;
typedef int              BOOL;
typedef unsigned int     UINT;
typedef unsigned long long ULONG_PTR;
typedef unsigned long long UINT_PTR;
typedef long long        LONG_PTR;
typedef UINT_PTR         WPARAM;
typedef LONG_PTR         LPARAM;

#define FALSE 0
#define TRUE  1
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
#define STD_INPUT_HANDLE  ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE  ((DWORD)-12)
#define GENERIC_READ      0x80000000u
#define GENERIC_WRITE     0x40000000u
#define FILE_SHARE_READ   0x00000001u
#define FILE_SHARE_WRITE  0x00000002u
#define CREATE_ALWAYS     2u
#define OPEN_EXISTING     3u
#define OPEN_ALWAYS       4u
#define FILE_ATTRIBUTE_NORMAL 0x00000080u
#define STARTF_USESTDHANDLES 0x00000100u
#define INFINITE          0xFFFFFFFFu
#define WAIT_OBJECT_0     0u
#define WM_CLOSE          0x0010u
#define MAX_PATH          260

typedef struct {
    DWORD  nLength;
    void*  lpSecurityDescriptor;
    BOOL   bInheritHandle;
} SECURITY_ATTRIBUTES;

typedef struct {
    DWORD  cb;
    char*  lpReserved;
    char*  lpDesktop;
    char*  lpTitle;
    DWORD  dwX;
    DWORD  dwY;
    DWORD  dwXSize;
    DWORD  dwYSize;
    DWORD  dwXCountChars;
    DWORD  dwYCountChars;
    DWORD  dwFillAttribute;
    DWORD  dwFlags;
    WORD   wShowWindow;
    WORD   cbReserved2;
    BYTE*  lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA;

typedef struct {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
} PROCESS_INFORMATION;

typedef struct { long long QuadPart; } HostLargeInt;

__declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD nStdHandle);
__declspec(dllimport) BOOL   __stdcall WriteFile(HANDLE, const void*, DWORD, DWORD*, void*);
__declspec(dllimport) BOOL   __stdcall ReadFile(HANDLE, void*, DWORD, DWORD*, void*);
__declspec(dllimport) void   __stdcall ExitProcess(UINT code);
__declspec(dllimport) char*  __stdcall GetCommandLineA(void);
__declspec(dllimport) BOOL   __stdcall DeleteFileA(const char*);
__declspec(dllimport) BOOL   __stdcall SetFilePointerEx(HANDLE, HostLargeInt, HostLargeInt*, DWORD);
__declspec(dllimport) HANDLE __stdcall GetProcessHeap(void);
__declspec(dllimport) void*  __stdcall HeapAlloc(HANDLE, DWORD, size_t);
__declspec(dllimport) BOOL   __stdcall HeapFree(HANDLE, DWORD, void*);
__declspec(dllimport) DWORD  __stdcall GetLastError(void);
__declspec(dllimport) DWORD  __stdcall GetTickCount(void);
__declspec(dllimport) void   __stdcall Sleep(DWORD ms);
__declspec(dllimport) BOOL   __stdcall CloseHandle(HANDLE);
__declspec(dllimport) DWORD  __stdcall WaitForSingleObject(HANDLE, DWORD);
__declspec(dllimport) BOOL   __stdcall GetExitCodeProcess(HANDLE, DWORD*);
__declspec(dllimport) BOOL   __stdcall TerminateProcess(HANDLE, UINT);
__declspec(dllimport) HANDLE __stdcall CreateFileA(const char*, DWORD, DWORD, SECURITY_ATTRIBUTES*, DWORD, DWORD, HANDLE);
__declspec(dllimport) BOOL   __stdcall CreateProcessA(const char*, char*, SECURITY_ATTRIBUTES*, SECURITY_ATTRIBUTES*, BOOL, DWORD, void*, const char*, STARTUPINFOA*, PROCESS_INFORMATION*);
__declspec(dllimport) HWND   __stdcall FindWindowA(const char* cls, const char* title);
__declspec(dllimport) BOOL   __stdcall PostMessageA(HWND, UINT, WPARAM, LPARAM);

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

/* Sentinel streams. fprintf routes these through WriteFile; any other FILE*
 * is a real fopen handle. */
#define stdout ((FILE*)(long long)1)
#define stderr ((FILE*)(long long)2)

#define ZeroMemory(p, n) host_memset((p), 0, (n))

/* ---- memory and strings ------------------------------------------------ */

/* Fill `n` bytes at `dst` with the low byte of `val`, then return `dst`. */
static inline void* host_memset(void* dst, int val, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    while (n--) *d++ = (unsigned char)val;
    return dst;
}

/* Copy `n` bytes from `src` onto `dst` and return `dst`. */
static inline void* host_memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

/* Compare `n` bytes of `a` with `b`. Returns 0 when they match, or the first
 * differing byte of `a` minus the byte of `b`. */
static inline int host_memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* x = (const unsigned char*)a;
    const unsigned char* y = (const unsigned char*)b;
    while (n--) {
        if (*x != *y) return (int)*x - (int)*y;
        x++; y++;
    }
    return 0;
}

/* Count bytes in `s` before the terminating NUL. A null `s` returns 0. */
static inline size_t host_strlen(const char* s) {
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

/* Compare C strings `a` and `b` as unsigned bytes. A null argument compares
 * as empty. Returns 0 when the strings are equal, else the byte difference. */
static inline int host_strcmp(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    while (*a && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

/* Find byte `c` in `s` and return that address. `c` of 0 returns the
 * terminator. A null `s`, or a missing `c`, returns NULL. */
static inline char* host_strchr(const char* s, int c) {
    if (!s) return NULL;
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return c == 0 ? (char*)s : NULL;
}

/* Find `needle` in `hay` and return that address. An empty `needle` returns
 * `hay`. A null `hay` or `needle` returns NULL. */
static inline char* host_strstr(const char* hay, const char* needle) {
    if (!hay || !needle) return NULL;
    if (!needle[0]) return (char*)hay;
    for (; *hay; hay++) {
        const char* h = hay;
        const char* n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)hay;
    }
    return NULL;
}

/* Copy `src` into `dst` and place a NUL inside `n` when `n` is positive.
 * A null `src` copies as empty. Returns `dst`. */
static inline char* host_strncpy(char* dst, const char* src, size_t n) {
    size_t i = 0;
    if (!src) src = "";
    for (; i + 1 < n && src[i]; i++) dst[i] = src[i];
    if (n) dst[i < n ? i : n - 1] = 0;
    return dst;
}

/* The compiler calls these by name when /Oi- turns an intrinsic into a
 * real call. They are the same bytes as the host_* helpers above. */

/* Fill `n` bytes at `dst` with the low byte of `val` and return `dst`. */
void* memset(void* dst, int val, size_t n) { return host_memset(dst, val, n); }
/* Copy `n` bytes from `src` to `dst` and return `dst`. The ranges must not overlap. */
void* memcpy(void* dst, const void* src, size_t n) { return host_memcpy(dst, src, n); }
/* Copy `n` bytes from `src` to `dst`, allowing overlap, and return `dst`. */
void* memmove(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    if (d == s || n == 0) return dst;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}
/* Compare `n` bytes. Returns 0 when they match, else the first byte difference. */
int memcmp(const void* a, const void* b, size_t n) { return host_memcmp(a, b, n); }
/* Count bytes in `s` before the NUL. A null `s` returns 0. */
size_t strlen(const char* s) { return host_strlen(s); }
/* Compare `a` and `b` as unsigned bytes. A null compares as empty. */
int strcmp(const char* a, const char* b) { return host_strcmp(a, b); }
/* Find byte `c` in `s`. `c` of 0 returns the terminator. Missing returns NULL. */
char* strchr(const char* s, int c) { return host_strchr(s, c); }
/* Find `needle` in `hay`. An empty needle returns `hay`. */
char* strstr(const char* hay, const char* needle) { return host_strstr(hay, needle); }
/* Copy `src` into `dst`, placing a NUL inside `n` when `n` is positive. */
char* strncpy(char* dst, const char* src, size_t n) { return host_strncpy(dst, src, n); }

/* Parse a leading decimal integer from `s`. Leading spaces are skipped.
 * A null `s`, or no digits, returns 0. */
static inline int atoi(const char* s) {
    int sign = 1;
    int v = 0;
    if (!s) return 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '+') s++;
    else if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    return sign < 0 ? -v : v;
}

/* Write `n` bytes of `s` to handle `h` with one WriteFile. A negative `n`
 * returns before any write. */
static inline void host_write_all(HANDLE h, const char* s, int n) {
    DWORD wrote = 0;
    if (n < 0) return;
    WriteFile(h, s, (DWORD)n, &wrote, NULL);
}

/* Write `msg` and a newline to stderr, then stop with status 1. A null
 * `msg` is written as "error". */
__declspec(noreturn) static inline void host_die(const char* msg) {
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    host_write_all(h, msg ? msg : "error", (int)host_strlen(msg ? msg : "error"));
    host_write_all(h, "\n", 1);
    ExitProcess(1);
}

/* setjmp for a /NODEFAULTLIB build. host_setjmp.h generates the image
 * with the assembler helpers: host_setjmp saves the nonvolatile registers
 * in `buf` and returns 0, and host_longjmp restores them. A test arms
 * host_fail_env so a generation failure jumps back to the test instead of
 * ending the process. With the arm clear, host_fail still stops the process. */
typedef __declspec(align(16)) struct HostJmpBuf {
    unsigned char raw[256];
} HostJmpBuf;
/* Save the caller's registers in `buf` and return 0. */
int host_setjmp(void* buf);
/* Resume at the matching host_setjmp, returning `code` there. `code` of 0
 * returns 1, so the caller can tell the jump from the first return. */
__declspec(noreturn) void host_longjmp(void* buf, int code);
#define _setjmp host_setjmp
#define longjmp host_longjmp

static HostJmpBuf* volatile host_fail_env = 0;
static volatile int host_fail_hit = 0;
static char host_fail_msg[512];

/* host_fail: print msg and stop, unless a test has armed host_fail_env.
 * The message is copied first: the caller's buffer dies at the longjmp. */
__declspec(noreturn) static inline void host_fail(const char* msg) {
    int i = 0;
    if (!msg) msg = "error";
    for (; msg[i] && i + 1 < (int)sizeof host_fail_msg; i++)
        host_fail_msg[i] = msg[i];
    host_fail_msg[i] = 0;
    if (host_fail_env) {
        void* env = host_fail_env;
        host_fail_env = 0;
        host_fail_hit = 1;
        longjmp(env, 1);
    }
    host_die(host_fail_msg);
}

/* The one way a generator reports a violated assumption.
 * When `cond` is true, stop with `msg`, unless a test armed host_fail_env
 * (see host_fail). */
static inline void crash_if(int cond, const char* msg) {
    if (cond) host_fail(msg);
}

/* Append a signed integer into `dst`. `v` is the value, `used` points at the
 * filled length, and `cap` is the limit. When `is_unsigned` is set, `v` is
 * rendered unsigned. Returns 1 when the digits fit, else 0. */
static inline int host_fmt_int(char* dst, int cap, int* used, long long v, int is_unsigned) {
    char tmp[32];
    int n = 0;
    unsigned long long u;
    if (is_unsigned) u = (unsigned long long)v;
    else if (v < 0) { u = (unsigned long long)(-v); }
    else u = (unsigned long long)v;
    if (u == 0) tmp[n++] = '0';
    while (u) { tmp[n++] = (char)('0' + (u % 10)); u /= 10; }
    if (!is_unsigned && v < 0) tmp[n++] = '-';
    for (int i = n - 1; i >= 0; i--) {
        if (*used + 1 >= cap) return 0;
        dst[(*used)++] = tmp[i];
    }
    return 1;
}

/* str_format / snprintf: %s %d %i %u %ld %lu %lld %llu %zu %% only.
 * Enough for every format string in this repo. Returns characters that
 * would have been written, excluding the terminating NUL. */
static inline int host_vformat(char* dst, size_t cap, const char* fmt, va_list ap) {
    int used = 0;
    int total = 0;
    if (!fmt) fmt = "";
    for (const char* p = fmt; *p; p++) {
        if (*p != '%') {
            total++;
            if (dst && used + 1 < (int)cap) dst[used++] = *p;
            continue;
        }
        p++;
        if (*p == '%') {
            total++;
            if (dst && used + 1 < (int)cap) dst[used++] = '%';
            continue;
        }
        int is_long = 0, is_ll = 0, is_size = 0, is_unsigned = 0;
        if (*p == 'z') { is_size = 1; p++; }
        else if (*p == 'l') {
            p++;
            if (*p == 'l') { is_ll = 1; p++; }
            else is_long = 1;
        }
        char kind = *p;
        if (kind == 'u') is_unsigned = 1;
        if (kind == 's') {
            const char* s = va_arg(ap, const char*);
            if (!s) s = "(null)";
            while (*s) {
                total++;
                if (dst && used + 1 < (int)cap) dst[used++] = *s;
                s++;
            }
        } else if (kind == 'd' || kind == 'i' || kind == 'u') {
            long long v;
            if (is_size) v = (long long)va_arg(ap, size_t);
            else if (is_ll) v = is_unsigned ? (long long)va_arg(ap, unsigned long long)
                                            : va_arg(ap, long long);
            else if (is_long) v = is_unsigned ? (long long)va_arg(ap, unsigned long)
                                              : (long long)va_arg(ap, long);
            else v = is_unsigned ? (long long)va_arg(ap, unsigned int)
                                 : (long long)va_arg(ap, int);
            if (is_size || kind == 'u') is_unsigned = 1;
            char tmp[32];
            int n = 0;
            unsigned long long u;
            if (is_unsigned) u = (unsigned long long)v;
            else if (v < 0) u = (unsigned long long)(-v);
            else u = (unsigned long long)v;
            if (u == 0) tmp[n++] = '0';
            while (u) { tmp[n++] = (char)('0' + (u % 10)); u /= 10; }
            if (!is_unsigned && v < 0) { total++; if (dst && used + 1 < (int)cap) dst[used++] = '-'; }
            for (int i = n - 1; i >= 0; i--) {
                total++;
                if (dst && used + 1 < (int)cap) dst[used++] = tmp[i];
            }
            (void)host_fmt_int;
        } else {
            total++;
            if (dst && used + 1 < (int)cap) dst[used++] = '?';
        }
    }
    if (dst && cap) dst[used < (int)cap ? used : (int)cap - 1] = 0;
    return total;
}

/* Format `fmt` and the `...` conversion values into `dst`, keeping a NUL
 * inside `cap`. Returns the length excluding that NUL, even if `cap` was short. */
static inline int snprintf(char* dst, size_t cap, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = host_vformat(dst, cap, fmt, ap);
    va_end(ap);
    return n;
}

/* Format `fmt` with va_list `ap` into `dst`, keeping a NUL inside `cap`.
 * Returns the length excluding that NUL, even if `cap` was short. */
static inline int vsnprintf(char* dst, size_t cap, const char* fmt, va_list ap) {
    return host_vformat(dst, cap, fmt, ap);
}

/* Format `fmt` and the `...` conversion values, then write them to `stream`.
 * stdout, stderr, and a null `stream` go through the console handles. */
static inline int fprintf(FILE* stream, const char* fmt, ...) {
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    int n = host_vformat(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n >= (int)sizeof buf) n = (int)sizeof buf - 1;
    if (stream == stderr) {
        host_write_all(GetStdHandle(STD_ERROR_HANDLE), buf, n);
    } else if (stream == stdout || stream == NULL) {
        host_write_all(GetStdHandle(STD_OUTPUT_HANDLE), buf, n);
    } else {
        fputs(buf, stream);
    }
    return n;
}

/* Format `fmt` and the `...` conversion values and write the text to stdout.
 * Returns the length actually written. */
static inline int printf(const char* fmt, ...) {
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    int n = host_vformat(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n >= (int)sizeof buf) n = (int)sizeof buf - 1;
    host_write_all(GetStdHandle(STD_OUTPUT_HANDLE), buf, n);
    return n;
}

/* Stop the process with status `code`. */
__declspec(noreturn) static inline void exit(int code) {
    ExitProcess((UINT)code);
}

/* ---- heap and files, in place of the C runtime ------------------------ */

/* Allocate `n` bytes from the process heap. A zero `n` still allocates one
 * byte so the result is a unique pointer. */
static inline void* malloc(size_t n) {
    if (n == 0) n = 1;
    return HeapAlloc(GetProcessHeap(), 0, n);
}

/* Release a block from malloc. A null `p` does nothing. */
static inline void free(void* p) {
    if (p) HeapFree(GetProcessHeap(), 0, p);
}

/* Open `filename`. `mode` is a C mode string: 'w' or 'a' creates the file,
 * anything else opens it for reading. Returns NULL when the open fails. */
static inline FILE* fopen(const char* filename, const char* mode) {
    DWORD access = GENERIC_READ;
    DWORD disp = OPEN_EXISTING;
    int writing = 0;
    if (mode) {
        for (const char* m = mode; *m; m++) {
            if (*m == 'w' || *m == 'a' || *m == '+') writing = 1;
        }
    }
    if (writing) {
        access = GENERIC_WRITE;
        disp = (mode && mode[0] == 'a') ? OPEN_ALWAYS : CREATE_ALWAYS;
    }
    HANDLE h = CreateFileA(filename, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    return (FILE*)h;
}

/* Close the handle from fopen. A null `stream` fails. Returns 0 on success. */
static inline int fclose(FILE* stream) {
    if (!stream) return -1;
    return CloseHandle((HANDLE)stream) ? 0 : -1;
}

/* Write the bytes of `s` to `stream`. Returns 0 on success, -1 on failure. */
static inline int fputs(const char* s, FILE* stream) {
    DWORD wrote = 0;
    int n;
    if (!s) s = "";
    n = (int)host_strlen(s);
    if (!WriteFile((HANDLE)stream, s, (DWORD)n, &wrote, NULL)) return -1;
    return 0;
}

/* Move the file pointer of `stream` by `offset` from `origin` (SEEK_SET,
 * SEEK_CUR, or SEEK_END). Returns 0 on success, -1 on failure. */
static inline int fseek(FILE* stream, long offset, int origin) {
    HostLargeInt dist;
    HostLargeInt neu;
    dist.QuadPart = offset;
    neu.QuadPart = 0;
    if (!SetFilePointerEx((HANDLE)stream, dist, &neu, (DWORD)origin)) return -1;
    return 0;
}

/* The current file position of `stream`, or -1 on failure. */
static inline long ftell(FILE* stream) {
    HostLargeInt zero;
    HostLargeInt pos;
    zero.QuadPart = 0;
    pos.QuadPart = 0;
    if (!SetFilePointerEx((HANDLE)stream, zero, &pos, SEEK_CUR)) return -1;
    return (long)pos.QuadPart;
}

/* Read `count` items of `size` bytes from `stream` into `ptr`. Returns how
 * many full items were read. */
static inline size_t fread(void* ptr, size_t size, size_t count, FILE* stream) {
    DWORD got = 0;
    size_t want;
    if (size == 0 || count == 0) return 0;
    want = size * count;
    if (!ReadFile((HANDLE)stream, ptr, (DWORD)want, &got, NULL)) return 0;
    return (size_t)got / size;
}

/* Delete the file at `path`. Returns 0 on success, -1 on failure. */
static inline int remove(const char* path) {
    return DeleteFileA(path) ? 0 : -1;
}

/* ---- stack commit and process entry ----------------------------------- */

/* How far commit_stack_pages walks. 180 chunks of 2500 bytes is about
 * 450 KiB, inside the default 1 MiB reserve, so the probe never steps off
 * the stack. The pages stay committed after the walk returns. */
#define STACK_PROBE_CHUNK 2500
#define STACK_PROBE_STEPS 180

/* Touch the stack one chunk at a time so each touch lands on the guard
 * page instead of jumping over it. This build links /NODEFAULTLIB, so
 * there is no __chkstk. `stepsLeft` is how many chunks are still to walk,
 * including this one. */
__declspec(noinline) void commit_stack_pages(int stepsLeft) {
    volatile unsigned char page[STACK_PROBE_CHUNK];
    page[0] = (unsigned char)stepsLeft;
    if (stepsLeft > 1) commit_stack_pages(stepsLeft - 1);
    /* Touched again after the call so this frame stays live and the
     * compiler cannot turn the recursion into a loop that never walks
     * downward. */
    page[STACK_PROBE_CHUNK - 1] = page[0];
}

/* Split the process command line into argc/argv. Quoted runs stay one
 * argument. The pointers alias a static copy and remain valid for main. */
static int host_argc = 0;
static char* host_argv[64];
static char host_cmdline[8192];

static void host_make_argv(void) {
    const char* src = GetCommandLineA();
    int n = 0;
    int i = 0;
    if (!src) src = "";
    while (src[n] && n + 1 < (int)sizeof host_cmdline) {
        host_cmdline[n] = src[n];
        n++;
    }
    host_cmdline[n] = 0;
    host_argc = 0;
    while (host_cmdline[i] && host_argc < 63) {
        char quote;
        while (host_cmdline[i] == ' ' || host_cmdline[i] == '\t') i++;
        if (!host_cmdline[i]) break;
        quote = 0;
        if (host_cmdline[i] == '"') {
            quote = '"';
            i++;
        }
        host_argv[host_argc++] = &host_cmdline[i];
        while (host_cmdline[i] && host_cmdline[i] != quote &&
               (quote || (host_cmdline[i] != ' ' && host_cmdline[i] != '\t'))) {
            i++;
        }
        if (host_cmdline[i]) host_cmdline[i++] = 0;
    }
    host_argv[host_argc] = NULL;
}

/* The program under test supplies main. Generators ignore the arguments.
 * test.exe reads them. */
int main(int argc, char** argv);

/* Process entry. Commits the stack, then runs main. The link line names
 * this symbol with /ENTRY:mainCRTStartup. */
void mainCRTStartup(void) {
    int code;
    commit_stack_pages(STACK_PROBE_STEPS);
    host_make_argv();
    code = main(host_argc, host_argv);
    ExitProcess((UINT)code);
}

/* ---- alignment ---------------------------------------------------------- *
 *
 * Default is ALIGN_NONE: fields are packed in declaration order and any
 * padding is written explicitly by the caller. Turn a mode on for a scope,
 * or pass it for one definition, when the bytes must match an external
 * layout or must not tear.
 *
 *   ALIGN_NONE   pack tight. A field may start at any byte. A load or store
 *                of that field can tear if another thread writes it, and it
 *                can split a cache line.
 *   ALIGN_C      each field is aligned to its size (capped at 8), which is
 *                the usual C ABI. Naturally aligned 1/2/4/8-byte accesses are
 *                atomic on x64. This is the mode to use for Win32 structs and
 *                any other interop record.
 *   ALIGN_CACHE  like ALIGN_C, and the field is also slid forward so it does
 *                not straddle a 64-byte cache line. Split-line accesses are
 *                not atomic and are much slower. A field larger than a line
 *                is aligned to the line; it still occupies more than one.
 *   ALIGN_PAGE   like ALIGN_C, and the field does not straddle a 4096-byte
 *                page. A split page can fault halfway through an access.
 *
 * The choice is a generation-time global with a per-definition override.
 * Generation itself is single-threaded; the modes describe the concurrency
 * of the program being generated.
 */

typedef enum {
    ALIGN_NONE  = 0,
    ALIGN_C     = 1,
    ALIGN_CACHE = 2,
    ALIGN_PAGE  = 3
} AlignMode;

static AlignMode align_default_mode = ALIGN_NONE;

/* Make `mode` the default field alignment for later definitions. */
static inline void align_set_default(AlignMode mode) { align_default_mode = mode; }
/* Return the current default field-alignment mode. */
static inline AlignMode align_get_default(void) { return align_default_mode; }

/* Bytes to insert before a field of `size` placed at `offset` under `mode`. */
static inline int align_pad(int offset, int size, AlignMode mode) {
    if (mode == ALIGN_NONE || size <= 0 || offset < 0) return 0;
    int natural = size > 8 ? 8 : size;
    if (natural < 1) natural = 1;
    int pad = 0;
    int rem = natural > 1 ? (offset % natural) : 0;
    if (rem) pad = natural - rem;
    int at = offset + pad;
    if (mode == ALIGN_CACHE) {
        int line = 64;
        if (size >= line) {
            int r = at % line;
            if (r) pad += line - r;
        } else if ((at % line) + size > line) {
            pad += line - (at % line);
        }
    } else if (mode == ALIGN_PAGE) {
        int page = 4096;
        if (size >= page) {
            int r = at % page;
            if (r) pad += page - r;
        } else if ((at % page) + size > page) {
            pad += page - (at % page);
        }
    }
    return pad;
}
