@echo off
rem ======================================================================
rem  config.bat — repository configuration.
rem
rem  Machine-specific settings live here as environment variables.
rem  build.bat and examples\build_examples.bat call this script and then
rem  use the variables it set. Do not add setlocal: the caller's
rem  environment is what those scripts read.
rem
rem  A variable that is already set is left unchanged. Set it in the
rem  environment before the build for a one-off override, or edit the
rem  default below for this machine.
rem
rem  Variables
rem
rem  VCVARS64
rem    Full path of Visual Studio's x64 developer script, vcvars64.bat.
rem    After this script returns, the build calls that path. The call
rem    puts cl, ml64, and link on PATH and sets INCLUDE and LIB.
rem    The default is Visual Studio.
rem ======================================================================
if "%VCVARS64%"=="" set "VCVARS64=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
