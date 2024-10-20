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
	set -e
	mkdir v8; cd v8
	git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git --depth=1
	export PATH="$PATH:$PWD/depot_tools"
	# echo 'call git "%*"' > depot_tools/git.bat
	export DEPOT_TOOLS_METRICS=0
	export DEPOT_TOOLS_REPORT_BUILD=0
	export DEPOT_TOOLS_UPDATE=0
	export DEPOT_TOOLS_WIN_TOOLCHAIN=0
	fetch v8; cd v8;
	git checkout tags/13.0.245.16
	gclient sync
	gn gen out --args="is_clang=true v8_enable_i18n_support=false v8_static_library=true is_debug=false symbol_level=0 use_custom_libcxx=false v8_monolithic=true is_component_build=false v8_use_external_startup_data=false v8_enable_fuzztest=false v8_enable_gdbjit=false v8_use_external_startup_data=false v8_enable_sandbox=true v8_enable_pointer_compression=true treat_warnings_as_errors=false"
	ninja -C out v8_monolith
	strip out/obj/v8_monolith.a
	mv include ../../.bin2/include/v8
	mv out/obj/v8_monolith.a ../../.bin2/lib
	cd ../..
	rm -rf v8
	set +e
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
if [ "${ARG_RELEASE+x}" ]; then
OPTS=" -O3"
#postbuild="upx --best main"
fi

clang++ -Wno-deprecated -fno-rtti -flto -I.bin/include -Wfatal-errors -std=c++20 -Wno-overflow -fwrapv -Wno-narrowing$OPTS$DEFS src/main.cpp .bin/src/gl.c -Wl,--strip-all -ffunction-sections -L.bin/lib -lSDL2 -lSDL2main -ldl -pthread -Wl,--gc-sections -Wl,--format=binary -Wl,$(find assets/*) -Wl,--format=default -o main && $postbuild