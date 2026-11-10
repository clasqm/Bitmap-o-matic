#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include <String.h>
#include <Window.h>

#include "IconDocument.h"
#include "Tools.h"


class BFilePanel;
class BMenuBar;
class HelpWindow;
class BPath;
class BMenuItem;
class BStringView;
class BTextControl;
class IconCanvasView;
class PaletteGridView;
class PreviewView;
class SwatchView;
class ToolBarView;


class EditorWindow : public BWindow {
public:
	EditorWindow();
	virtual ~EditorWindow();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();
	virtual void Quit();
	virtual void MenusBeginning();
	virtual void FrameResized(float width, float height);

	IconDocument* Document() { return &fDocument; }
	IconCanvasView* Canvas() { return fCanvas; }

	int32 Tool() const { return fTool; }
	void SetTool(int32 tool);

	int32 ActiveVariant() const { return fVariant; }
	void SetActiveVariant(int32 variant);

	uint8 FrontColor() const { return fFrontColor; }
	uint8 BackColor() const { return fBackColor; }
	void SetFrontColor(uint8 index);
	void SetBackColor(uint8 index);
	void SetBackTransparent();
	void SwapColors();

	// Called by the canvas before it modifies a variant.
	void PushUndo(int32 variant);
	void DocumentEdited();
	void UpdateStatus(const BString& text);
	void RefreshInterface();

private:
	void _BuildMenus();
	void _Layout();
	void _UpdateTitle();
	void _UpdateColorControls();
	void _RefreshStatus();
	void _DeleteFilePanels();
	void _CloseHelp();
	void _ShowAbout();
	void _ShowHelp();
	void _UpdateToolMark();
	void _UpdateVariantMark();
	void _UpdateMenuStates();

	void _NewDocument();
	bool _MaybeSave(uint32 pendingAction);
	void _OpenPath(const char* path);
	void _SaveDocument();
	void _Revert();
	void _ApplyRGB();
	void _ShowOpenPanel();
	void _ShowSavePanel(const char* suggestedName);
	void _ShowAttachPanel();
	void _HandleOpenPanel(BMessage* message);
	void _HandleSavePanel(BMessage* message);
	void _HandleAttachPanel(BMessage* message);
	static bool _PathFromSaveMessage(BMessage* message, BPath& path);

	void _CopyToClipboard(bool cut);
	void _PasteFromClipboard();
	void _ClearSelection();
	void _SelectAll();
	void _SelectTransparent(bool transparent);
	void _Deselect();

	void _ShowError(const char* text);

	struct Clipboard {
		uint8 pixels[1024];
		int32 width;
		int32 height;
		bool valid;
	};

	IconDocument fDocument;
	int32 fTool;
	int32 fVariant;
	uint8 fFrontColor;
	uint8 fBackColor;
	Clipboard fClipboard;
	uint32 fPendingAction;
	BString fStatusText;

	BMenuBar* fMenuBar;
	IconCanvasView* fCanvas;
	PreviewView* fPreview;
	ToolBarView* fToolBar;
	SwatchView* fSwatches;
	PaletteGridView* fPalette;
	BTextControl* fRed;
	BTextControl* fGreen;
	BTextControl* fBlue;
	BStringView* fStatus;

	BMenuItem* fToolItems[kToolCount];
	BMenuItem* fVariantItems[kIconVariantCount];
	BMenuItem* fUndoItem;
	BMenuItem* fRedoItem;
	BMenuItem* fCutItem;
	BMenuItem* fCopyItem;
	BMenuItem* fPasteItem;
	BMenuItem* fClearItem;

	BFilePanel* fOpenPanel;
	BFilePanel* fSavePanel;
	BFilePanel* fAttachPanel;
	HelpWindow* fHelpWindow;
};

#endif // EDITOR_WINDOW_H
