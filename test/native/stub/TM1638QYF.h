// Stub: zeichnet auf, was aufs Display geschrieben wird.
#pragma once
#include <Arduino.h>

class TM1638QYF {
public:
  static inline char letzte[16] = {0};
  static inline word tasten = 0;
  TM1638QYF(byte = 0, byte = 0, byte = 0, bool = true, byte = 7) {}
  void setupDisplay(bool, byte) {}
  void clearDisplay() { letzte[0] = 0; }
  word getButtons() { return tasten; }
  void setDisplayToString(String s, word dots = 0, byte pos = 0, const byte *font = nullptr) {
    (void)dots; (void)pos; (void)font;
    size_t n = s.length() > 15 ? 15 : s.length();
    memcpy(letzte, s.c_str(), n); letzte[n] = 0;
    int i = 0; while (letzte[i] == ' ') i++;          // führende Leerzeichen ab
    if (i) memmove(letzte, letzte + i, strlen(letzte + i) + 1);
  }
};
