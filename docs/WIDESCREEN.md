# Mega Man X3 16:9 widescreen

## Status

Implemented for Mega Man X3 (USA):

- true 342x224 rendering at 16:9;
- player/weapon HUD anchored to the left and boss HUD anchored to the right;
- exact BG1 and BG2 tiles in newly visible columns, including while scrolling;
- widescreen-aware enemy activation, visibility, drawing, and camera triggers;
- unchanged 4:3 output when the feature is disabled.

The launcher exposes the widescreen option. The embedded default remains
`Widescreen = 0`, so a new install starts in authentic 4:3 until selected.

## Mega Man X2 correspondence

X3 retained the relevant X2 engine structures with relocated code. The X2
implementation and `docs/OAM_SURVEY.md` were used as the structural reference;
every X3 address and live data source below was then verified against X3.

### Background streamers

| Purpose | X3 address |
|---|---:|
| tile composer | `$00:B91B` |
| world/layout derivation | `$00:BC60` |
| BG1 source setup | `$00:BC9E` |
| BG2 source setup | `$00:BCD5` |

The retained level-map inputs are:

| Layer | layout | screen definitions | metatile pointer | world X/Y | BGSC | VRAM map |
|---|---:|---:|---:|---:|---:|---:|
| BG1 | `$7E:E800` | `$7E:2800` | `[$09C5]` | `$1E5D/$1E60` | `$51` | `$5000` |
| BG2 | `$7E:EC00` | `$7E:A600` | `[$09C8]` | `$1E9D/$1EA0` | `$59` | `$5800` |

`X3ConfigureWsBgMargins` reconstructs only gutter tiles from these structures.
Before enabling a layer each frame, it compares 12 reconstructed native-view
samples against VRAM, permits at most one mismatch, and rejects uninitialized
or near-uniform sources. This makes menus and unrelated layer modes fail safe
to authentic PPU wrapping. Native-view tiles are never replaced.

Game-authored gutter writes take priority for 60 frames, matching X2's
stream-transition handling. `SNESRECOMP_WS_BG_MARGINS=0` disables this provider;
`SNESRECOMP_WS_BG_MARGINS_DEBUG=1` logs its validation state.

### HUD

Live X3 OAM on save slot 0 confirms the X2 layout:

- slots 0-5: player health, X=8;
- slots 7-13: weapon health, X=24;
- slots 16-22: boss health, X=232.

The player and weapon bars shift toward the widescreen left edge; the boss bar
shifts toward the right edge. The shift is enabled only when the measured
player-health signature is present (slot 0 tile `$86`, attribute `$34`, plus at
least four matching frame slots). Cutscenes that reuse slots 0-23 therefore
remain unshifted.

The bars are OAM-only. BG3 remains centered even while the HUD signature is
present because X3 uses BG3 for in-game dialogue. This is deliberately
different from a coupled BG3 HUD split: save slot 4's Bit encounter places
`(BIT)`, `Welcome, X.`, and `I'm Bit.` across the left/center split boundary,
which would pull the first two character tiles into the gutter.

### Object activation, visibility, and culling

X3's shared object-window family is the relocated X2 implementation:

| X3 routine | vanilla horizontal test |
|---|---|
| `$02:D58A` activation | `(objX - camX + $40) < $180` |
| `$02:D611` visibility | `(objX - camX + $60) < $1C0` |
| `$02:D636` draw/common AI tail | `(objX - camX + $20) < $140` |

`tools/apply_overrides.py` recognizes generated C structurally: a read of the
X camera anchor `$1E5D`, followed by a formula-consistent add/limit pair. A
`$1E60` read disarms the match so vertical windows remain unchanged. It also
recognizes camera-X/add followed by an actual `CMP dp+$05`; a plain read of
that field is not sufficient.

The matched horizontal windows grow by `margin + 32` on each side. The extra
32 pixels ensure an object activates outside the visible 16:9 edge instead of
popping on its outermost column. Centered-distance routine `$02:DAE7` grows
both its leading offset and final limit; changing only the offset would shift
rather than expand its window. `$13:EED6` is motion arithmetic, not a viewport
test, and is deliberately untouched.

The current generation contains exactly 30 object-window rewrites / 12
complete pairs.
`x3_rtl.c` also scans the private runtime ROM copy and patches 24 operands:
seven standard pairs, the `$02:DAE7` centered pair, the `$03:DDF3` bounds pair,
and six CMP triggers. This covers missing M/X variants plus interpreter-only
windows `$00:DF9F` and `$07:B36E`. Regeneration fails if either generated
census changes unexpectedly.

The helpers return their vanilla constants whenever widescreen is inactive.
`SNESRECOMP_WS_SPAWN=0` independently disables object-window widening.

### Dynamic level-record frontier

X3 retained X2's global dynamic record streamer byte-for-byte: `$00:DE3A`
selects 32-pixel columns and `$00:DED3` walks/allocates their records. These
records include scripted progression, not just ordinary actors. Widening the
frontier instantiated an intro-stage progression record from the 16:9 gutter;
descending the hangar ladder then selected the exterior route and could kill X.

The four exact generated hooks therefore retain the authentic 256-pixel
left/right probes and vertical sweep by default. Ordinary resident-object
activation, visibility, and draw windows remain widened, so this does not
disable widescreen rendering. `SNESRECOMP_WS_STREAM=1` restores the old widened
record frontier only as a diagnostic opt-in.

Interpreter tail-JMP arrivals do not consult the generated dispatch table, so
`x3_rtl.c` independently validates the exact DE3A/DED3 byte shapes and patches
the private cart copy. The left arm uses an equal-length, cycle-exact
`SBC/NOP/JMP` replacement; the other three changes are immediate operands.
No site changes unless every signature matches, and inactive/kill-switch mode
restores all original bytes. `tools/apply_overrides.py` requires exactly four
streamer hooks in addition to the 30 object-window rewrites; their helpers
resolve to the original values during normal play.

## Native promotion boundary

Profile promotion originally froze immediately after loading save slot 0.
First-frame return tracing isolated `$00:DD59 -> $03:8DA0`. Routine `$03:8DA0`
calls a generated WRAM helper at `$7E:26A0` which rewrites the guest return
stack and returns directly to the outer caller. A native C call frame cannot
represent that non-local return; it leaked five guest-stack bytes per call.

`force_lle 038DA0` in `recomp/bank00.cfg` keeps that exact executable boundary
on the interpreter tier while its callers remain native. `$00:9133`, which
enters the same stack-programmed WRAM dispatcher at `$7E:26A6` during the
CAPCOM/title transition, is held LLE for the same reason. The directive is
implemented in both Python and Rust analyzers in the paired latest-snesrecomp
worktree. Neither unsafe boundary receives a native dispatch slot.

## Validation

Save slot 0 was loaded through the paused debug bridge and driven right with a
deterministic input sequence.

- The repaired native build advanced beyond the formerly frozen first frame.
- Camera X moved from `$0100` to `$0204` over 180 rightward frames.
- Both BG layers stayed active while scrolling and accumulated hundreds of
  thousands of west/east shadow hits with zero misses.
- Captured 342x224 frames showed continuous jungle/terrain in both gutters.
- Enemy sprites were active and rendering into the east extension.
- Native entry into `$02:D636` was observed during the same scene.
- The runtime ROM census reported exactly 24 interpreter window operands and
  an exact DE3A streamer signature; generated reinjection reported exactly
  30 object sites / 12 pairs plus four streamer sites.
- With widescreen disabled, the pre-change interpreter build and the new
  native build produced an exact 256x224 pixel match after the same save and
  120-frame input sequence: zero differing pixels.

The DE3A hooks retain build/runtime-signature coverage. A TCP replay from the
intro-stage ladder save confirmed that the default authentic frontier keeps X
on the intended interior route while widescreen object windows remain active.

Generated sources are intentionally untracked. Always run
`tools/regen.sh`; it applies the widescreen overrides and checks idempotency.

## Owner feedback: doors and hunter landings (2026-10-26)

The shared engine now applies X1's strict mirrored door-body rule to X2/X3:
only the face looking into the current room is drawn. A presentation override
also suppresses recent uploads of the hidden face during opening/closing.
State loading resets unsaved renderer history. A 400-frame X3 crossing,
sampled every four frames against native width, has zero unexplained pixel
differences after excluding relocated HUD pixels and the hidden 16-pixel face.

The original Bit/Byte fall routines defer terrain collision until
`cameraY+$98/$78`. A vertical approach can put that plane below the floor;
the initial bad landing was also reproduced with the original ROM in Snes9x.
Use the original stage record to begin collision by `recordY-8`. Collision
chooses the actual landing; camera, actor coordinates, velocity and result
flags are not forced. Bit needs its later `$98` operand compensated as well.
Only `$07:907D` and `$3C:C588` receive this policy. Their interpreter hooks
validate the original bytes and complete the 16-bit LDA with its original
five slow-bus cycles and flags. No mutable ROM operands or extra serialized
state are required.

Generated reinjection now has 32 marked sites: the existing 30 horizontal
sites/12 pairs, plus these two landing reads, and four separate streamer
sites. `tools/test_hunter_landings.py` replays the before-door snapshots,
observes both initial landings and active combat, and refuses already-started
or completed setups. Byte lands at world Y663 and Bit at679, with visible feet
at703 against floor704 (the normal collision inset), instead of695/727.
Hybrid342, pure-interpreter684 and native256 checks pass. The owner accepts
Byte's floor, grab/recovery and exit, and confirms a single door face. The
remaining moving-door garble was traced separately to checkpoint preparation.

### Checkpoint graphics prerequisites

A direct stage-8 checkpoint-1 load selects graphics group4, which assumes
resident door asset `$56` from earlier group1. The prepared approach omitted
that prerequisite: `$7F:8256/$7F:8356` remained zero. The original `$01:E065`
door initializer cached those zeros and its animation referenced X's tiles.
Unmodified Snes9x reproduces the same garble with this incomplete setup. The
earlier native-width comparison only demonstrated renderer agreement; both
widths shared the missing graphics, so it did not establish authentic art.

`tools/hunter_approach.py` prepares the prerequisite through the original
`$00:B1FC` cooperative graphics job, loading group1 then restoring group4
before the room door initializes. It leaves actor/camera/collision/result
state to the original game. This explicit assist bypasses earlier traversal;
it does not validate the skipped graphics-group transition. The loaded base
is `$F0`, attributes `$2F`, and the door CHR is identical to the independent
original-ROM reference. Both the replay and owner-selector guards reject a
missing asset or an existing room door with stale cached graphics fields.

The owner subsequently accepted Byte and moving-door animation. That focused
handoff is complete; do not request it again. This does not qualify the remaining
fortress branches, final Sigma/endings, or other individual deferred checks.

### Locked Lab 1 arena margins (2026-10-27)

The Lab 1 end room exposed its neighboring corridor on the left and unbacked
map space on the right at very wide aspects. Its original locked bounds
`1600/0600/0600/1600/1600`, mode 1 and maps `$50/$58` now reuse the existing
Sigma wall-edge compositor and BG2 containment. The approach keeps its normal
streaming until the original room lock completes. No guest state is changed.

Paired native256, hybrid342 and interpreter684 runs plus the actual 360-frame
door approach preserve every native-view pixel, all 128 KiB WRAM and 64 KiB
VRAM in 48 samples. Byte420, factory360, Kaiser120, hub120 and native rematch
entry120 preserve complete output and WRAM against the preceding build.
Refreshed Kaiser and all-clear hub states each replay 120 frames identically
after current-build save/load. This qualifies presentation; alternate branch
selection, full Lab 1 combat and its following transition remain separate.

### Godkarmachine and Doppler follow-up (2026-10-27)

The lower Lab 1 checkpoint2 room and Doppler's real stage12 checkpoint2 end
room have the same boundary problem. Their original locked bounds
`1600/0700/0700/1600/1600` and `0a00/0400/0400/0a00/0a00` join Press and Sigma
in one explicit descriptor table. All four use the existing wall compositor;
ordinary corridors and native framing retain their previous policies.

God/Doppler native256, hybrid342 and interpreter684 provide 48 paired samples
with identical native-view pixels, full WRAM and all 64 KiB VRAM. Native entire
output is identical. Byte420, factory360, Kaiser120, hub120, rematch entry120
and Press120 preserve complete output/WRAM against the preceding build.
The actual post-Doppler clear also reaches its password screen after 900
frames with identical complete output/WRAM. Both current handoff states replay
120 frames identically after save/load.

Separate assisted combat runs now qualify live32HP Godkarmachine, Press
Disposer and Doppler, actual projectile defeats after HP assistance, and
native fortress counters `0->1` or `2->3`. God needs20 original dialogue
edges to transform; Doppler needs54 before combat and54 after. Press's
trunk and head are separate phases; aiming24 pixels above the parent origin
finishes the head. These mechanics runs retain the exact preceding28f17ee
binary identity; the new33949e75 executable adds only the presentation cases
and has the paired checks above. Private evidence lists all bypasses.

Checkpoint2/3 selection bypasses Lab 1's original branch fork. Position
assists bypass ordinary corridor obstacles before Doppler's real door.
The earlier fixture did not qualify normal fortress next-stage selection.
The clean native-context follow-up below supersedes that setup limitation;
no speculative marker or progression fix was adopted.

### Native fortress progression follow-up (2026-10-27)

Controller-only password entry and original stage selection now provide a
clean baseline with earlier progress and equipment initialized by the guest.
The reference password reaches normal stage select in hybrid342 and pure
native256. An independent Snes9x replay reaches the same screen with identical
64 KiB VRAM; an unchanged menu mode is normal while awaiting input.

On current33949e75, the original checkpoint3 loader preserves this native
context and bypasses only Lab1 traversal/branch selection. Press starts at
32 HP; the main/trunk survive idle at assisted1HP, then real shots destroy
both. Original stage-clear, counter0->1, password, stage select and next
laboratory selection pass without progression writes.

The native selection chooses stage14's Vile branch. Checkpoint2 bypasses
earlier traversal; right input then exercises the first door, descent, second
door and armor introduction. Actor99's real defeat creates on-foot Vile
actor97/sub2 at32HP. Each survives an idle check at1HP before real shots
produce its native defeat. Twenty-three B edges finish the postfight script;
the native stage-clear reaches counter1->2 and its password screen. Early
private captures provisionally named this encounter Volt; the observed second
form and corrected aggregate identify the Vile branch. Those filenames are
historical evidence, not alternate boss coverage.

The next native selection enters stage12/counter2. The all-clear hub fixture
also now opens its real right door into the corridor. Original39:A2D0 caches
`1FDA==FF` into door prerequisite `1F5F` bit0 at initialization: supplying FF
after initialization leaves the cached gate locked. Reinitializing with the
prerequisite already present naturally clears it. Player-only flight reaches
the upper platform and native gravity lands atY1119 before real right input
opens the door. The eight earlier rematch results are an explicit prerequisite
bypass here, separate from the retained ca0 actual-eight-rematch proof.
The optional human hub check is retired; owner notes remain unchanged.

Position assists then bypass ordinary corridor obstacles before Doppler's
original door. Native32HP, causal1HP idle check, real defeat and54 postfight
B edges reach counter2->3. Password and normal stage selection now enter
final stage13 without additional stage/counter/main-weapon writes. Its original
Zero approach/dialogue also returns control after18 B edges.

This adds current-build combat and normal progression evidence without
claiming the skipped approaches or the other Lab2 branch. Kaiser/escape/ending
and built-runtime/other-consumer qualification remain open.

The final title code8a71 also passes paired checks on combined engine2cb versus
ownera4: Byte420, factory360, God/Doppler/Press/Kaiser120 each, and the corrected
hub exit300 at342, native256 and interpreter684. All144 samples preserve the
complete presented image and128KiB WRAM; all three hub cases cross the door.
Compiler definitions confirm both trace builds use FULL audio history despite
the earlier cache-based SMALL/FULL inference. Performance remains a separate
measurement gate; these functional comparisons establish no speed claim.

Current combined-build Press save/load reproduces complete output and WRAM.
Six alternating paired360-frame live-Press samples match interpreter342,
software/dummy output, FULL trace history and optional coverage recordingOFF.
Time changes range from10.3% lower to22.0% higher (median3.7% lower): mixed
results within host variation are neutral/inconclusive. Separate earlier
coverageON samples are retained and are not pooled. X2 has its own workload
and results. Representative other-consumer/startup/memory qualification is
still open; no aggregate optimization or release-speed claim is made.

### Other Lab2 route and room boundaries (2026-10-27)

The other native Lab2 selection now has separate coverage. The earlier Vile
prerequisite is supplied before original stage selection; checkpoint2 bypasses
the approach. Native256 and interpreter684 follow the same780-frame original
door/descent/introduction timeline, create actor4A at32HP, and advance the native
fortress counter1->2 after an assisted1HP idle check and a real shot. No current
success, camera, control or other boss-part HP is written.

This route revealed a512px room whose expanded margins exposed a neighboring
corridor, black void and a wrapped BG2 boss copy. BG1 now reflects only beyond
the authored room. A shared PPU clip band contains BG2 using BG1 actual scroll,
retaining the real body in genuine offscreen room columns. It preserves native
camera-shake pixels and clears with the normal frame-local presentation policy.

Final758756b9 passes72 paired samples at256/342/684, centered and at both camera
edges: every native/authored-room pixel,128KiB WRAM and64KiB VRAM matches339.
The900-frame entrance probe compares rendered output and observes no correction
while the entrance door is visible. The full-health entrance, real-shot clear
and current-build save/load pass. Eight unrelated scenes preserve complete
output and WRAM over112 paired samples, including accepted Byte/factory,
God/Press/Doppler/Kaiser, the corrected hub exit and the other Vile route.
These checks do not qualify skipped traversal, the Kaiser finishing hit,
escape/ending, independent audio or the remaining human visuals.

The first correct clip had a measurable compositing cost at the widest camera
edge, so it was held from the owner package. Engine d0968cc reuses existing
hardware-window spans to draw contiguous room clips directly; mosaic, disjoint
rooms and source anchors retain the original compositor. Final2833b6b5 preserves
the entire presented picture and WRAM through72 pan samples at256/342/684 and
repeats the original full-health entrance, real-shot clear and native counter
transition. Six matched684 edge pairs all take less time than first-correct758;
X2 default-path results are small and mixed within host variation, hence
neutral/inconclusive. These are separate per-game protocol measurements,
not release benchmarks. Capture/Pause and the current-build Kaiser restart
also pass. Raw samples and identities are tracked under optimization campaign
beads-2wy8.1 and owning engine issue beads-8wg.2.151; broader consumer/startup/
memory qualification and the human final-room checks remain open.

### Refreshed engine base (2026-10-27)

Reviewed engine0095609 is rebased onto main5ac4b98 and has the same tracked
tree as compiled combinede772aca and integrationa99e2fc. The title now pins
this engine. New buildc3d20fc6 passes156 paired full-picture/WRAM samples
against preserved95ad, including Byte/factory, the fortress rooms, hub exits
and684px Lab2 pans. Its sampled Press PCM and full SPC state match byte for
byte in default and pure execution. Current save/load, Pause/Capture and the
refreshed live1HP Kaiser setup replay exactly.

The package selects native APU time and SLICE8 CRC with the standard title
renderer. Six paired240-frame samples in each execution mode are small and
mixed within host variation: neutral/inconclusive, with no broader speed claim.
Warm startup and memory costs are effectively unchanged. Earlier combat and
renderer proofs keep their original binary identities. The four pending
human checks are unchanged; accepted Byte/doors/factory checks are preserved.
Kaiser finishing hit/escape/ending, skipped content/independent audio and
broader-consumer follow-up remain open. Exact data is in `new_main_qualification`.

### Native final-room trigger qualification (2026-10-27)

On current c3d20fc6, the clean native fortress route now reaches the
original upper-room cue, full32HP Sigma introduction, real projectile
defeat and full32HP Kaiser introduction/control return. Native256/default
and684/interpreter repeats pass the same cues; introductions take22 and36
B pulses respectively. Combat hit timing is not asserted equal.

Actual ladder grabs/climbs and connector traversal work. Position-only
alignment and flight bypass ordinary shaft/wall-jump traversal; main
Sigma HP1 survives idle before the real hit. No camera, AI, progression
or result flags are written. The hit changes a mutable parameter byte;
stable actor-slot/id proves HP0/primary04 and corrects a false helper
failure. Original raw output and excluded setup experiments are retained.

Current Kaiser save/load and Pause/Capture/restart are exact in full
pixels and128KiB WRAM. Captures now include actual generated-CPU
registers. Kaiser finishing hit, escape and endings remain unverified;
the existing optional human segment remains available. See
`native_final_qualification` in the audit JSON for assists and evidence.

### Fixed epilogue and assisted aftermath (2026-10-27)

The opening ending scene repeats its sun and factory at wide widths.
Code87f198e retains the authored256px composition only for ending
mode6/phase0, reusing the existing frame-width hook. The scrolling
cast returns to adaptive widescreen in phase2. Gameplay is unchanged.

New3435c017 at342/default and684/interpreter preserves every native
epilogue pixel and128KiB WRAM across20 samples/6000frames per mode.
The cast restores full width and identical whole output. Actual clear
entry, four current-state replays, the portable debug-host regression,
Byte/factory/Kaiser/hub samples and current Kaiser Save/Pause/Capture
also pass. Source generator/core/package settings are unchanged.

The preceding c3d2 audit now exercises original virus creation, rising
lava/camera, contact/recovery, upper corridor, Zero rescue/dialogue and
counter3->5/ending mode6 at342 and684. A further24000 no-input/no-RAM
frames at342 reach Thanks for playing / Presented by CAPCOM. Kaiser
HP0 deliberately bypasses the unverified finishing hit. Position assists
bypass wall-jump traversal and correct an entry below a raised ledge;
no camera, AI, progression, escape or ending flags are written. The
same low prepared entry stalls natively too, so it is excluded as a
demonstrated widescreen defect. Real finishing hit, uninterrupted
physical escape and alternate rescue ending remain unverified. Exact
binary identities and caveats are in `epilogue_and_aftermath_followup`.

The subsequent current3435 audit also qualifies the alternate Doppler rescue
at native256/default and684/interpreter. An earlier Zero-unavailable prerequisite
is supplied before rescue-script allocation; Kaiser defeat and ordinary escape
traversal remain bypassed. Moving above the ledge before the original upper cue
lets the native walk finish without a post-control position correction. Both
runs reach original counter3->5, stage clear and ending mode6, with exact
current-build ending save/load. No camera, AI, current result or ending flags
are written. This closes the assisted alternate-rescue check, while the real
Kaiser finishing hit, ordinary physical escape, earlier Zero-loss/fresh-stage
loading and remaining content/human checks retain their separate limits.
See `alternate_rescue_followup` for the exact prerequisite and raw evidence.

### Maoh object canvas and shared presentation follow-up (2026-10-27)

Maoh uses BG1 for its roof, BG2 for a moving boss canvas and BG3 for the city.
The scoped stage0 policy keeps live boss tiles, hides unrelated circular-map
cells, widens the genuine skyline and repeats only32px world-edge strips
beyond the combat roof. Real in-room damage remains; outer margins no longer
reflect another door, boss or destroyed floor. Codee5e5f15 pins engine05786e5.

Build10994be1 passes fresh opening, original full32HP introduction, assisted
real projectile defeat, native clear/stage-select and controller next-stage
load at256/default,342/default and684/interpreter. All93 sampled native
pictures/WRAM/VRAM/OAM agree with3435; six current live/selection replays
match exactly. Connecting travel and HP1/aiming assists remain explicit.

The shared X2/X3 package passes188 full-output/128KiB WRAM regression
samples, non-silent PCM/full SPC/APU comparisons, and all four prepared
states’ save/load and Pause/Capture/restart checks. Accepted Byte/doors/
factory observations remain closed. Per-game timing samples are small
and mixed: neutral/inconclusive, without a speed claim. Actual trace is
FULL in these diagnostic builds despite the SMALL cache request.
Remaining human/content/branch/audio gaps are unchanged; see
`maoh_object_canvas_followup` for precise identities and bypasses.

### Actual earlier Zero loss (2026-10-27)

Build10994be1 passes the real Zero-loss prerequisite at256/default,
342/default and684/interpreter. The original communication menu/Start
selects playable28HP Zero. Only HP1 is supplied before idle survival;
actual Right/jump contact causes HP0/death. The original retreat sets
1FB2.40 and restores16HP X with normal control. All three current-build
return states replay with exact full output and128KiB WRAM. No position,
camera, AI, character, current result or unavailable-bit writes are used.

A saved ready menu bypasses upstream travel. Four contact pulses and30
retreat/dialogue pulses occur in each width case. Earlier too-early Start
inputs did not wait for the communication-page animation and are excluded
as setup errors; they do not establish a widescreen menu defect.

The naturally earned bit also survives a fresh original final-stage
checkpoint0 loader after a stage/counter teleport. Propagating it through
the complete alternate rescue remains open, as do the actual Kaiser
finishing hit and ordinary escape. Prior alternate-rescue evidence with
a supplied prerequisite keeps its original3435 binary and assists.

### Final-stage entrance camera (2026-10-27)

Buildd897f993 reuses the parked X1 view-anchor mechanism in engine819e2e1.
At the demonstrated final-stage entrance, the extra columns go inside the
room, removing the unused strip to its left. Native camera/scroll/OAM and
sequence state are unchanged. HUD and dialogue retain screen coordinates.
This is scoped to the measured chamber and internal widths up to512;684
retains the previous centred policy because its one-sided ordinary OBJ span
exceeds512. Other rooms and wider anchoring remain separate work.

Twenty-five controller-only movement samples retain the complete native
picture outside the existing HUD footprints and exact128KiB WRAM. The
observed image origin moves from0 to43 as X leaves the wall. Kaiser, escape,
Byte and factory retain whole output/WRAM in68 samples. Native256 and
684/interpreter remain unchanged in16 samples. Current-build entry/Kaiser
save/load and Pause/Capture/restart checks pass; seven PPU fixtures pass.
One initial unsettled Byte picture is retained as an inconclusive observer
sample; the independent settled replay passes every sample.

The unmodified Snes9x reference confirms the entrance composition and the
original shaft pipe walls. Static shaft-art inspection uses explicit camera
positioning and does not certify an independent original escape sequence.
The walls above the fire are authored background art. The owner separately
accepts the absence of a duplicate Kaiser head. Actual finishing-hit,
ordinary escape and other pending audit gates retain their own status.

The owner subsequently confirms that a normal charged shot finished the live
1HP Kaiser in build10994. This actual finishing-hit gate is now passed, along
with the no-duplicate-head check. No repeat combat handoff is required; the
new chamber camera visual and the other independent audit gaps remain open.

### Opening capture and rescue matrix (2026-10-27)

Buildd897f993 passes the fresh controller-only opening, original Mac
capture/Zero control, actual Mac and restraint finishing hits, and original
X rescue/control at256/default,342/default and684/interpreter. Capture takes
39/39/40 dialogue pulses and rescue16 at each width. All six current-build
Zero/X save-load replays preserve full output and128KiB WRAM. Original Mac
is32HP and restraints5HP before the separate HP1/idle/real-shot assists.

Connecting travel uses saved original-route approaches. Player aiming/health
and live target HP1 assists are explicit; capture/rescue scripts run only on
controller edges. No camera, AI, character, result or progression flags are
supplied. This is sequence qualification, not complete natural stage travel
or cross-width byte-identical timing. The initial retirement/flat-ground
assertion mistakes are retained separately from product results.

The owner also accepts the16:9 entrance camera anchor. The focused human
handoff is complete; do not repeat camera, Kaiser, doors, Byte, factory or X2.

### Earned Zero-loss finale and current Bit follow-up (2026-10-27)

Build d897f993 passes the fresh final-stage route at 256/default, 342/default
and 684/interpreter using the earlier naturally earned Zero-loss fixtures.
The original loader preserves that result; real Sigma damage, full Kaiser
introduction, actual virus damage, original top-camera arrival, controller
wall jumps over the raised ledge, alternate rescue, native counter 3->5/clear
and ending all execute. All three ending save/load replays preserve the
complete output and 128 KiB WRAM. Cross-width timing equality is not asserted.

Earlier fortress eligibility and connecting travel/shaft ascent are assisted.
Kaiser HP=0 explicitly bypasses the finishing hit, which the owner has already
qualified separately. After selecting earlier fortress eligibility, no
unavailable, camera, AI, current encounter outcome or later progression flags
are supplied; no writes occur after the native clear.
The earlier actual-loss fixture retains its 10994 identity. This closes fresh
loss-result propagation, not ordinary whole-stage traversal.

Current Bit also passes original floor contact at Y=679 (feet=703, floor=704,
camera Y=559), full 32 HP introduction, a real assisted Buster finishing hit,
native departure/result 01, right-door crossing and normal control return.
The before-room floor-only owner setup includes original resident door art.
The owner subsequently accepts Bit's corrected floor appearance. All prepared
human checks are complete; accepted scenes do not need repeating.

Very-wide entrance anchoring above 512, uncovered stage/sublevel/branch/exit
travel, independent original-game music/SFX and broader shared-engine consumer
qualification remain open. Machine-readable current_status is authoritative;
older identity-specific remaining lists describe their historical milestone.

### Producer coordinates for very-wide entrance views (2026-10-27)

The private candidate now anchors the final-stage entrance at684 and800px.
It observes full signed X at the original CX4 microcode write02:80E6 and
binds it to the actual OAM XY upload. An exact microcode signature guards
the policy; guest camera, OAM, collision, AI and progression stay untouched.
Pending and uploaded positions travel through the title save/rollback extension.
The shared desktop host also retains the16-bit budget: casting272 to eight
bits previously collapsed800px output to288px.

All nine PPU fixtures and the synthetic original-instruction observer pass.
Forty-six runtime samples retain full WRAM/VRAM/OAM equality;256/342px full
images match the accepted build in entry, Byte, factory and Kaiser workloads.
At684/800 the corresponding native world pixels below the HUD match; actual
800px output replaces the baseline288px wrap. Current-build full-image/WRAM
save/load replays pass at342,684 and800. Matched342px runtime samples are
inconclusive on the shared host (median+3.14%, range-2.64%..+5.05%);
this is not a speed claim. Raw evidence is `cx4-coordinate-qualification.json`
in the retained audit workspace.

The current owner build remainsd897 and all prepared human checks are accepted.
Full very-wide chamber travel/transitions, uncovered
content/audio and broader engine qualification remain before release.

Image inspection also exposed a moving61px BG2 strip outside its slower
parallax canvas. Very-wide anchoring now stays within both BG layers. The
repeated46samples and three current-state replays pass;<=512 placement
retains its accepted behavior. No new owner check is prepared.
