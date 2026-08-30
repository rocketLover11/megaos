@echo off
setlocal enabledelayedexpansion

set PROJECT_ROOT=%~dp0
set BUILD_DIR=%PROJECT_ROOT%build
set TOOLCHAIN_FILE=%PROJECT_ROOT%cmake\x86_64-elf-toolchain.cmake

set CMD=%1
if "%CMD%"=="" set CMD=all

if "%CMD%"=="clean" goto :clean
if "%CMD%"=="configure" goto :configure
if "%CMD%"=="build" goto :build
if "%CMD%"=="iso" goto :iso
if "%CMD%"=="run" goto :run
if "%CMD%"=="all" goto :all
goto :usage

:clean
echo ==^> Cleaning build directory
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
goto :eof

:configure
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
echo ==^> Configuring
cmake -S "%PROJECT_ROOT%" -B "%BUILD_DIR%" -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_FILE%"
goto :eof

:build
call :configure
echo ==^> Building kernel
cmake --build "%BUILD_DIR%" --target kernel
goto :eof

:iso
call :configure
echo ==^> Building ISO
cmake --build "%BUILD_DIR%" --target iso
goto :eof

:run
echo ==^> Launching QEMU
cmake --build "%BUILD_DIR%" --target run
goto :eof

:all
call :clean
call :configure
call :run
goto :eof

:usage
echo Usage: %~n0 [clean^|configure^|build^|iso^|run^|all]
exit /b 1