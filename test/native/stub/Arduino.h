// Minimal-Stubs, damit sich src/main.cpp nativ für Logiktests bauen lässt.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

typedef uint8_t byte;
typedef uint16_t word;
typedef std::string String;

extern uint32_t stubMillis;

inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int  digitalRead(int) { return 0; }
inline uint32_t millis() { return stubMillis; }
inline void delay(int) {}

#define OUTPUT 1
#define HIGH 1
#define LOW 0

// Serial-Ausgaben werden für die Anzeige nicht benötigt.
struct StubSerial {
  template <typename... Args> void begin(Args...) {}
  template <typename... Args> void println(Args...) {}
  template <typename... Args> void print(Args...) {}
  // main.cpp ruft Serial.printf(...) auf -> ins Leere schreiben
  template <typename... Args> int printf(Args... a) { (void)sizeof...(a); return 0; }
};
extern StubSerial Serial;
