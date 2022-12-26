/*
 * speaker_pcmMoby
 *
 * This is pcm_2560, but I changed it back to work with plain Arduino Uno/Nano instead of the mega.
 * I did this because I liked the data structures and functions, but I'm using a nano for this project
 * Changes from pcm_2560:
 * - uint32_t changed to unsigned char const *
 * - _far removed from the method names
 * - Added stopPlayback() and finishPlayback() methods
 * This could be trivially combined with the original, but I don't have time to test that right now before Halloween!
 */

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "Arduino.h"
#include "PCMMoby.h"

/*
 * The audio data needs to be unsigned, 8-bit, 8000 Hz, and small enough
 * to fit in flash. 10000-13000 samples is about the limit.
 *
 * sounddata.h should look like this:
 *     const int pcm_length=10000;
 *     const unsigned char pcm_data[] PROGMEM = { ..... };
 *
 */

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  // Since OCR2A controls pins 10 and 9
  #define DEFAULT_SPEAKER_PIN 10
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  // OCR2A controls pins 11 and 3
  #define DEFAULT_SPEAKER_PIN 11
#endif

// Temporary: Allows me to make these values public so I can monitor them
//#define DIAGNOSTIC_PRIVATE static
#define DIAGNOSTIC_PRIVATE

DIAGNOSTIC_PRIVATE int        speakerPin = DEFAULT_SPEAKER_PIN;
DIAGNOSTIC_PRIVATE unsigned char const *pcm_data=0;         // 32-bit address of the 8-bit PCM audio data
DIAGNOSTIC_PRIVATE uint16_t   pcm_length=0;                 // length of the 8-bit PCM audio data, size limited to 32k
DIAGNOSTIC_PRIVATE uint16_t   current_pcm_offset=0;         // Offset into the currently playing PCM audio data, size limited to 32k
DIAGNOSTIC_PRIVATE byte       lastSample;                   // The last 8-bit value in the sample, used for ramping
DIAGNOSTIC_PRIVATE bool       pcmPlaying=false;             // True when playing a sound, not when ramping down because that might mess-up someone's timing

// The arduino gcc compiler does not allow declaring structures greater that 32767 bytes.  This is too bad since
// many Arduinos have >32k of addressable memory, and the CPU is capable of handling 32-bit addresses and offsets.
// So sound data >32k must be done in multiple 32k blocks.  But there is no guarantee that the compiler will place
// those blocks together.  So to play sounds >32k, we must have a list of multiple blocks and chain them together.

DIAGNOSTIC_PRIVATE const pcm2560_entry * pcmPlayList=NULL;  // A list of PCM data blocks to chain together as one, to overcome the compiler's 32k limitation.
DIAGNOSTIC_PRIVATE int   pcmPlayListLen=0;
DIAGNOSTIC_PRIVATE int   pcmPlayListOffset=0;

// This is called at 8000-16000 Hz to load the next sample.
ISR(TIMER1_COMPA_vect) {
  if (pcmPlaying)
  {
    // Are we at the end of the sound data?
    if (current_pcm_offset >= pcm_length) {
      
      // Do we have another entry in the playlist?
      if (pcmPlayListOffset < pcmPlayListLen-1) {
        // Move to the next item in the playlist
        ++pcmPlayListOffset;
        pcm_data  = pcmPlayList[pcmPlayListOffset].data;
        pcm_length= pcmPlayList[pcmPlayListOffset].length;
        // Start playing it
        current_pcm_offset = 0;
        lastSample = pgm_read_byte(pcm_data + pcm_length -1); // I don't like doing this each time, it's redundant.
        // Now play the first sample
        OCR2A = pgm_read_byte(pcm_data + current_pcm_offset);
      }
  
      // Are we done ramping down the sound?
      else if (current_pcm_offset == pcm_length + lastSample) {
        pcmPlaying = false;
        finishPlayback();
      }
      
      // Ramping down the sound
      else {
        // Ramp down to zero to reduce the click at the end of playback.
        // - Don't set pcmPlaying = false here since that disables the interrupt.
        // - This means the isPlaying() will return true until the ramp down is completed.  If that isn't
        //   desirable then we need two flags, one to indicate to the caller that the audio is effectively
        //   done, and the other to make the ISR stop doing work.
        OCR2A = pcm_length + lastSample - current_pcm_offset;
      }
    }
    // No, so just play the sound
    else {
      OCR2A = pgm_read_byte(pcm_data + current_pcm_offset);
    }
    
    ++current_pcm_offset;
  }
}

// Start playing the sound at the specified sample rate
// INPUTS: pcm_data, pcm_length, pcmPlayListLen, pcmPlayListOffset <-- must already be set
static void startISR(unsigned int sampleRate)
{
  pinMode(speakerPin, OUTPUT);
  
  // Set up Timer 2 to do pulse width modulation on the speaker
  // pin.
  
  // Use internal clock (datasheet p.160)
  ASSR &= ~(_BV(EXCLK) | _BV(AS2));
  
  // Set fast PWM mode  (p.157)
  TCCR2A |= _BV(WGM21) | _BV(WGM20);
  TCCR2B &= ~_BV(WGM22);
  
  // Do non-inverting PWM on pin OC2A (p.155)
  // On the Arduino Uno this is pin 11, on the Mega this is pin 10.
  TCCR2A = (TCCR2A | _BV(COM2A1)) & ~_BV(COM2A0);
  TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
  
  // No prescaler (p.158)
  TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
  
  // Set initial pulse width to the first sample.
  OCR2A = pgm_read_byte(pcm_data);
  
  
  // Set up Timer 1 to send a sample every interrupt.
  
  cli();
  
  // Set CTC mode (Clear Timer on Compare Match) (p.133)
  // Have to set OCR1A *after*, otherwise it gets reset to 0!
  TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
  TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));
  
  // No prescaler (p.134)
  TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
  
  // Set the compare register (OCR1A).
  // OCR1A is a 16-bit register, so we have to do this with
  // interrupts disabled to be safe.
  // - Confirmed to work with 8000:  // 16000000 / 8000 = 2000
  // - Confirmed to work with 16000: // 16000000 / 16000 = 1000
  OCR1A = F_CPU / sampleRate;    
  
  // Enable interrupt when TCNT1 == OCR1A (p.136)
  TIMSK1 |= _BV(OCIE1A);
  
  lastSample = pgm_read_byte(pcm_data + pcm_length -1);
  current_pcm_offset = 0;
  pcmPlaying = true;
  sei();
}

static void stopISR()
{
  // Disable playback per-sample interrupt.
  TIMSK1 &= ~_BV(OCIE1A);
  
  // Disable the per-sample timer completely.
  TCCR1B &= ~_BV(CS10);
  
  // Disable the PWM timer.
  TCCR2B &= ~_BV(CS10);
  
  digitalWrite(speakerPin, LOW);
  pcmPlaying = false;
}

void startPlayback(unsigned char const *data, int length, unsigned int sampleRate)
{
  cli();
  // Disable playlist
  pcmPlayList = NULL;
  pcmPlayListLen = pcmPlayListOffset = 0;

  // Set sound data
  pcm_data = data;
  pcm_length = length;
  sei();
  startISR(sampleRate);
}

void startPlayback_multiple(const pcm2560_entry* const playList, int length, unsigned int sampleRate)
{
  // Set playlist
  pcmPlayList = playList;
  pcmPlayListLen = length;
  pcmPlayListOffset = 0;
  if (length>0)
  {
    // Set sound data
    pcm_data = playList[0].data;
    pcm_length = playList[0].length;

    startISR(sampleRate);
  }
}

void finishPlayback()
{
  cli();
  // Disable playlist
  pcmPlayList = NULL;
  pcmPlayListLen = pcmPlayListOffset = 0;
  sei();
}

void stopPlayback()
{
  cli();
  // Disable playlist
  pcmPlayList = NULL;
  pcmPlayListLen = pcmPlayListOffset = 0;

  // Skip to the end of the sound.  This causes the ramp-down and prevents clicks
  current_pcm_offset = pcm_length;
  sei();
}


bool isPlaying()
{
  return pcmPlaying;
}
