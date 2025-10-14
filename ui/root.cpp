#include "Root.h"
#include "Primitives.h"


namespace ava::ui {


void Root::draw(NVGcontext* vg, int width, int height, float gainValue) {
// background
fillRoundedRect(vg, 0, 0, (float)width, (float)height, 0.0f, {0.08f,0.08f,0.09f,1.0f});


// center panel
const float panelW = (float)width * 0.6f;
const float panelH = 120.0f;
const float panelX = ((float)width - panelW) * 0.5f;
const float panelY = (float)height * 0.15f;
fillRoundedRect(vg, panelX, panelY, panelW, panelH, 14.0f, {0.12f,0.12f,0.14f,1.0f});


// gain bar inside panel
const float margin = 16.0f;
gainBar_.draw(vg, panelX + margin, panelY + margin, panelW - 2*margin, panelH - 2*margin, gainValue);
}


} // namespace ava::ui