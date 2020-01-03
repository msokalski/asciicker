
#include "platform.h"

// 1. aggregate ALL input kinds:
// - A3D window (term.cpp): keyboard, character, mouse, gamepad
// - terminal esc codes (xterm.cpp): character, mouse, +kitty's keyboard codes
// - web callbacks (web.cpp): character, keyboard, mouse, touch, gamepad

// 2. dispatch input to screen stacking 

