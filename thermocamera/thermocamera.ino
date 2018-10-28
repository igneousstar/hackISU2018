#include <Wire.h>
#include <Adafruit_AMG88xx.h>

float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
Adafruit_AMG88xx amg;

//average position of the thermal input. this goes through the average function
float average[2];

//highest value and it's location [value, x, y]. This goes through the highest function.
float highest[3];

//min Temp. anyhting below this will be ignored. ADJUST THIS LATER
float minTempThreshold = 23;

void setup() {
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

  delay(100); // let sensor boot up
}


void loop() {
  thermalCamera();
}

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

  /*
     THIS DOESNT MATTER ANYMORE
     IGNORE THIS

    findAverage(pixels, average);
    Serial.print(average[0], 3);
    Serial.print(", ");
    Serial.print(average[1], 3);
    Serial.println();
    if (average[0] < 0) {
      Serial.print("\tMove right.");
    } else if (average[0] > 0) {
      Serial.print("\tMove left.");
    }

    if (average[1] < 0) {
      Serial.print("\tMove up.");
    } else if (average[1] > 0) {
      Serial.print("\tMove down.");
    }

  */

  //find the highest value and it's location.
  //All of that is stored inside the highest array
  findHighest(pixels, highest);

  //only care if any value is higher than the min threshold
  if (highest[0] > minTempThreshold) {

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
    if (abs(average[0]) < 3 && abs(average[1]) < 3) {
      Serial.println("SHOOT");
    } else {
      //directions for moving the thingy thing.
      if (highest[1] < 0) {
        Serial.print("\tMove left.");
      } else if (highest[1] > 0) {
        Serial.print("\tMove right.");
      }

      if (highest[2] < 0) {
        Serial.print("\tMove down.");
      } else if (highest[2] > 0) {
        Serial.print("\tMove up.");
      }
    }
  }


  Serial.println();
  Serial.println();
  Serial.println();

  delay(2000);
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


