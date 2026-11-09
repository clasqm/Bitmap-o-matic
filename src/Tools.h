#ifndef TOOLS_H
#define TOOLS_H

#include <stdint.h>

enum IconTool {
	kToolSelect = 0,
	kToolEraser,
	kToolPencil,
	kToolEyedropper,
	kToolFill,
	kToolLine,
	kToolRectangle,
	kToolFilledRectangle,
	kToolRoundRectangle,
	kToolFilledRoundRectangle,
	kToolEllipse,
	kToolFilledEllipse,
	kToolCount
};

inline const char*
ToolName(int32_t tool)
{
	switch (tool) {
		case kToolSelect:
			return "Selection";
		case kToolEraser:
			return "Eraser";
		case kToolPencil:
			return "Pencil";
		case kToolEyedropper:
			return "Eyedropper";
		case kToolFill:
			return "Fill";
		case kToolLine:
			return "Line";
		case kToolRectangle:
			return "Rectangle";
		case kToolFilledRectangle:
			return "Filled rectangle";
		case kToolRoundRectangle:
			return "Rounded rectangle";
		case kToolFilledRoundRectangle:
			return "Filled rounded rectangle";
		case kToolEllipse:
			return "Ellipse";
		case kToolFilledEllipse:
			return "Filled ellipse";
	}
	return "Tool";
}

#endif // TOOLS_H
