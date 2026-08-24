#include "wled.h"
#include <HTTPClient.h>
#include <mbedtls/md.h> // Integrierte Krypto-Bibliothek für den Secret Key (HMAC-SHA256)

#ifndef SWITCHBOT_MAX_DEVICES
  #define SWITCHBOT_MAX_DEVICES 4
#endif

typedef struct switchbot_device_t {
  String deviceId;        
  uint8_t buttonPin;      
  uint8_t action;         // 0=toggle, 1=on, 2=off
  bool enabled;           
} SwitchBotDevice;

class SwitchBotControl : public Usermod {

  private:
    bool enabled = false;
    bool initDone = false;
    
    // Konfiguration erweitert um Secret Key
    String apiToken = "";                                 
    String secretKey = "";                                
    
    SwitchBotDevice devices[SWITCHBOT_MAX_DEVICES];
    unsigned long lastRequestTime = 0;
    uint16_t minRequestInterval = 2000; // 2 Sekunden Schutzabstand wegen Cloud-Latenz
    
    static const char _name[];
    static const char _enabled[];
    static const char _apiToken[];
    static const char _secretKey[];
    static const char _deviceId[];
    static const char _buttonPin[];
    static const char _action[];

    bool sendSwitchBotCommand(const String& deviceId, uint8_t action);
    String getActionString(uint8_t action);
    String generateSignature(const String& token, const String& secret, const String& t, const String& nonce);

  public:
    inline void enable(bool en) { enabled = en; }
    inline bool isEnabled() { return enabled; }

    uint16_t getId() override { return USERMOD_ID_UNSPECIFIED; }

    void setup() override {
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        devices[i].deviceId = "";
        devices[i].buttonPin = 255;  
        devices[i].action = 0;       
        devices[i].enabled = false;
      }
      initDone = true;
    }

    void connected() override {}
    void loop() override {}

    bool handleButton(uint8_t b) override {
      if (!enabled || !initDone || apiToken.isEmpty() || secretKey.isEmpty()) return false;
      
      bool handled = false;
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        if (devices[i].enabled && devices[i].buttonPin == b) {
          if (millis() - lastRequestTime > minRequestInterval) {
            sendSwitchBotCommand(devices[i].deviceId, devices[i].action);
            lastRequestTime = millis();
          }
          handled = true; 
          break;
        }
      }
      return handled; 
    }

    void addToJsonInfo(JsonObject& root) override {
      if (!enabled) return;
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");
      JsonArray info = user.createNestedArray(FPSTR(_name));
      info.add(F("SwitchBot"));
      int activeDevices = 0;
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        if (devices[i].enabled) activeDevices++;
      }
      info.add(String(activeDevices));
      info.add(F(" aktiv"));
    }

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      top[FPSTR(_apiToken)] = apiToken;
      top[FPSTR(_secretKey)] = secretKey;
      
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        String deviceName = F("device_");
        deviceName += i;
        JsonObject device = top.createNestedObject(deviceName);
        device[FPSTR(_deviceId)] = devices[i].deviceId;
        device[FPSTR(_buttonPin)] = devices[i].buttonPin;
        device[FPSTR(_action)] = devices[i].action;
        device["enabled"] = devices[i].enabled;
      }
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      if (top.isNull()) return false;
      
      enabled = top[FPSTR(_enabled)] | false;
      apiToken = top[FPSTR(_apiToken)] | "";
      secretKey = top[FPSTR(_secretKey)] | "";
      
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        String deviceName = F("device_");
        deviceName += i;
        JsonObject device = top[deviceName];
        
        if (!device.isNull()) {
          devices[i].deviceId = device[FPSTR(_deviceId)] | "";
          devices[i].buttonPin = device[FPSTR(_buttonPin)] | 255;
          devices[i].action = device[FPSTR(_action)] | 0;
          devices[i].enabled = device["enabled"] | false;
        }
      }
      return true;
    }

    void appendConfigData() override {
      oappend(F("addInfo('SwitchBot:apiToken',1,'Open API Token von SwitchBot');"));
      oappend(F("addInfo('SwitchBot:secretKey',1,'Developer Secret Key von SwitchBot');"));
    }
};

const char SwitchBotControl::_name[] PROGMEM = "SwitchBot";
const char SwitchBotControl::_enabled[] PROGMEM = "enabled";
const char SwitchBotControl::_apiToken[] PROGMEM = "apiToken";
const char SwitchBotControl::_secretKey[] PROGMEM = "secretKey";
const char SwitchBotControl::_deviceId[] PROGMEM = "deviceId";
const char SwitchBotControl::_buttonPin[] PROGMEM = "buttonPin";
const char SwitchBotControl::_action[] PROGMEM = "action";

/**
 * Berechnet die offizielle SwitchBot v1.1 HMAC-SHA256 Signatur
 */
String SwitchBotControl::generateSignature(const String& token, const String& secret, const String& t, const String& nonce) {
  String dataToSign = token + t + nonce;
  
  uint8_t hmacResult[32]; // Fehlerbehebung: Array-Größe explizit definiert
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret.c_str(), secret.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)dataToSign.c_str(), dataToSign.length());
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);
  
  // In Hex konvertieren (SwitchBot verlangt Großbuchstaben-Hex)
  String sign = "";
  for (int i = 0; i < 32; i++) {
    char buf[3];
    sprintf(buf, "%02X", hmacResult[i]);
    sign += buf;
  }
  return sign;
}

/**
 * Sendet den Befehl an die offizielle Cloud API v1.1
 */
bool SwitchBotControl::sendSwitchBotCommand(const String& deviceId, uint8_t action) {
  if (deviceId.isEmpty() || apiToken.isEmpty() || secretKey.isEmpty() || !WLED_CONNECTED) {
    return false;
  }
  
  String url = "https://switch-bot.com" + deviceId + "/commands";
  
  // Fehlerbehebung: Extrahiert die Sekunden (.sec) aus dem Toki-Objekt, um Rechenfehler zu vermeiden
  Toki::Time tm = toki.getTime();
  String t = String((unsigned long long)tm.sec * 1000ULL + tm.ms);
  if (tm.sec == 0) t = String(millis()); // Fallback falls kein NTP synchronisiert ist
  
  String nonce = "WLEDUserMod"; 
  String sign = generateSignature(apiToken, secretKey, t, nonce);
  
  StaticJsonDocument<256> doc;
  doc["command"] = getActionString(action);
  doc["parameter"] = "default";
  doc["commandType"] = "command";
  
  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin(url);
  
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiToken);
  http.addHeader("sign", sign);
  http.addHeader("t", t);
  http.addHeader("nonce", nonce);
  
  http.setTimeout(2000); 
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    DEBUG_PRINTF("SwitchBot API HTTP Code: %d\n", httpCode);
  } else {
    DEBUG_PRINTF("SwitchBot API Verbindungsfehler: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  return (httpCode == 200);
}

String SwitchBotControl::getActionString(uint8_t action) {
  switch (action) {
    case 0: return F("toggle"); 
    case 1: return F("turnOn");
    case 2: return F("turnOff");
    default: return F("toggle");
  }
}

static SwitchBotControl switchbotControl;
REGISTER_USERMOD(switchbotControl);
