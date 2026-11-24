#include "UI/FindView.hpp"
#include "UI/MacroView.hpp"
#include "UI/Menu.hpp"
#include "UI/OptionList.hpp"
#include "UI/Theme.hpp"
#include "Notifications/Notify.hpp"
#include "Utils/Draw.hpp"
#include <algorithm>
using namespace scro;
using namespace scro::draw;

namespace {
#ifdef GEODE_IS_MOBILE
// Compact layout for phones.
static constexpr float W = 500.f, H = 300.f, SB = 124.f, HEAD = 40.f, FOOT = 50.f;
#else
// Desktop has more room.
static constexpr float W = 640.f, H = 380.f, SB = 140.f, HEAD = 40.f, FOOT = 50.f;
#endif
static constexpr float PW = W - SB - 28.f;
static constexpr char const* kGlobalTabs[] = {"General", "Speed", "Safety", "Replay", "Misc", "Find"};
static constexpr char const* kGlobalSections[] = {"General", "Speedhack", "Safety", "Recording", "Misc", "Find"};
static constexpr int kGlobalTabCount = 6;

class ScroMenu : public FLAlertLayer {
    CCNode* m_root = nullptr;
    CCNode* m_pane = nullptr;
    CCLabelBMFont* m_title = nullptr;
    CCLabelBMFont* m_desc = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_side;
    int m_tab = 0;
    int m_globalSub = 0;
    bool m_closing = false;
    float m_layoutScale = 1.f;

    CCMenu* newMenu(CCNode* parent) {
        auto m = CCMenu::create();
        m->setPosition(0, 0);
        m->setTouchPriority(-510);
        parent->addChild(m);
        return m;
    }

    void buildGlobalPane(bool animate) {
        float bodyH = H - HEAD - FOOT - 8.f;
        auto pane = CCNode::create();
        pane->setContentSize({PW, bodyH});

        auto tabs = newMenu(pane);
        float tabH = 22.f, gap = 3.f;
        float tabW = (PW - gap * (kGlobalTabCount - 1)) / kGlobalTabCount;
        float tabY = bodyH - tabH / 2;
        for (int i = 0; i < kGlobalTabCount; ++i) {
            auto it = CCMenuItemSpriteExtra::create(button(kGlobalTabs[i], tabW, tabH, m_globalSub == i, 0.4f),
                this, menu_selector(ScroMenu::onGlobalSub));
            it->setTag(i);
            it->setPosition(tabW / 2 + i * (tabW + gap), tabY);
            tabs->addChild(it);
        }

        float innerH = bodyH - tabH - 6.f;
        CCNode* content = nullptr;
        if (m_globalSub == 5) {
            content = FindView::create(PW, innerH, [this](Option* o) { onFindSelect(o); });
        } else {
            content = OptionList::create("Global", PW, innerH, animate,
                [this](Option* o) { setDesc(o); }, kGlobalSections[m_globalSub]);
        }
        if (content) {
            float contentY = m_globalSub == 5 ? 0.f : std::max(0.f, innerH - content->getContentSize().height);
            content->setPosition(0, contentY);
            pane->addChild(content);
        }
        pane->setPosition(SB + 14.f, H - HEAD - 6.f - bodyH);
        m_root->addChild(pane);
        m_pane = pane;
        if (animate) {
            pane->setPositionX(pane->getPositionX() + 10.f);
            fadeTree(pane, 0.f);
            pane->runAction(CCEaseOut::create(CCMoveBy::create(0.16f, {-10.f, 0.f}), 2.f));
        }
    }

    bool init() {
        if (!FLAlertLayer::init(120)) return false;
        m_noElasticity = true;
        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);
        this->setKeypadEnabled(true);
        this->setTouchEnabled(true);

        auto win = CCDirector::get()->getWinSize();
        float sx = (win.width - 16.f) / W;
        float sy = (win.height - 16.f) / H;
        m_layoutScale = std::clamp(std::min({1.f, sx, sy}), 0.55f, 1.f);
        m_root = CCNode::create();
        m_root->setContentSize({W, H});
        m_root->setAnchorPoint({0.5f, 0.5f});
        m_root->setPosition(win.width / 2, win.height / 2);
        m_mainLayer->addChild(m_root);

        m_root->addChild(panel(W, H, true));
        auto sideBox = inset(SB - 8.f, H - 16.f); sideBox->setPosition(8.f, 8.f); m_root->addChild(sideBox);
        auto bodyBox = inset(W - SB - 14.f, H - HEAD - FOOT - 2.f); bodyBox->setPosition(SB + 6.f, FOOT); m_root->addChild(bodyBox);

        m_root->addChild(label("ScroBot", 1.f, theme::TEXT, {0.5f, 0.5f}, {SB / 2 + 4.f, H - 24.f}));
        auto sm = newMenu(m_root);
        float sideTop = H - 58.f, sideBottom = 40.f;
        float sideStep = (sideTop - sideBottom) / std::max(1, kNumCategories - 1);
        for (int i = 0; i < kNumCategories; i++) {
            auto it = CCMenuItemSpriteExtra::create(button(kCategories[i], SB - 28.f, 24.f, i == 0, 0.5f),
                this, menu_selector(ScroMenu::onTab));
            it->setTag(i); it->setPosition(SB / 2 + 4.f, sideTop - i * sideStep);
            sm->addChild(it);
            m_side.push_back(it);
        }
        // GD close button on the top-left corner
        auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(ScroMenu::onClose));
        closeBtn->setPosition(4.f, H - 4.f);
        sm->addChild(closeBtn);

        m_title = label("", 1.2f, theme::TEXT, {0.f, 0.5f}, {SB + 16, H - 22});
        m_root->addChild(m_title);

        m_desc = label("", 0.8f, theme::MUTED, {0.f, 0.5f}, {SB + 14, FOOT / 2});
        m_root->addChild(m_desc);
        auto fm = newMenu(m_root);
        auto sv = CCMenuItemSpriteExtra::create(button("Save", 62, 26), this, menu_selector(ScroMenu::onSave));
        sv->setPosition(W - 14 - 31, FOOT / 2 + 1);
        auto rs = CCMenuItemSpriteExtra::create(button("Reset", 62, 26), this, menu_selector(ScroMenu::onReset));
        rs->setPosition(W - 14 - 62 - 6 - 31, FOOT / 2 + 1);
        fm->addChild(sv); fm->addChild(rs);

        showTab(0, false);
        m_root->setScale(m_layoutScale * 0.95f);
        m_root->setPositionY(m_root->getPositionY() - 8.f * m_layoutScale);
        m_root->runAction(CCSpawn::create(
            CCEaseExponentialOut::create(CCScaleTo::create(0.2f, m_layoutScale)),
            CCEaseExponentialOut::create(CCMoveBy::create(0.2f, {0.f, 8.f * m_layoutScale})), nullptr));
        return true;
    }

    void setDesc(Option* o) {
        std::string t = o ? (o->desc.empty() ? o->name : o->desc) : "Tap an option to see what it does.";
        m_desc->setString(t.c_str());
        // Reserve room for Reset/Save so help text never runs underneath the buttons.
        float helpW = std::max(60.f, W - SB - 170.f);
        fit(m_desc, helpW, 0.8f);
    }

    void showTab(int i, bool animate) {
        m_tab = i;
        for (int k = 0; k < kNumCategories; k++) {
            if (auto bs = typeinfo_cast<ButtonSprite*>(m_side[k]->getNormalImage()))
                bs->updateBGImage(k == i ? "GJ_button_01.png" : "GJ_button_04.png");
        }
        m_title->setString(kCategories[i]);
        fit(m_title, W - SB - 40.f, 1.2f);
        if (m_pane) m_pane->removeFromParent();
        std::string cat = kCategories[i];
        if (cat == "Global") {
            buildGlobalPane(animate);
            setDesc(nullptr);
            return;
        }

        float availH = H - HEAD - FOOT - 8.f;
        CCNode* pane = nullptr;
        float ph = availH;
#ifdef GEODE_IS_MOBILE
        constexpr bool isDesktop = false;
#else
        constexpr bool isDesktop = true;
#endif
        if (cat == "Replay") {
            auto mv = MacroView::create(PW, isDesktop, [this] { close(); });
            pane = mv; ph = mv->height();
        } else {
            auto ol = OptionList::create(cat, PW, availH, animate, [this](Option* o) { setDesc(o); });
            pane = ol; ph = ol->height();
        }
        pane->setPosition(SB + 14.f, H - HEAD - 6.f - ph);
        m_root->addChild(pane);
        m_pane = pane;
        setDesc(nullptr);
        if (animate) {
            pane->setPositionX(pane->getPositionX() + 10.f);
            fadeTree(pane, 0.f);
            pane->runAction(CCEaseOut::create(CCMoveBy::create(0.16f, {-10.f, 0.f}), 2.f));
        }
    }

    void onFindSelect(Option* o) {
        if (!o) return;
        int target = -1;
        for (int i = 0; i < kNumCategories; ++i) {
            if (o->cat == kCategories[i]) { target = i; break; }
        }
        if (target < 0) return;
        if (o->cat == "Global") {
            m_globalSub = 0;
            for (int i = 0; i < kGlobalTabCount - 1; ++i) {
                if (o->section == kGlobalSections[i]) { m_globalSub = i; break; }
            }
        }
        showTab(target, true);
        setDesc(o);
    }

    void onTab(CCObject* s) {
        int i = static_cast<CCNode*>(s)->getTag();
        if (i != m_tab && !m_closing) showTab(i, true);
    }
    void onGlobalSub(CCObject* s) {
        int i = static_cast<CCNode*>(s)->getTag();
        if (i >= 0 && i < kGlobalTabCount && i != m_globalSub && !m_closing) {
            m_globalSub = i;
            showTab(m_tab, true);
        }
    }
    void onReset(CCObject*) {
        if (std::string(kCategories[m_tab]) == "Replay") return;
        reset(kCategories[m_tab]);
        changed();
        showTab(m_tab, false);
        notify("Category reset");
    }
    void onSave(CCObject*) { save(); notify("Settings saved"); }
    void onClose(CCObject*) { close(); }
    void finish() { this->setKeypadEnabled(false); this->setTouchEnabled(false); this->removeFromParentAndCleanup(true); }
    void keyBackClicked() override { close(); }

public:
    void close() {
        if (m_closing) return;
        m_closing = true;
        m_root->runAction(CCSpawn::create(
            CCEaseIn::create(CCScaleTo::create(0.14f, m_layoutScale * 0.95f), 2.f),
            CCEaseIn::create(CCMoveBy::create(0.14f, {0.f, -8.f * m_layoutScale}), 2.f), nullptr));
        m_root->runAction(CCSequence::create(CCDelayTime::create(0.1f),
            CCCallFunc::create(this, callfunc_selector(ScroMenu::finish)), nullptr));
    }
    static ScroMenu* create() {
        auto r = new ScroMenu();
        if (r->init()) { r->autorelease(); return r; }
        delete r;
        return nullptr;
    }
};
}

void scro::ui::open() {
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene || scene->getChildByID("scrobot-menu")) return;
    if (auto m = ScroMenu::create()) { m->setID("scrobot-menu"); m->show(); }
}
void scro::ui::toggle() {
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene) return;
    if (auto m = scene->getChildByID("scrobot-menu")) static_cast<ScroMenu*>(m)->close();
    else open();
}
