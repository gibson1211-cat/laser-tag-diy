// =============================================
// Laser Tag Turret - Full Arduino Sketch (ESP32)
// For Tyler (8) & Emmit (6) - Kid-friendly, safe
// June 2026 - Matches current wiring
// =============================================

#include <Servo.h>

// === Pin Definitions (from turret-wiring.md) ===
#define PAN_SERVO_PIN     13   // Parallax Feedback 360° Pan
#define TILT_SERVO_PIN    12   // 180° Tilt servo
#define PIR_PIN           14   // HC-SR501 (H/Repeat mode)
#define IR_RECEIVER_PIN   27   // Combined TSOP4838 receivers
#define AUDIO_PIN         25   // PAM8403 amp signal
#define STATUS_LED_PIN    32   // Optional hit/ready LED

// Servo objects
Servo panServo;
Servo tiltServo;

// Constants
const int IR_HIT_THRESHOLD = 50;   // Adjust based on testing
const int SWEEP_DELAY = 15;        // ms per step for smooth movement

// Variables
bool motionDetected = false;
unsigned long lastHitTime = 0;
unsigned long lastSweepTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Laser Tag Turret Starting...");

  // Pin modes
  pinMode(PIR_PIN, INPUT);
  pinMode(IR_RECEIVER_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(AUDIO_PIN, OUTPUT);

  // Attach servos (use buck converter for power!)
  panServo.attach(PAN_SERVO_PIN);
  tiltServo.attach(TILT_SERVO_PIN);
  
  // Center position on startup
  panServo.write(90);
  tiltServo.write(90);
  
  digitalWrite(STATUS_LED_PIN, HIGH);  // Ready indicator
  delay(1000);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  Serial.println("Turret Ready! PIR in H/Repeat mode active.");
}

void loop() {
  // Check PIR motion (H/Repeat mode)
  if (digitalRead(PIR_PIN) == HIGH) {
    if (!motionDetected) {
      motionDetected = true;
      Serial.println("Motion detected - Activating turret!");
      triggerAlertSound();
      sweepScan();
    }
  } else {
    motionDetected = false;
  }

  // Check for IR hits from guns
  if (digitalRead(IR_RECEIVER_PIN) == LOW) {  // TSOP4838 pulls LOW on signal
    handleHit();
  }

  // Idle slow scan when no motion
  if (millis() - lastSweepTime > 5000 && !motionDetected) {
    idleScan();
    lastSweepTime = millis();
  }

  delay(10);  // Small loop delay
}

// Sweep scan when motion triggered
void sweepScan() {
  for (int pos = 0; pos <= 180; pos += 3) {
    panServo.write(pos);
    delay(SWEEP_DELAY);
    checkForHitsDuringSweep();
  }
  for (int pos = 180; pos >= 0; pos -= 3) {
    panServo.write(pos);
    delay(SWEEP_DELAY);
    checkForHitsDuringSweep();
  }
}

// Slow idle movement
void idleScan() {
  int pos = 45 + (sin(millis() / 1000.0) * 45);  // Gentle oscillation
  panServo.write(constrain(pos, 30, 150));
  tiltServo.write(80);
}

// Handle IR hit
void handleHit() {
  if (millis() - lastHitTime > 800) {  // Debounce
    lastHitTime = millis();
    Serial.println("HIT DETECTED!");
    digitalWrite(STATUS_LED_PIN, HIGH);
    triggerHitSound();
    
    // Quick "reaction" tilt
    tiltServo.write(120);
    delay(300);
    tiltServo.write(80);
    
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}

// Basic audio feedback (square wave on PAM8403)
void triggerAlertSound() {
  for (int i = 0; i < 3; i++) {
    tone(AUDIO_PIN, 800, 150);
    delay(200);
    tone(AUDIO_PIN, 600, 100);
    delay(150);
  }
  noTone(AUDIO_PIN);
}

void triggerHitSound() {
  tone(AUDIO_PIN, 1200, 80);
  delay(100);
  tone(AUDIO_PIN, 800, 120);
  delay(150);
  noTone(AUDIO_PIN);
}

void checkForHitsDuringSweep() {
  if (digitalRead(IR_RECEIVER_PIN) == LOW) {
    handleHit();
  }
}