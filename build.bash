#!/usr/bin/bash
if [ ! -d .bin ]; then
	rm -rf .bin2
	mkdir .bin2
	git clone https://github.com/BlobTheKat/glad-everywhere --depth=1
	mv glad-everywhere/include .bin2
	mv glad-everywhere/src .bin2
	rm -rf glad-everywhere
	git clone -b SDL2 https://github.com/libsdl-org/SDL --depth=1
	sudo apt-get install libasound2-dev libpulse-dev cmake upx
	cmake -S SDL -B SDL/build -DCMAKE_BUILD_TYPE=Release -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_DYNAMIC_API=OFF -DSDL_RENDER_DISABLED=ON -DSDL_RENDER_D3D=OFF -DSDL_RENDER_METAL=OFF -DSDL_VULKAN=OFF -DSDL_TEST=OFF && cmake --build SDL/build --config Release --parallel || exit
	mv SDL/include .bin2/include/SDL2
	mkdir .bin2/lib
	mv SDL/build/*.a .bin2/lib
	rm -rf SDL

	git clone https://github.com/nothings/stb
	mkdir .bin2/include/stb
	mv stb/*.h .bin2/include/stb
	rm -rf stb

	mv .bin2 .bin
fi

# Build
clear
DEFS=
for arg in "$@"; do
DEFS="$DEFS -D$arg"
declare "ARG_$arg$(expr "$arg" : ".*=.*" >/dev/null || echo "=")"
done
postbuild=./main
OPTS=" -fsanitize=undefined -O0 -fno-omit-frame-pointer -fno-inline -rdynamic -g3"
if [ "${ARG_RELEASE+x}" ]; then
if [[ "$OSTYPE" == "darwin"* ]]; then
OPTS=" -fno-rtti -O3 -Wl,-x -ffunction-sections -fdata-sections -Wl,-dead_strip -g0 -flto"
else
OPTS=" -fno-rtti -O3 -Wl,--strip-all -ffunction-sections -fdata-sections -Wl,--gc-sections -g0 -flto"
postbuild="upx --best main"
fi
fi

embed=" -Wl,--format=binary -Wl,$(find assets/*) -Wl,--format=default"
if [[ "$OSTYPE" == "darwin"* ]]; then
echo ".section __DATA,__bin_assets" > .bin/assets.s
OPTS="$OPTS -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework CoreGraphics -framework CoreHaptics -framework Cocoa -framework CoreVideo -framework IOKit -framework OpenGL -framework ForceFeedback -framework GameController -framework Carbon -lobjc"
for name in assets/*; do
n2="_binary_$(echo $name | tr '/.' '_')"
echo ".globl ${n2}_start\n${n2}_start:\n.incbin \"$(echo $name | tr '\\' '\\\\' | tr '"' '\\"')\"\n.globl ${n2}_end\n${n2}_end:" >> .bin/assets.s
export MACOSX_DEPLOYMENT_TARGET=14.6
done
fi

clang++ -Wno-unqualified-std-cast-call -Wno-deprecated -I.bin/include -Wfatal-errors -std=c++20 -Wno-overflow -fwrapv -Wno-narrowing$OPTS$DEFS src/main.cpp .bin/src/glad.c .bin/assets.s -L.bin/lib -lSDL2 -lSDL2main -ldl -pthread -o main && $postbuild
if [[ "$OSTYPE" == "darwin"* ]]; then
rm -f .bin/assets.s
rm -rf main.dSYM; fi