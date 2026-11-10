# X3 co-op implementation and qualification

Development branch: `feat/x3-coop`, based on X3 master `11fd733`.
Build reference: engine main `e9c20732`, UI master `cf6315a`.
This is an incomplete development implementation, not a co-op release.

## Implemented components

- X3-specific personal body, auxiliary and projectile context windows, with
  the Ride Armor and enemy/item pools excluded from projection.
- Native configurable-action mapping for both engine input seats.
- Personal previous-action history is retained outside NMI's live body mapping,
  so the second mapping pass cannot erase P1's new jump/fire edges.
- Separate alive, voluntarily absent and fallen states. The lifecycle API
  refuses fallen rejoin and partial-team retry, and transfers the anchor to
  the survivor. Development runtime handoff intercepts the native death state
  before its global freeze; runtime death/scene wiring is not yet qualified.
- Balanced native controller replay and post-enemy terrain replay. Enrollment
  is restricted to the native gameplay scheduler caller, excluding scripted
  controller calls. Both passes preserve elapsed guest time.
- Development Zero body presentation reads native layouts, CHR and palette
  lists from the validated cartridge. It suppresses the partner's body DMA
  replacement of X's pages. Animation inheritance, effects and visual fidelity
  still require qualification.
- Actor/pass snapshot extensions and a transactional actor decoder, with
  snapshot preflight before the common loader changes guest memory.

The actor experiment requires `SNESRECOMP_X3_COOP_PROTOTYPE=1`; it is not
advertised as a usable launcher mod. X/Zero roster reversal, the restriction
option, runtime survivor lifecycle, shared interactions, camera modes, paired
menus, HUDs, assists and online sessions remain implementation work.

## QA burndown

**Accepted areas: 0/15.** Component checks and a successful build are not
campaign or co-op gameplay acceptance.

| Area | Status | Acceptance |
| --- | --- | --- |
| Individual death | Stage-1 contact deaths pass for X/P1 and Zero/P2; other cases open | Either survivor retains play and progression, with no global fade, blanking or stale freeze. |
| Team wipe | Zero-then-X stage-1 route passes; other cases open | Simultaneous/staggered deaths produce one native retry and the intended life cost. |
| Fallen persistence | Component checks only | No checkpoint, stage exit, voluntary rejoin or menu revives a fallen player before team wipe. |
| Separated spawning | Not implemented | Either leader activates native enemies, supports and one-shot events without duplicate visits or cross-gap activation. |
| Supports/hazards | Shared rider contracts and stage5 class26 case pass; broader families open | Both riders retain support, momentum and crush behavior while each platform advances once. |
| Enemy interactions | Native contact and class08 targeting probes pass; broader AI/captures open | Targeting, contact, capture and release recognize either actor and clean up victims. |
| Combat ownership | Partner native buster update/damage probes pass; other attacks open | Concurrent attacks retain native damage/cooldowns and clear on lifecycle exits. |
| Doors/scripts | Not implemented | Either/both door policies and native scripts complete with either survivor. |
| Campaign branches | Not qualified | Intro, eight stages, optional encounters, fortress, refights and endings complete with either roster. |
| Menus/resources | Not implemented | Paired navigation, shared tank spending, healing and resume cannot leak inventory or input. |
| Visuals | Not qualified | Both views retain correct art, lighting, HUDs, Cx4 effects, margins and transitions. Unchanged X3 reaches a rendered scene; early black captures were startup transients. |
| Rollback/state | Component and stock-runtime checks | Complete initialized co-op snapshots, mid-pass restore, rewind and input replay agree, and malformed state is rejected. |
| Netplay | Not implemented | Two processes and LAN qualify shared/independent views, rollback, menus, deaths and disconnects. |
| Performance/stock regression | Not qualified | Built co-op runtime meets frame budget and disabling co-op preserves stock behavior. |
| Package/owner handoff | Not ready | Verified default settings, normal-window/audio handoff and one current ZIP in the delivery folder. |

## Current evidence and next gate

The standalone actor test passes projection-boundary isolation, both death
orders, survivor anchoring, fallen rejoin rejection, withdrawal guards, retry,
native action remapping and transactional decoding. The stock ROM-backed
host suite passes immediate restore, deterministic 30-frame replay, save-browser
behavior and rewind restoration with `SNESRECOMP_REWIND_INTERVAL=1`, the
  interval assumed by its six-step rewind check. An initialized two-actor
  fixture also passes immediate restore and deterministic partner-input replay
  after the input-history correction; none of these checks certifies a campaign
  or all rollback boundaries.

The unchanged-title comparison reaches a rendered scene on the same engine.
The stage-1 development route enrolled both actors and produced captures of
native Zero moving right while X's position stayed fixed, followed by both
actors resting on the floor. This is a narrow movement/presentation observation,
not stage-wide collision or visual acceptance. A low-HP natural-contact route
then reproduced and repaired two death hazards: the native death state freezing
shared pools, and the stage task queueing a team fade before the next controller
handoff. After repair, Zero retains control, the stage stays at gameplay phase
4, shared freeze bytes remain clear and brightness stays at 15. Native palette
ownership also transfers to Zero. The HUD still needs its survivor/character
layout work. The other death cases, team retry and progression remain open.

`tests/scenarios/coop_survivor.script` reproduces this route in a disposable
profile. It writes native initialized-co-op slot 11 (`save11.sav`) and survivor
slot 12 (the host's `savequick.sav`).
`mmx_state_tests` can additionally qualify those states using absolute paths in
`X3_COOP_TEST_STATE` and `X3_COOP_SURVIVOR_STATE`, with
`SNESRECOMP_REWIND_INTERVAL=1`. Never use player saves for these fixtures.
The survivor check passes another 60 frames with gameplay phase 4, brightness
15, no shared pool freeze, and X still fallen. The earlier route that stopped
at a step without killing X is not death-validation evidence.
Do not deliver or
replace the existing X3 archive with this unqualified prototype.

### Native contacts and owned updates

The native hostile body and player-shot scans now test the partner through
balanced guest continuations. Solid contacts check both bodies; a hostile shot
consumed by its first hit stops there. A player-shot scan retries the partner
only after its actual miss return, preserving reflection/immunity results.
Each player's native shot, armor and auxiliary pools receive one update.
The common world/enemy scheduler still runs once. Continuations and their
scratch/results participate in snapshots; invalid active PCs, pass combinations
and projected-seat identities are refused before loading the machine.

Cartridge-backed geometry probes pass P2-only enemy damage, overlapping solid
damage, hostile-shot single consumption, partner buster movement and partner
buster damage to a shared enemy. These isolated probes do not qualify enemy
AI, spawning, authored platforms or native graphics for those projectiles.
The generated-boundary check covers all ten routed entries and all four
register-width variants. Initialized immediate restore and input replay still
pass, together with the stock state/rewind checks.

`tests/scenarios/coop_p2_survivor.script` gives Zero low health before native
enrollment, restores X's health, then moves both into real stage-1 enemy
contact. Zero is fallen at x719 while X moves to x761. Phase remains 04/04,
brightness 15 and all seven shared freeze bytes remain zero. The captured
survivor image was inspected; another 60 frames retain the same life states.
`coop_team_retry.script` resumes that disposable slot10 and gives the remaining
X one health before further contact. Native retry reenrolls both with 16 HP;
the common life count changes from two to one. No death/retry result flags are
written by either setup. Pits, simultaneous wipes, boss retries, reversed
rosters and continuation restores remain unqualified.

Implementation-stage evidence under `build-coop/qa`: `p2-death-*`,
`team-retry-*`, and slots10/9; ROM-backed results in
`build-coop/qa-state/native-contact-result.log`. Game binary SHA256:
`9edbcf2d35820f313d743d36ea471cc3184c3ac0efe03867552b77666d30badf`.
Automated profiles use dummy audio/video, software rendering, native aspect
and the prototype switch. These narrow results support D01/D02/D08/D09 and
T03; they do not close their full roster/stage coverage or any of the 15
rollup areas. The planned post-implementation suite remains 180 touchpoints.

### Late draw ownership and charge palettes

Native owned draw requests now retain their seat in the pointer's unused high
bit across all six original queues. The actual late descriptor routine borrows
that seat's pool, submits it once, and restores the anchor at its balanced RTS.
The ROM-backed probe verifies the descriptor's real X/Y, guest-stack balance
and anchor restoration. Fresh project regeneration preserves all 27 emitted
hooked variants and produces byte-identical generated trees on a second pass.

The first simultaneous-buster captures showed both projectiles, then exposed
a separate fault: native Zero charge palette writes recolored X red. Each
actor now retains native OBJ palettes80/90 through projection and snapshots.
Rendering uses the anchor's ramps and restores emulated CGRAM afterward;
the additional Zero body uses his own live charge ramps. A 180-frame native
charge probe observes changing Zero colors while X's 64 palette bytes remain
unchanged. Exact CGRAM restoration and initialized replay pass. Inspected
`zero-charge-held` and `zero-charge-release` captures keep X blue while Zero
charges and fires. The software body also follows the non-anchor seat rather
than drawing a second copy of the surviving native anchor.

The new disposable fixture format is X3C3; regenerate older development
snapshots. `tests/scenarios/coop_buster_palette.script` resumes slot11 for
the firing/charge captures. Current ROM results are in
`build-coop/qa-state/native-palette-result.log`; refreshed P2 death and retry
logs use the `palette-coop_*` prefix. Both survivor directions and the tested
staggered retry still pass. These are implementation-stage cases under C01/C02,
E11 and visual ownership; private projectile CHR/palette combinations, visible
NMI alignment, reversed rosters, scene effects and full visual QA remain open.
The complete regenerated build passes the same ROM-backed cases in
`build-coop/qa-state/native-regenerated-result.log` (exit0), plus both CTests.
Its game SHA256 is
`e38d59b9e266a6f7d6c799e22d3de50cba6ef0299e6f4d34c2d538b4d3d5df89`.

### Native targeting and fatal-contact continuations

Living native enemies/hostile shots can borrow the nearest living body for
their existing normal/frozen update, then restore the canonical anchor. The
world actor runs once; camera ownership does not change during the borrow.
Common native facing/proximity coordinate loads also recognize the nearest
actor during initialization. Contact continuations retain their actual first
seat, allowing either target order without switching the world anchor.
Fatal contacts now settle before returning to the enemy/boss continuation,
which sees a surviving body's HP instead of prematurely selecting death logic.

The real stage-1 class08/variant1 fixture retains two living actors. With X
160 pixels left and Zero 32 pixels right, its actual facing/proximity branch
selects Zero. All 15 native enemy records match a single-seat reference update
against that same body, including a timer expiring once; its stack and canonical
ownership are restored. This is a narrow E04/E05 case, not all enemy behavior.
An inspected gameplay capture also shows that authored enemy beside active
actors; a blinking hurt sprite is checked against its native visibility flag.
Fatal-contact probes verify immediate survivor HP for either death order.
The existing contacts, palettes, replay, both survivor directions and staggered
retry still pass in `build-coop/qa-state/native-target-result.log` (exit0).
Use `tests/scenarios/coop_target.script` and optional `X3_COOP_TARGET_STATE`
with its disposable slot8 fixture. This checkpoint's binary SHA256 is
`444a65fca7e5e7c67818b9b950c54b6044ce7956dd6c67919e98c548f56437d5`.
Captors, scene admission, all platform types and separate-region activation
still need their own native ownership contracts and authored checks.

### Shared supports and an authored moving-platform case

Native BFAE contact and C07F carry now retain separate rider bits for the 15
enemy slots and 16 item/script slots. Both calls keep their actual entry
registers/scratch and balanced guest returns. A carry runs only for that
support's living riders; one actor leaving does not clear the other rider.
Native initialization clears histories on slot reuse. Continuations and
rider histories are serialized, and invalid rider bits fail preflight without
changing the machine. Shared actors still update once; owned weapon supports,
vehicles, specialized crush and custom-height helpers remain separate work.

ROM-backed isolated checks pass equal carry for two riders, independent
movement on two supports, releasing one rider, current-build history restore
and transactional malformed-history rejection. The geometry uses the actual
class37 table 06:C5D4, rather than an assumed player-sized support.

The authored moving case uses native stage5 event 3C:E12C: enemy class26,
variant2 at (1224,1464). A QA teleport and the original event-column spawner
prepare its real room; this explicitly bypasses camera admission, normal
activation frontiers and background-stream preparation. It does not set the
support's movement or rider result flags. With normal native updates, X and
Zero land and ride at their own standing heights. Over 45 frames, all 64 support
bytes match a single-seat native reference, including movement 1464→1434;
both runs advance the world 45 times. X can then jump off while Zero keeps
riding. A separate P2-first setup starts/moves the support while X remains
outside its native contact width; its rider mask contains only Zero.

The initial class37 candidate in this stage is stationary in the tested native
state. It is not used as evidence of automatic movement. Inspected class26
captures show both riders and the P2-only setup; background content after the
QA teleport is not accepted as streamed-world visual coverage.
Use `tests/scenarios/coop_platform.script` for disposable entry slot6 and
`X3_COOP_PLATFORM_STATE` for the native test. Optional output paths
`X3_COOP_PLATFORM_OUTPUT`/`X3_COOP_PLATFORM_P2_OUTPUT` save both-rider and
P2-only visual fixtures (local slots5/4). Current combined results are in
`build-coop/qa-state/native-combined-result.log`, with source build identity
recorded in the private issue. These narrow checks support P01/P02/P04 and
R01; all 15 rollups and the full 180-point campaign suite remain open.
The standard regeneration recipe passes its complete-tree byte comparison
after these hooks, including the refreshed generated build identity.
The regenerated build passes all current ROM-backed cases and both CTests in
`build-coop/qa-state/native-combined-regenerated-result.log` (exit0). Its game
SHA256 is `35f04da53fb2b0e0877e9a77d881efa3f2873fff427d2112aba50843f5db5121`.

### Independent camera and activation implementation checkpoint

`x3_coop_view` now tracks each living player's native follow position, current
limits and authored region targets. It reads only camera-limit commands from
X3's03:DD76 data; shared script commands still execute once in the cartridge.
The parser handles all15 stage lists, including stage14's stage11 alias and
non-monotonic region records. Camera follow and limit panning match a native
single-seat reference in the qualified stage1 setup.

The development independent profile (`SNESRECOMP_X3_COOP_INDEPENDENT=1`, with
the existing prototype switch) activates the union of the two native frontier
regions through the original event allocator. It excludes the gap and retains
shared actors visible to either living view. Admission uses the original
frontier columns: an early allocation at the wider retirement margin otherwise
retires the actor before its first visible update and holds its visit history.
Resident parallax-controller admission remains with the native anchor pending
private presentation. Owned auxiliary/projectile culling uses its owner's view.

The original camera body-bound/pit tail runs for the partner with private
camera limits. It receives the unclamped follow result so native pit damage
can still occur. Either actor's pit death leaves the other alive, and an
anchor death adopts the survivor's camera. Camera/visit state and interrupted
spawn/bound continuations are included in current-build snapshots. Loading an
enrolled state before the first reset frame now enables its actor schedule.

`tests/scenarios/coop_independent.script` uses a normal P2 dash-jump approach
after stage entry. X remains at384/camera256; Zero reaches761 and admits the
actual class08 enemy outside X's retirement rectangle. Thirty further console
frames advance the native world counter30 times; full machine replay matches.
Isolated native checks also cover gap exclusion, no duplicate allocation,
frontier admission, interrupted allocation-codec restore, stage aliases,
private camera panning and both pit handoffs. The existing support/contact,
palette, survivor, retry, stock snapshot and rewind checks still pass together
in `build-coop/qa-state/native-view-combined-result.log` (exit0); both CTests
pass. Standard strict regeneration reports complete byte-identical trees.
The qualified game SHA256 is
`159b7454eb9d88403acb1bfbe6b6078eee98acec2f430e31c478b6cf081058ad`.

Inspected desktop captures show a normal anchor view and the restored Zero
jump. This implements simulation camera/activation support, not separate view
presentation. Private background/resource rendering, split/selected displays,
HUD/UI, online profile negotiation and full campaign qualification remain open.
These narrow results support A01/A06/A07/A08/A09, D14 and R01/R02; none closes
an entire rollup. All15 broad areas and the full180-point final suite remain open.

### Native display handoff checkpoint

Private actor metadata now follows the actual descriptor/Cx4/OAM pipeline.
03:8DD7 freezes the pair and native list camera, actual03:8F70 submissions
retain native draw records and ownership, and03:8DF0 completes the list only
after the original sprite program returns. The DMA observer advances visible
metadata when the original Cx4 OAM is uploaded. Merely starting an NMI or
updating body physics cannot advance the displayed private actor.

The renderer uses the visible body, palette and list camera. Queued, completed
and visible records use an explicit snapshot codec, including native draw
order. Real-ROM checks observe the pipeline and primary ownership; isolated
handoff checks hold the prior visible state through queue/completion, advance
the matching body/camera/list on upload, preserve immediate state restore and
reject malformed presentation metadata before machine mutation. These tests
do not certify an authored heavy-load/lag sequence or separate-view rendering.

All current combined native tests pass in
`build-coop/qa-state/native-presentation-combined-result.log` (exit0), including
independent-route replay,45-frame authored support comparison, charge palette
isolation, both natural-contact survivors and native whole-team retry. Both
CTests pass; native hook entry routing did not change. Refreshed enrollment
and charge/fire captures were inspected. The qualified game SHA256 is
`5d1b6f8c57fb1b040a56288df57c29b70ab8dfa0455a497d18b749daf3986195`.
Full view presentation, lifecycle/menu integration and the180-point final
acceptance suite remain open; no rollup is closed by this checkpoint.

### Selected and split presentation checkpoint

Development `SNESRECOMP_X3_COOP_VIEW=p1|p2|split` now selects an independent
field or a512x224 two-field host display. Private scanout uses native world
tile/layout data, authored resource controllers, completed draw submissions,
native X/Zero body art and each selected actor's health meter. The original
PPU/HDMA state advances once; neither field reruns the shared simulation.
Native menus and unsupported scenes retain their original scanout.

Body CHR follows the original04:BCA2 upload path and the private controller's
equivalent upload. Live, queued, completed and visible atlases are serialized
with their matching body poses. This matters for actual ROM poses whose upload
list is empty or partial and therefore inherits previous tiles. A restored
Zero pose2 reproduces its pixels after another pose replaced the live atlas.
Current presentation codec isX3P2; disposable fixtures were regenerated.

The combined ROM checks pass in
`build-coop/qa-state/native-display-atlas-result.log` (exit0). The stage1 entry
and authored enemy-contact P1 fields match all native pixels, including their
health HUD. Separated selected fields differ, while each split half exactly
matches its selected field. Drawing leaves WRAM/CPU time unchanged;10-frame
full snapshots match across anchor/selected/split presentation choices using
one independent simulation profile. Both CTests and strict full-tree regeneration
pass. The host's composed split capture and enemy/contact fields were inspected.
Qualified game SHA256:
`0ce809186d74a8de278d601e5e39f13313a66e47c272e2299b4175b405155ecb`.

These are bounded implementation checks. Private general-resource phase1F18
assets, authored effects/lighting/forced scenes, X weapon HUD, shared HUD layout
choices, paired menus and public settings/netplay remain open. Zero's prototype
starting health still inherits X's; native Zero28HP and per-actor maximum/roster
ownership must be completed. No final QA rollup or campaign suite is accepted
by this checkpoint; the full180-point post-implementation suite remains open.

### Native health limits and reversed roster checkpoint

Enrollment preserves the native X body/resources in the seat assigned X and
seeds the other seat with native Zero28HP. The original04:8DEE stock exchange
provides Zero's maximum. Maximum HP is now personal projected state, including
current/queued/completed/visible snapshots (X3C4/X3P3), rather than one shared
native1FD2 value. Private HUDs use the matching visible actor's maximum.
Development `SNESRECOMP_X3_COOP_P1=zero` reverses the roster at enrollment;
restoring an enrolled state retains that roster through the next native retry.

The native04:AA09 health regeneration routine reaches each actor's own maximum
and does not exceed it. A zero maximum in saved actor data fails transactional
preflight. Fresh native fixtures pass both natural-contact death directions,
the staggered wipe with one shared life debit, and retry HP16/28. The P2-death
route now loads an explicit private Zero1HP precondition: run native tests with
`X3_COOP_LOW_P2_OUTPUT` pointing at disposable slot2 before running that script.
No death, survivor or retry result flags are supplied by the preparation.

Use `tests/scenarios/coop_reversed.script` for disposable slot1 and
`X3_COOP_REVERSED_STATE` for its ROM checks. Native movement, owned Zero combat
and30-frame replay pass with either seat controlling its assigned character.
The reversed X primary view matches all native pixels; the real host split
capture shows the native Z/X badges and their28/16HP meters in the right fields.
Evidence: `native-health-roster-result.log`, `native-reversed-display-result.log`
in `build-coop/qa-state`, and `build-coop/qa/host-reversed-split.png`.
Both CTests pass. Qualified game SHA256:
`677b8443029d636b669c6fa3e34bfc9f71a69e8cca2d3770cb7153b921005122`.

This completes the enrollment/normal healing ownership repair, not campaign
resource certification. Heart/capsule attribution, X upgrades/weapons, native
exchange restrictions, scenes/stage transitions and full reversed death/retry
coverage remain open. Public roster settings/profile negotiation are still
required. All15 final QA rollups and the180-point suite remain open.

### Remote common-object resources checkpoint

Private common-enemy/station graphics now resolve their actual resource-phase
file, VRAM placement and palette without uploading over the anchor's resources.
This uses X3's08:8623 stage directory,06:F732 compressed files,08:D7C9 upload
segments and06:8180 palette lists. Files are bounded immutable derived caches;
native02:E15C's actual group/file row selects common-object art. Customized
constructors, item/effect resources and dynamic cached-file uploads remain
separate work; this does not certify every sprite family.

The decoder matches native decompressed cache bytes for files00/02/28. File02's
segmented upload and palette match actual enemy VRAM/CGRAM. A real stage1 case
admits class2F from event3C:D1CE at1424,592 through the original union allocator.
Its native03:D6F0 initialization uses the stage table to place the Ride Armor
station at1712,432. A QA teleport then uses that upper room's03:E23A camera
limits; it bypasses route traversal and camera admission, not the allocator,
constructor or descriptor submission. The station renders in P2's selected and
real host split fields while the anchor remains phase0 with file3C's placement
and palette mapping unloaded. Inspected captures:
`build-coop/qa/display-resource-remote.png` and `host-remote-resource.png`.
Station interaction, boarding and vehicles are not accepted by an art check.

All current ROM cases pass in `build-coop/qa-state/native-resource-combined-result.log`
(exit0), including native/private field equality, split/selected simulation
equality, both rosters, health limits, contacts, supports, survivors and retry.
Both CTests pass. Game SHA256:
`d53982299907efe3c32703f8242030151aa1006ebe44472d49eb58907f977907`.
The remote-resource check also verifies painting leaves CPU time and shared
WRAM unchanged. Optional `X3_COOP_REMOTE_RESOURCE_OUTPUT` saves its disposable
slot7 fixture. All15 rollups/full180-point final acceptance remain open.
