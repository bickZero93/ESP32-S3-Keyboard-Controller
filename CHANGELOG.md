# Changelog

Alle relevanten Änderungen am **ESP32-S3 USB HID Keyboard Controller** werden hier dokumentiert.

## [1.13.10] - 2026-08-19

### Geändert

- Autostart-Sequenzauswahl als echtes Toggle umgesetzt: erneuter Klick hebt eine aktive direkte OS-Auswahl bzw. den aktiven Auto-Ordner wieder auf.
- Auto-Darstellung überarbeitet: alle vorhandenen OS-Sequenzen des gewählten Ordners werden als potenziell aktiv markiert.
- Aktuell erkanntes OS erhält im Auto-Modus einen eigenen Status `Aktuell erkannt`.
- Andere Auto-Kandidaten werden als `Potenziell aktiv` gekennzeichnet.
- Manuelle Auswahl markiert ausschließlich die fest gewählte OS-Sequenz als `Manuell gewählt`; nicht aktive Varianten werden ausgegraut.

## [1.13.9] - 2026-08-19

### Geändert

- Mobile Oberfläche nochmals moderat vergrößert.
- Textartige Eingabefelder verwenden mobil 20px Schrift und größere Feldhöhen.
- `touch-action: manipulation` ergänzt, um unerwünschten Fokus-/Double-Tap-Zoom zu vermeiden.
- Auto-Layout-Neubewertung wird während aktiver mobiler Eingabe gegen Keyboard-Resize-Effekte stabilisiert.
- Manueller Pinch-Zoom bleibt über den normalen Viewport möglich.

## [1.13.8] - 2026-08-19

### Geändert

- Vertikales Mobile-Scrolling explizit freigegeben und gehärtet.
- iOS-Momentum-Scrolling und Smooth-Scrolling ergänzt.
- Dynamischer Abstand zur fixierten Bottom-Navigation inkl. Safe-Area.
- Interne Scrollbereiche für Vorlagen, Autostartlisten und Log.
- `Nach oben`-Button ab größerer Scrolltiefe.
- Mobile Sequenzseite sinnvoller priorisiert: Builder/Start-Stop vor Runtime-Diagnose.

## [1.13.7] - 2026-08-19

### Hinzugefügt

- Session-OS-Modus im HID Controller: `Automatisch`, `Windows`, `Linux`, `macOS`, `Allround`.
- `Allround` umgeht OS-Filter vollständig.
- Manuelle Session-Auswahl wird im Aktivitätslog dokumentiert und nicht persistent gespeichert.

### Geändert

- Mobile Typografie, Buttons, Eingabefelder und Abstände weiter vergrößert.
- Nach jedem Neustart wird der HID-OS-Modus wieder auf `Automatisch` gesetzt.

## [1.13.6] - 2026-08-19

### Geändert

- Smartphone-Darstellung nach realem Samsung-S25-Test vergrößert.
- Größere Sekundärtexte, Karten, Kernbedienelemente und Bottom-Navigation.
- Skalierung ohne CSS `transform`/`zoom`, um native Viewportbreite beizubehalten.

## [1.13.5] - 2026-08-19

### Geändert

- Mobile Breakpoints für 320–360, 361–414, 415–480 und 481–768 CSS-px nachgeschärft.
- Mindestens 48px Touchziele und mindestens 16px Formschrift.
- Safe-Area-Padding ergänzt.
- Bottom-Navigation auf schmalen Smartphones als lesbares 3×2-Raster.
- Horizontaler Seitenüberlauf auf typischen Smartphone-/Tabletbreiten beseitigt.

## [1.13.4] - 2026-08-19

### Hinzugefügt

- Automatisches Desktop/Mobile-Layout anhand von Breite und Orientierung.
- Manueller Desktop/Mobil-Umschalter ohne Reload und ohne Verlust des UI-Zustands.
- Mobile Bottom-Navigation, einspaltige Builder, aufklappbare Karten und OS-Tabs.

### Geändert

- Vorlagen, Autostart, OS-Erkennung, Einstellungen und Log für Touch-Bedienung optimiert.

## [1.13.3] - 2026-08-19

### Hinzugefügt

- Windows-Taste als eigenständige Auswahl `Win`.
- Vorlagen-Textschritte mit derselben per-Schritt-Geschwindigkeitsauswahl wie Sequenzen.

### Geändert

- Sichtbare Bezeichnung `LGUI` vollständig auf `Win` vereinheitlicht; interne HID-Verarbeitung bleibt unverändert.
- Legacy-Einträge mit `LGUI` werden beim Laden als `Win` dargestellt.
- Vorlagen-Schrittbuilder funktional und optisch an Sequenz → Schritte angeglichen.

## [1.13.2] - 2026-08-19

### Geändert

- Passive Host-OS-Erkennung vollständig ESP32-seitig konsolidiert.
- Manuelle Host-OS-Überschreibung aus der früheren Erkennungslogik entfernt.
- Auto-Autostart folgt freigegebenen starken OS-Signalen: Windows über A, Linux über L4, macOS über M4.
- Fallback-OS wird nur verwendet, wenn ausdrücklich konfiguriert.
- Bei unbekanntem OS startet Auto keine beliebige OS-Sequenz.

### Host-Fingerprint

- Optionaler Arduino-ESP32-3.3.11-Core-Patch für Microsoft OS 1.0/2.0 und Descriptor-/Vendor-Beobachtung.
- Schwache Methoden B/C bestimmen niemals allein das OS.
- Methode D bleibt reine Diagnostik und wird nicht als OS-Beweis verwendet.

## [1.4.1] - 2026-08-14

### Hinzugefügt

- Eigener DuckyScript-3-Tab mit Datei-Upload und Quelltext-Editor.
- Zielsystemwahl und Kompatibilitätsmodi für den DuckyScript-Import.
- FUNCTION-Import, DEFINE/Variablen-Verarbeitung und Importdiagnose.
- Arduino-.INO/.CPP/.H-Import für erkennbare Tastaturabläufe.
- Touch-optimierte virtuelle QWERTZ-Tastatur.
- Firmware-Endpunkt `/version` und Versionsanzeige.

### Geändert

- Vorlagen primär nach LittleFS (`/templates.json`) migriert.
- Robusteres temporäres Schreiben und Backup der Vorlagen.
- Legacy-NVS-Migration und LocalStorage-Fallback.
- Geschwindigkeitsbehandlung und Clipboard-Verzögerung überarbeitet.

## [1.2.0]

- Autostart-Funktion.
- Geräteübergreifend gespeicherte Vorlagen.
- Individuelle Textschritt-Geschwindigkeit und Custom-ms.
- Optionaler Burst-Modus.

## [1.1.0]

- Vorlagen-System mit Editor/Vorschau.
- Copy+Paste-Modus über Windows-Clipboard/PowerShell.
- Sofortiges Stoppen laufender Sequenzen.
- Caps-Lock-Zustandsbehandlung.

## [1.0.0]

- Erstes Release.
- Deutsches QWERTZ-/Sonderzeichen-Mapping.
- Sequenz-Builder.
- WLAN-Konfiguration mit Fallback-Hotspot.
- OTA-Grundfunktion.
