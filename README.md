# ESP32-S3 USB HID Keyboard Controller

> WLAN-gesteuerter USB-HID-Keyboard-Controller für den ESP32-S3-Zero mit
> responsivem Webinterface, Sequenzen, DuckyScript-3-Import, Vorlagen,
> Autostart und OTA-Updates.

**Aktuelle Firmware:** `v1.4.1` · `2026-08-14`

------------------------------------------------------------------------

`<img width="500" height="700" alt="ESP32-S3 Keyboard Controller Webinterface" src="https://github.com/user-attachments/assets/f3075592-b8b3-40d3-89a4-1428b830e045" />`{=html}

## ✨ Features

  -----------------------------------------------------------------------
  Feature                             Beschreibung
  ----------------------------------- -----------------------------------
  ⚡ **Sequenz-Builder**              Text, Tasten/Kombinationen und
                                      Pausen zu Abläufen zusammenstellen

  🇩🇪 **DE-QWERTZ-Layout**             Manuelles HID-Mapping für deutsche
                                      Buchstaben, Umlaute und
                                      Sonderzeichen

  🐤 **DuckyScript 3 Import**         `.txt` laden, analysieren und in
                                      normale Controller-Schritte
                                      umwandeln

  🧩 **DuckyScript-Funktionen**       `FUNCTION`-Definitionen als
                                      auswählbare Sequenzbausteine
                                      importieren

  📥 **Arduino-.INO-Import**          Unterstützte Tastaturabläufe aus
                                      `.ino`, `.cpp` und `.h` übernehmen

  ⌨️ **Virtuelle QWERTZ-Tastatur**    Große Touch-Tasten für Schreiben,
                                      Modifier, Navigation und
                                      PC-Funktionen

  📋 **Copy+Paste-Modus**             Längere Texte unter Windows per
                                      PowerShell/Clipboard und `Ctrl+V`
                                      einfügen

  🚀 **Flexible Geschwindigkeit**     Presets, Custom-ms und individuelle
                                      Geschwindigkeit pro Text-Schritt

  📦 **Optionaler Burst-Modus**       Mehrere Zeichen pro
                                      Verarbeitungslauf; standardmäßig
                                      deaktiviert

  📁 **Persistente Vorlagen**         Vorlagen im LittleFS des ESP32, mit
                                      Backup und Migration älterer
                                      NVS-Daten

  ▶▶ **Autostart**                    Gespeicherte Vorlage nach dem Boot
                                      automatisch ausführen

  🔧 **OTA-Update**                   Kompilierte `.bin` direkt über den
                                      Browser flashen

  📡 **WLAN + Fallback-AP**           Verbindung mit dem Heimnetz oder
                                      eigener Hotspot bei fehlender
                                      Verbindung

  🌐 **mDNS**                         Im Heimnetz zusätzlich über
                                      `esp32-keyboard.local` erreichbar,
                                      sofern der Client mDNS unterstützt

  💾 **Browser-Persistenz**           Aktuelle Sequenz und
                                      UI-Einstellungen werden lokal im
                                      Browser gespeichert
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## 🛒 Hardware

  Komponente        Empfehlung
  ----------------- -------------------------------------
  Mikrocontroller   **Waveshare ESP32-S3-Zero**
  USB               USB-C mit nativem USB-OTG/HID
  Stromversorgung   Über USB vom Zielgerät oder separat

Der Sketch nutzt den ESP32-S3 gleichzeitig als USB-HID-Tastatur und als
WLAN-Webserver.

------------------------------------------------------------------------

## 🚀 Installation

### Voraussetzungen

-   Arduino IDE 2.x
-   ESP32-Boardpaket von Espressif Systems
-   ESP32-S3 mit nativem USB, getestet/ausgelegt für den Waveshare
    ESP32-S3-Zero

### ESP32-Boardpaket installieren

1.  Arduino IDE öffnen → **File → Preferences**
2.  Unter **Additional boards manager URLs** eintragen:

``` text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

3.  **Tools → Board → Boards Manager**
4.  Nach `esp32` suchen und **esp32 by Espressif Systems** installieren.

### Board-Einstellungen

  Einstellung        Wert
  ------------------ -----------------------------------------
  Board              `ESP32S3 Dev Module`
  USB Mode           **USB-OTG (TinyUSB)**
  USB CDC On Boot    **Enabled**
  Partition Scheme   **Minimal SPIFFS (1.9MB APP with OTA)**
  Flash Size         `8MB`

> Die Firmware verwendet **LittleFS** für Vorlagen. Das gewählte
> Partitionsschema muss daher einen nutzbaren Dateisystem-Bereich
> bereitstellen und gleichzeitig OTA erlauben.

### Upload

1.  `keyboard_controller.ino` öffnen.
2.  Board und Optionen wie oben einstellen.
3.  ESP32-S3 per USB verbinden.
4.  **Upload** starten.
5.  Nach dem Neustart steht das Webinterface zur Verfügung.

------------------------------------------------------------------------

## 📡 Erster Start und WLAN

Wenn keine gespeicherte WLAN-Verbindung hergestellt werden kann, startet
der Controller einen Access Point:

``` text
SSID:     ESP32-Keyboard
Passwort: 12345678
Adresse:  http://192.168.4.1/
```

Danach:

1.  Mit `ESP32-Keyboard` verbinden.
2.  `http://192.168.4.1/` öffnen.
3.  Tab **📡 WLAN** auswählen.
4.  SSID und Passwort des Heimnetzes speichern.
5.  Der ESP32 startet neu und verbindet sich mit dem WLAN.

Bei erfolgreicher Verbindung zeigt das Webinterface SSID, RSSI und
IP-Adresse an. Zusätzlich startet die Firmware mDNS unter:

``` text
http://esp32-keyboard.local/
```

Falls `.local` auf dem Client nicht aufgelöst wird, einfach die
angezeigte IP-Adresse verwenden.

------------------------------------------------------------------------

## 🖥️ Webinterface

Das Webinterface ist responsiv und für Desktop, Tablet und Smartphone
ausgelegt.

### ⚡ Sequenz

Eine Sequenz besteht aus bis zu **128 Schritten**.

  -----------------------------------------------------------------------
  Schritt                 Beschreibung            Beispiel
  ----------------------- ----------------------- -----------------------
  **Text**                Text im deutschen       `Hallo Welt!`
                          Layout tippen           

  **Taste**               Einzeltaste oder        `ENTER`, `LCTRL+c`,
                          Kombination senden      `LGUI+r`

  **Pause**               Wartezeit in            `500`
                          Millisekunden           
  -----------------------------------------------------------------------

Schritte können hinzugefügt, bearbeitet, gelöscht und umsortiert werden.

#### Geschwindigkeit

Für Text stehen globale Presets zur Verfügung:

  Modus                  Verzögerung
  -------------- -------------------
  Sehr langsam        120 ms/Zeichen
  Langsam              60 ms/Zeichen
  Normal               20 ms/Zeichen
  Schnell               5 ms/Zeichen
  Blitz                 0 ms/Zeichen
  Custom           0--500 ms/Zeichen

Jeder einzelne Text-Schritt kann die globale Geschwindigkeit
überschreiben.

#### Wiederholung

Sequenzen können einmalig oder dauerhaft ausgeführt werden. Im
Loop-Modus lässt sich zusätzlich die Pause zwischen zwei Durchläufen
einstellen. Eine laufende Sequenz kann über **Stop** abgebrochen werden.

#### Burst-Modus

Burst ist optional und standardmäßig aus. Die Burst-Größe kann im
Webinterface eingestellt werden. Ohne explizite Aktivierung arbeitet die
Firmware weiterhin zeichenweise.

#### Copy+Paste-Modus

Der Clipboard-Modus ist für **Windows** gedacht. Die Firmware öffnet
einen PowerShell-Aufruf, setzt den gewünschten Text in die
Zwischenablage und fügt ihn anschließend mit `Ctrl+V` ein.

Für diesen Modus existiert eine eigene konfigurierbare
Zeichenverzögerung.

> Clipboard ist schneller für lange Texte, setzt aber PowerShell und
> eine Windows-Umgebung voraus. Für plattformunabhängige Abläufe den
> normalen HID-Textmodus verwenden.

------------------------------------------------------------------------

## 🐤 DuckyScript 3 Import

`v1.4.1` enthält einen eigenen **DuckyScript-3-Tab**. Eine `.txt`-Datei
kann geladen oder der Quelltext direkt in den Editor eingefügt werden.

Der Importer:

-   analysiert direkt ausführbare Befehle,
-   wandelt unterstützte Befehle in Text-, Tasten- und Pause-Schritte
    um,
-   erkennt `FUNCTION`-Definitionen und bietet sie separat als
    Sequenzbausteine an,
-   löst unterstützte `DEFINE`s und Variablen beim Import auf,
-   kann statisch auswertbare OS-Bedingungen anhand eines gewählten
    Zielsystems behandeln,
-   zeigt Parserfehler, Warnungen, Anpassungen und übersprungene Befehle
    an.

### Zielsysteme

Im Importer kann zwischen folgenden Zielsystemen gewählt werden:

`Windows`, `macOS`, `Linux`, `ChromeOS`, `Android`, `iOS`, `Other`

### Kompatibilitätsmodi

  -----------------------------------------------------------------------
  Modus                               Verhalten
  ----------------------------------- -----------------------------------
  **Automatisch**                     Nicht sinnvoll abbildbare
                                      Hardwarefunktionen werden angepasst
                                      oder übersprungen

  **Strikt**                          Nicht unterstützte/unsichere
                                      Konstrukte werden als Fehler
                                      behandelt

  **Roh/Teilimport**                  Soweit möglich importieren und
                                      Warnungen ausgeben
  -----------------------------------------------------------------------

Hardwareabhängige Ducky-Funktionen, die der ESP32 als reine USB-Tastatur
nicht zuverlässig nachbilden kann, werden nicht einfach vorgetäuscht.
Dazu gehören beispielsweise Host-Lock-State-Wartebedingungen oder
Rubber-Ducky-spezifische LED-Funktionen.

------------------------------------------------------------------------

## 📥 Arduino-.INO-Import

Im Sequenz-Tab können `.ino`, `.cpp`, `.h` und Textdateien geladen
werden. Der Browser versucht, unterstützte Tastaturaktionen in normale
Controller-Schritte umzuwandeln.

Nach dem Import wird angezeigt, wie viele Schritte erkannt wurden und ob
Warnungen aufgetreten sind.

> Der Import ist als Konverter für erkennbare Tastaturabläufe gedacht,
> nicht als allgemeiner C++-Interpreter.

------------------------------------------------------------------------

## ⌨️ Virtuelle QWERTZ-Tastatur

Der Tab **Tasten** enthält eine Touch-optimierte virtuelle Tastatur mit
zwei Bereichen:

-   **ABC · Schreiben** für QWERTZ, Zahlen, Umlaute, Sonderzeichen und
    Modifier
-   **PC · Navigation** für typische Navigations- und Systemtasten

Modifier können für Kombinationen verwendet werden; ein Statusindikator
zeigt aktive Modifier im Webinterface an.

Zusätzlich stehen weiterhin typische Kombinationen und HID-Tasten zur
Verfügung, unter anderem:

-   Ctrl, Shift, Alt, AltGr und Windows-Taste
-   F1--F12 und Esc
-   Tab, Enter, Backspace, Delete und Insert
-   Home, End, Page Up/Down und Pfeiltasten
-   Caps Lock, Print Screen und Num Lock
-   Kombinationen wie `Ctrl+C`, `Ctrl+V`, `Alt+F4`, `Win+R` oder
    `Alt+Tab`

------------------------------------------------------------------------

## 📁 Vorlagen

Sequenzen können als Vorlagen auf dem ESP32 gespeichert werden. Dadurch
stehen sie unabhängig vom verwendeten Browser auf PC, Smartphone und
Tablet zur Verfügung.

Mit `v1.4.1` liegen Vorlagen nicht mehr primär als einzelner NVS-String
vor, sondern in **LittleFS**:

``` text
/templates.json
```

Beim Speichern wird zunächst eine temporäre Datei geschrieben und die
vorherige Version gesichert. Erst danach wird die neue Datei aktiviert.

Weitere Details:

-   maximales Vorlagen-JSON: **256 KiB**
-   temporäres Schreiben über `/templates.tmp`
-   Backup über `/templates.bak`
-   automatische Migration vorhandener Legacy-Vorlagen aus NVS, sofern
    noch keine LittleFS-Datei existiert
-   Browser-LocalStorage dient im Frontend zusätzlich als Fallback

Vorlagen können geladen, an eine vorhandene Sequenz angehängt,
bearbeitet, gelöscht und in der Vorschau aufgeklappt werden.

------------------------------------------------------------------------

## ▶▶ Autostart

Eine gespeicherte Vorlage kann nach dem Boot automatisch ausgeführt
werden.

Konfigurierbar sind:

-   Vorlage
-   Aktiviert/Deaktiviert
-   Startverzögerung
-   Wiederholungsanzahl (`0` = endlos)
-   Geschwindigkeit
-   Burst-Größe

Die Autostart-Konfiguration liegt in ESP32-NVS/Preferences. Änderungen
werden gespeichert, greifen aber bewusst erst nach einem Neustart.

> Beim Autostart muss das Zielgerät bereits bereit sein. Plane
> ausreichend Boot-/Programmstartzeit ein.

------------------------------------------------------------------------

## 🔧 OTA Firmware Update

Unter `/ota` kann eine in der Arduino IDE erzeugte Firmware-Datei direkt
über den Browser installiert werden.

1.  **Sketch → Export Compiled Binary**
2.  erzeugte `.bin` auswählen
3.  im Webinterface **WLAN → Firmware Update** öffnen
4.  `.bin` hochladen
5.  nach erfolgreichem Flash startet der ESP32 neu

Die OTA-Seite zeigt die aktuell laufende Firmware-Version an.

------------------------------------------------------------------------

## 🇩🇪 DE-Layout und HID

Die Firmware sendet rohe USB-HID-Keycodes und bildet Zeichen explizit
auf ein deutsches QWERTZ-Layout ab. Dadurch werden unter anderem Y/Z,
Shift-Zeichen, AltGr-Zeichen und deutsche Umlaute gezielt behandelt.

Beispiele:

  Zeichen              HID-Eingabe auf DE-Layout
  -------------------- ----------------------------------
  `@`                  `AltGr + Q`
  `{`                  `AltGr + 7`
  `[`                  `AltGr + 8`
  `]`                  `AltGr + 9`
  `}`                  `AltGr + 0`
  `\`                  `AltGr` + entsprechende DE-Taste
  `|`                  `AltGr + <`
  `ä`, `ö`, `ü`, `ß`   direkte DE-Tasten

Die Firmware verarbeitet außerdem UTF-8-Sequenzen für die unterstützten
deutschen Sonderzeichen.

------------------------------------------------------------------------

## 💾 Datenspeicherung

  Daten                               Speicherort
  ----------------------------------- -----------------------------------------------
  Aktuelle Sequenz/UI-Einstellungen   Browser LocalStorage
  Vorlagen                            ESP32 LittleFS (`/templates.json`)
  Legacy-Vorlagen                     NVS; werden bei Bedarf nach LittleFS migriert
  WLAN-Zugangsdaten                   ESP32 NVS / Preferences
  Autostart-Konfiguration             ESP32 NVS / Preferences
  Firmware                            ESP32 Flash / OTA-Partition

------------------------------------------------------------------------

## 🌐 HTTP-Endpunkte

Das Webinterface verwendet intern folgende Routen:

  Route               Zweck
  ------------------- -----------------------------------
  `/`                 Webinterface
  `/version`          Firmware-Version und Datum
  `/wifiinfo`         WLAN-Status
  `/run`              Sequenz starten
  `/stop`             laufende Sequenz stoppen
  `/status`           Laufstatus abfragen
  `/sendkey`          einzelne Taste/Kombination senden
  `/templates`        Vorlagen laden
  `/templates/save`   Vorlagen speichern
  `/autostart`        Autostart-Konfiguration laden
  `/autostart/save`   Autostart-Konfiguration speichern
  `/config`           WLAN-Konfiguration speichern
  `/ota`              OTA-Webseite
  `/update`           Firmware-Upload

------------------------------------------------------------------------

## 🐛 Bekannte Einschränkungen

-   Das Zeichenmapping ist auf ein **deutsches Host-Tastaturlayout**
    ausgelegt.
-   Der Copy+Paste-Modus ist **Windows/PowerShell-spezifisch**.
-   Der tatsächliche Caps-Lock-Zustand des Hosts ist beim Start nicht
    zuverlässig bekannt; die Firmware führt deshalb einen eigenen
    Zustandstracker.
-   DuckyScript-Hardwarefunktionen, die echte Rubber-Ducky-Hardware,
    LEDs, Buttons oder zuverlässige Host-Zustandsabfragen voraussetzen,
    können nicht vollständig emuliert werden.
-   Der DuckyScript-/INO-Import übersetzt unterstützte Abläufe in das
    interne Sequenzmodell und ist kein vollständiger Interpreter der
    jeweiligen Sprache.
-   `.local`/mDNS muss vom verwendeten Betriebssystem bzw. Netzwerk
    unterstützt werden.
-   Vorlagen sind auf **256 KiB JSON-Daten** begrenzt.

------------------------------------------------------------------------

## 📋 Changelog

Das ausführliche Änderungsprotokoll befindet sich in
[`CHANGELOG.md`](CHANGELOG.md).

### v1.4.1 --- 2026-08-14

-   DuckyScript-3-Importer mit eigenem Editor-Tab
-   Import von DuckyScript-Funktionen als Sequenzbausteine
-   Ziel-OS und Kompatibilitätsmodus für den DuckyScript-Import
-   `.ino`/C++-Import für erkennbare Tastaturabläufe
-   neue Touch-optimierte virtuelle QWERTZ-Tastatur
-   Vorlagen auf LittleFS umgestellt
-   atomisches Vorlagen-Speichern mit Temp-Datei und Backup
-   Migration älterer NVS-Vorlagen
-   Vorlagenlimit auf 256 KiB erhöht
-   überarbeitete Geschwindigkeits-/Burst-Behandlung
-   Firmware-Version im Webinterface und auf der OTA-Seite

------------------------------------------------------------------------

## 📄 Lizenz

MIT License. Siehe [`LICENSE`](LICENSE).

------------------------------------------------------------------------

## 🙏 Credits

Gebaut mit bzw. auf Basis von:

-   [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
-   [TinyUSB](https://github.com/hathach/tinyusb)
-   ESP32 `WiFi`, `WebServer`, `Preferences`, `LittleFS`, `Update` und
    `ESPmDNS`
-   JetBrains Mono und Inter im Webinterface

------------------------------------------------------------------------

## ⚠️ Hinweis

USB-HID-Eingaben wirken auf dem angeschlossenen Zielgerät wie echte
Tastatureingaben. Teste neue Sequenzen zuerst in einer ungefährlichen
Umgebung und verwende Autostart nur, wenn das erwartete Zielsystem und
Eingabefeld eindeutig sind.
