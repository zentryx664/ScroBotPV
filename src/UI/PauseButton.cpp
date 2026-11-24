#include "UI/Menu.hpp"
#include "UI/Theme.hpp"
#include "Utils/Draw.hpp"
#include <Geode/modify/PauseLayer.hpp>
using namespace geode::prelude;

class $modify(ScroPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto n = scro::draw::button("Scro", scro::metrics::pauseBtnW, scro::metrics::pauseBtnH);
        auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ScroPauseLayer::onScro));
        b->m_scaleMultiplier = 1.f;
        auto menu = CCMenu::create();
        menu->addChild(b);
        auto win = CCDirector::get()->getWinSize();
        menu->setPosition(scro::metrics::pauseBtnW / 2.f + 16.f, win.height - scro::metrics::pauseBtnH - 8.f);
        this->addChild(menu);
    }
    void onScro(CCObject*) { scro::ui::open(); }
};
