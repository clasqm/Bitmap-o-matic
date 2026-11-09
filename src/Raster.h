#ifndef RASTER_H
#define RASTER_H

#include <stdint.h>

// Palette-index drawing primitives used by the icon canvas. These operate on
// a row-major, top-to-bottom pixel buffer with no padding, exactly like the
// data stored in a BeOS icon.
namespace raster {

void Fill(uint8_t* pixels, int32_t width, int32_t height, uint8_t value);

void Line(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t value);

void Rectangle(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool filled,
	uint8_t value);

void RoundRectangle(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool filled,
	uint8_t value);

void Ellipse(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool filled,
	uint8_t value);

// Replaces the contiguous region of pixels that share the colour found at
// (x, y) with the given value.
void FloodFill(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x, int32_t y, uint8_t value);

} // namespace raster

#endif // RASTER_H
