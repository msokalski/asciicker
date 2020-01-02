
#include "platform.h"

// aggregate ALL input kinds:

// A3D window: keyboard, character, mouse, gamepad
// terminal esc codes: character, mouse, +kitty's keyboard codes
// web (callbacks): character, keyboard, mouse, touch, gamepad

// dispatch input to screen stacking 
// it will determine screen layer by position and by focus (for keyboard)
// then it will modify focus (if needed) and will dispatch to desired layer

