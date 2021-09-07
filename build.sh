
# sudo apt install libgpm-dev
# sudo apt install libx11-dev
# sudo apt install libxinerama-dev
# sudo apt install libgl1-mesa-dev


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
