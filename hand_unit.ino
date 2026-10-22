#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <math.h>

// ==================================================
// MPU6050
// ==================================================

#define SDA_PIN 21
#define SCL_PIN 22

#define MPU6050_ADDR 0x68

#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B


// ==================================================
// FORWARD / BACKWARD THRESHOLDS
// ==================================================

#define FORWARD_START   20.0
#define FORWARD_STOP    15.0

#define BACKWARD_START -20.0
#define BACKWARD_STOP  -15.0


// ==================================================
// LEFT / RIGHT THRESHOLDS
// ==================================================

#define LEFT_START   20.0
#define LEFT_STOP    15.0

#define RIGHT_START  -20.0
#define RIGHT_STOP   -15.0


// ==================================================
// WHEELCHAIR ESP32 MAC
// ==================================================

uint8_t wheelchairMAC[] = {
  0x48,
  0xE7,
  0x29,
  0xAD,
  0x28,
  0x24
};


// ==================================================
// ESP-NOW MESSAGE
// ==================================================

typedef struct {

  char command;

} Message;

Message message;


// ==================================================
// CURRENT COMMAND
// ==================================================

char currentCommand = 'S';


// ==================================================
// SEND INTERVAL
// ==================================================

unsigned long lastSendTime = 0;

#define SEND_INTERVAL 100


// ==================================================
// READ MPU6050
// ==================================================

void readAccelerometer(
  int16_t &accelX,
  int16_t &accelY,
  int16_t &accelZ
) {

  Wire.beginTransmission(MPU6050_ADDR);

  Wire.write(ACCEL_XOUT_H);

  Wire.endTransmission(false);

  Wire.requestFrom(
    MPU6050_ADDR,
    6,
    true
  );

  if (Wire.available() >= 6) {

    accelX =
      Wire.read() << 8 |
      Wire.read();

    accelY =
      Wire.read() << 8 |
      Wire.read();

    accelZ =
      Wire.read() << 8 |
      Wire.read();
  }
}


// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  // ==================================================
  // MPU6050
  // ==================================================

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  // Wake MPU6050

  Wire.beginTransmission(
    MPU6050_ADDR
  );

  Wire.write(
    PWR_MGMT_1
  );

  Wire.write(0x00);

  Wire.endTransmission();


  // ==================================================
  // WIFI
  // ==================================================

  WiFi.mode(WIFI_STA);


  // ==================================================
  // ESP-NOW
  // ==================================================

  if (
    esp_now_init() != ESP_OK
  ) {

    Serial.println(
      "ESP-NOW INITIALIZATION FAILED!"
    );

    return;
  }


  // ==================================================
  // ADD WHEELCHAIR
  // ==================================================

  esp_now_peer_info_t peerInfo = {};

  memcpy(
    peerInfo.peer_addr,
    wheelchairMAC,
    6
  );

  peerInfo.channel = 0;

  peerInfo.encrypt = false;


  if (
    esp_now_add_peer(
      &peerInfo
    ) != ESP_OK
  ) {

    Serial.println(
      "FAILED TO ADD WHEELCHAIR PEER!"
    );

    return;
  }


  // ==================================================
  // READY
  // ==================================================

  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "       HAND ESP32 CONTROLLER"
  );

  Serial.println(
    "========================================"
  );

  Serial.print(
    "Hand MAC: "
  );

  Serial.println(
    WiFi.macAddress()
  );

  Serial.println();

  Serial.println(
    "FORWARD  → Roll > +20°"
  );

  Serial.println(
    "BACKWARD → Roll < -20°"
  );

  Serial.println(
    "LEFT     → Side > +20°"
  );

  Serial.println(
    "RIGHT    → Side < -20°"
  );

  Serial.println(
    "STOP     → Neutral"
  );

  Serial.println();

  Serial.println(
    "ESP-NOW Ready"
  );

  Serial.println(
    "========================================"
  );
}


// ==================================================
// MAIN LOOP
// ==================================================

void loop() {

  int16_t accelX;
  int16_t accelY;
  int16_t accelZ;


  // ==================================================
  // READ SENSOR
  // ==================================================

  readAccelerometer(
    accelX,
    accelY,
    accelZ
  );


  // ==================================================
  // CONVERT TO G
  // ==================================================

  float ax =
    accelX / 16384.0;

  float ay =
    accelY / 16384.0;

  float az =
    accelZ / 16384.0;


  // ==================================================
  // FORWARD / BACKWARD ANGLE
  // ==================================================

  float roll =
    atan2(
      ay,
      sqrt(
        ax * ax +
        az * az
      )
    )
    * 180.0 / PI;


  // ==================================================
  // LEFT / RIGHT ANGLE
  // ==================================================

  float sideAngle =
    atan2(
      az,
      sqrt(
        ax * ax +
        ay * ay
      )
    )
    * 180.0 / PI;


  // ==================================================
  // DETERMINE COMMAND
  // ==================================================

  char newCommand = currentCommand;


  // ==================================================
  // CURRENTLY STOPPED
  // ==================================================

  if (currentCommand == 'S') {

    if (
      roll >= FORWARD_START
    ) {

      newCommand = 'F';
    }

    else if (
      roll <= BACKWARD_START
    ) {

      newCommand = 'B';
    }

    else if (
      sideAngle >= LEFT_START
    ) {

      newCommand = 'L';
    }

    else if (
      sideAngle <= RIGHT_START
    ) {

      newCommand = 'R';
    }

    else {

      newCommand = 'S';
    }
  }


  // ==================================================
  // CURRENTLY FORWARD
  // ==================================================

  else if (currentCommand == 'F') {

    if (
      roll <= FORWARD_STOP
    ) {

      newCommand = 'S';
    }

    else {

      newCommand = 'F';
    }
  }


  // ==================================================
  // CURRENTLY BACKWARD
  // ==================================================

  else if (currentCommand == 'B') {

    if (
      roll >= BACKWARD_STOP
    ) {

      newCommand = 'S';
    }

    else {

      newCommand = 'B';
    }
  }


  // ==================================================
  // CURRENTLY LEFT
  // ==================================================

  else if (currentCommand == 'L') {

    if (
      sideAngle <= LEFT_STOP
    ) {

      newCommand = 'S';
    }

    else {

      newCommand = 'L';
    }
  }


  // ==================================================
  // CURRENTLY RIGHT
  // ==================================================

  else if (currentCommand == 'R') {

    if (
      sideAngle >= RIGHT_STOP
    ) {

      newCommand = 'S';
    }

    else {

      newCommand = 'R';
    }
  }


  // ==================================================
  // COMMAND CHANGED
  // ==================================================

  if (
    newCommand != currentCommand
  ) {

    currentCommand =
      newCommand;


    Serial.print(
      "Roll: "
    );

    Serial.print(
      roll,
      1
    );

    Serial.print(
      "° | Side: "
    );

    Serial.print(
      sideAngle,
      1
    );

    Serial.print(
      "° → Command: "
    );


    if (
      currentCommand == 'F'
    ) {

      Serial.println(
        "FORWARD"
      );
    }

    else if (
      currentCommand == 'B'
    ) {

      Serial.println(
        "BACKWARD"
      );
    }

    else if (
      currentCommand == 'L'
    ) {

      Serial.println(
        "LEFT"
      );
    }

    else if (
      currentCommand == 'R'
    ) {

      Serial.println(
        "RIGHT"
      );
    }

    else {

      Serial.println(
        "STOP"
      );
    }
  }


  // ==================================================
  // SEND COMMAND
  // ==================================================

  if (
    millis() - lastSendTime >=
    SEND_INTERVAL
  ) {

    lastSendTime =
      millis();


    message.command =
      currentCommand;


    esp_err_t result =
      esp_now_send(
        wheelchairMAC,
        (uint8_t *)&message,
        sizeof(message)
      );


    if (
      result != ESP_OK
    ) {

      Serial.println(
        "ESP-NOW SEND FAILED!"
      );
    }
  }


  delay(20);
}