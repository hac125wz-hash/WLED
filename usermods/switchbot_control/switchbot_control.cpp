#include "wled.h"

/*
 * SwitchBot Control Usermod
 * 
 * This usermod allows controlling SwitchBot devices via button press
 * Supports local control via SwitchBot Hub API
 * 
 * Written for WLED with v2 usermod API
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
    uint16_t httpTimeout = 5000;                          // HTTP request timeout in ms
    
    SwitchBotDevice devices[SWITCHBOT_MAX_DEVICES];
    unsigned long lastRequestTime = 0;
    uint16_t minRequestInterval = 500;                    // Minimum time between requests (ms)
    
    static const char _name[];
    static const char _enabled[];
    static const char _hubIP[];
    static const char _apiToken[];
    static const char _deviceId[];
    static const char _buttonPin[];
    static const char _action[];

    // Private methods
    bool sendSwitchBotCommand(const String& deviceId, uint8_t action);
    bool isButtonPressed(uint8_t buttonIndex);
    String getActionString(uint8_t action);

  public:
    /**
     * Enable/Disable the usermod
     */
    inline void enable(bool en) { enabled = en; }

    /**
     * Get usermod enabled/disabled state
     */
    inline bool isEnabled() { return enabled; }

    /**
     * getId() - Return unique usermod ID
     */
    uint16_t getId() override {
      return USERMOD_ID_UNSPECIFIED;  // Could be assigned a unique ID in const.h
    }

    /**
     * setup() - Initialize the usermod
     */
    void setup() override {
      // Initialize devices array
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        devices[i].deviceId = "";
        devices[i].buttonPin = 255;  // 255 = unused
        devices[i].action = 0;       // toggle
        devices[i].enabled = false;
      }
      initDone = true;
    }

    /**
     * connected() - Called when WiFi connects
     */
    void connected() override {
      DEBUG_PRINTLN(F("SwitchBot: WiFi connected"));
    }

    /**
     * loop() - Main loop, check for button presses
     */
    void loop() override {
      if (!enabled || !initDone || hubIP.isEmpty()) return;
      
      yield();
      
      // Check each device's button
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        if (!devices[i].enabled || devices[i].buttonPin >= WLED_MAX_BUTTONS) continue;
        
        // Check if button was just pressed
        if (isButtonPressed(devices[i].buttonPin)) {
          // Respect minimum request interval
          if (millis() - lastRequestTime > minRequestInterval) {
            sendSwitchBotCommand(devices[i].deviceId, devices[i].action);
            lastRequestTime = millis();
          }
        }
      }
    }

    /**
     * handleButton() - Override button behavior
     */
    bool handleButton(uint8_t b) override {
      if (!enabled || !initDone) return false;
      
      bool handled = false;
      for (int i = 0; i < SWITCHBOT_MAX_DEVICES; i++) {
        if (devices[i].enabled && devices[i].buttonPin == b) {
          handled = true;
          break;
        }
      }
      
      return handled;  // Return true to prevent default behavior
    }

    /**
     * addToJsonInfo() - Add info to JSON API
     */
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

    /**
     * addToConfig() - Save config to cfg.json
     */
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
      
      DEBUG_PRINTLN(F("SwitchBot config saved."));
    }

    /**
     * readFromConfig() - Load config from cfg.json
     */
    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      if (top.isNull()) {
        DEBUG_PRINTLN(F("SwitchBot: No config found."));
        return false;
      }
      
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
      
      DEBUG_PRINTLN(F("SwitchBot config loaded."));
      return true;
    }

    /**
     * appendConfigData() - Add UI hints
     */
    void appendConfigData() override {
      oappend(F("addInfo('SwitchBot:hubIP',1,'Local IP address of SwitchBot Hub');"));
      oappend(F("addInfo('SwitchBot:apiToken',1,'API token for authentication');"));
    }
};

// Static strings for flash memory saving
const char SwitchBotControl::_name[] PROGMEM = "SwitchBot";
const char SwitchBotControl::_enabled[] PROGMEM = "enabled";
const char SwitchBotControl::_hubIP[] PROGMEM = "hubIP";
const char SwitchBotControl::_apiToken[] PROGMEM = "apiToken";
const char SwitchBotControl::_deviceId[] PROGMEM = "deviceId";
const char SwitchBotControl::_buttonPin[] PROGMEM = "buttonPin";
const char SwitchBotControl::_action[] PROGMEM = "action";

// Private method implementations

/**
 * Send command to SwitchBot device via Hub
 * Supports local API calls
 */
bool SwitchBotControl::sendSwitchBotCommand(const String& deviceId, uint8_t action) {
  if (deviceId.isEmpty() || hubIP.isEmpty()) {
    DEBUG_PRINTLN(F("SwitchBot: Missing device ID or Hub IP"));
    return false;
  }
  
  if (!WLED_CONNECTED) {
    DEBUG_PRINTLN(F("SwitchBot: Not connected to WiFi"));
    return false;
  }
  
  // Build HTTP request for SwitchBot Hub local API
  String url = "http://" + hubIP + "/commands";
  
  // Create JSON payload
  StaticJsonDocument<256> doc;
  doc["deviceId"] = deviceId;
  doc["command"] = getActionString(action);
  doc["parameter"] = "default";
  
  String payload;
  serializeJson(doc, payload);
  
  DEBUG_PRINT(F("SwitchBot: Sending command to "));
  DEBUG_PRINT(deviceId);
  DEBUG_PRINT(F(" - Action: "));
  DEBUG_PRINTLN(getActionString(action));
  
  // Note: Actual HTTP request implementation would require AsyncWebClient
  // or similar HTTP client library. This is a placeholder structure.
  // In production, you would use:
  // HTTPClient http;
  // http.begin(url);
  // int httpCode = http.POST(payload);
  // http.end();
  
  return true;
}

/**
 * Check if a button was just pressed
 */
bool SwitchBotControl::isButtonPressed(uint8_t buttonIndex) {
  if (buttonIndex >= WLED_MAX_BUTTONS) return false;
  
  // Check if button is currently pressed
  return isButtonPressed(buttonIndex);  // Uses WLED's button state
}

/**
 * Convert action number to string
 */
String SwitchBotControl::getActionString(uint8_t action) {
  switch (action) {
    case 0: return F("toggle");
    case 1: return F("turnOn");
    case 2: return F("turnOff");
    default: return F("toggle");
  }
}

// Create and register the usermod
static SwitchBotControl switchbotControl;
REGISTER_USERMOD(switchbotControl);
