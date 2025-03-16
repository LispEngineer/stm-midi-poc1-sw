/*
 * synth.h
 *
 *  Created on: 2025-03-16
 *  Updated on: 2025-03-16
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2025, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#ifndef INC_SYNTH_H_
#define INC_SYNTH_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "realmain.h"
#include "tonegen.h"
#include "midi.h"

#define SYNTH_POLYPHONY 8

typedef enum voice_state_enum {
  voice_off = 0,
  voice_on = 1
} voice_state_t;

// Contains all the state of each of our polyphonic
// voices.
typedef struct synth_voice {
  // Is this voice playing?
  voice_state_t state;
  // What MIDI note number is it playing? (0-127)
  uint8_t note;
  // What is the tone generator state?
  tonegen_state tonegen;
  // TODO: Future:
  // Were are we in the envelope?
  // When did the voicing start?
  // When did the note off happen?
} synth_voice_t;

// Initialize our synthesizer engine
void synth_init(uint16_t sample_rate);

// Take new MIDI inputs and handle them for our synth
void synth_process_midi(midi_message *mm);

// Handle regular checks for our synth
void synth_check();

// Fill up an output sound buffer with a specified # of samples
void synth_fill(int16_t *buf, size_t samples);


#endif /* INC_SYNTH_H_ */
