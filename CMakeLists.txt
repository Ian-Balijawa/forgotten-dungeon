cmake_minimum_required(VERSION 3.24)
project(ForgottenDungeon VERSION 1.0.0 DESCRIPTION "A 3D dungeon adventure where the dungeon remembers" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------- raylib
# Uses an installed raylib (e.g. `brew install raylib`) if present,
# otherwise downloads and builds raylib 5.5 automatically.
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
  FIND_PACKAGE_ARGS 5.5)
set(CMAKE_SKIP_INSTALL_RULES ON)   # keep raylib out of our installer
FetchContent_MakeAvailable(raylib)
set(CMAKE_SKIP_INSTALL_RULES OFF)

# ---------------------------------------------------------------- game
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(dungeon ${GAME_SOURCES})
target_include_directories(dungeon PRIVATE src)
target_link_libraries(dungeon PRIVATE raylib)

if(APPLE)
  set_target_properties(dungeon PROPERTIES
    OUTPUT_NAME "ForgottenDungeon"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "The Forgotten Dungeon"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.forgottendungeon.game"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
else()
  set_target_properties(dungeon PROPERTIES OUTPUT_NAME "forgotten-dungeon")
endif()

if(MSVC)
  target_compile_options(dungeon PRIVATE /W3)
  target_link_options(dungeon PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
else()
  target_compile_options(dungeon PRIVATE -Wall -Wextra -Wno-unused-parameter)
  if(MINGW)
    target_link_options(dungeon PRIVATE -mwindows)
  endif()
endif()

# ---------------------------------------------------------------- install
if(APPLE)
  install(TARGETS dungeon BUNDLE DESTINATION .)
  install(DIRECTORY assets/ DESTINATION ForgottenDungeon.app/Contents/Resources/assets)
elseif(WIN32)
  install(TARGETS dungeon RUNTIME DESTINATION .)
  install(DIRECTORY assets/ DESTINATION assets)
else()
  install(TARGETS dungeon RUNTIME DESTINATION bin)
  install(DIRECTORY assets/ DESTINATION share/forgotten-dungeon/assets)
  install(FILES packaging/linux/forgotten-dungeon.desktop DESTINATION share/applications)
endif()

include(cmake/Packaging.cmake)
