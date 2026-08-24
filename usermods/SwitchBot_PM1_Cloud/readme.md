# SwitchBot PM1 Cloud Control Usermod

## Beschreibung (DE)
Dieser Usermod ermöglicht die Kontrolle eines **SwitchBot PM1 Smart Relais** aus WLED über die offizielle SwitchBot Cloud API. Du kannst:
- Das Relais über die WLED Web-UI ein-/ausschalten
- Das Relais mit JSON-API-Befehlen steuern
- Den Relais-Status periodisch synchronisieren
- Direkt über die WLED-Seite steuern

## Voraussetzungen
- WLED 0.13.0 oder neuer
- ESP32 oder ESP8266
- SwitchBot PM1 Smart Plug/Relais
- WiFi-Verbindung
- SwitchBot API Token

## Setup-Anleitung

### 1. SwitchBot Credentials beschaffen

**API Token:**
1. Gehe zu https://profile.switch-bot.com/api
2. Melde dich mit deinem SwitchBot-Konto an
3. Kopiere deinen **API Token**

**Device ID:**
- Öffne die SwitchBot Mobile App
- Gehe zu Settings des PM1 Geräts
- Device ID kopieren
- Oder nutze: `curl -H "Authorization: YOUR_TOKEN" https://api.switch-bot.com/v1.0/devices`

### 2. WLED konfigurieren

**Option A: Platformio (Empfohlen)**

Bearbeite `usermods/platformio_override.usermods.ini`:
```ini
[env:esp32]
build_flags = ${common.build_flags}
  -D USERMOD_SWITCHBOT_PM1
  -D SWITCHBOT_API_TOKEN="dein_api_token_hier"
  -D SWITCHBOT_DEVICE_ID="deine_device_id_hier"
```

**Option B: Runtime-Konfiguration**

Nach dem Upload in der WLED Web-UI:
1. Gehe zu **Einstellungen** → **Usermods**
2. Gib deinen API Token und die Device ID ein
3. Aktiviere den Usermod

### 3. Kompilieren und hochladen

```bash
platformio run -e esp32 -t upload
```

## Verwendung

### Web UI
- Ein **SwitchBot** Bereich erscheint in den WLED-Einstellungen
- Toggle-Button zum Ein-/Ausschalten des Relais
- Aktueller Relais-Status anzeigen

### JSON API

**Relais AN:**
```bash
curl -X POST http://<WLED_IP>/json \
  -H "Content-Type: application/json" \
  -d '{"SwitchBot":{"on":true}}'
```

**Relais AUS:**
```bash
curl -X POST http://<WLED_IP>/json \
  -H "Content-Type: application/json" \
  -d '{"SwitchBot":{"on":false}}'
```

**Relais umschalten:**
```bash
curl -X POST http://<WLED_IP>/json \
  -H "Content-Type: application/json" \
  -d '{"SwitchBot":{"toggle":true}}'
```

### Beispiele mit anderen Tools

**PowerShell (Windows):**
```powershell
# Relais AN
$body = @{SwitchBot=@{on=$true}} | ConvertTo-Json
Invoke-WebRequest -Uri "http://<WLED_IP>/json" -Method Post -Body $body -ContentType "application/json"

# Relais AUS
$body = @{SwitchBot=@{on=$false}} | ConvertTo-Json
Invoke-WebRequest -Uri "http://<WLED_IP>/json" -Method Post -Body $body -ContentType "application/json"
```

**Python:**
```python
import requests
import json

wled_ip = "192.168.1.100"
url = f"http://{wled_ip}/json"

# Relais AN
payload = {"SwitchBot": {"on": True}}
requests.post(url, json=payload)

# Relais AUS
payload = {"SwitchBot": {"on": False}}
requests.post(url, json=payload)
```

**JavaScript/Node.js:**
```javascript
const fetch = require('node-fetch');

const wledIP = '192.168.1.100';
const url = `http://${wledIP}/json`;

// Relais AN
fetch(url, {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ SwitchBot: { on: true } })
});

// Relais AUS
fetch(url, {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ SwitchBot: { on: false } })
});
```

## Status-Sync
- Der Usermod prüft den Relais-Status alle 60 Sekunden
- WLED bleibt synchron, auch wenn das Relais von außen gesteuert wird
- Status wird lokal gecacht für sofortige UI-Rückmeldung

## Fehlerbehandlung

**API Token Fehler:**
- Verifiziere den Token auf https://profile.switch-bot.com/api
- Token muss Berechtigung für Gerätekontrolle haben

**Device ID Fehler:**
- Stelle sicher, dass die Device ID exakt übereinstimmt
- Kopiere aus der SwitchBot App oder vom API Response

**Verbindungsprobleme:**
- Prüfe, ob der ESP32 Internetzugriff hat
- WLED Logs für Fehlermeldungen prüfen
- Firewall-Einstellungen überprüfen

## Tipps & Tricks

### Automatisierung mit WLED-Presets
Du kannst ein Preset erstellen, das auch das Relais steuert:
```json
{
  "SwitchBot": {"on": true},
  "bri": 255,
  "seg": [{"fx": 0}]
}
```

### Mehrere Relais steuern
Erstelle mehrere Instanzen des Usermods mit unterschiedlichen Device IDs (benötigt Code-Änderung)

## Lizenz
Wie WLED (MIT)

## Autor
WLED Community
