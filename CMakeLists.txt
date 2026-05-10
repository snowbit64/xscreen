cmake_minimum_required(VERSION 3.16)
project(xscreen VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(XSCREEN_BUILD_ANDROID "Build for Android" OFF)

# ─────────────────────────────────────────────────────────────
# Dependencies via FetchContent
# ─────────────────────────────────────────────────────────────
include(FetchContent)

# tinyxml2
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        10.0.0
    GIT_SHALLOW    TRUE
)
set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tinyxml2)

# Lua 5.4
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.7
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(lua)
if(NOT lua_POPULATED)
    FetchContent_Populate(lua)
endif()

# Build Lua as a static library
file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
list(REMOVE_ITEM LUA_SOURCES "${lua_SOURCE_DIR}/lua.c")
list(REMOVE_ITEM LUA_SOURCES "${lua_SOURCE_DIR}/luac.c")
list(REMOVE_ITEM LUA_SOURCES "${lua_SOURCE_DIR}/onelua.c")

add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC "${lua_SOURCE_DIR}")

if(NOT WIN32)
    target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)
    target_link_libraries(lua_static PRIVATE m)
endif()

# ─────────────────────────────────────────────────────────────
# Shared core sources (renderer-independent)
# ─────────────────────────────────────────────────────────────
set(XSCREEN_CORE_SOURCES
    src/core/UIElement.cpp
    src/core/ProfileManager.cpp
    src/core/XMLParser.cpp
    src/core/WidgetFactory.cpp
    src/core/ScreenManager.cpp
    src/layout/LayoutEngine.cpp
    src/layout/VBoxLayout.cpp
    src/layout/HBoxLayout.cpp
    src/widgets/Button.cpp
    src/widgets/Label.cpp
    src/widgets/ImageWidget.cpp
    src/widgets/Container.cpp
    src/widgets/ScrollView.cpp
    src/widgets/ProgressBar.cpp
    src/widgets/TextField.cpp
    src/scripting/LuaBridge.cpp
    src/animation/Animation.cpp
    src/animation/AnimationManager.cpp
)

if(XSCREEN_BUILD_ANDROID)
    # ─────────────────────────────────────────────────────────
    # Android: OpenGL ES renderer + NativeActivity
    # ─────────────────────────────────────────────────────────
    set(XSCREEN_ANDROID_SOURCES
        ${XSCREEN_CORE_SOURCES}
        src/renderer/GLESRenderer.cpp
        src/android_main.cpp
    )

    add_library(xscreen SHARED ${XSCREEN_ANDROID_SOURCES})
    target_include_directories(xscreen PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
        ${lua_SOURCE_DIR}
    )
    target_link_libraries(xscreen PUBLIC
        tinyxml2
        lua_static
        android
        log
        EGL
        GLESv3
    )
else()
    # ─────────────────────────────────────────────────────────
    # Desktop: GLFW + OpenGL 3.3 renderer (same pipeline as Android)
    # ─────────────────────────────────────────────────────────
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(glfw)

    set(XSCREEN_DESKTOP_SOURCES
        ${XSCREEN_CORE_SOURCES}
        src/renderer/GLESRenderer.cpp
        third_party/glad/src/gl.c
    )

    add_library(xscreen_lib STATIC ${XSCREEN_DESKTOP_SOURCES})
    target_include_directories(xscreen_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include
        ${lua_SOURCE_DIR}
    )
    target_link_libraries(xscreen_lib PUBLIC
        glfw
        tinyxml2
        lua_static
    )
    if(WIN32)
        target_link_libraries(xscreen_lib PUBLIC opengl32)
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(xscreen_lib PUBLIC OpenGL::GL)
    endif()

    add_executable(xscreen src/main.cpp)
    target_link_libraries(xscreen PRIVATE xscreen_lib)

    # Copy ui/ assets next to executable
    add_custom_command(TARGET xscreen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/ui
            $<TARGET_FILE_DIR:xscreen>/ui
    )
endif()
