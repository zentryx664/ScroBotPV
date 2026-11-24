#include "Hacks/FpsBoost.hpp"
#include "Utils/Settings.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include "Analysis/ClickAnalyzer.hpp"
#include "Core/Tick.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;
using Clock = std::chrono::steady_clock;

// ---- options (own category) --------------------------------------------------------------
static scro::Option& boost = scro::toggle("FPS Boost", "fpsboost", "FPS Boost",
    "Forces the game to run at 120 FPS or higher (never below 120). Turn VSync off in GD's video settings for best results.", false, 10);
static scro::Option& target = scro::step("FPS Boost", "fpstarget120", "Target FPS",
    "Frame rate to force. The minimum is 120.", 0.f, 0.f, 7.f, 1.f, "", 20);
static scro::Option& boostFrames = scro::toggle("FPS Boost", "boostframes", "Boost Frames",
    "Click between frames: your taps register at their exact moment between physics steps, which makes cube jumps and wave turns far easier. Also lightens effects. Forces Safe Mode on until turned off.", false, 25);
#ifdef GEODE_IS_MOBILE
static scro::Option& zeroDelay = scro::toggle("FPS Boost", "zdcbf", "Zero-Delay CBF",
    "Mobile only. Taps are timed inside the physics step with no extra buffering and no background work.", false, 28);
#endif
static scro::Option& precision = scro::toggle("FPS Boost", "clickprec", "Click Precision",
#if defined(GEODE_IS_WINDOWS)
    "Not available on Windows.", false, 30);
#elif defined(GEODE_IS_MOBILE)
    "Disabled on Android to preserve Geometry Dash's native touch input.", false, 30);
#else
    "Applies every click at the exact moment you pressed it, even on laggy frames, so timing stays perfect.", true, 30);
#endif
static scro::Option& diagnostics = scro::toggle("FPS Boost", "fpsdiag", "Frame-Time Diagnostics",
    "Show smoothed frame time and recent frame-time peak in the in-level HUD.", false, 40);

static float const kPresets[] = {120, 144, 165, 240, 360, 480, 720, 1000};
static bool _setup = ([] {
    target.names = {"120", "144", "165", "240", "360", "480", "720", "1000"};
    for (auto* o : {&boost, &target, &boostFrames}) o->section = "Frame Rate";
#ifdef GEODE_IS_MOBILE
    zeroDelay.section = "Input";
#endif
    precision.section = "Input";
#ifdef GEODE_IS_MOBILE
    // Older saved configs may have this enabled. The queue hook is intentionally disabled
    // on Android, so keep the UI honest and prevent enabling a no-op setting.
    precision.locked = [] { return true; };
    precision.b = false;
#endif
    diagnostics.section = "Diagnostics";
    boost.onChange = target.onChange = [] { scro::fps::apply(); };
    boostFrames.onChange = [] {
        if (scro::fps::boostFrames()) scro::find("safe").b = true;    // Safe Mode is forced on
    };
    // turning FPS Boost on while Boost Frames is already on also forces Safe Mode
    auto prev = boost.onChange;
    boost.onChange = [prev] { prev(); if (scro::fps::boostFrames()) scro::find("safe").b = true; };
}(), true);

// Called by the Geode Loaded callback before scro::load(). The option objects above
// are registered during static initialization; touching them here guarantees this
// translation unit is linked and keeps the registration order explicit.
void scro::fps::initOptions() {
    (void)boost;
    (void)target;
    (void)boostFrames;
#ifdef GEODE_IS_MOBILE
    (void)zeroDelay;
#endif
    (void)precision;
    (void)diagnostics;
}

bool scro::fps::boostFrames() { return boost.b && ::boostFrames.b; }
static bool cbfActive() {
#ifdef GEODE_IS_MOBILE
    if (zeroDelay.b) return true;
#endif
    return scro::fps::boostFrames();
}

// ---- FPS ---------------------------------------------------------------------------------
static bool g_saved = false, g_origVar = false;
static float g_origFps = 60.f;

void scro::fps::apply() {
    auto gm = GameManager::sharedState();
    if (!gm) return;
    if (!g_saved) { g_saved = true; g_origFps = gm->m_customFPSTarget; g_origVar = gm->getGameVariable("0116"); }
    int preset = std::clamp(static_cast<int>(std::lround(target.f)), 0, 7);
    float fps = boost.b ? std::max(120.f, kPresets[preset]) : g_origFps;   // hard floor: 120
    gm->setGameVariable("0116", boost.b ? true : g_origVar);
    gm->m_customFPSTarget = fps;
    if (auto app = CCApplication::sharedApplication()) {
        app->setAnimationInterval(1.0 / static_cast<double>(fps));
    }
}

class $modify(ScroFpsMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (boost.b) scro::fps::apply();       // restore the saved setting on startup
#ifdef GEODE_IS_MOBILE
        // Clear values persisted by older builds; native mobile touch events must pass through.
        precision.b = false;
#endif
        if (scro::fps::boostFrames()) scro::find("safe").b = true;
        return true;
    }
};

// ---- Boost Frames: GD's own click-between-steps, forced on ---------------------------------
static bool g_cbsSaved = false, g_origBetween = false, g_origOn = false;
static void applyCbs(PlayLayer* pl) {
    if (!pl) return;
    if (cbfActive()) {
        if (!g_cbsSaved) { g_cbsSaved = true; g_origBetween = pl->m_clickBetweenSteps; g_origOn = pl->m_clickOnSteps; }
        pl->m_clickBetweenSteps = true;     // inputs are timed inside the physics step...
        pl->m_clickOnSteps = false;         // ...not snapped to the step boundary
    } else if (g_cbsSaved) {
        g_cbsSaved = false;
        pl->m_clickBetweenSteps = g_origBetween;
        pl->m_clickOnSteps = g_origOn;
    }
}

class $modify(ScroFpsLevel, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        if (boost.b) scro::fps::apply();       // GD resets the frame rate on level load
        g_cbsSaved = false;                    // new level: read its own settings again
        applyCbs(this);
    }
};
static scro::tick::Reg regCbs(+[](PlayLayer* pl, float, bool) { applyCbs(pl); });

// Watchdog: if anything (GD, another mod, the video settings) lowers the rate, push it back up.
static scro::tick::Reg regFps(+[](PlayLayer*, float dt, bool) {
    static float t = 0.f;
    t += dt;
    if (t < 1.f) return;
    t = 0.f;
    if (!boost.b) return;
    auto gm = GameManager::sharedState();
    if (gm && gm->m_customFPSTarget < 119.5f) scro::fps::apply();
});

// ---- Click Precision ---------------------------------------------------------------------
// Presses are timestamped when they arrive and released to the game at the exact point inside
// the frame (see TickDriver), instead of all landing at the start of a laggy frame.
namespace {
struct Raw { int b; bool d, p; Clock::time_point t; };
std::vector<Raw> g_held;
Clock::time_point g_last = Clock::now();
}
// queueButton is an inline function on Windows (no address in the bindings), so it cannot be
// hooked there. Click Precision is therefore only active on platforms where the hook exists.
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MOBILE)
bool scro::fps::precise() { return false; }
#else
bool scro::fps::precise() { return boost.b && precision.b && !cbfActive(); }
#endif
bool scro::fps::pending() { return !g_held.empty(); }
void scro::fps::hold(int b, bool d, bool p) {
    if (g_held.size() >= 128) g_held.erase(g_held.begin(), g_held.begin() + 64);
    g_held.push_back({b, d, p, Clock::now()});
}

std::vector<scro::fps::Held> scro::fps::drain() {
    auto now = Clock::now();
    float span = std::chrono::duration<float>(now - g_last).count();
    std::vector<Held> out;
    out.reserve(g_held.size());
    for (auto& r : g_held) {
        float f = 0.f;                                          // stale (e.g. after a pause) -> start of frame
        if (span > 0.f && span < 0.25f) f = std::clamp(std::chrono::duration<float>(r.t - g_last).count() / span, 0.f, 1.f);
        out.push_back({r.b, r.d, r.p, f});
    }
    g_held.clear();
    g_last = now;
    return out;
}

#ifndef GEODE_IS_WINDOWS
class $modify(ScroFpsQueue, GJBaseGameLayer) {
    void queueButton(int button, bool down, bool isPlayer1, double timestamp) {
        if (!scro::fps::g_bypass && PlayLayer::get()) scro::clicks::arrive();
        if (!scro::fps::g_bypass && scro::fps::precise() && PlayLayer::get())
            return scro::fps::hold(button, down, isPlayer1);
        GJBaseGameLayer::queueButton(button, down, isPlayer1, timestamp);
    }
};
#endif
        
