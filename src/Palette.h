#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

// A plain RGB triple, deliberately not rgb_color, so that the palette table
// does not depend on the Haiku headers.
struct IconRGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

namespace IconPalette {

// The 256 colours of the BeOS/Haiku B_CMAP8 system palette.
extern const IconRGB kColors[256];

// B_TRANSPARENT_MAGIC_CMAP8. A pixel with this index is transparent; the
// palette entry itself is white with alpha 0.
extern const uint8_t kTransparentIndex;

inline bool
IsTransparent(uint8_t index)
{
	return index == kTransparentIndex;
}

// Returns the palette index whose colour is closest to the given RGB value.
// The transparent entry is never returned.
uint8_t ClosestIndex(uint8_t red, uint8_t green, uint8_t blue);

} // namespace IconPalette

#endif // PALETTE_H
