#include <Wire.h>
#include <Servo.h>
#include <Adafruit_AMG88xx.h>

/***********************THEMAL SENSOR VARIABLES******************/
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
Adafruit_AMG88xx amg;

//average position of the thermal input. this goes through the average function
float average[2];

//highest value and it's location [value, x, y]. This goes through the highest function.
float highest[3];

//thermal camera function output (COMMANDS)
float thermalOutput[3];

//min Temp. anyhting below this will be ignored. ADJUST THIS LATER
float minTempThreshold = 23;

int thermTime = 0;
int thermTinit = 0;

/************************TRIGGER SERVO VARIABLES****************************/
int triggerPin = 11;
Servo trigger;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int servoTime = 0;
int milInit;

int threshold = 100;

boolean found_a_person = false; // if a heat source is found then turn on the gun.
boolean on_a_person = false;   // if the person is centered then fire!

//time of each stage of shooting (ms)
int triggerLength = 600;
int releaseLength = 1500;

//angle to assign to the servo.
int triggerOut = 180;
int triggerIn = 60;

//boolean useful in controlling the shooting
boolean pulling = false;
boolean pstart = true;
boolean rstart = true;

boolean offStart = true;

/************************ARM SERVO VARIABLES****************************/
int armPin = 10;
Servo arm;  // create servo object to control a servo
// twelve servo objects can be created on most boards

//angle to assign to the servo.
int armDown = 0;
int armUp = 180;

/************************RELAY VARIABLES****************************/
#define ON   0
#define OFF  1

int IN2 = 12;



void setup() {
  //THERMAL SENSOR SETUP
  Serial.begin(9600); //TODO will have to remove all Serial stuff when it is autonomous
  Serial.println(F("AMG88xx test"));

  bool status;

  // default settings
  //init sensor stuffs.
  status = amg.begin();
  if (!status) {
    Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
    while (1);
  }

  //TRIGGER SERVO SETUP
  trigger.attach(triggerPin);  // attaches the servo on your PWM pin
  milInit = 0;

  //TODO this will be removed.
  trigger.write(triggerOut);
  relay_init();

  //ARM SERVO SETUP
  arm.attach(armPin);  // attaches the servo on your PWM pin
  arm.write((armUp + armDown) / 2);

  delay(2000); //trigger placement and also sensor init
}


void loop() {
  //this does the thermal camera stuff and updates the thermalOutput array.
  //the thermalOutput array will tell us what the robot needs to do. and then it will do it.
  //hopefully.
  thermTime = millis() - thermTinit;
  if (thermTime % 500 == 0) {
    thermalCamera();
    if (pulling) {
      on_a_person = true;
    }
  }

  //the data from the thermalCamera is as follows:
  //thermalCamera[0] = left/right movement --->  left = -1   :    right = 1
  //thermalCamera[1] = down/up movement --->  down = -1   :    up = 1

  if (found_a_person) {
    //TODO here I need to turn the gun on.
    //and I need to allow it to turn on for say... 5 seconds before firing.
    //Otherwise there will be a jam,
    relay_SetStatus(ON);//turn on RELAY_2

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
        pulling = true;
      }
    }
  } else {
    relay_SetStatus(OFF);//turn off RELAY_2
  }
}

/**********************************THERMAL SENSOR FUNCTIONS***************************/

void thermalCamera() {
  Serial.print("Thermal grid:");
  amg.readPixels(pixels); //get data from the thermal sensor

  //print the raw data from the sensor
  Serial.println();
  Serial.println();
  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      Serial.print(pixels[i + (j * 8)], 3);
      Serial.print("\t");
    }
    Serial.println();
  }
  Serial.println();

  //find the highest value and it's location.
  //All of that is stored inside the highest array
  findHighest(pixels, highest);

  //only care if any value is higher than the min threshold
  if (highest[0] > minTempThreshold) {
    found_a_person = true;

    //if a value is above the threshold then create the mapped array and find the average.
    //map the pixels (see mapping function) to find a good average value.
    float mappedPixels[AMG88xx_PIXEL_ARRAY_SIZE];
    for (int i = 0; i < 64; i++) {
      mappedPixels[i] = pixels[i];
    }
    mapArray(mappedPixels);
    //now get the average of the mapped pixels.
    findAverage(mappedPixels, average);

    //print the mapped values to clearly see where the average is.
    //This will also clearly show the highest value.
    Serial.println();
    for (int j = 0; j < 8; j++) {
      for (int i = 0; i < 8; i++) {
        Serial.print(mappedPixels[i + (j * 8)], 0);
        Serial.print("\t");
      }
      Serial.println();
    }
    Serial.println();

    Serial.print(highest[0], 3);
    Serial.print(":    ");
    Serial.print(highest[1], 3);
    Serial.print(", ");
    Serial.print(highest[2], 3);
    Serial.println();

    //check to make sure the highest value and the average value is in the center
    if (abs(average[0]) < 3 && abs(average[1]) < 3) {
      Serial.println("SHOOT");
      on_a_person = true;
      thermalOutput[0] = 0;
      thermalOutput[1] = 0;
    } else {
      on_a_person = false;
      //directions for moving the thingy thing.
      if (highest[1] < 0) {
        Serial.print("\tMove left.");
        thermalOutput[0] = -1;
      } else if (highest[1] > 0) {
        Serial.print("\tMove right.");
        thermalOutput[0] = 1;
      }

      if (highest[2] < 0) {
        Serial.print("\tMove down.");
        thermalOutput[1] = -1;
      } else if (highest[2] > 0) {
        Serial.print("\tMove up.");
        thermalOutput[1] = 1;
      }
    }
  } else {
    found_a_person = false;
  }


  Serial.println();
  Serial.println();
  Serial.println();
}

//map the values to acheive a better average location
//I want to take values that are below the min temperature threshold and set them to 0.
//Then I will map the other values from 0-100 so there is a more clear average value.
void mapArray(float arr[]) {
  for (int i = 0; i < 64; i++) {
    if (arr[i] < minTempThreshold) {
      arr[i] = 0;
    } else {
      arr[i] = map(arr[i], minTempThreshold, 35, 0, 100);
    }
  }
}

//function that recieves a 1d array from sensor,
//and finds the average with respect to the center of the 2d array form

//normal cartesian coordinate signs.
// eg. negative, negative --->  move to the left and up.

//modifyme array is the two part array that holds the values.
void findAverage(float arr[], float modifyme[]) {
  float xsum = 0;
  float ysum = 0;

  float total = 0;

  //convert 1d array to a 2d array
  float arr2[8][8];
  for (int index = 0; index < 64; index++) {
    int x = index % 8;
    int y = index / 8;
    arr2[x][y] = arr[index];
  }

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      xsum += arr2[i][j] * (i < 4 ? i - 4 : i - 4 + 1);
      ysum += arr2[i][j] * (j < 4 ? j - 4 : j - 4 + 1);

      total += arr2[i][j];
    }
  }

  float xAverage = xsum / total;
  float yAverage = ysum / total;

  //println(xAverage + ", " + yAverage + " with respect to the center.");
  modifyme[0] = xAverage;
  modifyme[1] = yAverage;
}


//finds the highest value and it's position in the array (with respect to the center of the grid)
void findHighest(float arr[], float modifyme[]) {
  float highest = 0;
  float hx = 0;
  float hy = 0;

  //convert 1d array to a 2d array
  float arr2[8][8];
  for (int index = 0; index < 64; index++) {
    int x = index % 8;
    int y = index / 8;
    arr2[x][y] = arr[index];
  }

  //find the highest stuff
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (arr2[i][j] > highest) {
        //        Serial.println("something is happening here");
        highest = arr2[i][j];
        hx = (i < 4 ? i - 4 : i - 4 + 1);
        hy = (j < 4 ? j - 4 : j - 4 + 1);
      }
    }
  }

  //println(xAverage + ", " + yAverage + " with respect to the center.");
  modifyme[0] = highest;
  modifyme[1] = hx;
  modifyme[2] = hy;
}


/**********************RELAY FUNCTIONS***********************/
void relay_init(void) {
  pinMode(IN2, OUTPUT);
  relay_SetStatus(OFF); //turn off all the relay
}

//set the status of relays
void relay_SetStatus(unsigned char status_2) {
  digitalWrite(IN2, status_2);
}



