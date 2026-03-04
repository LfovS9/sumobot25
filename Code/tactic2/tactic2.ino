/*
 * ROBOT NAME: 20-SECOND TORNADO
 * STRATEGY:
 * 1. Wait 5 Seconds.
 * 2. Spin in a fixed circle for 20 Seconds (Safety Mode).
 * 3. Switch to Search & Destroy Mode (Attack Mode).
 */

// =============================================================
// 1. PIN DEFINITIONS (Your Specific Wiring)
// =============================================================

// LEFT MOTOR (Driver Channel B)
const int PIN_PWMA = 9;   
const int PIN_AIN1 = 8;   
const int PIN_AIN2 = 7;   

// RIGHT MOTOR (Driver Channel A)
const int PIN_PWMB = 3;   
const int PIN_BIN1 = 4;   
const int PIN_BIN2 = 2;   

// SENSORS
const int PIN_TRIG = 6;   
const int PIN_ECHO = 10;  
const int PIN_IR   = 5;   

// CORRECTION
bool INVERT_LEFT  = false; 
bool INVERT_RIGHT = false;

// =============================================================
// 2. TUNING SETTINGS
// =============================================================

// TIMING
// 5000ms (Start Delay) + 20000ms (Spin Time) = 25000ms Total
const unsigned long TIME_SWITCH_MODE = 25000; 

// PHASE 1: THE CIRCLE (First 20s)
// Make Right much slower than Left to create a tight circle.
int spdCircleL = 180; 
int spdCircleR = 40;   // <--- Decrease this to make the circle SMALLER

// PHASE 2: ATTACK SPEEDS (After 20s)
struct Params {
  int spdCruise; int spdScanTurn; int spdApproach; int spdCharge; 
  int spdReverse; int spdPivot; int detectFar; int chargeNear;
};
// Aggressive Profile
Params P = { 140, 45, 190, 255, 255, 210, 40, 20 };

// =============================================================
// 3. VARIABLES
// =============================================================

enum Mode { MODE_EDGE, MODE_PATROL, MODE_HUNT, MODE_APPROACH, MODE_CHARGE };
Mode mode = MODE_PATROL;

unsigned long attackLockUntil = 0;
unsigned long tHuntFlip = 0;
int targetConf = 0;
bool edgeTurnRight = true; 
bool huntCurveRight = true;

// =============================================================
// 4. MOTOR FUNCTIONS
// =============================================================

void motorLeft(int speed) {
  if (INVERT_LEFT) speed = -speed;
  speed = constrain(speed, -255, 255);
  if (speed == 0) {
    digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, LOW); analogWrite(PIN_PWMA, 0);
  } else if (speed > 0) {
    digitalWrite(PIN_AIN1, HIGH); digitalWrite(PIN_AIN2, LOW); analogWrite(PIN_PWMA, speed);
  } else {
    digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, HIGH); analogWrite(PIN_PWMA, -speed);
  }
}

void motorRight(int speed) {
  if (INVERT_RIGHT) speed = -speed;
  speed = constrain(speed, -255, 255);
  if (speed == 0) {
    digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, LOW); analogWrite(PIN_PWMB, 0);
  } else if (speed > 0) {
    digitalWrite(PIN_BIN1, HIGH); digitalWrite(PIN_BIN2, LOW); analogWrite(PIN_PWMB, speed);
  } else {
    digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, HIGH); analogWrite(PIN_PWMB, -speed);
  }
}

void driveLR(int left, int right) { motorLeft(left); motorRight(right); }
void stopMotors() { driveLR(0, 0); }

// =============================================================
// 5. SENSOR FUNCTIONS
// =============================================================

int readIR() { return digitalRead(PIN_IR); }

int readUltrasonicCM() {
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long dur = pulseIn(PIN_ECHO, HIGH, 15000); 
  if (dur == 0) return 0;
  return (int)(dur / 58UL);
}

// =============================================================
// 6. LOGIC & BEHAVIORS
// =============================================================

void doEdgeRecover() {
  stopMotors(); delay(20);
  driveLR(-P.spdReverse, -P.spdReverse);
  unsigned long t_start = millis();
  while (readIR() == 0 && (millis() - t_start < 1500)) {} // Wait for Black
  delay(100); 
  
  if (edgeTurnRight) driveLR(P.spdPivot, -P.spdPivot);
  else               driveLR(-P.spdPivot, P.spdPivot);
  delay(320); 
  edgeTurnRight = !edgeTurnRight;
  
  // Return to correct mode based on time
  if (millis() < TIME_SWITCH_MODE) mode = MODE_PATROL;
  else mode = MODE_HUNT;
}

void doHunt() {
  if (millis() - tHuntFlip > 1600UL) {
    tHuntFlip = millis(); huntCurveRight = !huntCurveRight;
  }
  int wiggle = (int)((millis() / 90) % 2 == 0 ? P.spdScanTurn : -P.spdScanTurn);
  int baseL = P.spdCruise; int baseR = P.spdCruise;
  if (huntCurveRight) baseR -= 20; else baseL -= 20;
  driveLR(baseL + wiggle, baseR - wiggle);
}

void doApproach() {
  int wiggle = (int)((millis() / 70) % 2 == 0 ? 55 : -55);
  driveLR(P.spdApproach + wiggle, P.spdApproach - wiggle);
}

void doCharge() { driveLR(P.spdCharge, P.spdCharge); }

// =============================================================
// 7. SETUP
// =============================================================
void setup() {
  pinMode(PIN_PWMA, OUTPUT); pinMode(PIN_AIN1, OUTPUT); pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_PWMB, OUTPUT); pinMode(PIN_BIN1, OUTPUT); pinMode(PIN_BIN2, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT); pinMode(PIN_ECHO, INPUT); pinMode(PIN_IR, INPUT);

  // 5-SECOND DELAY
  pinMode(13, OUTPUT);
  for(int i=0; i<5; i++) {
    digitalWrite(13, HIGH); delay(500);
    digitalWrite(13, LOW);  delay(500);
  }
  
  // Set Start Mode
  mode = MODE_PATROL;
}

// =============================================================
// 8. LOOP
// =============================================================
void loop() {
  
  // 1. ALWAYS CHECK EDGE (Safety First)
  if (readIR() == 0) { 
    doEdgeRecover(); 
    return; 
  }

  // 2. CHECK TIME
  if (millis() < TIME_SWITCH_MODE) {
    // === PHASE 1: FIRST 20 SECONDS (CIRCULAR MOTION) ===
    // Just drive in the circle. 
    // Note: If you want it to attack if enemy is SUPER close, uncomment below:
    /*
    int cm = readUltrasonicCM();
    if (cm > 0 && cm < 15) { doCharge(); return; }
    */
    
    driveLR(spdCircleL, spdCircleR);
    
  } else {
    // === PHASE 2: NORMAL BEHAVIOR (HUNT/ATTACK) ===
    
    int cm = readUltrasonicCM();
    bool valid = (cm >= 3 && cm <= 80);
    bool inFar  = valid && (cm <= P.detectFar);
    bool inNear = valid && (cm <= P.chargeNear);

    if (inFar) targetConf++; else targetConf--;
    targetConf = constrain(targetConf, 0, 10);
    
    if (inFar && targetConf >= 3) attackLockUntil = millis() + 350;

    if (inNear) mode = MODE_CHARGE;
    else if (millis() < attackLockUntil) mode = MODE_CHARGE;
    else if (targetConf >= 3) mode = MODE_APPROACH;
    else mode = MODE_HUNT;

    switch (mode) {
      case MODE_HUNT:     doHunt();     break;
      case MODE_APPROACH: doApproach(); break;
      case MODE_CHARGE:   doCharge();   break;
    }
  }
}