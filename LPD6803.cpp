#include "LPD6803.h"

using namespace LPD6803;
/*****************************************************************************
 * Example to control LPD6803-based RGB LED Modules in a strand
 * Original code by Bliptronics.com Ben Moyes 2009
 * Use this as you wish, but please give credit, or at least buy some of my LEDs!
 *
 * Code cleaned up and Object-ified by ladyada, should be a bit easier to use
 *
 * Library Optimized for fast refresh rates 2011 by michu@neophob.com
 *****************************************************************************/

uint8_t LPD6803::Color::RGBOrder[3] = {1, 2, 0};

// the arrays of ints that hold each LED's 15 bit color values
static Color *pixels;
static Color *next_pixels;
static Color *lerp_start_pixels;
static uint16_t numLEDs;

static uint8_t dataPin, clockPin;
 
enum lpd6803mode {
    START,
    HEADER,
    DATA,
    DONE
};

static lpd6803mode SendMode;   // Used in interrupt 0=start,1=header,2=data,3=data done
static byte  BitCount;   // Used in interrupt
static uint16_t  LedIndex;   // Used in interrupt - Which LED we are sending.
static byte  BlankCounter;  //Used in interrupt.

static byte lastdata = 0;
static uint16_t swapAsap = 0;   //flag to indicate that the colors need an update asap
static long period = 0;

//Interrupt routine.
//Frequency was set in setup(). Called once for every bit of data sent
//In your code, set global Sendmode to 0 to re-send the data to the pixels
//Otherwise it will just send clocks.
void LedOut() {
    // PORTB |= _BV(5);    // port 13 LED for timing debug
    switch(SendMode) {
        case DONE:            //Done..just send clocks with zero data
            if (swapAsap>0) {
                if(!BlankCounter)    //AS SOON AS CURRENT pwm IS DONE. BlankCounter 
                {
                        BitCount = 0;
                        LedIndex = swapAsap;  //set current led
                        SendMode = HEADER;
                        swapAsap = 0;
                }       
            }
            break;

        case DATA:               //Sending Data
            if ((1 << (15-BitCount)) & pixels[LedIndex].value) {
                if (!lastdata) {     // digitalwrites take a long time, avoid if possible
                        // If not the first bit then output the next bits 
                        // (Starting with MSB bit 15 down.)
                        digitalWrite(dataPin, 1);
                        lastdata = 1;
                }
            } else {
                if (lastdata) {       // digitalwrites take a long time, avoid if possible
                        digitalWrite(dataPin, 0);
                        lastdata = 0;
                }
            }
            BitCount++;
            
            if(BitCount == 16)    //Last bit?
            {
                LedIndex++;        //Move to next LED
                if (LedIndex < numLEDs) //Still more leds to go or are we done?
                {
                    BitCount=0;      //Start from the fist bit of the next LED
                } else {
                        // no longer sending data, set the data pin low
                        digitalWrite(dataPin, 0);
                        lastdata = 0; // this is a lite optimization
                        SendMode = DONE;  //No more LEDs to go, we are done!
                }
            }
            break;      
        case HEADER:            //Header
            if (BitCount < 32) {
                digitalWrite(dataPin, 0);
                lastdata = 0;
                BitCount++;
                if (BitCount==32) {
                        SendMode = DATA;      //If this was the last bit of header then move on to data.
                        LedIndex = 0;
                        BitCount = 0;
                }
            }
            break;
        case START:            //Start
            if (!BlankCounter)    //AS SOON AS CURRENT pwm IS DONE. BlankCounter 
            {
                BitCount = 0;
                LedIndex = 0;
                SendMode = HEADER; 
            }  
            break;   
    }

    // Clock out data (or clock LEDs)
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
    
    //Keep track of where the LEDs are at in their pwm cycle. 
    BlankCounter++;

    timer0_write(ESP.getCycleCount() + period);
    // PORTB &= ~_BV(5);   // pin 13 digital output debug
}

//---
LEDStrip::LEDStrip(uint16_t n, uint8_t dpin, uint8_t cpin) {
    dataPin = dpin;
    clockPin = cpin;
    numLEDs = n;

    pixels = (Color *)malloc(sizeof(Color) * numLEDs);
    next_pixels = (Color *)malloc(sizeof(Color) * numLEDs);
    for(int i = 0; i < numLEDs; i++) {
        pixels[i] = Color(0);
        next_pixels[i] = Color(0);
    }

    SendMode = START;
    BitCount = LedIndex = BlankCounter = 0;
    cpumax = 50;
}

//---
void LEDStrip::begin(void) {
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    
    setCPUmax(cpumax);
    
    noInterrupts();
    timer0_isr_init();
    timer0_attachInterrupt(LedOut);
    interrupts();
}

//---
uint16_t LEDStrip::numPixels(void) {
    return numLEDs;
}

//---
void LEDStrip::setCPUmax(uint8_t m) {
    cpumax = m;

    // each clock out takes 20 microseconds max
    period = 100;
    period *= 20;     // 20 microseconds per
    period *= 80;     // 80 Cycles per microsecond
    period /= m;

    noInterrupts();
    timer0_write(ESP.getCycleCount() + period);
    interrupts();
}

//---
void LEDStrip::show(void) {
    memcpy(pixels, next_pixels, sizeof(uint16_t) * numLEDs);
    SendMode = START;
}

uint16_t LEDStrip::getPixelColor(uint16_t n) {
    return pixels[n].value;
}

void LEDStrip::setPixel(uint16_t n, Color c) {
    setPixel(n, c.value);
}

void LEDStrip::setPixel(uint16_t n, uint16_t c) {
    if (n > numLEDs) return;
    next_pixels[n].value = 0x8000 | c;
}

void LEDStrip::LerpRange(uint16_t i_start, uint16_t i_end, uint16_t *values) {
  //struct Color color;
  //uint8_t i_start;
  //uint8_t i_end;
  //uint8_t steps;
  //uint8_t t_delay;
  //readInto((char *)&color, sizeof(color));
  //i_start = readSync();
  //i_end = readSync();
  //steps = readSync();
  //t_delay = readSync();
  //Color past[i_end - i_start + 1];
  //memcpy(past, &leds[i_start], sizeof(past));
  //for(int i = 0; i < steps - 1; i++) {
  //  for(int j = 0; j <= i_end - i_start; j++) {
  //    leds[j + i_start].r = (color.r - past[j].r) * i / steps + past[j].r;
  //    leds[j + i_start].g = (color.g - past[j].g) * i / steps + past[j].g;
  //    leds[j + i_start].b = (color.b - past[j].b) * i / steps + past[j].b;
  //  }
  //  delay(t_delay);
  //}
  //for(int i = i_start; i <= i_end; i++) {
  //  leds[i].r = color.r;
  //  leds[i].g = color.g;
  //  leds[i].b = color.b;
  //}
}