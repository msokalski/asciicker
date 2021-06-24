
sudo apt install libgpm-dev
sudo apt install libx11-dev
sudo apt install libxinerama-dev
sudo apt install libgl1-mesa-dev

if [ -f "/usr/bin/time" ]; then
    echo -e "BUILDING asciiid\n----------------------"
    /usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f ../MakeFiles/makefile_asciiid
    echo -e "BUILDING server\n----------------------"
    /usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f ../MakeFiles/makefile_server
    echo -e "BUILDING game\n----------------------"
    /usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f ../MakeFiles/makefile_game
    echo -e "BUILDING game_term\n----------------------"
    /usr/bin/time -f "----------------------\ndone in %e sec\n" make -j16 -f ../MakeFiles/makefile_game_term
else
    make -j16 -f ../MakeFiles/makefile_asciiid
    make -j16 -f ../MakeFiles/makefile_server
    make -j16 -f ../MakeFiles/makefile_game
    make -j16 -f ../MakeFiles/makefile_game_term
fi

