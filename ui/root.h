#pragma once
#include <nanovg.h>
#include "GainBar.h"


namespace ava::ui {


class Root {
public:
void draw(NVGcontext* vg, int width, int height, float gainValue);
private:
GainBar gainBar_{};
};


} // namespace ava::ui