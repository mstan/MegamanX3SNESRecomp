# Save states and rewind

F7 opens the save-state browser; F8 opens rewind. In recomp-ui, use the
Hotkeys page to rebind or clear either action, including modifier keys.
Existing F7/F8 quick-load defaults migrate to F11/F12; Shift+F1 through
Shift+F10 still save directly. User config.ini and keybinds.ini stay local.

The save browser has 12 slots: arrows select, X loads, S saves, Escape
closes. On a controller, Select+R opens it; Up/Down select, A loads, X saves,
and B closes. Rewind opens with Select+R3 by default; [Controller]
RewindGesture can change or disable that gesture. Left/Right select a frame,
A or Enter restores it, and B or Escape cancels. The guest stays frozen
while a menu is open. Rewind keeps about six seconds by default; the shared
SNESRECOMP_REWIND_DEPTH and SNESRECOMP_REWIND_INTERVAL settings control it.

New states include the title's execution cursor and the framework's complete
CPU, interpreter, APU timing and PPU rollback residue. States are intended
for matching runtime builds; old states retain the earlier compatibility
path. A file load or reset starts a new rewind history. There is no netplay
menu for these single-player games.

## Focused verification

Configure with -DMMX_STATE_TESTS=ON, build mmx_state_tests, then run the
framework's runner/tests/run_mmx_state_tests.py with --exe pointing to that
test executable and --rom pointing to your own verified ROM. It uses a
temporary save directory and checks binding migration, recomp-ui rebind and
clear, complete memory restore, deterministic replay, slot save, rewind,
and loading the file in a new process.

CI builds a ROM-free setup host using SNESRECOMP_SETUP_HOST=ON. Playable
builds use tools/regen.sh and the same shared generated-code, desktop-host,
SDL, OpenGL and mod-catalog CMake helpers as the current project template.
