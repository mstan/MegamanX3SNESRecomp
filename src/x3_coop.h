#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* X3 USA: $04:8004 selects $09D8; $00:DA7F/DC06 walk the
 * $0AE8..0CC7 auxiliaries; $00:DB9F walks nine $40-byte shots.
 * Ride Armor at $0CC8 and all enemy/item pools remain shared. */
enum {
  X3_COOP_BODY = 0x09d8, X3_COOP_BODY_BYTES = 0x110,
  X3_COOP_AUX = 0x0ae8, X3_COOP_AUX_BYTES = 0x1e0,
  X3_COOP_SHOTS = 0x10d8, X3_COOP_SHOT_BYTES = 0x240,
  X3_COOP_PALETTE = 0x400, X3_COOP_PALETTE_BYTES = 64, /* OBJ palettes80/90 */
  X3_COOP_X = 0, X3_COOP_ZERO = 1,
};
typedef enum X3CoopLife { X3_COOP_ABSENT, X3_COOP_ALIVE, X3_COOP_FALLEN } X3CoopLife;
typedef struct X3CoopPlayer {
  uint8_t body[X3_COOP_BODY_BYTES];
  uint8_t auxiliaries[X3_COOP_AUX_BYTES];
  uint8_t shots[X3_COOP_SHOT_BYTES];
  uint8_t palette[X3_COOP_PALETTE_BYTES];
  uint16_t previous_actions;
  uint8_t life, character, shot_command;
} X3CoopPlayer;
typedef struct X3CoopState {
  X3CoopPlayer players[2];
  uint8_t initialized, current, anchor, wipe;
} X3CoopState;

bool X3CoopInit(X3CoopState *state, const uint8_t ram[0x20000], unsigned p1);
bool X3CoopValid(const X3CoopState *state);
void X3CoopCapture(X3CoopState *state, const uint8_t ram[0x20000]);
bool X3CoopProject(X3CoopState *state, uint8_t ram[0x20000], unsigned seat);
/* These are lifecycle events, not frame polls. A checkpoint or section change
 * deliberately has no revival operation. Fallen actors can only return via
 * an explicit, complete-team retry. */
bool X3CoopFall(X3CoopState *state, unsigned seat);
bool X3CoopWithdraw(X3CoopState *state, unsigned seat);
bool X3CoopRejoin(X3CoopState *state, unsigned seat);
bool X3CoopTeamRetry(X3CoopState *state);
size_t X3CoopEncodedSize(void);
bool X3CoopEncode(const X3CoopState *state, void *out, size_t size);
bool X3CoopDecode(X3CoopState *state, const void *data, size_t size);
uint16_t X3CoopActions(uint16_t input, const uint8_t masks[6]);
