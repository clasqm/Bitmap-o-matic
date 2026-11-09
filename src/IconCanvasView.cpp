#include "IconCanvasView.h"

#include "EditorWindow.h"
#include "Messages.h"
#include "Palette.h"
#include "Raster.h"
#include "Tools.h"

#include <InterfaceDefs.h>
#include <Message.h>
#include <Window.h>

#include <math.h>
#include <string.h>


static const float kCanvasPadding = 10.0f;


static rgb_color
MakeColor(uint8 red, uint8 green, uint8 blue)
{
	rgb_color color = { red, green, blue, 255 };
	return color;
}


IconCanvasView::IconCanvasView(EditorWindow* window)
	:
	BView(BRect(0, 0, 0, 0), "icon canvas", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	fWindow(window),
	fVariant(kLargeVariant),
	fCellSize(12),
	fBoardX(0),
	fBoardY(0),
	fBoardSize(384),
	fPainting(false),
	fLastX(-1),
	fLastY(-1),
	fPreviewActive(false),
	fShapeStartX(-1),
	fShapeStartY(-1),
	fShapeEndX(-1),
	fShapeEndY(-1),
	fHasSelection(false),
	fSelection(0, 0, -1, -1),
	fSelecting(false),
	fSelectAnchorX(0),
	fSelectAnchorY(0),
	fSelectCurrentX(0),
	fSelectCurrentY(0),
	fMoveCandidate(false),
	fMoving(false),
	fMoveWidth(0),
	fMoveHeight(0),
	fMoveSource(0, 0, -1, -1),
	fMoveGrabX(0),
	fMoveGrabY(0),
	fMoveDeltaX(0),
	fMoveDeltaY(0),
	fStrokeColor(0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetDrawingMode(B_OP_COPY);
	memset(fPreviewPixels, IconPalette::kTransparentIndex, sizeof(fPreviewPixels));
	memset(fMovePixels, IconPalette::kTransparentIndex, sizeof(fMovePixels));
}


IconCanvasView::~IconCanvasView()
{
}


void
IconCanvasView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_UpdateGeometry();
}


void
IconCanvasView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_UpdateGeometry();
	Invalidate();
}


int32
IconCanvasView::PixelWidth() const
{
	return fWindow->Document()->VariantWidth(fVariant);
}


int32
IconCanvasView::PixelHeight() const
{
	return fWindow->Document()->VariantHeight(fVariant);
}


void
IconCanvasView::_UpdateGeometry()
{
	float width = Bounds().Width();
	float height = Bounds().Height();
	float size = width < height ? width : height;

	fCellSize = (int32)floorf((size - 2.0f * kCanvasPadding) / 32.0f);
	if (fCellSize < 4)
		fCellSize = 4;
	if (fCellSize > 40)
		fCellSize = 40;

	fBoardSize = fCellSize * 32.0f;
	fBoardX = floorf((width - fBoardSize) / 2.0f);
	fBoardY = floorf((height - fBoardSize) / 2.0f);
	if (fBoardX < 0)
		fBoardX = 0;
	if (fBoardY < 0)
		fBoardY = 0;
}


BRect
IconCanvasView::_PixelBoard() const
{
	float size = fCellSize * PixelWidth();
	float left = fBoardX + floorf((fBoardSize - size) / 2.0f);
	float top = fBoardY + floorf((fBoardSize - size) / 2.0f);
	return BRect(left, top, left + size - 1, top + size - 1);
}


bool
IconCanvasView::_PixelAt(BPoint where, int32& x, int32& y) const
{
	BRect board = _PixelBoard();
	if (!board.Contains(where))
		return false;

	int32 width = PixelWidth();
	x = (int32)floorf((where.x - board.left) / fCellSize);
	y = (int32)floorf((where.y - board.top) / fCellSize);
	if (x < 0 || y < 0 || x >= width || y >= width) {
		// Rounding at the right and bottom edges.
		if (x == width)
			x--;
		if (y == width)
			y--;
		if (x < 0 || y < 0 || x >= width || y >= width)
			return false;
	}
	return true;
}


const uint8*
IconCanvasView::_Pixels()
{
	if (fPreviewActive)
		return fPreviewPixels;
	return fWindow->Document()->Pixels(fVariant);
}


void
IconCanvasView::_DrawCheckerboard(BRect area)
{
	int32 check = fCellSize >= 8 ? fCellSize / 2 : 3;
	int32 startX = (int32)area.left;
	int32 startY = (int32)area.top;
	int32 endX = (int32)area.right;
	int32 endY = (int32)area.bottom;

	rgb_color dark = { 218, 218, 218, 255 };
	SetHighColor(dark);

	for (int32 y = startY; y <= endY; y += check) {
		for (int32 x = startX; x <= endX; x += check) {
			bool light = (((x - startX) / check) + ((y - startY) / check)) % 2
				== 0;
			if (light)
				continue;
			int32 right = x + check - 1;
			int32 bottom = y + check - 1;
			if (right > endX)
				right = endX;
			if (bottom > endY)
				bottom = endY;
			FillRect(BRect(x, y, right, bottom));
		}
	}
}


void
IconCanvasView::_DrawPixel(BRect pixelRect, uint8 index)
{
	if (IconPalette::IsTransparent(index))
		return;

	const IconRGB& color = IconPalette::kColors[index];
	rgb_color high = { color.r, color.g, color.b, 255 };
	SetHighColor(high);
	FillRect(pixelRect);
}


void
IconCanvasView::_DrawSelection()
{
	if (!fHasSelection && !fSelecting)
		return;

	BRect selection = fSelecting ? _NormalizedSelection() : fSelection;
	if (selection.left > selection.right || selection.top > selection.bottom)
		return;

	BRect board = _PixelBoard();
	float cell = fCellSize;
	BRect rect(board.left + selection.left * cell,
		board.top + selection.top * cell,
		board.left + (selection.right + 1) * cell - 1,
		board.top + (selection.bottom + 1) * cell - 1);

	SetHighColor(MakeColor(0, 0, 0));
	StrokeRect(rect);

	SetHighColor(MakeColor(255, 255, 255));
	for (float x = rect.left + 1; x <= rect.right; x += 8) {
		float right = x + 3;
		if (right > rect.right)
			right = rect.right;
		FillRect(BRect(x, rect.top, right, rect.top));
		FillRect(BRect(x, rect.bottom, right, rect.bottom));
	}
	for (float y = rect.top + 1; y <= rect.bottom; y += 8) {
		float bottom = y + 3;
		if (bottom > rect.bottom)
			bottom = rect.bottom;
		FillRect(BRect(rect.left, y, rect.left, bottom));
		FillRect(BRect(rect.right, y, rect.right, bottom));
	}
}


void
IconCanvasView::Draw(BRect updateRect)
{
	BRect board = _PixelBoard();
	int32 width = PixelWidth();
	int32 height = PixelHeight();

	SetDrawingMode(B_OP_COPY);

	SetHighColor(MakeColor(255, 255, 255));
	FillRect(board);
	_DrawCheckerboard(board);

	const uint8* pixels = _Pixels();
	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {
			uint8 index = pixels[y * width + x];
			if (IconPalette::IsTransparent(index))
				continue;

			BRect pixel(board.left + x * fCellSize,
				board.top + y * fCellSize,
				board.left + (x + 1) * fCellSize - 1,
				board.top + (y + 1) * fCellSize - 1);
			_DrawPixel(pixel, index);
		}
	}

	if (fCellSize >= 5) {
		SetHighColor(MakeColor(200, 200, 200));
		for (int32 i = 1; i < width; i++) {
			float x = board.left + i * fCellSize;
			FillRect(BRect(x, board.top, x, board.bottom));
			float y = board.top + i * fCellSize;
			FillRect(BRect(board.left, y, board.right, y));
		}
	}

	SetHighColor(MakeColor(80, 80, 80));
	StrokeRect(board);

	_DrawSelection();
}


void
IconCanvasView::_CancelInteraction()
{
	fPainting = false;
	fSelecting = false;
	fMoveCandidate = false;
	fMoving = false;
	fPreviewActive = false;
}


void
IconCanvasView::SetVariant(int32 variant)
{
	if (variant < 0 || variant >= kIconVariantCount || variant == fVariant)
		return;

	_CancelInteraction();
	fVariant = variant;
	fHasSelection = false;
	fSelection.Set(0, 0, -1, -1);
	fSelecting = false;
	fMoveCandidate = false;
	fMoving = false;
	fPreviewActive = false;
	Invalidate();
}


void
IconCanvasView::DocumentChanged()
{
	Invalidate();
}


void
IconCanvasView::ColorsChanged()
{
	Invalidate();
}


void
IconCanvasView::ToolChanged()
{
	_CancelInteraction();
	fMoveCandidate = false;
	fMoving = false;
	Invalidate();
}


void
IconCanvasView::Deselect()
{
	fHasSelection = false;
	fSelection.Set(0, 0, -1, -1);
	fSelecting = false;
	Invalidate();
}


void
IconCanvasView::SelectAll()
{
	int32 width = PixelWidth();
	fHasSelection = true;
	fSelection.Set(0, 0, width - 1, width - 1);
	fSelecting = false;
	Invalidate();
}


void
IconCanvasView::_SetPixel(int32 x, int32 y, uint8 value)
{
	int32 width = PixelWidth();
	if (x < 0 || y < 0 || x >= width || y >= width)
		return;

	fWindow->Document()->Pixels(fVariant)[y * width + x] = value;
}


void
IconCanvasView::_Changed()
{
	fWindow->DocumentEdited();
	Invalidate();
}


void
IconCanvasView::_UpdateHover(int32 x, int32 y, bool valid)
{
	BString status;
	if (valid) {
		int32 width = PixelWidth();
		uint8 index = fWindow->Document()->Pixels(fVariant)[y * width + x];
		if (IconPalette::IsTransparent(index)) {
			status.SetToFormat("Pixel %ld, %ld — transparent",
				(long)x, (long)y);
		} else {
			const IconRGB& color = IconPalette::kColors[index];
			status.SetToFormat("Pixel %ld, %ld — index %d, RGB %d, %d, %d",
				(long)x, (long)y, (int)index, (int)color.r, (int)color.g,
				(int)color.b);
		}
	} else {
		status.SetToFormat("%s — %ld x %ld pixels",
			IconDocument::VariantName(fVariant), (long)PixelWidth(),
			(long)PixelHeight());
	}
	fWindow->UpdateStatus(status);
}


void
IconCanvasView::MouseDown(BPoint where)
{
	MakeFocus(true);

	BMessage* message = Window() != NULL ? Window()->CurrentMessage() : NULL;
	int32 buttons = 0;
	if (message != NULL)
		message->FindInt32("buttons", &buttons);
	bool secondary = (buttons & B_SECONDARY_MOUSE_BUTTON) != 0;

	int32 x, y;
	if (!_PixelAt(where, x, y))
		return;

	int32 tool = fWindow->Tool();
	uint8 color = secondary ? fWindow->BackColor() : fWindow->FrontColor();
	fStrokeColor = color;

	switch (tool) {
		case kToolEyedropper:
		{
			uint8 index = fWindow->Document()->Pixels(fVariant)[
				y * PixelWidth() + x];
			if (secondary)
				fWindow->SetBackColor(index);
			else
				fWindow->SetFrontColor(index);
			break;
		}

		case kToolSelect:
			if (fHasSelection && fSelection.Contains(BPoint(x, y)))
				_BeginMove(x, y);
			else
				_BeginSelection(x, y);
			break;

		case kToolPencil:
		case kToolEraser:
			fWindow->PushUndo(fVariant);
			if (tool == kToolEraser)
				fStrokeColor = fWindow->BackColor();
			fPainting = true;
			fLastX = x;
			fLastY = y;
			_SetPixel(x, y, fStrokeColor);
			_Changed();
			break;

		case kToolFill:
		{
			uint8* pixels = fWindow->Document()->Pixels(fVariant);
			uint8 target = pixels[y * PixelWidth() + x];
			if (target != color) {
				fWindow->PushUndo(fVariant);
				raster::FloodFill(pixels, PixelWidth(), PixelHeight(), x, y,
					color);
				_Changed();
			}
			break;
		}

		default:
			fPreviewActive = true;
			fShapeStartX = x;
			fShapeStartY = y;
			fShapeEndX = x;
			fShapeEndY = y;
			_UpdateShapePreview(fStrokeColor);
			break;
	}

	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}


void
IconCanvasView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
	int32 width = PixelWidth();
	int32 x, y;
	bool inside = _PixelAt(where, x, y);

	if (fPainting) {
		BRect board = _PixelBoard();
		x = (int32)floorf((where.x - board.left) / fCellSize);
		y = (int32)floorf((where.y - board.top) / fCellSize);
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x >= width) x = width - 1;
		if (y >= width) y = width - 1;

		raster::Line(fWindow->Document()->Pixels(fVariant), width, width,
			fLastX, fLastY, x, y, fStrokeColor);
		fLastX = x;
		fLastY = y;
		_Changed();
		return;
	}

	if (fSelecting) {
		if (inside)
			_UpdateSelection(x, y);
		return;
	}

	if (fMoveCandidate || fMoving) {
		BRect board = _PixelBoard();
		int32 currentX = (int32)floorf((where.x - board.left) / fCellSize);
		int32 currentY = (int32)floorf((where.y - board.top) / fCellSize);
		int32 deltaX = currentX - fMoveGrabX;
		int32 deltaY = currentY - fMoveGrabY;

		if (!fMoving && (deltaX != 0 || deltaY != 0))
			fMoving = true;

		if (fMoving && (deltaX != fMoveDeltaX || deltaY != fMoveDeltaY)) {
			fMoveDeltaX = deltaX;
			fMoveDeltaY = deltaY;
			_BuildMovePreview();
			Invalidate();
		}
		return;
	}

	if (fPreviewActive) {
		BRect board = _PixelBoard();
		x = (int32)floorf((where.x - board.left) / fCellSize);
		y = (int32)floorf((where.y - board.top) / fCellSize);
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x >= width) x = width - 1;
		if (y >= width) y = width - 1;

		if (x != fShapeEndX || y != fShapeEndY) {
			fShapeEndX = x;
			fShapeEndY = y;
			_UpdateShapePreview(fStrokeColor);
			Invalidate();
		}
		return;
	}

	_UpdateHover(x, y, inside);
}


void
IconCanvasView::MouseUp(BPoint where)
{
	if (fPainting) {
		fPainting = false;
		return;
	}

	if (fSelecting) {
		fSelecting = false;
		if (fSelectCurrentX == fSelectAnchorX
			&& fSelectCurrentY == fSelectAnchorY) {
			fHasSelection = false;
			fSelection.Set(0, 0, -1, -1);
		} else {
			fHasSelection = true;
		}
		Invalidate();
		return;
	}

	if (fMoving) {
		_CommitMove();
		return;
	}

	if (fMoveCandidate) {
		// A click inside the selection without dragging keeps it selected.
		fMoveCandidate = false;
		Invalidate();
		return;
	}

	if (fPreviewActive)
		_CommitShape(fStrokeColor);
}


void
IconCanvasView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		if (bytes[0] == B_DELETE || bytes[0] == B_BACKSPACE) {
			Window()->PostMessage(new BMessage(MSG_CLEAR));
			return;
		}
		if (bytes[0] == B_ESCAPE) {
			Deselect();
			return;
		}
	}
	BView::KeyDown(bytes, numBytes);
}


void
IconCanvasView::_BeginSelection(int32 x, int32 y)
{
	fSelecting = true;
	fHasSelection = false;
	fSelectAnchorX = x;
	fSelectAnchorY = y;
	fSelectCurrentX = x;
	fSelectCurrentY = y;
	Invalidate();
}


void
IconCanvasView::_UpdateSelection(int32 x, int32 y)
{
	fSelectCurrentX = x;
	fSelectCurrentY = y;
	fSelection = _NormalizedSelection();
	Invalidate();
}


BRect
IconCanvasView::_NormalizedSelection() const
{
	int32 left = fSelectAnchorX < fSelectCurrentX
		? fSelectAnchorX : fSelectCurrentX;
	int32 top = fSelectAnchorY < fSelectCurrentY
		? fSelectAnchorY : fSelectCurrentY;
	int32 right = fSelectAnchorX > fSelectCurrentX
		? fSelectAnchorX : fSelectCurrentX;
	int32 bottom = fSelectAnchorY > fSelectCurrentY
		? fSelectAnchorY : fSelectCurrentY;
	return BRect(left, top, right, bottom);
}


void
IconCanvasView::_BeginMove(int32 x, int32 y)
{
	int32 width = PixelWidth();
	fMoveSource = fSelection;
	fMoveWidth = (int32)fSelection.Width() + 1;
	fMoveHeight = (int32)fSelection.Height() + 1;
	if (fMoveWidth <= 0 || fMoveHeight <= 0 || fMoveWidth > width
		|| fMoveHeight > width)
		return;

	const uint8* pixels = fWindow->Document()->Pixels(fVariant);
	for (int32 row = 0; row < fMoveHeight; row++) {
		memcpy(fMovePixels + row * fMoveWidth,
			pixels + ((int32)fMoveSource.top + row) * width
				+ (int32)fMoveSource.left,
			fMoveWidth);
	}

	fMoveGrabX = x;
	fMoveGrabY = y;
	fMoveDeltaX = 0;
	fMoveDeltaY = 0;
	fMoveCandidate = true;
	fMoving = false;
}


void
IconCanvasView::_BuildMovePreview()
{
	int32 width = PixelWidth();

	memcpy(fPreviewPixels, fWindow->Document()->Pixels(fVariant),
		width * width);

	for (int32 row = 0; row < fMoveHeight; row++) {
		int32 y = (int32)fMoveSource.top + row;
		if (y < 0 || y >= width)
			continue;
		memset(fPreviewPixels + y * width + (int32)fMoveSource.left,
			IconPalette::kTransparentIndex, fMoveWidth);
	}

	for (int32 row = 0; row < fMoveHeight; row++) {
		int32 y = (int32)fMoveSource.top + fMoveDeltaY + row;
		if (y < 0 || y >= width)
			continue;
		for (int32 column = 0; column < fMoveWidth; column++) {
			int32 x = (int32)fMoveSource.left + fMoveDeltaX + column;
			if (x < 0 || x >= width)
				continue;
			fPreviewPixels[y * width + x]
				= fMovePixels[row * fMoveWidth + column];
		}
	}

	int32 left = (int32)fMoveSource.left + fMoveDeltaX;
	int32 top = (int32)fMoveSource.top + fMoveDeltaY;
	int32 right = left + fMoveWidth - 1;
	int32 bottom = top + fMoveHeight - 1;
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= width) right = width - 1;
	if (bottom >= width) bottom = width - 1;
	fSelection.Set(left, top, right, bottom);

	fPreviewActive = true;
}


void
IconCanvasView::_CommitMove()
{
	int32 width = PixelWidth();

	fWindow->PushUndo(fVariant);
	memcpy(fWindow->Document()->Pixels(fVariant), fPreviewPixels,
		width * width);
	fPreviewActive = false;
	fMoving = false;
	fMoveCandidate = false;

	bool valid = fSelection.left <= fSelection.right
		&& fSelection.top <= fSelection.bottom;
	fHasSelection = valid;
	if (!valid)
		fSelection.Set(0, 0, -1, -1);

	_Changed();
	Invalidate();
}


void
IconCanvasView::_UpdateShapePreview(uint8 color)
{
	int32 width = PixelWidth();
	memcpy(fPreviewPixels, fWindow->Document()->Pixels(fVariant),
		width * width);

	int32 x0 = fShapeStartX < fShapeEndX ? fShapeStartX : fShapeEndX;
	int32 x1 = fShapeStartX > fShapeEndX ? fShapeStartX : fShapeEndX;
	int32 y0 = fShapeStartY < fShapeEndY ? fShapeStartY : fShapeEndY;
	int32 y1 = fShapeStartY > fShapeEndY ? fShapeStartY : fShapeEndY;

	switch (fWindow->Tool()) {
		case kToolLine:
			raster::Line(fPreviewPixels, width, width, fShapeStartX,
				fShapeStartY, fShapeEndX, fShapeEndY, color);
			break;
		case kToolRectangle:
			raster::Rectangle(fPreviewPixels, width, width, x0, y0, x1, y1,
				false, color);
			break;
		case kToolFilledRectangle:
			raster::Rectangle(fPreviewPixels, width, width, x0, y0, x1, y1,
				true, color);
			break;
		case kToolRoundRectangle:
			raster::RoundRectangle(fPreviewPixels, width, width, x0, y0, x1,
				y1, false, color);
			break;
		case kToolFilledRoundRectangle:
			raster::RoundRectangle(fPreviewPixels, width, width, x0, y0, x1,
				y1, true, color);
			break;
		case kToolEllipse:
			raster::Ellipse(fPreviewPixels, width, width, x0, y0, x1, y1,
				false, color);
			break;
		case kToolFilledEllipse:
			raster::Ellipse(fPreviewPixels, width, width, x0, y0, x1, y1,
				true, color);
			break;
		default:
			break;
	}
}


void
IconCanvasView::_CommitShape(uint8 color)
{
	(void)color;
	int32 width = PixelWidth();

	fWindow->PushUndo(fVariant);
	memcpy(fWindow->Document()->Pixels(fVariant), fPreviewPixels,
		width * width);
	fPreviewActive = false;

	_Changed();
	Invalidate();
}


void
IconCanvasView::ClearSelection(uint8 color)
{
	int32 width = PixelWidth();
	uint8* pixels = fWindow->Document()->Pixels(fVariant);

	fWindow->PushUndo(fVariant);
	if (fHasSelection) {
		int32 left = (int32)fSelection.left;
		int32 right = (int32)fSelection.right;
		int32 top = (int32)fSelection.top;
		int32 bottom = (int32)fSelection.bottom;
		if (left < 0) left = 0;
		if (top < 0) top = 0;
		if (right >= width) right = width - 1;
		if (bottom >= width) bottom = width - 1;

		for (int32 y = top; y <= bottom; y++)
			memset(pixels + y * width + left, color, right - left + 1);
	} else {
		raster::Fill(pixels, width, width, color);
	}

	_Changed();
	Invalidate();
}


void
IconCanvasView::PasteBlock(const uint8* pixels, int32 width, int32 height)
{
	int32 canvasWidth = PixelWidth();
	if (pixels == NULL || width <= 0 || height <= 0
		|| width > canvasWidth || height > canvasWidth)
		return;

	int32 destinationX = fHasSelection ? (int32)fSelection.left : 0;
	int32 destinationY = fHasSelection ? (int32)fSelection.top : 0;
	if (destinationX + width > canvasWidth)
		destinationX = canvasWidth - width;
	if (destinationY + height > canvasWidth)
		destinationY = canvasWidth - height;
	if (destinationX < 0)
		destinationX = 0;
	if (destinationY < 0)
		destinationY = 0;

	fWindow->PushUndo(fVariant);
	uint8* destination = fWindow->Document()->Pixels(fVariant);
	for (int32 y = 0; y < height; y++) {
		memcpy(destination + (destinationY + y) * canvasWidth + destinationX,
			pixels + y * width, width);
	}

	fHasSelection = true;
	fSelection.Set(destinationX, destinationY, destinationX + width - 1,
		destinationY + height - 1);

	_Changed();
	Invalidate();
}
