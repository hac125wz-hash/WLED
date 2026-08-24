#include "SwitchBotUsermod.h"
#include <HTTPClient.h>

SwitchBotUsermod::SwitchBotUsermod()
  : enabled(true), apiToken(""), deviceId(""), pollInterval(60000), lastMillis(0), lastStatus("unknown") {}

void SwitchBotUsermod::setup() {
  // nothing to do on setup
}

void SwitchBotUsermod::loop() {
  if (!enabled) return;
  if (millis() - lastMillis < pollInterval) return;
  lastMillis = millis();
  fetchDeviceStatus();
}

void SwitchBotUsermod::fetchDeviceStatus() {
  if (!WLED_CONNECTED) return;
  if (apiToken.length() == 0 || deviceId.length() == 0) return;

  HTTPClient https;
  String url = String("https://api.switch-bot.com/v1.0/devices/") + deviceId + "/status";
  if (https.begin(url)) {
    https.addHeader("Authorization", apiToken);
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      DynamicJsonDocument doc(2048);
      DeserializationError err = deserializeJson(doc, payload);
      if (!err) {
        JsonObject body = doc["body"];
        if (!body.isNull()) {
          if (body.containsKey("power")) lastStatus = String((const char*)body["power"]);
          else lastStatus = "ok";
        }
      }
    }
    https.end();
  }
}

void SwitchBotUsermod::sendCommand(const char* command) {
  if (!WLED_CONNECTED) return;
  if (apiToken.length() == 0 || deviceId.length() == 0) return;

  HTTPClient https;
  String url = String("https://api.switch-bot.com/v1.0/devices/") + deviceId + "/commands";
  
  if (https.begin(url)) {
    https.addHeader("Authorization", apiToken);
    https.addHeader("Content-Type", "application/json");
    
    // Payload erstellen
    DynamicJsonDocument doc(256);
    doc["command"] = command;
    doc["parameter"] = "default";
    doc["commandType"] = "command";
    
    String payload;
    serializeJson(doc, payload);
    
    int httpCode = https.POST(payload);
    if (httpCode == HTTP_CODE_OK) {
      DEBUG_PRINTLN(String("SwitchBot Command sent: ") + command);
      // Status aktualisieren
      lastStatus = command;
    } else {
      DEBUG_PRINTLN(String("SwitchBot Command failed: ") + httpCode);
    }
    https.end();
  }
}

void SwitchBotUsermod::addToJsonInfo(JsonObject &root) {
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");
  JsonArray arr = user.createNestedArray(F("SwitchBot"));
  arr.add(lastStatus);
  arr.add(F("status"));
}

void SwitchBotUsermod::addToJsonState(JsonObject &root) {
  JsonObject switchbot = root.createNestedObject(F("switchbot"));
  switchbot["power"] = lastStatus;
}

bool SwitchBotUsermod::readFromJsonState(JsonObject &root) {
  JsonObject switchbot = root["switchbot"];
  if (switchbot.isNull()) return false;
  
  if (switchbot.containsKey("power")) {
    String command = switchbot["power"];
    if (command == "on" || command == "On") {
      sendCommand("turnOn");
      return true;
    } else if (command == "off" || command == "Off") {
      sendCommand("turnOff");
      return true;
    }
  }
  return false;
}

bool SwitchBotUsermod::readFromConfig(JsonObject &root) {
  JsonObject um = root["SwitchBot"];
  if (um.isNull()) return false;
  apiToken = um["apiToken"] | apiToken;
  deviceId = um["deviceId"] | deviceId;
  pollInterval = um["pollInterval"] | pollInterval;
  enabled = um["enabled"] | enabled;
  return true;
}

void SwitchBotUsermod::addToConfig(JsonObject &root) {
  JsonObject um = root.createNestedObject("SwitchBot");
  um["apiToken"] = apiToken;
  um["deviceId"] = deviceId;
  um["pollInterval"] = pollInterval;
  um["enabled"] = enabled;
}

// register usermod
static SwitchBotUsermod switchbot_usermod;
REGISTER_USERMOD(switchbot_usermod);
