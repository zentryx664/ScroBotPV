#pragma once
#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace scro::draw {
CCLayerColor* rect(float w, float h, ccColor4B c);
CCLabelBMFont* label(std::string const& t, float scale, ccColor3B col, CCPoint anchor, CCPoint pos);
void fit(CCLabelBMFont* l, float maxWidth, float scale);                   // shrink a label to a width
void fadeTree(CCNode* n, float delay);
CCNode* button(char const* text, float w, float h, bool selected = false, float textScale = 0.f);  // GD style button
CCScale9Sprite* panel(float w, float h, bool tinted);                      // GD popup background
CCScale9Sprite* inset(float w, float h);                                   // dark inner box
}
