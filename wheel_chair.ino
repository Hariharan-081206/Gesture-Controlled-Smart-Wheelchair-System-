#include <WiFi.h>
#include <esp_now.h>

// ==================================================
// MOTOR 1 - LEFT MOTOR
// ==================================================

#define ENA 25
#define IN1 26
#define IN2 27


// ==================================================
// MOTOR 2 - RIGHT MOTOR
// ==================================================

#define ENB 33
#define IN3 12
#define IN4 13


// ==================================================
// HC-SR04
// ==================================================

#define TRIG_PIN 32
#define ECHO_PIN 34


// ==================================================
// SAFETY SETTINGS
// ==================================================

const float OBSTACLE_DISTANCE = 10.0;

const unsigned long COMMUNICATION_TIMEOUT = 500;


// ==================================================
// COMMAND
// ==================================================

volatile char receivedCommand = 'S';

volatile unsigned long lastCommandTime = 0;


// ==================================================
// SERIAL DISTANCE TIMER
// ==================================================

unsigned long lastDistancePrint = 0;


// ==================================================
// ESP-NOW RECEIVE CALLBACK
// ==================================================

void onDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *incomingData,
  int len
) {

  if (len < 1) {
    return;
  }

  char command = (char)incomingData[0];

  // Accept only valid commands
  if (
    command == 'F' ||
    command == 'B' ||
    command == 'L' ||
    command == 'R' ||
    command == 'S'
  ) {

    receivedCommand = command;

    lastCommandTime = millis();

    Serial.print("Received command: ");
    Serial.println(command);
  }
}


// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  // ==================================================
  // MOTOR PINS
  // ==================================================

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);


  // ==================================================
  // HC-SR04
  // ==================================================

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);


  // ==================================================
  // MOTOR OFF
  // ==================================================

  stopMotors();


  // ==================================================
  // WIFI
  // ==================================================

  WiFi.mode(WIFI_STA);


  Serial.println();
  Serial.println("========================================");
  Serial.println("       WHEELCHAIR ESP32");
  Serial.println("========================================");

  Serial.print("Wheelchair MAC: ");
  Serial.println(WiFi.macAddress());


  // ==================================================
  // ESP-NOW
  // ==================================================

  if (esp_now_init() != ESP_OK) {

    Serial.println(
      "!!! ESP-NOW INITIALIZATION FAILED !!!"
    );

    stopMotors();

    return;
  }


  esp_now_register_recv_cb(onDataRecv);


  // ==================================================
  // STATUS
  // ==================================================

  Serial.println("ESP-NOW: READY");
  Serial.println("Motor 1: READY");
  Serial.println("Motor 2: READY");
  Serial.println("HC-SR04: READY");

  Serial.println("Obstacle limit: 10 cm");
  Serial.println("Communication timeout: 500 ms");

  Serial.println();
  Serial.println("F = Forward");
  Serial.println("B = Backward");
  Serial.println("L = Left");
  Serial.println("R = Right");
  Serial.println("S = Stop");

  Serial.println("========================================");


  lastCommandTime = millis();
}


// ==================================================
// MAIN LOOP
// ==================================================

void loop() {

  unsigned long now = millis();


  // ==================================================
  // 1. COMMUNICATION SAFETY
  // ==================================================

  if (
    now - lastCommandTime >
    COMMUNICATION_TIMEOUT
  ) {

    if (receivedCommand != 'S') {

      Serial.println(
        "!!! COMMUNICATION LOST - MOTORS STOPPED !!!"
      );
    }

    receivedCommand = 'S';

    stopMotors();
  }


  // ==================================================
  // 2. READ HC-SR04
  // ==================================================

  float distance = getDistance();


  // ==================================================
  // 3. DISPLAY DISTANCE
  // ==================================================

  if (now - lastDistancePrint >= 500) {

    lastDistancePrint = now;

    if (distance > 0) {

      Serial.print("Distance: ");
      Serial.print(distance, 2);
      Serial.println(" cm");

    } else {

      Serial.println(
        "Distance: NO ECHO"
      );
    }
  }


  // ==================================================
  // 4. EXECUTE COMMAND
  // ==================================================

  char command = receivedCommand;


  switch (command) {

    // ----------------------------------------------
    // FORWARD
    // ----------------------------------------------

    case 'F':

      if (safeToMove(distance)) {

        forwardMotors();

      } else {

        stopMotors();

        printObstacleWarning("FORWARD");
      }

      break;


    // ----------------------------------------------
    // BACKWARD
    // ----------------------------------------------

    case 'B':

      if (safeToMove(distance)) {

        backwardMotors();

      } else {

        stopMotors();

        printObstacleWarning("BACKWARD");
      }

      break;


    // ----------------------------------------------
    // LEFT
    // ----------------------------------------------

    case 'L':

      if (safeToMove(distance)) {

        leftTurn();

      } else {

        stopMotors();

        printObstacleWarning("LEFT");
      }

      break;


    // ----------------------------------------------
    // RIGHT
    // ----------------------------------------------

    case 'R':

      if (safeToMove(distance)) {

        rightTurn();

      } else {

        stopMotors();

        printObstacleWarning("RIGHT");
      }

      break;


    // ----------------------------------------------
    // STOP
    // ----------------------------------------------

    case 'S':

      stopMotors();

      break;


    // ----------------------------------------------
    // INVALID
    // ----------------------------------------------

    default:

      stopMotors();

      break;
  }


  delay(20);
}


// ==================================================
// HC-SR04 DISTANCE
// ==================================================

float getDistance() {

  // Trigger pulse

  digitalWrite(TRIG_PIN, LOW);

  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);

  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);


  // Read echo

  unsigned long duration = pulseIn(
    ECHO_PIN,
    HIGH,
    30000
  );


  // No echo

  if (duration == 0) {

    return -1;
  }


  // Calculate distance

  float distance =
    duration * 0.0343 / 2.0;


  return distance;
}


// ==================================================
// SAFETY CHECK
// ==================================================

bool safeToMove(float distance) {

  // ------------------------------------------------
  // NO ECHO
  // ------------------------------------------------

  if (distance <= 0) {

    return false;
  }


  // ------------------------------------------------
  // OBSTACLE TOO CLOSE
  // ------------------------------------------------

  if (distance <= OBSTACLE_DISTANCE) {

    return false;
  }


  return true;
}


// ==================================================
// MOTOR 1 FORWARD
// ==================================================

void motor1Forward() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}


// ==================================================
// MOTOR 1 BACKWARD
// ==================================================

void motor1Backward() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}


// ==================================================
// MOTOR 2 FORWARD
// ==================================================

void motor2Forward() {

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}


// ==================================================
// MOTOR 2 BACKWARD
// ==================================================

void motor2Backward() {

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}


// ==================================================
// ENABLE MOTORS
// ==================================================

void enableMotors() {

  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
}


// ==================================================
// DISABLE MOTORS
// ==================================================

void disableMotors() {

  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
}


// ==================================================
// FORWARD
// ==================================================

void forwardMotors() {

  motor1Forward();
  motor2Forward();

  enableMotors();
}


// ==================================================
// BACKWARD
// ==================================================

void backwardMotors() {

  motor1Backward();
  motor2Backward();

  enableMotors();
}


// ==================================================
// LEFT TURN
// ==================================================

void leftTurn() {

  // Left motor backward
  motor1Backward();

  // Right motor forward
  motor2Forward();

  enableMotors();
}


// ==================================================
// RIGHT TURN
// ==================================================

void rightTurn() {

  // Left motor forward
  motor1Forward();

  // Right motor backward
  motor2Backward();

  enableMotors();
}


// ==================================================
// STOP
// ==================================================

void stopMotors() {

  disableMotors();

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}


// ==================================================
// OBSTACLE WARNING
// ==================================================

void printObstacleWarning(
  const char *direction
) {

  static unsigned long lastWarning = 0;

  unsigned long now = millis();


  if (now - lastWarning < 500) {
    return;
  }


  lastWarning = now;


  Serial.print(
    "!!! OBSTACLE / NO ECHO - "
  );

  Serial.print(direction);

  Serial.println(
    " MOVEMENT STOPPED !!!"
  );
}