// QYF-TM1638 Taschenrechner für ESP32 (4x4-Tastenfeld, 8-stellige Anzeige)
//
// Verkabelung TM1638-Modul -> ESP32:
//   VCC -> 5V (VU/VIN), GND -> GND, DIO -> GPIO18, CLK -> GPIO5, STB -> GPIO4
// Wichtig: QYF-Module brauchen die Klasse TM1638QYF (nicht TM1638).
//
// Tastenbelegung (S1..S16 = physische Tasten, Zuordnung siehe tasten.h):
//   Reihe 4:  S13=7   S14=8   S15=9   S16=/
//   Reihe 3:  S9 =4   S10=5   S11=6   S12=*
//   Reihe 2:  S5 =1   S6 =2   S7 =3   S8 =-
//   Reihe 1:  S1 =0   S2 =,   S3 ==   S4 =+
//
// Komma-Taste lang halten (> 0,7 s) = Alles löschen (AC). Nach einer
// Fehleranzeige ("E:DIV0") setzt jede Taste die Anzeige zuerst auf 0 zurück.
//
// Rechenweise wie ein Klassik-Taschenrechner: sofort ausführen, keine
// Punkt-vor-Strich. Eingabe <= 8 Ziffern, Ergebnisanzeige bis 8 Stellen.

#include <Arduino.h>
#include <TM1638.h>
#include <TM1638QYF.h>
#include "tasten.h"

#define TM1638_DIO_PIN 18
#define TM1638_CLK_PIN 5
#define TM1638_STB_PIN 4

const int LED_PIN = 2;   // Onboard-LED als Tasten-Quittierung

TM1638QYF display(TM1638_DIO_PIN, TM1638_CLK_PIN, TM1638_STB_PIN);

// ---- Tastenbelegung ------------------------------------------------------
enum Aktion { A_ZIFFER, A_OP, A_GLEICH, A_KOMMA };

struct Tastenbelegung {
  Aktion aktion;
  char   zeichen;   // Anzeige-/Operatorzeichen
  byte   ziffer;    // nur bei A_ZIFFER
};

const int8_t TASTE_LANG_AC = 2;   // S2 (Komma).lang drücken = Alles löschen

static const Tastenbelegung TASTE[17] = {
  { A_ZIFFER, '0', 0 },    // Index 0 unbenutzt
  { A_ZIFFER, '0', 0 },    // S1
  { A_KOMMA,  ',', 0 },    // S2
  { A_GLEICH, '=', 0 },    // S3
  { A_OP,     '+', 0 },    // S4
  { A_ZIFFER, '1', 1 },    // S5
  { A_ZIFFER, '2', 2 },    // S6
  { A_ZIFFER, '3', 3 },    // S7
  { A_OP,     '-', 0 },    // S8
  { A_ZIFFER, '4', 4 },    // S9
  { A_ZIFFER, '5', 5 },    // S10
  { A_ZIFFER, '6', 6 },    // S11
  { A_OP,     '*', 0 },    // S12
  { A_ZIFFER, '7', 7 },    // S13
  { A_ZIFFER, '8', 8 },    // S14
  { A_ZIFFER, '9', 9 },    // S15
  { A_OP,     '/', 0 },    // S16
};

// ---- Rechnerzustand ------------------------------------------------------
static float  akku          = 0;    // Ergebnis der bisherigen Kette
static char   ausstehend    = 0;    // '+', '-', '*', '/' oder 0
static bool   zweitesOperand = false; // Operator genommen, wartet auf Eingabe
static char   eingabe[12]   = {0};  // aktuelle Zahleneingabe inkl. optionalem '.'
static bool   fehler        = false;
static char   letzterOp     = 0;    // für wiederholtes "="
static float  letztesOperand = 0;
static bool   wiederhole    = false;

static const byte MAX_ZIFFERN = 8;      // gesamt (ohne Punkt)
static const byte MAX_NACHKOMMA = 6;

static byte anzahlZiffern() {
  byte n = 0;
  for (byte i = 0; i < sizeof eingabe && eingabe[i]; i++)
    if (eingabe[i] != '.') n++;
  return n;
}

static byte nachkommaStellen() {
  byte n = 0;
  bool hinter = false;
  for (byte i = 0; i < sizeof eingabe && eingabe[i]; i++) {
    if (eingabe[i] == '.') hinter = true;
    else if (hinter) n++;
  }
  return n;
}

static bool hatKomma() { return strchr(eingabe, '.') != NULL; }

static void eingabeLeeren() { eingabe[0] = 0; }

static float wertEingabe() {
  return eingabe[0] ? atof(eingabe) : 0.0f;
}

// ---- Anzeige -------------------------------------------------------------
// Text rechtsbündig in 8 Stellen anzeigen ('.' wird als Punkt-Segment gezeigt)
static void zeigeText(const char *text) {
  char feld[9];
  size_t len = strlen(text);
  if (len > 8) memcpy(feld, text + len - 8, 8);
  else { memset(feld, ' ', 8 - len); memcpy(feld + (8 - len), text, len); }
  feld[8] = 0;
  display.setDisplayToString(feld);
}

// Zahl in 8 Stellen; bei Bedarf Nachkommastellen reduzieren, sehr große oder
// kleine Zahlen in Exponentialschreibweise.
static void zeigeZahl(float zahl) {
  char text[16];

  if (zahl == 0) {
    snprintf(text, sizeof text, "0");
  } else if (fabs(zahl) >= 1e7f || fabs(zahl) < 1e-5f) {
    snprintf(text, sizeof text, "%.2E", (double)zahl);   // z.B. 1.23E+08
    for (char *p = text; *p; p++) if (*p >= 'a' && *p <= 'z') *p -= 32;
  } else {
    byte g = 6;
    for (;; g--) {
      if (g) snprintf(text, sizeof text, "%.*f", (int)g, (double)zahl);
      else   snprintf(text, sizeof text, "%.0f", (double)zahl);
      if (strlen(text) <= 8 || g == 0) break;
    }
    if (strchr(text, '.')) {               // überflüssige Nullen abräumen
      size_t l = strlen(text);
      while (l > 1 && text[l - 1] == '0') text[--l] = 0;
      if (l > 1 && text[l - 1] == '.') text[--l] = 0;
    }
  }
  zeigeText(text);
}

static void zeigeEingabe() {
  if (eingabe[0]) zeigeText(eingabe);
  else zeigeZahl(akku);
}

static void zeigeFehler() {
  fehler = true;
  display.setDisplayToString(" E:DIV0 ");   //Division durch Null / Überlauf
}

// ---- Rechnen -------------------------------------------------------------
static void fuehreAus(float b) {
  float r;
  switch (ausstehend) {
    case '+': r = akku + b; break;
    case '-': r = akku - b; break;
    case '*': r = akku * b; break;
    case '/':
      if (b == 0) { zeigeFehler(); return; }
      r = akku / b;
      break;
    default:  r = b; break;
  }
  if (!isfinite(r)) { zeigeFehler(); return; }
  akku = r;
  zweitesOperand = false;
}

// ---- Tasten --------------------------------------------------------------
static void allesLoeschen() {
  akku = 0; ausstehend = 0; zweitesOperand = false;
  wiederhole = false; letzterOp = 0; letztesOperand = 0;
  eingabeLeeren(); fehler = false;
  zeigeZahl(0);
}

static void verarbeiteTaste(int8_t nr) {
  if (fehler) {                 // nach einem Fehler: jede Taste setzt zurück
    allesLoeschen();
    if (TASTE[nr].aktion != A_ZIFFER) return;
  }

  switch (TASTE[nr].aktion) {
    case A_ZIFFER: {
      char z = (char)('0' + TASTE[nr].ziffer);
      if (zweitesOperand || (!eingabe[0] && wiederhole)) {
        eingabeLeeren();            // neue Zahl nach Ergebnis oder Operator
        zweitesOperand = false;
        wiederhole = false;
      }
      if (!eingabe[0]) { eingabe[0] = z; eingabe[1] = 0; }
      else if (strcmp(eingabe, "0") == 0) { eingabe[0] = z; }   // führende Null ersetzen
      else if (anzahlZiffern() < MAX_ZIFFERN) {
        size_t l = strlen(eingabe);
        if (hatKomma() && nachkommaStellen() >= MAX_NACHKOMMA) break;
        if (l < sizeof eingabe - 1) { eingabe[l] = z; eingabe[l + 1] = 0; }
      }
      zeigeEingabe();
      break;
    }

    case A_KOMMA:
      if (zweitesOperand) { eingabeLeeren(); zweitesOperand = false; }
      if (wiederhole && !eingabe[0]) wiederhole = false;
      if (!hatKomma()) {
        if (!eingabe[0]) { eingabe[0] = '0'; eingabe[1] = '.'; eingabe[2] = 0; }
        else { size_t l = strlen(eingabe); eingabe[l] = '.'; eingabe[l + 1] = 0; }
      }
      zeigeEingabe();
      break;

    case A_OP:
      if (zweitesOperand) {              // Operator korrigieren: '+' statt '-'
        eingabeLeeren();
      } else {
        if (ausstehend && eingabe[0]) { fuehreAus(wertEingabe()); if (fehler) return; }
        else if (eingabe[0]) akku = wertEingabe();   // sonst Ergebnis beibehalten
        eingabeLeeren();
      }
      wiederhole = false;
      ausstehend = TASTE[nr].zeichen;
      zweitesOperand = true;
      zeigeZahl(akku);
      break;

    case A_GLEICH:
      if (ausstehend) {
        // "12 + 3 =" -> 15, danach wiederholt "=" aufsteigend (18, 21, ...)
        float zweiter = zweitesOperand ? akku : wertEingabe();
        letzterOp = ausstehend;
        letztesOperand = zweiter;
        fuehreAus(zweiter);
        if (fehler) return;
        wiederhole = true;
        ausstehend = 0;
      } else if (wiederhole) {
        char gemerkt = letzterOp;
        ausstehend = gemerkt;
        fuehreAus(letztesOperand);
        if (fehler) return;
        ausstehend = 0;
      } else {
        akku = wertEingabe();
        wiederhole = false; letzterOp = 0;    // neue Ausgangszahl, keine Wiederholung
      }
      eingabeLeeren();
      zweitesOperand = false;
      zeigeZahl(akku);
      break;
  }
}

// ---- Main ----------------------------------------------------------------
static word letzterStatus = 0;
static bool aktionGelaufen = false;
static uint32_t zeitWechsel = 0;
static uint32_t ledAnSeit = 0;
static uint32_t druckStart = 0;      // Beginn des Tastendrucks (für Lang-Erkennung)
static int8_t   rueckfrageTaste = 0; // Taste, deren Aktion erst beim Loslassen feststeht
static const uint32_t ENTKOPPEL_MS = 90;
static const uint32_t LANG_MS = 700;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  display.setupDisplay(true, 4);
  display.clearDisplay();
  allesLoeschen();
  Serial.println("QYF-TM1638 Taschenrechner startet...");
  Serial.println("Reihen (oben nach unten): 7 8 9 / | 4 5 6 * | 1 2 3 - | 0 , = +");
  Serial.println("Komma-Taste lang halten = Alles loeschen (AC).");
  zeigeZahl(0);
}

// Die Komma-Taste hat eine Doppelfunktion: kurz = Dezimalpunkt,
// lang (> LANG_MS) = Alles löschen.
void loop() {
  word tasten = display.getButtons();
  uint32_t jetzt = millis();

  if (tasten != letzterStatus) {
    if (tasten == 0 && rueckfrageTaste) {       // Release der Komma-Taste
      if (jetzt - druckStart < LANG_MS) {
        Serial.printf("Taste S%d ','\n", rueckfrageTaste);
        verarbeiteTaste(rueckfrageTaste);
      }
      rueckfrageTaste = 0;
    }
    letzterStatus = tasten;
    zeitWechsel = jetzt;
    if (!tasten) aktionGelaufen = false;
  }

  if (tasten && !aktionGelaufen && (jetzt - zeitWechsel) >= ENTKOPPEL_MS) {
    int8_t nr = tasteFine(tasten);
    druckStart = zeitWechsel;
    if (nr > 0) {
      aktionGelaufen = true;
      digitalWrite(LED_PIN, HIGH);
      ledAnSeit = jetzt;
      if (nr == TASTE_LANG_AC) {
        rueckfrageTaste = nr;                    // Entscheidung bei Release
      } else {
        Serial.printf("Taste S%d '%c'\n", nr, TASTE[nr].zeichen);
        verarbeiteTaste(nr);
      }
    } else if (nr < 0) {
      display.setDisplayToString("  Mehr    ");
      aktionGelaufen = true;
    }
  }

  // Lang gedrückt halten -> AC, ohne auf das Loslassen zu warten
  if (rueckfrageTaste && (jetzt - druckStart) >= LANG_MS) {
    rueckfrageTaste = 0;
    Serial.println("Taste S2 lang -> Alles loeschen");
    allesLoeschen();
  }

  if (digitalRead(LED_PIN) && (jetzt - ledAnSeit) >= 120) digitalWrite(LED_PIN, LOW);
  delay(20);
}
