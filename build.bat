@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

for %%a in (%*) do set "%%a=1"
if not "%msvc%"=="1"    if not "%clang%"=="1" set msvc=1
if not "%release%"=="1" set debug=1
if "%debug%"=="1"       set release=0 && echo [debug mode]
if "%release%"=="1"     set debug=0 && echo [release mode]
if "%msvc%"=="1"        set clang=0 && echo [msvc compile]
if "%clang%"=="1"       set msvc=0 && echo [clang compile]

set cl_common=  /D_CRT_SECURE_NO_WARNINGS /nologo /I..\src\ /std:c11
set cl_debug=   call cl /Od /DBUILD_DEBUG=1 %cl_common%
set cl_release= call cl /O2 /DBUILD_DEBUG=1 %cl_common%
set cl_link=
set cl_out=     /Fe:

set shared=/LD 

if "%msvc%"=="1" set compile_debug=%cl_debug%
if "%msvc%"=="1" set compile_release=%cl_release%
if "%msvc%"=="1" set compile_dll=%cl_dll%
if "%msvc%"=="1" set link=%cl_link%
if "%msvc%"=="1" set out=%cl_out%

if "%debug%"=="1"   set compile=%compile_debug%
if "%release%"=="1" set compile=%compile_release%

if not exist build mkdir build

pushd build
%compile%  %shared% ..\src\krueger_shared.c %link% %out% libkrueger.dll || exit /b 1
%compile%           ..\src\krueger_main.c   %link% %out% krueger.exe || exit /b 1
if "%run%"=="1" call krueger.exe
popd
