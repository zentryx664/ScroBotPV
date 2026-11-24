#include "UI/OptionList.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include <algorithm>
using namespace scro;
using namespace scro::draw;

namespace {
bool hasFrame(char const* name) { return CCSpriteFrameCache::get()->spriteFrameByName(name) != nullptr; }

// Arrow button: the GD editor arrow when it exists, a plain button otherwise.
CCNode* arrow(bool right, float size) {
    char const* frame = right ? "edit_rightBtn_001.png" : "edit_leftBtn_001.png";
    if (hasFrame(frame)) {
        auto s = CCSprite::createWithSpriteFrameName(frame);
        s->setScale(size / s->getContentSize().height);
        auto n = CCNode::create();
        n->setContentSize(s->getScaledContentSize());
        s->setPosition(n->getContentSize() / 2);
        n->addChild(s);
        return n;
    }
    return button(right ? ">" : "<", size, size);
}

// A label wrapped in a node the size of the text, so tapping the name shows its description.
CCNode* nameNode(std::string const& text, float maxW, float& outW) {
    auto l = label(text, 0.9f, theme::TEXT, {0.5f, 0.5f}, {0, 0});
    fit(l, maxW, 0.9f);
    auto sz = l->getScaledContentSize();
    auto n = CCNode::create();
    n->setContentSize({sz.width, std::max(sz.height, 18.f)});
    l->setPosition(n->getContentSize() / 2);
    n->addChild(l);
    outW = sz.width;
    return n;
}
}

OptionList* OptionList::create(std::string const& cat, float w, float availH, bool animate, std::function<void(Option*)> onSel, std::string const& section) {
    auto r = new OptionList();
    if (r->init() && r->setup(cat, w, availH, animate, std::move(onSel), section)) { r->autorelease(); return r; }
    delete r;
    return nullptr;
}

bool OptionList::setup(std::string const& cat, float w, float availH, bool animate, std::function<void(Option*)> onSel, std::string const& section) {
    m_w = w; m_onSel = std::move(onSel);
    m_opts = inCategory(cat);
    if (!section.empty()) {
        std::erase_if(m_opts, [&](Option* o) { return o->section != section; });
    }
    int sections = 0;
    if (section.empty()) {
        std::string last;
        for (auto o : m_opts) if (o->section != last) { sections++; last = o->section; }
    }
    float secH = 18.f;
    float r = std::clamp((availH - sections * secH) / std::max<size_t>(m_opts.size(), 1), 22.f, 30.f);
    m_h = m_opts.size() * r + sections * secH;
    this->setContentSize({w, m_h});
    m_vals.assign(m_opts.size(), nullptr);

    auto menu = CCMenu::create();
    menu->setPosition(0, 0);
    menu->setTouchPriority(-510);
    this->addChild(menu);

    auto ac = theme::accent();
    float y = m_h, bs = 20.f, vw = 60.f, gap = 4.f, btnW = 56.f;
    std::string last;
    for (size_t i = 0; i < m_opts.size(); i++) {
        auto o = m_opts[i];
        int idx = static_cast<int>(i);
        if (section.empty() && o->section != last) {               // section heading
            last = o->section;
            y -= secH;
            this->addChild(label(o->section, 0.8f, ac, {0.f, 0.5f}, {2.f, y + secH / 2}));
            auto ln = rect(w, 1, ccc4(0, 0, 0, 90)); ln->setPosition(0, y + 1); this->addChild(ln);
        }
        y -= r;
        float cy = y + r / 2;

        if (o->kind == Kind::Toggle) {                              // GD checkbox + name
            auto t = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(OptionList::onToggle), 0.6f);
            t->toggle(o->b);
            t->setTag(idx);
            t->setPosition(16.f, cy);
            bool lk = o->locked && o->locked();
            if (lk) t->setEnabled(false);
            menu->addChild(t);
            float nw = 0;
            auto name = nameNode(o->name, w - 40.f - (lk ? 54.f : 0.f), nw);
            auto help = CCMenuItemSpriteExtra::create(name, this, menu_selector(OptionList::onHelp));
            help->m_scaleMultiplier = 1.f; help->setTag(idx);
            help->setPosition(34.f + nw / 2, cy);
            menu->addChild(help);
            if (lk) this->addChild(label("LOCKED", 0.7f, ccColor3B{255, 120, 120}, {1.f, 0.5f}, {w - 4.f, cy}));
        } else if (o->kind == Kind::Step) {
            float nw = 0;
            auto name = nameNode(o->name, w - (bs * 2 + vw + gap * 2) - 12.f, nw);
            auto help = CCMenuItemSpriteExtra::create(name, this, menu_selector(OptionList::onHelp));
            help->m_scaleMultiplier = 1.f; help->setTag(idx);
            help->setPosition(4.f + nw / 2, cy);
            menu->addChild(help);

            auto mi = CCMenuItemSpriteExtra::create(arrow(false, bs), this, menu_selector(OptionList::onStep));
            mi->setTag(idx * 2); mi->setPosition(w - bs * 1.5f - gap - vw - gap, cy);
            auto pi = CCMenuItemSpriteExtra::create(arrow(true, bs), this, menu_selector(OptionList::onStep));
            pi->setTag(idx * 2 + 1); pi->setPosition(w - bs / 2, cy);
            menu->addChild(mi); menu->addChild(pi);
            auto v = label(o->text(), 0.9f, ac, {0.5f, 0.5f}, {w - bs - gap - vw / 2, cy});
            fit(v, vw - 4.f, 0.9f);
            this->addChild(v);
            m_vals[i] = v;
        } else {                                                    // action: name + Open button
            float nw = 0;
            auto name = nameNode(o->name, w - btnW - 12.f, nw);
            auto help = CCMenuItemSpriteExtra::create(name, this, menu_selector(OptionList::onHelp));
            help->m_scaleMultiplier = 1.f; help->setTag(idx);
            help->setPosition(4.f + nw / 2, cy);
            menu->addChild(help);
            auto it = CCMenuItemSpriteExtra::create(button("Open", btnW, 22.f), this, menu_selector(OptionList::onAction));
            it->setTag(idx); it->setPosition(w - btnW / 2, cy);
            menu->addChild(it);
        }
    }
    if (animate) fadeTree(this, 0.f);
    return true;
}

void OptionList::onToggle(CCObject* s) {
    auto o = m_opts[static_cast<CCNode*>(s)->getTag()];
    if (o->locked && o->locked()) return;
    o->b = !o->b;                      // our value is the source of truth; the checkbox flips itself once per tap
    changed();
    if (o->onChange) o->onChange();
    if (m_onSel) m_onSel(o);
}
void OptionList::onStep(CCObject* s) {
    int tag = static_cast<CCNode*>(s)->getTag(), i = tag / 2;
    auto o = m_opts[i];
    float v = std::clamp(o->f + ((tag & 1) ? o->st : -o->st), o->mn, o->mx);
    o->f = std::round(v / o->st) * o->st;
    if (auto l = m_vals[i]) { l->setString(o->text().c_str()); fit(l, 56.f, 0.9f); }
    changed();
    if (o->onChange) o->onChange();
    if (m_onSel) m_onSel(o);
}
void OptionList::onAction(CCObject* s) {
    auto o = m_opts[static_cast<CCNode*>(s)->getTag()];
    if (o->action) o->action();
    if (m_onSel) m_onSel(o);
}
void OptionList::onHelp(CCObject* s) {
    if (m_onSel) m_onSel(m_opts[static_cast<CCNode*>(s)->getTag()]);
}
