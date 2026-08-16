# Changelog

Alle relevanten Änderungen am **ESP32-S3 USB HID Keyboard Controller**
werden in dieser Datei dokumentiert.

## \[1.4.1\] - 2026-08-14

### Hinzugefügt

-   Eigener **DuckyScript-3-Tab** mit Datei-Upload und Quelltext-Editor.
-   DuckyScript-Analyse mit wählbarem Zielsystem: Windows, macOS, Linux,
    ChromeOS, Android, iOS und Other.
-   Drei DuckyScript-Kompatibilitätsmodi: **Automatisch**, **Strikt**
    und **Roh/Teilimport**.
-   Erkennung von `FUNCTION`-Definitionen und Import einzelner
    Funktionen als normale Sequenzbausteine.
-   Verarbeitung unterstützter `DEFINE`s, Variablen und statisch
    auswertbarer Bedingungen beim DuckyScript-Import.
-   Importstatus mit Parserfehlern, Warnungen, Anpassungen und
    übersprungenen Hardwarebefehlen.
-   Import von `.ino`, `.cpp`, `.h` und Textdateien für erkennbare
    Tastaturabläufe.
-   Touch-optimierte **virtuelle QWERTZ-Tastatur** mit getrennten
    Bereichen für Schreiben und PC/Navigation.
-   Firmware-Endpunkt `/version` sowie Versionsanzeige im Hauptinterface
    und auf der OTA-Seite.

### Geändert

-   Vorlagen werden jetzt primär in **LittleFS** unter `/templates.json`
    gespeichert statt als einzelner NVS-String.
-   Maximale Größe der Vorlagendaten auf **256 KiB** angehoben.
-   Vorlagen-Speicherung robuster gemacht: Schreiben über temporäre
    Datei, Backup der vorherigen Datei und Aktivierung erst nach
    erfolgreichem Schreiben.
-   Vorhandene Legacy-Vorlagen aus NVS werden automatisch nach LittleFS
    migriert, wenn noch keine Vorlagendatei existiert.
-   Browser-LocalStorage bleibt als Frontend-Fallback für Vorlagen
    erhalten.
-   Geschwindigkeitsbehandlung überarbeitet: feste HID-Haltezeit ist von
    der eigentlichen Inter-Character-Verzögerung getrennt.
-   Text-Schritte unterstützen weiterhin individuelle Geschwindigkeiten
    inklusive Custom-ms.
-   Burst bleibt standardmäßig bei einem Zeichen und wird nur nach
    expliziter Aktivierung verwendet.
-   Clipboard-Modus besitzt eine eigene konfigurierbare
    Zeichenverzögerung.
-   Webinterface für mobile Bedienung und Vorlagenverwaltung weiter
    optimiert.

### Kompatibilität / Verhalten

-   Nicht sinnvoll emulierbare DuckyScript-Hardwarebefehle werden je
    nach gewähltem Modus gemeldet, angepasst oder übersprungen.
-   Host-Lock-State-Wartebefehle können nicht zuverlässig importiert
    werden, da der Controller den Lock-Zustand des Hosts nicht sicher
    abfragen kann.
-   Rubber-Ducky-spezifische LED-Funktionen werden nicht als
    Tastatursequenz emuliert.
-   Autostart-Änderungen greifen weiterhin erst nach einem Neustart des
    ESP32.

## \[1.2.0\]

### Hinzugefügt

-   Autostart-Funktion zum automatischen Ausführen einer Vorlage nach
    dem Boot.
-   Geräteübergreifend auf dem ESP32 gespeicherte Vorlagen.
-   Individuelle Geschwindigkeit pro Text-Schritt.
-   Custom-ms-Eingabe für Geschwindigkeiten.
-   Optionaler Burst-Modus.

### Geändert

-   OTA-Update-Ablauf verbessert.

## \[1.1.0\]

### Hinzugefügt

-   Vorlagen-System mit Editor und Vorschau.
-   Copy+Paste-Modus über PowerShell/Windows-Clipboard.
-   Sofortiges Stoppen laufender Sequenzen.
-   Caps-Lock-Zustandsbehandlung.

## \[1.0.0\]

### Hinzugefügt

-   Erstes Release.
-   Deutsches QWERTZ-/Sonderzeichen-Mapping.
-   Sequenz-Builder.
-   WLAN-Konfiguration mit Fallback-Hotspot.
-   OTA-Grundfunktion.
