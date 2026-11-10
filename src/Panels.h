#ifndef PANELS_H
#define PANELS_H

#include <View.h>

#include "IconDocument.h"
#include "Tools.h"


class EditorWindow;
class ToolButton;


// The tool bar draws nothing itself: it lays out one child view per tool, and
// each of those views carries the tool's tool tip.
class ToolBarView : public BView {
public:
	ToolBarView(EditorWindow* window);

	virtual void Draw(BRect updateRect);
	virtual void AttachedToWindow();
	virtual void FrameResized(float width, float height);

	void ToolChanged();

private:
	void _LayoutButtons();
	BRect _ButtonRect(int32 index) const;
	float _ButtonSize() const;

	EditorWindow* fWindow;
	ToolButton* fButtons[kToolCount];
};


class SwatchView : public BView {
public:
	SwatchView(EditorWindow* window);

	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);

private:
	BRect _FrontRect() const;
	BRect _BackRect() const;
	BRect _TransparentRect() const;
	void _DrawChip(BRect rect, uint8 index);

	EditorWindow* fWindow;
};


class PaletteGridView : public BView {
public:
	PaletteGridView(EditorWindow* window);

	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* message);
	virtual void MouseUp(BPoint where);

	void ColorsChanged();

private:
	BRect _GridRect() const;
	float _CellSize() const;
	int32 _IndexAt(BPoint where) const;
	void _Pick(int32 index, bool secondary, bool dragging);

	EditorWindow* fWindow;
	bool fDragging;
	bool fDraggingSecondary;
};


class PreviewView : public BView {
public:
	PreviewView(EditorWindow* window);

	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);

	void DocumentChanged();
	void VariantChanged();

private:
	BRect _CellRect(int32 variant) const;
	int32 _VariantAt(BPoint where) const;

	EditorWindow* fWindow;
};

#endif // PANELS_H
