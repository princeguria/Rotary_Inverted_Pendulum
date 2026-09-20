
#define STEP_PIN   5
#define DIR_PIN    6

#define ENCODER_A  2
#define ENCODER_B  3

#define SLEEP_PIN  8
#define RESET_PIN  9

#define SPEED_LIMIT 40000

// --- USER SETTINGS ---
float TARGET_ANGLE = 180.0;
float Kp = 35.0;
float Ki = 0.015;
float Kd = 0.1;

const float CPR = 2400.0;

float prevError = 0;
float integral = 0;
unsigned long lastTime = 0;

volatile long encoderCounts = 0;
volatile int lastEncoded = 0;

volatile unsigned long stepInterval = 2000;
volatile unsigned long lastStepMicros = 0;
bool motorEnabled = false;


// -----------------------------------------------------
//                  ENCODER ISR (UNO VERSION)
// -----------------------------------------------------
void updateEncoder() {
  int MSB = (PIND & (1 << 2)) >> 2; 
  int LSB = (PIND & (1 << 3)) >> 3; 
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderCounts++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderCounts--;

  lastEncoded = encoded;
}


// -----------------------------------------------------
//          ULTRA FAST STEPPER SPEED SETTER
// -----------------------------------------------------
void setMotorSpeed(float speed) {

  if (speed >= 0) digitalWrite(DIR_PIN, HIGH);
  else digitalWrite(DIR_PIN, LOW);

  speed = abs(speed);

  if (speed < 1) speed = 1;
  if (speed > SPEED_LIMIT) speed = SPEED_LIMIT;

  stepInterval = 1000000.0 / speed;
}

void enableDriver(bool state) {
  if (state == motorEnabled) return; 
  
  if (state) {
    digitalWrite(SLEEP_PIN, HIGH);
    digitalWrite(RESET_PIN, HIGH);
    delayMicroseconds(10); 
    motorEnabled = true;
  } else {
    digitalWrite(SLEEP_PIN, LOW); 
    digitalWrite(RESET_PIN, LOW);
    motorEnabled = false;
  }
}


// -----------------------------------------------------
//                    SETUP
// -----------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  pinMode(SLEEP_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  
  enableDriver(false);

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), updateEncoder, CHANGE);

  lastTime = micros();
}


// -----------------------------------------------------
//                    MAIN LOOP
// -----------------------------------------------------
void loop() {

  // ---------------- PID timing ----------------
  unsigned long now = micros();
  if (now == lastTime) return; 
  
  float dt = (now - lastTime) / 1000000.0;
  lastTime = now;

  // ---------------- Angle ---------------------
  float angle = abs(fmod((encoderCounts / CPR) * 360.0, 360.0));

  // ---------------- SAFETY SHUTOFF ----------------
  if (angle < 120 || angle > 205) {
     enableDriver(false);
     
     integral = 0;
     prevError = 0;
     
     static unsigned long lastPrintFallen = 0;
     if (millis() - lastPrintFallen > 200) {
       Serial.print("FALLEN! Ang: "); Serial.println(angle, 2);
       lastPrintFallen = millis();
     }
     return; 
  }

  enableDriver(true);

  // ---------------- PID -----------------------
  float error = angle - TARGET_ANGLE;

  integral += error * dt;
  integral = constrain(integral, -5000, 5000);

  float derivative = (error - prevError) / dt;
  prevError = error;

  float pid = Kp * error + Ki * integral + Kd * derivative;
   
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 100) {
    Serial.print("Ang: ");
    Serial.print(angle, 2);      
    Serial.print(" | Err: ");
    Serial.println(error); 
    lastPrint = millis();
  }

  setMotorSpeed(pid);

  // -------------------------------------------------
  //      ULTRA-FAST NON-BLOCKING STEP GENERATOR
  // -------------------------------------------------
  if (motorEnabled) {
    unsigned long microsNow = micros();
    if (microsNow - lastStepMicros >= stepInterval) {
      lastStepMicros = microsNow;

      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(STEP_PIN, LOW);
    }
  }
}
