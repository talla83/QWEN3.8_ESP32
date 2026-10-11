// Nativer Logiktest für src/main.cpp.
// Build (nutzt das über pip installierte zig als C++-Compiler):
//   python -m ziglang c++ -std=c++17 -O0 -Dloop=firmwareLoop \
//     -I test/native/stub test/native/test_native.cpp -o test/native/test_native.exe
// Ausführung: test\native\test_native.exe   (Exit-Code 1 bei Fehlern)
#include <cstdio>
#include <cstring>
#include <cstdint>

#include "stub/Arduino.h"

uint32_t stubMillis = 0;
StubSerial Serial;

#define loop firmwareLoop
#include "../../src/main.cpp"
#undef loop

static int fehlgeschlagen = 0;
static int tests = 0;

static void ruck(uint32_t ms) {           // ms vergehen lassen, loop() laufen lassen
  for (uint32_t t = 0; t < ms; t += 20) { stubMillis += 20; firmwareLoop(); }
}

static const char *rand(const char *s, char *puffer) {
  while (*s == ' ') s++;
  strcpy(puffer, s);
  size_t n = strlen(puffer);
  while (n && puffer[n - 1] == ' ') puffer[--n] = 0;
  return puffer;
}

static void erwarte(const char *was, const char *ist) {
  char puffer[24];
  tests++;
  const char *clean = rand(ist, puffer);
  if (strcmp(was, clean) != 0) {
    printf("FEHLER: erwartet '%s', Anzeige '%s'\n", was, clean);
    fehlgeschlagen++;
  }
}

// eine Taste drücken (halteMs) und loslassen
static void taste(int n, uint32_t halteMs = 240) {
  TM1638QYF::tasten = (word)(1u << (n - 1));
  ruck(halteMs);
  TM1638QYF::tasten = 0;
  ruck(40);
}

static void neu() {
  stubMillis = 0;
  TM1638QYF::tasten = 0;
  TM1638QYF::letzte[0] = 0;
  setup();
}

// "5 6 4 7 9 3" -> nacheinander S5, S6, S4, S7, S9, S3
static void folge(const char *liste) {
  const char *p = liste;
  while (*p) {
    int wert = 0;
    while (*p >= '0' && *p <= '9') { wert = wert * 10 + (*p - '0'); p++; }
    if (wert) taste(wert);
    while (*p && *p != ' ') p++;
    if (*p == ' ') p++;
  }
}

// Zifferntasten: 0=1 1=5 2=6 3=7 4=9 5=10 6=11 7=13 8=14 9=15
// Operatoren: + =4  - =8  * =12  / =16 ; Komma =2 ; Gleich =3
static void fall(const char *name, const char *tasten, const char *erwartung) {
  neu();
  folge(tasten);
  printf("%-30s", name);
  erwarte(erwartung, TM1638QYF::letzte);
}

int main() {
  neu();
  printf("%-30s", "Startanzeige");
  erwarte("0", TM1638QYF::letzte);

  fall("12+34=",            "5 6 4 7 9 3",        "46");
  fall("80-70=",            "14 1 8 13 1 3",      "10");
  fall("27*4=",             "6 13 12 9 3",        "108");
  fall("50/4=",             "10 1 16 9 3",        "12.5");
  fall("100/8=",            "5 1 1 16 14 3",      "12.5");
  fall("2/3=",              "6 16 7 3",           "0.666667");
  fall("5.2=",              "10 2 6 3",           "5.2");
  fall("0.3+1=",            "1 2 7 4 5 3",        "1.3");
  fall("0, dann 1, dann 1", "1 5 5",              "11");
  fall("12- dann berichtigt+ 34=", "5 6 8 4 7 9 3", "46");
  fall("9/0=",              "15 16 1 3",          "E:DIV0");
  fall("12+34-5=",          "5 6 4 7 9 8 10 3",   "41");
  fall("12+12=",            "5 6 4 5 6 3",        "24");
  fall("9 Ziffern",         "10 10 10 10 10 10 10 10 10", "55555555");
  fall("100000*100 (Exp)",  "5 1 1 1 1 1 12 5 1 1 3", "1.00E+07");
  fall("Operator nach Ergebnis", "5 6 4 9 3 4",   "16");

  // Nach = eine Zahl tippen -> neue Rechnung (46 anzeigen, dann 4 = ...)
  neu(); folge("5 6 4 7 9 3");            // 12+34= -> 46
  folge("5");                             // '1' tippen
  printf("%-30s", "nach Ergebnis 1 tippen");
  erwarte("1", TM1638QYF::letzte);

  // "=" zweimal: 12 + 12 = = -> 24 + 12 = 36
  neu(); folge("5 6 4 5 6 3 3");
  printf("%-30s", "12+12= = (36)");
  erwarte("36", TM1638QYF::letzte);

  // AC durch langes Drücken der Komma-Taste
  neu(); folge("5 6 4 7 9 3"); taste(2, 900);
  printf("%-30s", "AC (Komma lang)");
  erwarte("0", TM1638QYF::letzte);

  // Komma kurz bleibt Komma
  neu(); folge("5 2 6");
  printf("%-30s", "Komma kurz (1.2)");
  erwarte("1.2", TM1638QYF::letzte);

  printf("\n%d Prüfungen, %d Fehler\n", tests, fehlgeschlagen);
  return fehlgeschlagen != 0;
}
