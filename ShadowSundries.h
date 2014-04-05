#pragma once

#include "shadeMeInternals.h"

namespace ShadowSundries
{
	namespace EditorSupport
	{
		_DeclareMemHdlr(EnableCastsShadowsFlag, "allows the flag to be set on non-light refs");
	}

	namespace SundrySloblock
	{
		_DeclareMemHdlr(ConsoleDebugSelectionA, "provides more detail about the console debug selection");
		_DeclareMemHdlr(ConsoleDebugSelectionB, "");
	}

	void Patch(bool Editor);
}