cmake_minimum_required(VERSION 3.16)
project(MegaManX3SNESRecomp C CXX)

# Mega Man X3 (USA) — static recompilation via snesrecomp.
#
# Prerequisites:
#   * snesrecomp + recomp-ui at the repo root (submodules, or junctions in a
#     local multi-game working tree);
#   * generated C in src/gen/ (`bash tools/regen.sh` with a verified
#     mmx3.sfc staged at the repo root);
#   * SDL2 + OpenGL.

set(CMAKE_C_STANDARD 11)

set(SNESRECOMP_BUILD_VERSION "dev" CACHE STRING
    "Version reported in crash diagnostics and network metadata")

set(SNESRECOMP_ROOT "${CMAKE_SOURCE_DIR}/snesrecomp" CACHE PATH
    "Path to the snesrecomp checkout used by this game")
set(SNESRECOMP_ENABLE_MODS ON CACHE BOOL
    "Enable MMX3's built-in mod catalog and SNES mod packages" FORCE)
include(${SNESRECOMP_ROOT}/runner/runner.cmake)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# MinGW's SDL2 package ships SDL2main by default, but main.c provides a plain
# main() and uses the CRT entry point; SDL2main's WinMain would demand an
# SDL_main() that does not exist.
if(WIN32)
    list(FILTER SDL2_LIBRARIES EXCLUDE REGEX "SDL2main")
endif()

file(GLOB X3_GEN_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/gen/*.c)
if(NOT X3_GEN_SOURCES)
    message(FATAL_ERROR
        "src/gen/ is empty — run `bash tools/regen.sh` with a verified "
        "mmx3.sfc at the repo root before building.")
endif()

add_executable(MegaManX3SNESRecomp
    ${SNESRECOMP_RUNNER_SOURCES}
    # game-specific host runtime
    src/main.c
    src/opengl.c
    src/gen_stubs.c
    src/x3_display.c
    src/x3_cpu_infra.c
    src/x3_rtl.c
    src/x3_spc_player.c
    src/mods/x3_widescreen_plugin.c

    # OpenGL function loader
    third_party/gl_core/gl_core_3_1.c
    # recompiled banks (generated, gitignored)
    ${X3_GEN_SOURCES}
)
set_property(SOURCE src/main.c APPEND PROPERTY COMPILE_DEFINITIONS
    SNESRECOMP_BUILD_VERSION="${SNESRECOMP_BUILD_VERSION}")

target_include_directories(MegaManX3SNESRecomp PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/recomp
    ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
)
snesrecomp_target_fiber_compat(MegaManX3SNESRecomp)
snesrecomp_target_post_mortem(MegaManX3SNESRecomp TIER2)
snesrecomp_target_mmx_config(MegaManX3SNESRecomp)
snesrecomp_target_glsl_shader(MegaManX3SNESRecomp)

# prod-vs-debug via the single SNESRECOMP_ENABLE_TRACE switch (runner.cmake,
# default OFF). Prod: no debug_server.c / TCP server / rings.
if(SNESRECOMP_ENABLE_TRACE)
    set(_X3_TRACE 1)
else()
    set(_X3_TRACE 0)
endif()

target_compile_definitions(MegaManX3SNESRecomp PRIVATE
    SMW_DISABLE_LM=1
    SYSTEM_VOLUME_MIXER_AVAILABLE=0   # Win32 WASAPI volume control excluded
    SNESRECOMP_TRACE=${_X3_TRACE}
    SNESRECOMP_REVERSE_DEBUG=0        # raw WRAM stores; matches non-debug gen C
)

# The recompiled C is machine-generated; quiet its warnings and tolerate the
# K&R-style implicit declarations clang rejects by default. Keep the
# implicit-decl flag C-only so the launcher C++ TUs stay clean.
if(NOT MSVC)
    target_compile_options(MegaManX3SNESRecomp PRIVATE -w)
    target_compile_options(MegaManX3SNESRecomp PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration>)
endif()

target_link_libraries(MegaManX3SNESRecomp PRIVATE
    OpenGL::GL
    ${SNESRECOMP_RUNNER_LIBRARIES}
)

if(WIN32 AND NOT MSVC)
    # msys2/mingw SDL2 does not auto-rename main()->SDL_main; link SDL2 without
    # SDL2main. dbghelp is post_mortem.c's StackWalk64 backtrace.
    target_link_libraries(MegaManX3SNESRecomp PRIVATE
        C:/msys64/mingw64/lib/libSDL2.dll.a)
else()
    target_link_libraries(MegaManX3SNESRecomp PRIVATE ${SDL2_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(MegaManX3SNESRecomp PRIVATE ws2_32)
endif()

# Shared Dear ImGui pre-boot launcher (recomp-ui submodule/junction).
set(RECOMP_UI_ROOT "${CMAKE_SOURCE_DIR}/recomp-ui" CACHE PATH
    "Root directory of the recomp-ui launcher repo" FORCE)
if(NOT EXISTS "${RECOMP_UI_ROOT}/recomp_ui.cmake")
    message(FATAL_ERROR
        "recomp-ui missing at ${RECOMP_UI_ROOT}.\n"
        "Run: git submodule update --init --recursive recomp-ui")
endif()
include(${RECOMP_UI_ROOT}/recomp_ui.cmake)
recomp_target_launcher_ui(MegaManX3SNESRecomp
    BOXART ${CMAKE_SOURCE_DIR}/recomp/launcher/boxart.tga)

# Preload the game-owned catalog. Every feature remains disabled until the
# player enables it in Mods; package plugin IDs resolve only to implementations
# statically linked above.
set(X3_PRELOADED_MODS "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")
if(EXISTS "${X3_PRELOADED_MODS}/packages")
    add_custom_command(TARGET MegaManX3SNESRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${X3_PRELOADED_MODS}"
            "$<TARGET_FILE_DIR:MegaManX3SNESRecomp>/mods"
        COMMENT "Preloading the Mega Man X3 mod catalog")
endif()
