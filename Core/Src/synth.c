/*
 * synth.c
 *
 * Simple polyphonic synthesizer from scratch.
 *
 *  Created on: 2025-03-16
 *  Updated on: 2025-03-16
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2025, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#include "synth.h"

FAST_BSS synth_voice_t voices[SYNTH_POLYPHONY];

/** Initialize our synthesizer engine
 * Does:
 * 1. Initializes all our voices
 * 1A. Initializes all our tone generators
 */
void synth_init(uint16_t sample_rate) {
  for (int v = 0; v < SYNTH_POLYPHONY; v++) {
    voices[v].note = 0;
    voices[v].state = voice_off;
    tonegen_init(&voices[v].tonegen, sample_rate);
    tonegen_set(&voices[v].tonegen, 1024, 0); // Frequency, Amplitude
  }
}

/** Finds an unused voice number and returns its index, or negative if no
 * available voices. If there is already a playing note of the same value,
 * we'll return that voice number instead.
 */
static int find_voice(uint8_t note) {
  int empty = -1; // A voice that is empty
  for (int v = 0; v < SYNTH_POLYPHONY; v++) {
    if (voices[v].state == voice_off && empty < 0) {
      empty = v;
    } else if (voices[v].state == voice_on && voices[v].note == note) {
      return v;
    }
  }
  return empty;
}

/** Take new MIDI inputs and handle them for our synth.
 *
 * We assume only one of each note value can be playing at a time.
 *
 * If we run out of voices, we just don't play new notes (i.e., we
 * don't steal old ones).
 *
 * If a note on comes for a note already playing, we restart the
 * note playing.
 */
void synth_process_midi(midi_message *mm) {
  int v; // Voice number
  // FIXME: magic number 0xF0
  int midi_type = mm->type & 0xF0;

  if (midi_type == MIDI_NOTE_ON && mm->velocity == 0) {
    midi_type = MIDI_NOTE_OFF;
  }

  // Update our notes playing
  if (midi_type == MIDI_NOTE_ON) {
    v = find_voice(mm->note);
    if (v >= 0) {
      // Start or restart a note playing
      // TODO: Get rid of the / 100 by pre-calculating the frequencies, or use that exact
      // x100 frequency since our sample rate could actually handle it
      // Velocity is 0-127 (0 being off), so 127 * 250 = 31,750,
      // which is below 32,767, our max 16-bit integer.
      // We may want to allow a little bit more headroom by multiplying by less than 250.
      // FIXME: Magic numbers
      tonegen_set(&voices[v].tonegen, midi_note_freqX100[mm->note] / 100, mm->velocity * 250);
      voices[v].state = voice_on;
      voices[v].note = mm->note;
    }

  } else if (midi_type == MIDI_NOTE_OFF) { ////////////////////////////////////////////////
    // NOTE: We assume that there is only one note playing of each note.
    v = find_voice(mm->note);
    // If it didn't find this note to turn off, then ignore it.
    if (v >= 0 && voices[v].state != voice_off) {
      voices[v].state = voice_off;
      // Is this actually necessary?
      tonegen_set(&voices[v].tonegen, voices[v].tonegen.desired_freq, 0);
    }
  }
} // synth_process_midi()

/** Handle regular checks for our synth.
 * TODO:
 * 1. Make it show which voices are playing
 * 2. Make it show which notes are playing
 * 3. Make it show if there was any clipping
 */
void synth_check() {}

/** Fill up an output sound buffer with a specified # of samples.
 * This will have to be heavily optimized, and maybe made into something
 * that is more efficient or pre-calculates during synth_check, or something.
 * Or maybe the caller makes multiple calls to it over several iterations.
 *
 * This does a very simple mixer: it clips values that are too high/low.
 */
void synth_fill(int16_t *buf, size_t samples) {
  int32_t acc; // accumulator

  for (;;) {
    // The mixed value - sum of all samples for voices playing
    acc = 0;
    for (int v = 0; v < SYNTH_POLYPHONY; v++) {
      if (voices[v].state != voice_off) {
        acc += tonegen_next_sample(&voices[v].tonegen);
      }
    }
    // Now, clip the mixed output value to the actual range
    // FIXME: Magic numbers
    // TODO: See how frequently we get clipping
    if (acc > 32767)
      acc = 32767;
    else if (acc < -32768)
      acc = -32768;
    // And output that value
    *buf = (int16_t)acc;
    buf++;
    if (samples == 0)
      return;
    samples--;
  }
} // synth_fill
