/* vim: set ai et ts=4 sw=4: */
#ifndef __FONTS_H__
#define __FONTS_H__

#include <stdint.h>

typedef struct {
  const uint8_t width; // Cannot be above 16, as each row of pixels is one uint16_t
  uint8_t height;
  // The font data for ASCII printable characters.
  // Starting at character 32 (ASCII space), this has
  // height uint16_t words of up to width pixels, with the
  // highest bit containing the first pixel of that row.
  const uint16_t *data;
  // TODO(dougfields): Precalculate character size in bytes for allocator
  // to save a multiply later when drawing characters.
} FontDef;

// Width x height
extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

#endif // __FONTS_H__
