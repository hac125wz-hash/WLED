
#include "SwitchBotUsermod.h"
#if defined(ESP32)
#include <WiFiClientSecure.h>
#elif defined(ESP8266)
#include <WiFiClientSecureBearSSL.h>
#endif
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

#if defined(ESP32)
  WiFiClientSecure client;
  client.setInsecure();
  if (https.begin(client, url)) {
#elif defined(ESP8266)
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  if (https.begin(client, url)) {
#else
  // fallback: try without secure client
  if (https.begin(url)) {
#endif
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

void SwitchBotUsermod::addToJsonInfo(JsonObject &root) {
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");
  JsonArray arr = user.createNestedArray(F("SwitchBot"));
  arr.add(lastStatus);
  arr.add(F("status"));
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
