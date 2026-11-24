## v1.5.0 - Released!
### Fixed
- **Buttons disappeared after switching tabs.** The page fade-in ran twice and left buttons, toggles and labels at zero opacity. It now runs once and can never stack.
- **Jump Hack could not jump.** The "on ground" flag was set too late, after the game had already read the tap. It is now set when the tap is handled, and kept while the button is held.
- **Button and text sizes.** Everything in the menu now uses one shared size table. Phones get bigger rows, checkboxes, arrows, tabs and Save/Reset buttons that are easy to tap. Desktop stays compact.
- Sidebar tab buttons no longer change size when you switch tabs.
- Quick Restart and Frame Step buttons, and the Scro pause button, are bigger on phones.
- Page arrows (< >) in long lists no longer rebuild the list while the tapped button is still handling the tap.
- No Particles no longer un-hides particles the game hides on purpose.

### Unlocked
- **Safe Mode** can be switched freely, including while Boost Frames is on. Cheat-like options still send completions as test runs, so nothing is ever submitted.
- **Click Precision** is available on Android (off by default, needs FPS Boost). On Windows it has no effect, so it is no longer shown there.
- Boost Frames no longer forces Safe Mode on in the menu.

### Improved
- **Low Detail Mode 2** rewritten. Only objects near the screen are drawn: objects you have passed are hidden, and upcoming objects switch on as they approach the screen edge. Dying, restarting or turning the option off brings every object back. It is cheap to run: it only touches objects that cross the edge each frame.

### Platforms
- Windows, Android 64-bit and Android 32-bit. Screen Record and Zero-Delay CBF are Android only.

## v1.4.0
- Built for Geode 5.11.2 and Geometry Dash 2.2081
- GD style menu with colour and font settings
- FPS Boost (120 FPS floor), Boost Frames, Zero-Delay CBF (mobile)
- Low Detail Mode 2, Click Analyzer, Screen Record (mobile)
- Removed the in-menu Settings tab and the Attempt label

## v1.1.0
- Split into modules: one file per hack under src/Hacks, shared code in Utils, UI, Labels, Macro.
- New dark black-blue menu with outlined, glossy, reflective panels.
- PC: press Tab for draggable, collapsible mod panels. Mobile: star button in the pause menu.
- New: Auto Practice, Clock and Frame labels.
