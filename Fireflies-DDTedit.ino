////////////////////////////////////////////////////////////////////////////////////////////////////
// Realistic FireFlies / Lightning Bugs                                                           //
// Created by Robert Smith                                                                        //
// https://www.youtube.com/c/robsmithdev                                                          //
// https://robsmithdev.co.uk                                                                      //
//Rich DDT edit to ignore power, fade smoother & make all yellow; modify DEFINES to match strip   //
////////////////////////////////////////////////////////////////////////////////////////////////////
// Designed for ATMega328 microcontrollers.  The 'sleep' code may need modifying for others

#include <FastLED.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

// The data pin for the LEDs
#define LED_PIN     13

// Number of LEDs on the cable
#define NUM_LEDS    200

// Fade in/out time in milliseconds
#define FADETIME    120

// Maximum number of visible flies at a time
#define NUM_FLIES   55

// An array of LED values
CRGB leds[NUM_LEDS];

// A class to manage each fly
class Fly {  
  private:
    bool isYellowType;                // A yellow one flashes and moves, the white ones just twinkle  
    unsigned long when;               // When it should appear
    uint16_t duration;                 // How long for
    uint8_t targetLED;                // Which LED to control
  
    // For 'yellow' types
    uint8_t counter;                  // How many LEDs it can move along
    bool movementDirection;           // And in which direction (true is forward)
  
    // Returns the current brightness at this time, or -1 if the fly is 'dead'
    uint8_t getBrightness(const unsigned long now) {
      // Calculate elapsed time
      long elapsed = now - when;

      // Not on yet? brightness is 0
      if (elapsed<0) return 0;      
      
      // Fade in?
      if (elapsed < FADETIME) return map(elapsed,0,FADETIME,0,255);
      elapsed -= FADETIME;
      
      // Stay on during the duation
      if (elapsed < duration) return 255;
      elapsed -= duration;
  
      // Fade out
      if (elapsed >= duration) return 0;
      return map(elapsed,0,FADETIME,255,0);
    }
  
    // Get the colour we should show
    CRGB getColor(uint8_t brightness) {
      if (isYellowType) {
        return CRGB(0, brightness, brightness);
      } else {
        return CRGB(0, brightness, brightness);
      }
    }
  
    // Handle the dead fly, depending on which type it is
    void handleDeadFly(const unsigned long now) {
      // If ran out of count, or its not a yellow one
      if ((counter==0) || (!isYellowType)) {
        reset(now);
      } else {
        // Move to the next one
        if (movementDirection) targetLED++; else targetLED--;
        counter--;
        when = now + 1000;  // see again 1 second from now
      }
    }
    
  public:

   // set which type this is
   void setYellow(bool isYellowType) {
      this->isYellowType = isYellowType;
   }

    // Reset the fly
    reset(const unsigned long now, const bool longerInitialDelay = false) {
      when = now + (random(1,5) * 1000);   // upto 5 seconds before used again, lower for more at once
      if (longerInitialDelay) when += random(1,10)*1000;  // this allows them to gracefully appear rather than all at once
      
      if (isYellowType) {      
        movementDirection = random(0,2)==1;   // Movement direction      
        counter = random(2,10);       // Number of movements
  
        if (movementDirection)  // forward?
          targetLED = random(NUM_LEDS/2,NUM_LEDS-counter);      // To start anywhere remove the NUM_LEDS/2
        else // backwards
          targetLED = random((NUM_LEDS/2) + counter,NUM_LEDS);  // To start anywhere remove the (NUM_LEDS/2)

         duration = random(50,100);

      } else {
        // Random flash
        duration = random(50,100);
        targetLED = random(0,NUM_LEDS/2);  // If you want these to appear anywhere, remove the /2
      }    

      duration += FADETIME * 2;
    }


  // Run the fly
  void run(const unsigned long now) {
    long elapsed = now - when;
    
    // Has animation elapsed?
    if (elapsed > duration + FADETIME + FADETIME) {
      // Ensure its off
      leds[targetLED] = CRGB(0,0,0);
      // Reset
      handleDeadFly(now);
    }

    // Animate the LED
    leds[targetLED] = getColor(getBrightness(now));
  }
};


Fly flies[NUM_FLIES];
bool inDayMode = true;


void prepareAllFlies(const unsigned long now) {
    randomSeed(analogRead(A1)); // seed based off random noise as the pin isn't connected

    // Reset all flies
    for (uint8_t fly=0; fly<NUM_FLIES; fly++) { 
	  // This gives us a small number of yellow ones
      flies[fly].setYellow( fly < (NUM_FLIES/4) );
      flies[fly].reset(now, true);
    }  
}

void setup() {
    delay( 300 );
    FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  10 );

    pinMode(A2, INPUT);
    pinMode(A1, INPUT);
    pinMode(A0, INPUT);
    pinMode(LED_PIN, OUTPUT);

    prepareAllFlies(millis());
}

// watchdog interrupt, just used to disable the watchdog
ISR (WDT_vect) 
{
   wdt_disable();  // disable watchdog
}  

void loop()
{

      // Run the firefly simulation
      unsigned long now = millis();
      
	  // Update each fly
      for (uint8_t a=0; a<NUM_FLIES; a++) 
        flies[a].run(now);

      // Set brightness based on the value from A2 (variable resistor between 0 and 5v)
      FastLED.setBrightness(map(analogRead(A2),0,1023,10,255));
	  
	  // Update output
      FastLED.show();
	  
	  // And pause
      FastLED.delay(10);      
    
}
