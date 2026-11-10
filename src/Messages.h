#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

// Commands sent between the window and its views.
enum {
	MSG_NEW = 'newd',
	MSG_OPEN = 'open',
	MSG_SAVE = 'save',
	MSG_SAVE_AS = 'svas',
	MSG_ATTACH_ICONS = 'attc',
	MSG_REVERT = 'rvrt',
	MSG_QUIT = 'quit',

	MSG_UNDO = 'undo',
	MSG_REDO = 'redo',
	MSG_CUT = 'cut ',
	MSG_COPY = 'copy',
	MSG_PASTE = 'pste',
	MSG_CLEAR = 'clr ',
	MSG_SELECT_ALL = 'slct',
	MSG_SELECT_TRANSPARENT = 'sltr',
	MSG_SELECT_OPAQUE = 'slop',
	MSG_DESELECT = 'dslc',
	MSG_SWAP_COLORS = 'swap',

	MSG_SET_TOOL = 'tool',
	MSG_SET_VARIANT = 'vari',
	MSG_SET_FRONT = 'frnt',
	MSG_SET_BACK = 'back',
	MSG_SET_RGB = 'setr',
	MSG_SET_BACK_TRANSPARENT = 'bktr',

	MSG_OPEN_PANEL_DONE = 'opdn',
	MSG_SAVE_PANEL_DONE = 'spdn',
	MSG_ATTACH_PANEL_DONE = 'apdn',

	MSG_ABOUT = 'abou',
	MSG_HELP = 'help',
	MSG_HELP_CLOSED = 'hlpc',
	MSG_DOCUMENT_CHANGED = 'docd'
};

#endif // MESSAGES_H
