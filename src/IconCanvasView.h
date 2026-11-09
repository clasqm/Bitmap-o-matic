#ifndef ICON_CANVAS_VIEW_H
#define ICON_CANVAS_VIEW_H

#include <View.h>

#include "IconDocument.h"


class EditorWindow;


class IconCanvasView : public BView {
public:
	IconCanvasView(EditorWindow* window);
	virtual ~IconCanvasView();

	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* message);
	virtual void MouseUp(BPoint where);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void FrameResized(float width, float height);
	virtual void AttachedToWindow();

	void SetVariant(int32 variant);
	int32 Variant() const { return fVariant; }

	void DocumentChanged();
	void ColorsChanged();
	void ToolChanged();

	bool HasSelection() const { return fHasSelection; }
	BRect Selection() const { return fSelection; }
	void SelectAll();
	void Deselect();

	// Pastes an index buffer, either at the current selection or at the
	// top left corner, and selects the result.
	void PasteBlock(const uint8* pixels, int32 width, int32 height);

	// Fills the selection, or the whole icon when nothing is selected.
	void ClearSelection(uint8 color);

	int32 PixelWidth() const;
	int32 PixelHeight() const;

private:
	void _UpdateGeometry();
	BRect _PixelBoard() const;
	bool _PixelAt(BPoint where, int32& x, int32& y) const;

	const uint8* _Pixels();
	void _DrawCheckerboard(BRect area);
	void _DrawSelection();
	void _DrawPixel(BRect pixelRect, uint8 index);

	void _SetPixel(int32 x, int32 y, uint8 value);
	void _Changed();
	void _CancelInteraction();

	void _BeginSelection(int32 x, int32 y);
	void _UpdateSelection(int32 x, int32 y);
	BRect _NormalizedSelection() const;
	void _BeginMove(int32 x, int32 y);
	void _BuildMovePreview();
	void _CommitMove();
	void _CommitShape(uint8 color);
	void _UpdateShapePreview(uint8 color);
	void _UpdateHover(int32 x, int32 y, bool valid);

	EditorWindow* fWindow;

	int32 fVariant;
	int32 fCellSize;
	float fBoardX;
	float fBoardY;
	float fBoardSize;

	// Freehand drawing
	bool fPainting;
	int32 fLastX;
	int32 fLastY;

	// Shape preview
	bool fPreviewActive;
	uint8 fPreviewPixels[1024];
	int32 fShapeStartX;
	int32 fShapeStartY;
	int32 fShapeEndX;
	int32 fShapeEndY;

	// Selection
	bool fHasSelection;
	BRect fSelection;
	bool fSelecting;
	int32 fSelectAnchorX;
	int32 fSelectAnchorY;
	int32 fSelectCurrentX;
	int32 fSelectCurrentY;

	// Dragging a selection
	bool fMoveCandidate;
	bool fMoving;
	uint8 fMovePixels[1024];
	int32 fMoveWidth;
	int32 fMoveHeight;
	BRect fMoveSource;
	int32 fMoveGrabX;
	int32 fMoveGrabY;
	int32 fMoveDeltaX;
	int32 fMoveDeltaY;

	// Last colour used for the current stroke, so a right-button drag
	// keeps using the back colour.
	uint8 fStrokeColor;
};

#endif // ICON_CANVAS_VIEW_H
