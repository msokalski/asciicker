
#install fonts dir
xset fp+ /home/user/asciiid/fonts
xset fp rehash

#run
xterm -e .run/game_term \
    -xrm "xterm*font1: -gumix-*-*-*-*-*-8-*-*-*-*-*-*" \
    -xrm "xterm*font2: -gumix-*-*-*-*-*-10-*-*-*-*-*-*" \
    -xrm "xterm*font3: -gumix-*-*-*-*-*-12-*-*-*-*-*-*" \
    -xrm "xterm*font:  -gumix-*-*-*-*-*-14-*-*-*-*-*-*" \
    -xrm "xterm*font4: -gumix-*-*-*-*-*-16-*-*-*-*-*-*" \
    -xrm "xterm*font5: -gumix-*-*-*-*-*-18-*-*-*-*-*-*" \
    -xrm "xterm*font6: -gumix-*-*-*-*-*-20-*-*-*-*-*-*"
