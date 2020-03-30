//note. I2C Clock is hardware inputs, in this Arduino Nano CLK A5, DIO A4

/*
 * Additional Libraries used
 * 
 * https://github.com/avishorp/TM1637
 * https://github.com/adafruit/RTClib
 * https://github.com/adafruit/Adafruit_DotStar

*/
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include <Arduino.h>
#include <TM1637Display.h>

#include <Adafruit_DotStar.h> // this would be fine with 
// Because conditional #includes don't work w/Arduino sketches...
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
//#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET

#define NUMPIXELS 60 // Number of LEDs in strip

// LEDs strip pins:
#define DATAPIN    2
#define CLOCKPIN   3
Adafruit_DotStar strip = Adafruit_DotStar(
  NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG); // my strip is weird and ends up being GRB with this setting, may need to fix depending on your strip

RTC_DS3231 rtc;
//TM1637 pins
#define CLK 9
#define DIO 10
#define DIO1 6
#define CLK1 7
// #define AM 0b01110111 
// #define PM 0b01110011 // for tracking AM/PM
// values to toggle colon on clock
#define ColonOn 0b01000000  
#define ColonOff 0b00000000

// For Smoothing the potentiometer readings. Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the output will respond to
// the input. Using a constant rather than a normal variable lets us use this
// value to determine the size of the readings array.
const int numReadings = 50;
int readings[numReadings];      // the readings from the potentiometer input
int readIndex = 0;              // the index of the current reading
uint16_t total = 0;                  // the running total
int averageAnalog = 0;         // the average

const long ClockInterval = 500; // time value to toggle colon on clock
const long ButtonInterval = 1500; // time value to measure a long press of button
const long BlinkerInterval = 350; // time value to blink display during time setting
const int buttonPin = 12;     // the number of the pushbutton pin
unsigned long previousClockMillis = 0; //Time tracker for clock 
unsigned long previousButtonMillis = 0; //Timer tracker for button 
unsigned long previousBlinkerMillis = 0; //Timer tracker for time Set blinking 
uint8_t data[4];
int buttonState = 0;         // variable for reading the pushbutton status
int hh, mm, ss, mn, yr, dy, last_ss;  //hours, minutes seconds, month, year, day
boolean ColonStatus = 0;
boolean BlinkStatus = 0;
boolean SetClock = 0;
boolean longPress = 0;
boolean displaysON = 0;
uint8_t Colon[] = {ColonOn, ColonOff};
uint8_t displayBlink[] ={0xff, 0x99};
unsigned long currentMillis;
const uint8_t daysofMonth[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};  //during clock setting limits number of days for each month
const String currentMonth[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// these units are "pseudo" I2C, so each needs there own pins, instead of beaing able to share I2C with the RT clock chip
TM1637Display display0(CLK, DIO);
TM1637Display display1(CLK1, DIO1);

uint8_t ClearScreen[] = { 0x00, 0x00, 0xf00, 0x00 }; //to clear 7-segment displays
uint32_t secondcolors[2]; //stores the current colors of the clock seconds
boolean clockcolorToggler = 0;
uint8_t currentstripBrightness[] = {0,50};
unsigned int ColorDial; // track potentiometer value to map to LED colors
int startcolorA = 57; int startcolorB = 215; //intial colors for LEDs

int inputPin = A0; //potentiometer pin

void setup () {
  // initialize all the potentiometer readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;}
    
  secondcolors[0] = Wheel(startcolorA);
  secondcolors[1] = Wheel(startcolorB);
  strip.setBrightness(currentstripBrightness[1]);
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
  pinMode(buttonPin, INPUT);
  display0.setBrightness(0xff);
  display1.setBrightness(0xff);
  data[0] = 0b01000000;
  Serial.begin(9600);
  yr = 2018;
  last_ss = 1; //tracks the previous second so LED colors dont toggle repeatedly during the first second of each minute
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  DateTime now = rtc.now();
 
    
/*if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
     rtc.adjust(DateTime(2020, 3, 13, 19, 29, 30));
}*/
}

// hours and minutes to single 4-digit value
int timeToDecimal(int hours, int minutes) {
  if (hours > 12) hours -= 12;
  return hours*100 + minutes;
  }
  
void loop () {
    //read current time from millis and rtc
    currentMillis = millis();
    DateTime now = rtc.now();
    hh = now.hour(), DEC;
    mm = now.minute(), DEC;
    ss = now.second(), DEC;
    mn = now.month(), DEC;
    dy = now.day(), DEC;

    if (hh == 0) {hh = 12;} // this is to make 12 AM ream as 12 instead of 0

    if (currentMillis - previousClockMillis >= ClockInterval) {
    // save the last time colon was on
    previousClockMillis = currentMillis;
    // show time on LED display 0
    display0.showNumberDecEx(timeToDecimal(hh,mm), Colon[ColonStatus], false);
    // show Month/Day on LED display 1
    display1.showNumberDecEx(timeToDecimal(mn,dy), Colon[ColonOff], false);
    ColonStatus = !ColonStatus; //turn colon on or off
    }
    
    // alternate Dotstar LEDs color at beginning of each new minute
    if (ss == 0 && (ss - last_ss) != 0) {
      clockcolorToggler = !clockcolorToggler;
    }

    displayclockseconds();
    strip.show();
    buttonCheck(); //check the button
    //if held for 1.5 second, enter clock setting mode
    if (longPress) {
      longPress = false;
      previousBlinkerMillis = millis();
      ClockUpdate();
      }
    last_ss = ss; // tracks first loop after ss changes so color doesnt toggle continuously during first second
    if (buttonState == HIGH && !longPress) { 
      delay(100);
      buttonState = digitalRead(buttonPin);
      if (buttonState == LOW) {
        displaysON = !displaysON;
        display0.setBrightness(displayBlink[displaysON]);
        display1.setBrightness(displayBlink[displaysON]);
        strip.setBrightness(currentstripBrightness[!displaysON]);
        strip.show();
        }
      }
    
    //average the potentiometer reading and map it to a color value, conditional statements constrain values to 
    // between 40-500 amd 550-950. Lower value range creates a contrasting color scheme for LEDs, higher range uses 
    // a dimmed version of the first color, so clock is monochromatic
    
    analogAverager();
    if(averageAnalog <= 40) averageAnalog = 40;
    if(averageAnalog >= 500 && averageAnalog <= 525) averageAnalog = 500;
    if(averageAnalog > 525 && averageAnalog <= 550) averageAnalog = 550;
    if(averageAnalog >= 950) averageAnalog = 950;
    if(averageAnalog <= 500) {
      int newcolor = map(averageAnalog, 40, 500, 255, 0);
      if ((newcolor + startcolorA) > 255) {
        secondcolors[0] = Wheel(newcolor + startcolorA - 255); 
        }
      else secondcolors[0] = Wheel(newcolor + startcolorA);
  
      if ((newcolor + startcolorB) > 255) {
        secondcolors[1] = Wheel(newcolor + startcolorB - 255); 
        }
      else secondcolors[1] = Wheel(newcolor + startcolorB); 
    }
    
    else {
      int newcolor = map(averageAnalog, 550, 950, 255, 0);
      if ((newcolor + startcolorA) > 255) {
        secondcolors[0] = Wheel(newcolor + startcolorA - 255); 
        }
      else secondcolors[0] = Wheel(newcolor + startcolorA);
      secondcolors[1] = strip.Color((green(secondcolors[0]) >> 3), (red(secondcolors[0]) >> 3), (blue(secondcolors[0]) >> 3));      
    }
}

// tracks time button was pushed to determine if held for 1.5 seconds. Used  to begin setting the clock and when changing time parameters while setting
void buttonCheck() {
    buttonState = digitalRead(buttonPin);
    
    // if button is pressed now, but wasn't pressed last loop, capture current millis for button
    if (buttonState == HIGH && previousButtonMillis == 0) {
      previousButtonMillis = currentMillis;}
    
    // if button timer is running and button is no longer being pressed then reset timer status  
    if (buttonState == LOW && previousButtonMillis > 0) {previousButtonMillis = 0;}
    
    // if button is pressed, button timer is running and has been for more than 1 second
    if (buttonState == HIGH && previousButtonMillis > 0 && currentMillis - previousButtonMillis >= ButtonInterval) {
    previousButtonMillis = 0;
    Serial.println("Button held for two seconds");
    longPress = true; 
    }
 }


// Blink 7-segment display while setting the clock
void displayblinker() {
    if (currentMillis - previousBlinkerMillis >= BlinkerInterval) {
      display0.setBrightness(displayBlink[displaysON]);
      display1.setBrightness(displayBlink[displaysON]);
      displaysON = !displaysON;
      previousBlinkerMillis = currentMillis;
    }
}


// Start of Clock time updating 
void ClockUpdate() {
  display1.setSegments(ClearScreen, 4, 0);
  display0.showNumberDec(yr, false, 4, 0);
  delay(500);
  // set year
  while (longPress == false) {  // set year
    display0.showNumberDec(yr, false, 4, 0);
    currentMillis = millis();
    buttonCheck();
    displayblinker();
    if (buttonState == HIGH){
      delay(100);
      buttonState = digitalRead(buttonPin);
      if (buttonState == LOW) {
        yr += 1;        
        }
      }
    }
    
  Serial.println("Year Updated");
  Serial.println(yr);
  longPress = false;
  display0.setSegments(ClearScreen, 4, 0);
  previousBlinkerMillis = millis();
  
  //set Month
  display0.showNumberDec(mn, false, 2, 2);
  delay(500);
  while (longPress == false) { 
    display0.showNumberDec(mn, false, 2, 2);
    currentMillis = millis();
    buttonCheck();
    displayblinker();
    if (buttonState == HIGH){
      delay(100);
      buttonState = digitalRead(buttonPin);
      if (buttonState == LOW) {
        if (mn < 12) mn += 1;        
        else mn = 1;
        }
      }
    }
    
  Serial.println("Month Updated");
  Serial.println(currentMonth[mn-1]);  // Month array is 0-11, mn ranges 1-12
  longPress = false;
  display0.setSegments(ClearScreen, 4, 0);
  previousBlinkerMillis = millis();
  
  //set Day
  display0.showNumberDec(dy, false, 2, 2);
  delay(500);
  while (longPress == false) { 
    display0.showNumberDec(dy, false, 2, 2);
    currentMillis = millis();
    buttonCheck();
    displayblinker();
    if (buttonState == HIGH){
      delay(100);
      buttonState = digitalRead(buttonPin);
      if (buttonState == LOW) {
        if (dy < daysofMonth[mn-1]) dy += 1;        
        else dy = 1;
        }
      }
    }
    
  Serial.println("Day of Month Updated");
  Serial.println(dy); 
  longPress = false;
  display0.setSegments(ClearScreen, 4, 0); 
  previousBlinkerMillis = millis();   

     //set hour
  display0.showNumberDec(hh, false, 2, 2);
  delay(500);
  while (longPress == false) { 
    display0.showNumberDec(hh, false, 2, 2);
    currentMillis = millis();
    buttonCheck();
    displayblinker();
    if (buttonState == HIGH){
      delay(100);
      buttonState = digitalRead(buttonPin);
      if (buttonState == LOW) {
        if (hh < 23) hh += 1;        
        else hh = 0;
        }
      }
    }
    
  Serial.println("Hour Updated");
  Serial.println(hh); 
  longPress = false;
  display0.setSegments(ClearScreen, 4, 0); 
  previousBlinkerMillis = millis();   
  
   //set minutes
  display0.showNumberDec(mm, false, 2, 2);
  delay(500);
  while (longPress == false) { 
  display0.showNumberDec(mm, false, 2, 2);
  currentMillis = millis();
  buttonCheck();
  displayblinker();
  if (buttonState == HIGH){
    delay(100);
    buttonState = digitalRead(buttonPin);
    if (buttonState == LOW) {
      if (mm < 59) mm += 1;        
      else mm = 0;
      }
    }
  }
    
  Serial.println("Minutes Updated");
  Serial.println(mm); 
  longPress = false;
  display0.setSegments(ClearScreen, 4, 0);    

  previousBlinkerMillis = millis();
  Serial.println("Hold 1 second to confirm time");

  while (longPress == false) { 
  currentMillis = millis();
  display0.showNumberDecEx(timeToDecimal(hh,mm), Colon[ColonOff], false);
  display1.showNumberDecEx(timeToDecimal(mn,dy), Colon[ColonOff], false);
  buttonCheck();
  displayblinker();
  }
  // store the updated time in the RTC module
  rtc.adjust(DateTime(yr, mn, dy, hh, mm, 0)); 
  longPress = false; 
  }

// end of Clock time updating 

    
// Input a value 0 to 255 to get a color value. borrowed from Adafruit example to simplify LED colors.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/*
If you're dumb, like me, and glue the pixel strip on counterclockwise accidentally, use this instead to reverse the order the seconds tick off

void displayclockseconds() {
    //initialized the pixel colors, current color up to current second, and alternate color from there til 59 seconds
  for (uint8_t i = 0; i < ss; i++) {
    strip.setPixelColor(59-i, secondcolors[clockcolorToggler]);
    }
  for (uint8_t i = ss; i < 60; i++) {
    strip.setPixelColor(59-i, secondcolors[!clockcolorToggler]);
    }
    strip.show();
  }
 */
 
void displayclockseconds() {
    //use current color up to current second, and alternate color from there til 59 seconds
  for (uint8_t i = 0; i < ss; i++) {
    strip.setPixelColor(i, secondcolors[clockcolorToggler]);
    }
  for (uint8_t i = ss; i < 60; i++) {
    strip.setPixelColor(i, secondcolors[!clockcolorToggler]);
    }
    strip.show();
  }


// Since the potentiometer controls the color of the LEDs by reading analog value, this averages consecutive readings to reduce color flickering
void analogAverager(){
    // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead(inputPin);
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  averageAnalog = total / numReadings;
  // send it to the computer as ASCII digits
  delay(1);        // delay in between reads for stability
  }

uint8_t red(uint32_t c) {
  return (c >> 8);
}
uint8_t green(uint32_t c) {
  return (c >> 16);
}
uint8_t blue(uint32_t c) {
  return (c);
}
        
