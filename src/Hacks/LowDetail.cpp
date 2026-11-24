#include "Core/Tick.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <vector>
using namespace geode::prelude;

// Low Detail Mode 2: objects the player has already passed are hidden, but only the ones that are
// currently loaded. No in-game setting is changed, and everything comes back on restart or when off.
static scro::Option& opt = scro::toggle("Visuals", "lowdetail2", "Low Detail Mode 2",
    "Hides loaded objects you have already passed, so the game has less to draw. Objects return on restart.", false, 20);

namespace {
std::vector<GameObject*> g_hidden;     // only objects this feature hid
PlayLayer* g_layer = nullptr;

void restore() {
    for (auto o : g_hidden) o->setVisible(true);
    g_hidden.clear();
}
}

static void lowDetailTick(PlayLayer* pl, float, bool) {
    if (pl != g_layer) { g_hidden.clear(); g_layer = pl; }      // new level: old pointers are gone
    if (!opt.b) { if (!g_hidden.empty()) restore(); return; }
    auto layer = pl->m_objectLayer;
    if (!layer) return;
    float scale = layer->getScaleX() > 0.01f ? layer->getScaleX() : 1.f;
    float left = -layer->getPositionX() / scale;                // left edge of the screen in level units
    for (auto o : pl->m_activeObjects) {
        if (!o || !o->isVisible()) continue;
        if (o->getPositionX() + 30.f < left - 60.f) {           // fully behind the screen
            o->setVisible(false);
            g_hidden.push_back(o);
        }
    }
}
static scro::tick::Reg regLowDetail(lowDetailTick);

class $modify(ScroLowDetail, PlayLayer) {
    void resetLevel() {
        restore();                                              // objects ahead of a checkpoint are needed again
        PlayLayer::resetLevel();
    }
    void onQuit() {
        g_hidden.clear();
        g_layer = nullptr;
        PlayLayer::onQuit();
    }
};
