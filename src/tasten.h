// Tasten-Mapping für 4x4-Tastenfeld am TM1638 (QYF-Modul).
//
// Das Modul liefert beim 4x4-Scan 16 Bit (word). Welches Bit zu welcher
// physischen Taste gehört, ist modulabhängig -> einmalig mit der Debug-Ausgabe
// (Serial: "Roh: 0x....") kalibrieren:
//   Jede Taste nacheinander drücken und das gesetzte Bit notieren.
//   bitNr = Position in diesem Array, deren Wert = Bitindex (0..15) ist.
//
// Die Anordnung hier entspricht dem gängigen QYF-Schema
// (Bit = zeile*4 + spalte, untere Reihe = Bit 0):
//
//     [13][14][15][16]   <- Reihe 4
//     [ 9][10][11][12]   <- Reihe 3
//     [ 5][ 6][ 7][ 8]   <- Reihe 2
//     [ 1][ 2][ 3][ 4]   <- Reihe 1
//
#ifndef TASTEN_H
#define TASTEN_H

#include <Arduino.h>

// Index = Taste-1, Wert = Bitposition im 16-Bit-Wort von getButtons()
static const byte TASTEN_BIT[16] = {
  0, 1, 2, 3,        // Reihe 1
  4, 5, 6, 7,        // Reihe 2
  8, 9, 10, 11,      // Reihe 3
  12, 13, 14, 15     // Reihe 4
};

// Liefert die Nummer (1..16) der zuerst gefundenen gedrückten Taste,
// 0 wenn keine, -1 wenn mehrere gleichzeitig (=> "Mehrere").
static inline int8_t tasteFine(word mask) {
  int8_t fund = 0;
  for (int i = 0; i < 16; i++) {
    if (mask & (1 << TASTEN_BIT[i])) {
      if (fund) return -1;   // mehr als eine Taste
      fund = i + 1;
    }
  }
  return fund;
}

#endif
