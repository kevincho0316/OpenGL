#pragma once

enum InputType {
  // Touch or Mouse
  PointerDown,
  PointerUp,
  PointerMove,
  // Keyboard
  KeyDown,
  KeyUp,
};

struct inputEvent {
  InputType inputType;
  int pointID; // mouse will get 0
  float x, y;
  char key;
};
