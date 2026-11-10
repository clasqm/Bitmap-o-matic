#ifndef HELP_WINDOW_H
#define HELP_WINDOW_H

#include <Window.h>


class EditorWindow;


// A plain text window with the program's help; it closes with the editor.
class HelpWindow : public BWindow {
public:
	HelpWindow(EditorWindow* owner);

	virtual bool QuitRequested();

private:
	EditorWindow* fOwner;
};

#endif // HELP_WINDOW_H
