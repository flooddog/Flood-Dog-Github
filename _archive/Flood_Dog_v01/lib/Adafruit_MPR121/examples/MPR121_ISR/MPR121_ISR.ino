/*********************************************************
This is a library for the MPR121 12-channel Capacitive touch sensor

Designed specifically to work with the MPR121 Breakout in the Adafruit shop 
  ----> https://www.adafruit.com/products/

These sensors use I2C communicate, at least 2 pins are required 
to interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.  
BSD license, all text above must be included in any redistribution
**********************************************************/

#if defined(PARTICLE)
#include "Particle.h"
const int pinIRQ = D6;
#else
#include <Wire.h>
const int pinIRQ = 2;
#endif
#include "Adafruit_MPR121.h"

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

volatile bool capTouched;
void capISR()
{
  capTouched = true;
}

void setup() 
{
  #if !defined(PARTICLE)
  while (!Serial);        // needed to keep leonardo/micro from starting too fast!
  #endif

  Serial.begin(9600);
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1)
    {
      #if defined(PARTICLE)
      Particle.process();
      #endif
    } 
    
  }
  Serial.println("MPR121 found!");

  #if defined(PARTICLE)
  pinMode(pinIRQ, INPUT_PULLUP);
  attachInterrupt(pinIRQ, capISR, FALLING);
  #else
  pinMode(pinIRQ, INPUT);
  digitalWrite(pinIRQ, HIGH); // attach pullup-resistor
  attachInterrupt(digitalPinToInterrupt(pinIRQ), capISR, FALLING);
  #endif
}

void loop() 
{
  if (capTouched)
  {
    capTouched = false;

    // Get the currently touched pads
    currtouched = cap.touched();
  
    for (uint8_t i=0; i<12; i++) {
      // it if *is* touched and *wasnt* touched before, alert!
      if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
        Serial.print(i); Serial.println(" touched");
      }
      // if it *was* touched and now *isnt*, alert!
      if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
        Serial.print(i); Serial.println(" released");
      }
    }

    // reset our state
    lasttouched = currtouched;
  
    // debugging info, what
    Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t\t 0x"); Serial.println(cap.touched(), HEX);
    Serial.print("Filt: ");
    for (uint8_t i=0; i<12; i++) {
      Serial.print(cap.filteredData(i)); Serial.print("\t");
    }
    Serial.println();
    Serial.print("Base: ");
    for (uint8_t i=0; i<12; i++) {
      Serial.print(cap.baselineData(i)); Serial.print("\t");
    }
    Serial.println();
  }
}
