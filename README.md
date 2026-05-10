# xscreen

Declarative UI system based on XML + Lua for the game **South American Farming**.

Inspired by the Farming Simulator GUI architecture (FS19/22/25), redesigned as a modern, modular, extensible and cross-platform engine.

## Features

- **XML GUI Parser** — Declarative screen definitions with profiles, callbacks, and layout directives
- **Scene Tree** — `UIElement` base class with parent/children hierarchy, transform, visibility, update/render cycle
- **Layout Engine** — VBox, HBox, alignment, spacing, padding, percent and absolute sizing
- **Profile/Style System** — Reusable style profiles with inheritance (`extends`)
- **Lua Callback Bridge** — XML callbacks call Lua functions; no inline logic in XML
- **Screen Manager** — Stack-based navigation: `UI:show()`, `UI:close()`, `UI:push()`, `UI:pop()`
- **Animation System** — Framework for fade, slide, scale transitions with easing
- **Widgets** — Button, Label, Image, Container, VBox, HBox, ScrollView, ProgressBar, TextField
- **Data Binding (prepared)** — `{{variable}}` syntax ready for future implementation
- **Cross-platform** — Windows and Android builds via CMake and Gradle

## Project Structure

```
src/
├── core/           # UIElement, XMLParser, ProfileManager, ScreenManager, WidgetFactory
├── layout/         # LayoutEngine, VBoxLayout, HBoxLayout
├── widgets/        # Button, Label, Image, Container, ScrollView, ProgressBar, TextField
├── scripting/      # LuaBridge (Lua 5.4 integration)
├── animation/      # Animation, AnimationManager
├── renderer/       # Abstract Renderer, RaylibRenderer
└── main.cpp        # Application entry point

ui/
├── screens/        # XML screen definitions
├── styles/         # Global style profiles
├── scripts/        # Lua screen logic
├── animations/     # Animation definitions (future)
├── textures/       # UI textures (future)
└── fonts/          # Custom fonts (future)

samples/            # Reference FS19/22/25 GUI XML files
android/            # Android build configuration
```

## XML Format

```xml
<?xml version="1.0" encoding="utf-8"?>
<Gui>
    <Profiles>
        <Profile name="mainButton">
            <Property name="width" value="0.3"/>
            <Property name="height" value="0.08"/>
            <Property name="fontSize" value="28"/>
        </Profile>
    </Profiles>

    <Screen name="MainMenu" script="ui/scripts/mainMenu.lua">
        <VBox width="0.5" height="0.5" align="center" spacing="0.02">
            <Label text="South American Farming"/>
            <Button id="careerButton" profile="mainButton"
                    text="Career" onClick="onCareerClick"/>
            <Button profile="mainButton"
                    text="Settings" onClick="onSettingsClick"/>
        </VBox>
    </Screen>
</Gui>
```

## Lua Integration

```lua
MainMenu = {}

function MainMenu:onCareerClick()
    logInfo("Career clicked!")
end

function MainMenu:onSettingsClick()
    UI:show("SettingsScreen")
end
```

## Building

### Windows (Desktop)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Android

```bash
cd android
./gradlew assembleDebug
```

Requires Android SDK, NDK 26+, and JDK 17.

## Dependencies

All fetched automatically via CMake FetchContent:

- [raylib 5.5](https://github.com/raysan5/raylib) — Cross-platform rendering
- [tinyxml2 10.0](https://github.com/leethomason/tinyxml2) — XML parsing
- [Lua 5.4.7](https://github.com/lua/lua) — Scripting engine

## Architecture Notes

- **Renderer is decoupled** — `Renderer` is an abstract interface; `RaylibRenderer` is the current implementation
- **No inline logic in XML** — All callbacks reference Lua function names
- **Profile inheritance** — Profiles can extend other profiles via `extends` attribute
- **Modding-ready** — XML + Lua architecture allows external screen definitions
- **Data binding prepared** — `{{variable}}` syntax in text attributes for future binding system

## License

See [LICENSE](LICENSE).
