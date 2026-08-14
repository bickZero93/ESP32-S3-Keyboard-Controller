# Schnellstart

## 1. Arduino IDE Einstellungen

```
Board:            ESP32S3 Dev Module
USB Mode:         USB-OTG (TinyUSB)
USB CDC On Boot:  Enabled
Partition Scheme: Minimal SPIFFS (1.9MB APP with OTA)
Flash Size:       8MB
```

## 2. Hochladen

Sketch öffnen → Upload ▶

## 3. Verbinden

```
Hotspot:  ESP32-Keyboard
Passwort: 12345678
URL:      http://192.168.4.1/
```

## 4. WLAN eintragen

Tab WLAN → Zugangsdaten → Speichern → Neustart

## 5. Fertig!

Interface unter http://[IP-Adresse]/

## OTA Update

Sketch → Export Compiled Binary → .bin per http://[IP]/ota hochladen
