#include <Servo.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <FastLED.h>

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

Servo wheelServo;  // create a servo object to control a servo
Servo chuteServo;  // create another servo object to control a servo
Servo mixerServo;

const byte LED_DIGITAL = { 10 };
const byte NUM_LEDS =  5; // QUANTIDADE DE LEDS ENDERECAVEIS
CRGB leds[NUM_LEDS];

const int calibrating = 0; //change this to 0 when you want to run the machine and to 1 when you set it up for the first time
const int wheelOverRun = 80; //decrease this if the skittle goes past the colour sensor or increase it if the skittle stops before it reaches the sensor
//270 para 90 --- 160 para 89 --- 110 para 88 -- 80 para 87
const int infraRedSensor = 8;

int switchState;
int arrayIndexToAimAt;

float redReading, greenReading, blueReading; //somewhere to store our colour sensor readings

const int arrayRows = 7;
const int arrayColumns = 6;
int SAMPLES[arrayRows][arrayColumns] = {  // array goes red value, green value, blue value, colour, difference to sample (update by code later), angle for depositor
  {95, 82,  58,  "Red", 0, 30}, //red -0
  {107,  75,  52,  "Orange", 0, 45}, //orange -1
  {99,  87,  47,  "Yellow", 0, 60}, //yellow -2
  {80,  94,  58,  "Green", 0, 90}, //green -3 
  {73,  87,  72,  "Blue", 0, 105}, //blue -4   
  {86,  86,  61, "Brown", 0, 120},  //brown -5
  {91, 85, 59, "No Skittle", 0, 75}, //no skittle -6
}; 
// Determines the number of samples stored in the array
const byte samplesCount = sizeof(SAMPLES) / sizeof(SAMPLES[0]);


void setup()
{
  pinMode(infraRedSensor, INPUT);
  
  Serial.begin(9600);

  
  chuteServo.attach(5);  // attaches the servo on pin d5 to the servo object
  wheelServo.attach(6);  // attaches the servo on pin d6 to the servo object
  mixerServo.attach(7);  // attaches the servo on pin d7 to the servo object

    if (tcs.begin()) {
    //Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }

  // printArray(SAMPLES);
  FastLED.addLeds<WS2812B, LED_DIGITAL, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.clear();
    
}




void loop()
{
  switchState = digitalRead(infraRedSensor);
  
  if (switchState == HIGH){
    // if (calibrating == 0){
    //   clearScreen();
    // }
    // Serial.println();
    // Serial.println("Yes - contact switch aligned. Taking a reading....");
    delay(wheelOverRun); //allows the wheel to keep turning - adjust this value to ensure your skittle stops beenath the colour sensor.
    wheelServo.write(92);  //stops the skittle wheel    
    readColour();
    if (calibrating == 0){
      matchColour();
    }
    // if (calibrating == 0){
    //   printArray(SAMPLES);
    // }
    arrayIndexToAimAt = findClosestSample();
    aimChute();    
    if (calibrating == 1){
      waitForUser(); 
    }
    delay(500); //gives the chute time to settle in place before we send the skittle down
    } else {
    //Serial.println("Skittle moving wheel not aligned wiht contact switch yet - please gently rotate by hand unless a reading is produced shortly");     
    wheelServo.write(87);  //starts to turn the skittle collecting wheel
    //mixerServo.write(80);
  }
}



void readColour()
  {
    delay(70);  // takes 50ms to read
  
    tcs.getRGB(&redReading, &greenReading, &blueReading);
    
    // Serial.println("Colour sensor currently reads:  ");
    // Serial.print("R:"); Serial.print(int(redReading)); 
    // Serial.print("\tG:"); Serial.print(int(greenReading)); 
    // Serial.print("\tB:"); Serial.println(int(blueReading));
    // Serial.println();
  }

void matchColour()
{
  int colourDistance;

  // Iterate through the array to find a matching colour sample
  for (byte i = 0; i < samplesCount; i++)
  {
   colourDistance = getColourVariance(redReading, greenReading, blueReading, SAMPLES[i][0], SAMPLES[i][1], SAMPLES[i][2], SAMPLES[i][3]);
   SAMPLES[i][4] = colourDistance;
  }   
}



int getColourVariance(int redSensor, int greenSensor, int blueSensor, int redSample, int greenSample, int blueSample, char *colour)
{
  // Serial.print("Checking the skittle against the ");
  // Serial.print(colour);
  // Serial.println(" sample.");
  int redDifference;
  redDifference = compareValues(redSensor, redSample, "Red ");
  int greenDifference;
  greenDifference = compareValues(greenSensor, greenSample, "Green ");
  int blueDifference;
  blueDifference = compareValues(blueSensor, blueSample, "Blue ");
  // Serial.print("Total variance between the measured colour and sample for the ");
  // Serial.print(colour);
  // Serial.print(" item in the array = ");
  int totalDifference;
  totalDifference = redDifference + greenDifference + blueDifference;
  // Serial.println(totalDifference);
  // Serial.println();
  return totalDifference;
  
}

int compareValues(int sensorColour, int sampleColour, char *colour)
{
  int difference;
  // Serial.print("Comparing Colours:");
  // Serial.print(colour);
  // Serial.print(int(sensorColour)); 
  // Serial.print(" was measured compared with ");
  // Serial.print(int(sampleColour));
  // Serial.print(" which has a difference of = ");
  difference = sensorColour - sampleColour;
  if (difference < 0)
  {
    difference = difference * (-1);
  }
  // Serial.println(int(difference));
  return difference;
}

// void printArray( const int a[][ arrayColumns ] ) {
//    // loop through array's rows
//    for ( int i = 0; i < arrayRows; ++i ) {
//       // loop through columns of current row
//       for ( int j = 0; j < arrayColumns; ++j ){
//         Serial.print (a[ i ][ j ] );
//         Serial.print(", ");
//       }
//       Serial.println();
//    }
//    Serial.println(); 
// }

int findClosestSample()
{
  int index = 0 ;           

  for (int i = 0; i < arrayRows; i++) {    
    if (SAMPLES[i][4] < SAMPLES[index][4]) {
      index = i;
    }
  }    
  switch (index) {
    case 0:
      Serial.println("Vermelho");
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(255, 0, 0);              
      }
      FastLED.show();
      break;
    case 1:
      Serial.println("Laranja");
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(255, 100, 0);              
      }
      FastLED.show();
      break;
    case 2:
      Serial.println("Amarelo");
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(255, 255, 0);              
      }
      FastLED.show();
      break;
    case 3:
      Serial.println("Verde");
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(0, 255, 0);              
      }
      FastLED.show();
      break;
    case 4:
      Serial.println("Azul");
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(0, 0, 255);              
      }
      FastLED.show();
      break;
    case 5:
      Serial.println("Marron");
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(50, 10, 15);              
      }
      FastLED.show();
      break;
    default:
      //Serial.println("Branco");
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(255, 255, 255);              
      }
      FastLED.show();
      break; 
  }  
  return index;
}

void aimChute()
{
  chuteServo.write(SAMPLES[arrayIndexToAimAt][5]);  
}

// void clearScreen() {
//   for (int i = 0; i <= 80; i++) {
//     Serial.println();
//     //delay(10);
//   }
// }

void waitForUser()
{
  Serial.println();
  Serial.println("-----========   Waiting for a single character to be sent before proceeding     ======------");
  while(Serial.available() == 0)
  {
    delay(1);
  }
  Serial.read();
  Serial.read();
  Serial.read();
}
