/*
 * tonegen.c
 *
 *  Created on: Sep 9, 2024
 *  Updated on: 2024-09-08
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 *
 * Simple triangle wave tone generator for testing.
 */

#include <stdint.h>
#include "tonegen.h"

void tonegen_init(tonegen_state *tgs, uint32_t sample_rate) {
  tgs->sample_rate = sample_rate;
  tgs->desired_freq = 0;
  tgs->desired_ampl = 0;

  tgs->delta = 0;
  tgs->going_up = 1;

  tgs->last_is_left = 0;
  tgs->last_sample = 0;
}

void tonegen_set(tonegen_state *tgs, uint32_t desired_freq, int16_t desired_ampl) {
  if (desired_freq > tgs->sample_rate / 2)
    tgs->desired_freq = tgs->sample_rate / 2;
  else if (desired_freq < 1)
    // Prevent later divide by zero
    tgs->desired_freq = 1;
  else
    tgs->desired_freq = desired_freq;

  if (desired_ampl < 0)
    tgs->desired_ampl = 0;
  else
    tgs->desired_ampl = desired_ampl;

  // We need to get up and down at the desired_freq relative to
  // the sample rate.
  // Example: 32kHz, 1000 Hz = 32 steps to go up then down, or
  // 16 steps to go up and down then down and up

  // This first simple algorithm is going to be heavily aliased as it won't
  // make much difference between 1000 and 990 Hz with the
  // coarse math involved. Maybe use fixed precision math with some
  // extra decimals next try.

  // Also, by the time the desired freq gets to more than 1/5th the sample
  // rate, this delta maxes out.
  // Sample rate: 32,000; desired freq: 6,400; desired ampl: 32,000
  // samples = 32,000 / 6,400 = 5
  // delta = 32,000 / 5 * 4 = 25,600
  // --
  // Sample rate: 32,000; desired freq: 6,401; desired ampl: 32,000
  // samples = 32,000 / 6,401 = 4
  // delta = 32,000 / 4 * 4 = 32,000

  /* // OLD VERSION
  uint32_t samples = tgs->sample_rate / tgs->desired_freq;
  tgs->delta = tgs->desired_ampl / samples * 4; // Moving * 4 earlier doesn't help
  */

  // Scale by 256 (2^8) to get more accurate delta
  uint32_t samplesX256 = 256 * tgs->sample_rate / tgs->desired_freq;
  tgs->delta = tgs->desired_ampl * 4 * 256 / samplesX256;
}

int16_t tonegen_next_sample(tonegen_state *tgs) {
  // If we sent the left channel last time, the right channel
  // is always the same since we're being "mono"
  if (tgs->last_is_left) {
    tgs->last_is_left = 0;
    return tgs->last_sample;
  }

  // Check if we are changing direction (due to changed desired amplitude
  if (tgs->last_sample > tgs->desired_ampl) {
    tgs->going_up = 0;
  } else if (tgs->last_sample < -tgs->desired_ampl) {
    tgs->going_up = 1;
  }

  int32_t next_sample;

  if (tgs->going_up) {
    next_sample = tgs->last_sample + tgs->delta;
    if (next_sample > tgs->desired_ampl) {
      tgs->going_up = 0;
      // Reduce the next sample by the overage
      next_sample = tgs->desired_ampl - (next_sample - tgs->desired_ampl);
    }
  } else {
    next_sample = tgs->last_sample - tgs->delta;
    if (next_sample < -tgs->desired_ampl) {
      tgs->going_up = 1;
      next_sample = -tgs->desired_ampl - (next_sample - (-tgs->desired_ampl));
    }
  }

  tgs->last_sample = next_sample;
  return tgs->last_sample;
}
