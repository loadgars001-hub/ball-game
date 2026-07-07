cmake_minimum_required(VERSION 3.14)
project(BallGame VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.5 COMPONENTS graphics window system network REQUIRED)

# ── Lua ──────────────────────────────────────────────────────────────────
if(WIN32)
    # На Windows (через vcpkg) Lua ищется штатным модулем CMake FindLua —
    # vcpkg toolchain сам подставляет нужные пути после:
    #   vcpkg install lua:x64-windows
    find_package(Lua REQUIRED)
else()
    # На Linux используем системную Lua через pkg-config
    # (пакет liblua5.4-dev на Ubuntu/Debian: sudo apt install liblua5.4-dev)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(LUA REQUIRED lua5.4 lua54 lua-5.4 lua)
endif()

# ── sol2 (header-only C++ биндинг для Lua) ─────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.3.0
)
# ВАЖНО: используем Populate + add_subdirectory(... EXCLUDE_FROM_ALL) вместо
# FetchContent_MakeAvailable(sol2) — иначе install()-правила самой sol2
# (заголовки, sol2-config.cmake, sol2.pc) попадают в наш `.deb` пакет при
# сборке через CPack, хотя конечному пользователю игры они не нужны:
# EXCLUDE_FROM_ALL отключает install() для этой поддиректории, но саму
# библиотеку (INTERFACE-таргет sol2) по-прежнему можно линковать как обычно.
FetchContent_GetProperties(sol2)
if(NOT sol2_POPULATED)
    FetchContent_Populate(sol2)
    add_subdirectory(${sol2_SOURCE_DIR} ${sol2_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# ── Box2D ────────────────────────────────────────────────────────────────
# На Windows (vcpkg) Box2D ставит полноценный CMake CONFIG-пакет с целью
# box2d::box2d — пробуем его первым на любой платформе, это самый надёжный
# способ. Если не найден (типичный случай на Linux без vcpkg) — откатываемся
# на pkg-config, а если и его нет — на ручной поиск заголовка/библиотеки.
#   Windows: vcpkg install box2d:x64-windows
#   Linux:   sudo apt install libbox2d-dev
find_package(box2d CONFIG QUIET)
if(NOT box2d_FOUND)
    if(NOT WIN32)
        pkg_check_modules(BOX2D QUIET box2d)
    endif()
    if(NOT BOX2D_FOUND)
        find_path(BOX2D_INCLUDE_DIRS NAMES box2d/box2d.h)
        find_library(BOX2D_LIBRARIES NAMES box2d libbox2d)
        if(NOT BOX2D_INCLUDE_DIRS OR NOT BOX2D_LIBRARIES)
            message(FATAL_ERROR "Box2D not found. Windows: vcpkg install box2d:x64-windows. Linux: sudo apt install libbox2d-dev")
        endif()
    endif()
endif()

add_executable(ball_game
    main.cpp
    ball.cpp
    wall.cpp
    level.cpp
    game_core.cpp
    game_menu.cpp
    game_editor.cpp
    game_script_editor.cpp
    game_hud.cpp
    item.cpp
    script_engine.cpp
    physics_world.cpp
    spinner.cpp
    network.cpp
)

target_include_directories(ball_game PRIVATE ${LUA_INCLUDE_DIRS} ${LUA_INCLUDE_DIR} ${BOX2D_INCLUDE_DIRS})

target_link_libraries(ball_game
    sfml-graphics sfml-window sfml-system sfml-network
    sol2
)

# std::filesystem: на GCC/MinGW до недавних версий требует явной линковки
# stdc++fs; на MSVC (Windows) это часть стандартной библиотеки, линковать
# отдельно не нужно и такой цели там просто нет.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(ball_game stdc++fs)
endif()

# Lua: на Windows find_package(Lua) даёт переменные LUA_LIBRARIES,
# на Linux pkg_search_module даёт те же имена — линкуем одинаково.
target_link_libraries(ball_game ${LUA_LIBRARIES})
target_link_directories(ball_game PRIVATE ${LUA_LIBRARY_DIRS})

# Box2D: предпочитаем CONFIG-цель (Windows/vcpkg), иначе — переменные из
# pkg-config/ручного поиска (Linux).
if(box2d_FOUND)
    target_link_libraries(ball_game box2d::box2d)
else()
    target_include_directories(ball_game PRIVATE ${BOX2D_INCLUDE_DIRS})
    target_link_libraries(ball_game ${BOX2D_LIBRARIES})
    target_link_directories(ball_game PRIVATE ${BOX2D_LIBRARY_DIRS})
endif()

# ── Установка (нужна и для `make install`, и для сборки .deb через CPack) ──
install(TARGETS ball_game RUNTIME DESTINATION bin)
install(FILES packaging/ball_game.desktop
        DESTINATION share/applications)
install(FILES packaging/ball_game.png
        DESTINATION share/icons/hicolor/128x128/apps)

# ── CPack: сборка .deb пакета ────────────────────────────────────────────
# Использование:
#   cd build
#   cpack -G DEB
# Результат — файл вида ball-game_1.0.0_amd64.deb, который ставится через:
#   sudo apt install ./ball-game_1.0.0_amd64.deb
# (apt сам подтянет недостающие зависимости из списка ниже, если они ещё не
# установлены — в этом и есть смысл использовать именно `apt install`,
# а не голый `dpkg -i`, который зависимости не разрешает).
if(NOT WIN32)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "ball-game")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_CONTACT "you@example.com") # замените на свой e-mail — Debian требует непустой Maintainer
    set(CPACK_PACKAGE_VENDOR "BallGame Project")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Physics ball puzzle game with Lua scripting and LAN multiplayer")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://example.com")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})

    # Библиотеки времени исполнения — версии соответствуют Ubuntu 24.04
    # (SFML 2.6, Lua 5.4, Box2D 2.4.1). Если собираете на другом дистрибутиве
    # с другими версиями пакетов, поправьте номера здесь под `dpkg -l | grep`.
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libsfml-graphics2.6, libsfml-window2.6, libsfml-system2.6, libsfml-network2.6, liblua5.4-0, libbox2d2"
    )

    # CPack сам корректно определяет архитектуру (amd64/arm64/...) и формирует
    # имя файла по debian-конвенции <name>_<version>_<arch>.deb
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

    include(CPack)
endif()
