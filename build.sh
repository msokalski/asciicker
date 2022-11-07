
# sudo apt install libgpm-dev
# sudo apt install libx11-dev
# sudo apt install libxinerama-dev
# sudo apt install libgl1-mesa-dev

# STARTING FROM Y9
# asciicker makes use of a js interpreter
# while web builds exploit the browser's interpreter,
# you'll need to build libv8_monolith.a static lib from sources for desktop builds
# assuming we're on linux and we have things like git, gcc/clang, ninja etc installed

# recipe based on:
# https://chromium.googlesource.com/external/github.com/v8/v8.wiki/+/8c0be5e888bda68437f15e2ea9e317fd6229a5e3/Building-with-GN.md
# - Build instructions (raw workflow)

# on LINUX:
# we need depot_tools so:
# > cd ~
# > git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
# > export PATH=~/depot_tools:$PATH

# obtain v8 sources:
# > cd <asciicker_path>/v8
# > fetch v8                                # fetch and grab a cup of coffie
# > git pull                                # pull for a total freshness of sources
# > gclient sync                            # fetch build dependencies

# configure and build:
# > cd <asciicker_path>/v8/v8
# > gn gen out.gn/x64.release --args='
# 	 treat_warnings_as_errors=false
#	 use_custom_libcxx=false
#    use_custom_libcxx_for_host=false
#    use_glib=false
#	 use_sysroot=false
#    v8_enable_i18n_support=false
#	 v8_use_external_startup_data=false 
#	 v8_monolithic=true 
#    v8_static_library=true
#    v8_enable_webassembly=false
#	 is_debug=false
#    enable_iterator_debugging=false
#    host_cpu="x64"
#	 target_cpu="x64" 
#	 v8_target_cpu="x64"'
# > ninja -C out.gn/x64.release v8_monolith

# on WINDOWS:
# install MSVC2022
# install Win10 SDK 10.0.20348.0/1, important: it have to include dubug tools !!!
# https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/
# click modify, choose change, next, enable debugging tools for windows, click change
# get depot_tools from https://storage.googleapis.com/chrome-infra/depot_tools.zip
# unpack it to d:\depot_tools and add it to PATH envvar
# also add this envvar to user variables:
#   DEPOT_TOOLS_WIN_TOOLCHAIN=0
# > cd <asciicker_path>/v8
# > fetch v8
# > git pull
# > gclient sync
# GENERATE EMPTY BUILD CONFIG:
# > python d:\depot_tools\gn.py gen out.gn/x64.release
# CONFIGURE BUILD:
# overwrite out.gn/x64.release/args.gn with <asciicker_path>/v8/x64.release-args.gn
# you can see all set params including defaulted by:
# > python d:\depot_tools\gn.py args out.gn/x64.release --list > ../list.txt
# BUILD:
# > ninja -C out.gn/x64.release v8_monolith
#
# repeat above three steps also with x64.debug, x86.release and x86.debug
# now you can batch build all asciicker configurations

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	if [ -f "/usr/bin/time" ]; then
		echo -e "BUILDING asciiid\n----------------------"
		/usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f makefile_asciiid
		echo -e "BUILDING server\n----------------------"
		/usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f makefile_server
		echo -e "BUILDING game\n----------------------"
		/usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f makefile_game
		echo -e "BUILDING game_term\n----------------------"
		/usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f makefile_game_term
	else
		make -j16 -f makefile_asciiid
		make -j16 -f makefile_server
		make -j16 -f makefile_game
		make -j16 -f makefile_game_term
	fi
else
	make -j16 -f makefile_asciiid_mac
	make -j16 -f makefile_server
	make -j16 -f makefile_game_mac
	make -j16 -f makefile_game_term_mac
fi
