// =============================================
// Laser Tag Gun - Full Arduino Sketch (ESP32)
// For Tyler (8) & Emmit (6) - Kid-friendly, safe
// June 2026 - Matches current gun-wiring.md
// =============================================

#include <Arduino.h>

// === Pin Definitions (from gun-wiring.md) ===
#define IR_LED_PIN        26   // 4x IR LEDs via 2N2222 transistor base
#define IR_RECEIVER_PIN   27   // Combined TSOP4838 receivers
#define AUDIO_PIN         25   // PAM8403 amp signal
#define HIT_LED_PIN       32   // Hit indicator LEDs (PWM capable)
#define STATUS_LED_PIN    33   // Optional status / trigger LED

// Constants
const int FIRE_DEBOUNCE = 300;     // ms between shots
const int HIT_DEBOUNCE = 600;      // ms between registering hits

// Variables
unsigned long lastFireTime = 0;
unsigned long lastHitTime = 0;
bool isHit = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Laser Tag Gun Starting...");

  // Pin modes
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(IR_RECEIVER_PIN, INPUT);
  pinMode(AUDIO_PIN, OUTPUT);
  pinMode(HIT_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  digitalWrite(STATUS_LED_PIN, HIGH);  // Ready indicator
  delay(800);
  digitalWrite(STATUS_LED_PIN, LOW);

  Serial.println("Gun Ready! Trigger on GPIO for fire (or add button later).");
}

void loop() {
  // === Shooting Logic ===
  // For now: Trigger firing with a button on another pin or Serial command.
  // Quick test: Hold a wire from GPIO 0 (BOOT) to GND to "fire", or add a real trigger button later.
  if (digitalRead(0) == LOW && millis() - lastFireTime > FIRE_DEBOUNCE) {  
    fireLaser();
    lastFireTime = millis();
  }

  // Check for incoming hits from other guns / turret
  if (digitalRead(IR_RECEIVER_PIN) == LOW) {   // TSOP4838 pulls LOW on valid 38kHz signal
    registerHit();
  }

  delay(10);  // Small loop delay for responsiveness
}

// Fire the IR LEDs (4x via transistor)
void fireLaser() {
  Serial.println("PEW! Laser fired!");

  triggerFireSound();

  // Pulse IR LEDs multiple times for strong signal
  for (int i = 0; i < 8; i++) {
    digitalWrite(IR_LED_PIN, HIGH);
    delayMicroseconds(200);
    digitalWrite(IR_LED_PIN, LOW);
    delayMicroseconds(300);
  }

  // Visual feedback
  digitalWrite(HIT_LED_PIN, HIGH);  // Muzzle flash effect
  delay(80);
  digitalWrite(HIT_LED_PIN, LOW);
}

// Register being hit
void registerHit() {
  if (millis() - lastHitTime > HIT_DEBOUNCE) {
    lastHitTime = millis();
    Serial.println("HIT! You've been tagged!");

    isHit = true;
    digitalWrite(HIT_LED_PIN, HIGH);   // Bright hit indicator

    triggerHitSound();

    // Visual "hit reaction"
    for (int i = 0; i < 5; i++) {
      digitalWrite(HIT_LED_PIN, LOW);
      delay(80);
      digitalWrite(HIT_LED_PIN, HIGH);
      delay(80);
    }

    digitalWrite(HIT_LED_PIN, LOW);
    isHit = false;
  }
}

// Audio feedback via PAM8403
void triggerFireSound() {
  tone(AUDIO_PIN, 1800, 60);
  delay(70);
  tone(AUDIO_PIN, 1200, 90);
  delay(100);
  noTone(AUDIO_PIN);
}

void triggerHitSound() {
  tone(AUDIO_PIN, 400, 200);
  delay(220);
  tone(AUDIO_PIN, 200, 300);
  delay(320);
  noTone(AUDIO_PIN);
}