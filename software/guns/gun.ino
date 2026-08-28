// =============================================
// Laser Tag Gun - Tag v0 (ESP32)
// Tyler (8) & Emmit (6) - IR only, kid-safe
// Pins match hardware/wiring/gun-wiring.md (repo source of truth)
// Hostname: laser-gun-1   OTA: 3232   MQTT: 192.168.20.249:1883
// =============================================

#include <Arduino.h>
#include "tag_net.h"

// === Pin Definitions (from gun-wiring.md) ===
#define IR_LED_PIN        26   // 4x IR LEDs via 2N2222 transistor base
#define IR_RECEIVER_PIN   27   // Combined TSOP4838 receivers
#define AUDIO_PIN         25   // PAM8403 amp signal
#define HIT_LED_PIN       32   // Hit indicator LEDs (PWM capable)
#define STATUS_LED_PIN    33   // Optional status / trigger LED

// Test trigger: GPIO 0 (BOOT). Not in the wiring table; hold-to-GND to fire.
// Do not hold at reset or the chip stays in download mode. See README.
#define TRIGGER_PIN       0

#define GUN_ID            "gun-1"
#define GUN_HOSTNAME      "laser-gun-1"
#define START_LIVES       5

const int FIRE_DEBOUNCE = 300;
const int HIT_DEBOUNCE = 600;

unsigned long lastFireTime = 0;
unsigned long lastHitTime = 0;
int lives = START_LIVES;

static void irCarrier(unsigned int duration_us) {
  unsigned long end = micros() + duration_us;
  while ((long)(end - micros()) > 0) {
    digitalWrite(IR_LED_PIN, HIGH);
    delayMicroseconds(13);  // ~38 kHz for TSOP4838
    digitalWrite(IR_LED_PIN, LOW);
    delayMicroseconds(13);
  }
}

static void fireSound() {
  if (!tagNetAudioAllowed()) {
    return;
  }
  tone(AUDIO_PIN, 1800, 60);
  delay(70);
  tone(AUDIO_PIN, 1200, 90);
  delay(100);
  noTone(AUDIO_PIN);
}

static void hitSound() {
  if (!tagNetAudioAllowed()) {
    return;
  }
  tone(AUDIO_PIN, 400, 200);
  delay(220);
  tone(AUDIO_PIN, 200, 300);
  delay(320);
  noTone(AUDIO_PIN);
}

static void fireLaser() {
  Serial.println("PEW! Laser fired!");
  fireSound();
  for (int i = 0; i < 8; i++) {
    irCarrier(600);
    delayMicroseconds(400);
  }
  digitalWrite(HIT_LED_PIN, HIGH);
  delay(80);
  digitalWrite(HIT_LED_PIN, LOW);
  tagNetOnFire();
}

static void registerHit() {
  if (millis() - lastHitTime <= HIT_DEBOUNCE) {
    return;
  }
  lastHitTime = millis();
  if (lives > 0) {
    lives--;
  }
  Serial.printf("HIT! lives=%d\n", lives);
  tagNetOnHit("unknown", GUN_ID, lives);

  digitalWrite(HIT_LED_PIN, HIGH);
  hitSound();
  for (int i = 0; i < 5; i++) {
    digitalWrite(HIT_LED_PIN, LOW);
    delay(80);
    digitalWrite(HIT_LED_PIN, HIGH);
    delay(80);
  }
  digitalWrite(HIT_LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Laser Tag Gun Starting (Tag v0)...");

  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(IR_RECEIVER_PIN, INPUT);
  pinMode(AUDIO_PIN, OUTPUT);
  pinMode(HIT_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  digitalWrite(IR_LED_PIN, LOW);

  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(400);
  digitalWrite(STATUS_LED_PIN, LOW);

  tagNetBegin(GUN_HOSTNAME, GUN_ID, "gun");
  Serial.println("Gun Ready. Trigger: GPIO 0 to GND. IR works without Wi-Fi.");
}

void loop() {
  tagNetLoop();

  digitalWrite(STATUS_LED_PIN, tagNetMqttConnected() ? HIGH : LOW);

  if (digitalRead(TRIGGER_PIN) == LOW && millis() - lastFireTime > FIRE_DEBOUNCE) {
    lastFireTime = millis();
    fireLaser();
  }

  if (digitalRead(IR_RECEIVER_PIN) == LOW) {
    registerHit();
  }

  delay(5);
}
