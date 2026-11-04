@echo off
rem ======================================================================
rem  build_examples.bat - build, run and validate the examples/ showcase.
rem
rem  Each example is a self-contained code-generation demo of a *generation-
rem  time* debugging/instrumentation feature woven in only when a flag is set,
rem  with ZERO FOOTPRINT when it is not. Every example is built twice from the
rem  SAME source:
rem    - "enabled"  : compiled with /DEX_ENABLE (the feature is switched on),
rem    - "disabled" : compiled without it (the baseline shipped program).
rem  Both are generated, assembled, linked and run; the script asserts the
rem  expected exit code of each and confirms the disabled assembly contains no
rem  trace of the feature (the zero-footprint claim). :need checks feature
rem  lines in the captured output and in ex11's causal_rank.txt. ex1 also
rem  builds EX_FP_TREE and expects exit 1.
rem
rem  Run standalone from anywhere, or via the repo-root build.bat (which calls
rem  it). Artifacts go in the shared ..\out directory, same as build.bat.
rem ======================================================================
setlocal
rem Repository settings are environment variables set by ..\config.bat.
call "%~dp0..\config.bat"
call "%VCVARS64%" >nul 2>&1
if not exist "%~dp0..\out\" mkdir "%~dp0..\out"
pushd "%~dp0..\out" || exit /b 1
call :examples_main
set RC=%errorlevel%
popd
endlocal & exit /b %RC%

:examples_main
rem ex_console.h stays in ..\examples. Each example's sources live in a
rem subfolder, so a quoted include of ex_console.h needs that directory on /I.
set INC=/I ..\examples /I ..\core /I ..\c /I ..\cpp
rem Same no-CRT link as the repo-root build. Commit is commit_stack_pages
rem in the generator, and the emitted main walks the stack itself.
set "CC=cl /nologo /GS- /Gs262144 /Oi-"
set "GENLIBS=host_setjmp.obj kernel32.lib user32.lib"
set "GENLINK=/link /NODEFAULTLIB /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup"
rem Same generated setjmp as the repo-root build. This script can run alone,
rem so it emits the image here instead of assuming build.bat already did.
%CC% %INC% ..\core\host_setjmp_main.c kernel32.lib user32.lib %GENLINK%
if errorlevel 1 exit /b 1
.\host_setjmp_main.exe
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo host_setjmp.obj host_setjmp.asm
if errorlevel 1 exit /b 1

echo === examples/ code-generation showcase ===

rem :ex NAME SRC GENBASE ONEXIT OFFEXIT ABSENT
rem   NAME    - artifact prefix (NAME_on.exe / NAME_off.exe)
rem   SRC     - generator .c
rem   GENBASE - emitted <GENBASE>_output.asm / .obj
rem   ONEXIT  - required exit code of the enabled build
rem   OFFEXIT - required exit code of the disabled build
rem   ABSENT  - a feature-only token that must NOT appear in the disabled asm
call :ex ex1  ..\examples\ex1_fpaccuracy\ex1_fpaccuracy.c  ex1  1 0 fp_trap "abs error x1e6 = " || goto :fail
call :need ex1_on.out.txt "[TRAP] floating-point error exceeded threshold" || goto :fail
rem EX_FP_TREE is the third setting. The enabled build above is the shadow.
%CC% /DEX_ENABLE /DEX_FP_TREE %INC% ..\examples\ex1_fpaccuracy\ex1_fpaccuracy.c /Fe:ex1_tree_gen.exe %GENLIBS% %GENLINK% >nul 2>&1 || (echo   [ex1] FAIL compile-tree & goto :fail)
.\ex1_tree_gen.exe >nul 2>&1 || (echo   [ex1] FAIL generate-tree & goto :fail)
ml64 /nologo /c ex1_output.asm >nul 2>&1 || (echo   [ex1] FAIL assemble-tree & goto :fail)
link /nologo /subsystem:console /entry:main /STACK:2097152 /OUT:ex1_tree.exe ex1_output.obj kernel32.lib >nul 2>&1 || (echo   [ex1] FAIL link-tree & goto :fail)
.\ex1_tree.exe >nul 2>&1
set RC=%errorlevel%
if not "%RC%"=="1" (echo   [ex1] FAIL tree exit=%RC% expected=1 & goto :fail)
echo   [ex1] OK  tree exit=1
call :ex ex2  ..\examples\ex2_overflow\ex2_overflow.c  ex2  1 0 ovf_trap       || goto :fail
call :ex ex3  ..\examples\ex3_profile\ex3_profile.c   ex3  0 0 rdtsc          || goto :fail
call :need ex3_on.out.txt "function,ticks" || goto :fail
call :need ex3_on.out.txt "work_small," || goto :fail
call :need ex3_on.out.txt "work_large," || goto :fail
call :need ex3_off.out.txt "[done] profiling disabled; no timing compiled in" || goto :fail
call :ex ex4  ..\examples\ex4_contracts\ex4_contracts.c ex4  1 0 ct_trap "balance = 100" || goto :fail
call :ex ex5  ..\examples\ex5_layout\ex5_layout.c    ex5  0 0 Monster_hp     || goto :fail
call :need ex5_on.out.txt "sum hp  = 100" || goto :fail
call :need ex5_on.out.txt "sum atk = 17" || goto :fail
call :need ex5_on.out.txt "sum dmg = 7" || goto :fail
call :need ex5_on.out.txt "sum loy = 8" || goto :fail
call :need ex5_on.out.txt "sum mrk = 5" || goto :fail
call :need ex5_off.out.txt "sum hp  = 100" || goto :fail
call :need ex5_off.out.txt "sum atk = 17" || goto :fail
call :need ex5_off.out.txt "sum dmg = 7" || goto :fail
call :need ex5_off.out.txt "sum loy = 8" || goto :fail
call :need ex5_off.out.txt "sum mrk = 5" || goto :fail
call :ex ex6  ..\examples\ex6_uaf\ex6_uaf.c       ex6  1 0 uaf_trap       || goto :fail
call :need ex6_on.out.txt "use A (live)  = 111" || goto :fail
call :need ex6_on.out.txt "use B (live)  = 222" || goto :fail
call :need ex6_on.out.txt "[TRAP] use after free" || goto :fail
call :need ex6_off.out.txt "use A (live)  = 111" || goto :fail
call :need ex6_off.out.txt "use B (live)  = 222" || goto :fail
call :need ex6_off.out.txt "use A (freed) = 333" || goto :fail
call :ex ex7  ..\examples\ex7_fuzz\ex7_fuzz.c      ex7  0 0 run_all_tests "PASS mul2" || goto :fail
call :need ex7_on.out.txt "ex7 done" || goto :fail
call :need ex7_off.out.txt "ex7 done" || goto :fail
call :ex ex8  ..\examples\ex8_fuzzmock\ex8_fuzzmock.c  ex8  0 0 run_all_tests  || goto :fail
call :need ex8_on.out.txt "read_and_double() = 14" || goto :fail
call :need ex8_off.out.txt "read_and_double() = 14" || goto :fail
call :ex ex9  ..\examples\ex9_exceptions\ex9_exceptions.c ex9 0 0 veh_trap       || goto :fail
call :need ex9_on.out.txt "[CAUGHT] overdraft, code = 42" || goto :fail
call :need ex9_off.out.txt "result = 4294967246" || goto :fail
call :ex ex10 ..\examples\ex10_bounds\ex10_bounds.c   ex10 1 0 bounds_trap    || goto :fail
call :need ex10_on.out.txt "arr[2] = 30" || goto :fail
call :need ex10_on.out.txt "[TRAP] index out of bounds" || goto :fail
call :need ex10_off.out.txt "arr[2] = 30" || goto :fail
call :need ex10_off.out.txt "arr[5] = " || goto :fail
call :ex ex11 ..\examples\ex11_causal\ex11_causal.c   ex11 0 0 causal_delay   || goto :fail
call :need causal_rank.txt "critical_step" || goto :fail
call :need causal_rank.txt "side_step" || goto :fail
call :need causal_rank.txt " e2e=" || goto :fail
call :ex ex12 ..\examples\ex12_quantisation\ex12_quantisation.c ex12 0 0 q_update "mean A x100 = 1500" "mean A x100 = 1725" || goto :fail
call :need ex12_on.out.txt "var  A x100 = 80" || goto :fail
call :need ex12_on.out.txt "mean B x100 = 1900" || goto :fail
call :need ex12_on.out.txt "var  B x100 = 80" || goto :fail
call :need ex12_off.out.txt "var  A x100 = 156" || goto :fail
call :need ex12_off.out.txt "mean B x100 = 2031" || goto :fail
call :need ex12_off.out.txt "var  B x100 = 272" || goto :fail

echo All examples built, ran, and passed (zero-footprint verified).
exit /b 0

:fail
echo examples build FAILED.
exit /b 1

rem --- :need FILE TEXT ---------------------------------------------------
rem findstr /C: is a literal search. A missing line fails the build.
:need
findstr /C:"%~2" "%~1" >nul
if errorlevel 1 (
    echo   FAIL missing "%~2" in %~1
    exit /b 1
)
exit /b 0

rem --- :ex NAME SRC GENBASE ONEXIT OFFEXIT ABSENT ------------------------
:ex
setlocal
set NAME=%~1
set SRC=%~2
set GEN=%~3
set ONEXIT=%~4
set OFFEXIT=%~5
set ABSENT=%~6
set ONTEXT=%~7
set OFFTEXT=%~8

rem ---- enabled build (/DEX_ENABLE) ----
%CC% /DEX_ENABLE %INC% %SRC% /Fe:%NAME%_on_gen.exe %GENLIBS% %GENLINK% >nul 2>&1 || (echo   [%NAME%] FAIL compile-enabled & endlocal & exit /b 1)
.\%NAME%_on_gen.exe >nul 2>&1 || (echo   [%NAME%] FAIL generate-enabled & endlocal & exit /b 1)
ml64 /nologo /c %GEN%_output.asm >nul 2>&1 || (echo   [%NAME%] FAIL assemble-enabled & endlocal & exit /b 1)
link /nologo /subsystem:console /entry:main /STACK:2097152 /OUT:%NAME%_on.exe %GEN%_output.obj kernel32.lib >nul 2>&1 || (echo   [%NAME%] FAIL link-enabled & endlocal & exit /b 1)

rem ---- disabled build (baseline) ----
%CC% %INC% %SRC% /Fe:%NAME%_off_gen.exe %GENLIBS% %GENLINK% >nul 2>&1 || (echo   [%NAME%] FAIL compile-disabled & endlocal & exit /b 1)
.\%NAME%_off_gen.exe >nul 2>&1 || (echo   [%NAME%] FAIL generate-disabled & endlocal & exit /b 1)
ml64 /nologo /c %GEN%_output.asm >nul 2>&1 || (echo   [%NAME%] FAIL assemble-disabled & endlocal & exit /b 1)
link /nologo /subsystem:console /entry:main /STACK:2097152 /OUT:%NAME%_off.exe %GEN%_output.obj kernel32.lib >nul 2>&1 || (echo   [%NAME%] FAIL link-disabled & endlocal & exit /b 1)

rem ---- run enabled, assert exit code and optional text ----
.\%NAME%_on.exe > %NAME%_on.out.txt 2>&1
set RC=%errorlevel%
if not "%RC%"=="%ONEXIT%" (echo   [%NAME%] FAIL enabled exit=%RC% expected=%ONEXIT% & endlocal & exit /b 1)
if not "%ONTEXT%"=="" (
    findstr /C:"%ONTEXT%" %NAME%_on.out.txt >nul || (echo   [%NAME%] FAIL enabled text '%ONTEXT%' & endlocal & exit /b 1)
)

rem ---- run disabled, assert exit code and optional text ----
.\%NAME%_off.exe > %NAME%_off.out.txt 2>&1
set RC=%errorlevel%
if not "%RC%"=="%OFFEXIT%" (echo   [%NAME%] FAIL disabled exit=%RC% expected=%OFFEXIT% & endlocal & exit /b 1)
if not "%OFFTEXT%"=="" (
    findstr /C:"%OFFTEXT%" %NAME%_off.out.txt >nul || (echo   [%NAME%] FAIL disabled text '%OFFTEXT%' & endlocal & exit /b 1)
)

rem ---- zero-footprint: the feature token must be absent from the disabled asm
rem      (%GEN%_output.asm currently holds the disabled/baseline emission) ----
findstr /C:"%ABSENT%" %GEN%_output.asm >nul 2>&1 && (echo   [%NAME%] FAIL zero-footprint: '%ABSENT%' present in baseline & endlocal & exit /b 1)

echo   [%NAME%] OK  enabled=%ONEXIT% disabled=%OFFEXIT%  zero-footprint(%ABSENT%) clean
endlocal
exit /b 0
