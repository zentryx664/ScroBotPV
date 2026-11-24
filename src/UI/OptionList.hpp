#pragma once
#include "Utils/Settings.hpp"
#include <Geode/Geode.hpp>
#include <functional>
#include <string>
#include <vector>
using namespace geode::prelude;

namespace scro {
// All options of one category as rows, grouped under section headings.
class OptionList : public CCNode {
    float m_w = 0, m_h = 0, m_availH = 0;
    int m_page = 0, m_pageCount = 1;
    std::string m_category, m_section;
    std::vector<Option*> m_opts;
    std::vector<CCLabelBMFont*> m_vals;
    std::function<void(Option*)> m_onSel;
    bool setup(std::string const& cat, float w, float availH, bool animate, std::function<void(Option*)> onSel, std::string const& section);
public:
    static OptionList* create(std::string const& cat, float w, float availH, bool animate, std::function<void(Option*)> onSel, std::string const& section = "");
    float height() const { return m_h; }
    void onToggle(CCObject*);
    void onStep(CCObject*);
    void onAction(CCObject*);
    void onHelp(CCObject*);
    void onPage(CCObject*);
    void rebuild() { this->removeAllChildrenWithCleanup(true); setup(m_category, m_w, m_availH, false, m_onSel, m_section); }
};
}
