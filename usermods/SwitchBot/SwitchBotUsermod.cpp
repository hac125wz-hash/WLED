#include "SwitchBotUsermod.h"

// Registriert den Usermod beim WLED-System, damit er mitkompiliert wird
static SwitchBotPM1Cloud switchbot_pm1_usermod;
REGISTER_USERMOD(switchbot_pm1_usermod);
