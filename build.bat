@echo off
setlocal enabledelayedexpansion

goto :main

: Check if command exists and get it from scoop if not
:check
if exist "%userprofile%\scoop\apps\%~2\current\" (
	set "path=%path%;%userprofile%\scoop\apps\%~2\current\bin"
	exit /b
)
where %~1 >nul 2>nul
if %errorlevel% == 0 exit /b
call scoop install %~2
if %errorlevel% == 0 (
	set "path=%path%;%userprofile%\scoop\apps\%~2\current\bin"
	exit /b
)
powershell -Command "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser; Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression"
call scoop install %~2
set "path=%path%;%userprofile%\scoop\apps\%~2\current\bin"
exit /b
:main
if not exist ".bin\" (
	rmdir /s /q .bin2 2>nul
	mkdir .bin2 2>nul
	call :check git git

	: glad-everywhere
	call git clone https://github.com/BlobTheKat/glad-everywhere --depth=1
	move glad-everywhere\include .bin2 >nul
	move glad-everywhere\src .bin2 >nul
	rmdir /s /q glad-everywhere

	: SDL2
	call git clone -b SDL2 https://github.com/libsdl-org/SDL --depth=1
	call cmake -S SDL -B SDL/build -DCMAKE_BUILD_TYPE=Release -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_DYNAMIC_API=OFF -DSDL_RENDER_DISABLED=ON -DSDL_RENDER_D3D=OFF -DSDL_RENDER_METAL=OFF -DSDL_VULKAN=OFF -DSDL_TEST=OFF
	if %errorlevel% neq 0 exit
	call cmake --build SDL/build --config Release --parallel
	if %errorlevel% neq 0 exit
	move SDL\include .bin2\include\SDL2 >nul
	move SDL\build\Release .bin2\lib >nul
	rmdir /s /q SDL
	echo id ICON icon.ico> .bin2\icon.rc

	: v8
	md v8
	cd v8
	call git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git --depth=1
	set "path=%path%;%CD%/depot_tools"
	echo call git.exe "%%*"> git.bat
	set DEPOT_TOOLS_METRICS=0
	set DEPOT_TOOLS_REPORT_BUILD=0
	set DEPOT_TOOLS_UPDATE=0
	set DEPOT_TOOLS_WIN_TOOLCHAIN=0
	call fetch v8
	cd v8
	call git checkout tags/13.0.245.16
	call gclient sync
	call gn gen out --args="is_clang=true v8_enable_i18n_support=false v8_static_library=true is_debug=false symbol_level=0 use_custom_libcxx=false v8_monolithic=true is_component_build=false v8_use_external_startup_data=false v8_enable_fuzztest=false v8_enable_gdbjit=false v8_use_external_startup_data=false v8_enable_sandbox=true v8_enable_pointer_compression=true treat_warnings_as_errors=false"
	call ninja -C out v8_monolith
	move include ..\..\.bin2\include\v8 >nul
	move  out\obj\v8_monolith.a ..\..\.bin2\lib >nul
	cd ..\..
	rmdir /s /q v8
	move .bin2 .bin >nul
)

if "%PROCESSOR_ARCHITECTURE%" == "ARM64" ( call :check clang++ llvm-arm64 ) else ( call :check clang++ llvm )
call :check objcopy binutils

set assets=
set defs=
call llvm-rc .bin\icon.rc
md .bin\assets
for %%F in (assets\*) do (
	call objcopy -I binary -O default "%%F" ".bin\%%F.obj"
	set assets=!assets! ".bin\%%F.obj"
)
for /f %%A in ("%*") do (
	set defs=!defs! /D%%A
	echo "%%A" | findstr "=" >nul && set ARG_%%A; || set ARG_%%A=;
)
set LOPTS= /subsystem:console
if "%ARG_RELEASE%" neq "" (
	set OPTS= /O2 /GR- /Gy
	set LOPTS= /release /opt:ref /subsystem:windows /dynamicbase /nxcompat /highentropyva
)

call clang-cl -Wno-unused-command-line-argument -fuse-ld=lld -Wno-deprecated -Wno-overflow -Wno-narrowing -flto /I".bin/include" /I".bin/include/v8" -Wfatal-errors /Dmain=SDL_main!defs!!OPTS! /std:c++20 src\main.cpp .bin\src\gl.c!assets! /link!LOPTS! /LIBPATH:".bin/lib" v8_monolith.lib dbghelp.lib SDL2-static.lib SDL2main.lib advapi32.lib user32.lib gdi32.lib winmm.lib imm32.lib ole32.lib oleaut32.lib shell32.lib setupapi.lib version.lib .bin\icon.res /OUT:main.exe
set /A ERR=%errorlevel%
del /q .bin\icon.res
del /q main.lib
rmdir /s /q .bin\assets
if %ERR% neq 0 exit

if "%ARG_RELEASE%" neq "" (
	: v8 doesn't play well with upx
	: call :check upx upx
	: upx --best main.exe
	cls
) else (
	cls
	call main.exe
)


endlocal