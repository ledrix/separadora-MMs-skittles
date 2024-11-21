#include <Servo.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <FastLED.h>

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

Servo wheelServo;  // create a servo object to control a servo
Servo chuteServo;  // create another servo object to control a servo
Servo mixerServo;  // crete another servo object to control servo

const byte BOTAO_START_STOP = { 2 };
const byte LED_DIGITAL = { 3 };
const byte SENSOR_RODA = { 8 };
const byte SENSOR_TUBO = { 9 };
const byte VIBRA = { 10 };

const byte NUM_LEDS =  5; // QUANTIDADE DE LEDS ENDERECAVEIS
CRGB leds[NUM_LEDS];


const int wheelOverRun = 80; //decrease this if the skittle goes past the colour sensor or increase it if the skittle stops before it reaches the sensor
//270 para 90 --- 165 para 89 --- 110 para 88 -- 80 para 87

int arrayIndexToAimAt;

float redReading, greenReading, blueReading; //somewhere to store our colour sensor readings

const int arrayRows = 7;
const int arrayColumns = 6;
int SAMPLES[arrayRows][arrayColumns] = {  // array goes red value, green value, blue value, colour, difference to sample (update by code later), angle for depositor
  {133, 66,  44,  "Red", 0, 25}, //red -- ok
  {152,  56,  31,  "Orange", 0,55}, //orange -1 -- ok
  {118,  89,  28,  "Yellow", 0, 85}, //yellow -2 -- ok  
  {73,  116,  45,  "Green", 0, 115}, //green -3 -- ok
  {47,  89,  103,  "Blue", 0, 145}, //blue -4 
  {96,  84,  54, "Brown", 0, 175},  //brown -5  
  {97, 85, 54, "No Skittle", 0, 175}, //no skittle -6
}; 
// Determines the number of samples stored in the array
const byte samplesCount = sizeof(SAMPLES) / sizeof(SAMPLES[0]);

volatile bool toogle_start_stop = false; 

void setup()
{
  pinMode(SENSOR_RODA, INPUT);
  pinMode(SENSOR_TUBO, INPUT);
  pinMode(BOTAO_START_STOP, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), tootgle, FALLING);
  pinMode(VIBRA, OUTPUT);

  chuteServo.attach(5); // attaches the servo on pin d5 to the servo object
  chuteServo.write(75); // No Sittle default position 
  wheelServo.attach(6); // attaches the servo on pin d6 to the servo object
  wheelServo.write(92); // stop motor
  mixerServo.attach(7); // attaches the servo on pin d6 to the servo object
  mixerServo.write(91); // stop motor
 
  
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






void loop() {  

  if (toogle_start_stop == true) {

    chuteServo.attach(5); // attaches the servo on pin d5 to the servo object     
    wheelServo.attach(6); // attaches the servo on pin d6 to the servo object    
    mixerServo.attach(7); // attaches the servo on pin d6 to the servo object
    

    if (digitalRead(SENSOR_RODA) == HIGH) {
     wheelServo.write(87);
    }
    mixerServo.write(100);
    // static unsigned long debounce;
    // if ((millis() - debounce) > 10000) {    
    //   while (digitalRead(SENSOR_TUBO) == LOW) { 
    //     wheelServo.write(92);
    //     mixerServo.write(100);
    //     delay(1000);
    //     byte count = 0;     
    //     while (count < 3) {
    //     mixerServo.write(80);
    //     digitalWrite(VIBRA, HIGH);
    //     delay(200);
    //     mixerServo.write(91);
    //     digitalWrite(VIBRA, LOW);
    //     delay(200);
    //     count++; 
    //     }           
    //   }     
    //   debounce = millis();                        
    // }          
  
    if (digitalRead(SENSOR_RODA) == HIGH){

      delay(wheelOverRun); //allows the wheel to keep turning - adjust this value to ensure your skittle stops beenath the colour sensor.   
      wheelServo.write(92);  //stops the skittle wheel
      digitalWrite(VIBRA, LOW);    
      readColour();    
      matchColour();  
      arrayIndexToAimAt = findClosestSample();
      aimChute();
      delay(500); //gives the chute time to settle in place before we send the skittle down
      } else {
      //Serial.println("Skittle moving wheel not aligned wiht contact switch yet - please gently rotate by hand unless a reading is produced shortly");                 
      wheelServo.write(87);  //starts to turn the skittle collecting wheel
      digitalWrite(VIBRA, HIGH);
    
    }
  } else {

    wheelServo.detach();
    mixerServo.detach();
    chuteServo.detach();

      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(255, 255, 0);              
      }
      FastLED.show();
      delay(300);
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(0, 0, 0);        
      }
      FastLED.show();
      delay(300);
  }
     
  
}






void tootgle() {
   if(digitalRead(BOTAO_START_STOP) == LOW) {
    toogle_start_stop = !toogle_start_stop;
  }
}

void readColour()
  { 
    delay(300);  // takes 50ms to read
    tcs.getRGB(&redReading, &greenReading, &blueReading);   
    
  }

void matchColour() {
  int colourDistance;
  // Iterate through the array to find a matching colour sample
  for (byte i = 0; i < samplesCount; i++)
  {
   colourDistance = getColourVariance(redReading, greenReading, blueReading, SAMPLES[i][0], SAMPLES[i][1], SAMPLES[i][2], SAMPLES[i][3]);
   SAMPLES[i][4] = colourDistance;
  }   
}

int getColourVariance(int redSensor, int greenSensor, int blueSensor, int redSample, int greenSample, int blueSample, char *colour) {  

  int redDifference;
  redDifference = compareValues(redSensor, redSample, "Red ");
  int greenDifference;
  greenDifference = compareValues(greenSensor, greenSample, "Green ");
  int blueDifference;
  blueDifference = compareValues(blueSensor, blueSample, "Blue ");  
  int totalDifference;
  totalDifference = redDifference + greenDifference + blueDifference;  
  return totalDifference;

}

int compareValues(int sensorColour, int sampleColour, char *colour) {

  int difference;  
  difference = sensorColour - sampleColour;
  if (difference < 0)
  {
    difference = difference * (-1);
  }  
  return difference;

}

int findClosestSample() {

  int index = 0;
  for (int i = 0; i < arrayRows; i++) {    
    if (SAMPLES[i][4] < SAMPLES[index][4]) {
      index = i;const byte LED_DIGITAL = { 11 };
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
        leds[i] = CRGB(255, 70, 0);              
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
        leds[i] = CRGB(255, 0, 150); 
        FastLED.show();
      }
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

void aimChute() {
  chuteServo.write(SAMPLES[arrayIndexToAimAt][5]);  
}