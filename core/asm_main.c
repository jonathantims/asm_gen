#include "host.h"
#include "asm.h"
#include "asm_funcs.h"

/* Generate output.asm, whose program maps a 1 MB log, writes "started",
 * shows a window, and exits 123 when that window closes. */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    write_set_case_sensitive_identifiers();

    /* External Win32 functions. asm_win64_params assigns rcx, rdx, r8, r9
     * and then qword ptr [rsp+40] upward. log_write below keeps its own
     * slot list because that convention is private. */
    AsmExtern exit_process = write_common_extern("ExitProcess",
        asm_win64_params((const char*[]){ "exit_code" }, 1), 1, asm_void);
    AsmExtern create_file = write_common_extern("CreateFileA",
        asm_win64_params((const char*[]){
            "lpFileName", "dwDesiredAccess", "dwShareMode", "lpSecurityAttributes",
            "dwCreationDisposition", "dwFlagsAndAttributes", "hTemplateFile"
        }, 7), 7, (AsmDataField){"rax", "hFile"});
    AsmExtern create_mapping = write_common_extern("CreateFileMappingA",
        asm_win64_params((const char*[]){
            "hFile", "lpAttributes", "flProtect", "dwMaxHigh", "dwMaxLow", "lpName"
        }, 6), 6, (AsmDataField){"rax", "hMap"});
    AsmExtern map_view = write_common_extern("MapViewOfFile",
        asm_win64_params((const char*[]){
            "hMap", "dwAccess", "offHigh", "offLow", "numBytes"
        }, 5), 5, (AsmDataField){"rax", "pView"});
    AsmExtern get_module = write_common_extern("GetModuleHandleA",
        asm_win64_params((const char*[]){ "lpModuleName" }, 1), 1,
        (AsmDataField){"rax", "hInstance"});
    AsmExtern register_class = write_common_extern("RegisterClassA",
        asm_win64_params((const char*[]){ "lpWndClass" }, 1), 1,
        (AsmDataField){"ax", "atom"});
    AsmExtern create_window = write_common_extern("CreateWindowExA",
        asm_win64_params((const char*[]){
            "dwExStyle", "lpClassName", "lpWindowName", "dwStyle",
            "x", "y", "nWidth", "nHeight", "hWndParent", "hMenu", "hInstance", "lpParam"
        }, 12), 12, (AsmDataField){"rax", "hWnd"});
    AsmExtern def_window_proc = write_common_extern("DefWindowProcA",
        asm_win64_params((const char*[]){ "hWnd", "Msg", "wParam", "lParam" }, 4),
        4, (AsmDataField){"rax", "result"});
    AsmExtern post_quit = write_common_extern("PostQuitMessage",
        asm_win64_params((const char*[]){ "nExitCode" }, 1), 1, asm_void);
    AsmExtern get_message = write_common_extern("GetMessageA",
        asm_win64_params((const char*[]){
            "lpMsg", "hWnd", "wMsgFilterMin", "wMsgFilterMax"
        }, 4), 4, (AsmDataField){"eax", "result"});
    AsmExtern translate_message = write_common_extern("TranslateMessage",
        asm_win64_params((const char*[]){ "lpMsg" }, 1), 1, asm_void);
    AsmExtern dispatch_message = write_common_extern("DispatchMessageA",
        asm_win64_params((const char*[]){ "lpMsg" }, 1), 1, asm_void);

    /* Constants. The whole log file / mapped view is one 1 MB circular buffer. */
    AsmDataField LOG_SIZE = write_constant("LOG_SIZE", "100000h", "size");   /* 1 MB */

    /* Window message id handled by WndProc; WM_DESTROY posts the quit message. */
    AsmDataField WM_DESTROY = write_constant("WM_DESTROY", "2", "Msg");

    /* Initialized data: the file name and the demo log record. */
    write_data();
        AsmString logName = asm_string("logName", "\"output.log\",0", "lpFileName");
        AsmString msgStarted = asm_string("msgStarted", "\"started\",10", "src");

        /* Window class name and title (NUL-terminated C strings). */
        AsmString className = asm_string("className", "\"AsmGenWindow\",0", "lpClassName");
        AsmString windowTitle = asm_string("windowTitle", "\"AsmGen Test Window\",0", "lpWindowName");
        const char* wndClassBase = "wndClass";   /* base label, reused by the layout entry below */
        AsmDataField offsetWndClass = write_offset_constant(
            "offset_wndClass", (AsmDataField){wndClassBase, "lpWndClass", NULL}, "lpWndClass");

        /* Error records logged (via log_write) when a Win32 call fails; ,13,10 append CR LF. */
        AsmString errRegister = asm_string("errRegister", "\"RegisterClassA failed\",13,10", "src");
        AsmString errCreate = asm_string("errCreate", "\"CreateWindowExA failed\",13,10", "src");

        /* WNDCLASSA. Offsets accumulate from each field's size, so hInstance
         * lands at @24. lpfnWndProc and lpszClassName are known at assemble
         * time; only wcInstance is patched at runtime. */
        AsmLayout wnd = asm_layout_begin("WNDCLASSA", 11);
        asm_layout_field(&wnd, wndClassBase, "0", "style", 4);
        asm_layout_field(&wnd, "", "0", "pad", 4);
        asm_layout_field(&wnd, "lpfnWndProc", "offset WndProc", "lpfnWndProc", 8);
        asm_layout_field(&wnd, "cbClsExtra", "0", "cbClsExtra", 4);
        asm_layout_field(&wnd, "cbWndExtra", "0", "cbWndExtra", 4);
        int wcInstanceIndex = wnd.index;
        asm_layout_at(&wnd, "wcInstance", "0", "hInstance", 24, 8);
        asm_layout_field(&wnd, "hIcon", "0", "hIcon", 8);
        asm_layout_field(&wnd, "hCursor", "0", "hCursor", 8);
        asm_layout_field(&wnd, "hbrBackground", "0", "hbrBackground", 8);
        asm_layout_field(&wnd, "lpszMenuName", "0", "lpszMenuName", 8);
        asm_layout_field(&wnd, "lpszClassName", "offset className", "lpszClassName", 8);
        AsmMemoryLayoutRefs wndClass = write_data_memory_layout(wnd.layout);
        AsmDataField wcInstance = wndClass.fields[wcInstanceIndex];   /* "qword ptr wcInstance" */

    /* Uninitialized data: handles, the mapped base, the ring write head, and the MSG buffer. */
    write_dataq();
        AsmDataField hFile = write_unitialised_data("hFile", "hFile");
        AsmDataField hMap = write_unitialised_data("hMap", "hMap");
        AsmDataField pView = write_unitialised_data("pView", "pView");
        AsmDataField logPos = write_unitialised_data("logPos", "head");
        /* MSG. The base alias and hwnd share offset 0, so this table uses
         * explicit offsets rather than the running total. */
        const char* msgBufBase = "msgBuf";
        AsmLayout msg = asm_layout_begin("MSG", 10);
        asm_layout_at(&msg, msgBufBase, "?", "lpMsg", 0, 8);
        asm_layout_at(&msg, "msgHwnd", "?", "hwnd", 0, 8);
        asm_layout_at(&msg, "msgMessage", "?", "message", 8, 4);
        asm_layout_at(&msg, "", "?", "pad", 12, 4);
        asm_layout_at(&msg, "msgWParam", "?", "wParam", 16, 8);
        asm_layout_at(&msg, "msgLParam", "?", "lParam", 24, 8);
        asm_layout_at(&msg, "msgTime", "?", "time", 32, 4);
        asm_layout_at(&msg, "", "?", "pad", 36, 4);
        asm_layout_at(&msg, "msgPtX", "?", "pt.x", 40, 4);
        asm_layout_at(&msg, "msgPtY", "?", "pt.y", 44, 4);
        write_data_memory_layout(msg.layout);
        AsmDataField offsetMsgBuf = write_offset_constant(
            "offset_msgBuf", (AsmDataField){msgBufBase, "lpMsg", NULL}, "lpMsg");

    write_code();

    /*
     * log_write(rcx = source pointer, rdx = byte count)
     *
     * Appends the record into the 1 MB mapped ring at [pView], advancing the
     * in-memory head logPos and wrapping around the end of the buffer. A record
     * is clamped to at most LOG_SIZE bytes so a copy can never run past the
     * mapped region. It follows a private convention: src in rcx, len in rdx, no
     * return value. It is defined before main so its descriptor (log_proc) is in
     * scope at the call site inside main. It clobbers rsi/rdi (nonvolatile under
     * the Windows ABI) without saving them, which is safe only because its sole
     * caller is the entry-point main, which does not rely on them. Its rep movsb
     * copies assume the direction flag is already clear (forward), which the
     * Windows x64 ABI guarantees on entry, so no explicit cld is emitted.
     */
    /* Parameters arrive in rcx/rdx per log_write's private convention. */
    AsmDataField rcx_src = {"rcx", "src"};
    AsmDataField rdx_len = {"rdx", "len"};
    const AsmDataField log_params[] = {rcx_src, rdx_len};
    AsmExtern log_proc = write_proc("log_write", log_params, 2, asm_void);

        /* Clamp len (rdx) to LOG_SIZE. */
        const char* lw_size_ok =
            write_if_unsigned_less_than_or_equal_then_jump_to("lw_size_ok", rdx_len, LOG_SIZE);
            write_set_mov(rdx_len, LOG_SIZE);
        write_label(lw_size_ok);

        AsmDataField rsi_src = write_replace_mov("rsi", rcx_src);      /* rsi = src */
        AsmDataField r8_len = write_replace_mov("r8", rdx_len);        /* r8  = len */
        AsmDataField r9_head = write_replace_mov("r9", logPos);        /* r9  = head */
        AsmDataField rdi_pView = write_replace_mov("rdi", pView);
        write_add(rdi_pView, r9_head);            /* rdi = pView + head */
        AsmDataField rdi_dst = change_tag(rdi_pView, "dst");    /* rdi is now the write cursor */
        AsmDataField r10_size = write_replace_mov("r10", LOG_SIZE);
        write_sub(r10_size, r9_head);             /* r10 = space until end = LOG_SIZE - head */
        AsmDataField r10_space = change_tag(r10_size, "space");  /* r10 now holds the tail space */

        /* If the record fits before the end, do a single copy. */
        const char* lw_single =
            write_if_unsigned_less_than_or_equal_then_jump_to("lw_single", r8_len, r10_space);

            /* Wrap case: copy r10 bytes at the tail, the remainder at the base. */
            AsmDataField rcx_space = write_replace_mov("rcx", r10_space);
            write_rep_movsb(rdi_dst, rsi_src, rcx_space);
            write_set_mov(rdi_dst, pView);        /* restart the write cursor at the buffer base */
            AsmDataField rcx_len = write_replace_mov("rcx", r8_len);
            write_sub(rcx_len, r10_space);            /* remaining = len - space */
            write_rep_movsb(rdi_dst, rsi_src, rcx_len);
            AsmDataField r9_len = write_replace_mov("r9", r8_len);
            write_sub(r9_len, r10_space);             /* new head = len - space */
            r9_head = change_tag(r9_len, "head");     /* r9 now holds the new head again */
            write_set_mov(logPos, r9_head);
            write_ret();

        write_label(lw_single);
        rcx_len = write_replace_mov("rcx", r8_len);
        write_rep_movsb(rdi_dst, rsi_src, rcx_len);
        write_add(r9_head, r8_len);               /* head += len */
        const char* lw_store =
            write_if_unsigned_less_than_then_jump_to("lw_store", r9_head, LOG_SIZE);
            write_sub(r9_head, LOG_SIZE);             /* wrap (head == LOG_SIZE -> 0) */
        write_label(lw_store);
        write_set_mov(logPos, r9_head);
        write_ret();

    write_endp(&log_proc);

    /*
     * WndProc(rcx = hWnd, rdx = Msg, r8 = wParam, r9 = lParam)
     *
     * The window procedure Windows invokes for each message. It handles only
     * WM_DESTROY (posts WM_QUIT so the message loop in main can end) and forwards
     * everything else to DefWindowProcA. It uses only volatile registers and does
     * no pushes or persistent stack adjustment, so rsp stays 16-byte aligned for
     * a call (8 mod 16 at entry); each write_extern_call lays down its own shadow
     * frame, exactly like main. On the default path rcx/rdx/r8/r9 still hold the
     * original four arguments, so they are forwarded to DefWindowProcA unchanged.
     */
    static const AsmDataField wndproc_params[] = {
        {"rcx", "hWnd"}, {"rdx", "Msg"}, {"r8", "wParam"}, {"r9", "lParam"}
    };
    AsmDataField rax_lresult = {"rax", "result"};
    AsmExtern wnd_proc = write_proc("WndProc", wndproc_params, 4, rax_lresult);

        /* if (Msg != WM_DESTROY) goto wp_default; */
        AsmDataField edx_msg = {"edx", "Msg"};
        const char* wp_default =
            write_if_not_equal_then_jump_to("wp_default", edx_msg, WM_DESTROY);

            /* WM_DESTROY: PostQuitMessage(0); return 0; */
            write_extern_call(&post_quit, literal("0", "nExitCode"));
            write_xor(wnd_proc.return_reg, wnd_proc.return_reg);
            write_ret();

        write_label(wp_default);
        /* return DefWindowProcA(hWnd, Msg, wParam, lParam); (args already in place) */
        write_extern_call(&def_window_proc,
            (AsmDataField){"rcx", "hWnd"},
            (AsmDataField){"rdx", "Msg"},
            (AsmDataField){"r8",  "wParam"},
            (AsmDataField){"r9",  "lParam"});
        write_ret();

    write_endp(&wnd_proc);

    /* ---- main: map the log file, log a record, create a window, then exit 123. ---- */
    AsmDataField eax_exit = {"eax", "exit_code"};
    AsmExtern main_proc = write_proc("main", NULL, 0, eax_exit);
        /* write_proc committed the stack: this image has no __chkstk. */

        /*
         * CreateFileA(logName, GENERIC_READ|GENERIC_WRITE, 0, NULL,
         *             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)
         * GENERIC_READ|GENERIC_WRITE = 0C0000000h, CREATE_ALWAYS = 2,
         * FILE_ATTRIBUTE_NORMAL = 80h. CREATE_ALWAYS truncates any existing file to
         * zero length so each launch starts from a guaranteed-empty buffer; the
         * mapping below then re-extends it to a fresh, zero-filled 1 MB.
         */
        write_extern_call(&create_file,
            logName.addr,
            literal("0C0000000h", "dwDesiredAccess"),
            literal("0",          "dwShareMode"),
            literal("0",          "lpSecurityAttributes"),
            literal("2",          "dwCreationDisposition"),
            literal("80h",        "dwFlagsAndAttributes"),
            literal("0",          "hTemplateFile"));
        AsmDataField rax_hFile = create_file.return_reg;
        write_set_mov(hFile, rax_hFile);

        /*
         * CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, LOG_SIZE, NULL)
         * PAGE_READWRITE = 4. Passing LOG_SIZE as the maximum size grows the
         * backing file to exactly 1 MB (it is zero-filled when newly created).
         */
        write_extern_call(&create_mapping,
            hFile,
            literal("0",           "lpAttributes"),
            literal("4",           "flProtect"),
            literal("0",           "dwMaxHigh"),
            treat_as(LOG_SIZE, "size", "dwMaxLow"),
            literal("0",           "lpName"));
        AsmDataField rax_hMap = create_mapping.return_reg;
        assert_type(rax_hMap);
        write_set_mov(hMap, rax_hMap);

        /*
         * MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0)
         * FILE_MAP_ALL_ACCESS = 0F001Fh; numBytes = 0 maps the whole 1 MB file.
         */
        write_extern_call(&map_view,
            hMap,
            literal("0F001Fh", "dwAccess"),
            literal("0",       "offHigh"),
            literal("0",       "offLow"),
            literal("0",       "numBytes"));
        AsmDataField rax_pView = map_view.return_reg;
        assert_type(rax_pView);
        write_set_mov(pView, rax_pView);

        /* Ring write head starts at offset 0 for this run. */
        AsmDataField zero_head = literal("0", logPos.type);
        write_set_mov(logPos, zero_head);

        /* Demo: log one record through the circular buffer (src in rcx, len in rdx). */
        write_call(&log_proc, msgStarted.addr, msgStarted.len);

        /*
         * GetModuleHandleA(NULL) returns this image's base = the HINSTANCE the
         * window class and CreateWindowExA are scoped to; store it in the class.
         */
        write_extern_call(&get_module, literal("0", "lpModuleName"));
        AsmDataField rax_hInstance = get_module.return_reg;
        write_set_mov(wcInstance, rax_hInstance);

        /* RegisterClassA(&wndClass); ax = class atom (0 on failure). */
        write_extern_call(&register_class, offsetWndClass);
        AsmDataField ax_atom = register_class.return_reg;
        AsmDataField zero_atom = literal("0", "atom");
        const char* mw_class_ok =
            write_if_not_equal_then_jump_to("mw_class_ok", ax_atom, zero_atom);
            /* Registration failed: log it and bail out with a nonzero code. */
            write_call(&log_proc, errRegister.addr, errRegister.len);
            write_extern_call(&exit_process, literal("1", "exit_code"));
        write_label(mw_class_ok);

        /*
         * CreateWindowExA(0, className, windowTitle,
         *                 WS_OVERLAPPEDWINDOW|WS_VISIBLE = 10CF0000h,
         *                 CW_USEDEFAULT x4 = 80000000h, NULL, NULL, hInstance, NULL)
         * Returns the new HWND in rax (NULL on failure).
         */
        write_extern_call(&create_window,
            literal("0",          "dwExStyle"),
            className.addr,
            windowTitle.addr,
            literal("10CF0000h",  "dwStyle"),
            literal("80000000h",  "x"),
            literal("80000000h",  "y"),
            literal("80000000h",  "nWidth"),
            literal("80000000h",  "nHeight"),
            literal("0",          "hWndParent"),
            literal("0",          "hMenu"),
            wcInstance,
            literal("0",          "lpParam"));
        AsmDataField rax_hWnd = create_window.return_reg;
        AsmDataField zero_hWnd = literal("0", "hWnd");
        const char* mw_window_ok =
            write_if_not_equal_then_jump_to("mw_window_ok", rax_hWnd, zero_hWnd);
            /* Window creation failed: log it and bail out with a nonzero code. */
            write_call(&log_proc, errCreate.addr, errCreate.len);
            write_extern_call(&exit_process, literal("1", "exit_code"));
        write_label(mw_window_ok);

        /*
         * Message loop. GetMessageA returns >0 for a normal message, 0 for
         * WM_QUIT (posted by WndProc on WM_DESTROY), and -1 on error; treat <=0
         * as "stop" so a quit or an error both end the loop.
         */
        const char* mw_msg_loop = write_label("mw_msg_loop");
        write_extern_call(&get_message,
            offsetMsgBuf,
            literal("0", "hWnd"),
            literal("0", "wMsgFilterMin"),
            literal("0", "wMsgFilterMax"));
        AsmDataField eax_result = get_message.return_reg;
        AsmDataField zero_result = literal("0", "result");
        const char* mw_msg_done =
            write_if_signed_less_than_or_equal_then_jump_to("mw_msg_done", eax_result, zero_result);
            write_extern_call(&translate_message, offsetMsgBuf);
            write_extern_call(&dispatch_message, offsetMsgBuf);
            write_jmp(mw_msg_loop);
        write_label(mw_msg_done);

        /* Exit with code 123 once the window has been closed. */
        write_extern_call(&exit_process, literal("123", "exit_code"));

    write_endp(&main_proc);


    write_end("");

    return write_asm_file("output.asm");
}
