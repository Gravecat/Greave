# CMake build file for Greave, intended for use with MinGW/GCC on Windows and GCC on Linux. Support for OS X, Visual Studio, clang, etc. is not currently available.
# For MinGW/MSYS builds, be sure to specify -G"MinGW Makefiles" when setting up the build folder.
# Copyright (c) 2020-2021 Raine "Gravecat" Simmons and the Greave contributors. Licensed under the GNU Affero General Public License v3 or any later version.

cmake_minimum_required(VERSION 3.13)


# Non-platform-specific stuff.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -pedantic-errors")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
set(CMAKE_CXX_FLAGS_RELEASE  "-O2")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
add_definitions(-D__STDC_LIMIT_MACROS)
add_definitions(-D_USE_MATH_DEFINES)
set(SQLITECPP_LIBRARIES sqlite_cpp)
set(LODEPNG_LIBRARIES lodepng)


# Main platform-specific settings.
if(TARGET_WINDOWS)

  # Platform-specific libraries.
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  set(OS_LIBRARIES mingw32)
  set(TOLK_LIBRARIES Tolk)
  add_definitions(-DGREAVE_TOLK)
  set(YAMLCPP_LIBRARIES yaml-cpp.dll)
  if(INCLUDE_CURSES)
    set(CURSES_LIBRARIES pdcurses)
  else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
  endif(INCLUDE_CURSES)
  if(INCLUDE_SDL)
    set(OS_LIBRARIES ${OS_LIBRARIES} SDL2main)
    set(SDL2_EXTRA_LIBRARIES SDL2.dll SDL2_ttf.dll)
  endif(INCLUDE_SDL)

  # Application icon
  enable_language(RC)
  set(CMAKE_RC_COMPILER_INIT windres)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  set(GREAVE_RC core/app-icon.rc)

elseif(TARGET_LINUX)

  # Platform-specific libraries.
  set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/src/3rdparty/cmake_modules)
  find_package(Threads REQUIRED)
  find_package(X11 REQUIRED)
  set(OS_LIBRARIES ${X11_LIBRARIES})
  set(YAMLCPP_LIBRARIES yaml-cpp)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -Wl,-rpath='${ORIGIN}'")
  if(INCLUDE_CURSES)
    set(CURSES_LIBRARIES ncurses)
  endif(INCLUDE_CURSES)
  if(INCLUDE_SDL)
    find_package(SDL2 REQUIRED)
    find_package(SDL2TTF REQUIRED)
    set(SDL2_EXTRA_LIBRARIES ${SDL2TTF_LIBRARY})
  endif(INCLUDE_SDL)

else()

    message([FATAL_ERROR] "Greave is not currently supported on this platform.")

endif(TARGET_WINDOWS)

# Source files.
set(GREAVE_CPPS
  actions/abilities.cc
  actions/ai.cc
  actions/arena.cc
  actions/cheat.cc
  actions/combat.cc
  actions/doors.cc
  actions/eat-drink.cc
  actions/help.cc
  actions/inventory.cc
  actions/look.cc
  actions/potions.cc
  actions/rest.cc
  actions/status.cc
  actions/travel.cc
  core/bones.cc
  core/core.cc
  core/core-constants.cc
  core/filex.cc
  core/guru.cc
  core/list.cc
  core/mathx.cc
  core/message.cc
  core/parser.cc
  core/prefs.cc
  core/random.cc
  core/strx.cc
  core/terminal.cc
  core/terminal-curses.cc
  core/terminal-sdl2.cc
  world/inventory.cc
  world/item.cc
  world/mobile.cc
  world/player.cc
  world/room.cc
  world/shop.cc
  world/time-weather.cc
  world/world.cc
)


# Binary file. GREAVE_RC should be blank for non-Windows builds.
add_executable(greave ${GREAVE_CPPS} ${GREAVE_RC})


# Include directories. 3rdparty is included here, because otherwise yaml-cpp gets unhappy.
target_include_directories(greave PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/3rdparty"
)


# Platform-specific library directories.
if(TARGET_WINDOWS)
  target_link_directories(greave PRIVATE "${CMAKE_SOURCE_DIR}/lib/win64")
else()
  target_link_directories(greave PRIVATE "${CMAKE_SOURCE_DIR}/lib/lin64")
endif(TARGET_WINDOWS)


# Link libraries. Platform-specific stuff should be set in the main platform-specific section near the top.
target_link_libraries(greave
  ${OS_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CURSES_LIBRARIES}
  ${SQLITECPP_LIBRARIES}
  ${TOLK_LIBRARIES}
  ${YAMLCPP_LIBRARIES}
  ${SDL2_LIBRARIES}
  ${SDL2_EXTRA_LIBRARIES}
  ${LODEPNG_LIBRARIES}
)


# Post-build, make a 'bin' folder and copy the binary file in there.
add_custom_command(TARGET greave POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:greave> "${CMAKE_BINARY_DIR}/bin"
)


# LodePNG and SQLiteCpp come with source files as well as headers, so we'll compile them separately.
add_subdirectory(3rdparty/LodePNG)
add_subdirectory(3rdparty/SQLiteCpp)
