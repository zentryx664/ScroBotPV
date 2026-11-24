#include "Hacks/FpsBoost.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>
#include "Analysis/ClickAnalyzer.hpp"
#include "Core/Tick.hpp"

using namespace geode::prelude;
using Clock = std::chrono::steady_clock;

namespace {
// Keep every option registration out of ELF global constructors. Android loads a mod's
// shared library before Geode's settings/mod services are guaranteed to be ready.
struct FpsOptions {
    scro::Option& boost;
    scro::Option& target;
    scro::Option& boostFrames;
#ifdef GEODE_IS_MOBILE
    scro::Option& zeroDelay;
#endif
    scro::Option& precision;
    scro::Option& diagnostics;

    FpsOptions()
      : boost(scro::toggle("FPS Boost", "fpsboost", "FPS Boost",
          "Unlocks the FPS target below. Actual FPS depends on your display, device, and game load.", false, 10)),
        target(scro::step("FPS Boost", "fpstarget120", "Target FPS",
          "Requested frame rate. Values above your display refresh rate may not produce additional visible frames.", 2.f, 0.f, 11.f, 1.f, "", 20)),
        boostFrames(scro::toggle("FPS Boost", "boostframes", "Boost Frames",
          "Uses Geometry Dash click-between-steps behavior. Forces Safe Mode on while enabled.", false, 25)),
#ifdef GEODE_IS_MOBILE
        zeroDelay(scro::toggle("FPS Boost", "zdcbf", "Zero-Delay CBF",
          "Mobile input timing option. May behave differently across devices.", false, 28)),
#endif
        precision(scro::toggle("FPS Boost", "clickprec", "Click Precision",
#ifdef GEODE_IS_WINDOWS
          "Not available on Windows.", false, 30)),
#else
          "Times queued clicks within the current update frame.", true, 30)),
#endif
        diagnostics(scro::toggle("FPS Boost", "fpsdiag", "Frame-Time Diagnostics",
          "Show smoothed frame time and recent frame-time peak in the in-level HUD.", false, 40)) {
        target.names = {"60", "75", "90", "100", "120", "144", "165", "240", "360", "480", "720", "1000"};
        for (auto* option : {&boost, &target, &boostFrames}) option->section = "Frame Rate";
#ifdef GEODE_IS_MOBILE
        zeroDelay.section = "Input";
#endif
        precision.section = "Input";
        diagnostics.section = "Diagnostics";

        boost.onChange = [] {
            scro::fps::apply();
            if (scro::fps::boostFrames()) scro::find("safe").b = true;
        };
        target.onChange = [] { scro::fps::apply(); };
        boostFrames.onChange = [] {
            if (scro::fps::boostFrames()) scro::find("safe").b = true;
        };
    }
};

FpsOptions& options() {
    static FpsOptions value;
    return value;
}

constexpr float kPresets[] = {60.f, 75.f, 90.f, 100.f, 120.f, 144.f,
                              165.f, 240.f, 360.f, 480.f, 720.f, 1000.f};

struct RawClick { int button; bool down; bool player1; Clock::time_point time; };
std::vector<RawClick>& heldQueue() {
    static std::vector<RawClick> value;
    return value;
}
Clock::time_point& lastClickTime() {
    static Clock::time_point value = Clock::now();
    return value;
}
}

// Called from the Geode Loaded callback before scro::load(), so all options are registered
// before saved values are read and before any settings UI can enumerate them.
void scro::fps::initOptions() {
    (void)options();
}

bool scro::fps::boostFrames() {
    auto& o = options();
    return o.boost.b && o.boostFrames.b;
}

static bool cbfActive() {
    auto& o = options();
#ifdef GEODE_IS_MOBILE
    if (o.zeroDelay.b) return true;
#endif
    return scro::fps::boostFrames();
}

// ---- FPS control -------------------------------------------------------------------------
static bool g_savedOriginal = false;
static bool g_originalUnlock = false;
static float g_originalFps = 60.f;

void scro::fps::apply() {
    auto& o = options();
    auto* gm = GameManager::sharedState();
    if (!gm) return;

    if (!g_savedOriginal) {
        g_originalFps = std::max(1.f, gm->m_customFPSTarget);
        g_originalUnlock = gm->getGameVariable("0116");
        g_savedOriginal = true;
    }

    const int preset = std::clamp(static_cast<int>(std::lround(o.target.f)), 0, 11);
    const float requestedFps = o.boost.b ? kPresets[preset] : g_originalFps;
    const bool unlock = o.boost.b ? true : g_originalUnlock;

    gm->setGameVariable("0116", unlock);
    gm->m_customFPSTarget = requestedFps;

    // Guard the Cocos application pointer. The mod can receive setting callbacks during
    // transitions where GameManager exists but the application singleton is unavailable.
    if (auto* app = CCApplication::sharedApplication()) {
        app->setAnimationInterval(1.0 / static_cast<double>(requestedFps));
    }
}

class $modify(ScroFpsMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto& o = options();
        if (o.boost.b) scro::fps::apply();
        if (scro::fps::boostFrames()) scro::find("safe").b = true;
        return true;
    }
};

// ---- Boost Frames -------------------------------------------------------------------------
static bool g_cbsSaved = false;
static bool g_originalBetweenSteps = false;
static bool g_originalOnSteps = false;

static void applyCbs(PlayLayer* layer) {
    if (!layer) return;
    if (cbfActive()) {
        if (!g_cbsSaved) {
            g_cbsSaved = true;
            g_originalBetweenSteps = layer->m_clickBetweenSteps;
            g_originalOnSteps = layer->m_clickOnSteps;
        }
        layer->m_clickBetweenSteps = true;
        layer->m_clickOnSteps = false;
    } else if (g_cbsSaved) {
        g_cbsSaved = false;
        layer->m_clickBetweenSteps = g_originalBetweenSteps;
        layer->m_clickOnSteps = g_originalOnSteps;
    }
}

class $modify(ScroFpsLevel, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        if (options().boost.b) scro::fps::apply();
        g_cbsSaved = false;
        applyCbs(this);
    }
};

static scro::tick::Reg regCbs(+[](PlayLayer* layer, float, bool) {
    applyCbs(layer);
});

static scro::tick::Reg regFps(+[](PlayLayer*, float dt, bool) {
    static float elapsed = 0.f;
    elapsed += std::max(0.f, dt);
    if (elapsed < 1.f) return;
    elapsed = 0.f;

    auto& o = options();
    if (!o.boost.b) return;
    auto* gm = GameManager::sharedState();
    if (gm && std::fabs(gm->m_customFPSTarget - kPresets[
            std::clamp(static_cast<int>(std::lround(o.target.f)), 0, 11)]) > 0.5f) {
        scro::fps::apply();
    }
});

// ---- Click Precision ----------------------------------------------------------------------
#ifdef GEODE_IS_WINDOWS
bool scro::fps::precise() { return false; }
#else
bool scro::fps::precise() {
    auto& o = options();
    return o.boost.b && o.precision.b && !cbfActive();
}
#endif

bool scro::fps::pending() { return !heldQueue().empty(); }

void scro::fps::hold(int button, bool down, bool player1) {
    // Bound queue growth in case the game pauses before the next update.
    auto& queue = heldQueue();
    if (queue.size() >= 128) queue.erase(queue.begin(), queue.begin() + 64);
    queue.push_back({button, down, player1, Clock::now()});
}

std::vector<scro::fps::Held> scro::fps::drain() {
    auto now = Clock::now();
    const auto previous = lastClickTime();
    const float span = std::chrono::duration<float>(now - previous).count();
    std::vector<Held> out;
    auto& queue = heldQueue();
    out.reserve(queue.size());
    for (auto const& click : queue) {
        float fraction = 0.f;
        if (span > 0.f && span < 0.25f) {
            fraction = std::clamp(
                std::chrono::duration<float>(click.time - previous).count() / span,
                0.f, 1.f
            );
        }
        out.push_back({click.button, click.down, click.player1, fraction});
    }
    queue.clear();
    lastClickTime() = now;
    return out;
}

#ifndef GEODE_IS_WINDOWS
class $modify(ScroFpsQueue, GJBaseGameLayer) {
    void queueButton(int button, bool down, bool isPlayer1, double timestamp) {
        if (!scro::fps::g_bypass && PlayLayer::get()) scro::clicks::arrive();
        if (!scro::fps::g_bypass && scro::fps::precise() && PlayLayer::get()) {
            scro::fps::hold(button, down, isPlayer1);
            return;
        }
        GJBaseGameLayer::queueButton(button, down, isPlayer1, timestamp);
    }
};
#endif
