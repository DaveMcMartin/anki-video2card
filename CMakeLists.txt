cmake_minimum_required(VERSION 3.25)

project(AnkiVideo2Card VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG preview-3.1.3
)

set(SDL_SHARED ON CACHE BOOL "Build shared SDL library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build static SDL library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "Disable installing SDL" FORCE)

FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(cpp-httplib)

# Find Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBP REQUIRED libwebp)
pkg_check_modules(MPV REQUIRED mpv)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale libswresample)

# Try to find Mecab with pkg-config first, fall back to manual detection
pkg_check_modules(MECAB mecab)
if(NOT MECAB_FOUND)
    # Fallback: use mecab-config on macOS
    find_program(MECAB_CONFIG mecab-config)
    if(MECAB_CONFIG)
        execute_process(
            COMMAND ${MECAB_CONFIG} --libs
            OUTPUT_VARIABLE MECAB_LIBS
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${MECAB_CONFIG} --inc-dir
            OUTPUT_VARIABLE MECAB_INCLUDE_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Parse libs to extract library path and library name
        string(REGEX MATCH "-L[^ ]+" MECAB_LIB_PATH "${MECAB_LIBS}")
        string(REPLACE "-L" "" MECAB_LIBRARY_DIRS "${MECAB_LIB_PATH}")
        set(MECAB_LIBRARIES mecab)
        set(MECAB_FOUND TRUE)
        message(STATUS "Found Mecab via mecab-config")
        message(STATUS "  MECAB_INCLUDE_PATH: ${MECAB_INCLUDE_PATH}")
        message(STATUS "  MECAB_LIBRARY_DIRS: ${MECAB_LIBRARY_DIRS}")
    else()
        message(FATAL_ERROR "Mecab not found. Please install it: brew install mecab mecab-ipadic")
    endif()
endif()

add_library(imgui_sdl3 STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imgui_sdl3 PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
)



target_link_libraries(imgui_sdl3 PUBLIC SDL3::SDL3)

# Force re-scan of source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.h"
)

if(APPLE)
    list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/assets/logo.icns")
    set_source_files_properties("${CMAKE_SOURCE_DIR}/assets/logo.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
elseif(WIN32)
    list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/assets/windows_icon.rc")
endif()

add_executable(AnkiVideo2Card MACOSX_BUNDLE ${SOURCES})

target_include_directories(AnkiVideo2Card PRIVATE
    src
    src/core
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/assets
    ${WEBP_INCLUDE_DIRS}
    ${MPV_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${MECAB_INCLUDE_PATH}
    ${MECAB_INCLUDE_DIRS}
)

target_link_libraries(AnkiVideo2Card PRIVATE
    imgui_sdl3
    SDL3::SDL3
    nlohmann_json::nlohmann_json
    httplib::httplib
    ${WEBP_LIBRARIES}
    ${MPV_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${MECAB_LIBRARIES}
)

target_link_directories(AnkiVideo2Card PRIVATE
    ${WEBP_LIBRARY_DIRS}
    ${MPV_LIBRARY_DIRS}
    ${FFMPEG_LIBRARY_DIRS}
    ${MECAB_LIBRARY_DIRS}
)

if(APPLE)
    # Copy assets to Resources directory
    add_custom_command(TARGET AnkiVideo2Card POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:AnkiVideo2Card>/../Resources/assets
    )

    # Create Frameworks directory in the app bundle
    add_custom_command(TARGET AnkiVideo2Card POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:AnkiVideo2Card>/../Frameworks
    )

    # Copy SDL3 library to Frameworks directory
    add_custom_command(TARGET AnkiVideo2Card POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/lib/libSDL3.0.dylib
        $<TARGET_FILE_DIR:AnkiVideo2Card>/../Frameworks/libSDL3.0.dylib
    )

    # Update the SDL3 library's install name to use @rpath
    add_custom_command(TARGET AnkiVideo2Card POST_BUILD
        COMMAND install_name_tool -id @rpath/libSDL3.0.dylib
        $<TARGET_FILE_DIR:AnkiVideo2Card>/../Frameworks/libSDL3.0.dylib
    )

    # Update the executable's reference to SDL3 to use @rpath
    add_custom_command(TARGET AnkiVideo2Card POST_BUILD
        COMMAND install_name_tool -change
        ${CMAKE_BINARY_DIR}/lib/libSDL3.0.dylib
        @rpath/libSDL3.0.dylib
        $<TARGET_FILE:AnkiVideo2Card>
    )

    # Set the rpath in the executable to find frameworks
    set_target_properties(AnkiVideo2Card PROPERTIES
        MACOSX_BUNDLE_ICON_FILE logo.icns
        MACOSX_BUNDLE_BUNDLE_NAME "Anki Video2Card"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.ankivideo2card.app"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0"
        INSTALL_RPATH "@loader_path/../Frameworks"
    )
else()
    add_custom_command(TARGET AnkiVideo2Card POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:AnkiVideo2Card>/assets
    )
endif()

if(WIN32)
    set_target_properties(AnkiVideo2Card PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()
