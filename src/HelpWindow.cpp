#include "HelpWindow.h"

#include "EditorWindow.h"
#include "Messages.h"

#include <InterfaceDefs.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <TextView.h>


namespace {

const char* kHelpText =
	"Bitmap-o-matic\n"
	"\n"
	"Bitmap-o-matic edits the bitmap icons that BeOS used before HVIF: a "
	"32 x 32 and a 16 x 16 image of 256 palette colours, kept in the "
	"attributes of an ordinary file. Haiku still draws such icons, but "
	"nothing was left that could edit them. This program restores that "
	"ability, so old icon sets can be repaired and new ones drawn.\n"
	"\n"
	"The window\n"
	"\n"
	"The grid in the middle is the image you are editing; the checkerboard "
	"pattern shows where it is transparent. The two thumbnails on the right "
	"are the large (32 x 32) and the mini (16 x 16) image: the one with the "
	"highlighted border is the one being edited, and clicking the other one "
	"switches to it. They are edited as separate pictures; there is no "
	"automatic scaling between them.\n"
	"\n"
	"The row of buttons above the palette holds the twelve tools; hold the "
	"mouse still over a button and its name appears. The same tools are in "
	"the Edit menu under Tools, with the keys 1 to 0, - and =.\n"
	"\n"
	"Colours\n"
	"\n"
	"The palette shows the 256 colours of the BeOS system palette. Click a "
	"colour with the left button to make it the front colour, or with the "
	"right button to make it the back colour. The chips in the bottom right "
	"corner show both: click them to swap the two. The T button beside them "
	"turns the back colour transparent, which is how pixels are erased. The "
	"three number fields set the front colour from a red, green and blue "
	"value; the nearest palette entry is chosen.\n"
	"\n"
	"Drawing\n"
	"\n"
	"Drag inside the grid with the left button to draw with the front "
	"colour, or with the right button to draw with the back colour. Pencil "
	"and eraser draw freehand; line, rectangle, rounded rectangle and "
	"ellipse preview the shape while you drag and draw it when you release "
	"the button, in outline or filled as the tool says. Bucket fill floods "
	"the area of one colour under the pointer. The eyedropper picks the "
	"colour already in the icon instead of drawing.\n"
	"\n"
	"Selecting\n"
	"\n"
	"The selection tool drags a rectangle. Drag inside an existing "
	"selection to move those pixels somewhere else. Edit > Select All "
	"selects the whole image, and Edit > Select All Transparent and Select "
	"All Non-transparent select only the transparent or only the drawn "
	"pixels, which is handy for moving a drawing or making everything around "
	"it transparent. Clear (or the Delete key) fills the selection with the "
	"back colour, so with a transparent back colour it erases. Cut, Copy and "
	"Paste work within the program, and Escape deselects.\n"
	"\n"
	"Files\n"
	"\n"
	"File > Open opens a .beicon project, a file that already carries "
	"BEOS:L:ICON and BEOS:M:ICON attributes, or a raw 1024- or 256-byte "
	"B_CMAP8 icon stream left over from an old BeOS program. The format is "
	"recognised from the contents, not from the file name.\n"
	"\n"
	"Save writes a .beicon project file with both images and also attaches "
	"them to that very file, so Tracker shows the icon straight away. Save As "
	"does the same under a new name. File > Attach Icons to File writes the "
	"icons into the attributes of some other file that already exists, "
	"without changing its contents. Revert throws away the changes and "
	"reloads the file from disk.\n"
	"\n"
	"Undo and Redo in the Edit menu step through the changes made to the "
	"image being edited.\n";

} // namespace


HelpWindow::HelpWindow(EditorWindow* owner)
	:
	BWindow(BRect(120, 120, 600, 600), "Bitmap-o-matic Help",
		B_TITLED_WINDOW, B_NOT_ZOOMABLE),
	fOwner(owner)
{
	SetSizeLimits(320, 1200, 240, 2000);

	BRect bounds = Bounds();
	BRect textRect = bounds;
	textRect.OffsetTo(0, 0);
	textRect.InsetBy(6, 6);

	BTextView* text = new BTextView(bounds, "help text", textRect,
		B_FOLLOW_ALL);
	text->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	text->SetText(kHelpText);
	text->SetWordWrap(true);
	text->MakeEditable(false);
	text->MakeSelectable(true);

	BScrollView* scroll = new BScrollView("help scroll", text, B_FOLLOW_ALL,
		0, false, true);
	AddChild(scroll);

	CenterOnScreen();
}


bool
HelpWindow::QuitRequested()
{
	// Let the editor forget this window, so that the next Help command
	// makes a fresh one.
	BMessenger messenger(fOwner);
	if (messenger.IsValid())
		messenger.SendMessage(MSG_HELP_CLOSED);

	return true;
}
