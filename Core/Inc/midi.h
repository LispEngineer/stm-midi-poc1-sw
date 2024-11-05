/*
 * midi.h
 *
 *  Created on: Sep 8, 2024
 *  Updated on: 2024-09-08
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 *
 */

#ifndef INC_MIDI_H_
#define INC_MIDI_H_

// MIDI Messages
// TODO: Use a C23 enum if this is supported by our compiler
// These generally are the same as the status byte used
#define MIDI_NONE     ((uint8_t)0x00) // If we have no current running status
#define MIDI_ERROR    ((uint8_t)0x01) // If we see an error somehow

// Channel mode messages
#define MIDI_MODE_ALL_SOUND_OFF ((uint8_t)120)
#define MIDI_MODE_RESET_ALL     ((uint8_t)121)
#define MIDI_MODE_LOCAL_CONTROL ((uint8_t)122)
#define MIDI_MODE_ALL_NOTES_OFF ((uint8_t)123)
#define MIDI_MODE_OMNI_OFF      ((uint8_t)124)
#define MIDI_MODE_OMNI_ON       ((uint8_t)125)
#define MIDI_MODE_MONO          ((uint8_t)126)
#define MIDI_MODE_POLY          ((uint8_t)127)

// Channel voice/mode messages
#define MIDI_NOTE_OFF ((uint8_t)0x80)
#define MIDI_NOTE_ON  ((uint8_t)0x90)
// System messages
// Real time mesages
#define MIDI_RT_TIMING_CLOCK ((uint8_t)0xF8)
// ...
#define MIDI_RT_SYSTEM_RESET ((uint8_t)0xFF)

/*
 * We return this structure when we have received a fully
 * formed MIDI message.
 */
typedef struct {
  // One of the MIDI_ enumerated types above; the same
  // as the MIDI status byte when >= 0x80
  uint8_t type;
  uint8_t channel;
  union {
    uint8_t data1;
    uint8_t note;
    uint8_t tcqf_message_type; // MIDI Time Code Quarter Frame
    uint8_t lsb; // Least significant 7 of 14 bits
    uint8_t control; // Control # 0-119
    uint8_t pressure; // Channel aftertouch pressure
    uint8_t program; // Program #
  };
  union {
    uint8_t data2;
    uint8_t velocity; // Note velocity or poly aftertouch pressure
    uint8_t tcqf_value; // MIDI Time Code Quarter Frame
    uint8_t msb; // Most significant 7 of 14 bits
    uint8_t cc_value; // Control value for CC
  };
} midi_message;

#define MIDI_14bits(mmptr) (((mmptr)->lsb & 0x7F) | (((mmptr)->msb & 0x7F) << 7))

/*
 * This structure maintains the internal state of
 * an incoming out outgoing MIDI I/O channel.
 * It is primarily used to keep track of running
 * status both incoming and outgoing, but also
 * accumulates bytes for messages before returning
 * a fully parsed message.
 *
 * Currently we ignore SysEx instead of storing
 * any SysEx stuff.
 */

typedef struct {
  // Last status byte received - applicable only to voice/mode messages
  // So if the first nibble has to start with 8-E
  // If this is a zero, we do not know the last status.
  uint8_t last_status;
  // Did we receive a data1 yet?
  uint8_t received_data1;
  // If we received a data1, what was it?
  // TODO: Consider changing this to 1xxxxxxxb if we didn't receive a data 1, since that
  // will never be a valid data byte
  uint8_t data1;

  // TODO: Track Omni/Poly/Mono for all 16 tracks
  // TODO: Track MSB & LSB for each CC and their last value
  // TODO: Track the state of every key
  //       Reset them when the Channel Mode Changes
  //       Track them when the sustain/sostenuto is on (MIDI 1.0 Spec 4.2.1 page A-5)
} midi_stream;

extern const uint32_t midi_note_freqX100[];

void midi_stream_init(midi_stream *ms);
int midi_stream_receive(midi_stream *ms, uint8_t b, midi_message *msg);
int midi_snprintf(char *str, size_t size, midi_message *mm);

#endif /* INC_MIDI_H_ */
