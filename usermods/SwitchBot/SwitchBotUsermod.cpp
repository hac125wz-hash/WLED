#include "SwitchBotUsermod.h"

SwitchBotUsermod::SwitchBotUsermod()
: enabled(true), apiToken(""), deviceId(""), pollInterval(60000), lastMillis(0), lastStatus("unknown")
{
}

void SwitchBotUsermod::setup() {
  // nothing to do here
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

  WiFiClientSecure *client = new WiFiClientSecure;
  HTTPClient https;
  client->setInsecure();

  String url = "https://api.switch-bot.com/v1.0/devices/" + deviceId + "/status";
  if (https.begin(*client, url)) {
	https.addHeader("Authorization", apiToken);
	int httpCode = https.GET();
	if (httpCode == 200) {
	  DynamicJsonDocument doc(2048);
	  String payload = https.getString();
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
  delete client;
}

void SwitchBotUsermod::addToJsonInfo(JsonObject &root) {
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");
  JsonArray arr = user.createNestedArray(F("SwitchBot"));
  arr.add(lastStatus);
  arr.add(F("status"));
}

void SwitchBotUsermod::readFromConfig(JsonObject &root) {
  JsonObject um = root["SwitchBot"];
  if (um.isNull()) return;
  apiToken = um["apiToken"] | apiToken;
  deviceId = um["deviceId"] | deviceId;
  pollInterval = um["pollInterval"] | pollInterval;
  enabled = um["enabled"] | enabled;
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
