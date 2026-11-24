#pragma once
#include <Geode/Geode.hpp>
#include <string>
using namespace geode::prelude;

// Colors and font come from the mod settings (Geode mod page > ScroBot > Settings).
namespace scro::theme {
inline ccColor3B panel() { return Mod::get()->getSettingValue<ccColor3B>("ui-color"); }
inline ccColor3B accent() { return Mod::get()->getSettingValue<ccColor3B>("ui-accent"); }
inline ccColor3B const TEXT = {255, 255, 255};
inline ccColor3B const MUTED = {205, 205, 205};

// "Pusab" / "Gold" / "Chat" -> GD font file
inline char const* font() {
    auto f = Mod::get()->getSettingValue<std::string>("ui-font");
    if (f == "Gold") return "goldFont.fnt";
    if (f == "Chat") return "chatFont.fnt";
    return "bigFont.fnt";
}
// chatFont is small, the other two are big: scale text so every font fills the same space
inline float fontScale() { return std::string_view(font()) == "chatFont.fnt" ? 1.f : 0.5f; }
}

// Sizes shared by every menu page. Phones get bigger rows and buttons so they are easy to tap;
// desktop keeps the compact layout.
namespace scro::metrics {
#ifdef GEODE_IS_MOBILE
inline constexpr float rowH = 26.f, sectionH = 14.f, navH = 24.f;
inline constexpr float toggleScale = 0.68f, nameScale = 1.05f, valueScale = 0.9f;
inline constexpr float arrowSize = 24.f, valueW = 58.f, actionW = 60.f, actionH = 24.f;
inline constexpr float pageBtnW = 30.f, pageBtnH = 20.f;
inline constexpr float tabH = 24.f, tabText = 0.44f, sideH = 24.f, sideText = 0.5f;
inline constexpr float footW = 74.f, footH = 30.f;
inline constexpr float hudBtn = 34.f, pauseBtnW = 62.f, pauseBtnH = 28.f;
#else
inline constexpr float rowH = 21.f, sectionH = 13.f, navH = 20.f;
inline constexpr float toggleScale = 0.55f, nameScale = 0.95f, valueScale = 0.8f;
inline constexpr float arrowSize = 18.f, valueW = 50.f, actionW = 52.f, actionH = 19.f;
inline constexpr float pageBtnW = 22.f, pageBtnH = 16.f;
inline constexpr float tabH = 20.f, tabText = 0.34f, sideH = 20.f, sideText = 0.4f;
inline constexpr float footW = 64.f, footH = 26.f;
inline constexpr float hudBtn = 28.f, pauseBtnW = 54.f, pauseBtnH = 24.f;
#endif
}
