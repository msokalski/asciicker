
#install fonts dir
xset fp+ $PWD/fonts
xset fp rehash

#run
xterm \
    -xrm "xterm*font1: -gumix-*-*-*-*-*-8-*-*-*-*-*-*" \
    -xrm "xterm*font2: -gumix-*-*-*-*-*-10-*-*-*-*-*-*" \
    -xrm "xterm*font3: -gumix-*-*-*-*-*-12-*-*-*-*-*-*" \
    -xrm "xterm*font:  -gumix-*-*-*-*-*-14-*-*-*-*-*-*" \
    -xrm "xterm*font4: -gumix-*-*-*-*-*-16-*-*-*-*-*-*" \
    -xrm "xterm*font5: -gumix-*-*-*-*-*-18-*-*-*-*-*-*" \
    -xrm "xterm*font6: -gumix-*-*-*-*-*-20-*-*-*-*-*-*" \
    -e .run/asciicker_term 

#cleanup
xset fp- $PWD/fonts
xset fp rehash    
