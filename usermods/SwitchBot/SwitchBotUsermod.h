#pragma once
#include "wled.h"

class SwitchBotUsermod : public Usermod {
private:
  bool enabled = true;
  String apiToken = "";
  String deviceId = "";
  uint32_t pollInterval = 60000; // ms
  uint32_t lastMillis = 0;
  String lastStatus = "unknown";

  void fetchDeviceStatus();
  void sendCommand(const char* command);

public:
  SwitchBotUsermod();
  void setup() override;
  void loop() override;
  void addToJsonInfo(JsonObject &root) override;
  bool readFromConfig(JsonObject &root) override;
  void addToConfig(JsonObject &root) override;
  void addToJsonState(JsonObject &root) override;
  bool readFromJsonState(JsonObject &root) override;
};
