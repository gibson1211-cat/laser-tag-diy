// =============================================
// Laser Tag Turret - Tag v0 (ESP32)
// Tyler (8) & Emmit (6) - IR + PIR only, no face tracking
// Pins match hardware/wiring/turret-wiring.md (repo source of truth)
// Hostname: laser-turret-N   OTA: 3232   MQTT: 192.168.20.249:1883
// Flash four units by changing TURRET_ID (1..4) before compile.
// =============================================

#include <ESP32Servo.h>
#include "tag_net.h"

#ifndef TURRET_ID
#define TURRET_ID 1
#endif

#if TURRET_ID < 1 || TURRET_ID > 4
#error TURRET_ID must be 1..4
#endif

// === Pin Definitions (from turret-wiring.md) ===
#define PAN_SERVO_PIN     13   // Parallax Feedback 360° Pan
#define TILT_SERVO_PIN    12   // 180° Tilt servo (strapping pin — see README)
#define PIR_PIN           14   // HC-SR501 (H/Repeat mode)
#define IR_RECEIVER_PIN   27   // Combined TSOP4838 receivers
#define AUDIO_PIN         25   // PAM8403 amp signal
#define STATUS_LED_PIN    32   // Optional hit/ready LED

#define START_LIVES       5
const int SWEEP_DELAY = 15;

Servo panServo;
Servo tiltServo;

bool motionDetected = false;
unsigned long lastHitTime = 0;
unsigned long lastSweepTime = 0;
int lives = START_LIVES;

static char hostname_buf[24];
static char device_id_buf[16];

static void pump() {
  tagNetLoop();
}

static void waitMs(unsigned int ms) {
  unsigned long end = millis() + ms;
  while ((long)(end - millis()) > 0) {
    pump();
    delay(5);
  }
}

static void alertSound() {
  if (!tagNetAudioAllowed()) {
    return;
  }
  for (int i = 0; i < 3; i++) {
    tone(AUDIO_PIN, 800, 150);
    waitMs(200);
    tone(AUDIO_PIN, 600, 100);
    waitMs(150);
  }
  noTone(AUDIO_PIN);
}

static void hitSound() {
  if (!tagNetAudioAllowed()) {
    return;
  }
  tone(AUDIO_PIN, 1200, 80);
  waitMs(100);
  tone(AUDIO_PIN, 800, 120);
  waitMs(150);
  noTone(AUDIO_PIN);
}

static void handleHit() {
  if (millis() - lastHitTime <= 800) {
    return;
  }
  lastHitTime = millis();
  if (lives > 0) {
    lives--;
  }
  Serial.printf("HIT DETECTED turret-%d lives=%d\n", TURRET_ID, lives);
  digitalWrite(STATUS_LED_PIN, HIGH);
  hitSound();
  tagNetOnHit("unknown", device_id_buf, lives);
  tiltServo.write(120);
  waitMs(300);
  tiltServo.write(80);
  digitalWrite(STATUS_LED_PIN, LOW);
}

static void checkForHitsDuringSweep() {
  if (digitalRead(IR_RECEIVER_PIN) == LOW) {
    handleHit();
  }
  pump();
}

static void sweepScan() {
  for (int pos = 0; pos <= 180; pos += 3) {
    panServo.write(pos);
    waitMs(SWEEP_DELAY);
    checkForHitsDuringSweep();
  }
  for (int pos = 180; pos >= 0; pos -= 3) {
    panServo.write(pos);
    waitMs(SWEEP_DELAY);
    checkForHitsDuringSweep();
  }
}

static void idleScan() {
  int pos = 45 + (int)(sin(millis() / 1000.0) * 45);
  panServo.write(constrain(pos, 30, 150));
  tiltServo.write(80);
}

void setup() {
  Serial.begin(115200);
  snprintf(hostname_buf, sizeof(hostname_buf), "laser-turret-%d", TURRET_ID);
  snprintf(device_id_buf, sizeof(device_id_buf), "turret-%d", TURRET_ID);
  Serial.printf("Laser Tag Turret Starting (Tag v0) id=%d host=%s\n", TURRET_ID, hostname_buf);

  pinMode(PIR_PIN, INPUT);
  pinMode(IR_RECEIVER_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(AUDIO_PIN, OUTPUT);

  panServo.attach(PAN_SERVO_PIN);
  tiltServo.attach(TILT_SERVO_PIN);
  panServo.write(90);
  tiltServo.write(90);

  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(400);
  digitalWrite(STATUS_LED_PIN, LOW);

  tagNetBegin(hostname_buf, device_id_buf, "turret");
  Serial.println("Turret Ready. PIR H/Repeat. No vision/event. IR works without Wi-Fi.");
}

void loop() {
  pump();
  digitalWrite(STATUS_LED_PIN, tagNetMqttConnected() ? HIGH : (millis() / 500) % 2);

  if (digitalRead(IR_RECEIVER_PIN) == LOW) {
    handleHit();
  }

  // Hunt only during play. Never follow faces (no vision/event).
  if (tagNetPlayEnabled() && digitalRead(PIR_PIN) == HIGH) {
    if (!motionDetected) {
      motionDetected = true;
      Serial.println("Motion (PIR) - sweep");
      alertSound();
      sweepScan();
    }
  } else {
    motionDetected = false;
  }

  if (tagNetPlayEnabled() && millis() - lastSweepTime > 5000 && !motionDetected) {
    idleScan();
    lastSweepTime = millis();
  }

  delay(5);
}
