/*
 * speaker_pcmMoby
 *
 * Plays 8-bit PCM audio on pin 11 using pulse-width modulation (PWM).
 * For Arduino with Atmega168 at 16 MHz.
 *
 * Uses two timers. The first changes the sample value 8000 times a second.
 * The second holds pin 11 high for 0-255 ticks out of a 256-tick cycle,
 * depending on sample value. The second timer repeats 62500 times per second
 * (16000000 / 256), much faster than the playback rate (8000 Hz), so
 * it almost sounds halfway decent, just really quiet on a PC speaker.
 *
 * Takes over Timer 1 (16-bit) for the 8000 Hz timer. This breaks PWM
 * (analogWrite()) for Arduino pins 9 and 10. Takes Timer 2 (8-bit)
 * for the pulse width modulation, breaking PWM for pins 11 & 3.
 *
 * References:
 *     http://www.uchobby.com/index.php/2007/11/11/arduino-sound-part-1/
 *     http://www.atmel.com/dyn/resources/prod_documents/doc2542.pdf
 *     http://www.evilmadscientist.com/article.php/avrdac
 *     http://gonium.net/md/2006/12/27/i-will-think-before-i-code/
 *     http://fly.cc.fer.hr/GDM/articles/sndmus/speaker2.html
 *     http://www.gamedev.net/reference/articles/article442.asp
 *
 * Timer Mapping:
 *     The original code was written for the ATmega168.  I don't have one
 *     so I assume it is the same as the ATmega386.
 *     
 *   ATMega368: has 3 timers
 *     Timer 0: pins 5,6
 *     Timer 1: pins 9,10
 *     Timer 2: pins 11,3
 *   ATMega2560: has 5 times  
 *     Timer 0: pin 13,4
 *     Timer 1: pins 12,11
 *     Timer 2: pins 10,9
 *     Timer 3: pins 5,3,2
 *     Timer 4: pins 8,7,6
 *
 * Michael Smith <michael@hurts.ca>
 * Updates by William Garrison <mobydisk@mobydisk.com>
 * - Added stopPlayback() and finishPlayback() methods
 */

/*
 * The audio data needs to be unsigned, 8-bit, 8000-16000 Hz, and small enough
 * to fit in flash.  No individual sound can exceed 32676 bytes due to limitations
 * of the avr-gcc compiler.
 *
 * sounddata.h should look like this:
 *     const int sounddata_length=10000;
 *     const unsigned char sounddata_data[] PROGMEM = { ..... };
 *
 */

#ifndef __PCM2560_H__
#define __PCM2560_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  unsigned char const *data;
  int      length;
} pcm2560_entry;

// Start playing audio
void startPlayback(unsigned char const *data, int length, unsigned int sampleRate);
void startPlayback_multiple(const pcm2560_entry* const playList, int length, unsigned int sampleRate);
void stopPlayback();
void finishPlayback();
bool isPlaying();

#ifdef __cplusplus
}
#endif

#endif
