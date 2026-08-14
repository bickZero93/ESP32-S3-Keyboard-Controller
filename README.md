# ESP32-S3 USB HID Keyboard Controller

> Ein vollständiger WLAN-gesteuerter USB-Tastatur-Controller auf dem ESP32-S3-Zero.  
> Steuere deinen PC drahtlos über ein modernes Webinterface – von jedem Gerät im Netzwerk.

---

## ✨ Features im Überblick

| Feature | Beschreibung |
|---|---|
| ⚡ **Sequenz-Builder** | Beliebige Kombinationen aus Text, Tasten & Delays |
| ⌨️ **Vollständiges DE-Layout** | Alle Sonderzeichen, Umlaute, Symbole korrekt |
| 📋 **Copy+Paste Modus** | Blitzschnelles Einfügen via PowerShell & Ctrl+V |
| 📁 **Vorlagen** | Sequenzen speichern & geräteübergreifend abrufen |
| ⏱ **Autostart** | Vorlage automatisch beim Boot ausführen |
| 🔧 **OTA-Updates** | Firmware direkt über den Browser flashen |
| 📡 **WLAN + Hotspot** | Heimnetz oder eigener Fallback-Hotspot |
| 💾 **Persistent** | Alle Einstellungen bleiben nach Neustart erhalten |

---

## 🛒 Hardware

| Komponente | Empfehlung |
|---|---|
| Mikrocontroller | **Waveshare ESP32-S3-Zero** |
| Anschluss | USB-C (fungiert gleichzeitig als HID-Tastatur) |
| Stromversorgung | Per USB vom Ziel-PC oder separatem Netzteil |

---

## 🚀 Installation

### Voraussetzungen

- [Arduino IDE 2.x](https://www.arduino.cc/en/software)
- ESP32-Boardpaket von Espressif

### Boardpaket installieren

1. Arduino IDE öffnen → **File → Preferences**
2. Bei *Additional boards manager URLs* eintragen:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **Tools → Board → Boards Manager** → `esp32` suchen → **esp32 by Espressif Systems** installieren

### Board-Einstellungen (wichtig!)

| Einstellung | Wert |
|---|---|
| Board | `ESP32S3 Dev Module` |
| USB Mode | **USB-OTG (TinyUSB)** |
| USB CDC On Boot | **Enabled** |
| Partition Scheme | **Minimal SPIFFS (1.9MB APP with OTA)** |
| Flash Size | `8MB` |

### Upload

1. `keyboard_controller.ino` in Arduino IDE öffnen
2. Board-Einstellungen wie oben setzen
3. Upload-Button drücken ▶
4. Fertig!

---

## 📡 Erster Start & WLAN-Setup

Beim ersten Start öffnet der ESP32 einen eigenen Hotspot:

```
SSID:     ESP32-Keyboard
Passwort: 12345678
```

1. Mit dem Hotspot verbinden
2. Browser öffnen → `http://192.168.4.1/`
3. Tab **📡 WLAN** → WLAN-Daten eingeben → Speichern
4. ESP32 startet neu und verbindet sich mit deinem Heimnetz

> Nach dem Verbinden ist das Interface unter der angezeigten IP-Adresse erreichbar,  
> z.B. `http://192.168.1.130/`

---

## 🖥️ Das Webinterface

Das Interface ist vollständig responsiv und auf dem Handy optimiert.

### Tab 1 – ⚡ Sequenz

Der Kern des Controllers. Hier baust du Sequenzen aus einzelnen Schritten.

```
┌─────────────────────────────────────────────┐
│  ⚡ Sequenz  ⌨️ Tasten  📁 Vorlagen  📡 WLAN  │
├─────────────────────────────────────────────┤
│  Schritte                                   │
│  ┌──────────────────────────────────────┐   │
│  │ 1  "notepad"           5ms  TEXT  ↑↓✎✕ │  │
│  │ 2  ENTER               TASTE  ↑↓✎✕    │  │
│  │ 3  500ms warten        PAUSE  ↑↓✎✕    │  │
│  └──────────────────────────────────────┘   │
│                                             │
│  [📝 Text ▼] [Eingabe...]    [Geschw. ▼] [+]│
└─────────────────────────────────────────────┘
```

**Schritt-Typen:**

| Typ | Beschreibung | Beispiel |
|---|---|---|
| 📝 **Text** | Beliebiger Text wird getippt | `Hallo Welt!` |
| ⌨️ **Taste** | Einzeltaste oder Kombination | `ENTER`, `LCTRL+c`, `Win+R` |
| ⏱ **Pause** | Wartezeit in Millisekunden | `500` (= 0,5 Sek) |

**Schritte verwalten:**
- **↑↓** – Reihenfolge ändern
- **✎** – Schritt bearbeiten (Wert + Geschwindigkeit)
- **✕** – Schritt löschen

**Einstellungen:**

| Einstellung | Beschreibung |
|---|---|
| Wiederholungen | `1× Einmalig` oder `🔁 Dauerhaft` |
| Tippgeschwindigkeit | 🐢 120ms → 🚶 60ms → 👤 20ms → 🚀 5ms → ⚡ 0ms → 🌌 Custom |
| Burst-Modus | Mehrere Zeichen pro USB-Paket (Checkbox zum Aktivieren) |
| Eingabe-Modus | Normal tippen oder Copy+Paste |

**Tippgeschwindigkeit pro Schritt:**  
Jeder Text-Schritt kann eine eigene Geschwindigkeit haben. Beim Hinzufügen einfach das Dropdown rechts neben dem Textfeld nutzen.

**Copy+Paste Modus:**  
Statt jeden Buchstaben einzeln zu senden, wird der gesamte Text per PowerShell in die Zwischenablage kopiert und mit `Ctrl+V` eingefügt. Ideal für langen Text. **Nur Windows.**

---

### Tab 2 – ⌨️ Schnelltasten

Einzelne Tasten oder Kombinationen per Klick senden – ohne Sequenz aufzubauen.

```
┌─────────────────────────────────────────────┐
│  Modifier                                   │
│  [L Ctrl] [L Shift] [L Alt] [⊞ Win] ...    │
│                                             │
│  Funktionstasten                            │
│  [ESC] [F1] [F2] ... [F12]                 │
│                                             │
│  Häufige Kombinationen                      │
│  [Ctrl+C] [Ctrl+V] [Alt+F4] [Win+R] ...    │
└─────────────────────────────────────────────┘
```

Verfügbare Tasten:
- Alle **Modifier**: Ctrl, Shift, Alt, AltGr, Windows-Taste (links/rechts)
- Alle **Funktionstasten**: F1–F12, ESC
- **Navigation**: Tab, Enter, Backspace, Delete, Home, End, PgUp, PgDn, Pfeile
- **System**: Caps Lock, Space, Print Screen, Num Lock
- **Häufige Combos**: Ctrl+C/V/X/Z/A/S, Alt+F4, Ctrl+Alt+Del, Win+D/L/R, Alt+Tab, Task-Manager

---

### Tab 3 – 📁 Vorlagen

Speichere häufig genutzte Sequenzen als Vorlage – auf dem ESP32 gespeichert und auf **allen Geräten** verfügbar.

```
┌─────────────────────────────────────────────┐
│  Vorlage bearbeiten                         │
│  ┌─────────────────────────────────────┐    │
│  │ Name: Login-Sequenz                 │    │
│  │ 1  "benutzername"    TEXT  ↑↓✕      │    │
│  │ 2  TAB               TASTE  ↑↓✕     │    │
│  │ 3  "passwort123"     TEXT  ↑↓✕      │    │
│  │ 4  ENTER             TASTE  ↑↓✕     │    │
│  └─────────────────────────────────────┘    │
│  [💾 Speichern]  [⬇ Aus Sequenz laden]      │
│                                             │
│  Gespeicherte Vorlagen                      │
│  ┌─────────────────────────────────────┐    │
│  │ 📄 Login-Sequenz    4 Schritte  ▼   │    │
│  │   + Anhängen  ▶ Laden  ✎  ✕         │    │
│  └─────────────────────────────────────┘    │
└─────────────────────────────────────────────┘
```

**Vorlage erstellen:**
1. Schritte im Editor oben aufbauen (genau wie Sequenz)
2. Namen vergeben
3. **💾 Speichern**

**Oder:** Aktuelle Sequenz übernehmen mit **⬇ Aus Sequenz laden**

**Vorlage nutzen:**
- **▶ Laden** – ersetzt die aktuelle Sequenz
- **+ Anhängen** – hängt die Vorlage an die Sequenz an
- **▼** – Vorschau der Schritte aufklappen
- **✎** – Vorlage bearbeiten
- **✕** – Vorlage löschen

> **Hinweis:** Vorlagen werden auf dem ESP32 gespeichert (nicht im Browser).  
> Sie sind auf PC, Handy und Tablet gleich verfügbar.

---

### Tab 4 – ▶▶ Autostart

Eine Vorlage wird automatisch nach dem Boot des ESP32 ausgeführt.

```
┌─────────────────────────────────────────────┐
│  ▶▶ Autostart                    [●] Aktiv  │
│                                             │
│  Vorlage auswählen                          │
│  ┌─────────────────────────────────────┐    │
│  │ ◉ Login-Sequenz   4 Schritte  ▼    │    │
│  │ ○ Makro-2         2 Schritte  ▼    │    │
│  └─────────────────────────────────────┘    │
│                                             │
│  Verzögerung nach Boot   [−] 5 s [+]        │
│  Wiederholungen          [−] 1x  [+]        │
│                                             │
│  [💾 Autostart speichern]                   │
│  💡 Tritt erst nach Neustart in Kraft!      │
└─────────────────────────────────────────────┘
```

**So funktioniert es:**
1. Vorlage anlegen (Tab Vorlagen)
2. Autostart-Tab öffnen → Vorlage auswählen (Radiobutton)
3. Toggle aktivieren
4. Verzögerung einstellen (wie lange nach Boot gewartet wird)
5. Wiederholungen: `1x`, `2x`, ... oder `Endlos`
6. **💾 Speichern**
7. ESP32 neu starten → Vorlage wird automatisch ausgeführt

> ⚠️ Der PC/das Zielprogramm muss beim Autostart bereit sein!  
> Verzögerung entsprechend groß wählen (mind. 5 Sek empfohlen).

---

### Tab 5 – 📡 WLAN

WLAN-Zugangsdaten ändern und aktuelle Verbindung einsehen.

```
┌─────────────────────────────────────────────┐
│  ● Secure-Potter-2.4G (-65dBm)             │
│    IP: 192.168.1.130                        │
│                                             │
│  Neues WLAN einrichten                      │
│  WLAN-Name: [________________]              │
│  Passwort:  [________________]              │
│  [💾 Speichern & Neustart]                  │
│                                             │
│  [🔧 Firmware Update (OTA)]                 │
└─────────────────────────────────────────────┘
```

---

### OTA Firmware Update

Neue Firmware direkt über den Browser flashen – kein USB nötig!

```
┌─────────────────────────────────────────────┐
│  🔧 Firmware Update                         │
│  Aktuell: v1.2.0 vom 2026-08-14            │
│                                             │
│  ┌─────────────────────────────────────┐    │
│  │  📦 Datei hierher ziehen            │    │
│  │  keyboard_controller.ino.bin (1MB) │    │
│  └─────────────────────────────────────┘    │
│  [Firmware hochladen]                       │
│  ████████████████████ 100%                  │
│  ✓ Fertig! ESP32 startet neu...            │
└─────────────────────────────────────────────┘
```

**So geht's:**
1. In Arduino IDE: **Sketch → Export Compiled Binary**
2. `.bin` Datei im Sketch-Ordner unter `build/` finden
3. Im Webinterface: **📡 WLAN → 🔧 Firmware Update**
4. `.bin` auswählen oder reinziehen → hochladen
5. ESP32 flasht sich selbst und startet neu

---

## 💡 Beispiel-Sequenzen

### Windows Ausführen-Dialog öffnen und Befehl eingeben

```
Schritt 1: Taste    LGUI+r        (Win+R)
Schritt 2: Pause    800ms         (Dialog öffnen lassen)
Schritt 3: Text     notepad       (Programm eingeben)
Schritt 4: Taste    ENTER
```

### Login-Formular ausfüllen

```
Schritt 1: Text     benutzer@mail.de
Schritt 2: Taste    TAB
Schritt 3: Text     MeinPasswort123!
Schritt 4: Taste    ENTER
```

### Bildschirm sperren + entsperren (Demo)

```
Schritt 1: Taste    LGUI+l        (Win+L = Sperren)
```

### Langen Text blitzschnell einfügen (Copy+Paste Modus)

```
Copy+Paste aktivieren, dann:
Schritt 1: Text     Sehr langer Text der sonst ewig dauern würde...
```

---

## ⚙️ Technische Details

### DE-Layout Mapping

Der ESP32 sendet HID-Keycodes basierend auf **US-Layout**. Da Windows als **DE-Layout** interpretiert, werden alle Zeichen manuell gemappt:

| Zeichen | Gesendet als |
|---|---|
| `&` | Shift + 6 |
| `@` | AltGr + Q |
| `{` | AltGr + 7 |
| `[` | AltGr + 8 |
| `]` | AltGr + 9 |
| `}` | AltGr + 0 |
| `\|` | AltGr + < |
| `ä ö ü ß` | Direkte Umlauttasten |

### Tippgeschwindigkeit

| Modus | Delay | Verwendung |
|---|---|---|
| 🐢 Sehr langsam | 120ms/Zeichen | Sehr träge Systeme |
| 🚶 Langsam | 60ms/Zeichen | Ältere PCs |
| 👤 Normal | 20ms/Zeichen | Standard |
| 🚀 Schnell | 5ms/Zeichen | Schnelle PCs |
| ⚡ Blitz | 0ms/Zeichen | Maximum |
| 🌌 Custom | Eigener Wert | Feinabstimmung |

### Datenspeicherung

| Daten | Speicherort |
|---|---|
| Sequenz-Schritte | Browser LocalStorage |
| Einstellungen | Browser LocalStorage |
| WLAN-Daten | ESP32 NVS (Preferences) |
| Vorlagen | ESP32 NVS (Preferences) |
| Autostart-Config | ESP32 NVS (Preferences) |

---

## 🔌 Pinout ESP32-S3-Zero

```
USB-C ──► USB HID (Tastatur) + Stromversorgung
WLAN  ──► 2.4 GHz 802.11 b/g/n (intern)
```

---

## 📋 Changelog

### v1.2.0
- Autostart-Funktion (Vorlage beim Boot ausführen)
- Vorlagen geräteübergreifend auf ESP32 gespeichert
- Geschwindigkeit pro Schritt einstellbar
- Custom ms-Eingabe für alle Speed-Dropdowns
- Burst-Modus als optionale Checkbox
- OTA-Update Verbesserungen

### v1.1.0
- Vorlagen-System mit vollem Editor
- Vorschau per Klick aufklappbar
- Copy+Paste Modus via PowerShell
- Stop unterbricht sofort
- CAPS LOCK Erkennung via getLEDs

### v1.0.0
- Erstes Release
- Vollständiges DE-Layout
- Sequenz-Builder
- WLAN-Setup + Hotspot-Fallback
- OTA-Update Grundgerüst

---

## 🐛 Bekannte Einschränkungen

- Copy+Paste Modus funktioniert nur unter **Windows**
- CAPS LOCK Status beim ersten Einstecken unbekannt (ESP32 trackt ab dann selbst)
- Vorlagen-Speicher begrenzt auf ca. **8KB** (NVS Limit)
- mDNS (`.local` Adresse) unter Windows nur mit Bonjour/iTunes

---

## 📄 Lizenz

MIT License – frei verwendbar, veränderbar und weitergabe.

---

## 🙏 Credits

Gebaut mit:
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [TinyUSB](https://github.com/hathach/tinyusb)
- JetBrains Mono & Inter Font (Google Fonts)
