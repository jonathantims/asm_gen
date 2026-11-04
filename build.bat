@echo off
rem Repository settings are environment variables set by config.bat.
call "%~dp0config.bat"
call "%VCVARS64%" >nul 2>&1

rem Every artifact is written in out\. Generators and the programs they emit
rem use relative filenames, so this directory is the working directory for
rem the rest of the build. Delete out\ to clean.
if not exist "%~dp0out\" mkdir "%~dp0out"
pushd "%~dp0out" || exit /b 1
call :main
set RC=%errorlevel%
popd
exit /b %RC%

:main
rem ======================================================================
rem  Sources are organised into component folders:
rem    core\   - the MASM x64 emitter (asm.h, asm_funcs.h) + its demo asm_main.c
rem    c\      - the typed-value / register layer (c_funcs.h) + c_main.c
rem    gc\     - the swappable garbage collectors (gc*.h, gc.h)
rem    cpp\ csharp\ js\ py\ rust\  - the per-language wrapper tours
rem    test\   - the shared test harness
rem  All #include "..." are plain quoted names; %INC% adds every component
rem  folder to the compiler's header search path so nothing needs rewriting.
rem  The working directory is out\, so asm, obj, exe, ilk, log, and out.txt
rem  files land there and are git-ignored.
rem ======================================================================
set INC=/I ..\core /I ..\c /I ..\gc /I ..\cpp /I ..\csharp /I ..\js /I ..\py /I ..\rust

rem Generators and the test harness link without the C runtime.
rem /GS- drops the security cookie. /Gs262144 keeps frames under 256 KiB
rem from calling __chkstk, which this link does not have. mainCRTStartup
rem runs commit_stack_pages first. host_setjmp.obj is setjmp/longjmp,
rem generated from host_setjmp.h before any other generator links it.
rem That generator cannot link the object it is about to emit.
set "CC=cl /nologo /GS- /Gs262144 /Oi-"
set "GENLIBS=host_setjmp.obj kernel32.lib user32.lib"
set "GENLINK=/link /NODEFAULTLIB /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup"

%CC% %INC% ..\core\host_setjmp_main.c kernel32.lib user32.lib %GENLINK%
if errorlevel 1 exit /b 1
.\host_setjmp_main.exe
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo host_setjmp.obj host_setjmp.asm
if errorlevel 1 exit /b 1

rem --- Build output.exe from core\asm_main.c (asm_funcs.h) ---
%CC% %INC% ..\core\asm_main.c %GENLIBS% %GENLINK%
if errorlevel 1 exit /b 1
.\asm_main.exe
if errorlevel 1 exit /b 1
ml64 /nologo /c output.asm
if errorlevel 1 exit /b 1
rem /STACK is 2 MiB. Generated main walks just over 1 MiB (commit_stack_pages),
rem which does not fit in the default 1 MiB reserve.
link /nologo /subsystem:windows /entry:main /STACK:2097152 output.obj kernel32.lib user32.lib
if errorlevel 1 exit /b 1

rem --- Build c_output.exe from c\c_main.c (c_funcs.h) ---
%CC% %INC% ..\c\c_main.c %GENLIBS% %GENLINK%
if errorlevel 1 exit /b 1
.\c_main.exe
if errorlevel 1 exit /b 1
ml64 /nologo /c c_output.asm
if errorlevel 1 exit /b 1
link /nologo /subsystem:windows /entry:main /STACK:2097152 c_output.obj kernel32.lib user32.lib
if errorlevel 1 exit /b 1

rem --- Build and run the shared test suite against both exes ---
%CC% %INC% ..\test\test.c %GENLIBS% %GENLINK%
if errorlevel 1 exit /b 1
.\test.exe output.exe c_output.exe
if errorlevel 1 exit /b 1

rem ======================================================================
rem  Language-wrapper demos.
rem
rem  cpp is a non-GC tour: one generator -> cpp_output.asm -> cpp_output.exe.
rem
rem  csharp/rust/py/js each embed the swappable garbage-collector tour, so
rem  they are built across ALL FOUR collector options (the 16-cell matrix:
rem  4 languages x 4 options). The collector is chosen with -DGC_OPTION and
rem  the generator emits <lang>_output_<option>.asm, which assembles/links to
rem  a console <lang>_output_<option>.exe that prints the tour and exits 0.
rem  The generator exe is <lang>_main.exe (distinct from the program exes).
rem ======================================================================

call :build_lang cpp    "[C++] feature tour"
if errorlevel 1 exit /b 1

call :build_gc csharp "[C#] feature tour" "MethodTable dispatch" "box holds an Int32" "box payload 42"
if errorlevel 1 exit /b 1
call :build_gc rust   "[Rust] feature tour"
if errorlevel 1 exit /b 1
call :build_gc py     "[Python] feature tour"
if errorlevel 1 exit /b 1
call :build_gc js     "[JavaScript] feature tour" "-> 1" "state persisted" "fresh env"
if errorlevel 1 exit /b 1

rem --- Intra-option flag spot-checks (the alternative behaviours) ----------
echo --- GC flag spot-checks ---
call :gc_flag py refcount     "/DGC_CYCLES=trial"  "[Python] feature tour" cycles
if errorlevel 1 exit /b 1
call :gc_flag py marksweep    "/DGC_COALESCE"      "[Python] feature tour" coalesce
if errorlevel 1 exit /b 1
call :gc_flag py compact      "/DGC_MOVE=sliding"  "[Python] feature tour" sliding
if errorlevel 1 exit /b 1
call :gc_flag py conservative "/DGC_SCAN=full"     "[Python] feature tour" scanfull
if errorlevel 1 exit /b 1

rem --- Lower-layer spill toggle: verify c_funcs.h compiles with spilling on -
echo --- C_ENABLE_SPILL compile check ---
%CC% /c /DGC_OPTION=marksweep /DC_ENABLE_SPILL %INC% ..\py\py_main.c
if errorlevel 1 exit /b 1
echo   [C_ENABLE_SPILL] compiles OK

rem --- examples/ code-generation showcase (each built enabled + disabled) ---
echo --- examples/ showcase ---
call ..\examples\build_examples.bat
if errorlevel 1 exit /b 1

echo All language-wrapper demos built and passed.
exit /b 0

rem --- :build_lang <lang> <marker> ---------------------------------------
rem Compile the generator, run it to emit <lang>_output.asm, assemble, link a
rem console exe, then run the console test asserting exit 0 and the marker line.
:build_lang
set LANG=%~1
set MARKER=%~2
%CC% %INC% ..\%LANG%\%LANG%_main.c %GENLIBS% %GENLINK%
if errorlevel 1 exit /b 1
.\%LANG%_main.exe
if errorlevel 1 exit /b 1
ml64 /nologo /c %LANG%_output.asm
if errorlevel 1 exit /b 1
link /nologo /subsystem:console /entry:main /STACK:2097152 /OUT:%LANG%_output.exe %LANG%_output.obj kernel32.lib
if errorlevel 1 exit /b 1
.\test.exe --console %LANG%_output.exe "%MARKER%"
if errorlevel 1 exit /b 1
exit /b 0

rem --- :build_gc <lang> <marker> -----------------------------------------
rem Build one GC-language tour against every collector option (4 cells). The
rem generator is recompiled per option with -DGC_OPTION so each emits its own
rem <lang>_output_<option>.asm; each resulting console exe must print the
rem language marker and exit 0.
:build_gc
set LANG=%~1
set MARKER=%~2
set X1=%~3
set X2=%~4
set X3=%~5
for %%O in (refcount marksweep compact conservative) do (
  %CC% /DGC_OPTION=%%O %INC% ..\%LANG%\%LANG%_main.c %GENLIBS% %GENLINK% || exit /b 1
  .\%LANG%_main.exe || exit /b 1
  ml64 /nologo /c %LANG%_output_%%O.asm || exit /b 1
  link /nologo /subsystem:console /entry:main /STACK:2097152 /OUT:%LANG%_output_%%O.exe %LANG%_output_%%O.obj kernel32.lib || exit /b 1
  if not "%X3%"=="" (
    .\test.exe --console %LANG%_output_%%O.exe "%MARKER%" "%X1%" "%X2%" "%X3%" || exit /b 1
  ) else if not "%X2%"=="" (
    .\test.exe --console %LANG%_output_%%O.exe "%MARKER%" "%X1%" "%X2%" || exit /b 1
  ) else if not "%X1%"=="" (
    .\test.exe --console %LANG%_output_%%O.exe "%MARKER%" "%X1%" || exit /b 1
  ) else (
    .\test.exe --console %LANG%_output_%%O.exe "%MARKER%" || exit /b 1
  )
  echo   [%LANG% x %%O] OK
)
exit /b 0

rem --- :gc_flag <lang> <opt> <flags> <marker> <tag> ----------------------
rem Build one cell with an extra intra-option flag enabled, linking to a
rem distinctly named exe so it does not clobber the plain-option artifacts.
:gc_flag
set LANG=%~1
set OPT=%~2
set FLAGS=%~3
set MARKER=%~4
set TAG=%~5
%CC% /DGC_OPTION=%OPT% %FLAGS% %INC% ..\%LANG%\%LANG%_main.c %GENLIBS% %GENLINK%
if errorlevel 1 exit /b 1
.\%LANG%_main.exe
if errorlevel 1 exit /b 1
ml64 /nologo /c %LANG%_output_%OPT%.asm
if errorlevel 1 exit /b 1
link /nologo /subsystem:console /entry:main /STACK:2097152 /OUT:%LANG%_output_%OPT%_%TAG%.exe %LANG%_output_%OPT%.obj kernel32.lib
if errorlevel 1 exit /b 1
.\test.exe --console %LANG%_output_%OPT%_%TAG%.exe "%MARKER%"
if errorlevel 1 exit /b 1
echo   [%LANG% x %OPT% + %FLAGS%] OK
exit /b 0
