#include "EditorWindow.h"

#include "HelpWindow.h"
#include "IconCanvasView.h"
#include "Messages.h"
#include "Palette.h"
#include "Panels.h"
#include "Tools.h"

#include <Alert.h>
#include <Application.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Path.h>
#include <StringView.h>
#include <TextControl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// BView::SetFrame() belongs to the private part of the Haiku API; a child
// view is positioned and sized with the public MoveTo() and ResizeTo().
static void
SetViewFrame(BView* view, BRect frame)
{
	view->MoveTo(frame.LeftTop());
	view->ResizeTo(frame.Width(), frame.Height());
}


EditorWindow::EditorWindow()
	:
	BWindow(BRect(90, 70, 570, 580), "Bitmap-o-matic", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fTool(kToolPencil),
	fVariant(kLargeVariant),
	fFrontColor(0),
	fBackColor(IconPalette::kTransparentIndex),
	fPendingAction(0),
	fStatusText(),
	fMenuBar(NULL),
	fCanvas(NULL),
	fPreview(NULL),
	fToolBar(NULL),
	fSwatches(NULL),
	fPalette(NULL),
	fRed(NULL),
	fGreen(NULL),
	fBlue(NULL),
	fStatus(NULL),
	fUndoItem(NULL),
	fRedoItem(NULL),
	fCutItem(NULL),
	fCopyItem(NULL),
	fPasteItem(NULL),
	fClearItem(NULL),
	fOpenPanel(NULL),
	fSavePanel(NULL),
	fAttachPanel(NULL),
	fHelpWindow(NULL)
{
	memset(&fClipboard, 0, sizeof(fClipboard));
	fClipboard.width = 0;
	fClipboard.height = 0;
	fClipboard.valid = false;
	memset(fClipboard.pixels, IconPalette::kTransparentIndex,
		sizeof(fClipboard.pixels));

	for (int32 i = 0; i < kToolCount; i++)
		fToolItems[i] = NULL;
	for (int32 i = 0; i < kIconVariantCount; i++)
		fVariantItems[i] = NULL;

	_BuildMenus();

	fCanvas = new IconCanvasView(this);
	fPreview = new PreviewView(this);
	fToolBar = new ToolBarView(this);
	fSwatches = new SwatchView(this);
	fPalette = new PaletteGridView(this);

	fRed = new BTextControl("red", "Red:", "0", new BMessage(MSG_SET_RGB));
	fGreen = new BTextControl("green", "Green:", "0",
		new BMessage(MSG_SET_RGB));
	fBlue = new BTextControl("blue", "Blue:", "0",
		new BMessage(MSG_SET_RGB));

	fRed->SetDivider(34.0f);
	fGreen->SetDivider(34.0f);
	fBlue->SetDivider(34.0f);

	fRed->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fGreen->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fBlue->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);

	BFont smallFont(*be_plain_font);
	smallFont.SetSize(10.0f);

	fStatus = new BStringView("status", "");
	fStatus->SetFont(&smallFont);
	fStatus->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	AddChild(fMenuBar);
	AddChild(fCanvas);
	AddChild(fPreview);
	AddChild(fToolBar);
	AddChild(fSwatches);
	AddChild(fPalette);
	AddChild(fRed);
	AddChild(fGreen);
	AddChild(fBlue);
	AddChild(fStatus);

	_Layout();
	fCanvas->MakeFocus(true);
	_UpdateTitle();
	_UpdateColorControls();
	_UpdateToolMark();
	_UpdateVariantMark();
	_UpdateMenuStates();
	CenterOnScreen();

	BString status;
	status.SetToFormat("Pencil — drag with the left button for the front "
		"colour, with the right button for the back colour");
	UpdateStatus(status);
}


EditorWindow::~EditorWindow()
{
	_DeleteFilePanels();
}


void
EditorWindow::Quit()
{
	// The file panels are loopers of their own; they have to go before the
	// window does, or the application never shuts down cleanly.
	_DeleteFilePanels();
	_CloseHelp();
	BWindow::Quit();
}


void
EditorWindow::_DeleteFilePanels()
{
	delete fOpenPanel;
	fOpenPanel = NULL;
	delete fSavePanel;
	fSavePanel = NULL;
	delete fAttachPanel;
	fAttachPanel = NULL;
}


void
EditorWindow::_BuildMenus()
{
	fMenuBar = new BMenuBar(BRect(0, 0, 100, 20), "menu bar");

	BMenu* fileMenu = new BMenu("File");
	fileMenu->AddItem(new BMenuItem("New", new BMessage(MSG_NEW), 'N',
		B_COMMAND_KEY));
	fileMenu->AddItem(new BMenuItem("Open…", new BMessage(MSG_OPEN), 'O',
		B_COMMAND_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Save", new BMessage(MSG_SAVE), 'S',
		B_COMMAND_KEY));
	fileMenu->AddItem(new BMenuItem("Save As…", new BMessage(MSG_SAVE_AS),
		'S', B_COMMAND_KEY | B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Attach Icons to File…",
		new BMessage(MSG_ATTACH_ICONS)));
	fileMenu->AddItem(new BMenuItem("Revert", new BMessage(MSG_REVERT)));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Quit", new BMessage(MSG_QUIT), 'Q',
		B_COMMAND_KEY));
	fMenuBar->AddItem(fileMenu);

	BMenu* editMenu = new BMenu("Edit");

	fUndoItem = new BMenuItem("Undo", new BMessage(MSG_UNDO), 'Z',
		B_COMMAND_KEY);
	fRedoItem = new BMenuItem("Redo", new BMessage(MSG_REDO), 'Z',
		B_COMMAND_KEY | B_SHIFT_KEY);
	editMenu->AddItem(fUndoItem);
	editMenu->AddItem(fRedoItem);
	editMenu->AddSeparatorItem();

	fCutItem = new BMenuItem("Cut", new BMessage(MSG_CUT), 'X',
		B_COMMAND_KEY);
	fCopyItem = new BMenuItem("Copy", new BMessage(MSG_COPY), 'C',
		B_COMMAND_KEY);
	fPasteItem = new BMenuItem("Paste", new BMessage(MSG_PASTE), 'V',
		B_COMMAND_KEY);
	fClearItem = new BMenuItem("Clear", new BMessage(MSG_CLEAR));
	editMenu->AddItem(fCutItem);
	editMenu->AddItem(fCopyItem);
	editMenu->AddItem(fPasteItem);
	editMenu->AddItem(fClearItem);
	editMenu->AddSeparatorItem();

	editMenu->AddItem(new BMenuItem("Select All",
		new BMessage(MSG_SELECT_ALL), 'A', B_COMMAND_KEY));
	editMenu->AddItem(new BMenuItem("Deselect",
		new BMessage(MSG_DESELECT), 'A', B_COMMAND_KEY | B_SHIFT_KEY));
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem("Select All Transparent",
		new BMessage(MSG_SELECT_TRANSPARENT)));
	editMenu->AddItem(new BMenuItem("Select All Non-transparent",
		new BMessage(MSG_SELECT_OPAQUE)));
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem("Swap Front and Back",
		new BMessage(MSG_SWAP_COLORS), 'B', B_COMMAND_KEY));
	editMenu->AddSeparatorItem();

	const char* toolNames[kToolCount] = {
		"Selection", "Eraser", "Pencil", "Eyedropper", "Fill", "Line",
		"Rectangle", "Filled Rectangle", "Rounded Rectangle",
		"Filled Rounded Rectangle", "Ellipse", "Filled Ellipse"
	};
	const char toolShortcuts[kToolCount] = {
		'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '='
	};

	BMenu* toolsMenu = new BMenu("Tools");
	for (int32 i = 0; i < kToolCount; i++) {
		BMessage* message = new BMessage(MSG_SET_TOOL);
		message->AddInt32("tool", i);
		fToolItems[i] = new BMenuItem(toolNames[i], message,
			toolShortcuts[i], B_COMMAND_KEY);
		toolsMenu->AddItem(fToolItems[i]);
	}
	editMenu->AddItem(toolsMenu);

	BMenu* variantMenu = new BMenu("Icon");
	for (int32 i = 0; i < kEditableVariantCount; i++) {
		BMessage* message = new BMessage(MSG_SET_VARIANT);
		message->AddInt32("variant", i);
		fVariantItems[i] = new BMenuItem(IconDocument::VariantName(i),
			message);
		variantMenu->AddItem(fVariantItems[i]);
	}
	editMenu->AddItem(variantMenu);

	fMenuBar->AddItem(editMenu);

	BMenu* helpMenu = new BMenu("Help");
	helpMenu->AddItem(new BMenuItem("About Bitmap-o-matic",
		new BMessage(MSG_ABOUT)));
	helpMenu->AddSeparatorItem();
	helpMenu->AddItem(new BMenuItem("Bitmap-o-matic Help",
		new BMessage(MSG_HELP), '?', B_COMMAND_KEY));
	fMenuBar->AddItem(helpMenu);
}


void
EditorWindow::_Layout()
{
	BRect bounds = Bounds();
	float width = bounds.Width();
	float height = bounds.Height();

	const float margin = 8.0f;
	const float statusHeight = 18.0f;
	const float panelHeight = 104.0f;
	const float toolsHeight = 52.0f;
	const float previewWidth = 140.0f;
	const float rgbWidth = 116.0f;
	const float swatchWidth = 74.0f;
	const float swatchHeight = 76.0f;

	float menuHeight = 22.0f;
	if (fMenuBar != NULL) {
		float preferredWidth = 0.0f;
		float preferredHeight = 0.0f;
		fMenuBar->GetPreferredSize(&preferredWidth, &preferredHeight);
		if (preferredHeight > 10.0f)
			menuHeight = preferredHeight;
		SetViewFrame(fMenuBar, BRect(0, 0, width, menuHeight - 1));
	}

	float statusTop = height - statusHeight;
	if (fStatus != NULL)
		SetViewFrame(fStatus, BRect(margin, statusTop + 2, width - margin,
			height - 1));

	float panelBottom = statusTop - margin;
	float panelTop = panelBottom - panelHeight;

	// The colour chips sit in the bottom right corner; the palette and the
	// RGB fields share the rest of this row, so the window stays narrow.
	float swatchesLeft = width - margin - swatchWidth;
	if (fSwatches != NULL) {
		float swatchesTop = panelBottom - swatchHeight;
		if (swatchesTop < panelTop)
			swatchesTop = panelTop;
		SetViewFrame(fSwatches, BRect(swatchesLeft, swatchesTop,
			width - margin, panelBottom));
	}

	float rgbLeft = swatchesLeft - margin - rgbWidth;
	if (fRed != NULL) {
		SetViewFrame(fRed, BRect(rgbLeft, panelTop + 2, swatchesLeft - margin,
			panelTop + 26));
		SetViewFrame(fGreen, BRect(rgbLeft, panelTop + 34,
			swatchesLeft - margin, panelTop + 58));
		SetViewFrame(fBlue, BRect(rgbLeft, panelTop + 66,
			swatchesLeft - margin, panelTop + 90));
	}

	if (fPalette != NULL)
		SetViewFrame(fPalette, BRect(margin, panelTop, rgbLeft - margin,
			panelBottom));

	float toolsBottom = panelTop - margin;
	float toolsTop = toolsBottom - toolsHeight;
	if (fToolBar != NULL)
		SetViewFrame(fToolBar, BRect(margin, toolsTop, width - margin,
			toolsBottom));

	float canvasTop = menuHeight + margin;
	float canvasBottom = toolsTop - margin;
	float canvasRight = width - margin - previewWidth - margin;

	if (fCanvas != NULL)
		SetViewFrame(fCanvas, BRect(margin, canvasTop, canvasRight,
			canvasBottom));

	if (fPreview != NULL)
		SetViewFrame(fPreview, BRect(canvasRight + margin, canvasTop,
			width - margin, canvasBottom));
}


void
EditorWindow::FrameResized(float width, float height)
{
	BWindow::FrameResized(width, height);
	_Layout();
}


void
EditorWindow::_UpdateTitle()
{
	BString name = fDocument.FileName();
	BString title;
	title.SetToFormat("%s%s — Bitmap-o-matic",
		fDocument.IsDirty() ? "* " : "", name.String());
	SetTitle(title.String());
}


void
EditorWindow::_UpdateColorControls()
{
	const IconRGB& color = IconPalette::kColors[fFrontColor];

	BString value;
	value.SetToFormat("%d", (int)color.r);
	fRed->SetText(value.String());
	value.SetToFormat("%d", (int)color.g);
	fGreen->SetText(value.String());
	value.SetToFormat("%d", (int)color.b);
	fBlue->SetText(value.String());
}


void
EditorWindow::_UpdateToolMark()
{
	for (int32 i = 0; i < kToolCount; i++) {
		if (fToolItems[i] != NULL)
			fToolItems[i]->SetMarked(i == fTool);
	}
}


void
EditorWindow::_UpdateVariantMark()
{
	for (int32 i = 0; i < kIconVariantCount; i++) {
		if (fVariantItems[i] != NULL)
			fVariantItems[i]->SetMarked(i == fVariant);
	}
}


void
EditorWindow::_UpdateMenuStates()
{
	if (fUndoItem != NULL)
		fUndoItem->SetEnabled(fDocument.CanUndo());
	if (fRedoItem != NULL)
		fRedoItem->SetEnabled(fDocument.CanRedo());

	bool hasSelection = fCanvas != NULL && fCanvas->HasSelection();
	if (fCutItem != NULL)
		fCutItem->SetEnabled(hasSelection);
	if (fCopyItem != NULL)
		fCopyItem->SetEnabled(hasSelection);
	if (fPasteItem != NULL)
		fPasteItem->SetEnabled(fClipboard.valid);
	if (fClearItem != NULL)
		fClearItem->SetEnabled(true);
}


void
EditorWindow::MenusBeginning()
{
	BWindow::MenusBeginning();
	_UpdateMenuStates();
	_UpdateToolMark();
	_UpdateVariantMark();
}


void
EditorWindow::SetTool(int32 tool)
{
	if (tool < 0 || tool >= kToolCount)
		return;

	fTool = tool;
	fCanvas->ToolChanged();
	fToolBar->ToolChanged();
	_UpdateToolMark();

	BString status;
	status.SetToFormat("%s selected", ToolName(tool));
	UpdateStatus(status);
}


void
EditorWindow::SetActiveVariant(int32 variant)
{
	if (variant < 0 || variant >= kIconVariantCount || variant == fVariant)
		return;

	fVariant = variant;
	fCanvas->SetVariant(variant);
	fPreview->VariantChanged();
	_UpdateVariantMark();

	BString status;
	status.SetToFormat("Editing the %s",
		IconDocument::VariantShortName(variant));
	UpdateStatus(status);
}


void
EditorWindow::SetFrontColor(uint8 index)
{
	fFrontColor = index;
	fCanvas->ColorsChanged();
	fSwatches->Invalidate();
	fPalette->ColorsChanged();
	_UpdateColorControls();
}


void
EditorWindow::SetBackColor(uint8 index)
{
	fBackColor = index;
	fSwatches->Invalidate();
	fPalette->ColorsChanged();
}


void
EditorWindow::SetBackTransparent()
{
	SetBackColor(IconPalette::kTransparentIndex);
	UpdateStatus("The back colour is now transparent");
}


void
EditorWindow::SwapColors()
{
	uint8 temporary = fFrontColor;
	fFrontColor = fBackColor;
	fBackColor = temporary;

	fCanvas->ColorsChanged();
	fSwatches->Invalidate();
	fPalette->ColorsChanged();
	_UpdateColorControls();
	UpdateStatus("Swapped the front and back colours");
}


void
EditorWindow::PushUndo(int32 variant)
{
	fDocument.PushUndo(variant);
}


void
EditorWindow::DocumentEdited()
{
	fDocument.SetDirty(true);
	_UpdateTitle();
	fCanvas->DocumentChanged();
	fPreview->DocumentChanged();
	_UpdateMenuStates();
}


void
EditorWindow::UpdateStatus(const BString& text)
{
	fStatusText = text;
	_RefreshStatus();
}


void
EditorWindow::_RefreshStatus()
{
	if (fStatus == NULL)
		return;

	// The status line carries the last message and, once the icon has a file,
	// the full path of that file.
	BString line = fStatusText;
	const char* path = fDocument.Path();
	if (path != NULL && *path != '\0') {
		if (line.Length() > 0)
			line << "  —  ";
		line << path;
	}
	fStatus->SetText(line.String());
}


void
EditorWindow::RefreshInterface()
{
	fCanvas->DocumentChanged();
	fPreview->DocumentChanged();
	fSwatches->Invalidate();
	fPalette->ColorsChanged();
	_UpdateTitle();
	_UpdateColorControls();
	_UpdateMenuStates();
	_RefreshStatus();
}


void
EditorWindow::_ShowError(const char* text)
{
	BAlert* alert = new BAlert("Bitmap-o-matic", text, "OK", NULL, NULL,
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


void
EditorWindow::_NewDocument()
{
	fDocument.New();
	fVariant = kLargeVariant;
	fCanvas->SetVariant(kLargeVariant);
	fCanvas->Deselect();
	fClipboard.valid = false;
	fPendingAction = 0;

	RefreshInterface();
	fPreview->VariantChanged();
	UpdateStatus("New icon");
}


bool
EditorWindow::_MaybeSave(uint32 pendingAction)
{
	if (!fDocument.IsDirty())
		return true;

	BAlert* alert = new BAlert("Unsaved changes",
		"This icon has been modified. Save the changes before continuing?",
		"Save", "Cancel", "Don't Save", B_WIDTH_AS_USUAL,
		B_OFFSET_SPACING, B_WARNING_ALERT);
	int32 choice = alert->Go();

	if (choice == 1) {
		fPendingAction = 0;
		return false;
	}
	if (choice == 2) {
		fPendingAction = 0;
		return true;
	}

	if (fDocument.Path()[0] == '\0') {
		fPendingAction = pendingAction;
		_ShowSavePanel("Untitled.beicon");
		return false;
	}

	BString error;
	if (fDocument.Save(error) != B_OK) {
		_ShowError(error.String());
		if (fDocument.IsDirty())
			return false;
	}

	fPendingAction = 0;
	_UpdateTitle();
	_RefreshStatus();
	return true;
}


void
EditorWindow::_OpenPath(const char* path)
{
	BString error;
	status_t result = fDocument.Open(path, error);
	if (result != B_OK) {
		_ShowError(error.String());
		return;
	}

	fVariant = kLargeVariant;
	fCanvas->SetVariant(kLargeVariant);
	fCanvas->Deselect();
	fClipboard.valid = false;
	fPendingAction = 0;

	RefreshInterface();
	fPreview->VariantChanged();

	// The full path is appended to the status line by _RefreshStatus().
	UpdateStatus("Opened");
}


void
EditorWindow::_SaveDocument()
{
	if (fDocument.Path()[0] == '\0') {
		_ShowSavePanel("Untitled.beicon");
		return;
	}

	BString error;
	if (fDocument.Save(error) != B_OK) {
		_ShowError(error.String());
		if (fDocument.IsDirty())
			return;
	}

	fDocument.SetDirty(false);
	RefreshInterface();

	UpdateStatus("Saved");
}


void
EditorWindow::_Revert()
{
	if (fDocument.Path()[0] == '\0') {
		UpdateStatus("This icon has not been saved yet");
		return;
	}

	BString path(fDocument.Path());
	BString error;
	if (fDocument.Open(path.String(), error) != B_OK) {
		_ShowError(error.String());
		return;
	}

	fVariant = kLargeVariant;
	fCanvas->SetVariant(kLargeVariant);
	fCanvas->Deselect();

	RefreshInterface();
	fPreview->VariantChanged();
	UpdateStatus("Reverted to the saved version");
}


void
EditorWindow::_ShowOpenPanel()
{	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
			B_FILE_NODE | B_DIRECTORY_NODE, false,
			new BMessage(MSG_OPEN_PANEL_DONE));
	}
	fOpenPanel->Show();
}


void
EditorWindow::_ShowSavePanel(const char* suggestedName)
{
	if (fSavePanel == NULL) {
		fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL,
			B_FILE_NODE, false, new BMessage(MSG_SAVE_PANEL_DONE));
	}
	fSavePanel->SetSaveText(suggestedName);
	fSavePanel->Show();
}


void
EditorWindow::_ShowAttachPanel()
{	if (fAttachPanel == NULL) {
		fAttachPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
			NULL, B_FILE_NODE | B_DIRECTORY_NODE, false,
			new BMessage(MSG_ATTACH_PANEL_DONE));
	}
	fAttachPanel->Show();
}


void
EditorWindow::_HandleOpenPanel(BMessage* message)
{
	entry_ref reference;
	if (message->FindRef("refs", &reference) != B_OK)
		return;

	BPath path(&reference);
	if (path.InitCheck() != B_OK)
		return;

	_OpenPath(path.Path());
}


bool
EditorWindow::_PathFromSaveMessage(BMessage* message, BPath& path)
{
	entry_ref directory;
	const char* name = NULL;
	if (message->FindRef("directory", &directory) != B_OK
		|| message->FindString("name", &name) != B_OK)
		return false;

	if (path.SetTo(&directory) != B_OK)
		return false;

	return path.Append(name) == B_OK;
}


void
EditorWindow::_HandleSavePanel(BMessage* message)
{
	BPath path;
	if (!_PathFromSaveMessage(message, path))
		return;

	BString error;
	if (fDocument.SaveProject(path.Path(), error) != B_OK) {
		_ShowError(error.String());
		// The project itself may have been written even when attaching the
		// icons to it failed; in that case the document is already clean.
		if (fDocument.IsDirty())
			return;
	}

	uint32 pending = fPendingAction;
	fPendingAction = 0;

	RefreshInterface();

	if (pending == MSG_QUIT) {
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	if (pending == MSG_NEW) {
		_NewDocument();
		return;
	}

	UpdateStatus("Saved");
}


void
EditorWindow::_HandleAttachPanel(BMessage* message)
{
	entry_ref reference;
	if (message->FindRef("refs", &reference) != B_OK)
		return;

	BPath path(&reference);
	if (path.InitCheck() != B_OK)
		return;

	BString error;
	if (fDocument.WriteIconAttributes(path.Path(), error) != B_OK) {
		_ShowError(error.String());
		return;
	}

	RefreshInterface();

	BString status;
	status.SetToFormat("Icons attached to %s", path.Leaf());
	UpdateStatus(status);
}


void
EditorWindow::_ApplyRGB()
{
	int32 red = atoi(fRed->Text());
	int32 green = atoi(fGreen->Text());
	int32 blue = atoi(fBlue->Text());

	if (red < 0) red = 0;
	if (red > 255) red = 255;
	if (green < 0) green = 0;
	if (green > 255) green = 255;
	if (blue < 0) blue = 0;
	if (blue > 255) blue = 255;

	SetFrontColor(IconPalette::ClosestIndex((uint8)red, (uint8)green,
		(uint8)blue));
}


void
EditorWindow::_CopyToClipboard(bool cut)
{
	int32 width = fDocument.VariantWidth(fVariant);
	int32 left = 0;
	int32 top = 0;
	int32 copyWidth = width;
	int32 copyHeight = width;

	if (fCanvas->HasSelection()) {
		BRect selection = fCanvas->Selection();
		left = (int32)selection.left;
		top = (int32)selection.top;
		copyWidth = (int32)selection.Width() + 1;
		copyHeight = (int32)selection.Height() + 1;
	}

	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (copyWidth > width - left) copyWidth = width - left;
	if (copyHeight > width - top) copyHeight = width - top;
	if (copyWidth <= 0 || copyHeight <= 0)
		return;

	const uint8* pixels = fDocument.Pixels(fVariant);
	for (int32 y = 0; y < copyHeight; y++) {
		memcpy(fClipboard.pixels + y * copyWidth,
			pixels + (top + y) * width + left, copyWidth);
	}

	fClipboard.width = copyWidth;
	fClipboard.height = copyHeight;
	fClipboard.valid = true;

	_UpdateMenuStates();

	if (cut) {
		fCanvas->ClearSelection(fBackColor);
		UpdateStatus("Cut");
	} else {
		UpdateStatus("Copied");
	}
}


void
EditorWindow::_PasteFromClipboard()
{
	if (!fClipboard.valid)
		return;

	int32 width = fDocument.VariantWidth(fVariant);
	if (fClipboard.width > width || fClipboard.height > width) {
		UpdateStatus("The copied block is too large for this icon");
		return;
	}

	fCanvas->PasteBlock(fClipboard.pixels, fClipboard.width,
		fClipboard.height);
	UpdateStatus("Pasted — drag the selection to move it");
}


void
EditorWindow::_ClearSelection()
{
	fCanvas->ClearSelection(fBackColor);
	UpdateStatus("Cleared");
}


void
EditorWindow::_SelectAll()
{
	fCanvas->SelectAll();
	UpdateStatus("Selected the whole icon");
}


void
EditorWindow::_Deselect()
{
	fCanvas->Deselect();
	UpdateStatus("Selection cleared");
}


void
EditorWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_NEW:
			if (_MaybeSave(MSG_NEW))
				_NewDocument();
			break;

		case MSG_OPEN:
			_ShowOpenPanel();
			break;

		case MSG_SAVE:
			_SaveDocument();
			break;

		case MSG_SAVE_AS:
		{
			BString name = fDocument.Path()[0] == '\0'
				? BString("Untitled.beicon") : fDocument.FileName();
			_ShowSavePanel(name.String());
			break;
		}

		case MSG_ATTACH_ICONS:
			_ShowAttachPanel();
			break;

		case MSG_REVERT:
			_Revert();
			break;

		case MSG_QUIT:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_UNDO:
			if (fDocument.Undo()) {
				RefreshInterface();
				UpdateStatus("Undo");
			}
			break;

		case MSG_REDO:
			if (fDocument.Redo()) {
				RefreshInterface();
				UpdateStatus("Redo");
			}
			break;

		case MSG_CUT:
			_CopyToClipboard(true);
			break;

		case MSG_COPY:
			_CopyToClipboard(false);
			break;

		case MSG_PASTE:
			_PasteFromClipboard();
			break;

		case MSG_CLEAR:
			_ClearSelection();
			break;

		case MSG_SELECT_ALL:
			_SelectAll();
			break;

		case MSG_SELECT_TRANSPARENT:
			_SelectTransparent(true);
			break;

		case MSG_SELECT_OPAQUE:
			_SelectTransparent(false);
			break;

		case MSG_ABOUT:
			_ShowAbout();
			break;

		case MSG_HELP:
			_ShowHelp();
			break;

		case MSG_HELP_CLOSED:
			fHelpWindow = NULL;
			break;

		case MSG_DESELECT:
			_Deselect();
			break;

		case MSG_SWAP_COLORS:
			SwapColors();
			break;

		case MSG_SET_TOOL:
		{
			int32 tool = -1;
			if (message->FindInt32("tool", &tool) == B_OK)
				SetTool(tool);
			break;
		}

		case MSG_SET_VARIANT:
		{
			int32 variant = -1;
			if (message->FindInt32("variant", &variant) == B_OK)
				SetActiveVariant(variant);
			break;
		}

		case MSG_SET_FRONT:
		{
			int32 index = 0;
			if (message->FindInt32("index", &index) == B_OK)
				SetFrontColor((uint8)index);
			break;
		}

		case MSG_SET_BACK:
		{
			int32 index = 0;
			if (message->FindInt32("index", &index) == B_OK)
				SetBackColor((uint8)index);
			break;
		}

		case MSG_SET_BACK_TRANSPARENT:
			SetBackTransparent();
			break;

		case MSG_SET_RGB:
			_ApplyRGB();
			break;

		case MSG_OPEN_PANEL_DONE:
			_HandleOpenPanel(message);
			break;

		case MSG_SAVE_PANEL_DONE:
			_HandleSavePanel(message);
			break;

		case MSG_ATTACH_PANEL_DONE:
			_HandleAttachPanel(message);
			break;

		case B_CANCEL:
			fPendingAction = 0;
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
EditorWindow::QuitRequested()
{
	return _MaybeSave(MSG_QUIT);
}


void
EditorWindow::_SelectTransparent(bool transparent)
{
	fCanvas->SelectPixels(transparent);
	UpdateStatus(transparent ? "Selected all transparent pixels"
		: "Selected all non-transparent pixels");
}


void
EditorWindow::_ShowAbout()
{
	BString text(
		"Bitmap-o-matic\n"
		"An editor for the 32 x 32 and 16 x 16 bitmap icons that BeOS kept "
		"in the attributes of a file.\n\n"
		"Copyright 2026 Michel Clasquin-Johnson\n\n"
		"Built with the assistance of Gemini and Freebuff.\n\n"
		"Licensing: this program is released into the public domain under "
		"the Unlicense. See the LICENSE file for the full text. It comes "
		"with no warranty of any kind.");

	BAlert* alert = new BAlert("About Bitmap-o-matic", text.String(), "OK",
		NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


void
EditorWindow::_ShowHelp()
{
	if (fHelpWindow != NULL && fHelpWindow->Lock()) {
		fHelpWindow->Activate();
		fHelpWindow->Unlock();
		return;
	}

	fHelpWindow = new HelpWindow(this);
	fHelpWindow->Show();
}


void
EditorWindow::_CloseHelp()
{
	if (fHelpWindow == NULL)
		return;

	HelpWindow* help = fHelpWindow;
	fHelpWindow = NULL;
	if (help->Lock())
		help->Quit();
}
