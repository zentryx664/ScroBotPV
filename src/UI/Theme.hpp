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
