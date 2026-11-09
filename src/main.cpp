#include <Application.h>

#include "EditorWindow.h"


int
main()
{
	BApplication application("application/x-vnd.Clasqm-BitmapOMatic");

	EditorWindow* window = new EditorWindow();
	window->Show();

	application.Run();
	return 0;
}
