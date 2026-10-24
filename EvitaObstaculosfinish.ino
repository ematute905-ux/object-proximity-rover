#include "HUSKYLENS.h"
#include "Wire.h"

HUSKYLENS huskylens;

// L293D motor driver pins
const int enA = 9;   
const int in1 = 8;   
const int in2 = 7;   
const int enB = 3;   
const int in3 = 5;   
const int in4 = 4;   

// Motor speed calibration
int leftSpeed = 165;
int rightSpeed = 190;  
int turnSpeed = 170; 

// HC-SR04 pins
const int trigPin = 13; 
const int echoPin = 12;

// Camera connection status
bool isCameraConnected = false;
unsigned long lastAttemptTime = 0; 

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.println("System starting...");
  
  if (huskylens.begin(Wire)) {
    Serial.println("Camera ready");
    isCameraConnected = true;
  } else {
    Serial.println("Camera error - Blind mode ON");
  }
}

void loop() {
  // Reconnect camera if connection drops
  if (!isCameraConnected) {
    if (millis() - lastAttemptTime > 2000) { 
      Serial.println("Searching for camera...");
      if (huskylens.begin(Wire)) {
        Serial.println("Camera reconnected!");
        isCameraConnected = true;
      }
      lastAttemptTime = millis();
    }
  }

  // Ultrasonic distance reading
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2; 

  // Physical avoidance logic
  if (distance > 0 && distance < 15) { 
    Serial.println("Wall detected - Turning");
    turnRight();
    delay(400); 
  } 
  else {
    bool visualObstacle = false;
    
    // Read visual data if camera is available
    if (isCameraConnected) {
      if (huskylens.request() && huskylens.available()) {
        while (huskylens.available()) {
          HUSKYLENSResult result = huskylens.read();
          
          if (result.command == COMMAND_RETURN_BLOCK && result.ID > 0) {
            // Check bounding box width for proximity
            if (result.width > 150) { 
              visualObstacle = true;
            }
          }
        }
      }
    }

    // Final movement execution
    if (visualObstacle) {
      turnRight();
      delay(300); 
    } else {
      moveForward();
    }
  }
}

void moveForward() {
  analogWrite(enA, leftSpeed);
  analogWrite(enB, rightSpeed);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void turnRight() {
  analogWrite(enA, turnSpeed);
  analogWrite(enB, turnSpeed);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void stopMotors() {
  analogWrite(enA, 0);
  analogWrite(enB, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}