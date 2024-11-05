/*
 * midi.c
 *
 *  Created on: Sep 8, 2024
 *  Updated on: 2024-09-08
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 *
 * Parses incoming MIDI messages into a persistent buffer, one
 * byte at a time. Handles running status.
 *
 * For notes on MIDI: see MIDI.md
 */

#include <stdint.h>
#include <stdio.h>
#include "midi.h"

// These are the frequencies of MIDI notes from
// 0 to 127, times 100 Hz, per this web page:
// https://inspiredacoustics.com/en/MIDI_note_numbers_and_center_frequencies
// (Equal temperament at A 440 Hz)
const uint32_t midi_note_freqX100[] = {
  818,
  866,
  918,
  972,
  1030,
  1091,
  1156,
  1225,
  1298,
  1375,
  1457,
  1543,
  1635,
  1732,
  1835,
  1945,
  2060,
  2183,
  2312,
  2450,
  2596,
  2750,
  2914,
  3087,
  3270,
  3465,
  3671,
  3889,
  4120,
  4365,
  4625,
  4900,
  5191,
  5500,
  5827,
  6174,
  6541,
  6930,
  7342,
  7778,
  8241,
  8731,
  9250,
  9800,
  10383,
  11000,
  11654,
  12347,
  13081,
  13859,
  14683,
  15556,
  16481,
  17461,
  18500,
  19600,
  20765,
  22000,
  23308,
  24694,
  26163,
  27718,
  29366,
  31113,
  32963,
  34923,
  36999,
  39200,
  41530,
  44000,
  46616,
  49388,
  52325,
  55437,
  58733,
  62225,
  65926,
  69846,
  73999,
  78399,
  83061,
  88000,
  93233,
  98777,
  104650,
  110873,
  117466,
  124451,
  131851,
  139691,
  147998,
  156798,
  166122,
  176000,
  186466,
  197553,
  209300,
  221746,
  234932,
  248902,
  263702,
  279383,
  295996,
  313596,
  332244,
  352000,
  372931,
  395107,
  418601,
  443492,
  469864,
  497803,
  527404,
  558765,
  591991,
  627193,
  664488,
  704000,
  745862,
  790213,
  837202,
  886984,
  939727,
  995606,
  1054808,
  1117530,
  1183982,
  1254385,
};

/** Initializes a new MIDI data stream to start values.
 */
void midi_stream_init(midi_stream *ms) {
  ms->last_status = 0;
  ms->received_data1 = 0;
}

// FIXME: Fix all these magic numbers

/** Receives a byte on a MIDI stream.
 * Returns true if we received a full message.
 * Puts the message in the specified location, if one is fully received.
 */
int midi_stream_receive(midi_stream *ms, uint8_t b, midi_message *msg) {

  if (b & 0x80) {
    // Status byte

    switch (b & 0xF0) {

    // Channel Voice/Mode status bytes ///////////////////////////////////////
    case 0x80: // Note off (2 bytes)
    case 0x90: // Note on (2 bytes)
    case 0xA0: // Poly aftertouch (2 bytes)
    case 0xB0: // Control change (2 bytes) & Channel Mode (1st byte 120-127)
    case 0xC0: // Program Change (1 byte)
    case 0xD0: // Channel aftertouch (1 byte)
    case 0xE0: // Pitch bend (2 bytes)
      ms->last_status = b;
      ms->received_data1 = 0;
      return 0; // Expecting more later

    // System status bytes ///////////////////////////////////////////////////
    case 0xF0: // System: Exclusive, Common, Real-Time
      // Real time messages do not interfere with running status
      if (b >= 0xF8) {
        // Real-time message does not change running status and has zero data bytes
        msg->type = b;
        return 1;
      }
      switch (b & 0x7) { // So, 0xFn where n = 0 - 7
      case 0x0: // Start of SysEx - we ignore all data bytes from here
        ms->last_status = b;
        return 0;
      case 0x1: // MIDI Time Code Quarter Frame - (1 byte)
      case 0x2: // Song Position Pointer - (2 bytes)
      case 0x3: // Song select (1 byte)
        ms->last_status = b;
        ms->received_data1 = 0;
        return 0; // Expecting more later
      case 0x4: // Undefined
      case 0x5: // Undefined
        ms->last_status = MIDI_NONE;
        // We probably don't need to return anything
        return 0; // Error
      case 0x6: // Tune request (0 bytes)
        ms->last_status = MIDI_NONE; // End running status
        msg->type = b;
        return 1;
      case 0x7: // SysEx EOX (end of eXclusive) - we are ignoring this for now
        ms->last_status = MIDI_NONE;
        return 0;
      }
    }

    // No fall through - should be unreachable
    return 0;
  }

  //////////////////////////////////////////////////////////////////////////////////
  // Data byte Handling

  switch (ms->last_status & 0xF0) {

  case 0x80: // Note off (2 bytes)
  case 0x90: // Note on (2 bytes)
  case 0xA0: // Poly aftertouch (2 bytes)
    if (ms->received_data1) {
      msg->type = ms->last_status;
      msg->note = ms->data1;
      msg->velocity = b;
      msg->channel = ms->last_status & 0x0F;
      // Running status remains
      ms->received_data1 = 0;
      // If it's a note ON with velocity 0, let's convert it to
      // a note OFF
      if ((msg->type & 0xF0) == MIDI_NOTE_ON &&
          msg->velocity == 0) {
        msg->type = MIDI_NOTE_OFF | msg->channel;
      }
      return 1;
    }
    // Store data 1 for next time
    ms->data1 = b;
    ms->received_data1 = 1;
    return 0;

  case 0xB0: // Control change (2 bytes) & Channel Mode (1st byte 120-127)
    if (ms->received_data1) {
      if (ms->data1 >= 120) {
        // Channel mode message
        msg->type = ms->data1;
        msg->data1 = b;
        msg->data2 = b;
        msg->channel = ms->last_status & 0x0F;
        ms->received_data1 = 0;
        return 1;
      }
      // Regular Control Change
      msg->type = ms->last_status;
      msg->control = ms->data1;
      msg->cc_value = b;
      msg->channel = ms->last_status & 0x0F;
      ms->received_data1 = 0;
      return 1;
    }
    ms->data1 = b;
    ms->received_data1 = 1;
    return 0;

  case 0xC0: // Program Change (1 byte)
    msg->type = ms->received_data1;
    msg->channel = ms->last_status & 0x0F;
    msg->program = b;
    return 1;

  case 0xD0: // Channel aftertouch (1 byte)
    msg->type = ms->received_data1;
    msg->channel = ms->last_status & 0x0F;
    msg->pressure = b;
    return 1;

  case 0xE0: // Pitch bend (2 bytes)
    if (ms->received_data1) {
      msg->type = ms->last_status;
      msg->msb = b;
      msg->lsb = ms->data1;
      msg->channel = ms->last_status & 0x0F;
      ms->received_data1 = 0;
      return 1;
    }
    ms->received_data1 = 1;
    ms->data1 = b;
    return 0;
  }

  switch (ms->last_status) {

  // System ////////////////////////////////////////////////////////////////////////

  case 0xF0: // SysEx - we're ignoring data in SysEx
    return 0;
  case 0xF1: // MIDI Time Code Quarter Frame - (1 byte)
    msg->type = ms->last_status;
    msg->tcqf_message_type = ((b & 0x70) >> 4);
    msg->tcqf_value = (b & 0x0F);
    ms->last_status = MIDI_NONE; // No running status
    return 1;
  case 0xF2: // Song Position Pointer - (2 bytes)
    if (ms->received_data1) {
      msg->type = ms->last_status;
      msg->lsb = ms->data1;
      msg->msb = b;
      ms->last_status = MIDI_NONE;
      return 1;
    }
    ms->received_data1 = 1;
    ms->data1 = b;
    return 0;
  case 0xF3: // Song select (1 byte)
    msg->type = ms->last_status;
    msg->data1 = b; // Song number
    ms->last_status = MIDI_NONE;
    return 1;
  case 0xF4: // Undefined - should never get here
  case 0xF5: // Undefined - should never get here
    ms->last_status = MIDI_NONE; // End running status
    return 0; // Error
  case 0xF6: // Tune request (0 bytes) - should never get here
    ms->last_status = MIDI_NONE; // End running status
    return 0;
  case 0xF7: // SysEx EOX (end of eXclusive) - we are ignoring this for now
    ms->last_status = MIDI_NONE;
    return 0;
  }

  // We should never get here unless ms->last_status < 128
  return 0;
}

// 0-101 inclusive
const char *cc_names[] = {
    "Bank select",
    "Mod wheel",
    "Breath control",
    NULL, // 3
    "Foot controller",
    "Portamento time",
    "Data entry MSB",
    "Channel volume", // 7
    "Balance",
    NULL, // 9
    "Pan",
    "Expression",
    "Effect 1",
    "Effect 2",
    NULL, NULL, // 14-15
    "General purpose 1",
    "General purpose 2",
    "General purpose 3",
    "General purpose 4", // 19
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, // 20-31
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 32-63 - LSB for 0-31
    "Sustain", // 64
    "Portamento on/off",
    "Sostenuto",
    "Soft pedal",
    "Legato",
    "Hold 2", // 69
    "Sound control 1",
    "Sound control 2",
    "Sound control 3",
    "Sound control 4",
    "Sound control 5",
    "Sound control 6",
    "Sound control 7",
    "Sound control 8",
    "Sound control 9",
    "Sound control 10", // 79
    "General purpose 5",
    "General purpose 6",
    "General purpose 7",
    "General purpose 8", // 83
    "Portamento control", // 84
    NULL, NULL, NULL, NULL, NULL, NULL, // 85-90
    "Effect Depth 1",
    "Effect Depth 2",
    "Effect Depth 3",
    "Effect Depth 4",
    "Effect Depth 5", // 95
    "Data increment",
    "Data decrement",
    "NRPM LSB",
    "NRPN MSB",
    "RPN LSB",
    "RPN MSB" // 101
};
const size_t num_cc_names = sizeof(cc_names) / sizeof(cc_names[0]);

/** Output human readable information about a full MIDI message. */
int midi_snprintf(char *str, size_t size, midi_message *mm) {

  const char *cc_name;
  char ccn_temp[32];

  switch (mm->type & 0xF0) {
  case 0x90:
    if (mm->velocity > 0) {
      return snprintf(str, size, "Note on: Chan %d, Note: %d, Vel: %d",
                      mm->channel, mm->note, mm->velocity);
    }
    // Fall through
  case 0x80:
    return snprintf(str, size, "Note off: Chan %d, Note: %d, Vel: %d",
                    mm->channel, mm->note, mm->velocity);
  case 0xA0:
    return snprintf(str, size, "Poly AT: Chan %d, Note: %d, Pres: %d",
                    mm->channel, mm->note, mm->velocity);

  case 0xB0:
    if (mm->control >= 120) {
      switch (mm->control) {
      case 120:
        return snprintf(str, size, "All sound off: Chan %d", mm->channel);
      case 121:
        return snprintf(str, size, "Reset all controllers: Chan %d", mm->channel);
      case 122:
        return snprintf(str, size, "Local control: %s, Chan %d",
            mm->cc_value == 0 ? "Off" : "On", mm->channel);
      case 123:
        return snprintf(str, size, "All notes off: Chan %d", mm->channel);
      case 124:
        return snprintf(str, size, "Omni mode: Off: Chan %d", mm->channel);
      case 125:
        return snprintf(str, size, "Omni mode: On, Chan %d", mm->channel);
      case 126:
        if (mm->cc_value == 0) {
          return snprintf(str, size, "Mono mode: All voices, Chan: %d", mm->channel);
        }
        return snprintf(str, size, "Mono mode: Voices: %d, Chan: %d", mm->cc_value, mm->channel);
      case 127:
        return snprintf(str, size, "Poly mode: On, Chan: %d", mm->channel);
      }
    }

    // First figure out the channel name
    if (mm->control >= num_cc_names) {
      snprintf(ccn_temp, sizeof(ccn_temp) - 1, "CC %d", mm->control);
      cc_name = ccn_temp;
    } else if (mm->control >= 32 && mm->control <= 63) {
      const char *name = cc_names[mm->control - 32];
      if (name == NULL) {
        snprintf(ccn_temp, sizeof(ccn_temp) - 1, "CC %d LSB", mm->control - 32);
      } else {
        snprintf(ccn_temp, sizeof(ccn_temp) - 1, "%s LSB", name);
      }
      cc_name = ccn_temp;
    } else if (cc_names[mm->control] == NULL) {
      snprintf(ccn_temp, sizeof(ccn_temp) - 1, "CC %d", mm->control);
      cc_name = ccn_temp;
    } else {
      cc_name = cc_names[mm->control];
    }

    return snprintf(str, size, "CC: %s, Val: %d, Chan: %d", cc_name, mm->cc_value, mm->channel);

  case 0xC0:
    return snprintf(str, size, "Program: Chan %d, Prog: %d",
                    mm->channel, mm->program);
  case 0xD0:
    return snprintf(str, size, "Chan AT: Chan %d, Pres: %d",
                    mm->channel, mm->pressure);
  case 0xE0:
    return snprintf(str, size, "Bend: Chan %d, Amt: %d",
                    mm->channel, MIDI_14bits(mm));
  }

  return snprintf(str, size, "MIDI msg: %02X, %d, %d",
                  mm->type, mm->data1, mm->data2);
}
