cmake_minimum_required(VERSION 3.20)
project(MegaManX3SNESRecomp C CXX)
set(CMAKE_C_STANDARD 11)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _version)
string(STRIP "${_version}" GAME_RELEASE_VERSION)
set(SNESRECOMP_BUILD_VERSION "" CACHE STRING "Build stamp (empty: Release uses VERSION, otherwise dev)")
if(SNESRECOMP_BUILD_VERSION STREQUAL "")
    set(SNESRECOMP_BUILD_VERSION "$<IF:$<CONFIG:Release>,${GAME_RELEASE_VERSION},dev>")
endif()
set(SNESRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/snesrecomp" CACHE PATH "snesrecomp checkout")
set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui" CACHE PATH "recomp-ui checkout")
set(SNESRECOMP_ENABLE_MODS ON CACHE BOOL "Shared mod package runtime" FORCE)
option(SNESRECOMP_REVERSE_DEBUG "Instruction-level debugger instrumentation" OFF)
include(${SNESRECOMP_ROOT}/runner/runner.cmake)

add_executable(MegaManX3SNESRecomp
    ${SNESRECOMP_RUNNER_SOURCES}
    src/main.c src/gen_stubs.c src/x3_display.c src/x3_cpu_infra.c
    src/x3_rtl.c src/x3_spc_player.c src/mmx_replay.c src/mmx_startup.c src/mods/x3_widescreen_plugin.c)
target_include_directories(MegaManX3SNESRecomp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/recomp ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
target_compile_definitions(MegaManX3SNESRecomp PRIVATE
    SMW_DISABLE_LM=1 SYSTEM_VOLUME_MIXER_AVAILABLE=0
    SNESRECOMP_TRACE=$<BOOL:${SNESRECOMP_ENABLE_TRACE}>
    SNESRECOMP_REVERSE_DEBUG=$<BOOL:${SNESRECOMP_REVERSE_DEBUG}>)
set_property(SOURCE src/main.c src/mmx_replay.c APPEND PROPERTY COMPILE_DEFINITIONS
    SNESRECOMP_BUILD_VERSION="${SNESRECOMP_BUILD_VERSION}")
if(NOT MSVC)
    target_compile_options(MegaManX3SNESRecomp PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration>)
endif()
target_link_libraries(MegaManX3SNESRecomp PRIVATE ${SNESRECOMP_RUNNER_LIBRARIES})
snesrecomp_target_sdl(MegaManX3SNESRecomp)
snesrecomp_target_fiber_compat(MegaManX3SNESRecomp)
snesrecomp_target_desktop_host(MegaManX3SNESRecomp TIER2)
snesrecomp_target_generated_code(MegaManX3SNESRecomp ${CMAKE_CURRENT_SOURCE_DIR}/src/gen)
snesrecomp_target_mod_catalog(MegaManX3SNESRecomp ${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded)

include(${RECOMP_UI_ROOT}/recomp_ui.cmake)
recomp_target_launcher_ui(MegaManX3SNESRecomp CONSOLE snes
    BOXART ${CMAKE_CURRENT_SOURCE_DIR}/recomp/launcher/boxart.tga)

include(${SNESRECOMP_ROOT}/runner/cmake/mmx_state_tests.cmake)
snesrecomp_target_mmx_state_tests(MegaManX3SNESRecomp ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
if(TARGET mmx_state_tests)
    get_target_property(_state_sources mmx_state_tests SOURCES)
    list(REMOVE_ITEM _state_sources "${SNESRECOMP_RUNNER_ROOT}/tests/mmx_state_runtime.c")
    list(APPEND _state_sources tests/mmx_state_runtime.c)
    set_property(TARGET mmx_state_tests PROPERTY SOURCES "${_state_sources}")
endif()

# Keep capture identities tied to the exact game and engine source revisions.
find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE MMX_REPLAY_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${SNESRECOMP_ROOT}
    OUTPUT_VARIABLE MMX_REPLAY_ENGINE OUTPUT_STRIP_TRAILING_WHITESPACE)
set_property(SOURCE src/mmx_replay.c APPEND PROPERTY COMPILE_DEFINITIONS
    MMX_REPLAY_REVISION="${MMX_REPLAY_REVISION}" MMX_REPLAY_ENGINE="${MMX_REPLAY_ENGINE}")
if(WIN32)
    set_target_properties(MegaManX3SNESRecomp PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/logging.ini")
    configure_file(logging.ini logging.ini COPYONLY)
endif()
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(${SNESRECOMP_ROOT}/runner/tests/diagnostics diagnostic-writer-tests)
endif()
