/**
 * FEATURES:
 * 1. 5-Second Mandatory Delay (Rule Compliant)
 * 2. Dual Strategy: SAFE (Default) vs AGGRESSIVE (Jumper on Pin 12)
 * 3. Predator Hunt: "Wiggle Cruise" instead of spinning
 * 4. Smart Edge Escape: Reverses until safe (Sensor-based)
 * 5. Attack Lock: Keeps charging if target is briefly lost
 */

// IMPORTANT: Your wiring is SWAPPED vs default:
//   LEFT wheel  -> B channel (BIN1/BIN2/PWMB)
//   RIGHT wheel -> A channel (AIN1/AIN2/PWMA)
//
// We keep the *function names* motorLeft/motorRight the same,
// but map their pins to match your physical robot.

// LEFT WHEEL (physically on B channel)
const int PIN_PWMA = 3;   // LEFT speed  (Driver PWMB)
const int PIN_AIN1 = 4;   // LEFT dir    (Driver BIN1)
const int PIN_AIN2 = 2;   // LEFT dir    (Driver BIN2)

// RIGHT WHEEL (physically on A channel)
const int PIN_PWMB = 9;   // RIGHT speed (Driver PWMA)
const int PIN_BIN1 = 8;   // RIGHT dir   (Driver AIN1)
const int PIN_BIN2 = 7;   // RIGHT dir   (Driver AIN2)

// Ultrasonic Sensor
const int PIN_TRIG = 6;   // white wire
const int PIN_ECHO = 10;  // orange/yellow wire

// IR Sensor (Downward center)
const int PIN_IR = 5;   // Black=1, White=0

// Strategy Selector (Jumper Wire)
const int PIN_STRAT = 12; // GND = Aggressive, Open = Safe

// Motor Direction Correction (Set true if wheel spins backwards)
bool INVERT_LEFT  = false;
bool INVERT_RIGHT = false;

// --- 2. TUNING PARAMETERS ---
struct Params {
  int spdCruise;    // Hunting speed
  int spdScanTurn;  // Wiggle intensity
  int spdApproach;  // Stalking speed
  int spdCharge;    // Kill speed
  int spdReverse;   // Escape speed
  int spdPivot;     // Turn speed
  int detectFar;    // Distance to start Stalking (cm)
  int chargeNear;   // Distance to start Charging (cm)
  int confEnter;    // Pings needed to confirm target
  int lockMs;       // Duration to keep charging after losing target
};

// SAFE PROFILE - Careful, validates targets
Params P_SAFE = { 120, 35, 165, 220, 180, 190, 40, 28, 3, 260 };

// AGGRESSIVE PROFILE - Fast, impulsive
Params P_AGG  = { 140, 45, 190, 255, 255, 210, 40, 20, 2, 350 };

Params P; // Active profile

// --- 3. STATE VARIABLES ---
enum Mode { MODE_EDGE, MODE_HUNT, MODE_APPROACH, MODE_CHARGE };
Mode mode = MODE_HUNT;

unsigned long tLast = 0;
unsigned long attackLockUntil = 0;
unsigned long tHuntFlip = 0;
int targetConf = 0;

bool edgeTurnRight = true;   // Alternates left/right escape
bool huntCurveRight = true;  // Alternates left/right cruise curve

const int IR_BLACK = 1;
const int IR_WHITE = 0;

// --- 4. MOTOR FUNCTIONS ---
void motorLeft(int speed) {
  if (INVERT_LEFT) speed = -speed;
  speed = constrain(speed, -255, 255);

  if (speed == 0) {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    analogWrite(PIN_PWMA, 0);
  } else if (speed > 0) {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
    analogWrite(PIN_PWMA, speed);
  } else {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, HIGH);
    analogWrite(PIN_PWMA, -speed);
  }
}

void motorRight(int speed) {
  if (INVERT_RIGHT) speed = -speed;
  speed = constrain(speed, -255, 255);

  if (speed == 0) {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
    analogWrite(PIN_PWMB, 0);
  } else if (speed > 0) {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
    analogWrite(PIN_PWMB, speed);
  } else {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, HIGH);
    analogWrite(PIN_PWMB, -speed);
  }
}

void driveLR(int left, int right) {
  motorLeft(left);
  motorRight(right);
}

void stopMotors() {
  driveLR(0, 0);
}

// --- 5. SENSOR FUNCTIONS ---
int readIR() {
  return digitalRead(PIN_IR);
}

int readUltrasonicCM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long dur = pulseIn(PIN_ECHO, HIGH, 25000);
  if (dur == 0) return 0;
  return (int)(dur / 58UL);
}

// --- 6. BEHAVIORS ---

void doEdgeRecover() {
  stopMotors();
  delay(20);

  driveLR(-P.spdReverse, -P.spdReverse);
  long t_start = millis();

  // Reverse while WHITE; stop when BLACK or timeout
  while (readIR() != IR_BLACK && (millis() - t_start < 1500)) {
    // keep reversing
  }

  delay(100);

  if (edgeTurnRight) driveLR(P.spdPivot, -P.spdPivot);
  else               driveLR(-P.spdPivot, P.spdPivot);

  delay(320);
  edgeTurnRight = !edgeTurnRight;

  driveLR(P.spdApproach, P.spdApproach);
  delay(140);

  targetConf = 0;
  attackLockUntil = 0;
  mode = MODE_HUNT;
}

void doHunt() {
  if (millis() - tHuntFlip > 1600UL) {
    tHuntFlip = millis();
    huntCurveRight = !huntCurveRight;
  }

  int wiggle = (int)((millis() / 90) % 2 == 0 ? P.spdScanTurn : -P.spdScanTurn);
  int baseL = P.spdCruise;
  int baseR = P.spdCruise;

  if (huntCurveRight) baseR -= 20;
  else baseL -= 20;

  driveLR(baseL + wiggle, baseR - wiggle);
}

void doApproach() {
  int wiggle = (int)((millis() / 70) % 2 == 0 ? (P.spdScanTurn + 20) : -(P.spdScanTurn + 20));
  driveLR(P.spdApproach + wiggle, P.spdApproach - wiggle);
}

void doCharge() {
  driveLR(P.spdCharge, P.spdCharge);
}

// --- 7. SETUP & LOOP ---
void setup() {
  Serial.begin(9600);

  pinMode(PIN_STRAT, INPUT_PULLUP);
  P = (digitalRead(PIN_STRAT) == LOW) ? P_AGG : P_SAFE;

  pinMode(PIN_PWMA, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);

  pinMode(PIN_PWMB, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_IR, INPUT);
  stopMotors();

  // Mandatory 5-second delay (blink LED 13)
  pinMode(13, OUTPUT);
  for (int i = 0; i < 5; i++) {
    digitalWrite(13, HIGH); delay(500);
    digitalWrite(13, LOW);  delay(500);
  }

  tLast = millis();
  tHuntFlip = millis();
}

void loop() {
  if (millis() - tLast < 30) return;
  tLast = millis();

  // A. EDGE CHECK (priority)
  if (readIR() != IR_BLACK) {
    mode = MODE_EDGE;
  }

  // B. TARGET LOGIC
  if (mode != MODE_EDGE) {
    int cm = readUltrasonicCM();
    bool valid = (cm >= 3 && cm <= 80);
    bool inFar  = valid && (cm <= P.detectFar);
    bool inNear = valid && (cm <= P.chargeNear);

    if (inFar) targetConf = min(targetConf + 1, 10);
    else       targetConf = max(targetConf - 1, 0);

    if (inFar && targetConf >= P.confEnter) {
      attackLockUntil = millis() + P.lockMs;
    }

    if (inNear) mode = MODE_CHARGE;
    else if (millis() < attackLockUntil) mode = MODE_CHARGE;
    else if (targetConf >= P.confEnter) mode = MODE_APPROACH;
    else mode = MODE_HUNT;
  }

  // C. EXECUTE
  switch (mode) {
    case MODE_EDGE:     doEdgeRecover(); break;
    case MODE_HUNT:     doHunt();        break;
    case MODE_APPROACH: doApproach();    break;
    case MODE_CHARGE:   doCharge();      break;
  }
}
