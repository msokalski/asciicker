if [ -f "/usr/bin/time" ]; then
    /usr/bin/time -f "----------------------\ndone in %e sec" make -j16 -f makefile_asciiid
    /usr/bin/time -f "----------------------\ndone in %e sec" make -j16 -f makefile_server
    /usr/bin/time -f "----------------------\ndone in %e sec" make -j16 -f makefile_game
else
    make -j16 -f makefile_asciiid
    make -j16 -f makefile_server
    make -j16 -f makefile_game
fi

