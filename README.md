# Autonomous Rover with HuskyLens and Ultrasonic Sensor

I built this autonomous mobile robot to combine physical distance measurement with computer vision. It uses an HC-SR04 ultrasonic sensor to prevent crashes and a HuskyLens camera via I2C to recognize specific objects and navigate around them.

## Hardware setup

* Brain: Arduino Uno / Mega
* Vision: HuskyLens AI Camera
* Distance: HC-SR04 Ultrasonic Sensor
* Motors: DC Gear Motors and a L293D motor driver
* Power: External battery pack to isolate motor noise from the Arduino

## Pinout

| Component | Arduino Pin | Function |
|---|---|---|
| L293D enA / enB | 9 / 3 (PWM) | Motor speeds |
| L293D in1 / in2 | 8 / 7 | Left motor direction |
| L293D in3 / in4 | 5 / 4 | Right motor direction |
| HC-SR04 | Trig 13 / Echo 12 | Distance sensing |
| HuskyLens | SDA / SCL | I2C communication |

## How the code works (and the fail-safe)

When building this, I realized that if the camera wires disconnected while moving, the Arduino loop() would freeze. To fix this, I implemented a non-blocking fail-safe system:

1. Blind mode fallback: If the HuskyLens disconnects, the rover doesn't stop. It automatically switches to using only the ultrasonic sensor to navigate safely.
2. On-the-fly reconnection: Instead of using delay(), the code uses millis() to attempt an I2C reconnection every 2 seconds in the background without interrupting the motor control.
3. Visual proximity: The camera doesn't just detect an object; it reads the bounding box pixel width (result.width > 150). This allows the rover to know when the learned object is actually close, not just visible in the background.

## Core logic snippet

```cpp
  // 1. Non-blocking camera reconnection attempt
  if (!camaraConectada) {
    if (millis() - tiempoUltimoIntento > 2000) { 
      if (huskylens.begin(Wire)) {
        camaraConectada = true; 
      }
      tiempoUltimoIntento = millis();
    }
  }

  // 2. Physical distance check (Always active)
  int distancia = duracion * 0.034 / 2; 

  // 3. Layer 1: Physical Avoidance (Emergency stop & turn)
  if (distancia > 0 && distancia < 15) { 
    giroADerecha();
    delay(400); 
  } 
  else {
    bool obstaculoVisual = false;
    
    // 4. Layer 2: Visual Avoidance (Only if camera is working)
    if (camaraConectada) {
      if (huskylens.request() && huskylens.available()) {
        while (huskylens.available()) {
          HUSKYLENSResult result = huskylens.read();
          // Trigger if it's a learned object (ID > 0) and physically close
          if (result.command == COMMAND_RETURN_BLOCK && result.ID > 0) {
            if (result.width > 150) { 
              obstaculoVisual = true;
            }
          }
        }
      }
    }

    // 5. Final movement decision
    if (obstaculoVisual) {
      giroADerecha();
      delay(300); 
    } else {
      avanzar();
    }
  }
