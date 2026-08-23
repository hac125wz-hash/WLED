#pragma once
#include "wled.h"

class SwitchBotUsermod : public Usermod {
private:
  bool enabled = true;
  String apiToken = "";
  String deviceId = "";
  unsigned long pollInterval = 60000; // ms
  unsigned long lastMillis = 0;
  String lastStatus = "unknown";

  void fetchDeviceStatus() {
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
		DeserializationError err = deserializeJson(doc, https.getString());
		if (!err) {
		  JsonObject body = doc["body"];
		  if (!body.isNull()) {
			// store a small summary in lastStatus
			if (body.containsKey("power")) lastStatus = String((const char*)body["power"]);
			else lastStatus = "ok";
		  }
		}
	  }
	  https.end();
	}
	delete client;
  }

public:
  void setup() override {
	// nothing to do here
  }

  void loop() override {
	if (!enabled) return;
	if (millis() - lastMillis < pollInterval) return;
	lastMillis = millis();
	fetchDeviceStatus();
  }

  void addToJsonInfo(JsonObject &root) override {
	JsonObject user = root["u"];
	if (user.isNull()) user = root.createNestedObject("u");
	JsonArray arr = user.createNestedArray(F("SwitchBot"));
	arr.add(lastStatus);
	arr.add(F("status"));
  }

  void readFromConfig(JsonObject &root) override {
	JsonObject um = root["SwitchBot"];
	if (um.isNull()) return;
	apiToken = um["apiToken"] | apiToken;
	deviceId = um["deviceId"] | deviceId;
	pollInterval = um["pollInterval"] | pollInterval;
	enabled = um["enabled"] | enabled;
  }

  void addToConfig(JsonObject &root) override {
	JsonObject um = root.createNestedObject("SwitchBot");
	um["apiToken"] = apiToken;
	um["deviceId"] = deviceId;
	um["pollInterval"] = pollInterval;
	um["enabled"] = enabled;
  }
};

// register usermod
static SwitchBotUsermod switchbot_usermod;
REGISTER_USERMOD(switchbot_usermod);
