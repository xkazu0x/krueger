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

set cl_common=/D_CRT_SECURE_NO_WARNINGS /nologo /I..\src\ /std:c11 /W4 /wd4100 /wd4200 /wd4310
set cl_debug=call cl /Od /DBUILD_DEBUG=1 %cl_common%
set cl_release=call cl /O2 /DBUILD_DEBUG=0 %cl_common%
set cl_shared=/LD
set cl_link=
set cl_out=/Fe:

set clang_common=-D_CRT_SECURE_NO_WARNINGS -I../src/ -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter
set clang_debug=call clang -O0 -g -DBUILD_DEBUG=1 %clang_common%
set clang_release=call clang -O2 -DBUILD_DEBUG=0 %clang_common%
set clang_shared=-shared
set clang_link=
set clang_out=-o

if "%msvc%"=="1" set compile_debug=%cl_debug%
if "%msvc%"=="1" set compile_release=%cl_release%
if "%msvc%"=="1" set shared=%cl_shared%
if "%msvc%"=="1" set link=%cl_link%
if "%msvc%"=="1" set out=%cl_out%

if "%clang%"=="1" set compile_debug=%clang_debug%
if "%clang%"=="1" set compile_release=%clang_release%
if "%clang%"=="1" set shared=%clang_shared%
if "%clang%"=="1" set link=%clang_link%
if "%clang%"=="1" set out=%clang_out%

if "%debug%"=="1"   set compile=%compile_debug%
if "%release%"=="1" set compile=%compile_release%

if not exist build mkdir build

pushd build
%compile% %shared% ..\src\krueger.c %link% %out% libkrueger.dll || exit /b 1
%compile% ..\src\krueger_main.c %link% %out% krueger.exe || exit /b 1
if "%run%"=="1" call krueger.exe
popd
