#ifndef FRICTION_H_INFINITE_MOUSE_MOVEMENT
#define FRICTION_H_INFINITE_MOUSE_MOVEMENT

/// Allows the mouse to move across the border of the screen and return back to the other side. Is useful for e.g. scaling / rotating an object in the canvas.
void setCursorMovementInfinite();

/// Blocks the mouse from going across the border of the screen
void setCursorMovementDefault();

/// Allows the mouse only to move 1px and returns it to original state
/// (like in a videogame). Could be useful for e.g. blender-style combo boxes.
/// Should additionally be hidden or change the cursor style,
/// and make this function return values depending on how much the mouse moves.
void setCursorMovementLock();

#endif
