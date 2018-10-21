#include <Wire.h>
#include <Servo.h>
#include <Adafruit_AMG88xx.h>

#define LEFT_F 9
#define LEFT_B 8

#define RIGHT_F 6
#define RIGHT_B 7

#define RIGHT_EDGE A0
#define LEFT_EDGE A1
#define FRONT_EDGE A2
#define RIGHT_BUMP A3
#define LEFT_BUMP 5
#define INT_PIN 2

#define TRIGGER_SERVO 11
#define VERTICAL_SERVO 10

#define RELAY 12


/***********************THEMAL SENSOR VARIABLES******************/
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
Adafruit_AMG88xx amg;

//average position of the thermal input. this goes through the average function
float average[2];

//highest value and it's location [value, x, y]. This goes through the highest function.
float highest[3];

//thermal camera function output (COMMANDS)
float thermalOutput[2];

//min Temp. anyhting below this will be ignored. ADJUST THIS LATER
float minTempThreshold = 25;

long thermTime = 0;
long thermTinit = 0;

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
int armPos;
int armJump = 3;

/************************RELAY VARIABLES****************************/
#define ON   0
#define OFF  1

int IN2 = 12;

/************************OTHER VARIABLES****************************/

/**
   keeps track of the state of all
   ir proximity sensors
*/
char irSensors[5] {0, 0, 0, 0, 0};

/**
   0: move forward
   1: right edge
   2: left edge
   3: front edge
   4: right bump
   5: left bump
   6: double bump
   7: left right movement
   8: up down movement
   9: shoot
   10: out of amo
*/
int state = 0;

/**
   keeps track of amo left
   program halts if amo == 0
*/
char amoCount = 6;

/*
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
*/
void setup() {
  //MOVEMENT
  pinMode(LEFT_B, OUTPUT);
  pinMode(LEFT_F, OUTPUT);
  pinMode(RIGHT_B, OUTPUT);
  pinMode(RIGHT_F, OUTPUT);
  pinMode(LEFT_EDGE, INPUT);
  pinMode(RIGHT_EDGE, INPUT);
  pinMode(FRONT_EDGE, INPUT);
  pinMode(RIGHT_BUMP, INPUT);
  pinMode(LEFT_BUMP, INPUT);
  pinMode(INT_PIN, OUTPUT);
  //make sure all motors are off
  setStop();

  //THERMAL SENSOR SETUP
  Serial.begin(9600);
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
  trigger.attach(TRIGGER_SERVO);  // attaches the servo on your PWM pin
  milInit = 0;


  trigger.write(triggerOut);
  relay_init();

  //ARM SERVO SETUP
  arm.attach(VERTICAL_SERVO);  // attaches the servo on your PWM pin
  armPos = (armUp + armDown) / 2;
  arm.write(armPos);

  delay(2000); //trigger placement and also sensor init

  setBackward();
  delay(500);
}




void loop() {
  //Update irSensors array
  state = 0;


  thermalCamera();
  //now the thermal data is updated: which way to move. found person? on person?

  setStop();
  delay(2);
  readIr();



  //update state with prorities to avoiding the edge
  updateState();
  Serial.println(state);

  if (state == 0) {
    moveForward();
  }
  else if (state == 1) {
    avoidRightEdge();
  }
  else if (state == 2) {
    avoidLeftEdge();
  }
  else if (state == 3) {
    avoidFrontEdge();
  }
  else if (state == 4) {
    avoidBump();
  }
  else if (state == 5) {
    avoidBump();
  }
  else if (state == 6) {
    avoidBump();
  }
  else if (state == 7) {
    setStop();
    //left right thermal movement
    if (thermalOutput[0] == -1) {
      setLeft();
      delay(100);
    } else if (thermalOutput[0] == 1) {
      setRight();
      delay(100);
    }
  } else if (state == 8) {
    setStop();
    //up down thermal movement
    if (thermalOutput[1] == -1) {
      //move servo up
      armPos -= 10;
      if (armPos < armDown) {
        armPos = armDown;
      }
      arm.write(armPos);
    } else if (thermalOutput[1] == 1) {
      //move servo up
      armPos += 10;
      if (armPos > armUp) {
        armPos = armUp;
      }
      arm.write(armPos);
    }
  } else if (state == 9) {
    setStop();
    //fire the gun. pull the trigger with the servo
    shootTheShizOutOfEm();
  }

  else if (state == 10) {
    while (1) {
      setStop(); //out of amo, end program to reload
    }
  }
}


void shootTheShizOutOfEm() {

  relay_SetStatus(ON);
  delay(5000);

  //begin the pulling
  milInit = millis();

  servoTime = 0;
  while (servoTime < triggerLength) {
    servoTime = millis() - milInit;
    if (servoTime % 10 == 0) {
      trigger.write((int)map(servoTime, 0, triggerLength, triggerOut, triggerIn));
    }
  }
  trigger.write(triggerIn);


  //begin the release
  milInit = millis();
  
  servoTime = 0;
  while (servoTime < releaseLength) {
    servoTime = millis() - milInit;
    trigger.write((int)map(servoTime, 0, releaseLength, triggerIn, triggerOut));
  }
  trigger.write(triggerOut);

  delay(1000);

  amoCount--;
  relay_SetStatus(OFF);
}


/*
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
 ************************************************************************************************
*/

/**********************************THERMAL SENSOR FUNCTIONS***************************/

float arbThresh = 3;
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
    Serial.print(average[0], 3);
    Serial.print(", ");
    Serial.print(average[1], 3);
    Serial.println();

    //check to make sure the highest value and the average value is in the center
    if (abs(average[0]) < arbThresh && abs(average[1]) < arbThresh) {
      Serial.println("SHOOT");
      on_a_person = true;
      thermalOutput[0] = 0;
      thermalOutput[1] = 0;
    } else {
      on_a_person = false;
      //directions for moving the thingy thing.
      if (average[0] < -arbThresh) {
        Serial.print("\tMove up.");
        thermalOutput[1] = 1;
      } else if (average[0] > arbThresh) {
        Serial.print("\tMove down.");
        thermalOutput[1] = -1;
      } else {
        thermalOutput[1] = 0;
      }

      if (average[1] < -arbThresh) {
        Serial.print("\tMove right.");
        thermalOutput[0] = 1;
      } else if (average[1] > arbThresh) {
        Serial.print("\tMove left.");
        thermalOutput[0] = -1;
      } else {
        thermalOutput[0] = 0;
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
      arr[i] = map(arr[i], minTempThreshold, 35, 5, 50);
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
  pinMode(RELAY, OUTPUT);
  relay_SetStatus(OFF); //turn off all the relay
}

//set the status of relays
void relay_SetStatus(unsigned char status_2) {
  digitalWrite(RELAY, status_2);
}




/****************************MOVEMENT******************************/

void readIr() {
  irSensors[0] = digitalRead(RIGHT_EDGE);
  irSensors[1] = digitalRead(LEFT_EDGE);
  irSensors[2] = digitalRead(FRONT_EDGE);
  irSensors[3] = digitalRead(RIGHT_BUMP);
  irSensors[4] = digitalRead(LEFT_BUMP);
}
/**
   sets all tracks to move forward
*/
void setForward() {
  digitalWrite(LEFT_B, LOW);
  digitalWrite(RIGHT_B, LOW);
  digitalWrite(LEFT_F, HIGH);
  digitalWrite(RIGHT_F, HIGH);
}

/**
   sets all tracks to move backwards
*/
void setBackward() {
  digitalWrite(LEFT_F, LOW);
  digitalWrite(RIGHT_F, LOW);
  digitalWrite(LEFT_B, HIGH);
  digitalWrite(RIGHT_B, HIGH);
}

/**
   sets tracks to rotate left
*/
void setLeft() {
  digitalWrite(LEFT_F, LOW);
  digitalWrite(RIGHT_B, LOW);
  digitalWrite(LEFT_B, HIGH);
  digitalWrite(RIGHT_F, HIGH);
}

/**
   sets all tracks to rotate right
*/
void setRight() {
  digitalWrite(LEFT_B, LOW);
  digitalWrite(RIGHT_F, LOW);
  digitalWrite(LEFT_F, HIGH);
  digitalWrite(RIGHT_B, HIGH);
}

/**
   stops all track movement
*/
void setStop() {
  digitalWrite(LEFT_F, LOW);
  digitalWrite(LEFT_B, LOW);
  digitalWrite(RIGHT_F, LOW);
  digitalWrite(RIGHT_B, LOW);
}

/**
   gradually moves the bot forward
   stops to let the sensors read
   without noise from motors
*/
void moveForward() {
  setForward();
  delay(100);
}

/**
   used to avoid a front edge
*/
void avoidFrontEdge() {
  setStop();
  setBackward();
  delay(500);
  setLeft();
  delay(1000);
  setStop();
}

/**
   used to avoid right edge
*/
void avoidRightEdge() {
  setStop();
  setBackward();
  delay(500);
  setLeft();
  delay(500);
  setStop();
}

void avoidLeftEdge() {
  setStop();
  setBackward();
  delay(500);
  setRight();
  delay(500);
  setStop();
}

void avoidBump() {
  setStop();
  setBackward();
  delay(500);
  setLeft();
  delay(1000);
  setStop();
}

void updateState() {
  /**
    0: move forward
    1: right edge
    2: left edge
    3: front edge
    4: right bump
    5: left bump
    6: double bump
    7: left right movement
    8: up down movement
    9: shoot
    10: out of amo
  */
  //  state = 0;
  //  found_a_person = false;
  if (amoCount == 0) {
    state = 10;
  } else if (found_a_person) {
    if (on_a_person) {
      state = 9;
    } else {
      if (thermalOutput[0] != 0) {
        state = 7;
      } else if (thermalOutput[1] != 0) {
        state = 8;
      }
    }
  } else if (irSensors[3] == 0) {
    state = 4;
  } else if (irSensors[4] == 0) {
    state = 5;
  } else if (irSensors[2] == 1) {
    state = 3;
  } else if (irSensors[1] == 1) {
    state = 2;
  } else if (irSensors[0] == 1) {
    state = 1;
  }
}

// irSensors[0] = digitalRead(RIGHT_EDGE);
//  irSensors[1] = digitalRead(LEFT_EDGE);
//  irSensors[2] = digitalRead(FRONT_EDGE);
//  irSensors[3] = digitalRead(RIGHT_BUMP);
//  irSensors[4] = digitalRead(LEFT_BUMP);

