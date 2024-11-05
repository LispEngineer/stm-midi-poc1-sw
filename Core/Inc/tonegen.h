/*
 * tonegen.h
 *
 *  Created on: Sep 9, 2024
 *  Updated on: 2024-09-08
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */

#ifndef INC_TONEGEN_H_
#define INC_TONEGEN_H_

typedef struct {
  int16_t last_sample;
  int last_is_left; // Did we last provide left or right channel? (we're mono for now)

  uint32_t sample_rate;
  uint32_t desired_freq;
  int16_t desired_ampl; // Maximum positive signal value ; 0 or negative = silent

  // Calculated variables
  int16_t delta; // How much we change by each time - positive only
  int going_up; // Are we on the upward slope or downward slope

} tonegen_state;


void tonegen_init(tonegen_state *tgs, uint32_t sample_rate);
void tonegen_set(tonegen_state *tgs, uint32_t desired_freq, int16_t desired_ampl);
int16_t tonegen_next_sample(tonegen_state *tgs);




#endif /* INC_TONEGEN_H_ */
