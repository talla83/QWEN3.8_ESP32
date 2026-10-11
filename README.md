# 🧮 ESP32-Taschenrechner – ein Spaß-Projekt mit KI

[![YouTube Video](https://img.youtube.com/vi/4kD0AB6MCVo/hq720.jpg)](https://www.youtube.com/watch?v=4kD0AB6MCVo)

> ▶️ **Das Video zum Projekt:** [ESP32-Taschenrechner mit KI entwickelt](https://www.youtube.com/watch?v=4kD0AB6MCVo)

Dies ist ein **reines Spaß- und Lernprojekt**. Die Idee: Lässt sich mit einer
Künstlichen Intelligenz (hier **Qwen 3.8**) komplett funktionsfähiger
**ESP32-Code** für einen echten Taschenrechner erzeugen – von der Tastenabfrage
über die Rechenlogik bis zur Anzeige?

Das Ergebnis: Ein klassischer Taschenrechner auf Basis eines ESP32 und eines
günstigen **TM1638-Moduls** mit 4×4-Tastenfeld und 8-stelliger Sieben-Segment-Anzeige.
Der gesamte Code wurde gemeinsam mit der KI geschrieben, getestet und korrigiert.

---

## ✨ Funktionen

- **4 Grundrechenarten** (`+` `-` `*` `/`) – Rechenweise wie ein Klassik-Taschenrechner
  (sofort ausführen, keine Punkt-vor-Strich-Regel).
- **8-stellige Anzeige** mit bis zu 6 Nachkommastellen.
- **Komma-Eingabe** über die `,`-Taste.
- **`=`-Taste**: wiederholtes Drücken wiederholt den letzten Rechenschritt.
- **Alles löschen (AC)**: die Komma-Taste lang halten (> 0,7 s).
- **Fehleranzeige** (z. B. `E:DIV0` bei Division durch 0) – jede Taste setzt zurück auf `0`.
- **Onboard-LED** (GPIO2) quittiert jeden Tastendruck.

---

## 🔧 Hardware

| Bauteil | Beschreibung |
|---------|--------------|
| ESP32 DevKit (`esp32dev`) | Mikrocontroller |
| QYF-TM1638-Modul | 4×4-Tastenfeld + 8-stellige LED-Anzeige + LEDs |
| USB / 5 V | Stromversorgung über `VU`/`VIN` |

### Verkabelung TM1638-Modul → ESP32

| TM1638 | ESP32 |
|--------|-------|
| `VCC`  | `5V` (VU/VIN) |
| `GND`  | `GND` |
| `DIO`  | `GPIO18` |
| `CLK`  | `GPIO5` |
| `STB`  | `GPIO4` |

> ⚠️ **Wichtig:** QYF-Module brauchen die Klasse `TM1638QYF` (nicht `TM1638`).

### Tastenbelegung

```
Reihe 4:  7   8   9   /
Reihe 3:  4   5   6   *
Reihe 2:  1   2   3   -
Reihe 1:  0   ,   =   +
```

---

## 💻 Software & Build

Entwickelt mit **PlatformIO** (`platform-espressif32`, Arduino-Framework).

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    th0m4sk/TM1638-extended
```

Bauen und hochladen:

```bash
pio run            # kompilieren
pio run -t upload  # auf den ESP32 flashen
pio device monitor # Serial-Konsole (115200 baud)
```

### Projektstruktur

```
src/
  main.cpp    # Rechenlogik, Anzeige & Tastenverarbeitung
  tasten.h    # Mapping der 4×4-Tasten auf Bitpositionen
test/
  native/     # Unit-Tests (mit Arduino-Stubs)
```

> Die Fein-Kalibrierung der Tasten erfolgt über die Serial-Ausgabe
> `Roh: 0x....` – jede Taste nacheinander drücken und das gesetzte Bit notieren
> (siehe Kommentare in `tasten.h`).

---

## 🤖 Wie die KI geholfen hat

Der Code ist nicht von Grund auf selbst getippt, sondern im Dialog mit
**Qwen 3.8** entstanden: Prompt → Code-Entwurf → Flashen → Testen → Fehler
(z. B. Vertipp-Routen, Anzeigepositionen, Komma-Handling) → Prompt-Anpassung.
Ein schönes Beispiel dafür, wie gut (und wo weniger gut) KI heute
Embedded-Code für Einsteiger-Projekte liefern kann.

---

## 📄 Lizenz & Hinweis

Dieses Projekt ist ein Hobby- und Demonstrationsprojekt ohne Gewähr.
Viel Spaß beim Nachbauen – und schau dir gern das [Video](https://www.youtube.com/watch?v=4kD0AB6MCVo) dazu an! 🎬
