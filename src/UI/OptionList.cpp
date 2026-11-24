#include "UI/OptionList.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include <algorithm>
#include <utility>
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
    auto l = label(text, metrics::nameScale, theme::TEXT, {0.5f, 0.5f}, {0, 0});
    fit(l, maxW, metrics::nameScale);
    auto sz = l->getScaledContentSize();
    auto n = CCNode::create();
    n->setContentSize({sz.width, std::max(sz.height, metrics::rowH - 2.f)});
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

bool OptionList::setup(std::string const& cat, float w, float availH, bool animate,
                       std::function<void(Option*)> onSel, std::string const& section) {
    m_w = w;
    m_availH = availH;
    m_onSel = std::move(onSel);
    m_category = cat;
    m_section = section;

    m_opts = inCategory(cat);
    if (!section.empty()) {
        std::erase_if(m_opts, [&](Option* option) { return option->section != section; });
    }

    // Paginate by the real height of each row, including section headers. This avoids
    // placing options above the visible panel on mobile and desktop alike.
    constexpr float rowH = metrics::rowH;
    constexpr float sectionH = metrics::sectionH;
    constexpr float navH = metrics::navH;
    constexpr float topBottomPadding = 8.f;
    const float contentH = std::max(rowH + sectionH, availH - navH - topBottomPadding);

    std::vector<std::pair<size_t, size_t>> pages;
    if (m_opts.empty()) {
        pages.emplace_back(0, 0);
    } else {
        size_t pageStart = 0;
        while (pageStart < m_opts.size()) {
            size_t pageEnd = pageStart;
            float usedH = 0.f;
            std::string previousSection;

            while (pageEnd < m_opts.size()) {
                auto const* option = m_opts[pageEnd];
                const bool needsHeading = section.empty() && option->section != previousSection;
                const float itemH = rowH + (needsHeading ? sectionH : 0.f);

                if (pageEnd > pageStart && usedH + itemH > contentH) break;
                usedH += itemH;
                previousSection = option->section;
                ++pageEnd;
            }

            // Always make forward progress, even if the panel is unusually short.
            if (pageEnd == pageStart) ++pageEnd;
            pages.emplace_back(pageStart, pageEnd);
            pageStart = pageEnd;
        }
    }

    m_pageCount = static_cast<int>(pages.size());
    m_page = std::clamp(m_page, 0, m_pageCount - 1);
    const size_t first = pages[static_cast<size_t>(m_page)].first;
    const size_t last = pages[static_cast<size_t>(m_page)].second;

    m_h = availH;
    this->setContentSize({w, m_h});
    m_vals.assign(m_opts.size(), nullptr);

    if (m_opts.empty()) {
        this->addChild(label("No options in this section", 0.72f, theme::MUTED,
            {0.5f, 0.5f}, {w / 2.f, availH / 2.f}));
        return true;
    }

    auto menu = CCMenu::create();
    menu->setPosition(0, 0);
    menu->setTouchPriority(-510);
    this->addChild(menu);

    const auto accent = theme::accent();
    float y = availH - (m_pageCount > 1 ? navH + 4.f : 4.f);
    constexpr float buttonSize = metrics::arrowSize;
    constexpr float valueW = metrics::valueW;
    constexpr float gap = 3.f;
    constexpr float actionW = metrics::actionW;
    constexpr float toggleSize = 40.f * metrics::toggleScale;   // checkbox sprite is about 40 px at scale 1
    std::string lastSection;

    for (size_t i = first; i < last; ++i) {
        auto* option = m_opts[i];
        const int index = static_cast<int>(i);

        if (section.empty() && option->section != lastSection) {
            lastSection = option->section;
            y -= sectionH;
            this->addChild(label(option->section, 0.68f, accent,
                {0.f, 0.5f}, {2.f, y + sectionH / 2.f}));
            auto line = rect(w, 1.f, ccc4(0, 0, 0, 90));
            line->setPosition(0.f, y + 1.f);
            this->addChild(line);
        }

        y -= rowH;
        const float centerY = y + rowH / 2.f;

        if (option->kind == Kind::Toggle) {
            auto toggle = CCMenuItemToggler::createWithStandardSprites(
                this, menu_selector(OptionList::onToggle), metrics::toggleScale);
            toggle->toggle(option->b);
            toggle->setTag(index);
            toggle->setPosition(toggleSize / 2.f + 2.f, centerY);
            const bool locked = option->locked && option->locked();
            if (locked) toggle->setEnabled(false);
            menu->addChild(toggle);

            float nameW = 0.f;
            auto name = nameNode(option->name, w - toggleSize - 14.f - (locked ? 48.f : 0.f), nameW);
            auto help = CCMenuItemSpriteExtra::create(name, this, menu_selector(OptionList::onHelp));
            help->m_scaleMultiplier = 1.f;
            help->setTag(index);
            help->setPosition(toggleSize + 8.f + nameW / 2.f, centerY);
            menu->addChild(help);

            if (locked) {
                this->addChild(label("LOCKED", 0.58f, ccColor3B{255, 120, 120},
                    {1.f, 0.5f}, {w - 4.f, centerY}));
            }
        } else if (option->kind == Kind::Step) {
            float nameW = 0.f;
            auto name = nameNode(option->name,
                w - (buttonSize * 2.f + valueW + gap * 2.f) - 12.f, nameW);
            auto help = CCMenuItemSpriteExtra::create(name, this, menu_selector(OptionList::onHelp));
            help->m_scaleMultiplier = 1.f;
            help->setTag(index);
            help->setPosition(4.f + nameW / 2.f, centerY);
            menu->addChild(help);

            auto minus = CCMenuItemSpriteExtra::create(arrow(false, buttonSize), this,
                menu_selector(OptionList::onStep));
            minus->setTag(index * 2);
            minus->setPosition(w - buttonSize * 1.5f - gap - valueW - gap, centerY);
            auto plus = CCMenuItemSpriteExtra::create(arrow(true, buttonSize), this,
                menu_selector(OptionList::onStep));
            plus->setTag(index * 2 + 1);
            plus->setPosition(w - buttonSize / 2.f, centerY);
            menu->addChild(minus);
            menu->addChild(plus);

            auto value = label(option->text(), metrics::valueScale, accent, {0.5f, 0.5f},
                {w - buttonSize - gap - valueW / 2.f, centerY});
            fit(value, valueW - 4.f, metrics::valueScale);
            this->addChild(value);
            m_vals[i] = value;
        } else {
            float nameW = 0.f;
            auto name = nameNode(option->name, w - actionW - 12.f, nameW);
            auto help = CCMenuItemSpriteExtra::create(name, this, menu_selector(OptionList::onHelp));
            help->m_scaleMultiplier = 1.f;
            help->setTag(index);
            help->setPosition(4.f + nameW / 2.f, centerY);
            menu->addChild(help);

            auto open = CCMenuItemSpriteExtra::create(button("Open", actionW, metrics::actionH, false, 0.65f),
                this, menu_selector(OptionList::onAction));
            open->m_scaleMultiplier = 1.f;
            open->setTag(index);
            open->setPosition(w - actionW / 2.f, centerY);
            menu->addChild(open);
        }
    }

    if (m_pageCount > 1) {
        auto previous = CCMenuItemSpriteExtra::create(
            button("<", metrics::pageBtnW, metrics::pageBtnH, false, 0.65f), this, menu_selector(OptionList::onPage));
        previous->m_scaleMultiplier = 1.f;
        previous->setTag(-1);
        previous->setPosition(w - metrics::pageBtnW * 1.5f - 6.f, metrics::navH / 2.f);
        previous->setEnabled(m_page > 0);
        menu->addChild(previous);

        auto next = CCMenuItemSpriteExtra::create(
            button(">", metrics::pageBtnW, metrics::pageBtnH, false, 0.65f), this, menu_selector(OptionList::onPage));
        next->m_scaleMultiplier = 1.f;
        next->setTag(1);
        next->setPosition(w - metrics::pageBtnW / 2.f - 2.f, metrics::navH / 2.f);
        next->setEnabled(m_page + 1 < m_pageCount);
        menu->addChild(next);

        const std::string pageText = std::to_string(m_page + 1) + "/" + std::to_string(m_pageCount);
        this->addChild(label(pageText, 0.6f, theme::MUTED,
            {1.f, 0.5f}, {w - metrics::pageBtnW * 2.f - 12.f, metrics::navH / 2.f}));
    }

    (void)animate;   // the menu fades the whole pane in once; fading here too left buttons invisible
    return true;
}

void OptionList::onToggle(CCObject* sender) {
    const int index = static_cast<CCNode*>(sender)->getTag();
    if (index < 0 || static_cast<size_t>(index) >= m_opts.size()) return;
    auto* option = m_opts[static_cast<size_t>(index)];
    if (option->locked && option->locked()) return;
    option->b = !option->b;
    changed();
    if (option->onChange) option->onChange();
    if (m_onSel) m_onSel(option);
}

void OptionList::onStep(CCObject* sender) {
    const int tag = static_cast<CCNode*>(sender)->getTag();
    const int index = tag / 2;
    if (index < 0 || static_cast<size_t>(index) >= m_opts.size()) return;
    auto* option = m_opts[static_cast<size_t>(index)];
    const float value = std::clamp(option->f + ((tag & 1) ? option->st : -option->st),
        option->mn, option->mx);
    option->f = std::round(value / option->st) * option->st;
    if (auto* valueLabel = m_vals[static_cast<size_t>(index)]) {
        valueLabel->setString(option->text().c_str());
        fit(valueLabel, metrics::valueW - 4.f, metrics::valueScale);
    }
    changed();
    if (option->onChange) option->onChange();
    if (m_onSel) m_onSel(option);
}

void OptionList::onAction(CCObject* sender) {
    const int index = static_cast<CCNode*>(sender)->getTag();
    if (index < 0 || static_cast<size_t>(index) >= m_opts.size()) return;
    auto* option = m_opts[static_cast<size_t>(index)];
    if (option->action) option->action();
    if (m_onSel) m_onSel(option);
}

void OptionList::onHelp(CCObject* sender) {
    const int index = static_cast<CCNode*>(sender)->getTag();
    if (index < 0 || static_cast<size_t>(index) >= m_opts.size()) return;
    if (m_onSel) m_onSel(m_opts[static_cast<size_t>(index)]);
}

void OptionList::onPage(CCObject* sender) {
    if (m_pageCount <= 1) return;
    const int delta = static_cast<CCNode*>(sender)->getTag();
    const int nextPage = std::clamp(m_page + delta, 0, m_pageCount - 1);
    if (nextPage == m_page) return;
    m_page = nextPage;
    // rebuild next frame so the button that was just tapped is not destroyed while it is still handling the tap
    this->runAction(CCCallFunc::create(this, callfunc_selector(OptionList::rebuild)));
}
