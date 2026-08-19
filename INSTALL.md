# Installation – ESP32-S3 USB HID Keyboard Controller v1.13.10

Diese Anleitung beschreibt die empfohlene Installation der Firmware **v1.13.10** mit Arduino-ESP32 **3.3.11**.

## 1. Voraussetzungen

- Arduino IDE 2.x
- ESP32-S3 mit nativem USB-OTG, Referenz: Waveshare ESP32-S3-Zero
- USB-Datenkabel
- Windows für den mitgelieferten automatischen TinyUSB-Core-Patcher
- Arduino-ESP32 Core **3.3.11**, wenn der passive Host-Fingerprint-Patch verwendet werden soll

## 2. ESP32 Board-Paket installieren

In Arduino IDE unter **File/Datei → Preferences/Einstellungen → Additional boards manager URLs** hinzufügen:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Danach:

1. **Tools → Board → Boards Manager** öffnen.
2. Nach `esp32` suchen.
3. **esp32 by Espressif Systems** auswählen.
4. Für den Host-Fingerprint-Patch exakt **3.3.11** installieren.

> Der PowerShell-Patcher prüft bewusst auf den Core-Pfad von 3.3.11 und bricht ab, wenn diese Version nicht gefunden wird.

## 3. Optional, aber für vollständige Auto-OS-Erkennung empfohlen: Core-Patch

Die Firmware kann ohne Patch kompiliert werden. Für die erweiterten passiven Methoden A/L1/L4/M1/M3/M4 benötigt sie jedoch die zusätzlichen TinyUSB-Hooks.

### Patch installieren

Arduino IDE vollständig schließen. Dann PowerShell im Repository-Ordner öffnen und ausführen:

```powershell
powershell -ExecutionPolicy Bypass -File .\install_passive_host_fingerprint_core_3_3_11.ps1
```

Der Patcher verändert ausschließlich:

```text
%LOCALAPPDATA%\Arduino15\packages\esp32\hardware\esp32\3.3.11\cores\esp32\esp32-hal-tinyusb.c
```

Er legt Backups an:

```text
esp32-hal-tinyusb.c.optionA_backup
esp32-hal-tinyusb.c.hostfp_preupgrade_backup
```

Danach Arduino IDE neu starten.

### Patch zurücksetzen

Am einfachsten: Arduino-ESP32 3.3.11 im Boards Manager entfernen und neu installieren. Alternativ kann die vom Installer angelegte Originaldatei `.optionA_backup` manuell zurückkopiert werden.

> Ein Update auf eine andere Arduino-ESP32-Core-Version überschreibt bzw. ersetzt den gepatchten Core. Den 3.3.11-Patcher nicht auf andere Versionen anwenden.

## 4. Board-Einstellungen

Die USB-Optionen sind wichtig, da die Firmware ihre USB-Interfaces selbst erstellt.

| Einstellung | Wert |
|---|---|
| Board | `ESP32S3 Dev Module` |
| USB Mode | **USB-OTG (TinyUSB)** |
| USB CDC On Boot | **Disabled** |
| USB Firmware MSC On Boot | **Disabled** |
| USB DFU On Boot | **Disabled** |
| Flash Size | `8MB` empfohlen |
| Partition Scheme | OTA-fähig und mit nutzbarem Dateisystem-Bereich für LittleFS |

Die Firmware enthält Compile-Guards. Falsch aktivierte zusätzliche USB-Interfaces führen absichtlich zu einem Compilerfehler, statt später unklare USB-Probleme zu verursachen.

## 5. Firmware flashen

1. `keyboard_controller.ino` in Arduino IDE öffnen.
2. Board und Port auswählen.
3. Einstellungen aus Abschnitt 4 kontrollieren.
4. **Verify/Kompilieren** ausführen.
5. **Upload** starten.
6. Nach dem Upload den ESP32 neu starten bzw. USB neu verbinden.

## 6. WLAN-Erststart

Wenn keine gespeicherte WLAN-Verbindung verfügbar ist, startet der Controller einen Fallback-Access-Point. Mit dem AP verbinden und die im Projekt konfigurierte lokale Webadresse öffnen. Dort WLAN-Zugangsdaten eintragen und speichern.

Nach erfolgreicher Verbindung kann die Oberfläche über die angezeigte IP-Adresse und – sofern der Client mDNS unterstützt – über:

```text
http://esp32-keyboard.local/
```

erreicht werden.

## 7. USB-Profil

Auf der Startseite stehen DynamicModes zur Verfügung:

- **Tastatur**
- **Tastatur + Storage**

Ein Profilwechsel führt zu einer kurzen USB-Reenumeration. Das ist normal.

## 8. Automatische OS-Erkennung prüfen

Unter **Ausführmodus / OS-Erkennung** kann ein Erkennungstest gestartet werden. Mit installiertem 3.3.11-Core-Patch sollten die vorgesehenen passiven Methoden als verfügbar erscheinen.

Der HID-Controller startet nach jedem Neustart im Session-Modus:

```text
🔄 Automatisch
```

Manuelle Modi Windows/Linux/macOS/Allround gelten nur bis zum nächsten Neustart.

## 9. Vorlagen und Autostart

Vorlagen werden in LittleFS gespeichert. Autostart-Konfigurationen werden in NVS/Preferences gesichert und beim nächsten Boot ausgewertet.

Für Auto-Autostart können in einem Ordner Windows-, Linux- und macOS-Varianten hinterlegt werden. Die automatisch erkannte Variante wird beim Boot ausgewählt; fehlende Varianten werden nur über einen ausdrücklich konfigurierten Fallback ersetzt.

## 10. OTA-Update

Nach der Erstinstallation können spätere Firmware-Versionen ohne USB-Flash aktualisiert werden:

1. In Arduino IDE **Sketch → Export Compiled Binary**.
2. Die erzeugte Firmware-`.bin` auswählen.
3. OTA-Seite des ESP32 öffnen.
4. Datei hochladen.
5. Nach erfolgreichem Flash startet das Gerät automatisch neu.

## Fehlerbehebung

### Compiler meldet USB CDC/MSC/DFU

Die entsprechende Board-Menüoption muss **Disabled** sein. Die Firmware erzeugt die benötigten USB-Interfaces selbst.

### Methoden A/L4/M4 sind nicht verfügbar

- Arduino-ESP32-Version prüfen: **3.3.11**.
- Core-Patcher erneut ausführen.
- Arduino IDE danach vollständig neu starten.

### Webinterface erreichbar, USB-Eingaben funktionieren nicht

- Native USB-OTG-Buchse/-Verbindung verwenden.
- USB Mode auf **USB-OTG (TinyUSB)** prüfen.
- Zielgerät nach Profilwechsel kurz reenumerieren lassen.
- Im Runtime-Bereich prüfen, ob `USB bereit` gemeldet wird.

### Vorlagen fehlen nach Flash

Partitionierung und LittleFS-Bereich prüfen. Ein vollständiges Löschen des Flashs entfernt auch persistente Vorlagen/WLAN-/NVS-Daten.
