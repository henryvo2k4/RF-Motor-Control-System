#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>
#include <Servo.h>

#define EN_PIN 8

#define STEP_X 2
#define DIR_X 5

#define STEP_Y 3
#define DIR_Y 6

#define STEP_Z 4
#define DIR_Z 7

RF24 radio(49, 53);  // CE, CSN
const byte address[6] = "00001";
char receivedText[32];

int xA = 512, yA = 512;
int xB = 512, yB = 512;

unsigned long lastStepX = 0;
unsigned long lastStepY = 0;
unsigned long lastStepZ = 0;

unsigned long lastMoveX = 0;
unsigned long lastMoveY = 0;
unsigned long lastMoveZ = 0;

bool savedX = true;
bool savedY = true;
bool savedZ = true;

const int stepInterval = 500;  // microseconds
bool stepStateX = false, stepStateY = false, stepStateZ = false;

float positionX = 0;
float positionY = 0;
float positionZ = 0;

const float stepsPerDegree = 1.0;
// const float maxDegX = 180.0*99.5;
// const float maxDegY = 180.0*99.5;
// const float maxDegZ = 180.0*99.5;
// const float minDeg = 0.0;

// EEPROM Address
#define EEPROM_ADDR_X      0
#define EEPROM_ADDR_Y      (EEPROM_ADDR_X + sizeof(float))
#define EEPROM_ADDR_Z      (EEPROM_ADDR_Y + sizeof(float))
#define EEPROM_FLAG_ADDR   (EEPROM_ADDR_Z + sizeof(float))
#define EEPROM_ADDR_SERVO  (EEPROM_FLAG_ADDR + sizeof(byte))
#define EEPROM_MAGIC       0xA5

// ==== Servo control ====
Servo myServo;
int currentAngle = 90;
unsigned long lastStepServo = 0;
unsigned long lastMoveServo = 0;
bool savedServo = true;
const int servoStepInterval = 20 * 1000; // µs giữa các bước servo (20ms)
const int servoStepSize = 1; // mỗi lần tăng/giảm 1°

void setup() {
  Serial.begin(9600);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.startListening();
  radio.setPALevel(RF24_PA_LOW);
  Serial.println("Receiver started...");

  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);  // Enable driver

  pinMode(STEP_X, OUTPUT); pinMode(DIR_X, OUTPUT);
  pinMode(STEP_Y, OUTPUT); pinMode(DIR_Y, OUTPUT);
  pinMode(STEP_Z, OUTPUT); pinMode(DIR_Z, OUTPUT);

  // // EEPROM load
  // byte flag;
  // EEPROM.get(EEPROM_FLAG_ADDR, flag);
  // if (flag == EEPROM_MAGIC) {
  //   EEPROM.get(EEPROM_ADDR_X, positionX);
  //   EEPROM.get(EEPROM_ADDR_Y, positionY);
  //   EEPROM.get(EEPROM_ADDR_Z, positionZ);
  //   EEPROM.get(EEPROM_ADDR_SERVO, currentAngle);
  //   Serial.println("EEPROM data valid.");
  // } else {
  //   positionX = positionY = positionZ = 0;
  //   currentAngle = 90;
  //   Serial.println("EEPROM data invalid. Resetting positions to 0.");
  // }

  Serial.print("Restored positions - X: ");
  Serial.print(positionX, 2);
  Serial.print(" | Y: ");
  Serial.print(positionY, 2);
  Serial.print(" | Z: ");
  Serial.print(positionZ, 2);
  Serial.print(" | Servo: ");
  Serial.println(currentAngle);

  // Servo init
  myServo.attach(24);
  myServo.write(currentAngle);
}

void loop() {
  if (radio.available()) {
    radio.read(&receivedText, sizeof(receivedText));
    Serial.print("Received: ");
    Serial.println(receivedText);

    parseData(receivedText, xA, yA, xB, yB);
  }

  unsigned long now = micros();

  // Trục X
  if ((yB < 400 || yB > 600) && now - lastStepX >= stepInterval) {
    if (yB > 600 ) {
      digitalWrite(DIR_X, LOW);
      doStep(STEP_X, stepStateX);
      positionX += stepsPerDegree;
      lastStepX = now;
      lastMoveX = millis();
      savedX = false;
    } else if (yB < 400 ) {
      digitalWrite(DIR_X, HIGH);
      doStep(STEP_X, stepStateX);
      positionX -= stepsPerDegree;
      lastStepX = now;
      lastMoveX = millis();
      savedX = false;
    }
  }

  // Trục Y
  if ((xB < 400 || xB > 600) && now - lastStepY >= stepInterval) {
    if (xB > 600 ) {
      digitalWrite(DIR_Y, HIGH);
      doStep(STEP_Y, stepStateY);
      positionY += stepsPerDegree;
      lastStepY = now;
      lastMoveY = millis();
      savedY = false;
    } else if (xB < 400 ) {
      digitalWrite(DIR_Y, LOW);
      doStep(STEP_Y, stepStateY);
      positionY -= stepsPerDegree;
      lastStepY = now;
      lastMoveY = millis();
      savedY = false;
    }
  }

  // Trục Z
  if ((xA < 400 || xA > 600) && now - lastStepZ >= stepInterval) {
    if (xA > 600 ) {
      digitalWrite(DIR_Z, LOW);
      doStep(STEP_Z, stepStateZ);
      positionZ += stepsPerDegree;
      lastStepZ = now;
      lastMoveZ = millis();
      savedZ = false;
    } else if (xA < 400 ) {
      digitalWrite(DIR_Z, HIGH);
      doStep(STEP_Z, stepStateZ);
      positionZ -= stepsPerDegree;
      lastStepZ = now;
      lastMoveZ = millis();
      savedZ = false;
    }
  }

  // ==== Servo gradual control ====
  if ((yA < 400 || yA > 600) && now - lastStepServo >= servoStepInterval) {
    if (yA > 600 && currentAngle < 150) {
      currentAngle += servoStepSize;
      if (currentAngle > 150) currentAngle = 150;
      myServo.write(currentAngle);
      lastStepServo = now;
      lastMoveServo = millis();
      savedServo = false;
      // Serial.print("Servo angle: ");
      // Serial.println(currentAngle);
    } 
    else if (yA < 400 && currentAngle > 0) {
      currentAngle -= servoStepSize;
      if (currentAngle < 90) currentAngle = 90;
      myServo.write(currentAngle);
      lastStepServo = now;
      lastMoveServo = millis();
      savedServo = false;
      // Serial.print("Servo angle: ");
      // Serial.println(currentAngle);
    }
  }

  // Save to EEPROM after 2s stop
  unsigned long nowMillis = millis();
  // if (!savedX && (nowMillis - lastMoveX >= 2000)) {
  //   // EEPROM.put(EEPROM_ADDR_X, positionX);
  //   // EEPROM.put(EEPROM_FLAG_ADDR, EEPROM_MAGIC);
  //   savedX = true;
  //   Serial.print("Saved X: "); Serial.println(positionX, 2);
  // }
  // if (!savedY && (nowMillis - lastMoveY >= 2000)) {
  //   // EEPROM.put(EEPROM_ADDR_Y, positionY);
  //   // EEPROM.put(EEPROM_FLAG_ADDR, EEPROM_MAGIC);
  //   savedY = true;
  //   Serial.print("Saved Y: "); Serial.println(positionY, 2);
  // }
  // if (!savedZ && (nowMillis - lastMoveZ >= 2000)) {
  //   // EEPROM.put(EEPROM_ADDR_Z, positionZ);
  //   // EEPROM.put(EEPROM_FLAG_ADDR, EEPROM_MAGIC);
  //   savedZ = true;
  //   Serial.print("Saved Z: "); Serial.println(positionZ, 2);
  // }
  if (!savedServo && (nowMillis - lastMoveServo >= 2000)) {
    // EEPROM.put(EEPROM_ADDR_SERVO, currentAngle);
    // EEPROM.put(EEPROM_FLAG_ADDR, EEPROM_MAGIC);
    savedServo = true;
    Serial.print("Saved Servo angle: "); Serial.println(currentAngle);
  }
}

void doStep(int stepPin, bool &state) {
  state = !state;
  digitalWrite(stepPin, state);
}

bool parseData(const char* text, int& outXA, int& outYA, int& outXB, int& outYB) {
  return sscanf(text, "A (%d,%d); B (%d,%d)", &outXA, &outYA, &outXB, &outYB) == 4;
}
