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

	call git clone https://github.com/richgel999/stb
	mkdir .bin2/include/stb 2>nul
	move stb/*.h .bin2/include/stb >nul
	rmdir /s /q stb

	echo id ICON icon.ico> .bin2\icon.rc
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

set R=%*
goto :loop

:strLen
setlocal disableDelayedExpansion
set len=0
if defined %~1 for /f "delims=:" %%N in (
  '"(cmd /v:on /c echo(!%~1!&echo()|findstr /o ^^"'
) do set /a "len=%%N-3"
endlocal & if "%~2" neq "" (set %~2=%len%) else echo %len%
exit /b

:loop
for /f %%A in ("!R!") do (
	set defs=!defs! /D%%A
	echo "%%A" | findstr "=" >nul && set ARG_%%A; || set ARG_%%A=;
	set A=1%%A
	call :strLen A len
	set R=!R:~%len%!
)
if "!R!" neq "" goto :loop

set OPTS= -gcolumn-info /Od /Ob0 /EHsc /GR -gcodeview /Z7 -fsanitize=undefined -fsanitize=address
set LOPTS= /subsystem:console /DEBUG /DEBUG:FULL
if "%ARG_RELEASE%" neq "" (
	set OPTS= /GR- /Gy /O2 -Ofast
	set LOPTS= /release /opt:ref /opt:icf /subsystem:windows /dynamicbase /nxcompat /highentropyva
)
if "%ARG_PROFILE%" neq "" (
	set OPTS= -gcolumn-info /EHsc /GR -gcodeview /Z7 /O2 -Ofast
	set LOPTS= /release /subsystem:console
)

call clang-cl -Wno-unqualified-std-cast-call -Wno-unused-command-line-argument -fuse-ld=lld -Wno-deprecated -Wno-overflow -Wno-narrowing -flto /I".bin/include" -Wfatal-errors /Dmain=SDL_main!defs!!OPTS! /std:c++20 src\main.cpp .bin\src\glad.c!assets! /link!LOPTS! /LIBPATH:".bin/lib" SDL2-static.lib SDL2main.lib advapi32.lib user32.lib gdi32.lib winmm.lib imm32.lib ole32.lib oleaut32.lib shell32.lib setupapi.lib version.lib .bin\icon.res /OUT:main.exe
set /A ERR=%errorlevel%
del /q .bin\icon.res
del /q main.lib
rmdir /s /q .bin\assets
if %ERR% neq 0 exit

if "%ARG_RELEASE%" neq "" (
	call :check upx upx
	upx --best main.exe
	cls
) else (
	cls
	call main.exe
)


endlocal