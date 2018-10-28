

#include <Servo.h>

/************************ARM SERVO VARIABLES****************************/
int armPin = 10;
Servo arm;  // create servo object to control a servo
// twelve servo objects can be created on most boards

//angle to assign to the servo.
int armDown = 0;
int armUp = 180;

void setup() {
  arm.attach(armPin);  // attaches the servo on your PWM pin
  arm.write(armDown);
  delay(2000);
}

void loop() {

  for (int i = 0; i < 100; i++) {
    arm.write(map(i, 0, 99, armDown, armUp));
    delay(50);
  }

  for (int i = 99; i >= 0; i--) {
    arm.write(map(i, 99, 0, armUp, armDown));
    delay(50);
  }

}
