# Schnellstart – v1.13.10

## 1. Arduino-ESP32

Für die vollständige passive OS-Erkennung **esp32 by Espressif Systems 3.3.11** installieren.

## 2. Optionalen Host-Fingerprint-Patch installieren

Arduino IDE schließen und im Repository-Ordner ausführen:

```powershell
powershell -ExecutionPolicy Bypass -File .\install_passive_host_fingerprint_core_3_3_11.ps1
```

## 3. Board-Einstellungen

```text
Board:                    ESP32S3 Dev Module
USB Mode:                 USB-OTG (TinyUSB)
USB CDC On Boot:          Disabled
USB Firmware MSC On Boot: Disabled
USB DFU On Boot:          Disabled
Flash Size:               8MB
Partition Scheme:         OTA-fähig + LittleFS-Bereich
```

## 4. Hochladen

`keyboard_controller.ino` öffnen → Kompilieren → Upload.

## 5. Webinterface öffnen

Fallback-AP bzw. die vom ESP32 ausgegebene IP verwenden. Im Heimnetz ist zusätzlich `http://esp32-keyboard.local/` vorgesehen.

## 6. Prüfen

- Startseite: USB-Profil auswählen.
- HID Controller: `🔄 Automatisch` ist nach Neustart Standard.
- OS-Erkennung: Erkennungstest ausführen.
- Sequenz: ersten Text-/Tastenschritt anlegen und starten.

Ausführliche Anleitung: **[INSTALL.md](INSTALL.md)**.
