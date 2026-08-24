#include "wled.h"
#include <HTTPClient.h>
#include <mbedtls/md.h> // Integrierte Krypto-Bibliothek für den Secret Key (HMAC-SHA256)

class SwitchBotControl : public Usermod {

  private:
    bool enabled = false;
    bool initDone = false;
    
    // Globale API Einstellungen
    String apiToken = "";                                 
    String secretKey = "";                                
    String deviceId = "";
    uint8_t action = 0;      // Standard-Aktion: 0=toggle, 1=on, 2=off

    unsigned long lastRequestTime = 0;
    uint16_t minRequestInterval = 2000; // 2 Sekunden Schutzabstand wegen Cloud-Latenz
    
    static const char _name[];
    static const char _enabled[];
    static const char _apiToken[];
    static const char _secretKey[];
    static const char _deviceId[];
    static const char _action[];

    bool sendSwitchBotCommand(uint8_t actionToSnd);
    String getActionString(uint8_t actionToSnd);
    String generateSignature(const String& token, const String& secret, const String& t, const String& nonce);

  public:
    inline void enable(bool en) { enabled = en; }
    inline bool isEnabled() { return enabled; }

    uint16_t getId() override { return USERMOD_ID_UNSPECIFIED; }

    void setup() override {
      initDone = true;
    }

    void connected() override {}
    void loop() override {}

    /**
     * HIER WIRD DIE HTML-ANFRAGE ABGEFANGEN
     * WLED-konforme Funktionssignatur mit zwei Parametern (request & subPage)
     * Aufrufbar über: http://<ESP-IP>/win&SB=1 (Standard-Aktion)
     * Oder gezielt:  http://<ESP-IP>/win&SB=on / off / toggle
     */
    void handleHttpGet(AsyncWebServerRequest *request, const String& subPage) override {
      if (!enabled || !initDone || apiToken.isEmpty() || secretKey.isEmpty() || deviceId.isEmpty()) return;
      
      // Prüfen, ob der "SB" Parameter in der URL vorkommt
      if (request->hasParam("SB")) {
        String reqParam = request->getParam("SB")->value();
        uint8_t targetAction = action; // Fallback auf Standard-Einstellung

        // Parameter auswerten, falls via URL übergeben
        if (reqParam == "on" || reqParam == "1")        targetAction = 1;
        else if (reqParam == "off" || reqParam == "2")  targetAction = 2;
        else if (reqParam == "toggle" || reqParam == "0") targetAction = 0;

        // Spam-Schutz
        if (millis() - lastRequestTime > minRequestInterval) {
          sendSwitchBotCommand(targetAction);
          lastRequestTime = millis();
          request->send(200, "text/plain", "SwitchBot Command Sent");
        } else {
          request->send(429, "text/plain", "Rate Limit - Please Wait");
        }
      }
    }

    void addToJsonInfo(JsonObject& root) override {
      if (!enabled) return;
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");
      JsonArray info = user.createNestedArray(FPSTR(_name));
      info.add(F("SwitchBot Link"));
      info.add(F("/win&SB=1"));
    }

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      top[FPSTR(_apiToken)] = apiToken;
      top[FPSTR(_secretKey)] = secretKey;
      top[FPSTR(_deviceId)] = deviceId;
      top[FPSTR(_action)] = action;
    }

    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      if (top.isNull()) return false;
      
      enabled = top[FPSTR(_enabled)] | false;
      apiToken = top[FPSTR(_apiToken)] | "";
      secretKey = top[FPSTR(_secretKey)] | "";
      deviceId = top[FPSTR(_deviceId)] | "";
      action = top[FPSTR(_action)] | 0;
      
      return true;
    }

    void appendConfigData() override {
      oappend(F("addInfo('SwitchBot:apiToken',1,'Open API Token');"));
      oappend(F("addInfo('SwitchBot:secretKey',1,'Developer Secret Key');"));
      oappend(F("addInfo('SwitchBot:deviceId',1,'SwitchBot Device ID');"));
    }
};

const char SwitchBotControl::_name[] PROGMEM = "SwitchBot";
const char SwitchBotControl::_enabled[] PROGMEM = "enabled";
const char SwitchBotControl::_apiToken[] PROGMEM = "apiToken";
const char SwitchBotControl::_secretKey[] PROGMEM = "secretKey";
const char SwitchBotControl::_deviceId[] PROGMEM = "deviceId";
const char SwitchBotControl::_action[] PROGMEM = "action";

String SwitchBotControl::generateSignature(const String& token, const String& secret, const String& t, const String& nonce) {
  String dataToSign = token + t + nonce;
  
  uint8_t hmacResult[32]; // Fix: Echte Array-Größe für mbedtls zugewiesen
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret.c_str(), secret.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)dataToSign.c_str(), dataToSign.length());
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);
  
  String sign = "";
  for (int i = 0; i < 32; i++) {
    char buf[3]; // Fix: Ausreichend Platz für 2 Hex-Zeichen + Null-Terminator
    snprintf(buf, sizeof(buf), "%02X", hmacResult[i]);
    sign += buf;
  }
  return sign;
}

bool SwitchBotControl::sendSwitchBotCommand(uint8_t actionToSnd) {
  if (deviceId.isEmpty() || apiToken.isEmpty() || secretKey.isEmpty() || !WLED_CONNECTED) {
    return false;
  }
  
  String url = "https://switch-bot.com" + deviceId + "/commands";
  
  Toki::Time tm = toki.getTime();
  String t = String((unsigned long long)tm.sec * 1000ULL + tm.ms);
  if (tm.sec == 0) t = String(millis()); 
  
  String nonce = "WLEDUserMod"; 
  String sign = generateSignature(apiToken, secretKey, t, nonce);
  
  StaticJsonDocument<256> doc;
  doc["command"] = getActionString(actionToSnd);
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

String SwitchBotControl::getActionString(uint8_t actionToSnd) {
  switch (actionToSnd) {
    case 0: return F("toggle"); 
    case 1: return F("turnOn");
    case 2: return F("turnOff");
    default: return F("toggle");
  }
}

static SwitchBotControl switchbotControl;
REGISTER_USERMOD(switchbotControl); // Fix: REGISTER_USERMOD korrigiert
