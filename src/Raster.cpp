#include "Raster.h"

#include <math.h>
#include <string.h>
#include <vector>


namespace {

inline void
SetPixel(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x, int32_t y, uint8_t value)
{
	if (x < 0 || y < 0 || x >= width || y >= height)
		return;
	pixels[y * width + x] = value;
}


inline void
HorizontalLine(uint8_t* pixels, int32_t width, int32_t height,
	int32_t y, int32_t x0, int32_t x1, uint8_t value)
{
	if (y < 0 || y >= height)
		return;
	if (x0 > x1) {
		int32_t swap = x0;
		x0 = x1;
		x1 = swap;
	}
	if (x1 < 0 || x0 >= width)
		return;
	if (x0 < 0)
		x0 = 0;
	if (x1 >= width)
		x1 = width - 1;
	memset(pixels + y * width + x0, value, x1 - x0 + 1);
}

} // namespace


namespace raster {


void
Fill(uint8_t* pixels, int32_t width, int32_t height, uint8_t value)
{
	if (pixels == NULL || width <= 0 || height <= 0)
		return;
	memset(pixels, value, width * height);
}


void
Line(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t value)
{
	int32_t dx = abs(x1 - x0);
	int32_t dy = -abs(y1 - y0);
	int32_t stepX = x0 < x1 ? 1 : -1;
	int32_t stepY = y0 < y1 ? 1 : -1;
	int32_t error = dx + dy;

	for (;;) {
		SetPixel(pixels, width, height, x0, y0, value);
		if (x0 == x1 && y0 == y1)
			break;
		int32_t doubled = 2 * error;
		if (doubled >= dy) {
			error += dy;
			x0 += stepX;
		}
		if (doubled <= dx) {
			error += dx;
			y0 += stepY;
		}
	}
}


void
Rectangle(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool filled,
	uint8_t value)
{
	if (x0 > x1) {
		int32_t swap = x0;
		x0 = x1;
		x1 = swap;
	}
	if (y0 > y1) {
		int32_t swap = y0;
		y0 = y1;
		y1 = swap;
	}
	if (filled) {
		for (int32_t y = y0; y <= y1; y++)
			HorizontalLine(pixels, width, height, y, x0, x1, value);
		return;
	}

	Line(pixels, width, height, x0, y0, x1, y0, value);
	Line(pixels, width, height, x0, y1, x1, y1, value);
	Line(pixels, width, height, x0, y0, x0, y1, value);
	Line(pixels, width, height, x1, y0, x1, y1, value);
}


void
RoundRectangle(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool filled,
	uint8_t value)
{
	if (x0 > x1) {
		int32_t swap = x0;
		x0 = x1;
		x1 = swap;
	}
	if (y0 > y1) {
		int32_t swap = y0;
		y0 = y1;
		y1 = swap;
	}
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 >= width)
		x1 = width - 1;
	if (y1 >= height)
		y1 = height - 1;
	if (x0 > x1 || y0 > y1)
		return;

	int32_t boxWidth = x1 - x0 + 1;
	int32_t boxHeight = y1 - y0 + 1;
	int32_t radius = boxWidth < boxHeight ? boxWidth : boxHeight;
	radius /= 3;
	if (radius > 4)
		radius = 4;
	if (radius < 1)
		radius = 1;

	for (int32_t y = y0; y <= y1; y++) {
		float inset = 0.0f;
		if (y < y0 + radius) {
			float dy = (y0 + radius) - y - 0.5f;
			inset = radius - sqrtf(radius * radius - dy * dy);
		} else if (y > y1 - radius) {
			float dy = y - (y1 - radius) - 0.5f;
			inset = radius - sqrtf(radius * radius - dy * dy);
		}

		int32_t left = x0 + (int32_t)floorf(inset + 0.5f);
		int32_t right = x1 - (int32_t)floorf(inset + 0.5f);
		if (left > right)
			continue;

		if (filled) {
			HorizontalLine(pixels, width, height, y, left, right, value);
		} else if (y == y0 || y == y1) {
			HorizontalLine(pixels, width, height, y, left, right, value);
		} else {
			SetPixel(pixels, width, height, left, y, value);
			SetPixel(pixels, width, height, right, y, value);
		}
	}
}


void
Ellipse(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool filled,
	uint8_t value)
{
	if (x0 > x1) {
		int32_t swap = x0;
		x0 = x1;
		x1 = swap;
	}
	if (y0 > y1) {
		int32_t swap = y0;
		y0 = y1;
		y1 = swap;
	}
	if (x1 < 0 || y1 < 0 || x0 >= width || y0 >= height)
		return;

	float centerX = (x0 + x1) / 2.0f;
	float centerY = (y0 + y1) / 2.0f;
	float radiusX = (x1 - x0) / 2.0f;
	float radiusY = (y1 - y0) / 2.0f;

	if (radiusX < 0.5f || radiusY < 0.5f) {
		SetPixel(pixels, width, height, (int32_t)centerX, (int32_t)centerY,
			value);
		return;
	}

	for (int32_t y = y0; y <= y1; y++) {
		float dy = (y - centerY) / radiusY;
		if (dy < -1.0f || dy > 1.0f)
			continue;
		float span = radiusX * sqrtf(1.0f - dy * dy);
		int32_t left = (int32_t)floorf(centerX - span + 0.5f);
		int32_t right = (int32_t)floorf(centerX + span + 0.5f);
		if (left > right)
			continue;

		if (filled) {
			HorizontalLine(pixels, width, height, y, left, right, value);
		} else if (y == y0 || y == y1) {
			HorizontalLine(pixels, width, height, y, left, right, value);
		} else {
			SetPixel(pixels, width, height, left, y, value);
			SetPixel(pixels, width, height, right, y, value);
		}
	}
}


void
FloodFill(uint8_t* pixels, int32_t width, int32_t height,
	int32_t x, int32_t y, uint8_t value)
{
	if (pixels == NULL || x < 0 || y < 0 || x >= width || y >= height)
		return;

	uint8_t target = pixels[y * width + x];
	if (target == value)
		return;

	std::vector<int32_t> stack;
	stack.push_back(y * width + x);
	while (!stack.empty()) {
		int32_t index = stack.back();
		stack.pop_back();
		if (pixels[index] != target)
			continue;

		int32_t currentX = index % width;
		int32_t currentY = index / width;
		pixels[index] = value;

		if (currentX > 0)
			stack.push_back(index - 1);
		if (currentX + 1 < width)
			stack.push_back(index + 1);
		if (currentY > 0)
			stack.push_back(index - width);
		if (currentY + 1 < height)
			stack.push_back(index + width);
	}
}


} // namespace raster
