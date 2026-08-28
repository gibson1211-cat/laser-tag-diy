#pragma once
// Tag v0: Wi-Fi + ArduinoOTA (3232) + MQTT (tag/event, tag/match, ha/cmd, ha/state).
// Local IR must keep working if Wi-Fi or MQTT is down.
// Turret firmware must never subscribe to vision/event (no face tracking).

#include <Arduino.h>

// Call from setup() after Serial.begin. hostname e.g. "laser-gun-1".
void tagNetBegin(const char *hostname, const char *device_id, const char *kind);

// Call every loop() iteration, and inside long servo waits.
void tagNetLoop();

// Fire / hit publishers (no-op if MQTT is down).
void tagNetOnFire();
void tagNetOnHit(const char *shooter_id, const char *target_id, int lives);

// Match / HA play gate. Default true until MQTT says otherwise (bench).
// Night hours (20:00-08:00 America/New_York, NTP) force audio off.
bool tagNetPlayEnabled();
bool tagNetAudioAllowed();
bool tagNetMqttConnected();
bool tagNetWifiConnected();

const char *tagNetDeviceId();
const char *tagNetHostname();
const char *tagNetMatchId();  // may be "null" string when unknown
