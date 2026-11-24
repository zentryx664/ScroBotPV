#include "Draw.hpp"
#include "UI/Theme.hpp"

namespace scro::draw {
CCLayerColor* rect(float w, float h, ccColor4B c) {
    auto r = CCLayerColor::create(c, w, h);
    r->ignoreAnchorPointForPosition(false);
    r->setAnchorPoint({0.f, 0.f});
    return r;
}

CCLabelBMFont* label(std::string const& t, float scale, ccColor3B col, CCPoint anchor, CCPoint pos) {
    auto l = CCLabelBMFont::create(t.c_str(), theme::font());
    l->setScale(scale * theme::fontScale());
    l->setColor(col);
    l->setAnchorPoint(anchor);
    l->setPosition(pos);
    return l;
}
void fit(CCLabelBMFont* l, float maxWidth, float scale) {
    float fs = theme::fontScale();
    l->limitLabelWidth(maxWidth, scale * fs, 0.25f * fs);
}

template <class T> static bool fadeAs(CCNode* n, float delay) {
    if (auto p = typeinfo_cast<T*>(n)) {
        GLubyte o = p->getOpacity();
        p->setOpacity(0);
        n->runAction(CCSequence::create(CCDelayTime::create(delay), CCFadeTo::create(0.12f, o), nullptr));
        return true;
    }
    return false;
}
void fadeTree(CCNode* n, float delay) {
    if (fadeAs<CCLabelBMFont>(n, delay)) return;
    if (!fadeAs<CCLayerColor>(n, delay)) fadeAs<CCSprite>(n, delay);
    if (auto ch = n->getChildren())
        for (auto c : CCArrayExt<CCNode*>(ch)) fadeTree(c, delay);
}

CCNode* button(char const* text, float w, float h, bool selected, float textScale) {
    // green = selected, grey = normal (the two standard GD button textures)
    auto b = ButtonSprite::create(text, static_cast<int>(w), true, theme::font(),
        selected ? "GJ_button_01.png" : "GJ_button_04.png", h,
        textScale > 0.f ? textScale : (theme::fontScale() < 1.f ? 0.6f : 0.9f));
    return b;
}

CCScale9Sprite* panel(float w, float h, bool tinted) {
    auto s = CCScale9Sprite::create("GJ_square01.png");
    s->setContentSize({w, h});
    s->setAnchorPoint({0.f, 0.f});
    if (tinted) s->setColor(theme::panel());
    return s;
}
CCScale9Sprite* inset(float w, float h) {
    auto s = CCScale9Sprite::create("square02_001.png");
    s->setContentSize({w, h});
    s->setAnchorPoint({0.f, 0.f});
    s->setColor({0, 0, 0});
    s->setOpacity(95);
    return s;
}
}
