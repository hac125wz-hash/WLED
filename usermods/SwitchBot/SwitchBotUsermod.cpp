#include "SwitchBot_PM1_Cloud.h"

// Registriert den Usermod beim WLED-System, damit er mitkompiliert wird
static SwitchBotPM1Cloud switchbot_usermod;  // Changed: SwitchBot -> SwitchBotPM1Cloud
REGISTER_USERMOD(switchbot_usermod);
