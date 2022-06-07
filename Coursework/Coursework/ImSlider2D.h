#pragma once

#include "imGUI/imgui.h"

// from https://gist.github.com/soufianekhiat/4d937b15bf7290abed9608569d229201

namespace ImGui {
	bool InputVec2(char const *pLabel, ImVec2 *pValue, ImVec2 const vMinValue, ImVec2 const vMaxValue, float const fScale = 1.0f);
	bool SliderScalar2D(char const *pLabel, float *fValueX, float *fValueY, const float fMinX, const float fMaxX, const float fMinY, const float fMaxY, float const fZoom = 1.0f);
} // namespace ImGui
