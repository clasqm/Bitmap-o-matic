#include "Panels.h"

#include "EditorWindow.h"
#include "Palette.h"
#include "Tools.h"

#include <InterfaceDefs.h>
#include <Message.h>
#include <Window.h>

#include <math.h>
#include <string.h>


namespace {

rgb_color
MakeColor(uint8 red, uint8 green, uint8 blue)
{
	rgb_color color = { red, green, blue, 255 };
	return color;
}


void
DrawCheckerboard(BView* view, BRect area, int32 check, rgb_color dark)
{
	if (area.left > area.right || area.top > area.bottom)
		return;

	view->SetHighColor(MakeColor(255, 255, 255));
	view->FillRect(area);

	view->SetHighColor(dark);
	int32 left = (int32)area.left;
	int32 top = (int32)area.top;
	int32 right = (int32)area.right;
	int32 bottom = (int32)area.bottom;
	if (check < 2)
		check = 2;

	for (int32 y = top; y <= bottom; y += check) {
		for (int32 x = left; x <= right; x += check) {
			if ((((x - left) / check) + ((y - top) / check)) % 2 == 0)
				continue;

			int32 cellRight = x + check - 1;
			int32 cellBottom = y + check - 1;
			if (cellRight > right)
				cellRight = right;
			if (cellBottom > bottom)
				cellBottom = bottom;
			view->FillRect(BRect(x, y, cellRight, cellBottom));
		}
	}
}


void
StrokeDashedRect(BView* view, BRect rect, rgb_color color)
{
	view->SetHighColor(color);
	for (float x = rect.left; x <= rect.right; x += 5) {
		float right = x + 2;
		if (right > rect.right)
			right = rect.right;
		view->StrokeLine(BPoint(x, rect.top), BPoint(right, rect.top));
		view->StrokeLine(BPoint(x, rect.bottom), BPoint(right, rect.bottom));
	}
	for (float y = rect.top; y <= rect.bottom; y += 5) {
		float bottom = y + 2;
		if (bottom > rect.bottom)
			bottom = rect.bottom;
		view->StrokeLine(BPoint(rect.left, y), BPoint(rect.left, bottom));
		view->StrokeLine(BPoint(rect.right, y), BPoint(rect.right, bottom));
	}
}


void
DrawIndexedPixels(BView* view, const uint8* pixels, int32 width, BRect area,
	int32 scale)
{
	DrawCheckerboard(view, area, 8, MakeColor(206, 206, 206));

	for (int32 y = 0; y < width; y++) {
		for (int32 x = 0; x < width; x++) {
			uint8 index = pixels[y * width + x];
			if (IconPalette::IsTransparent(index))
				continue;

			const IconRGB& color = IconPalette::kColors[index];
			view->SetHighColor(MakeColor(color.r, color.g, color.b));
			view->FillRect(BRect(area.left + x * scale,
				area.top + y * scale,
				area.left + (x + 1) * scale - 1,
				area.top + (y + 1) * scale - 1));
		}
	}
}


void
DrawToolIcon(BView* view, int32 tool, BRect r, rgb_color background)
{
	rgb_color ink = MakeColor(0, 0, 0);
	rgb_color white = MakeColor(255, 255, 255);

	view->SetHighColor(ink);
	view->SetPenSize(1);

	float width = r.Width();
	float height = r.Height();

	switch (tool) {
		case kToolSelect:
			StrokeDashedRect(view, r, ink);
			break;

		case kToolEraser:
		{
			BPoint points[4] = {
				BPoint(r.left + width * 0.15f, r.bottom - height * 0.15f),
				BPoint(r.left + width * 0.55f, r.top + height * 0.15f),
				BPoint(r.left + width * 0.9f, r.top + height * 0.45f),
				BPoint(r.left + width * 0.5f, r.bottom - height * 0.1f)
			};
			view->FillPolygon(points, 4);
			view->SetHighColor(white);
			view->FillRect(BRect(r.left + width * 0.45f,
				r.top + height * 0.3f, r.left + width * 0.72f,
				r.top + height * 0.55f));
			break;
		}

		case kToolPencil:
		{
			view->SetPenSize(3);
			view->StrokeLine(BPoint(r.left + width * 0.2f,
				r.bottom - height * 0.2f),
				BPoint(r.right - width * 0.25f, r.top + height * 0.25f));
			BPoint tip[3] = {
				BPoint(r.left + width * 0.1f, r.bottom - height * 0.1f),
				BPoint(r.left + width * 0.3f, r.bottom - height * 0.28f),
				BPoint(r.left + width * 0.3f, r.bottom - height * 0.05f)
			};
			view->SetHighColor(ink);
			view->FillPolygon(tip, 3);
			break;
		}

		case kToolEyedropper:
		{
			view->SetPenSize(3);
			view->StrokeLine(BPoint(r.left + width * 0.25f,
				r.bottom - height * 0.25f),
				BPoint(r.right - width * 0.3f, r.top + height * 0.3f));
			view->SetPenSize(1);
			view->FillEllipse(BRect(r.right - width * 0.4f,
				r.top + height * 0.1f, r.right - width * 0.1f,
				r.top + height * 0.4f));
			break;
		}

		case kToolFill:
		{
			BPoint bucket[5] = {
				BPoint(r.left + width * 0.1f, r.top + height * 0.45f),
				BPoint(r.left + width * 0.5f, r.top + height * 0.1f),
				BPoint(r.left + width * 0.9f, r.top + height * 0.5f),
				BPoint(r.left + width * 0.55f, r.top + height * 0.85f),
				BPoint(r.left + width * 0.25f, r.top + height * 0.7f)
			};
			view->FillPolygon(bucket, 5);
			view->FillEllipse(BRect(r.right - width * 0.32f,
				r.bottom - height * 0.3f, r.right - width * 0.12f,
				r.bottom - height * 0.05f));
			break;
		}

		case kToolLine:
			view->StrokeLine(BPoint(r.left + width * 0.15f,
				r.bottom - height * 0.15f),
				BPoint(r.right - width * 0.15f, r.top + height * 0.15f));
			view->FillRect(BRect(r.left + width * 0.1f,
				r.bottom - height * 0.2f, r.left + width * 0.22f,
				r.bottom - height * 0.08f));
			view->FillRect(BRect(r.right - width * 0.22f,
				r.top + height * 0.08f, r.right - width * 0.1f,
				r.top + height * 0.2f));
			break;

		case kToolRectangle:
			view->StrokeRect(r);
			break;

		case kToolFilledRectangle:
			view->FillRect(r);
			break;

		case kToolRoundRectangle:
			view->FillRoundRect(r, 4, 4);
			view->SetHighColor(background);
			view->FillRoundRect(BRect(r.left + 2, r.top + 2, r.right - 2,
				r.bottom - 2), 3, 3);
			break;

		case kToolFilledRoundRectangle:
			view->FillRoundRect(r, 4, 4);
			break;

		case kToolEllipse:
			view->StrokeEllipse(r);
			break;

		case kToolFilledEllipse:
			view->FillEllipse(r);
			break;

		default:
			break;
	}
}

} // namespace


//	#pragma mark - ToolBarView


// Each tool is a view of its own, so that every button can carry its own
// tool tip; a tool tip always belongs to a whole view.
class ToolButton : public BView {
public:
	ToolButton(EditorWindow* window, int32 index)
		:
		BView(BRect(0, 0, 0, 0), "tool button", B_FOLLOW_NONE,
			B_WILL_DRAW),
		fWindow(window),
		fIndex(index),
		fActive(false)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetToolTip(ToolName(fIndex));
	}

	virtual void Draw(BRect updateRect)
	{
		BRect rect = Bounds();
		rgb_color face = fActive ? MakeColor(196, 212, 238)
			: MakeColor(246, 246, 246);

		SetDrawingMode(B_OP_COPY);
		SetHighColor(face);
		FillRect(rect);

		SetHighColor(fActive ? MakeColor(70, 90, 130)
			: MakeColor(152, 152, 152));
		FillRect(BRect(rect.left + 1, rect.bottom, rect.right, rect.bottom));
		FillRect(BRect(rect.right, rect.top + 1, rect.right, rect.bottom));

		SetHighColor(fActive ? MakeColor(120, 140, 180)
			: MakeColor(255, 255, 255));
		FillRect(BRect(rect.left, rect.top, rect.right - 1, rect.top));
		FillRect(BRect(rect.left, rect.top, rect.left, rect.bottom - 1));

		SetHighColor(fActive ? MakeColor(50, 65, 100)
			: MakeColor(110, 110, 110));
		StrokeRect(rect);

		float inset = rect.Width() > 26.0f ? 8.0f : 4.0f;
		DrawToolIcon(this, fIndex, BRect(rect.left + inset,
			rect.top + inset, rect.right - inset, rect.bottom - inset), face);
	}

	virtual void MouseDown(BPoint where)
	{
		(void)where;
		fWindow->SetTool(fIndex);
	}

	void SetActive(bool active)
	{
		if (active == fActive)
			return;
		fActive = active;
		Invalidate();
	}

private:
	EditorWindow* fWindow;
	int32 fIndex;
	bool fActive;
};


ToolBarView::ToolBarView(EditorWindow* window)
	:
	// B_FRAME_EVENTS is not a default: without it Haiku never calls
	// FrameResized(), and the buttons would keep the layout they were given
	// while the bar still had no width.
	BView(BRect(0, 0, 0, 0), "tool bar", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fWindow(window)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	for (int32 i = 0; i < kToolCount; i++) {
		fButtons[i] = new ToolButton(window, i);
		AddChild(fButtons[i]);
	}

	_LayoutButtons();
}


float
ToolBarView::_ButtonSize() const
{
	const float gap = 4.0f;
	float width = Bounds().Width();
	if (width < 1.0f)
		return 18.0f;

	float size = (width - 8.0f - (kToolCount - 1) * gap) / kToolCount;
	if (size > 38.0f)
		size = 38.0f;
	if (size < 18.0f)
		size = 18.0f;
	return floorf(size);
}


BRect
ToolBarView::_ButtonRect(int32 index) const
{
	const float gap = 4.0f;
	float size = _ButtonSize();
	float total = kToolCount * size + (kToolCount - 1) * gap;
	float left = floorf((Bounds().Width() - total) / 2.0f);
	if (left < 0.0f)
		left = 0.0f;
	float top = floorf((Bounds().Height() - size) / 2.0f);
	if (top < 0.0f)
		top = 0.0f;

	return BRect(left + index * (size + gap), top,
		left + index * (size + gap) + size - 1, top + size - 1);
}


void
ToolBarView::_LayoutButtons()
{
	for (int32 i = 0; i < kToolCount; i++) {
		if (fButtons[i] == NULL)
			continue;
		BRect rect = _ButtonRect(i);
		if (fButtons[i]->Frame() == rect)
			continue;
		fButtons[i]->MoveTo(rect.LeftTop());
		fButtons[i]->ResizeTo(rect.Width(), rect.Height());
	}
}


void
ToolBarView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_COPY);
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(Bounds());
}


void
ToolBarView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_LayoutButtons();
	ToolChanged();
}


void
ToolBarView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_LayoutButtons();
	Invalidate();
}


void
ToolBarView::ToolChanged()
{
	int32 active = fWindow->Tool();
	for (int32 i = 0; i < kToolCount; i++) {
		if (fButtons[i] != NULL)
			fButtons[i]->SetActive(i == active);
	}
}


//	#pragma mark - SwatchView


SwatchView::SwatchView(EditorWindow* window)
	:
	BView(BRect(0, 0, 0, 0), "swatches", B_FOLLOW_NONE, B_WILL_DRAW),
	fWindow(window)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BRect
SwatchView::_FrontRect() const
{
	return BRect(Bounds().left + 2, Bounds().top + 14,
		Bounds().right - 22, Bounds().top + 38);
}


BRect
SwatchView::_BackRect() const
{
	return BRect(Bounds().left + 2, Bounds().top + 52,
		Bounds().right - 22, Bounds().top + 76);
}


BRect
SwatchView::_TransparentRect() const
{
	BRect back = _BackRect();
	return BRect(Bounds().right - 19, back.top,
		Bounds().right - 2, back.top + 17);
}


void
SwatchView::_DrawChip(BRect rect, uint8 index)
{
	SetDrawingMode(B_OP_COPY);
	if (IconPalette::IsTransparent(index)) {
		DrawCheckerboard(this, rect, 5, MakeColor(196, 196, 196));
	} else {
		const IconRGB& color = IconPalette::kColors[index];
		SetHighColor(MakeColor(color.r, color.g, color.b));
		FillRect(rect);
	}

	SetHighColor(MakeColor(60, 60, 60));
	StrokeRect(rect);

	// Show the palette index inside the chip.
	BString text;
	text.SetToFormat("%d", (int)index);
	SetHighColor(MakeColor(0, 0, 0));
	SetDrawingMode(B_OP_OVER);
	float textWidth = StringWidth(text.String());
	BPoint point((rect.left + rect.right - textWidth) / 2.0f,
		rect.bottom - 4);
	if (!IconPalette::IsTransparent(index)) {
		const IconRGB& color = IconPalette::kColors[index];
		int luminance = (color.r * 30 + color.g * 59 + color.b * 11) / 100;
		SetHighColor(luminance > 128 ? MakeColor(0, 0, 0)
			: MakeColor(255, 255, 255));
	}
	DrawString(text.String(), point);
	SetDrawingMode(B_OP_COPY);
}


void
SwatchView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_COPY);
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(Bounds());

	BFont font(*be_plain_font);
	font.SetSize(10.0f);
	SetFont(&font);
	SetHighColor(MakeColor(40, 40, 40));
	DrawString("Front", BPoint(Bounds().left + 3, Bounds().top + 11));
	DrawString("Back", BPoint(Bounds().left + 3, Bounds().top + 49));

	_DrawChip(_FrontRect(), fWindow->FrontColor());
	_DrawChip(_BackRect(), fWindow->BackColor());

	BRect transparent = _TransparentRect();
	bool isTransparent = IconPalette::IsTransparent(fWindow->BackColor());
	SetHighColor(isTransparent ? MakeColor(196, 212, 238)
		: MakeColor(246, 246, 246));
	FillRect(transparent);
	SetHighColor(MakeColor(90, 90, 90));
	StrokeRect(transparent);
	SetHighColor(MakeColor(0, 0, 0));
	font.SetSize(10.0f);
	SetFont(&font);
	float textWidth = StringWidth("T");
	DrawString("T", BPoint((transparent.left + transparent.right
		- textWidth) / 2.0f, transparent.bottom - 4));

	SetFont(be_plain_font);
}


void
SwatchView::MouseDown(BPoint where)
{
	if (_TransparentRect().Contains(where)) {
		fWindow->SetBackTransparent();
		return;
	}
	if (_FrontRect().Contains(where) || _BackRect().Contains(where)) {
		fWindow->SwapColors();
		return;
	}
}


//	#pragma mark - PaletteGridView


PaletteGridView::PaletteGridView(EditorWindow* window)
	:
	BView(BRect(0, 0, 0, 0), "palette", B_FOLLOW_NONE, B_WILL_DRAW),
	fWindow(window),
	fDragging(false),
	fDraggingSecondary(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


float
PaletteGridView::_CellSize() const
{
	float cell = Bounds().Width() / 32.0f;
	float vertical = Bounds().Height() / 8.0f;
	if (vertical < cell)
		cell = vertical;
	cell = floorf(cell);
	if (cell < 4.0f)
		cell = 4.0f;
	if (cell > 24.0f)
		cell = 24.0f;
	return cell;
}


BRect
PaletteGridView::_GridRect() const
{
	float cell = _CellSize();
	float width = cell * 32.0f;
	float height = cell * 8.0f;
	float left = floorf(Bounds().left + (Bounds().Width() - width) / 2.0f);
	float top = floorf(Bounds().top + (Bounds().Height() - height) / 2.0f);
	return BRect(left, top, left + width - 1, top + height - 1);
}


int32
PaletteGridView::_IndexAt(BPoint where) const
{
	BRect grid = _GridRect();
	if (!grid.Contains(where))
		return -1;

	float cell = _CellSize();
	int32 column = (int32)floorf((where.x - grid.left) / cell);
	int32 row = (int32)floorf((where.y - grid.top) / cell);
	if (column < 0 || column > 31 || row < 0 || row > 7)
		return -1;

	return row * 32 + column;
}


void
PaletteGridView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_COPY);
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(Bounds());

	BRect grid = _GridRect();
	float cell = _CellSize();

	for (int32 index = 0; index < 256; index++) {
		int32 column = index % 32;
		int32 row = index / 32;
		BRect rect(grid.left + column * cell, grid.top + row * cell,
			grid.left + (column + 1) * cell - 1,
			grid.top + (row + 1) * cell - 1);

		if (IconPalette::IsTransparent((uint8)index)) {
			DrawCheckerboard(this, rect, (int32)cell / 2 > 2
				? (int32)cell / 2 : 3, MakeColor(190, 190, 190));
		} else {
			const IconRGB& color = IconPalette::kColors[index];
			SetHighColor(MakeColor(color.r, color.g, color.b));
			FillRect(rect);
		}
	}

	// The back colour is marked with a dashed ring, the front colour with a
	// solid black and white double ring.
	uint8 back = fWindow->BackColor();
	int32 backColumn = back % 32;
	int32 backRow = back / 32;
	BRect backRect(grid.left + backColumn * cell,
		grid.top + backRow * cell,
		grid.left + (backColumn + 1) * cell - 1,
		grid.top + (backRow + 1) * cell - 1);
	SetHighColor(MakeColor(0, 0, 0));
	StrokeRect(backRect);
	StrokeDashedRect(this, BRect(backRect.left + 1, backRect.top + 1,
		backRect.right - 1, backRect.bottom - 1), MakeColor(255, 255, 255));

	uint8 front = fWindow->FrontColor();
	int32 frontColumn = front % 32;
	int32 frontRow = front / 32;
	BRect frontRect(grid.left + frontColumn * cell,
		grid.top + frontRow * cell,
		grid.left + (frontColumn + 1) * cell - 1,
		grid.top + (frontRow + 1) * cell - 1);
	SetHighColor(MakeColor(0, 0, 0));
	StrokeRect(frontRect);
	SetHighColor(MakeColor(255, 255, 255));
	StrokeRect(BRect(frontRect.left + 1, frontRect.top + 1,
		frontRect.right - 1, frontRect.bottom - 1));

	SetHighColor(MakeColor(120, 120, 120));
	StrokeRect(grid);
}


void
PaletteGridView::MouseDown(BPoint where)
{
	int32 index = _IndexAt(where);
	if (index < 0)
		return;

	BMessage* message = Window() != NULL ? Window()->CurrentMessage() : NULL;
	int32 buttons = 0;
	if (message != NULL)
		message->FindInt32("buttons", &buttons);

	fDragging = true;
	fDraggingSecondary = (buttons & B_SECONDARY_MOUSE_BUTTON) != 0;
	_Pick(index, fDraggingSecondary, false);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}


void
PaletteGridView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
	if (!fDragging)
		return;

	int32 index = _IndexAt(where);
	if (index >= 0)
		_Pick(index, fDraggingSecondary, true);
}


void
PaletteGridView::MouseUp(BPoint where)
{
	fDragging = false;
}


void
PaletteGridView::_Pick(int32 index, bool secondary, bool dragging)
{
	(void)dragging;
	if (secondary)
		fWindow->SetBackColor((uint8)index);
	else
		fWindow->SetFrontColor((uint8)index);
}


void
PaletteGridView::ColorsChanged()
{
	Invalidate();
}


//	#pragma mark - PreviewView


PreviewView::PreviewView(EditorWindow* window)
	:
	BView(BRect(0, 0, 0, 0), "preview", B_FOLLOW_NONE, B_WILL_DRAW),
	fWindow(window)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BRect
PreviewView::_CellRect(int32 variant) const
{
	const float gap = 6.0f;
	float width = Bounds().Width() - gap * 2.0f;
	float height = (Bounds().Height() - gap * 3.0f) / 2.0f;

	// The large thumbnail sits on top, the mini one below it.
	float left = Bounds().left + gap;
	float top = Bounds().top + gap + variant * (height + gap);
	return BRect(floorf(left), floorf(top), floorf(left + width) - 1,
		floorf(top + height) - 1);
}


int32
PreviewView::_VariantAt(BPoint where) const
{
	for (int32 i = 0; i < kEditableVariantCount; i++) {
		if (_CellRect(i).Contains(where))
			return i;
	}
	return -1;
}


void
PreviewView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_COPY);
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(Bounds());

	BFont font(*be_plain_font);
	font.SetSize(9.0f);

	for (int32 i = 0; i < kEditableVariantCount; i++) {
		BRect cell = _CellRect(i);
		bool active = i == fWindow->ActiveVariant();

		SetHighColor(MakeColor(250, 250, 250));
		FillRect(cell);
		SetHighColor(active ? MakeColor(70, 100, 160)
			: MakeColor(140, 140, 140));
		StrokeRect(cell);
		if (active) {
			StrokeRect(BRect(cell.left + 1, cell.top + 1, cell.right - 1,
				cell.bottom - 1));
		}

		SetFont(&font);
		SetHighColor(MakeColor(40, 40, 40));
		DrawString(IconDocument::VariantShortName(i),
			BPoint(cell.left + 5, cell.top + 12));

		int32 width = fWindow->Document()->VariantWidth(i);
		float available = cell.Width() - 10.0f;
		float availableHeight = cell.Height() - 24.0f;
		if (availableHeight < available)
			available = availableHeight;
		int32 scale = (int32)floorf(available / width);
		if (scale < 1)
			scale = 1;

		float iconSize = width * scale;
		BRect iconRect(floorf(cell.left + (cell.Width() - iconSize) / 2.0f),
			floorf(cell.top + 16.0f + (availableHeight - iconSize) / 2.0f),
			floorf(cell.left + (cell.Width() + iconSize) / 2.0f) - 1,
			floorf(cell.top + 16.0f + (availableHeight + iconSize) / 2.0f)
				- 1);

		DrawIndexedPixels(this, fWindow->Document()->Pixels(i), width,
			iconRect, scale);

		SetHighColor(MakeColor(120, 120, 120));
		StrokeRect(iconRect);
	}

	SetFont(be_plain_font);
}


void
PreviewView::MouseDown(BPoint where)
{
	int32 variant = _VariantAt(where);
	if (variant >= 0)
		fWindow->SetActiveVariant(variant);
}


void
PreviewView::DocumentChanged()
{
	Invalidate();
}


void
PreviewView::VariantChanged()
{
	Invalidate();
}
