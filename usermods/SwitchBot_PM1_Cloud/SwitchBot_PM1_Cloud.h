#pragma once
/*
 * Usermod: SwitchBot PM1 Cloud Control
 * Version: 1.0.0
 * Description: Control a SwitchBot PM1 Smart Plug via Cloud API
 * Author: WLED Community
 * 
 * This usermod allows you to control a SwitchBot PM1 relay via WLED's UI and JSON API.
 * The PM1 is controlled through the official SwitchBot Cloud API.
 * 
 * Setup:
 * 1. Get your SwitchBot API Token from: https://profile.switch-bot.com/api
 * 2. Get your PM1 Device ID (visible in SwitchBot app or via API)
 * 3. Configure in WLED settings or via platformio_override.usermods.ini:
 *    -D SWITCHBOT_API_TOKEN="your_token_here"
 *    -D SWITCHBOT_DEVICE_ID="your_device_id_here"
 */

#include "wled.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

class SwitchBotPM1Cloud : public Usermod {
private:
  static const char _name[];
  static const char _enabled[];
  static const char _apiToken[];
  static const char _deviceId[];

  bool enabled = true;
  bool relayState = false;  // Current state of the relay
  unsigned long lastUpdate = 0;
  unsigned long lastStatusCheck = 0;
  
  #ifdef SWITCHBOT_API_TOKEN
    String apiToken = SWITCHBOT_API_TOKEN;
  #else
    String apiToken = "";
  #endif
  
  #ifdef SWITCHBOT_DEVICE_ID
    String deviceId = SWITCHBOT_DEVICE_ID;
  #else
    String deviceId = "";
  #endif

  const char* API_HOST = "api.switch-bot.com";
  const uint16_t statusCheckInterval = 60000;  // Check status every 60 seconds

  // Helper function to send command to SwitchBot API
  bool sendCommand(const char* command) {
    if (apiToken.isEmpty() || deviceId.isEmpty()) {
      DEBUG_PRINTLN(F("SwitchBot: API Token or Device ID not configured"));
      return false;
    }

    HTTPClient http;
    String url = "https://api.switch-bot.com/v1.0/devices/" + deviceId + "/commands";
    
    http.begin(url);
    http.addHeader("Authorization", apiToken);
    http.addHeader("Content-Type", "application/json");

    // Create JSON payload
    StaticJsonDocument<256> doc;
    doc["command"] = command;
    doc["parameter"] = "default";
    doc["commandType"] = "command";

    String payload;
    serializeJson(doc, payload);

    DEBUG_PRINT(F("SwitchBot: Sending command: "));
    DEBUG_PRINTLN(command);

    int httpResponseCode = http.POST(payload);
    
    if (httpResponseCode == 200) {
      relayState = (strcmp(command, "turnOn") == 0);
      lastUpdate = millis();
      DEBUG_PRINTLN(F("SwitchBot: Command sent successfully"));
      http.end();
      return true;
    } else {
      DEBUG_PRINT(F("SwitchBot: Error code: "));
      DEBUG_PRINTLN(httpResponseCode);
      DEBUG_PRINTLN(http.getString());
      http.end();
      return false;
    }
  }

  // Get current relay status from API
  bool updateStatus() {
    if (apiToken.isEmpty() || deviceId.isEmpty()) {
      return false;
    }

    HTTPClient http;
    String url = "https://api.switch-bot.com/v1.0/devices/" + deviceId + "/status";
    
    http.begin(url);
    http.addHeader("Authorization", apiToken);

    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
      String response = http.getString();
      
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, response);
      
      if (!error) {
        JsonObject body = doc["body"];
        if (body.containsKey("power")) {
          String power = body["power"];
          relayState = (power == "on");
          lastStatusCheck = millis();
          DEBUG_PRINT(F("SwitchBot: Relay state updated: "));
          DEBUG_PRINTLN(relayState ? "ON" : "OFF");
          http.end();
          return true;
        }
      }
    }
    
    http.end();
    return false;
  }

public:
  void setup() {
    DEBUG_PRINTLN(F("SwitchBot PM1 Cloud Control initialized"));
    updateStatus();
  }

  void loop() {
    // Periodically check status from API
    if (millis() - lastStatusCheck > statusCheckInterval) {
      updateStatus();
    }
  }

  // Turn relay ON
  void turnOn() {
    sendCommand("turnOn");
  }

  // Turn relay OFF
  void turnOff() {
    sendCommand("turnOff");
  }

  // Toggle relay state
  void toggle() {
    if (relayState) {
      turnOff();
    } else {
      turnOn();
    }
  }

  // Get current relay state
  bool getRelayState() {
    return relayState;
  }

  // Handle JSON API calls
  void handleJsonRequest(JsonObject& root) {
    JsonObject usermod = root["SwitchBot"];
    if (usermod) {
      if (usermod.containsKey("on")) {
        bool state = usermod["on"];
        if (state) {
          turnOn();
        } else {
          turnOff();
        }
      }
      if (usermod.containsKey("toggle")) {
        toggle();
      }
    }
  }

  // Read configuration from JSON
  bool readFromConfig(JsonObject& root) {
    bool changed = false;

    JsonObject usermod = root["SwitchBot"];
    if (usermod) {
      if (usermod["enabled"].is<bool>()) {
        enabled = usermod["enabled"];
        changed = true;
      }
      if (usermod["apiToken"].is<String>()) {
        String newToken = usermod["apiToken"];
        if (newToken.length() > 0 && newToken != apiToken) {
          apiToken = newToken;
          changed = true;
        }
      }
      if (usermod["deviceId"].is<String>()) {
        String newId = usermod["deviceId"];
        if (newId.length() > 0 && newId != deviceId) {
          deviceId = newId;
          changed = true;
        }
      }
    }
    return changed;
  }

  // Add configuration to JSON
  void addToConfig(JsonObject& root) {
    JsonObject usermod = root.createNestedObject("SwitchBot");
    usermod["enabled"] = enabled;
    usermod["apiToken"] = apiToken;
    usermod["deviceId"] = deviceId;
  }

  uint16_t getId() {
    return USERMOD_ID_SWITCHBOT_PM1;
  }

  // Enable/Disable usermod
  inline void enable(bool en) { enabled = en; }
  inline bool isEnabled() { return enabled; }
};

const char SwitchBotPM1Cloud::_name[] = "SwitchBot PM1";
const char SwitchBotPM1Cloud::_enabled[] = "enabled";
const char SwitchBotPM1Cloud::_apiToken[] = "apiToken";
const char SwitchBotPM1Cloud::_deviceId[] = "deviceId";
