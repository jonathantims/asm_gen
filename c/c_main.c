
/*
 * c_main.c - The same memory-mapped-log + Win32-window program that
 * asm_main.c generates, but written through the C-like layer in c_funcs.h
 * instead of by naming every register directly.
 *
 * Functions, parameters, and locals are declared without storage locations.
 * The two Win32 structures are laid out automatically from their field types.
 * The generated assembly is emitted to c_output.asm and is behaviourally
 * identical to output.asm, so the same test suite validates both.
 */

#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"

#define FILE_ATTRIBUTE_NORMAL 0x80u
#define FILE_MAP_ALL_ACCESS   0xF001Fu
#define PAGE_READWRITE        0x04u
#define WS_OVERLAPPEDWINDOW   0x00CF0000u
#define WS_VISIBLE            0x10000000u
#define CW_USEDEFAULT         0x80000000u

/* Generate the C-layer window demo. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    /* Win32 interop structs need natural C alignment. The global default is
     * ALIGN_NONE (pack tight, padding explicit). This program opts in. */
    align_set_default(ALIGN_C);
    c_prog_begin("c_output.asm");

    /* External Win32 functions. c_extern places the parameters. Only the
     * tags are named here. */
    const char* exit_params[] = {"exit_code"};
    AsmExtern exit_process = c_extern("ExitProcess", exit_params, 1, C_PTR, NULL);

    const char* create_file_params[] = {
        "lpFileName", "dwDesiredAccess", "dwShareMode", "lpSecurityAttributes",
        "dwCreationDisposition", "dwFlagsAndAttributes", "hTemplateFile"
    };
    AsmExtern create_file = c_extern("CreateFileA", create_file_params, 7, C_PTR, "hFile");

    const char* create_mapping_params[] = {
        "hFile", "lpAttributes", "flProtect", "dwMaxHigh", "size", "lpName"
    };
    AsmExtern create_mapping = c_extern("CreateFileMappingA", create_mapping_params, 6, C_PTR, "hMap");

    const char* map_view_params[] = {
        "hMap", "dwAccess", "offHigh", "offLow", "numBytes"
    };
    AsmExtern map_view = c_extern("MapViewOfFile", map_view_params, 5, C_PTR, "pView");

    const char* get_module_params[] = {"lpModuleName"};
    AsmExtern get_module = c_extern("GetModuleHandleA", get_module_params, 1, C_PTR, "hInstance");

    const char* register_class_params[] = {"lpWndClass"};
    AsmExtern register_class = c_extern("RegisterClassA", register_class_params, 1, C_SHORT, "atom");

    const char* create_window_params[] = {
        "dwExStyle", "lpClassName", "lpWindowName", "dwStyle",
        "x", "y", "nWidth", "nHeight", "hWndParent", "hMenu", "hInstance", "lpParam"
    };
    AsmExtern create_window = c_extern("CreateWindowExA", create_window_params, 12, C_PTR, "hWnd");

    const char* def_window_proc_params[] = {"hWnd", "Msg", "wParam", "lParam"};
    AsmExtern def_window_proc = c_extern("DefWindowProcA", def_window_proc_params, 4, C_PTR, "result");

    const char* post_quit_params[] = {"nExitCode"};
    AsmExtern post_quit = c_extern("PostQuitMessage", post_quit_params, 1, C_PTR, NULL);

    const char* get_message_params[] = {"lpMsg", "hWnd", "wMsgFilterMin", "wMsgFilterMax"};
    AsmExtern get_message = c_extern("GetMessageA", get_message_params, 4, C_INT, "result");

    const char* translate_message_params[] = {"lpMsg"};
    AsmExtern translate_message = c_extern("TranslateMessage", translate_message_params, 1, C_PTR, NULL);

    const char* dispatch_message_params[] = {"lpMsg"};
    AsmExtern dispatch_message = c_extern("DispatchMessageA", dispatch_message_params, 1, C_PTR, NULL);

    /* ---- Constants (CValues usable directly in comparisons/arithmetic). ---- */
    CValue LOG = c_constant("LOG_SIZE", c_dec(0x100000), C_PTR, "size");
    CValue WM  = c_constant("WM_DESTROY", c_dec(2), C_PTR, "Msg");

    /* ---- Initialised data. Each string is declared with c_string, which emits
     * the bytes, an address constant, and a byte-length constant in one call and
     * hands back {off, len} operands — so the generator never writes an offset
     * constant, an AsmDataField, or the assembler's `$ - <label>` length idiom.
     * The off/len tags match the parameters each string is later passed to. */
        CStr logName     = c_string("logName",     c_db_text("output.log", 0, 1), "lpFileName", "len");
        CStr msgStarted  = c_string("msgStarted",  c_db_text("started", 1, 0), "src", "len");
        CStr className   = c_string("className",   c_db_text("AsmGenWindow", 0, 1), "lpClassName", "len");
        CStr windowTitle = c_string("windowTitle", c_db_text("AsmGen Test Window", 0, 1), "lpWindowName", "len");
        CStr errRegister = c_string("errRegister", c_db_text("RegisterClassA failed", 2, 0), "src", "len");
        CStr errCreate   = c_string("errCreate",   c_db_text("CreateWindowExA failed", 2, 0), "src", "len");

        /* WNDCLASSA (72 bytes) laid out automatically from field types. The base
         * label "wndClass" is the first field; padding is inserted for the
         * 8-byte members. Address-valued fields use c_offset to store a symbol's
         * address without naming the assembler "offset" keyword. Only wcInstance
         * is patched at runtime. */
        CStruct wc = c_struct_begin("wndClass", "0");
            c_struct_field(&wc, "",              "0",                    "style",         C_INT);   /* @0  base */
            c_struct_field(&wc, "lpfnWndProc",   c_offset("WndProc"),    "lpfnWndProc",   C_PTR);   /* @8  */
            c_struct_field(&wc, "cbClsExtra",    "0",                    "cbClsExtra",    C_INT);   /* @16 */
            c_struct_field(&wc, "cbWndExtra",    "0",                    "cbWndExtra",    C_INT);   /* @20 */
            c_struct_field(&wc, "wcInstance",    "0",                    "hInstance",     C_PTR);   /* @24 */
            c_struct_field(&wc, "hIcon",         "0",                    "hIcon",         C_PTR);   /* @32 */
            c_struct_field(&wc, "hCursor",       "0",                    "hCursor",       C_PTR);   /* @40 */
            c_struct_field(&wc, "hbrBackground", "0",                    "hbrBackground", C_PTR);   /* @48 */
            c_struct_field(&wc, "lpszMenuName",  "0",                    "lpszMenuName",  C_PTR);   /* @56 */
            c_struct_field(&wc, "lpszClassName", c_offset("className"),  "lpszClassName", C_PTR);   /* @64 */
        CStructRef wndClass = c_struct_emit(&wc);

    /* ---- Uninitialised data. Program variables held as CValues so they can be
     * assigned/read through the c_* helpers (c_assign, c_dup, ...). ---- */
        CValue hFile  = c_global("hFile", C_PTR, "hFile");
        CValue hMap   = c_global("hMap", C_PTR, "hMap");
        CValue pView  = c_global("pView", C_PTR, "pView");
        CValue logPos = c_global("logPos", C_PTR, "head");

        /* MSG (48 bytes), filled by GetMessageA; base label "msgBuf". */
        CStruct msg = c_struct_begin("msgBuf", "?");
            c_struct_field(&msg, "",           "?", "lpMsg",   C_PTR);   /* @0  base / hwnd */
            c_struct_field(&msg, "msgMessage", "?", "message", C_INT);   /* @8  */
            c_struct_field(&msg, "msgWParam",  "?", "wParam",  C_PTR);   /* @16 */
            c_struct_field(&msg, "msgLParam",  "?", "lParam",  C_PTR);   /* @24 */
            c_struct_field(&msg, "msgTime",    "?", "time",    C_INT);   /* @32 */
            c_struct_field(&msg, "msgPt",      "?", "pt",      C_LONG);  /* @40 */
        CStructRef msgBuf = c_struct_emit(&msg);

    /* dispose_demo is a small use of c_mark_const and c_dispose.
     * limit is stored, then locked. scratch is added to and then disposed,
     * so the call to dispose_sink does not spill or restore scratch.
     * A later read of scratch would be a generation error. limit is only
     * read after it is marked const. */
    CFunc dispose_sink = c_begin_function("dispose_sink", NULL, 0, C_PTR, NULL);
        c_ret(&dispose_sink);
    c_end_function(&dispose_sink);

    CFunc dispose_demo = c_begin_function("dispose_demo", NULL, 0, C_INT, "exit_code");
        CValue limit = c_local_imm_n("limit", C_INT, 4);
        c_mark_const(&limit);
        CValue scratch = c_local_imm_n("scratch", C_INT, 1);
        c_add(&scratch, limit);
        c_dispose(&scratch);
        c_call(&dispose_sink);
        c_ret_value(&dispose_demo, limit);
    c_end_function(&dispose_demo);

    /* log_write(src, len): append a record into the 1 MB mapped ring, wrapping
     * at the end. c_copy_bytes keeps a source that the copy would otherwise
     * overwrite, so the wrap path can pass src twice. */
    const CParam lw_params[] = {{"src", C_PTR}, {"len", C_PTR}};
    CFunc log_write = c_begin_function("log_write", lw_params, 2, C_PTR, NULL);
        CValue src = c_param(&log_write, 0);
        CValue len = c_param(&log_write, 1);

        /* Clamp len to LOG_SIZE. */
        CIf tooBig = c_if_ugt(len, LOG);
            c_assign(&len, LOG);
        c_end_if(&tooBig);

        CValue head = c_dup("head", C_PTR, logPos);
        CValue dst  = c_dup("dst", C_PTR, pView);
        c_add(&dst, head);                 /* dst = pView + head */
        CValue space = c_dup("space", C_PTR, LOG);
        c_sub(&space, head);               /* space = LOG_SIZE - head */

        /* If the record does not fit before the end, wrap. */
        CIf wrap = c_if_ugt(len, space);
            src = c_copy_bytes(dst, src, space);
            c_assign(&dst, pView);
            CValue rem = c_dup("len", C_PTR, len);
            c_sub(&rem, space);
            src = c_copy_bytes(dst, src, rem);
            c_assign(&logPos, rem);
            c_ret(&log_write);
        c_end_if(&wrap);

        /* Single copy path. */
        c_copy_bytes(dst, c_param(&log_write, 0), len);
        c_add(&head, len);                 /* head += len */
        CIf noWrap = c_if_uge(head, LOG);
            c_sub(&head, LOG);             /* wrap (head == LOG_SIZE -> 0) */
        c_end_if(&noWrap);
        c_assign(&logPos, head);
        c_ret(&log_write);
    c_end_function(&log_write);

    /*
     * WndProc(hWnd, Msg, wParam, lParam): handle WM_DESTROY (post quit) and
     * forward everything else to DefWindowProcA.
     */
    const CParam wp_params[] = {
        {"hWnd", C_PTR}, {"Msg", C_PTR}, {"wParam", C_PTR}, {"lParam", C_PTR}
    };
    CFunc wnd_proc = c_begin_function("WndProc", wp_params, 4, C_PTR, "result");
        CValue msgv = c_param(&wnd_proc, 1);

        CIf isDestroy = c_if_eq(msgv, WM);
            c_call_ext_void(&post_quit, c_lit_n(0, "nExitCode"));
            c_ret_zero(&wnd_proc);
        c_end_if(&isDestroy);

        /* return DefWindowProcA(hWnd, Msg, wParam, lParam); args already in place */
        c_call_ext_void(&def_window_proc,
            c_cell(c_param(&wnd_proc, 0)), c_cell(c_param(&wnd_proc, 1)),
            c_cell(c_param(&wnd_proc, 2)), c_cell(c_param(&wnd_proc, 3)));
        c_ret(&wnd_proc);
    c_end_function(&wnd_proc);

    /* ---- main: map the log, log a record, create a window, then exit 123. ---- */
    CFunc main_proc = c_begin_function("main", NULL, 0, C_INT, "exit_code");

        c_call(&dispose_demo);

        CValue hf = c_call_ext(C_PTR, &create_file,
            logName.off,
            c_lit_u(GENERIC_READ | GENERIC_WRITE, "dwDesiredAccess"),
            c_lit_n(0, "dwShareMode"),
            c_lit_n(0, "lpSecurityAttributes"),
            c_lit_n(CREATE_ALWAYS, "dwCreationDisposition"),
            c_lit_u(FILE_ATTRIBUTE_NORMAL, "dwFlagsAndAttributes"),
            c_lit_n(0, "hTemplateFile"));
        c_assign(&hFile, hf);

        CValue hm = c_call_ext(C_PTR, &create_mapping,
            c_cell(hFile),
            c_lit_n(0, "lpAttributes"),
            c_lit_u(PAGE_READWRITE, "flProtect"),
            c_lit_n(0, "dwMaxHigh"),
            c_cell(LOG),
            c_lit_n(0, "lpName"));
        c_assign(&hMap, hm);

        CValue pv = c_call_ext(C_PTR, &map_view,
            c_cell(hMap),
            c_lit_u(FILE_MAP_ALL_ACCESS, "dwAccess"),
            c_lit_n(0, "offHigh"),
            c_lit_n(0, "offLow"),
            c_lit_n(0, "numBytes"));
        c_assign(&pView, pv);

        /* Ring write head starts at offset 0. */
        c_assign_imm(&logPos, c_dec(0));

        /* Log one record through the circular buffer. */
        c_call(&log_write, msgStarted.off, msgStarted.len);

        /* GetModuleHandleA(NULL) -> HINSTANCE stored into the window class. */
        CValue hi = c_call_ext(C_PTR, &get_module, c_lit_n(0, "lpModuleName"));
        CValue wcInstance = c_field(&wndClass, "wcInstance", C_PTR);
        c_assign(&wcInstance, hi);

        CValue atom = c_call_ext(C_SHORT, &register_class, c_struct_base(&wndClass, "lpWndClass"));
        CIf classFailed = c_if_eq(atom, c_imm_n(0, C_SHORT, "atom"));
            c_call(&log_write, errRegister.off, errRegister.len);
            c_call_ext_void(&exit_process, c_lit_n(1, "exit_code"));
        c_end_if(&classFailed);

        CValue hwnd = c_call_ext(C_PTR, &create_window,
            c_lit_n(0, "dwExStyle"),
            className.off,
            windowTitle.off,
            c_lit_u(WS_OVERLAPPEDWINDOW | WS_VISIBLE, "dwStyle"),
            c_lit_u(CW_USEDEFAULT, "x"),
            c_lit_u(CW_USEDEFAULT, "y"),
            c_lit_u(CW_USEDEFAULT, "nWidth"),
            c_lit_u(CW_USEDEFAULT, "nHeight"),
            c_lit_n(0, "hWndParent"),
            c_lit_n(0, "hMenu"),
            c_cell(wcInstance),
            c_lit_n(0, "lpParam"));
        CIf windowFailed = c_if_eq(hwnd, c_imm_n(0, C_PTR, "hWnd"));
            c_call(&log_write, errCreate.off, errCreate.len);
            c_call_ext_void(&exit_process, c_lit_n(1, "exit_code"));
        c_end_if(&windowFailed);

        /* Message loop. */
        CLoop loop = c_loop_begin();
            CValue res = c_call_ext(C_INT, &get_message,
                c_struct_base(&msgBuf, "lpMsg"),
                c_lit_n(0, "hWnd"),
                c_lit_n(0, "wMsgFilterMin"),
                c_lit_n(0, "wMsgFilterMax"));
            CIf running = c_if_sgt(res, c_imm_n(0, C_INT, "result"));
                c_call_ext_void(&translate_message, c_struct_base(&msgBuf, "lpMsg"));
                c_call_ext_void(&dispatch_message, c_struct_base(&msgBuf, "lpMsg"));
                c_loop_continue(&loop);
            c_end_if(&running);
            c_loop_break(&loop);
        c_loop_end(&loop);

        /* Exit with code 123 once the window has been closed. */
        c_call_ext_void(&exit_process, c_lit_n(123, "exit_code"));

    c_end_function(&main_proc);

    return c_prog_end();
}
