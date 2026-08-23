#pragma once
#include "wled.h"

class SwitchBotUsermod : public Usermod {
private:
  bool enabled;
  String apiToken;
  String deviceId;
  unsigned long pollInterval;
  unsigned long lastMillis;
  String lastStatus;

  void fetchDeviceStatus();

public:
  SwitchBotUsermod();
  void setup() override;
  void loop() override;
  bool addToJsonInfo(JsonObject &root) override;
  void readFromConfig(JsonObject &root) override;
  void addToConfig(JsonObject &root) override;
};
