# X3 co-op implementation and qualification

Development branch: `feat/x3-coop`, based on X3 master `11fd733`.
Current pinned build reference: engine `36a3d80e`, UI `cf6315a`.
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

The launcher catalog now offers X / Zero Co-op, with roster and camera policy
choices and a separate local-view feature with compact or side-by-side shared
HUDs. Private views show their actor's native health, X's applicable weapon
energy/icon and personal charge indicator. Diagnostic environment overrides
remain available. Holding Select withdraws a living actor; release and hold
again to return beside the partner when clear native ground is available.
Either living actor can open the paired pause with Start. Both navigate their
own panel; Fire selects equipment or uses a shared subtank, and Start resumes
when healing finishes. Learner mode has focused native protection and return
checks. Adaptive co-op presentation and network startup have pilot evidence;
full campaign, lifecycle, presentation and online qualification remain open.

Full campaign is the default Zero policy. Stock restrictions start him
withdrawn; his Select control calls him when the original game permits it.
Stock encounter return and permanent loss are implemented and have focused
native-route checks; the complete campaign suite remains open.

## QA burndown

**Accepted areas: 0/15.** Component checks and a successful build are not
campaign or co-op gameplay acceptance.

| Area | Status | Acceptance |
| --- | --- | --- |
| Individual death | D01-D05/D07/D09/D14/D16 accepted (9/16); other death criteria open | Either survivor retains play and progression, with no global fade, blanking or stale freeze. |
| Team wipe | T01-T05/T08-T10 accepted (8/10); boss/door wipe, transients and game over remain open | Simultaneous/staggered deaths produce one native retry and the intended life cost. |
| Fallen persistence | F02-F07/F09 accepted (7/10); checkpoints, difficult placement and reconnect/options remain open | No checkpoint, stage exit, voluntary rejoin or menu revives a fallen player before team wipe. |
| Separated spawning | A06-A08 accepted (3/14); other frontiers, lifecycle and campaign families open | Either leader activates native enemies, supports and one-shot events without duplicate visits or cross-gap activation. |
| Supports/hazards | P01-P04 accepted (4/16); other support, terrain and vehicle contracts remain open | Both riders retain support, momentum and crush behavior while each platform advances once. |
| Enemy interactions | E01/E04/E11 accepted (3/14); remaining contacts, target switching, captures and attachments open | Targeting, contact, capture and release recognize either actor and clean up victims. |
| Combat ownership | C01/C02 accepted (2/12); other attacks, native combo/hit windows and lifecycle cases open | Concurrent attacks retain native damage/cooldowns and clear on lifecycle exits. |
| Doors/scripts | Stage1 entrance/transport and stage5/stage8 capsule tasks pass focused both-roster/API-retirement checks; visuals, other scenes and authored scene deaths remain open | Either/both door policies and native scripts complete with either survivor. |
| Campaign branches | Blast Hornet admission and Zero survivor reward observed; ordinary next-stage flow and other routes open | Intro, eight stages, optional encounters, fortress, refights and endings complete with either roster. |
| Menus/resources | M01-M11 accepted (11/12); transition/menu overlap remains open | Paired navigation, shared tank spending, healing and resume cannot leak inventory or input. |
| Visuals | Real Fit fields and independent width-isolation checks pass; full visuals/audio remain open | Both views retain correct art, lighting, HUDs, Cx4 effects, margins and transitions. |
| Rollback/state | R01/R04-R10 accepted (8/10); remaining interrupted-state and malformed-input coverage open | Complete initialized co-op snapshots, mid-pass restore, rewind and input replay agree, and malformed state is rejected. |
| Netplay | N02/N04/N05/N11/N12 accepted (5/12); remaining peer gameplay behavior stays open | Two processes and LAN qualify shared/independent views, rollback, menus, deaths and disconnects. |
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

### Native health refill ownership checkpoint

Health items now borrow their recipient for each original normal/frozen item
update, retaining the recipient through the entire native refill. The world
item advances once. A pending refill keeps its original owner when shared
scene control changes. Full-health Zero can bank overflow into the same native
shared subtanks, with the original capacity, amount, timer and sounds.

Two demonstrated failures are repaired: removing a recipient's view could cull
the refill item before cleanup and leave the world frozen; and the other actor
could move while native terrain collision was skipped. The item now survives
until native cleanup, and the non-collector parks while the collector's native
animation and item timer continue. Cleanup can temporarily inspect a retired
body with zero HP without capturing changes or restoring its life/resources.
The native cleanup releases the shared freeze before returning to the survivor.

The authored check uses stage1 health item3C:D1FB at1848,1448. A teleport,
damaged HP and lower-room camera band prepare the approach; route traversal and
camera admission are explicitly bypassed. Original allocation/contact/refill
results are not supplied. Both roster assignments pass Zero20->28 with X16
unchanged, all seven freeze flags released, no fade/stage exit, and identical
48-frame replay from a pending refill. Movement/jump input leaves both actors
parked during collision freeze. An owned empty subtank80 becomes82 once through
the native full-health banking path; other tanks/personal HP stay unchanged.

Isolated withdrawal/fallen lifecycle events cancel a pending refill without
revival or healing the unrelated survivor. A codec probe saves/restores inside
the real retired-recipient borrow; malformed saved recipients fail before
mutation. These probes do not certify launcher withdrawal, an authored death
during frozen gameplay, or complete scene/menu integration. A native frozen-item
call also retains the recipient with shared scene control set.

Refill state uses the explicit8-byteX3R1 chunk; current full snapshots412611B.
Fresh native fixtures and all current ROM checks pass in
`build-coop/qa-state/native-pickup-combined-result.log` (exit0). Both CTests
pass. Strict generation reports byte-identical complete trees with45 routed
native variants, and the regenerated build passes the combined checks.
Inspected real host split captures `host-health-mid.png` and
`host-health-complete.png` show the correct20/28HP meter, native healing pose,
surviving X and bright distinct rooms. Game SHA256:
`7cdb4382516568746010294ccf81577d6076d1684c62b4255bf75d5c96fd6a4f`.

These narrow checks support M07/M08/M10 and R01/R02. Pickup contact fallback
when the nearest body misses, other item families, anchor-owned refill/retirement,
menu spending and full campaign qualification remain open. The full180-point
post-implementation suite and all15 final rollups remain open.

### Native pickup contact and team lives checkpoint

The nearest body origin can miss an item that touches the other character's
different native hitbox. CC5C now retries only that original overlap helper
after a true miss, restores its entry registers/scratch, and keeps the winning
collector through the single item update. Missing item/player geometry returns
are included. The continuation remains snapshot/preflight validated.

A stage1 authored health-item fixture compares a nearer X miss and farther
Zero hit against separate original single-body calls. The paired result matches
all48 item bytes from exactly one Zero update. Both roster assignments cover
the second-contact continuation, zero-HP/fallen candidates, missing geometry,
and simultaneous contacts. Two touching bodies heal only X8->16, leaving Zero20
unchanged; both park during the collision freeze and the native cleanup releases
it. Camera/position preconditions bypass route traversal and camera admission.

Native class03/04 life items borrow their collector too. Full-campaign co-op
omits Zero's three stock character exclusions at01:D9CD/DA25/DA7C while retaining
the original award, cap and retirement. Focused dropped-object fixtures cover
airborne and both grounded branches, both rosters, simultaneous contact, one
life2->3, and consuming the item at the cap9. Personal HP remains unchanged and
no refill freeze remains. These fixtures bypass drop RNG/allocation and do not
qualify authored drop spawning or item graphics.

Fresh fixtures and combined ROM qualification pass in
`build-coop/qa-state/native-pickup-complete-result.log` (exit0), including the
existing survivor/retry, targeting, support, private display and replay checks.
Both CTests pass. Strict generation is byte-identical with50 routed native
variants. Inspected real host split captures `host-pickup-final-12.png` and
`host-pickup-final-62.png` retain the distinct bright rooms, unchanged X16HP and
Zero20->28HP with the native refill pose. Game SHA256:
`3f69f70cf28c774e1c684c244f609562000cb9597c009db188bc1ffffb0deaa8`.
Weapon energy, persistent acquisitions, paired menus, public settings and the
full180-point final acceptance remain open. All15 final rollups remain open.

### Remote X weapon-energy ownership checkpoint

Class01 weapon-energy items now borrow native X, including when Zero leads the
world or stands nearer the item. Native Zero eligibility stays unchanged; this
adds no Zero special weapons. The existing recipient latch also retains X for
normal/frozen energy refill and cleanup. Item movement and award still run once.

A matched native controller check reproduced an unclaimed remote-X item while
the original X-only call collected it. Both rosters now match all48 item bytes
from that single native update, restore a pending refill identically, refill
X8->16 energy with native inventory flags, and leave other inventories and both
personal HP values unchanged. Zero's actual hitbox overlaps rejected items with
distant/fallen/withdrawn X. Isolated withdrawal of a pending X releases every
freeze without awarding energy or restoring participation. This qualifies the
lifecycle API cleanup, not the future launcher withdrawal interface.

These dropped-object fixtures bypass allocation/RNG, route traversal, native
leader handoff and camera admission. They explicitly prepare a coherent whole
actor context and camera; visual output advances the native presentation queue
after those preparations. Earlier captures from mismatched fixture camera,
palette and queued presentation are excluded from visual evidence.

Combined ROM checks pass in `build-coop/qa-state/native-weapon-combined-result.log`
(exit0), as do both CTests. No new generated boundaries or state layout were
needed. Inspected real host split captures `host-weapon-final-12.png` and
`host-weapon-final-62.png` retain red Zero, X's native refill/standing poses,
correct personal health and distinct bright rooms. Weapon HUD completion is
still open. Game SHA256:
`62d934a73bf832e5ba2c736002f83a604375cee6069aa4e1c2b24dfdf8c7f52a`.
Persistent acquisitions, simultaneous resource tasks, paired menus, public
settings and the full180-point acceptance remain open;0/15 final rollups pass.

Competing-refill risk was evaluated without changing the runtime. Native
00:DAAF checks1F28 after each item and switches to the frozen pool path; it
does not admit a second refiller in that pass. Two differently sized health
drops and a mixed X-energy/Zero-health pair pass in both slot orders and both
rosters. Each original collector receives the intended amount once, the next
item waits for cleanup, and96-frame full-machine replay agrees. No premature
freeze release or stale freeze was observed. Evidence:
`build-coop/qa-state/native-competing-result.log` (exit0), same game binary
`62d934a73bf832e5ba2c736002f83a604375cee6069aa4e1c2b24dfdf8c7f52a`.
These are dropped-controller fixtures with prepared cameras/positions, not
authored drop/RNG or campaign qualification. No new refill arbitration was
needed. The full feature and180-point acceptance contract remains open.

### Launcher package and local view selection checkpoint

`megaman-x3.coop/coop` selects the native roster and Unified/Independent world
policy before boot. `megaman-x3.coop/view` selects automatic split/shared,
split, P1, P2 or leader presentation. The latter has a trusted presentation-only
plugin and no dependency that can disable actors when its checkbox is cleared.
Window previews read selections without freezing the later simulation profile.
Both engine input seats are available through the launcher.

Co-op currently uses one common256-pixel simulation field, regardless of local
layout. The host composes split output separately. Adaptive widescreen private
fields, the online cosmetic grant/profile handshake, stock restrictions and the
remaining co-op controls are still required before release.

Actual persisted package selections boot with every prototype override cleared.
Automatic split, P1 and P2 runs follow the same2256-frame native input route.
Their CPU/WRAM, canonical SPC/DSP, PPU, actors, views, continuations and queued
presentation agree byte-for-byte. Ordinary files differ only in the host DAC
output ring/cursors at[66496,99276), whose native ABI boundaries were verified;
that consumer queue is excluded from this cross-process comparison. Each split
pane exactly matches the corresponding selected-player capture. Evidence:
`build-coop/qa-public/view-equivalence-result.log` and its public-* captures.

Disabling the view feature retains the same canonical machine and both actors.
Reversed roster/Unified selects the intended native characters; co-op off leaves
the stock single actor. Persisted choices survive each run. These cases and
inspected gameplay captures are in `qa-public/policy-result.log`; the launcher's
interactive controls still need a visual/navigation check.

A fresh final public boot passes the combined ROM suite, including full current
save/load/replay, survivors/retry, contacts, supports and competing resources:
`build-coop/qa-state/native-public-settings-combined-result.log` (exit0). Both
CTests pass. Engine build-only repair `ee208443` fixes nested Windows shell
handling of multiple catalog IDs; missing packages are still rejected. Game
SHA256 `b8602217aa8b925462fa8a0d91c1243944d91969d1584921913de0095d7d3673`.
All15 final QA rollups and the full180-point acceptance suite remain open.

### Native door admission and remote gate art checkpoint

The authored stage1 midboss entrance3C:D27C initializes through the original
union allocator and door constructor. A matched original single-body call
admits the touching actor, while the previous co-op implementation left the
gate waiting for its distant world owner. Co-op now borrows a candidate for
native door classes07/10/11 and retries the whole side-effect-free01:E0AE
admission predicate after a miss. Success adopts the entrant's independent
camera and current/target room limits before changing the shared leader.
Native opening, tile updates, forced movement and cleanup remain one task.

The fixture now explicitly reverses world ownership as well as roster seats.
Both X and Zero enter remotely in both rosters. An excluded nearest actor
retries the other actor's full predicate; two excluded, zero-HP and fallen
candidates cannot admit the gate. Simultaneous contacts select one entrant.
An interrupted second predicate passes snapshot preflight/load, and300-frame
replay from an admitted gate agrees through the native walk and freeze cleanup.
Teleports and prepared room-camera bands bypass route/camera admission history;
these checks do not certify normal campaign traversal or every gate family.

Visual inspection found unrelated sprite tiles on the remote gate. The private
resource lookup now covers its native files19/56 and groups1B/4C. File56 has352
decoded bytes but a384-byte native DMA segment; rejecting the whole file had
fallen back to unrelated native VRAM. Segments now retain meaningful decoded
bytes, with padding transparent and pixel reads bounded. Actual host split
captures `build-coop/qa-door/host-door-p1-x-{mid,complete}.png` and
`host-door-p2-zero-{mid,complete}.png` show clean X/Zero art and bright gate
rooms during opening and after cleanup.

The visual run also demonstrates an unresolved failure: the distant partner
falls during the committed camera handoff. Both panes subsequently follow the
survivor; this is not evidence that independent cameras passed. Door transport,
parking/arrival, the optional both-present policy, owner retirement and remaining
authored gates stay open under S01-S07/D13/T06/V12. No final scene rollup passes.

Strict regeneration reports51 routed variants and byte-identical generated
trees. The regenerated build, both CTests and combined native qualification
pass; evidence is `build-coop/qa-state/native-door-combined-result.log` (exit0),
including existing contacts, supports, resources, independent routes, survivors
and native retry. Game SHA256:
`a146c4a20aac00eff60531ce95dac00d85344e8d2e8226e83a109cd7b8dc4c57`.
Five additional X1 parent/source-reviewed door/capsule repair dispositions are
in `COOP_FIX_REFERENCES.md`. The full180-point final suite and all15 final QA
rollups remain open. The release archive is unchanged.

### Door participant transport and native commitment checkpoint

Door admission now parks the other living actor's controller, terrain pass and
owned transient effects. The original world/door task continues once. A saved
12-byteX3S1 chunk owns the participant, door, departure/wait/arrival phase and
admission policy. Queued presentation uses the original7F/7E warp animation
records and private CHR. It does not change live HP or revive retired actors.
Arrival aligns the characters' actual native collision bottoms, retaining their
different standing heights, and gives the arriving actor32 invulnerability ticks.

The authored entrance3C:D27C passes remote X and remote Zero in both seats.
Both remain alive with unchanged personal HP, then independently resume native
movement. Transport waits for the original body lock as well as door cleanup:
the following class2C miniboss introduction legitimately locks the entrant
again after the gate closes. Its normal timer and original control release run
without being bypassed. Snapshot/replay follows that actual completion point,
rather than assuming300 frames means the whole scene has finished.

An entrance sensor01:CF02 could request Zero's stock exchange back to X; native
00:A30C then reset the camera during the admitted door. Full-campaign co-op
omits that request while preserving the separate native exit sensor. The stock
participation/loss option remains unimplemented; co-op off retains native rules.

The public Door admission option defaults to Either player. Both players tests
both complete original admission predicates, waiting only for living available
participants. Both-player traversal, fallen/absent/zero-HP exclusions and
simultaneous contact pass for this authored entrance. Fresh public-package boots
with every prototype override cleared save the selected policy before gameplay;
`build-coop/qa-public/public-door-policy-result.log` records both selections.
Interactive launcher navigation is still unverified.

Isolated lifecycle API events cover either actor falling/withdrawing immediately
after admission and during the following introduction. The survivor finishes
the native sequence with its own character/HP; the retired actor remains fallen
or absent. These are not authored enemy-death or launcher-withdrawal acceptance.
Malformed scene fields and an inconsistent living scene owner fail before
mutation. Current snapshots are412623B, with fresh disposable fixtures.

Post-arrival visual checks caught rejected moving snapshots: native door
commitment can hold a new minimum beyond its old maximum. Private views now
store the same effective clamped interval and retain the native commitment to
the exit sensor. Strict private-state validation remains intact. Moving snapshots
pass preflight/load and resume in the actual host; their earlier black cold-boot
captures are excluded from gameplay evidence.

Inspected actual host split images in `build-coop/qa-door/host-transport-*` cover
the waiting scene, arrival and released actors, including both reversed-roster
release captures. They show bright fields, living personal meters, correct actor
art and separate post-transport movement. This does not qualify the miniboss's
complete graphics/combat or other scene families.

Strict regeneration reports52 routed native variants and byte-identical complete
trees. The regenerated build, both CTests and
`build-coop/qa-state/native-door-transport-combined-result.log` pass (exit0),
including prior contacts, resources, supports, independent routes, survivors and
native retry. Game SHA256:
`1fca554cf7013773f2e0b06987d635f3341a16b1dc1a4d710ece4e9b6b67ef97`.
Other authored gates, native wipe/fatal-contact cases during scenes, exit-sensor
traversal, capsules/demos and broader campaign integration remain open. This is
focused S01-S07/R01-R02/V12 evidence; all15 final rollups and the full180-point
final suite remain open. The release archive is unchanged.

### Native persistent acquisitions checkpoint

Authored stage1 heart3C:D3E7 and stage5 subtank3C:E394 now borrow native X
even when Zero owns the world. Both roster orders retain the cartridge's
X-only eligibility: touching Zero cannot claim the item with distant, fallen
or withdrawn X. Simultaneous contact produces the same48-byte item state as
one original single-X update. Shared acquisition flags/tank reserves remain in
world RAM; Zero retains28 HP/max and receives no invented weapon inventory.

The retained recipient now covers these timed native tasks. Movement/jump
input parks both actors, the original task releases all seven freeze bytes,
and a pending award replays to the identical full machine. The heart gives
native X two capacity/current-HP points; the subtank adds one empty shared tank
without changing either actor's health.

Interruption probes found a permanent-reward loss: native01:E968 commits the
heart flag before its two timed capacity increments. Retirement could cancel
the increments after that permanent flag was stored. Cleanup now calculates
retired X's committed capacity exactly as native04:DBA6, retaining stored HP
and life state. It copies only native04:D130's action release, rather than
capturing a projected retired body with zero HP. API fall/withdrawal before
the wait and partway through the award passes for both items/rosters. Fallen
rejoin is still refused. Living return/control probes explicitly restore the
prepared private bounds; public safe rejoin placement remains unimplemented.
The route/roster preparation and short control window above the subtank pit
do not qualify that route or a safe return location.

Actual host split captures exposed unrelated enemy art on the remote heart.
The bounded native resource resolver now recognizes heart group38/file36 and
subtank group96/file8C, preserving the subtank's original common pickup palette.
Inspected `build-coop/qa-door/host-persistent-*` images show the corrected red
heart, authored subtank and both seat orders before/after their awards, with
bright separate fields and personal meters. The heart before/after image pair
retains the reproduced graphics failure as evidence.

Both X3 CTests and `build-coop/qa-state/native-persistent-combined-result.log`
pass (exit0), including the new acquisitions plus existing doors, resources,
contacts, supports, separated views, survivors and native retry. Game SHA256:
`139c811ba9ca053f24e828045c52592fdea30f0e8a151848219c847e7df460d5`.
The52 existing routed variants and state layout are unchanged. This is focused
M09/M10/R01/V06 evidence, not completed persistent-upgrade/menu/campaign
qualification. Capsules/demos, shared menu spending and public rejoin remain
open; all15 final rollups and the full180-point final suite remain open. No
release archive was replaced.

### Native capsule ownership and recorded demonstrations checkpoint

Native class4D capsules now borrow X before their constructor evaluates health,
armor and character requirements, including an uninitialized object with zero
object HP. A constructor probe reproduced the previous Zero-owned evaluation;
the corrected native entry observes X's own HP/max and character in both seats.
Stateful capsule contact is excluded from generic paired hostile retries.
Opening and pod admission adopt X's view before committing the native script.

The saved scene now distinguishes doors, capsule dialogue and acquisition. Its
explicit16-byteX3S2 chunk retains both seats' raw input history. Shared text uses
X's seat, or the survivor's after X retires. Native recorded input and temporary
mapping own X's actions throughout the demonstration; conflicting physical
left/jump/fire input cannot replace them. Old disposable snapshots are rejected;
current states are412627B. No native save migration is involved.

Stage5's authored capsule3C:E11D awards native armor bit04; stage8's arms capsule
awards bit02 and exercises the longer follow-on demonstration. Both use the
retail allocator and constructor, with coordinates/indices read from06:CCBE,
06:CCFA and06:BBFD. Teleports and prepared camera bands bypass route prerequisites;
they do not set admission, award or completion flags. Single-X reference updates
and both co-op roster orders agree. The recorded-body traces match exactly:
stage5 has81 frames/hashcc33dfad0d9e1d88; arms has411/hashca29beaa17538dc6.
Native locks clear and participant arrival finishes after the actual script.

Six lifecycle API interruptions per roster cover either participant withdrawing
or falling during dialogue/acquisition, including the recorded demonstration.
Already admitted X-only tasks finish with physical zero HP and no retired-player
controller pass. Their script-owned body/auxiliary/palette writes are retained
without importing that temporary zero HP or changing the stored life state.
The arms-only extra demonstration is omitted for retired X after the award.
Earned armor survives; an opening alone awards nothing. The survivor retains HP,
brightness and actual native movement afterward; fallen rejoin is refused.
These are API retirement probes, not authored fatal-contact/wipe qualification.

Pending dialogue and acquisition replay to the identical full machine; interrupted
native borrows pass runtime-extension preflight/load. Malformed scene owner,
phase, kind and raw-history fields reject transactionally. Retired-NPC constructor
ownership and preservation of the survivor's caption input fix two additional
interruption failures found while exercising these cases.

The private resource resolver now retains the capsule/hologram's native40-tile
layout bias and the hologram's fixedE4 palette. Actual split host captures under
`build-coop/qa-door/host-capsule-s2-*` and `host-capsule-arms-*` cover pod art,
caption, completed awards and retired participants. **Hologram presentation is
still unqualified:** it was not visible in the inspected caption captures.
A matched native/private diagnostic also differs in this prepared room; its
`capsule-hologram-reference-result.log` is a failing diagnostic, not passed QA.
Do not accept S08/S09/V12/V14 from these static captures. General safe arrival,
Unified capsule activation, other upgrades/prerequisites, scene deaths/wipes
and moving-clip/native presentation checks remain open.

Strict regeneration is byte-identical with56 routed variants. Both X3 CTests,
`native-capsule-final-combined-result.log`, `native-capsule-arms-result.log`,
`native-capsule-arms-aligned-result.log`,
`native-capsule-support-retry-result.log` and `native-capsule-aligned-result.log`
under `build-coop/qa-state` pass. The combined check includes contacts, resources,
doors, supports, separated views, both contact-death survivors and native retry.
Its fresh independent support fixture now explicitly matches private cameras to
the test teleport before native allocation; its original spawn/riding assertions
are retained. Production executable SHA256:
`98657b71ace14555d38bcd71eb391cdbdfb966aa5304c6c7240d5568f51cbd6a`.
The current test executable after the camera-preparation adjustment is
`d4dea53a4a2dcaf4d45359f616b6b41315e9a03ebdedc0ac66e3bfe2cdb53afd`.
All15 final QA rollups and the full180-point final suite remain open. The current
delivery ZIP is unchanged.

### Native stage-entry ownership repair

Fresh native stage construction now suspends personal-context capture, paired
world/contact passes and private rendering until the ordinary gameplay caller
has completed the original arrival. Living actors bind to the native new-stage
body and their own character initialization. Fallen and voluntarily absent
actors retain their personal contexts and life state; the living world owner
does not change. Scripted reconstruction calls are excluded from this boundary.

The existing16-byteX3S2 scene payload records pending arrival without changing
snapshot length. Saving during arrival must not capture the temporary native X
into the retained actor. Malformed scene fields, a stage mismatch and overlapping
controller, terrain, contact, owned-object, draw or world-target continuations
are rejected before machine mutation.

`tests/x3_coop_stage_test.inc` exercises the real stage5 loader in12 API-prepared
roster/owner/lifecycle cases: both living, either fallen and either absent, in
both seat assignments. Pending and completed immediate restore, full pending
replay, native character/HP/max, entry coordinates, standing heights, brightness
and subsequent native movement pass. Each malformed borrowed continuation is
independently valid without the pending stage, so rejection tests the actual
cross-state invariant. This preparation bypasses previous travel and stage-end
triggers; API retirement is not authored death coverage.

The original natural-X-death/Zero-survivor loader route also passes unchanged:
X stays fallen with stored rawHP128/max16; Zero remains nativeB6=2 withHP/max28
at384,2711 after the stage5 load. The former result incorrectly changed Zero to
nativeX and16HP. Slot16 wraps to0 in the host as before.

Evidence under`build-coop/qa-state`: `native-stage-regression-before.log` retains
the pending-save counterexample; `native-stage-regression-final.log`,
`native-stage-survivor-validated.log`, `native-stage-combined-result.log` and
`native-stage-arms-result.log` pass. The combined run includes the existing
roster, doors, resources, independent views, contacts, supports, capsule,
survivor and retry cases; the separate arms run covers its longer native demo.
Both X3 CTests pass. No generated boundaries changed. GUI SHA256:
`3ead286e3bc5fd1b085e9e65cc3361bbb18925474445077e6e4ffb2c7f61b654`;
test SHA256:`47db70eb31c6e6a500bbcaa6ab5c2396fb1146c25b7af9db77de8d7af9d3bf2d`.

These are prerequisite F04/R05/R09 implementation checks. Actual stage exits,
campaign rewards, safe public rejoin, native scene visuals, menus and online
qualification remain open. All15 final rollups and the180-point suite remain
open; the delivery archive is unchanged.


### Public withdrawal and safe native return

Holding Select for90 world updates withdraws that seat. Release and hold again
to return; if no safe landing is available, the held return waits. The request
latches after success so a continued hold cannot toggle participation repeatedly.
Both seats use the same simulation inputs and deterministic ordering. Simultaneous
withdrawals leave one living participant, and neither controls nor safe placement
can restore a fallen actor. Scripted/captured/vehicle actions must release before
public participation changes are admitted.

Return reads the current native stage layout, metatiles and collision classes,
including dynamic map edits. It requires a visible flat terrain floor, full
standing clearance, a clear corridor and space around shared actors/projectiles.
It retains the returning character's HP and personal context, clears cancelled
transients and replaces obsolete private camera bounds. Unsupported terrain or
an unsafe/busy location refuses the request without changing the machine. This
is a conservative ground return; broader authored hazards, supports and vehicle
ownership still require their campaign checks.

Clearing transients also exposed persistent X armor children. Return and scene
arrival now reconstruct those three children from the shared acquired-parts bits,
matching original04:B5BB construction rather than borrowing Zero's or stale X
slots. Native child updates continue after return. A Zero-led authored stage1
door also retains X's acquired children through parking and arrival in both
rosters. Acquired-parts flags are explicit test preconditions; these checks do
not establish armor acquisition or every weapon/upgrade combination.

The8-byteX3J1 chunk serializes both Select counters/latches. Full snapshots are
now412635B; older disposable fixtures are rejected and the existing native
scenario scripts regenerated the working fixtures. Partial-hold replay, return
with stale view bounds, completed restore, malformed counters/reserved fields,
unenrolled participation and pending-stage consistency pass transactionally.
The original stage-entry codec and generated boundaries are unchanged.

ROM-backed return/control checks cover both seats/rosters, unequal personal HP,
fallen exclusion, last-player/simultaneous withdrawal guards, subsequent native
movement, an actual jump with no nearby landing, and an isolated corridor made
through the original native metatile writer. The corridor setup is not authored
wall/door coverage. Public launcher-selected startup with every prototype
override cleared also withdraws and returns Zero28HP beside X16HP, then moves
Zero from416 to455 on native stage5 ground. The composed scene was inspected;
launcher navigation and owner play checks remain open.

Evidence in`build-coop/qa-state`: `native-return-base-result.log` and
`native-return-combined-result.log` pass the native actor/resource/view/door,
armored arrival, supports, capsule, both survivor and retry cases on GUI
SHA256`56e8f7e072f482523eee9e1586b52409ccc889581d5bd2d4204cb969dcd7b785` /
test`27bd9d9decc6e01b9f9e65370f30281cdd65b9743e46ee49994edfa18e6364f6`.
After the corridor and ordinary-action guards, focused
`native-return-corridor-result.log`, `native-return-arms-result.log` and
`public-return-final-result.log` pass on GUI
`49d9efda5feec1d77bdaf632f02ed36627c7e13aee5220fc122aed1d2fb59edb` /
test`0bddcc448c15e98184d48001e11a75075b7229e7b2b4cb22265af96f43b673ea`.
Both X3 CTests pass. No generated routing changes were needed. The public
control image and states are in`build-coop/qa-public/public-return-*` and
its disposable slots14/15.

This supports F05/F06/F07, C10 and R05/R09 narrowly. It does not qualify all
terrain, native scene fatalities, campaign exits, shared menus, HUD layouts,
assists, online play or the final180-point suite. All15 final rollups remain
open and the delivered archive is unchanged. Old fixture copies under
`qa-state/pre-x3j1-fixtures` remain temporary reference data for ongoing scene
analysis; remove obsolete copies after retaining the needed evidence.

## Shared layouts and owning weapon/charge HUDs

The presentation-only launcher feature offers compact character-based stacking
or seat-based side-by-side meters. Zero's compact frame is reflected with its
symbol upright. Private views retain their own upright health meter. X's
acquired selection uses the original04:DCC5 energy strips and06:97AD DMA-list
icon; Zero receives no special-weapon bar. Small badge indicators use each
actor's native published charge level and submitted palette.

Presentation frames now retain the18 native weapon bytes and64 HUD palette
bytes from the submitted world update. X3P4 replaces X3P3 in all three lists;
current full snapshots are412881 bytes. Old disposable snapshots are rejected.
Live RAM/PPU changes cannot recolor or drain an already submitted HUD frame.
Shared composition removes original HUD pixels after native OBJ evaluation,
preserving OAM, hardware sprite quotas/flags and boss meter slots. Borrowed
overlay bindings are restored after each draw.

`tests/x3_coop_hud_test.inc`, selected with X3_COOP_HUD_STATE and the real
X3_COOP_HUD_REVERSED_STATE fixture, exercises native weapon cycling, unequal HP,
actual Zero charging, private ownership, frozen colors/energy and current-build
visible-state restore. Both real roster assignments match all native owner-view
pixels with0 differences, including the acquired weapon bar/icon. Compact,
horizontal and native-reference rendering preserve identical complete machine
snapshots on both legacy and new PPU renderers. The native-reference selection
is internal test instrumentation; the launcher exposes only the two layouts.

Evidence under`build-coop/qa-state`: `native-hud-backend-result.log`,
`native-hud-base-result.log`, `native-hud-combined-result.log` and
`native-hud-arms-result.log` have child exit0. The combined run includes both
natural-contact survivors and actual team retry using regenerated X3P4 fixtures,
plus the existing stage/door/resource/support/capsule/view/return gates. Both
X3 CTests pass. Launcher-selected fresh startup with every prototype override
cleared passes both HUD choices; owned resume copies were loaded and consumed,
and actual256x224 composed captures were inspected at
`qa-public/public-hud-{compact,horizontal}.png`.

Tested GUI SHA256:
`89337aca84758afad42dfcc27e7e7f64648059d3254bb3bfedf2ed53d7a39b79`.
Test SHA256:
`5b3928974faa1f63de2d08d581018c6d409f4ac87d57d645dc3ae588fcf5fde8`.
This qualifies focused HUD ownership/composition and state invariance. It does
not complete V08/V09/V10/V14: damage/refill transitions, survivor HUD handoff,
pause/aspect/scene coverage and moving clips remain for the final campaign
pass. Stock restrictions, paired menus, assists, adaptive co-op fields, online
play and other listed campaign requirements also remain open. All15 rollups
remain open and the delivered archive is unchanged.


## Native stock participation and single-participant doors

Full campaign remains the default. The stock launcher option starts Zero
withdrawn and uses his Select control for the original game's call. Eligibility
matches executed00:CBB2 in both rosters: native actions2C/76, stages0/9/C/D,
encounter lock1F5D.80, used-call flag1FE1.01 and permanent-loss flag1FB2.40.
Calling consumes the original04:8DF5 flag. Withdrawal cannot repeat a used call;
the native stage loader owns its reset. Native stage entry also leaves stock
Zero withdrawn until requested.

The native01:CF02 restricted-room sensor records a pending stock return rather
than initiating a character exchange over the other co-op context. The balanced
world boundary applies it after borrowed item work returns. X retains personal
health and control; a living withdrawn X returns only to clear native ground.
While waiting, Zero can move/jump/dash toward ground but cannot charge or fight.
A fallen X stays fallen and the unavailable team uses one original native retry.
Actual stock Zero death commits the cartridge's permanent-loss flag without
fading the survivor's world. Death/withdrawal clears a pending return.

Authored door checks exposed a separate single-participant gap:01:E0AE began
co-op scene tracking only with two living actors. It now tracks a living entrant
with either roster size; only the optional contact retry requires a partner.
The original stage1 gate and following introduction complete in765 updates in
both seat assignments: stock Zero hands back to X, stock X proceeds with Zero
unavailable under Both policy, and full-campaign X or Zero proceeds alone under
Both policy. The correct native body, personal maximum, final camera and released
locks survive the sequence. X arrives at2577,1439 and Zero at2577,1431.

`tests/x3_coop_stock_test.inc` additionally covers original menu eligibility,
real Select calls, native stage reset, safe airborne return, movement without
charging while pending, transactional malformed requests, pending-return death,
whole-machine replay and native single-cost retry. Both real roster fixtures
also reach Zero death through actual enemy contact; the permanent loss and
fallen state survive real native next-stage construction. The stage requests
bypass travel, and API lifecycle/encounter preparations do not qualify authored
hazards or complete campaign routes.

X3J2 replaces X3J1 without increasing its8-byte size: two hold counters, stock
profile and pending-return flag. Full states remain412881 bytes with X3P4
presentation. Stock profile and pending work restore with the simulation;
inconsistent requests reject before mutation. Old disposable states are rejected.

Final focused evidence in`build-coop/qa-state`:
`native-stock-final-result.log`, `native-stock-final-base-result.log` and both
X3 CTests pass. The broader`native-stock-combined-result.log` passes before the
single-participant repair; final focused gates cover that repair and the affected
base actor/resource/view/door cases. Final GUI SHA256:
`d63b2ad69ca4b45844f8b89228bc801ed047a53f844585c668277e5d33e11b38`;
state tests:`895a58dee9165b6b3fcc5f365c0f8a5c66c3a6f0465a3e442b7ce54f38ac7494`.
Launcher-selected stock startup, calls and refusal of repeat calls are verified
with prototype overrides cleared and actual host captures in
`qa-public/public-stock*.png`. Native saves and delivered builds are untouched.

These are implementation gates for stock policy, single-participant scene
ownership and related state. All15 final rollups remain open. Paired menus,
assists, adaptive co-op fields, online qualification, other campaign content and
the remaining180-point checks still require completion before release.

## Personal native status and pause staging

Native RAM1FFF combines the character selector with temporary armor/damage
status. It now travels with each actor through projection, scripted retirement
and snapshots. A before-fix core probe reproduced X's status leaking into Zero.
ROM-backed contacts in both actual rosters now match the original04:CB31 path:
with acquired body armor and contact damage4, X loses2 HP normally or1 with
the native C0 protection state; Zero loses4. The original consumes/transitions
only the victim's status. The distant actor's health and flags stay unchanged,
and whole-machine contact replay agrees.

Actor wire X3C5 is2806 bytes; presentation X3P5 embeds the expanded contexts.
Complete states are412889 bytes. Old disposable snapshots are intentionally
rejected. Native progress is unchanged.

Native pause replaces gameplay CGRAM staging and OAM with menu assets. Paused
actor capture/projection now preserves personal gameplay palettes, and private
world composition, partner objects and presentation-history capture stop while
1F37 bit0 is set. The original exit value80 already draws the restored world,
so it remains eligible for gameplay presentation. Actual Start pause freezes
both bodies and shared pools; idle menu work preserves all three gameplay
presentation lists. Native menu pixels agree exactly with the solo renderer,
paused immediate restore agrees, and native resume restores partner movement.
These are prerequisites for paired menus, not paired-navigation acceptance.

Qualification evidence under`build-coop/qa-state`:
`native-flags-final-base-result.log`, `native-flags-final-menu-result.log`,
`native-flags-combined-result.log` and`native-flags-arms-result.log` pass,
along with both X3 CTests. Fresh prototype-free public withdrawal/return
also passes (`flags-public-lease.json`, original public route logs).
GUI SHA256:`d20980eb6c62907d68cc8b700b00443b45ae5d88fa40c7e987cc2cc28a63a4a6`;
state tests:`8f44f82807db2be9fcc0bd7f08cabd991bff6dd848523cfe4e882ed737731bc0`.
Fixtures11/1/3/8/6/13/quick/2/10/9 were regenerated for this wire format.
All15 final rollups remain open; delivered archives and owner data are unchanged.

## Paired native pause

Either living seat can request the original00:A2FA pause; P1 wins simultaneous
requests. Input sampling follows the common emulated frame, with Start at
native action1000. The original weapon-cycle action0010 remains gameplay.
The native scheduler enters and leaves pause once; shared enemies, shots and
items remain frozen. The two panel cursors and selection histories stay
separate. X's acquired selection commits through original00:D632, then the
world regains its original anchor. Zero keeps his native arms.

The panels reuse native pause glyphs and the PPU fade. Shared output displays
both panels, split gives each half its own panel, and selected-player output
shows the selected panel. Rendering the same paused machine in these views
leaves the complete machine identical. Controller navigation changes only
the requesting panel. A retired actor's panel has no active controls.

Native00:C8B2 and C901 supply the original subtank pulse cadence, HP limit,
sound and reserve cost. The actors borrow the one native handler in seat order.
A shared tank has one owner. Different tanks can heal concurrently. The first
finishing heal leaves any active tank's slot in place, and the last heal runs
original00:CEAC repacking once. Empty tanks and full HP refuse a refill.
Start does not close an active heal, but the other panel can still navigate.

Exit has one requesting owner and uses the original00:CEF4 eligibility check
and C6F8 wait/fade. Only that owner can confirm or cancel. The focused test
uses the real stage unlock as a prepared prerequisite, then confirms through
actual input; it is not a boss-clear or campaign-route result.

The explicit128-byte X3M1 chunk saves input edges/frame serial, both panels,
healing fields, modal/commit ownership and any borrowed native continuation.
Full snapshots are413017 bytes. Preflight rejects inconsistent phase, life,
tank, cursor, CPU width, action and continuation metadata before mutation.
Native healing matches the original HP/reserve trace over all56 full-tank
ticks in both rosters; active-heal replay matches the whole machine. The
interrupted pulse also restores the saved native borrow in place. A natural
contact-death survivor can pause, restore and resume without re-enrolling the
fallen partner. These are focused implementation checks; the final campaign,
moving presentation, online and owner checks remain open.

Final menu/sole-Zero exit evidence is`native-paired-menu-zero-exit-result.log`
under`build-coop/qa-state`. Both rosters reach the actual original stage-exit
task and restored-world draw on confirmed Escape. Active pause is distinct
from the restored-world values80 and81, so Escape's low bit cannot retain UI
palette or draw ownership. If X is fallen or absent, Zero still runs his
original equipment graphics restore. Final GUI SHA256:
`298a0a01bba0976837671262fc66028b90684b543ed31460674a626c44ea646a`;
state tests:`cac885d1dcc7ec539d2845ece1a8e94bc0230a84c15eda439cc38af1788a6ba4`.

The base gate passes in`native-paired-menu-final-base-result.log`.
Stock, HUD, return, stage, reversed roster, independent routes and targeting
pass before an outdated authored support test reads the old tail offset in
`native-paired-menu-final-rest-result.log`. The corrected test still requires
the original rider bits; support, capsule, both survivors and retry pass in
`native-paired-menu-escape-result.log`. That run precedes only the sole-Zero
exit restore change, covered by the focused final gate. Both X3 CTests pass.

The prototype-free launcher route opens pause with Zero at455, retains X at384,
and lets X resume once; Zero then moves to494. The actual512x224 paired output
is`build-coop/qa-public/paired-menu-final-split.png`; the current-build resume
copy was loaded and consumed. The runtime observations and panel capture cover
an isolated native-resolution profile, not adaptive widescreen or a human
controller handoff. No final QA rollup or release is closed by this component.

## Unified tracking and safe catch-up

The shared camera feeds both living positions to the original00:E088/E0A7
tracking helpers. The midpoint rounds toward the world owner; original
dead zones, movement and authored camera clamps still run. During excessive
separation the camera keeps the leading actor in view. At a balanced world
boundary, the trailing living actor can land beside that leader only on
clear native terrain with room for the body and an unobstructed corridor.
Both life states, HP and weapon choice survive. Unsafe airborne arrivals
wait. This uses the same landing checks as voluntary return and does not
run during pause, forced scenes, or Independent camera policy.

Both bodies run the original camera bounds helper with a balanced guest
return. The shared pit threshold uses the real lower room bound, including
the current native pan, and the original signed16-bit coordinate comparison.
Being below the viewport or wrapping above map Y0 cannot manufacture a death.
A body beyond the real pit threshold cannot use catch-up; original04:D06A
applies the native pit damage.

Both actual roster fixtures pass native midpoint tests in both axes and both
directions, just-before/at-threshold and negative-Y bounds checks, and full
replay. On the real stage5 ledge atx928..1183/y2720, either seat walks into
the authored1184..1263 opening. Both reachx1216 in38 movement updates; native
damage fires once atY2818 or2821. The other actor stays alive, bright and
controllable. The setup positions bodies on the real ledge; it does not
claim the earlier route or a complete pit matrix. Safe ground catch-up and
airborne refusal pass for either leader with the exact retained HP and replay.

Strict regeneration is byte-identical with58 routed variants. Final state
SHA256 `24eabf7f3cebdf5eec7c4434857aeada12a04be881206c0a6feed39953221c6d`
passes the camera, authored pits, catch-up, Independent view, voluntary return,
stock Zero and authored support checks on SERVER in
`build-coop/qa-state/native-shared-camera-server-final-result.log`. Both X3
CTests pass. Final GUI SHA256
`d55f864b265d71da6c1f97e96575e87ff5485b39e2e56292d347d28589449769`.

The earlier prototype-free Unified GUI pilot
(`d91c1195b0e73bce10f2f3e9822bca9ec0d323ed73864302413109e705cb967d`)
observes X stationary at384 while Zero reaches437 and the camera moves from
256 to282. After the longer real walk, catch-up moves X forward to529 beside
Zero at622, with both living and camera447. This is native-resolution gameplay
evidence, not a matched performance or widescreen result. Learner mode,
remaining authored routes, netplay and all15 final rollups remain open.

### Optional Player 2 learner protection

The default leaves original damage unchanged. The opt-in option protects the
second seat from ordinary native enemy contact and direct hazard damage. It
keeps Unified tracking on P1 and uses the same safe ground search as voluntary
return and catch-up. Fatal rescue is available only while both actors are alive
and there is clear ground beside P1. Scripts, captures, menu transitions and
native stock Zero restrictions retain their own rules. P1 death still leaves
P2 in control, and a previously fallen P2 cannot return through this option.

The contact path uses the original no-damage return at04:CD65 and direct
hazards use the existing RTL at04:D0BF before the native helper saves status.
Pending rescue waits for the balanced world boundary; the X3J2 profile and
pending flags encode it without increasing the413017-byte snapshot. An
interrupted original pit-damage return passes codec preflight and restoration;
clearing its required option is refused. Generated routing is62 variants.

The SERVER ROM test binary SHA256
`2de5a35a79523b27a5f773370cf20edce6c6d113d5aee2fede9a032416d3726f`
passes in `build-coop/qa-state/native-learner-server-result.log` (child exit0).
Native contact preserves P2 HP in both roster orders and retains the ordinary
two-point hit on P1. In the real stage5 pit, P2 walks38 and falls16 frames,
then returns at28 HP. The same approach with P1 jumping cannot rescue P2;
original death occurs. P1's own pit death leaves P2 with bright, controllable
gameplay. Whole-machine replay and a real previously fallen fixture pass.
The staged launcher catalog confirms the actual default and opt-in route on
GUI SHA256 `3bb361945080898f071eea8bcccb13bfafcacf5cf6d80669c205e8da149ebd18`.
Both runs leave X at384 and move Zero to437. The default advances the camera
to282, while the opt-in keeps it at256. X3J2 records profile0 and2
respectively. Both child runs and the combined native regression pass; the
final suite remains open.

The follow-up exact state SHA256
`7c9f08b3ee5ee66a30d028c9b0b58c307bf9d4e34975b5739b48d91496e5d5e7`
checks the real safe pit rescue with either roster: Zero at28 HP after16
falling frames and X at16 HP after14. The same unsafe-return, P1-death and
snapshot checks pass for both.

### Authored Blast Hornet approach and Zero survivor reward

Select stage1 checkpoint2 after the original00:9A75 stage selection resets
the checkpoint byte. The original loader starts both actors atx7808. Actual
P2 movement admits both actors through the final class07 door in both roster
orders and starts the original class53 Blast Hornet at32 HP. This bypasses
earlier corridor traversal. No boss admission flag was written.

With only boss HP assisted to1, X's shots miss and native damage kills X.
Zero remains the survivor. Positioning the survivor for an actual Zero shot
reduces the boss to0. The original death, graphics upload and reward finish;
1FBE becomesFF and the native password screen appears. No victory, reward or
exit flag was written. Evidence: `public-boss-server-native-window-result.log`,
`public-boss-server-victory-result.log` and `boss-reversed` captures under
`build-coop/qa-state`, on GUI SHA256 `d55f864b265d71da6c1f97e96575e87ff5485b39e2e56292d347d28589449769`.
That early checkpoint setup had not earned the intro-completion prerequisite
at1FD3. Input at the password returned to the intro, so that setup does not
qualify ordinary stage selection. Native00:9A4B also sets1FD5 for stages whose
clear bit is already set; it is not solely an intro-mode flag. The later
earned-intro campaign and survivor evidence below supersedes this setup gap.
Broader authored targeting and campaign cases remain open.

### Adaptive co-op display and LAN pilots

The co-op compositor samples additional columns from the existing world. Its
panel width does not change the guest PPU margins or activation choices. Exact
state-test SHA256 `c4ee4fcb2439c0b8e55749afe469f424291926d11d40b2691f71d9d677fa3da1`
passes the ROM regression and adaptive view checks on SERVER. A342-pixel
private field preserves central world pixels and has content on both sides.
Two448-pixel split fields show distinct regions. Native, selected, split and
wider local presentations preserve the complete simulation snapshot.

The actual staged Fit default on GUI SHA256
`3bb361945080898f071eea8bcccb13bfafcacf5cf6d80669c205e8da149ebd18`
renders256/342/448 columns at960x720,1280x720 and1680x720 respectively.
Each resumed run gets its own disposable snapshot because the launcher
consumes it. The two wider fields have37/45 and37/47 distinct margin colors
in the same live stage5 scene. See `public-fit-live-server-result.log` and
`public-learner-server/fit-live-*.bmp` under `build-coop/qa-state`.
The same exact state binary used for the reversed learner terrain also
verifies paused wide restoration: it discards the old composed scene and
centers the current native picture until a new wide raster is available.
On GUI SHA256 `bca15ca54154491324fdef257675caa1aca7deed4eea19ab282cdefe5b8bb695`,
`fit-order-boss-wide.bmp` shows Blast Hornet's original OAM meter anchored
at the wider right edge. That is one authored room, not a full visuals pass.

The first net-enabled GUI SHA256
`344f14645e1869aa3785a5a105c8ccce8fd8ad7a15afc4d44da067ab2b89fd98`
links the pinned launcher backend. Its two actual rollback processes agree on
portable profile0 and transcript `0c17fb26` and admit210/211 frames without a
reported desync. Their local layout choices differ. This is a startup pilot;
it does not qualify native controller progression, mixed hosts, rollback
episodes, in-game transitions or disconnect recovery. Evidence is
`build-coop/qa-state/x3-lan-server-result.log`.

The exact-build gate folds the running executable SHA256, original ROM
identity and agreed portable profile into the driver's required content
fingerprint. The matched Windows pair at SHA256
`b97852ff474c49c4c510a21da68e93c3ec91120e5cf3741655b1fba2119a6cec`
admits195/196 frames. An otherwise identical disposable executable with
different bytes is refused by both peers with zero admitted frames. See
`x3-lan-strict-server-result.log`. No cross-build pair is qualified.


## Intro restore and real LAN gameplay

A current-build running snapshot taken before actor enrollment can resume before
RESET settings have run. Restore now applies selected co-op settings only to
that unenrolled running machine; enrolled actor state and frame-zero rollback
remain authoritative. The native opening dialogue then finishes and enrolls
both actors. `intro-resume-fixed-server-result.log` passes on GUI SHA256
`26e122c3d3fc7bbfe7f70ad156b17c38a9a1284f8b88a9a88dd3634677bcc64b`.
The actual opening images show one playable X and one playable Zero.

GUI SHA256 `2819182cc1de4e98d3df599acee4dab035e70b618540569dc80cff3db1c8327d`
passes real PC-to-SERVER LAN gameplay. Both peers agree at ticks3657/3707/3787/3867:
X/Zero positions2656/2656,2656/2656,2656/2587,2710/2587. Local screens use
seats0/1, negotiated portable profile0/transcript0c17fb26, no reported desync.
Evidence: `x3-cross3-local-result.log` and `x3-cross3-server-result.log`.
The original title, intro and dialogue run on both peers; no gameplay state
or completion flags are injected. Two-process online Unified also passes the
same native route and independent input isolation with view-1 on both peers
(`x3-unified-server-result.log`, tick3653 enrollment).

The required session fingerprint now also includes the selected hardware OBJ
quota mode. Matching NoSpriteLimits choices admit209 frames; opposite choices
are refused at zero frames (`x3-policy-server-result.log`). Diagnostic execution
mode, floor and prototype overrides are refused before frame0; delay mode is
refused by the seated-rollback requirement (`x3-refusals-server-result.log`).
An oracle setting is checked only when that backend is actually compiled.
The existing default config bit in a normal build does not block netplay.

Current state-test SHA256
`091bd59c0563dd4dcef219a8ddd1c699194f3ed15cf7394a919127ef28c7f0f7`
passes the learner-both-rosters and adaptive complete-machine comparison
(`intro-resume-state-server-result.log`). Both co-op CTests pass. These focused
results do not close online combat, delayed-input rollback, scenes, menus or
owner checks. All15 final rollups and the full180-point acceptance remain open.


## Current 180-point acceptance disposition

The controlling180 IDs remain unchanged. Four IDs are accepted;176 remain
open (including partially exercised IDs). No final rollup is complete:0/15.
A failed setup is retained as setup evidence and does not establish a game
defect or qualify a gameplay ID.

| ID | Decision and concrete evidence | Identity |
| --- | --- | --- |
| D02 | Passed: original Mac contact kills Zero in both roster orders; living X receives control and defeats Mac with a real buster shot, with full-machine replay. The launcher also destroys the real restraint and completes original rescue/dialogue while Zero stays fallen. Position/health and main-HP1 assists are explicit; no life, defeat or result flags are set by this route. `intro-helper-combat-state-server-result.log`, `intro-helper-clear-server-result.log`. | Statef58e59e0... / GUI0c8d4cb7...; native intro helper handoff fix. |
| F09 | Passed: living P2 learner damage protection and actual stage5 pit rescue in both rosters, unsafe return refusal, P1-death survivor, previously fallen exclusion and deterministic replay; native safe shared catch-up also retains personal state. `intro-resume-state-server-result.log`, `adaptive-learner-final-server-result.log`, `native-shared-camera-server-final-result.log`. | Current state091bd59c...; fixes681a7b8/6e11ade/2171edb. |
| N02 | Passed: real PC/SERVER LAN runs the native title/opening/dialogue, enrolls both actors and admits independent seat movement with agreed world progression and no reported desync. `x3-cross3-local-result.log`, `x3-cross3-server-result.log`. | GUI2819182c...; source1cbcb6f. |
| N04 | Passed: two real processes negotiate Unified, use shared view-1 on both peers and agree on native intro progression and both seats movements. No local presentation option selects a different simulation. `x3-unified-server-result.log`. | Same GUI2819182c... and portable profile0. |

Online delayed transport passes25ms each way and a300ms guest-input hold.
It causes no actual misprediction, so it is not rollback proof. The existing
recomp-net validation injector then produces22 real resimulation episodes
and22 committed repairs on each peer, including living P2 movement. One
late row at3735 was initially outside the open episode and is explicitly
retried and committed in epoch89. No post-fork/lost-correction result remains.
Both peers agree at common tick4200 on X2710/Zero2587 and live stage0; local
first-predicted enrollment ages differ after rollback and are not compared as
simulation clocks. `x3-forced2-server-result.log` passes. The first forced run
failed that unsuitable age/tick comparison; its evidence remains. Online
combat, menus, wipes and scene cases under N06-N09 remain open.

Actual native Maoh clear (HP1 assisted head shot) and original Zero dialogue
now earn intro completion at1FD3 and enter the real campaign. Blast Hornet
then awards its native1FBE weapon and returns to an actual next stage. The
P2 Zero leading final door admits both; a real boss contact with X prepared
at1HP leaves X fallen and Zero alive at20HP. Zero finishes with an original
left-facing buster shot, earns the same reward and enters the next native
stage with X still fallen and Zero alive at28HP. No death, reward, clear or
exit flag is written. Only position/health and bossHP1 are assisted; checkpoint2
bypasses earlier travel. Evidence: `intro-maoh-finish-server-result.log`,
`campaign-hornet-survivor-server-result.log`,
`campaign-hornet-survivor-clear-server-result.log`, `hornet-survivor-*` captures
and slots8/9 retained in `boss-reversed`. The P2 script syntax requires every
button token to carry its prefix; the earlier unqualified Y was a QA input
error. Full G01/G09/F03/F04/S14 acceptance still needs their remaining cases.


The other seven native Maverick clears pass on the same exact GUI. Each
route uses the earned-intro baseline, the original stage loader and checkpoint2
(traversal bypass), an actual P2-led final door, and the original boss at32HP.
Positioned X fires his normal buster with only bossHP1 and personal-health
assistance. Every main actor reaches original primary04 withHP0. Its own
weapon byte changes00→DC, and the original reward/confirmation flow enters
the next campaign stage with X16HP and Zero28HP, both alive.

| Native stage / boss | Main class | Earned native weapon byte |
| --- | --- | --- |
| 2 / Blizzard Buffalo | 52 | 1FC8:00→DC |
| 3 / Gravity Beetle | 59 | 1FC6:00→DC |
| 4 / Toxic Seahorse | 57 | 1FBC:00→DC |
| 5 / Volt Catfish | 58 | 1FC0:00→DC |
| 6 / Crush Crawfish | 54 | 1FC2:00→DC |
| 7 / Tunnel Rhino | 55 | 1FCA:00→DC |
| 8 / Neon Tiger | 56 | 1FC4:00→DC |

Evidence: `campaign-maverick-entries-server-result.log` and
`campaign-maverick-clears-server-result.log` (both child0); unique ready/clear
current-build snapshots, after-shots and transition WRAM plus images under
`build-coop/qa-state/boss-reversed`. This qualifies the eight original arena
admissions/finishing-hit/reward/return paths as subcases, not the entire authored
stage/targeting/capture/survivor contract of G02-G09. Those IDs remain open.


## Native intro roster ownership and actual rescue

The intro's native Zero construction now belongs to the existing Zero seat;
captured X retains his own context. Original03:AC57 transfers the temporary
body after its selector/health writes. The real X rescue is03:AD70, with
construction bounded by03:AD7F/AD87.03:B2A7 is a different stock exchange path
and is not the intro release. Pending native constructions retain the outgoing
contexts through snapshot capture. The existing scene wire has no added bytes.

The original rescue-room door runs inside this intro ownership, keeping X
parked. A retired capture actor's reused slot no longer retains unrelated
enemies. Native Mac(class62/sub1) and restraint(class4C) both reach primary04/
HP80 after actual Zero shots. HP1 assistance survives idle frames first; position
and personal-health assistance bypass prior traversal/combat difficulty. No
capture, defeat, rescue or progression result is written by the QA route.

Exact GUI1bbf7377e6d8cb20025bc5d9db389d78f632fe1358751e0a6d24888a73abec54
passes the native restraint hit, original release/dialogue and roster return:
X remains X at16HP, Zero remains Zero at28HP, both alive, scene cleared.
Evidence: native-rescue-return-server-result.log (child0), following the
original Mac/restraint hits in native-rescue-exact-server-result.log. The first
unfixed release showed native X control but left that body assigned to Zero;
that was a failure, not acceptance. The final roster assertion detects it.

State e525061de96eafbaf84669f2a3020cae522006af91806ca3ae456403c18587a3 passes
both roster orders, original capture/dialogue, X11HP/Zero19HP construction,
actual Zero movement, complete-machine replay and invalid-phase transactional
refusal. It also passes the actual restraint finishing shot and X release in
both rosters. Original03:AEC6 refills X to his own maximum; Zero stays at19HP.
That authored X-only refill is preserved. Both release replays match the whole
machine. The same run retains the learner/adaptive regressions.
Evidence: intro-qualified-state-server-result.log (child0).

Standing action alone is too early for an intro control assertion: the native
CPU can still be in graphics decompression at00:8450..845E. The test requires
actual movement and replays the pending native work. Likewise CC8=FF/04/02/00
is present during actual Mac combat; waiting for CC8=0 there let Zero die from
real attacks. The retained focus run proves a separate unresolved case: after
helper Zero falls, the generic scene fallback projects captured X into the
arena with action66 and no control. G01 remains open for that survivor case,
fallen/absent/stock participation and interrupted-constructor qualification.

The supported recipe emits64 routed variants. A fresh generated tree matches
the compiled source tree byte-for-byte after publishing its two generated
identity metadata updates: intro-qualified-regen-compare-result.log (child0).
The hook postprocessor removes retired routes/includes; its regression test
and all4 CTests pass. The preceding dirty build is not a release. Overall
acceptance remains F09/N02/N04 only:3/180 IDs,177 open/partial,0/15 rollups.


### Living X after the intro helper falls

Death settlement now releases living captured X at the helper's current floor,
before the native camera/pit checks can clamp his old capture position into the
rescue room's lower void. The borrowed native continuation finishes before this
handoff. X keeps his resources; Zero remains fallen. Mac, the restraint and the
original rescue/dialogue still have to finish normally.

The end-of-frame attempt was too late: a low-health helper setup left both actors
fallen. The early balanced handoff fixes it. A simple walk/jump assertion was
confounded by immediate native Mac damage and knockback; the decisive check is
actual survivor combat. Exact state f58e59e05cc2f642504733ea498c9800d1b0cb2867a0909e1e172696e22dad73
observes the real fatal contact at tick202 in both rosters, X alive at11HP and
Zero fallen, then an actual X shot produces native Mac primary04/HP80 after26
input frames. HP1 assistance is verified idle first. The whole native result
replays identically. All earlier intro/rescue and learner/adaptive checks pass
in that same run (intro-helper-combat-state-server-result.log, child0).

Exact GUI0c8d4cb7c8ea3235abd846714e7dbb502c78f0195a29e6cdd3cab5d007a8b726
passes actual Zero fatal contact, X's real Mac/restraint finishing shots and the
original final rescue/dialogue. Final scene0 has X alive16HP and Zero fallen,
selector2/HP80, with no resurrection. Position/health and main-HP1 assists are
used for the shots, not to set the defeat/release. The old1bbf7377 baseline
fails the same low-health route with both actors fallen. Evidence:
intro-helper-clear-server-result.log (child0), intro-helper-baseline-server-result.log
(child1). Failed walk/jump and the erroneous combat-helper syntax attempt are
retained as unqualified setup/check output; they are not accepted runs.

D02 is now accepted for its native contact-death/survivor-control contract.
D01 and the other hazard, continuation, capture and authored-stage criteria keep
their independent remaining coverage. G01 is still open: fallen X through
capture/release, absent/stock participation, interrupted constructors and the
remaining original intro progression/presentation checks are not qualified.
Current total4/180 accepted,176 open/partial,0/15 rollups. All4 CTests pass.
The96.4MiB generated comparison tree was removed after its exact match; its
receipt/result and the current generated metadata remain. No release replaced.


### Fallen X and the original rescue cinematic

The original rescue temporarily constructs an X body even when X has already
fallen. Scene kind9 now keeps that cinematic body separate from the retained
participants until native03:AED9 cleanup. Zero returns at the finished floor
with his own health and identity; X stays fallen. No wire bytes were added.
Native00:E49E maps the actual scene owner's pad while dialogue freezes the world
task, retaining both seats' input history and recorded-input demonstrations.

State 4b404a350d438e732f88bfbd1cce3b2c6ff0a1731aa6d3c510ac51beb0b90039
passes both rosters through actual capture/movement, all-alive and prior-fallen-X
restraint shots/rescue, natural mid-cinematic save/load, whole-machine replay and
transactional invalid-phase refusal. The explicit prior-fallen precondition does
not claim to test a native X death. Helper fatal-contact/combat and the existing
learner/adaptive regressions pass together: intro-owned-input-state-server-result.log,
child0. GUI 341072bd3d78d98cdb0f67d46d8db0137aa84779521578450004e7c88103570b
passes P2-only confirmation with X fallen11HP and Zero alive19HP/selector2/scene0
(intro-owned-input-server-result.log). The same GUI passes real helper death,
X's Mac/restraint shots and P1-only confirmation with Zero still fallen
(intro-helper-owned-server-result.log). Both child0. Earlier P2-only confirmation
on541e4027 fails; the frozen-world input boundary resolves that failure.

The public withdrawal route on541e4027 also completes original Mac/restraint
shots and rescue with Zero remaining absent, returning the sole X to selector0,
16HP/max16 and scene0 (intro-zero-absent-full-server-result.log, child0). Its
intermediate original story-Zero body is a distinct unresolved ownership/rejoin
case; this narrow final result is not G01 acceptance. Constructor interruption,
prior-fallen/stock Zero, full intro progression and presentation remain open.
All4 CTests pass. Fresh supported generation matches the complete compiled tree
byte-for-byte,66 routed variants/6006AOT/223LLE
(intro-owned-input-final-regen-result.log, child0). Accepted total remains
4/180(D02/F09/N02/N04),176 open/partial,0/15 rollups. No release replaced.


### Native X contact death with Zero continuing

D01 now passes both roster orders on GUI341072bd3d78d98cdb0f67d46d8db0137aa84779521578450004e7c88103570b,
commit845b833. The original stage1 route starts with X at1HP; the last living
sample has X710/1135 approaching native class08/sub1 at736/1135,9HP. Real contact
at the next input chunk leaves only X fallen at frame264. Zero remains28HP,
moves independently, advances the world counter exactly30 in30frames and keeps
the original gameplay/freeze bytes clear. After the native ledge is jumped, Zero's
actual buster shot defeats the authored class08 actor (primary04/HP80), with X
still fallen. The shot uses position/personal-HP and idle-verified target-HP1
assistance; no death or defeat result is supplied.

Evidence: contact-x-{normal,reversed}-jump-server-result.log (both child0),
unique fallen/final snapshots and native captures on SERVER net-lan. Reversed
seats are an explicit snapshot roster precondition, not launcher assignment
coverage. The first two follow-up routes stopped before reaching the enemy:
one sampled at the old spawn and one walked into the native ledge. Their child1
logs remain unqualified; the successful route jumps it. Other hazards and broad
continuation/presentation criteria remain separate. Current accepted total is
5/180(D01/D02/F09/N02/N04),175 open/partial,0/15 rollups.


### Capture with an unavailable helper and retained native colors

The capture now owns its original scene even when Zero is already withdrawn or
fallen. Its native constructor releases living X at the helper's entry instead
of enrolling an unavailable Zero or assigning Zero's body/28HP to X's seat.
The outgoing X context is retained at03:AC57, before its original calls load
Zero's palette/graphics. The former03:AC67 boundary was after those calls and
left a red-tinted X before rescue. The earlier boundary preserves X's actual
native colors; the launcher capture now visibly shows blue X.

Exact state 670f25567f74ca5f7622e69089e925fc9a2f8cfe8ec1fbead89fb526236c0bc1
passes all8 actual capture cases: both roster orders with Zero living, withdrawn,
prior-fallen or stock-withdrawn. Retained X11HP/max16/native palette and Zero19HP/
max28/lifecycle are checked; available actors actually move and the complete
machine replays. The original7/8 constructor instructions exercise in-place
runtime serialization/preflight/load before continuing. This is a codec check
inside the real constructor, not a claim of a host-frame CPU cut at that exact
instruction. The same run passes all-alive/fallen-X real restraint/rescue and
whole-machine replay, actual helper death/combat and learner/adaptive coverage.
Evidence: intro-palette-state-server-result.log(child0). The unavailable baseline
e535d4d9 fails with native Zero/28HP assigned to logical X.

GUI 3c2cac76f5005358e783292abfaae88eead1c4dbe14c3f57321ddeb786b19ace
passes actual public withdrawal, capture entry and the full X Mac/restraint
finishing shots/original rescue: X stays X16HP/max16, Zero stays absent/28HP,
final scene0. Position/personal-HP/target-HP1 assistance bypasses traversal and
combat difficulty without writing capture, defeat or rescue outcomes. Evidence:
intro-palette-{native,full}-server-result.log(both child0), native-control image
and unique snapshots on SERVER net-lan. The prior4c8958ba route passed mechanics
but failed the palette review; it is superseded. All4 CTests pass. Generated
recipe/hook routes are unchanged from the verified66-route complete-tree match.

G01 remains open for X already fallen before capture admission and the remaining
story/participant presentation checks. Total stays5accepted/175open/0rollups.


### Actual default pit deaths in both roster orders

D03 and D04 pass the authored stage5 ledge/opening at1184..1263, y2720, in
Unified view with learner and stock restrictions disabled. In each actual
roster, each seat walks38frames into the opening, then native gravity reaches
one original127-damage pit call after14or16frames. Only that actor falls; the
other retains the anchor, no team wipe begins, and24frames of Left move the
survivor in bright native gameplay. Positions prepare the real ledge; neither
fatal-damage nor death/retry flags are supplied.

Evidence: default-authored-pits-server-result.log(child0), state
a4807eaed4b6f1862473ac7c9e25d31705d3f804b61323ec58c0a6dd133ff42f, existing actual stage5 normal/reversed ground
fixtures save6/save2. The test entry reuses the original authored-pit check and
asserts both real rosters and default profiles. These checks do not qualify
independent-view bounds, other hazards or fatal-contact snapshot timing.
Accepted total7/180(D01-D04/F09/N02/N04),173open/partial,0/15rollups.


### Rescue when X fell before capture admission

Actual Zero/P2 traversal with X already fallen reaches Mac and the original
restraint without an active capture scene. Rescue now takes ownership at its
own03:AD70 entry and retains Zero through the temporary X body. The actual
native task at0CC8 is accepted as the source of this cinematic. This route had
never acquired the original exchange token1F45; the borrowed exchange now owns
that token before original Zero exit consumes it. Native cleanup returns it to
zero. The previous path underflowed0toFF and left the return scene pending.

State b60f96b30acd6fb1459e8bb9edfd9bc00f2f4f8e2964ad20c0dc15e8cf54d510
passes6 original restraint/rescue cases: both rosters with living X, tracked
fallen X and untracked fallen X with no capture token. Actual finishing shots,
natural mid-cinematic save/load, complete-machine replay and invalid-state
refusal pass. The same build passes all4 default real-pit cases. Evidence:
intro-exchange-token-state-server-result.log(child0). All4 CTests pass.

GUI 305411a07eb2b78d10e22eb6fc82ddd13225b8abf1dfadb19df452ab17ff3060
passes the exact actual Zero-only untracked restraint fixture using P2-only
confirmation: X stays fallen16HP/selector0, Zero stays alive28HP/selector2,
1F45=0 and scene0. The matcheda5f03bb7 baseline fails with scene6 still pending
and1F45=FF on that same fixture/driver. Evidence:
intro-exchange-token-server-result.log(child0),
intro-exchange-baseline-server-result.log(child1). Earlier full-driver attempts
had one confirmation loop still on P1 and are unqualified; the corrected
observation run establishes the native counter failure separately. No result
or terminal cleanup flag is written by the qualification route.

Current total7accepted/173open/0rollups. G01 still needs final original-NPC/live
Zero presentation qualification; this mechanical repair alone is not that pass.


### Paired menus and shared healing qualification

M01–M07 and R06 are accepted on runtime commit6df79ba. Exact state test
binary e88af430e3d94996695285f4c8ac5251705fc7fb8df927f6fab7604b10a13305
passes paired-menu-current-result.log (receipt child0), using the actual native
normal/reversed stage fixtures and default participation profile0000. Either
character opens the original pause; both panels navigate concurrently while
the shared pools and personal positions stay frozen. X's acquired selection
commits without changing Zero's inventory, and simultaneous Start exits once.

Shared tank races select one deterministic owner. Every tick of a full heal
matches the original native routine; overlapping full/partial operations spend
their own reserves before one compaction. Full-health and empty-reserve requests
are refused without spending. Healing blocks resume while allowing partner
navigation. A real contact-death survivor can pause/resume while the fallen
actor cannot navigate, spend or return. Active healing replays the complete
machine; interrupted native pulse preflight/load and paused pixels also pass.
Malformed menu/CPU-width/phase fields are refused transactionally. Shared,
split and selected panel rendering leave the same machine state.

The shared Escape modal also passes focused owner/cancel/refusal checks, but
M11 remains partial pending the other required shared frontend/save flows.
M08–M10 and M12 retain their independent requirements. Total15/180 accepted:
D01–D04, F09, M01–M07, R06, N02/N04;165open/partial,0/15rollups.

The pending intro observer originally misidentified ordinary green enemy
class20/gfx42 as Zero. An actual native capture establishes that this is an
enemy, not a duplicate story participant. The proposed capture omission and
unneeded broader admission were discarded. The corrected observer watches
native story slot0CC8 with Zero CHR; G01 remains open until final disposition.


### Intro, default spikes and native continuation acceptance

G01 is accepted after intro-roster-final-state-server-result.log(child0) on
state5d78f573682a1ff09bb11867c54f1a19a61556bd462a07133097ca9e5129c1b3,
runtime6df79ba. It passes all8 capture cases, actual Zero-only pre-captor
movement in both rosters,6 actual restraint/rescue cases and both helper
fatal-contact/survivor-combat cases. Actual uploaded descriptors retain one
living Zero; native story slot0CC8 cannot display a second Zero beside him.
Unavailable participants keep their life/resources and X retains his native
identity after rescue. Whole-machine replay, pending constructors and the
retired-X cinematic pass. The exact GUI305411a07eb2b78d10e22eb6fc82ddd13225b8abf1dfadb19df452ab17ff3060
previously passes the actual untracked Zero-only restraint route with P2-only
confirmation and native exchange-token cleanup. Original Maoh/earned-intro
progression evidence remains valid; the later intro changes do not alter its
scene0 path. Teleports/HP assistance bypass travel/difficulty, not outcomes.
The ordinary green class20/gfx42 enemy remains intact. No capture-skip or
broader admission/decoder experiment was retained in production.

D05, D09 and D16 are accepted on the same state binary:
native-spikes-room-server-result.log(child0). Both seats in both real rosters
fall onto the authored stage5 spike row1632..1759/y1472, with the other on the
adjacent1568/y1392 plateau. Default learner/stock profiles are off. Native
fatal127 damage occurs once after22/23 frames, inside terrain pass1 with no
controller pass or pit call. Only the touching actor falls. The survivor moves
1568→1537 in24 Left frames, retains native phase4/4 and brightness0F, and all
shared pool freezes stay clear. The last frame before contact and the actual
fatal result restore/replay the complete machine and exact displayed pixels.
This is a host-frame cut across real contact, not a claimed CPU-instruction cut.
Earlier observation attempts assumed a death action that the native damage
helper does not set. A lower-room camera retained by the first teleport also
pushed the safe actor into terrain; matching the actual upper band corrects
that fixture. Those failed attempts remain unqualified. D08 remains separate:
these lethal floor contacts occur after controller updates.

E11 is accepted on stateb94de829baf0ea064f660b964354422dd3e7a847986d6ec08fc972cfcb53e9c6:
native-contact-register-final-result.log(child0). In each real roster, isolated
native body geometry makes the first or second seat the sole successful
contact. A matched original single-actor call establishes the return. Paired
first-hit/second-miss and first-miss/second-hit calls preserve exact
A/X/Y/D/S/P/DB/PB, the24 scratch bytes, object bytes and victim health. Guest
stacks remain balanced. This qualifies the native continuation contract;
authored targeting/capture coverage retains its separate requirements.
The preceding current-job launcher used incorrect hash/ROM argument indices,
failed before gameplay and was corrected; it is not qualification evidence.

M08, S01/S02 and R01 are accepted by native-contact-state-current-result.log
(child0), statee88af430e3d94996695285f4c8ac5251705fc7fb8df927f6fab7604b10a13305.
Both actual rosters retain original collector eligibility and amounts, remote
weapon-energy ownership, full-health shared banking and competing-drop cleanup.
Original stage1 gate3C:D27C admits the second actor first, follows its native
forced walk/introduction and returns independent control to both. Both-present
admission waits appropriately and transports both without crushing. Native
initialized actor/owned-pool/input/resource snapshots restore immediately and
replay the complete machine. Other gate families, general authored streaming,
stock-unavailable policy and campaign exits keep their own open criteria.

All4 CTests pass. Total24/180 accepted: D01–D05/D09/D16, E11, F09, G01,
M01–M08, S01/S02, R01/R06, N02/N04. Remaining156open/partial,0/15rollups.
No release/delivery replacement or remote action was performed.


### Team retries, stock loss and public participation

T01–T04, T09/T10 and R04 are accepted on runtime6df79ba, with tests prepared
from the actual normal/reversed stage5 ground fixtures. native-team-current-
result.log(child0), stateffcd1c2cad29ce5b3618ed2b5a7c7b15b1150f78ce40811f48fb0156cd82b3e1,
passes simultaneous and both character-ordered deaths in both rosters. The
simultaneous actors fall on the same frame22. Ordered deaths occur at22/45.
The original retry completes at598/622/621 loop ticks, respectively, with one
shared life debit2→1, the requested roster, full personal16/28HP and actual
post-retry control. A snapshot immediately before the final fatal contact
replays death, full wipe, debit and retry to the same complete machine.

native-team-extra-result.log(child0), stateb6484dccaa4c567012154c5529c7fadec719abf86f7a687885dd5a09975607c3,
passes all6 additional cases. Stock Zero actually falls on spikes before X;
retry preserves native permanent-loss bit40, leaves Zero unavailable and
refuses his return while X moves normally. A real95-frame Select withdrawal
followed by the last living X's death reaches a default full-team retry. With
Zero already fallen, X's actual native pit call occurs exactly once and reaches
retry; the authored hole and original lower-room camera are retained. Each
case debits2→1 once, passes complete-machine replay and advances the world
exactly24 ticks during24 post-retry movement frames. These cases do not claim
boss-lock, door-wipe, game-over or every stale transient requirement.

F05/F06/F07 are accepted by native-return-selection-result.log(child0), state
6b424b49ef13da7ac9b088dea6fe219b89904ae1cc03e42ec4bf41cd5da092c5.
Both roster preconditions and both participants retain personal11/19HP through
safe withdrawal/return and actual30-frame movement. X uses real native R input
to select the acquired Toxic Seahorse weapon before withdrawal; selection2 and
its unspent5C energy remain afterward. Native Zero retains selection0. Actual
Select holds withdraw/return with the proper release latch; simultaneous holds
leave one living participant. API and real Select return cannot revive a fallen
actor. Refused airborne placement is transactional; same-side corridor landing,
private-view reset/replay and original acquired armor reconstruction also pass.
The retired preconditions use the lifecycle API and do not claim new authored
hazard coverage. F08's other placement hazards remain open.

S03 is accepted by native-stock-current-result.log(child0), same6b424b49 state.
Stock availability agrees with the executed original menu predicate in both
rosters across15 native stage IDs and6 relevant restrictions. Real Select
honors one call per stage; actual native stage loading resets that call without
enrolling Zero automatically. Real enemy contact earns permanent Zero loss,
which survives the next native stage. Authored Both-policy gates and their
original committed walk/introduction do not wait for dead/absent/stock-unavailable
Zero. Default unrestricted scenes remain unrestricted after stock probes.

F02 is accepted by the existing current native-contact-state-current gate
matrix(statee88af430, child0): either participant is API-retired during original
admission or the following miniboss introduction, then the actual survivor
finishes the committed room walk beyond2552 with camera2560 and native phase4/4.
The retired actor stays fallen/absent, personal survivor HP stays intact and all
locks release. This is real room-transfer persistence with an explicit retired
precondition; authored fatal contact during a door remains a separate open check.

D08 remains open. The ceiling preparation ended in a pit, and the wall contact
produced a fallen actor without establishing the required controller-pass
observation. Those experiments are unqualified and their pending test code was
removed. Native vehicle/pilot fatal paths are a concrete remaining candidate;
ordinary post-controller spike evidence must not be relabeled as D08.

Total36/180 accepted: D01–D05/D09/D16; T01–T04/T09/T10; E11; F02/F05–F07/F09;
G01; M01–M08; S01–S03; R01/R04/R06; N02/N04. Remaining144open/partial,
0/15rollups. New Buffalo survivor/target/reward/next-stage coverage is prepared
but has no accepted result yet. Production source remains unchanged.


### Native physical input and survivor reward ownership

The shared native input alias now follows the living scene owner or survivor
through frozen reward/password screens. Original08:85DC is copied to WRAM
7E:25DC and polls both physical ports. The adapter retains physical P1 current/
previous input in the existing serialized cache before lending native JOY1 to
P2; native JOY2 already retains its own history. Scripted recorded input remains
original. Inactive and pending-arrival snapshots retain valid physical history;
invalid low controller bits still reject before mutation. No new generated
route or wire bytes are needed. Disabling co-op restores physical P1 input.

D07, F03 and F04 are accepted on state
038210f65716aa303c900059bc93154f5f5e8fd5556e2770964677aaee4039a9:
native-modal-input-campaign-result.log(child0). Both roster preconditions use
the actual earned-intro stage2 Buffalo encounter. Real boss contact kills Zero
after23frames; nine original boss updates observe living X, whose real buster
finishes the original boss after6 attack frames. Target HP1 assistance survives
idle frames before firing. Original reward1FC8 becomesDC and the actual stage1
return completes in2553frames with Zero still fallen. Position/personal health
and target-health assistance bypass difficulty, without supplying death, clear,
reward or exit results. Earlier real Hornet X-death/Zero-survivor reward and
next-stage evidence supplies the opposite survivor; a current-build Buffalo
Zero-survivor extension and last-survivor boss retry remain prepared, unrun.
G02's full authored mechanics and T05 are still open.

The previous6f984273 state passes X/P1 but stalls X/P2 at native phase4/10
after6000frames. Exact GUI
bed93772e37d945f2a87459bc5ceb3a53e0ab3271af4a86c7743330eb386a9cc
passes native-password-owner-final-server-result.log(child0) on that actual
stalled state. Fallen P1's B is ignored; its physical8000 history survives an
inactive-scene snapshot and actual load. Living P2 B reaches stage1/rewardDC;
30 Right frames move native blue X384to423 while Zero stays fallen. The TCP
capture native-password-owner-final-moving.bmp was inspected. The first GUI
attempt failed the driver's startup counter assumption when loading a state;
the corrected driver treats the initial restored counter as a load boundary.

native-modal-input-regression-result.log and native-modal-input-intro-server-
result.log both exit0 on the same038210 state. They retain full paired menus,
shared healing, stock rules, public return, all12 team-retry cases, native
contacts/pickups, stage1 gates, independent movement, native support and stage5
capsules. All12 pending-stage cases also save/load actual nonzero physical P1
input and reject malformed histories transactionally. Intro capture, unavailable
participants, sole Zero approaches, all6 restraint rescues, helper death and
complete-machine replay pass. All4 CTests pass; the66 generated routes remain
unchanged. The separate native-modal-input-arms-server-result.log also exits0 on that
same state: both rosters preserve the exact411-frame native arm demonstration
and all12 opening/award participant-retirement cases.

M09 and M10 are accepted by that combined current result: actual native heart
class0B and subtank05 allocation, original eligibility and timers run in both
rosters. Simultaneous contact grants one original X-eligible award, with one
shared tank or two additional X health points; native Zero retains28HP. Both
actors stay parked under competing movement/jump input, native cleanup releases
every freeze, and pending acquisition replays the entire machine. Explicit
API-retirement subcases preserve earned awards and personal resources; those
subcases do not claim an authored death. Native capsule4D simultaneous-contact
checks retain one original X upgrade task and exact recorded demonstration.
Other capsule variants remain part of S08/S09's open authored coverage.

N11 is accepted on that exactbed93772 GUI by
native-modal-input-compat-server-result.log(child0). Two actual peer processes
with identical bytes start portable profile0 and reach207/202frames; changing
one executable's bytes rejects both at0frames. Matching sprite-limit settings
reach198frames; differing simulation settings reject both at0frames without a
build mismatch. Forbidden diagnostic execution/floor/prototype overrides and
delay mode all refuse before frame0. The capability policy source has remained
unchanged since1cbcb6f; N12 remains open for explicit capability qualification.

Total42/180 accepted: D01-D05/D07/D09/D16; T01-T04/T09/T10; E11;
F02-F07/F09; G01; M01-M10; S01-S03; R01/R04/R06; N02/N04/N11.
Remaining138open/partial,0/15rollups. No release/delivery replacement or remote
action was performed.


### Exhausted retries and native game-over recovery

T08 is accepted on runtimeaef7666 / GUI
bed93772e37d945f2a87459bc5ceb3a53e0ab3271af4a86c7743330eb386a9cc:
native-gameover-final-server-result.log(child0), both real stage5 rosters.
Actual actors fall together on the authored spike row with native lives0.
Native phase4/6 reaches the original0A/0/0 password/game-over screen around
frame270 and stays there without another retry. Earned-intro1FD3=1 and
Ice-weapon1FC8=DC are supplied progress prerequisites for this recovery test;
the test does not claim their acquisition. Their native password is displayed
and they remain intact after actual continue and stage selection. The wipe
owner's real Start/B input resets native lives to2 and reaches actual stage1
play, with both original characters alive at16/28HP in their original seats.
Actual30-frame Right input moves X in each roster. Terminal password and
post-continue TCP images were inspected; they show native blue X/red Zero and
full personal meters. No damage/death/game-over/continue result is written.

The initial observation script confirmed only with the non-owning P1 B, so
its child0 records an observation, not T08 qualification. The next attempt
used the actual wipe owner but asserted enrollment at the first phase4/4,
while the original entry warp was still running. Its child1 is unqualified.
The final driver waits for the native arrival lock/warp to finish and checks
actual enrollment, roster, resources and control. Native save/SRAM dialogs
are never opened.

E04 is accepted from native-modal-input-regression-result.log(state038210,
child0) and native-modal-input-campaign-result.log(same state, child0): actual
stage1 class08/variant1 faces/detects nearer P2 and matches every enemy byte
of one original single-target update, restoring canonical ownership afterward.
Actual Buffalo updates receive the sole living X in either seat after Zero's
real death; the survivor completes combat. This qualifies a living/nearest
target, not E05's changing attack target or the separate capture/attachment
contracts.

Total44/180 accepted;136open/partial;0/15rollups. AddedT08/E04 to the42-ID
set above. New T05/P01/P04/D14/E01 checks remain prepared, not accepted, while
their local compiler waits for measured capacity. No delivery changed.


### Corrected pit disposition and native rider qualification

D03, D04 and T04 are reopened. The previously selected stage5 opening at
1184..1263 is an authored descent, not a terminal pit: cartridge regionE981
covers x1120..1311/y2704..2815 and commands1E7E=2816 plus1E6E=768. The old
teleport retained a2560 bottom without admitting that controller, so its native
fatal call does not establish a real pit. Keep those raw results as bypassed
setup evidence. They no longer qualify these three criteria. A true authored
lethal opening still needs selection and qualification. The other spike-based
team retries, stock loss and R04 remain independently qualified.

The separated-room test reveals a different real defect: Native04:B5A2 compares
the secondary body with the anchor's top bound. With lower world camera2560
and upper partner camera1280, the upper body is pushed downward until it falls.
Candidate14dc5dcf60ff039d8217e18f04f2cc6be4aec1c4c861cc2401fc9cad367b6319
changes only that comparison's flags at B5A5 to use the secondary view's current
top. The original CMP still executes, retaining A and elapsed cycles. This
fix keeps the upper body1367 alive while the lower body2719 remains alive.
The next lower-room descent correctly stays alive and changes its own camera
limit; the earlier test incorrectly expected death there. D14 is still open
for its actual pit-death coverage, and this terrain repair remains uncommitted
until its remaining positive/regression cases are qualified. No new wire bytes
or generated roots are added.

P01 and P04 are accepted on runtimeaef7666 / state
05002abae389f3bebc3207ca11f5a6f4adb72f15b0661307297b03a3c8350a46,
native-platform-current-result.log(child0). Both actual stage5 rosters spawn
original event3C:E12C/class26 at1224,1464. After45 frames, its complete native
record and movement1434 match one original single-player simulation, with
native X/Zero standing heights1413/1405 and45 world ticks. Each character's
first5 departure frames match the original single-character position and
horizontal/vertical velocity. After30 frames only that rider's latch clears;
the other keeps riding. Zero alone also admits/rides the actual support in
either seat. These results do not qualify other support families or vehicles.

Current accepted43/180;137open/partial;0/15rollups. Relative to44 above,
reopenD03/D04/T04 and addP01/P04. E01's new isolated immunity comparison still
needs its corrected raw-body setup to run: a context projection was capturing
and overwriting the prepared world's hurt flag. The native predicate actually
reads body88/8A. T05's native checkpoint2/singledebit return works, but its
matched position must be compared after the native arrival lock/warp finishes.
The current Zero-survivor Buffalo clear passes both rosters (contact23, shot3,
target6, rewardDC/nextstage2553); actual post-arrival control is being added.


### Genuine pit replacements, immunity and native boss retry

D03 and D04 are accepted again using a different verified opening: stage1
x1104..1199 has empty collision below floor1200 and no command extending its
lower room. Adjacent E1FC changes only minimumY=512 and minimumX=1024.
native-true-pits-server-result.log and native-protected-true-pits-server-
result.log both finish all four Unified pit cases before their separate D14
failure. Both real rosters/each seat walk49frames to x1152; original fatal
damage occurs at y1280/1284, with one native call, only that actor fallen,
correct survivor anchor and actual surviving movement. The protected version
uses native body88=255 to exclude incidental enemy contact; it still executes
the original fatal pit call. No death result is supplied. The rejected stage5
descent evidence remains retracted.

E01 is accepted on stated3489df8dade6ef04115d30ce57e6d26ebd31369268ee650d42cebd6eb405b4f,
native-protected-immunity-platform-result.log(child0). Both actual rosters
exercise all four combinations of personal hurt protection with overlapping
native geometry. Each paired victim matches its original single-body health
and native88 timer; repeated contact also matches each original result. The
first precondition incorrectly used30, then a same-seat context projection
recaptured the unmodified raw body. The corrected setup installs the prepared
body directly and uses the original CB31 absoluteA60/A62 predicate. This is
an isolated native contact contract, without authored-AI/capture claims. The
same successful run retains both-roster P01/P04 and actual subtank acquisition/
interruption. Its short rejoined-collector input window is8frames so native
controller initialization finishes before the existing movement threshold.

T04 and T05 are accepted by native-personal-view-last-pit-boss-result.log
(child0), state43f6474b521ac9bf09cbf394dd469489c6b079daea214ea10ccf8195586eae8c.
For T04, both actors actually fall in the genuine stage1 pit, in both rosters
and either final-survivor seat. One final native fatal call reaches full-team
retry after578 loop frames, debits lives2to1 once and restores actual control.
For T05, actual Buffalo contact first kills Zero; the final X also dies from
real boss contact. Both rosters match the original single-X checkpoint2 and
landing5760,927, release arena locks, debit2to1 once and restore16/28HP. The
paired retry takes599frames versus the matched original598 and replays the
complete machine identically. Earlier comparison failure observed the original
arrival warp too early; both sides now require the native entry lock to finish.

The same current run strengthens F03/F04: real X death leaves Zero to defeat
Buffalo in3 firing frames after23 contact frames; six native target updates
observe Zero. Both rosters finish rewardDC and actual native stage1 arrival
after2585frames with selector2, then move24frames with24 world ticks while
X stays fallen. The earlier2553-frame condition observed the stage phase before
its native constructor finished; it is superseded by this actual-control check.

Current48/180 accepted;132open/partial;0/15rollups. AddedE01/T05 and replaced
D03/D04/T04 with genuine-pit evidence relative to the43 correction above. D14
remains failed/open: even with incidental enemy hurt excluded, its primary
lower actor rises to1075 rather than finishing the real pit fall. The partial
B5A5 comparison patch is removed. A complete native camera borrow now surrounds
secondary controller and terrain passes, reusing the existing serialized64-byte
backup and pass3 with balanced restore; its active runtime codec passes, but
it does not resolve that remaining movement. A bounded native-call observation
and failed-case snapshot are being built to identify the actual source. This
view-context candidate is still uncommitted and needs the broader regressions.


### Fixed startup capability decision — N12

N12 is accepted from the unchanged native capability path and actual two-host
agreement. `src/x3_coop_netplay_host.inc` queries SDL SSE2/SSE3/AVX2/NEON,
advertises the available portable C backend/profile, and fixes the selected
profile before guest frame zero. The actual PC/SERVER gameplay pair at GUI
SHA2562819182cc1de4e98d3df599acee4dab035e70b618540569dc80cff3db1c8327d
agreed on profile0/transcript0c17fb26 and advanced the same co-op world
(`x3-cross3-{local,server}-result.log`). The later bed93772 exact-build pair
records the same agreement before its required identity match; the retained
`build-coop/qa-state/modal-profile-host.log` is its original process log.
Reconstructing the documented ordered capability transcript uniquely identifies
actual CPU masks7/7 (SSE2/SSE3/AVX2), backend1/profile1/architecture1; this is
derived from the exchanged transcript, not inferred from OS versions.
Only the qualified portable profile is offered, so no unqualified SIMD path
is selected. The barrier and identity code have remained unchanged since
1cbcb6f. Rollback retains the fixed agreement; complete executable SHA256
remains required, and actual changed bytes still fail before frame zero.
This accepts the current portable fallback contract, with no cross-build or
unqualified optimized-profile compatibility claim.

Total49/180 accepted,131open/partial,0/15rollups. D14 and the new initialized
rewind checks remain unaccepted pending the queued candidate build/results.


### Personal native camera context, real pits and initialized rewind

D14 is accepted on state test001ef96ef7e89d7da82ca7ca6a9177e915d4826cf4193da0b03d479802aa4bae
(`native-room-limits-receipt.json`,child0). The secondary controller and
post-enemy terrain borrow the complete native camera context, then restore
the anchor context. This prevents retail terrain correction from pushing an
upper-room body down according to the lower anchor camera. The existing
serialized camera backup and pass3 retain this context without adding wire
bytes or routed roots. Actual active projection preflight/load passes; unknown
pass values, missing independent policy and orphaned camera backups fail.

The original single-X reference confirms that native stage1 class05 at1152
carries the earlier walking approach (rider81/playerY1075). That rise is native
support behavior. The corrected pit setup keeps that support alive and places
the actor below it at the real stage1 terminal gap; gravity and native damage
select the outcome. Both actual rosters and either lower-room seat survive
60frames in separated1024/256 camera bands, then the lower actor makes one
native fatal-pit call after19X/21Zero frames. The upper survivor retains full
health, moves1696->1726 under24frames of real input, remains bright at04/04,
and no team wipe occurs. Teleports explicitly match native horizontal and
vertical limits; stale stage1 horizontal limits caused an earlier fixture
movement failure and are not a runtime regression.

R08 and R10 are accepted from the completed initialized-rewind subcases in
`native-rewind-supports-result.log`, state SHA256
a945b8f0ae421b0b2d2f040a3fb0e29f841ee72a880378862828531f3809a8b0.
Both real rosters move and create both real native buster pools, then the
actual rewind ring restores six frames to an exact complete-machine snapshot.
Replay reproduces the entire discarded movement/combat future and exact
rendered pixels, including animation/CHR/projectile history. A later separate
support reference fails in that same command; it does not erase the completed
rewind cases or qualify P02.

P03 is accepted by `native-landing-carry-receipt.json`(child0) on001ef96e.
For each actual X/Zero seat in both real rosters, every one of45 approach,
landing and carry frames matches that actor's original single-player X/Y.
Native class26 at3C:E12C moves once; original landing really occurs. Existing
P01/P04 displacement, independent release and original jump momentum remain
passing. P02's second authored class26/variant84 reference remains open.

Actual53/180 accepted,127open/partial,0/15rollups. Four lightweight CTests
pass; broader affected native scenes/resources/views and the new GUI remain
in qualification. No package delivery or remote action has occurred.


### Authored independent riders, shared modals and complete camera regression

P02 is accepted by `native-support-child-receipt.json`(child0), state SHA256
a01bd7a0593a73b9451822cfcc008605fc0f9b2ec5cce6195392984b78f51cb9.
Original stage5 records3C:E12C (class26/variant2 at1224,1464) and3C:E1BB
(variant1 at1480,1944) create two actual moving supports in separate1280/1792
camera rooms. In both actual rosters, every45 paired rider X/Y samples equal
that actor's original single-player reference; each rider history belongs only
to its actual support, and45frames advance45world ticks. Both native support
records match their references after resolving the allocated trail-child slot.
Native07:E649 stores that shared allocation pointer in3E; two supports allocate
different slots. Their complete32-byte class1E child contents match the original
references and retain the correct native parent pointer, so this comparison
does not discard a behavior mismatch. The previous variant84 reference is
stationary under its original campaign prerequisites; its gating at07:E49B
checks completed-boss and encounter flags and remains separate authored work.

M11 is accepted from the current `native-room-limits-regression` child0 Escape
checks plus the unchanged input/modal path's actual password/game-over proofs.
Both real rosters choose one simultaneous Exit owner; another seat cannot
confirm, the owner can cancel, the other living seat can later own it, and
original native eligibility/refusal and actual world-draw/stage-exit execution
remain intact. Actual `native-password-owner-final-server` proves fallen-seat
input refusal and living-X confirmation in either seat, including saved input
history and actual resumed movement. `native-gameover-final-server` proves
coherent continue/full-roster recovery and retained progress in both rosters.
These GUI proofs used the same aef7666 input path as the current candidate;
shared modal phases exclude the newly added personal camera borrow. X3's
native progress route is its password/continue flow (cartridge RAM exponent0),
not a native SRAM file picker. No save/import dialog was opened.

The full affected `native-room-limits-regression` receipt exits0 on001ef96e,
covering native scenes/capsules, physical input histories, contacts/items,
stock participation, public withdrawal/rejoin, menus/healing, supports,
independent worlds and team retries. `native-personal-view-intro-server` also
exits0 on c674939b80942fe24eb9abc3de1d325690cb87f4cc1daac217c158d5e6eb6529:
capture/rescue, unavailable actors, helper death and complete-machine replay.
Arms-capsule qualification is running separately on the same runtime.

The actual new GUI SHA256
83de8aff898a449ead8013df4794b0221c304704b12f73c09c5e55485947f498
passes `native-personal-view-gui`(child0). Its muted TCP-only split check
retains two native rooms, then a position prerequisite below the live bridge
lets native gravity/fatal damage kill X. Zero retains28HP/y423 and moves
1696->1726 while X stays fallen. Both-room and survivor-moving BMPs were
visually inspected in `build-coop/qa-state/personal-view-gui`; full presentation
and owner controller approval remain open. No desktop focus or saves changed.

Actual55/180 accepted,125open/partial,0/15rollups. No release is delivered.


### Both-roster activation, simulation isolation and camera-state replay

A06/A07/A08, R07 and N05 are accepted with
`native-activation-leader-receipt.json`(child0), state SHA256
727cb5ac43279f641a45976d77579202c11b619ae5ecb65f778ccdd74fb50524.
Both actual rosters explicitly install seat0 as native world leader, exercising
a Zero-led world in the reversed roster. The first reversed comparison failed
because its original fixture had seat1 as anchor while this helper assumed
seat0/P2 ownership; its copied reference and live bodies differed. Selecting
and installing the same real leader fixes that fixture prerequisite, with the
original camera-coordinate/current-limit assertions retained. No runtime
change was needed for this comparison.

Actual stage1 native event records near736,1136 and1840,1056 are admitted,
while records1104,1198 and1232,1184 in the empty inter-view gap remain absent.
Repeated allocator calls preserve all enemy records; a real P2-only enemy is
retained by native culling, while moving it into the gap is rejected. An actual
pending native allocation retains its frontier/visit continuation and restores
the same allocation order. The separate authored support comparisons also
match exactly one native object update with both riders and overlapping views.
These establish A06-A08's union/overlap contracts; teleports and direct native
column calls bypass traversal, so A01-A05 remain open.

With both real actors and authored regions active,30world frames advance once
and retain the remote enemy. Complete-machine restore and another30frames
reproduce cameras, visit history, world actors and native phase exactly. Together
with current actual gate/capsule scene-owner snapshot/replay results, this
accepts R07. Actual selected/split rendering leaves CPU cycles and WRAM intact,
and six local presentation/layout/width choices produce identical complete
simulation snapshots after10frames in each choice. This accepts N05; full
peer latency/rollback and remote transitions remain separate open checks.

The current-runtime arms-capsule regression also exits0
(`native-personal-view-arms-server-receipt.json`,c674939b state). Both roster
assignments retain the exact411-frame original recorded demonstration and all
participant-retirement cases; the broad and intro gates remain child0.

Actual60/180 accepted,120open/partial,0/15rollups. All final owner/presentation,
remaining authored campaign, online behavior, performance and package gates
remain required. No delivery archive or remote repository was changed.


### Native simultaneous firing and independent charge

C01 and C02 are accepted by `native-concurrent-combat-receipt.json`(child0),
state SHA25616152ad5f15dfe0453d56d3452e5f42fa7127d1fe9a276e34c3955ec1ba5c994.
Both actual rosters run240frames per scenario: simultaneous rapid taps,
staggered full-charge releases, P1 charging beside P2 taps and the opposite
assignment. Each actor's original single-player reference runs from the same
ground/camera/input prerequisite. Every command, native charge-counter pair,
64palette bytes,480auxiliary bytes and576projectile bytes equals that actor's
original trace on every paired frame. Simultaneous taps reach the original
three-shot active limit in both seats. Staggered/asymmetric holds reach the
actual original X/Zero charge levels without arming, cancelling or recoloring
the other actor. Every240-frame result also reproduces the complete machine
from its starting snapshot; the native world ticks once per frame.

This isolates native combat by clearing shared hostile/item slots and setting
local spawn-visit prerequisites, with both actors on actual stage1 ground at
x384. It does not claim authored enemy/boss behavior, persistent acquisition,
Zero's complete saber chain or nonzero hit-history retention under other weapon
pressure; C03-C12 remain open. No runtime correction was required.

Actual62/180 accepted,118open/partial,0/15rollups.


### Transactional current-build snapshot rejection

R09 is accepted by `native-transactional-state-receipt.json`(child0), state
SHA256440dee4bb7f212fdc2f69091836454bef8e35a43f86ff5f28ce510948d53eacc.
Both actual initialized rosters exercise20 concrete malformed fields through
the real snapshot loader: actor current/anchor/lifecycle/character/maximum HP,
controller/terrain passes, scene owner/kind/physical history, participation
profile, view policy/mask/coordinates, spawn count, unknown/orphaned camera
backup, presentation identity, rider bits and menu header. Four additional
actor/view/menu truncations are rejected. Every refusal leaves the entire
413017-byte machine snapshot unchanged. Actual active personal-projection
codec validation separately accepts supported pass3 and rejects malformed
pass/profile/orphan combinations. Current-build correctness is covered; no
old-state conversion or backward compatibility requirement is introduced.

Actual63/180 accepted,117open/partial,0/15rollups.


### Active authored rider and lifecycle snapshot ownership

R05 is accepted from `native-support-replay-receipt.json`(child0), state
SHA256058a669d476f45d016757be00ef1118f711986742b7ffae130f0ac35d672a3e5,
combined with the qualified current-runtime arrival, public withdrawal/rejoin
and actual intro-capture restore/replay cases. Both real rosters save while
two actors actually ride separate authored moving supports. Immediate load
restores the complete machine, native trail children, private cameras and
rider ownership exactly.24frames of actual jump/departure and partner movement
then replay the entire machine and both ownership histories identically.
The broad actual participation/arrival checks retain pending input/hold state;
intro capture/rescue/helper cases retain the actual captured actor and script
owner through restore. No checkpoint/rejoin revival or old-state conversion
is introduced. R02/R03 interrupted-pass replay remains open.

Actual64/180 accepted,116open/partial,0/15rollups.


### Interrupted native pass rollback

R02/R03 are accepted from native-midpass-support-receipt.json(child0),
state SHA25600c5b1a9fe4a0791d87d3921ff8d7355de43c237ffe27ee1afae2e415b2c8615.
Both actual rosters capture a complete rollback blob during the secondary
048000 controller,04B58A terrain,04CB31 contact and04BFAE support passes.
Contact actually damages only the secondary actor; support actually acquires
only that rider. Restoring the cut and running the original native continuation
retains exact architectural registers, balanced guest stack, master clock and
complete canonical guest/title state. Caller APU execution scopes0 and1 survive
restore. The engine correction preserves this caller role instead of restoring
an interpreter-active flag into an external call; guest APU timing remains
serialized. The former canonical discrepancy was IBRS+1026, exactly that role.

All10 rollback criteria now accepted; actual66/180 accepted,114open/partial,
1/15rollups. Other feature/visual/campaign/online/release gates remain open.
