#!/bin/bash
# Install required libraries:
#sudo apt install libgpm-dev
#sudo apt install libx11-dev
#sudo apt install libxinerama-dev
#sudo apt install libgl1-mesa-dev

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	if [ -f "/usr/bin/time" ]; then
		echo -e "BUILDING asciiid\n----------------------"
		/usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f Makefiles/makefile_asciiid
		echo -e "BUILDING server\n----------------------"
		/usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f Makefiles/makefile_asciicker_server
		echo -e "BUILDING game\n----------------------"
		/usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f Makefiles/makefile_asciicker
		echo -e "BUILDING game_term\n----------------------"
		/usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f Makefiles/makefile_asciicker_term
	else
		make -j16 -f Makefiles/makefile_asciiid
		make -j16 -f Makefiles/makefile_server
		make -j16 -f Makefiles/makefile_asciicker
		make -j16 -f Makefiles/makefile_asciicker_term
	fi
else
	make -j16 -f Makefiles/makefile_asciiid_mac
	make -j16 -f Makefiles/makefile_server
	make -j16 -f Makefiles/makefile_asciicker_mac
	make -j16 -f Makefiles/makefile_asciicker_term_mac
fi
