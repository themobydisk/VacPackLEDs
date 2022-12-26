// Uncomment this line to get some serial output.  I don't want that in the production build because it slows the update rate.
//#define DEBUG

#include <FastLED.h>
#include "PCMMoby.h"
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include "vac_sustained_02 1s 16khz 8-bit unsigned.h"
#include "vac_shoot_01 16khz 8-bit unsigned.h"

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    6

// How many NeoPixels are attached to the Arduino?
// - I cut-off the last 6
#define LED_COUNT (144-6)

#define BRIGHTNESS 50

// Declare our NeoPixel strip object:
CRGB leds[LED_COUNT];

// Sound loop
pcm2560_entry VACPACK_LOOP[6];
pcm2560_entry VACPACK_SHOOT[6];

void setup() {
  memset(leds,0,sizeof(leds));
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);
  
  // Buttons
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  
  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("Hello world!");
  #endif

  // Setup a loop that plays the sound 6 times in a row.
  for (int i=0; i<sizeof(VACPACK_LOOP)/sizeof(*VACPACK_LOOP); i++)
    VACPACK_LOOP[i] = { PCM_VACPACK_LOOP, sizeof(PCM_VACPACK_LOOP) };

  for (int i=0; i<sizeof(VACPACK_SHOOT)/sizeof(*VACPACK_SHOOT); i++)
    VACPACK_SHOOT[i] = { PCM_VACPACK_SHOOT, sizeof(PCM_VACPACK_SHOOT) };
}

bool greenButtonPressedAlready = false;
bool redButtonPressedAlready = false;

void loop() {
  #ifdef DEBUG
    unsigned long before = millis();
  #endif

  bool greenButton = digitalRead(7);
  bool redButton = digitalRead(8);

  // If both buttons are pressed, just cancel it out.  This simplfies things.
  if (greenButton && redButton)
    greenButton = redButton = false;

  if (greenButton) {
    if (!greenButtonPressedAlready)
        startPlayback_multiple(VACPACK_LOOP, sizeof(VACPACK_LOOP)/sizeof(*VACPACK_LOOP), 20000); // This is supposed to be 16khz, but it plays slowly so I am compensating
    chase(8, 0xFFFF00, false);
  } 
  else if (redButton) {
    if (!redButtonPressedAlready)
        startPlayback_multiple(VACPACK_SHOOT, sizeof(VACPACK_SHOOT)/sizeof(*VACPACK_SHOOT), 14000); // This is supposed to be 16khz but lacks bass, so I'm playing it a bit slower
    chase(16, 0xFFFFFF, true);
  } else {
    // When they release a button, "nicely" stop the current sound
    // - You can change finishPlayback() to stopPlayback() if you prefer an immediate stop to the sound
    if (greenButtonPressedAlready || redButtonPressedAlready && isPlaying)
      finishPlayback();
    idlePulse();
  }

  greenButtonPressedAlready = greenButton;
  redButtonPressedAlready = redButton;

  // This effectively limits the update rate to 100fps
  delay(10);

  #ifdef DEBUG
    unsigned long after = millis();
    Serial.print("FPS=");
    Serial.println(1000/(after-before));
  #endif
}

void chase(byte colorShift, uint32_t whiteMask, bool direction)
{
  const int cycleTimeMS = 2000;
  const int numDots = 6;
  const int dotSpacing = LED_COUNT/numDots;
  const uint32_t FF = 0xFF;

  float t = (millis() % cycleTimeMS)/(float)cycleTimeMS;

  // This actually uses quite a lot of CPU time.  There are better ways.  WIP.
  for (int offset=0; offset<dotSpacing; offset++)
  {
    uint32_t color;

    // Gimme a range 0..1
    float scale = offset/(float)dotSpacing;
    
    // The top 25% is white, fading down to the desired color
    if (scale > 0.75)
    {
      color = FF << colorShift;

      scale = (scale-0.75)*4; // Rescale back to 0..1
      uint32_t whitish=static_cast<uint32_t>(scale*255) << 16
                      | static_cast<uint32_t>(scale*255) << 8
                      | static_cast<uint32_t>(scale*255);
      whitish &= whiteMask;
      whitish &= ~(FF << colorShift);

      color = whitish | color;
    }
    
    // The next quartile, the lights fade from the color down to black
    else if (scale > 0.5)
    {
      scale = (scale-0.5)*4;      // Rescale back to 0..1
      color = static_cast<uint32_t>(scale*255) << colorShift;
    }
    // The bottom half is just black
    else
    {
      color = 0;
    }
  
    for (int d=0; d<numDots; d++)
    {
      int dotIndex = d*dotSpacing;
      int finalIndex = (int)(dotIndex + offset + t*LED_COUNT) % LED_COUNT;
      if (direction == false)
        finalIndex = LED_COUNT-1 - finalIndex;
      leds[finalIndex] = color;
    }
  }

  FastLED.show();
}

void idlePulse()
{
  // Fade back-and-forth from dimmest to dimmest + range
  // NOTE: WS2812B response is very non-linear, and varies significantly based on the brightness
  const int fadeTimeMS = 4000;
  const int range = 32;
  const uint32_t FF = 50;

  // Range 0..1
  float t = (millis() % fadeTimeMS)/(float)fadeTimeMS;
  // Range -1..1
  t = (t-0.5)*2;
  // Reparameterize to spend more time around 0, so that the bright parts seem like pulses
  t = pow(t,5);
  // Now calculate a brightness within that range
  uint32_t brightness = abs(t)*range;
  
  const uint32_t color = (brightness << 16) | (FF << 8) | FF;

  #ifdef DEBUG
    Serial.print("t=");
    Serial.print(t);
    Serial.print(",brightness=");
    Serial.print(brightness);
    Serial.print(", color=");
    Serial.print(color,HEX);
    Serial.println();
  #endif

  for(int c=0; c<LED_COUNT; c++) {
    leds[c] = color;
  }
  FastLED.show();
}
