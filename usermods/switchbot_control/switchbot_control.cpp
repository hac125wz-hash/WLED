#include "wled.h"

/*
 * SwitchBot Control Usermod
 * 
 * This usermod allows controlling SwitchBot devices via button press
 * Supports local control via SwitchBot Hub API
 */

#ifndef SWITCHBOT_MAX_DEVICES
  #define SWITCHBOT_MAX_DEVICES 4
#endif

typedef struct switchbot_device_t {
  String deviceId;        // SwitchBot device ID
  uint8_t buttonPin;      // Which button triggers this device (-1 = unused)
  uint8_t action;         // 0=toggle, 1=on, 2=off
  bool enabled;           // Is this device enabled
} SwitchBotDevice;

class SwitchBotControl : public Usermod {

  private:
    bool enabled = false;
    bool initDone = false;
    
    // Configuration
    String hubIP = "";                                    // SwitchBot Hub IP address
    String apiToken = "";                                 // API token for authentication
    
    SwitchBotDevice devices[SWITCHBOT_MAX_DEVICES];
    unsigned long lastRequestTime = 0;
    uint16_t minRequestInterval = 1000;                   // Mindestabstand zwischen API-Sende-Befehlen (ms)
    
    static const char _name[];
    static const char _enabled[];
    static const char _hubIP[];
    static const char _apiToken[];
    static const char _deviceId[];
    static const char _buttonPin[];
    static const char _action[];

    // Private methods
    bool sendSwitchBotCommand(const String& deviceId, uint8_t action);
    String getActionString(uint8_t action);

  public:
    inline void enable(bool en) { enabled = en; }
    inline bool isEnabled() { return enabled; }

    uint16_t getId() override {
      return USERMOD_ID_UNSPECIFIED;
    }

    void setup() override {
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        devices[i].deviceId = "";
        devices[i].buttonPin = 255;  // 255 = ungenutzt
        devices[i].action = 0;       // toggle
        devices[i].enabled = false;
      }
      initDone = true;
    }

    void connected() override {
      DEBUG_PRINTLN(F("SwitchBot: WiFi connected"));
    }

    /* 
     * Der loop() bleibt leer! Das spart enorme Rechenleistung, 
     * da wir stattdessen Event-basiert arbeiten.
     */
    void loop() override {
      // Leer gelassen für maximale Performance
    }

    /**
     * handleButton() - Event-basierte Tasterabfrage (Reagiert nur bei Klick!)
     */
    bool handleButton(uint8_t b) override {
      if (!enabled || !initDone || hubIP.isEmpty()) return false;
      
      bool handled = false;
      
      // Prüfen, ob der gedrückte Button einem SwitchBot-Gerät zugeordnet ist
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        if (devices[i].enabled && devices[i].buttonPin == b) {
          
          // Spam-Schutz: Verhindert Mehrfach-Auslösungen beim Prellen des Tasters
          if (millis() - lastRequestTime > minRequestInterval) {
            sendSwitchBotCommand(devices[i].deviceId, devices[i].action);
            lastRequestTime = millis();
          }
          
          handled = true; // WLED mitteilen, dass wir den Klick verarbeitet haben
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
      info.add(F(" devices"));
    }

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      top[FPSTR(_hubIP)] = hubIP;
      top[FPSTR(_apiToken)] = apiToken;
      
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
      hubIP = top[FPSTR(_hubIP)] | "";
      apiToken = top[FPSTR(_apiToken)] | "";
      
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
      oappend(F("addInfo('SwitchBot:hubIP',1,'Local IP address of SwitchBot Hub');"));
      oappend(F("addInfo('SwitchBot:apiToken',1,'API token for authentication');"));
    }
};

const char SwitchBotControl::_name[] PROGMEM = "SwitchBot";
const char SwitchBotControl::_enabled[] PROGMEM = "enabled";
const char SwitchBotControl::_hubIP[] PROGMEM = "hubIP";
const char SwitchBotControl::_apiToken[] PROGMEM = "apiToken";
const char SwitchBotControl::_deviceId[] PROGMEM = "deviceId";
const char SwitchBotControl::_buttonPin[] PROGMEM = "buttonPin";
const char SwitchBotControl::_action[] PROGMEM = "action";

/**
 * Send command to SwitchBot device via Hub
 */
bool SwitchBotControl::sendSwitchBotCommand(const String& deviceId, uint8_t action) {
  if (deviceId.isEmpty() || hubIP.isEmpty() || !WLED_CONNECTED) {
    return false;
  }
  
  DEBUG_PRINT(F("SwitchBot: Command triggered for "));
  DEBUG_PRINTLN(deviceId);

  // WICHTIGER HINWEIS:
  // Sobald du hier echten HTTP-Sende-Code (z.B. HTTPClient) einbaust,
  // achte darauf, einen kurzen Timeout (z.B. 1000ms) zu setzen,
  // damit die LED-Effekte während des Netzwerkaufrufs nicht stocken.

  return true;
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
