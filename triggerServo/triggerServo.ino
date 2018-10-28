

#include <Servo.h>

/************************SERVO VARIABLES****************************/
int triggerPin = 11;
Servo trigger;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int servoTime = 0;
int milInit;

int threshold = 100;

boolean found_a_person = true; // if a heat source is found then turn on the gun.
boolean on_a_person = false;   // if the person is centered then fire!

//time of each stage of shooting (ms)
int triggerLength = 600;
int releaseLength = 1500;

//angle to assign to the servo.
int triggerOut = 180;
int triggerIn = 60;

//boolean useful in controlling the shooting
boolean pulling = true;
boolean pstart = true;
boolean rstart = true;

boolean offStart = true;

/************************RELAY VARIABLES****************************/
#define ON   0
#define OFF  1

int IN2 = 12;

void setup() {
  trigger.attach(triggerPin);  // attaches the servo on your PWM pin
  milInit = 0;

  //TODO this will be removed.
  trigger.write(triggerOut);
  delay(2000);

  relay_init();
}

int i = 0;

boolean readys = false;
void loop() {
  if (found_a_person) {
    //TODO here I need to turn the gun on.
    //and I need to allow it to turn on for say... 5 seconds before firing.
    //Otherwise there will be a jam,
    relay_SetStatus(ON);//turn on RELAY_2

    if (!readys) {
      delay(5000);
      readys = true;
      on_a_person = true;
    }

    if (on_a_person) {
      //fire the gun. pull the trigger with the servo
      if (pulling) {
        if (pstart) {
          milInit = millis();
          pstart = false;
        } else {
          servoTime = millis() - milInit;
          if (servoTime % 10 == 0) {//this acts as a delay for updating the servo
            trigger.write((int)map(servoTime, 0, triggerLength, triggerOut, triggerIn));
          }

          if (servoTime > triggerLength) {
            pulling = false; //jump out of the pulling if and start releasing.
            rstart = true;
          }
        }
      } else {
        //release the trigger.
        if (rstart) {
          milInit = millis();
          rstart = false;
        } else {
          servoTime = millis() - milInit;
          if (servoTime % 10 == 0) {//this acts as a delay for updating the servo
            trigger.write((int)map(servoTime, 0, releaseLength, triggerIn, triggerOut));
          }

          if (servoTime > releaseLength) {
            pulling = true; //jump out of the pulling if and start releasing.
            pstart = true;
          }
        }
      }
    } else {
      //if your not centered on the person then don't pull the trigger.
      if (offStart) {
        trigger.write(triggerOut);
        offStart = false;
      }
    }
  } else {
    relay_SetStatus(OFF);//turn off RELAY_2
  }
}
// drive around like normal just scanning and such.
// and avoiding edges and such.


/**********************RELAY FUNCTIONS***********************/
void relay_init(void) {
  pinMode(IN2, OUTPUT);
  relay_SetStatus(OFF); //turn off all the relay
}

//set the status of relays
void relay_SetStatus(unsigned char status_2) {
  digitalWrite(IN2, status_2);
}



/*
   NOTES
   180 to 60 for trigger pulling.
   60 to 180 for trigger release.
*/
