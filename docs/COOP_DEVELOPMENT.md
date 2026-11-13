# X3 co-op implementation and qualification

Development branch: `feat/x3-coop`, based on X3 master `11fd733`.
Current pinned build reference: engine `614694ab`, UI `cf6315ad`.
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
  before its global freeze; the remaining authored death and scene cases are tracked below.
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
full campaign, remaining lifecycle and presentation qualification remain open;
accepted online checks are listed below.

Full campaign is the default Zero policy. Stock restrictions start him
withdrawn; his Select control calls him when the original game permits it.
Stock encounter return and permanent loss are implemented and have focused
native-route checks; the complete campaign suite remains open.

## QA burndown

**Accepted touch points: 114/180; 66 open/partial. Accepted areas: 3/15
(snapshot/rewind/rollback, menus/resources and netplay).** Component checks and a successful build are not
campaign or co-op gameplay acceptance.

| Area | Status | Acceptance |
| --- | --- | --- |
| Individual death | D01-D05/D07/D09-D11/D14-D16 accepted (12/16); other death criteria open | Either survivor retains play and progression, with no global fade, blanking or stale freeze. |
| Team wipe | T01-T05/T08-T10 accepted (8/10); boss/door wipe, transients and game over remain open | Simultaneous/staggered deaths produce one native retry and the intended life cost. |
| Fallen persistence | F01-F09 accepted (9/10); reconnect/options persistence remains open | No checkpoint, stage exit, voluntary rejoin or menu revives a fallen player before team wipe. |
| Separated spawning | A01/A03-A04/A06-A08/A14 accepted (7/14); other frontiers, lifecycle and campaign families open | Either leader activates native enemies, supports and one-shot events without duplicate visits or cross-gap activation. |
| Supports/hazards | P01-P05/P07-P08/P10-P11/P14-P16 accepted (12/16); other support, terrain and vehicle contracts remain open | Both riders retain support, momentum and crush behavior while each platform advances once. |
| Enemy interactions | E01-E04/E11 accepted (5/14); remaining contacts, target switching, captures and attachments open | Targeting, contact, capture and release recognize either actor and clean up victims. |
| Combat ownership | C01-C05/C07-C10/C12 accepted (10/12); armored parts and audio remain open | Concurrent attacks retain native damage/cooldowns and clear on lifecycle exits. |
| Doors/scripts | S01-S03/S08 accepted (4/14); all nine capsule families retain native X ownership; presentation, other scenes and authored scene deaths remain open | Either/both door policies and native scripts complete with either survivor. |
| Campaign branches | G01-G11/G13-G14 accepted (13/16); fortress traversal, final phases and endings open | Intro, eight stages, optional encounters, fortress, refights and endings complete with either roster. |
| Menus/resources | M01-M12 accepted (12/12); native scene/death/restore overlap gate passes | Paired navigation, shared tank spending, healing and resume cannot leak inventory or input. |
| Visuals | Real Fit fields and independent width-isolation checks pass; full visuals/audio remain open | Both views retain correct art, lighting, HUDs, Cx4 effects, margins and transitions. |
| Rollback/state | R01-R10 accepted (10/10); complete native interrupted-state, restore/replay, rewind and malformed-input gates pass | Complete initialized co-op snapshots, mid-pass restore, rewind and input replay agree, and malformed state is rejected. |
| Netplay | N01-N12 accepted (12/12); actual remote checkpoint, gate/script and reward transitions pass | Two processes and LAN qualify shared/independent views, rollback, menus, deaths and disconnects. |
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
| 3 / Toxic Seahorse | 59 | 1FC6:00→DC |
| 4 / Tunnel Rhino | 57 | 1FBC:00→DC |
| 5 / Volt Catfish | 58 | 1FC0:00→DC |
| 6 / Crush Crawfish | 54 | 1FC2:00→DC |
| 7 / Neon Tiger | 55 | 1FCA:00→DC |
| 8 / Gravity Beetle | 56 | 1FC4:00→DC |

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


### Shared native weapon children and full Zero chain

C03 passes native-saber-window-receipt.json(child0), regenerated state
SHA25635c38c8686f6d8cd64005cd14fba49d15ee87d32e09fcad62ec4130b91b49973.
Both rosters execute420frames of full native Zero charge, double buster and
saber beside active native X attacks. Every frame retains original command,
charge, readiness/counters, animation, palette and complete private pools.
The actual04:CB74 damage predicate against fixed native geometry has the same
13 saber damage frames303..315 in both rosters; the swing starts292. Probes
restore the original timeline, and the entire concurrent future replays exactly.
The previous paired swing began276 because shared32-byte weapon children
updated the canonical actor's AA counter after personal projectile processing.
D859 allocation now records its real creator; DB07/DB2C update each shared
child once with that context. No counter/timer adjustment supplies the result.
The40-byte owner table is serialized and preflight-validated; direct native
slot reuse resets ownership and rendered/culling descriptors retain the creator.
Strict full regeneration is byte-identical with71 routed variants. Four CTests
and all eight original concurrent firing/charge cases pass on this binary
(native-effect-base-combat child0).

D11/C12 pass native-retirement-caller-receipt.json(child0), state
SHA2568caafdd026ead55aa3fb28887a82a039202fcd8763fcfc2c11c79abc7e06bc46.
Both rosters/either victim take real fatal native contact during held charge
and after charged beams create actual shared child effects. Only the victim
falls; all its private damage sources and shared children retire. Survivor
health, charge, pools, palette and every shared child allocation remain exact.
Subsequent24frames retain the bright living survivor and24 world ticks. The
first follow-up failure was an injected-call fixture defect: its direct-page
register remained on the enemy slot when the outer LLE frame resumed. The
corrected test restores that outer caller while preserving actual elapsed time.

Actual69/180accepted,111open/partial,1/15rollups. Broader affected regression
and armed-charge withdrawal qualification continue before local integration.


### Native attack cancellation on withdrawal

The shared-effect correction passes the affected combined native regression
(native-effect-regression-receipt child0, SHA25635c38c8686f6d8cd64005cd14fba49d15ee87d32e09fcad62ec4130b91b49973),
including actual contacts/pickups/doors/capsules, paired menus/healing, physical
input history, stock participation, safe return, supports, independent cameras
and team retry. R02/R03 eight real continuation cuts, R09 both-roster21 malformed
fields(including new owner values) plus4 truncations, and actual separate-rider
replay pass on the40-byte owner-table format, raw413057bytes.

Additional armed-withdrawal checks exposed native state left behind when the
pools were discarded: Zero B7 remained2, X AA/7D remained2/1, and X35 remained3
with the native firing limit67 also3. The same saved X body fails to fire in
original solo, confirming a retained native capacity count rather than an input
policy issue.04:A412..A416 rejects that full count before allocation.
Cancellation now clears35/7D/AA/B7 together with their discarded pools and
charge fields. It preserves native recoil/timers, resources, selection and the
other actor. native-retired-counters-receipt(child0), state
SHA2562c6be347a313e85183b30d04844b89de0af731511e48d85d51af2759304a43dd,
passes all eight actual charged-death cases plus six armed-charge/active-X-beam
withdrawals and safe returns across both rosters. Each returned actor fires a
fresh native buster on its first requested frame; Zero must earn his combo
again. Four CTests pass. C10 remains open for complete weapon/transition
qualification; this does not increase the69 accepted criteria.


### Full native shared-effect pressure and creator reuse

C07 passes native-effect-width-receipt.json(child0), state
SHA256d3cebb7a32d75b869324e5fdc65de66a823dacf79cf8a4b2a3f7ce9a84f590bd.
Both actual rosters retain live native busters in each private pool while real
02:D9A8 weapon-child constructors fill all40 shared32-byte slots. Existing
three world effects remain; competing creators allocate19/18 children. Full
allocation refusal preserves every native effect record, both creator tags/
counts and both complete live projectile/hit-history pools. The full machine
restores exactly under pressure. Living withdrawal releases only19 of its
own children; the other creator and world records remain identical. The
survivor reuses all freed slots with new ownership; actual unowned D859 reuse
removes a previous personal tag. Construction bypasses normal firing admission
to exercise the native pool limit; native per-actor firing limits remain
covered by C01/C02. The first setup used an8-bit caller X: the constructor's
PHP/PLP correctly narrowed its return. Using its native16-bit index contract
returns the actual slot and passes the complete check.

The final cancellation production code also passes the existing SERVER intro/
capture/rescue/helper-death suite and full arms acquisition/demonstration/
retirement suite: native-owned-intro-server and native-owned-arms-server
receipts child0, stateSHA2562c6be347a313e85183b30d04844b89de0af731511e48d85d51af2759304a43dd.
The actual411-frame arms demonstration matches the original action trace.

Actual70/180 accepted,110open/partial,1/15rollups. C04/C10 complete weapon and
transition qualification, other campaign/visual/online/runtime criteria, owner
validation and release delivery remain open.


### Acquired native X weapons and actual built combat capture

C04 is accepted across all nine native weapon slots(eight boss weapons plus
Hyper Cannon), normal/charged or native automatic-charge behavior, and empty
meters in both actual rosters:54 scenarios. native-acid-active child0, state
SHA256c7fc0edb7d5ce8a2ad8ebe409154601afb9fb38ecf124886a6197e500be1301c,
covers native weapon slot1 normal/charged; native-weapons-rest child0 on the same binary
covers slots2..9 in all three modes; native-acid-empty child0, state
SHA2562dca831d7bb3d0205fd8d0f6e255786a4d0468c63d037de5f3a2ddb37059ee6b,
completes native slot1 empty meters. Native input refuses unearned selections,
selects the acquired slot for X while Zero retains selection0, and preserves
original per-frame commands, charge, palette, all private pools, energy and
damage outcomes. Every complete paired future replays exactly. Charged specials
actually reach native charge8; Hyper Cannon uses its native automatic path.
Normal/charged native slot1 spends10/3 and deals10/3 in this controlled target
setup, identically in each roster. Empty meters spend0 and produce no damage.

The earned-intro Buffalo fixture supplies the real initialized target. Armor/
weapon flags and protection are prepared prerequisites, not acquisition proof.
Enemy AI alone is frozen through retail1F27; the first setup incorrectly used
1F25, which freezes player shots/effects, and is unqualified. Motion/charge
checks reject that setup. Target health127 and cleared hurt cooldown before
each frame isolate native damage amounts; this bypass does not accept C05 or
boss AI/campaign criteria. Original Buffalo damage row19 at06:E771 and
04:CFEC's deflection response explain native zero responses for the relevant
projectiles; both normal and charged Gravity Well retain native zero damage
to this boss. Those outcomes are preserved rather than replaced by new damage.

The committed runtime also passes actual muted TCP GUI combat on executable
SHA256ff854fbc5b822af070e1712fab3163560216db29e9ef4ad5e22b553a9d071e0c:
native-owned-visual-full receipt0,444frames/7.4s at60fps, independent movement
followed by the full420-frame concurrent charge/combo cycle. Both actors end
alive, full health and idle. Charge/saber/recovery keyframes were inspected;
the native video and keys remain in qa-state/owned-visual. This is supporting
visual evidence, not acceptance of the complete V14/human checks.444 intermediate
BMPs remain there because native shell cleanup was rejected before execution;
retain them for now and remove through an allowed audited cleanup at completion.

Actual71/180 accepted,109open/partial,1/15rollups. C10 complete weapon retirement
and transition coverage remains open.

### Complete native weapon retirement — C10

C10 is accepted across both actual rosters, all nine native X weapon slots and
normal/charged-or-automatic casts, for each of withdrawal, fatal contact, native
stage loading and team retry:144 lifecycle cases. native-weapon-lifecycle-gate
passes all36withdrawal cases on state SHA256
80c2075015d004ec2bb22ab25775cd8b4bd1f303d2c7bb8247dc5aabf8573445; its later
retry assertion failed before observing the final victim's next native settlement
boundary and is retained. native-all-weapon-transitions exits0 on state SHA256
14bf9e0a500f0453746eb34b0c1a0148facc9708f38782d5306d0e8dfccc25dc:
36fatal-contact cases,36native stage-constructor cases and36actual team retries.
The last survivor's actual HP0 is observed through the next original controller/
stage boundary; no death, wipe, constructor or retry result is supplied.

Native charged Parasitic Bomb exposed a stale orbit count:04:B753's four real
projectile allocations increment body35/7D/89 to4/4/4. Discarding those pools
previously retained89=4. Actual cancellation now clears89 with its discarded
shot/child counters. This reset is restricted to cancellation and does not alter
ordinary native attack timing. Only the departing creator's full pools/shared
child tags are removed; the other actor's complete sources, HP and resources
remain intact. Public return retains selection and energy, emits no idle ghost
attack, takes actual native contact damage after explicit test protection expires,
and casts the same weapon again. Harm and firing checks branch from the same
unchanged idle snapshot, with legitimate native cooldowns allowed to expire.
Earlier mixed post-hurt firing fixtures remain unqualified setup evidence.

Fatal cases use original04:CB31 and low-HP/contact-position prerequisites; they
retain a living partner and24actual subsequent world ticks with no retired
damage sources. Stage cases bypass prior travel through the existing native
stage request, then exercise the real constructor and cleanup; they do not
accept an authored exit. Both-roster Parasitic examples complete stage5 entry
in357frames. Team retries use two actual native fatal contacts, preserve the
original stage2 checkpoint and debit one shared life, then rebuild both native
bodies with full health, cleared attack readiness/counts and real fresh busters.
Parasitic examples take598frames after the final fatal boundary. Existing native
Zero armed-charge/beam-death and withdrawal cases supply his independent combat
lifecycle coverage; native X3 Zero does not receive special weapons.

Actual72/180 accepted,108open/partial,1/15rollups. C05 concurrency/cooldown and
C09 broader lifetime/hurt/movement checks retain their separate open criteria.

### Native boss hit windows and projectile contact ownership — C05/E02

C05 and E02 are accepted on native-boss-native-window, child0, state SHA256
c21f6587e1d4d9d45f7f76af9ea5fc7c0ece312207f8a88306076e8d7ee95df8. Eight cases
cover both actual rosters, either first-contact/nearest actor and concurrent
normal or charged native busters. Actual native input creates both real shot
pools; collision positioning and target-health127 are prerequisites. The
competing actor is positioned farther away so the reference actor remains the
actual nearest AI target. Prior travel/unlocks are supplied by the existing
earned Buffalo encounter, and difficulty is assisted; this is not G02 coverage.

Two complete original single-actor world frames establish the native first-hit
response. Paired frames retain the same damage and immunity, with the winner's
complete projectile/hit-history pool matching the original. Normal hits deal1
and charged hits2 in every case. Both actors' real attacks are present, rather
than a fabricated hit flag or substituted pool. Existing C03/C04 native timing/
contact comparisons cover Zero's saber and acquired X weapons under E02 too.

The original boss uses body37, not generic body30, for its hit window. Native
03:CA8A installs60 and changes damage row28 to1;02:DF64 decrements37 once per
world update and restores the original row from1F57 at expiry. The first two
world frames leave59; every remaining frame retains the same HP, decrements
the real timer exactly once and eventually restores the original damage row19.
The full interval takes61frames including initial contact, without a second
actor bypassing immunity or adding damage. Every complete paired future,
including both private pools and native boss state, replays byte-for-byte.

Earlier direct-CB74/generic30 probes remain unqualified setup: neither a bare
damage call nor reading the wrong field establishes native boss immunity. The
final test uses complete native world frames and the actual authored response.
No production cooldown, damage or targeting rule was changed for this result.
Actual74/180 accepted,106open/partial,1/15rollups. C06 armored/part responses,
E03 consumed hostile projectiles and complete authored boss mechanics remain
separate open checks.

The a72c1b4 runtime also passes native-retired-weapon-copy-gui(child0) on current
GUI SHA256e6f0e00e489cd8bda19ea5c2e222e749ad636bb79ef1a2f9f93eb029f2e38f74.
A real charged Parasitic fixture has4orbs at export and1remaining after the
first3resumed GUI frames. Actual P2 Select withdrawal/return removes only X,
clears35/7D/89 and all old private sources/tags, preserves HP16/28, selection4
and exact energy, returns X safely beside Zero, and accepts a fresh native cast.
Enemy/hostile freeze is applied only after public return to isolate fresh fire
from encounter difficulty. Before/returned BMPs were inspected. This supports
current built lifecycle behavior without accepting V14/Z03 or a four-orb-at-
GUI-cancellation claim; the latter is the existing immediate CPP evidence.
The initial strict GUI count assertion and a subsequent consumed-resume-file
setup failure remain in their logs. The repeatable script now preserves its
actual source and launches an expendable copy because --resume-state consumes
its input. All four images/states stay in qa-state/retired-weapon-gui; no owner
saves/settings or desktop focus were changed.

### Persistent Buffalo ice and actual consumed hostile shots — E03

E03 is accepted on native-consumed-hostile-center, child0, state SHA256
9e0bf60884376758dc079c35a04702e40d1f74099129832fbb47f15f4a169c83. Both actual
rosters run the authored stage1 class08/variant1 enemy from save8. Safe player
positions/protection, an empty player attack pool and the native idle/next-state
prerequisite prepare the encounter. Original AI/animation emits a real1318
class02 bullet after49frames. Its original04:EA85 update calls CB31 at EB00;
a successful hit branches to actual retirement at EB0A. Collision positions
and expired explicit protection then let each original actor supply its own
reference: X loses2HP to14/16, Zero to26/28, and the actual source enters1/4/0.
Either first-contact actor in both rosters receives that original result while
the partner retains full health. The complete source record matches exactly
one original native update;24subsequent frames cannot turn it into a delayed
partner hit, and the full25-frame machine future replays exactly. Initial
idle-next0 and small-bullet Y+16 preparations were unqualified setup; native
constructor idle-next2 and actual collision-center placement resolve them.

The earlier Buffalo source is a different native contract. Original AI emits
1318/class20/variant0 after597frames in both rosters, without supplied spawn or
attack flags. The hostile table00:F8D8 resolves it to08:9CE6; the regular NPC
table00:FA2F is separate. Moving/grounded ice calls CB31 at08:9DA2 and remains
active afterwards. The blanket first-hit hostile guard incorrectly omitted the
second eligible actor on its collision frame. Original references lose3HP for
either character and retain a live1/2/0source; the old paired result is13/28
instead of13/25. native-persistent-ice-before(child1) preserves this failure on
state3d32b53fe858a75feeead3242cc3340e3cf833a226202b5df22970070acd2d89.

The runtime now evaluates the other eligible actor for this persistent class20
body contact while retaining actual single-use hostile consumption. Native-
persistent-ice-fixed exits0 on state SHA256
5278493c5d0b38d80de8bba8ee1079339d8de24413e5f5d09c4317b9043ab407: both roster
orders and either first actor receive exact original13/25 or25/13 on the same
collision frame, preserve exactly one source update, retain independent native
hurt protection for24frames and replay the complete machine exactly. This is
a qualified authored G02 mechanic repair; the complete arena/campaign and
environmental-stun criteria remain open. Four CTests pass after the correction.
Actual75/180 accepted,105open/partial,1/15rollups.

The combined native-persistent-ice-regression gate exits0 on the same9e0bf608
state binary. It retains the existing native contacts, pickups, doors, capsule
scenes, menus/healing, participation/stock/team retries, independent worlds and
supports, plus all eight real interrupted-pass cuts, transactional preflight
cases and all eight actual boss damage/immunity comparisons.

### Hostile targeting before native health initialization

A newly emitted hostile can read the player before its native constructor
assigns health. The HP-based world-target filter therefore left a new Buffalo
ice attack bound to the farther world actor. Actual597-frame birth fixtures
preserve its uninitialized1318/class20 source. With the other living actor
nearby, a matched original00:DC59 update produces native horizontal velocity0;
the previous paired constructor produces-1152. native-hostile-constructor-caller
exits1 on state97260e9c3d2312a1c646781429a823bf25a7d5788a8e2244a23f6105c98bbe9b.
This comparison holds the constructor's caller bodies fixed rather than adding
player movement/stock-return work before comparing source state.

New hostile constructors(state1=0 at00:DCB3) can now select their living target
before HP initialization. Existing scene ownership and freeze guards still
apply; ordinary NPC constructors and retired active objects retain their prior
eligibility rules. native-hostile-constructor-fixed exits0 on state SHA256
13e106d84b540c3d9632004bf0b01e69fb344ca149c1db138d5d222664a59747: both rosters
and either world actor match all64 original source bytes, initialize nativeHP6
and preserve exact whole-machine/elapsed-time replay. The same combined run
passes four persistent-ice, four actual consumed-shot and eight native boss
immunity cases. Four CTests also pass. No new wire bytes or native damage/
velocity formula are introduced. This repairs actual G02/E14 setup behavior;
the complete authored criteria remain open and the total stays75/180.

The ff58812 runtime also passes native-persistent-ice-current-gui(child0),
GUI SHA256a60fc7e0537f453244cc75af3846f44522d25dd64e191a585fb480e99bb8c4f9.
Queueing a snapshot before one real host frame captures both native3HP losses
at worldclock209(delta1);24moreframes end at233 with the same independent hurt
protection. The exact-clock assertions prevent a delayed capture from hiding
a one-frame partner omission. Both BMPs were inspected; the two native bodies
and their meters render in the split field. This is supporting built evidence,
not new full G02/E06/V14/Z03 acceptance. The disposable script preserves the
source state and passes a copy to the consuming resume-state interface.

### Native rightward admission, retired regions and safe catch-up — A01/A14/D15

These three IDs are accepted on native-activation-qualification(child0), state
SHA25693f4dcf36a6c014131cf030e03b1ff87a653ed554f28f5ac66d12a4caea04f62.
Both actual rosters run480frames of P2 right/dash/jump input from the real stage1
entrance. P1 remains at384 or423 while P2 passes1100. The two original class08/01
enemies initialize naturally at736,1136 and976,1184 with nativeHP9: first-roster
frames78/210 and reversed122/274. P2 cameras482/708 admit those events while
P1 remains at256/295. No spawn, visit, camera-motion or readiness result is
supplied during traversal. Initial protection255 is explicit assistance.

The real stage5 platform case positions P2 at1050 in the upper room while P1
remains at384 in the bottom room. The initial P2 camera922/1280 excludes the
unvisited1224,1464 event. Only native input then moves P2/camera. Native admission
occurs at frame10 or12, camera960, P2x1088; class26 initializes and moves under its
actual AI. Both finish160worldframes alive, P2x1257 and P1 still384. Earlier
x950 preparation was inside the left wall and did not move; it is unqualified
setup evidence. This covers rightward admission, not prior travel to the upper
room or the separate upward/downward/reversal criteria.

A14 covers both retired seats, both rosters and absent/fallen prerequisites.
Public withdrawal is exercised through the balanced runtime request. Fallen
state is an explicit lifecycle-API prerequisite, not an authored fatality claim;
existing native-death criteria supply that separate behavior. The actual next
world frame excludes the retired private region and admits only the living
region’s original enemy at736,1136 or1840,1056. Its private valid bit is cleared,
the living bit remains, and gap/overlap/frontier/replay/presentation checks still
pass. The first fallen prerequisite omitted the newly selected world actor’s
full native current/target camera limits, clamping the camera into a third
region. Transferring those existing private limits fixes the setup; runtime
activation was unchanged. Both failure and diagnostic logs remain.

D15 reuses and reruns the existing native shared-camera contract on this current
binary: both-roster/either-victim bounds distinguish just-before/at the actual
pit threshold and negative-Y wrapping; full-machine replay matches. Real stage5
ground catch-up with either leader preserves personalHP11 and both living
actors. A genuine native jump makes the same destination unsafe, so catch-up
waits while both remain alive. Safe/refused futures replay exactly. These
checks qualify recoverable offscreen bodies without weakening the real pit
threshold or accepting broader placement/hazard criteria.

Actual78/180 accepted,102open/partial,1/15rollups. A02-A05/A09-A13, authored
vertical/platform/vehicle/campaign cases and final presentation remain open.


### Charged Gravity Well qualification correction

C04 and C10 are reopened. The old fixed180-frame hold reached native charge8
but released while the normal Gravity Well remained active. The actual
current-build capture in qa-state/lift-visual/gravity-arena-cut.sav has
body35=1 and main projectile class12, not charged class21. Native04:A4CA
correctly refuses this charged allocation while the ordinary shot remains.
Consequently the earlier charged-Gravity subcases do not establish those
criteria; the other recorded cases remain useful individual evidence. This
is a qualification gap, not evidence of an introduced gameplay defect.

The revised input waits for native shot occupancy to clear before releasing
Gravity Well and explicitly requires each special's actual charged source
type. It replays the original reference's exact input schedule. C09's42
fixed-field native-update comparisons on state SHA256
5510eaef241faa755283ecacf0d1713b89af9802b198acaffdbd4e02205daf32 pass,
but its Gravity case has the same gap and C09 remains unaccepted. The original
and paired lift-activation attempts using that premature release also failed;
they are unqualified prerequisites, not a platform regression.

Actual76/180 accepted,104open/partial,1/15rollups. No production code changed
for this qualification repair.


### Actual charged-source requalification and native hurt lifetime — C04/C09/C10

C04 and C10 are accepted again; C09 is now accepted. The native charged-source
gate exits0 on state SHA256
d7716b027a2a5b1dae37ac12b4f8d65b85a04ed8774baad0608880f5c412df9d
(qa-state/native-charged-source-roster-receipt.json). All16 boss-weapon
charged cases, eight slots/both rosters, explicitly observe the actual charged
projectile class16..23. Gravity Well needs256 held frames here, spends2 units
including its preceding normal shot, and is compared for676 frames. Other
charged slots retain180/420 frames. Every native command, charge, palette,
owned pool, energy and damage sample agrees; complete paired futures replay
exactly. The unchanged normal/empty/Hyper Cannon cases retain their earlier
independent evidence. This closes the qualification gap without changing
production weapon admission, charge timing or damage.

The actual charged Gravity Well hurt comparison also exits0 on d7716b
(native-charged-gravity-hurt). Both seats take original native2HP nonfatal
contact damage while the real class21 allocation is live. Its actual native
cancellation and96 subsequent complete gameplay updates match the original.
Together with the other40 original X/Zero cases in native-fixed-view-original
on5510eaef241faa755283ecacf0d1713b89af9802b198acaffdbd4e02205daf32,
C09 covers all9 X slots in normal/charged-or-automatic modes/both rosters and
six actual Zero charge/combo/saber cuts. Contact preserves the other complete
personal context. Each subsequent update compares command, charge, pose,
palette, complete auxiliary/shot pools, all40 shared effects, actual movement
and ongoing native energy costs. Two paired futures replay the full machine.

These lifetime comparisons use the actual00:D957..DA3A gameplay scheduler with
exactly one native world tick, balanced caller and retained elapsed guest time.
Both counterfactuals have the same explicitly fixed camera field; changing a
shared camera into a solo-following field otherwise changes legitimate native
culling. Native VBlank throughput/lag and moving-camera presentation are
separate Q02/Q03/V criteria. Failed unmatched-field and host-frame-phase probes
are retained, not masked or called native lifetime defects.

Native-charged-gravity-lifecycle exits0 on d7716b: public withdrawal waits until
f654 after the real scripted cast releases native control; return preserves
resources and fires again with no stale protection. Real stage construction
starts from the live class21 cut atf256 and clears its sources. Strengthened
native-live-gravity-death-retry also exits0 on state SHA256
27f08c935e6e0fb62f612432eb7dd0a8e885e4fdc7a35959a982bdd3423f3a6e:
both roster deaths/retries now begin atf256 with the real source live, use
original fatal contact, retain the survivor, then debit one life/reconstruct
both native bodies on team retry. Earlier late fatal cuts after native cast
retirement remain recorded but are superseded for active-source coverage.
Other C10 lifecycle cases remain independently qualified.

Actual79/180 accepted,101open/partial,1/15rollups. P05 remains open: actual
Gravity Well now activates the authored stage5 lift, but its stop/reversal
source comparison still has an unresolved exact mismatch. No platform
correctness or visual acceptance follows from successful activation.


### Shared Gravity Well query and native support reversal — P05

P05 is accepted by native-matched-gravity-lift(child0), state SHA256
5d814df278c5491c5d033631a1fe17f6310c7fa3f28563366ced48c44416670b.
The actual stage5 class37 lift at1080,896 is allocated from its native event.
Arms/acquired weapon flags and route/camera placement are explicit prerequisites.
Native R selects Gravity Well; Y remains held until the normal orb retires,
then the real charged class21 activates the lift. No activation, NPC state,
velocity, endpoint or rider result is supplied. Both native bodies produced
by that actual ride are retained. Both roster orders derive from the same
world/body fixture; the separate hazard fixture carried unrelated action
history and is excluded from this matched reversal decision.

At its actual upper endpoint Y704, each original single-character reference
and the paired run agree on the complete64-byte source on every update. The
native stop lasts60 updates, downward reversal starts at61, and24 further
updates end atY726. Both actual rider coordinates match their native reference
every update; after settling each living body stays exactly29(X)/37(Zero)
pixels above the surface with native top contact. The world advances once
per complete gameplay update. The earlier host-frame sample had original
world clock250/paired249 and an animation-bit mismatch; matched native updates
resolve that timing confound without masking source bytes.

A real ownership gap was also found in native02:DDD3's shared Gravity Well
query: it polls canonical first-shot bytes10D8/10E2/10E6/1108. A querying NPC
may have Zero projected for targeting, although X still owns the active weapon.
Four post-load hooks now read X's actual live pool for this world query,
retaining all original instructions, branches, stack operations and guest
cycles. Compiled entryDDD3 is routed through the existing observable boundary
pipeline. With co-op disabled, original Zero still reports no weapon.

Eight actual query controls cover both rosters: original X returns true,
co-op X and co-op Zero return true for the same live charged source, and
original Zero returns false. Both co-op results preserve the original X
query's complete CpuState and elapsed time. An earlier candidate query only
replaced canonical RAM without installing RuntimeState.current; that malformed
fixture is excluded. The corrected test installs the complete current/anchor
context through the existing snapshot API. The original-query behavior and
the original first-shot reader establish the ownership defect independently
of the excluded fixture. Four CTests pass with the new compiled boundary.

Actual80/180 accepted,100open/partial,1/15rollups. Other support families,
captures, campaign/presentation/runtime/online and final owner/release gates
retain their independent open criteria.

The affected native-shared-gravity-regression also exits0 on5d814df2,
including existing contacts/pickups/doors/capsules, paired menus/healing,
stock rules/return, independent views, separate supports, eight interrupted
controller/terrain cuts, malformed-state preflight and eight boss cooldown
cases. Current GUI SHA256
38e05d400f2e31f5cdf08ccfc44c1d19e49890ca2b26f6cb5ac49d7788fb9ca6
builds successfully with the same production repair. Its muted TCP capsule
capture records240 consecutive frames in qa-state/capsule-current using a
current-build native snapshot and bounded still-image storage. Presentation
is still unqualified: consecutive frames confirm the native3-visible/1-hidden
hologram state cadence, but the inspected region is black. Compare an actual
original native capture before attributing it to the private renderer; old
80-frame-spaced stills cannot resolve this. Other ordinary capsule stages2/7
currently fail native allocation in the prepared approach, before mechanics
qualification; their actual event/admission prerequisites need resolution.

Native-normal-gravity-query also exits0 on state SHA256
38565078adca80709fef6befc2d2b2a3106d9db06ac2fa23b95fffad44169517:
16 original/co-op controls cover actual normal and charged Gravity Well in
both rosters, including the normal query's additional native visibility load.
Both co-op projected contexts retain the original full guest caller/time.


### Native helmet survey completion and retired-owner cleanup

All four ordinary armor families now have actual acquisition/demonstration
coverage in both rosters. Stage2 boots pass native-capsule-settled on state
8e35f4d264660576eaa124bc4e43b034b5a74e1ff5e50ee4af129413428e62a6,
including249 native recorded frames/hash9ccbe317184cdcfe and all lifecycle
API interruptions. The new-stage setup uses the same120-frame room/body
settling as the established stage qualification; prematurely teleporting
before that settling killed X and invalidated the first approach attempts.
Same-stage preparation leaves the existing native stage intact.

Native helmet acquisition has97 recorded frames/hash2acc5221849625ba, then
a survey that waits at04:8FB0 for a new Shoot action. Original neutral input
and Jump presses correctly leave that survey open. Real Y/Shoot edges close
it through the native return, with native flags and both controls released.
The paired original trace and interactive exit agree in both seats.

A separate co-op defect appeared when X retired during acquisition: the
recorded segment completed, but the script started the optional survey with
no living X controller, leaving1F45/1F4F at2. The existing optional-arm guard
at13:C5BD now also skips the optional helmet survey for an already retired
X, taking the original completed return after retaining the earned armor.
This does not revive, heal or grant control to the retired actor. This
qualification interrupts before the optional survey starts; retirement
during an already running survey remains a separate scene-lifecycle case.

Native-capsule-qualified exits0 on state SHA256
3918766ca8468a56a4f34ac52ac6560c5f078de3564273b42ea734fff842d4a5:
helmet, body and arm families, both rosters, original recorded traces, actual
modal exit, exact full-machine replay, native X eligibility/simultaneous
contact, six API retirement cases per roster, preserved survivor HP/brightness
and subsequent real movement. Body81/hashcc33dfad0d9e1d88 and arms411/hash
ca29beaa17538dc6 remain unchanged. Four CTests pass. No renderer code changed.

Chip/golden variants and complete capsule presentation remain unqualified
under S08/S09/V12. The current native/private image comparison still has an
invalid native background prerequisite after teleporting. Explicitly loading
the native stage5 sprite phase makes original Dr.Light visible. The private
resolver supplies the same eight pieces, coordinates, native OAM attributes
and nonzero E4 colors; their background occlusion still needs a valid complete
native field/route comparison. Temporary renderer diagnostics were removed.
Actual acceptance stays80/180,100open/partial,1/15rollups.


### All native capsule families and Zero-first ownership — S08

S08 is accepted on state SHA256
80a3726016324f7adf611770cea75fcc0fe3f4c8d72fd12be61da8df4ec80483,
production HEADba79a80/engine614694ab. Native-capsule-matched exits0 for
stages6/10; native-capsule-family-gate exits0 for stages1/2/3/4/5/7/8.
These cover all four ordinary parts, four chips and the golden capsule from
the actual cartridge tables, in both roster assignments. The test supplies
only each chip's base armor/no-existing-chip requirements, or the golden
capsule's actual all-parts/hearts/full-health prerequisites. Acquired chip
flags are never supplied as the result.

An actual Zero contact leaves the same capsule waiting. A subsequent X
contact, without resetting that earlier native world update, admits exactly
one X-owned scene with no early award. Fallen, absent and zero-HP X cannot
lend eligibility to Zero. Simultaneous contact produces the same complete
64-byte capsule update as original X. The actual native acquisition then
compares both flag banks1FD1/1FD7 exactly with its single-player reference,
retains each player's original health, and reproduces the original recorded
control trace despite conflicting live inputs. Opening and award futures
replay to the identical complete machine. Six API-retirement cuts per roster
preserve earned flags, survivor health/brightness and fallen/absent status.
API retirement remains distinct from authored scene hazards.

Native-capsule-unified exits0 on state SHA256
1d2ad73e50a2b96ffb33ffc61b3e14976a70b478f70d3e0a45cdf39711e1ad74:
body and helmet scenes, both rosters, the same eligibility, actual acquisition,
native demonstration, replay and retirement checks with independent views
disabled. Allocation/route preparation precedes this camera-policy switch;
Unified spawn/frontier admission is not claimed by this scene check. Four
CTests pass after temporary diagnostics were removed.

The chip traces are stage1:65/f0370716aa992c49, stage3:209/e4bb2e9564be5a07,
stage6:65/64ce8a31a9307b8a, stage4:273/371bef0991c685b0 and
golden:65/34f28e1135300070. The four ordinary traces remain those recorded
above. Preparation now keeps the capsule event inside its unrounded camera
field. The opening bound2400 accommodates stage3's original1798-frame
dialogue plus participant transport, retaining the same actual completion
conditions.

Stage6/golden alcoves limit an opening survivor's movement to seven pixels.
Actual jump/dash input in both directions matches the original single-player
X/Y and action on every one of30 updates in both rosters, including real
movement. This is native terrain behavior, not a scene lock. Other cases
retain their ordinary movement and health/brightness checks. No production
terrain, capsule timing or eligibility change was needed.

S09/V presentation remains open. Explicit native sprite/CHR loading and
original foreground row streaming repair the teleported reference's skipped
asset prerequisites; with that foreground present the original also hides
Dr.Light. This does not establish a private-only occlusion defect. Full
native room/resource admission and moving scene presentation still need a
valid route. Diagnostic code was removed from production/test sources and
retained only with the private QA evidence. Actual81/180 accepted,99open,
1/15rollups; no release delivery is implied.


### Authored checkpoint crossing preserves the fallen actor — F01

F01 is accepted by native-checkpoint(child0), state SHA256
13657fbd6c4d6262aa14e90cc1d2baa0a982995c773e36ecfec515c14f341ae2.
Stage5's actual3C:E386/E474 records allocate special class0B/parameters1,2
at2944,1152 and3200,3008. Native3D:FF1D compares the living world body's
position and commits1FB5 only after it crosses the authored line. The setup
uses route/camera positioning, damage protection and API retirement of the
partner; it never supplies the checkpoint result or claims a new death-hazard
qualification. Both targets start unearned at checkpoint0.

All eight target/roster/survivor cases pass. Real Right/jump/dash input earns
checkpoint1 at frame46 for either character. Checkpoint2 is crossed at26
updates for X and18 for Zero. After16 further native frames, the entire
fallen personal context remains byte-identical, the survivor keeps HP/life,
no retry/life debit occurs, and public rejoin remains forbidden. Each actual
pending crossing replays to the identical complete machine and crossing
frame. No production change was required. Actual82/180 accepted,98open,
1/15rollups.


### Difficult native return placement — F08

F08 is accepted by native-difficult-return and native-difficult-support
(both child0), state SHA256
b5f9d1202bcc9f64e54ab785ae78ece61f5f7ecd9964218369fb219bf9272a02.
The actual stage5 plateau ends atx1632 beside the authored spike field.
With the survivor atx1612, public withdrawal/return chooses the clear plateau
atx1580 for either returning character in either roster. Both keep their
original health through60 real native terrain updates, and each complete
return future replays exactly. No damage protection, map edits or supplied
landing result is used in these placement cases.

The same native stage has a32px-high passage atx3064/floor960. Original X
settles there under the authored roof; Zero's public return is refused in
both seats. The entire machine remains byte-identical on refusal, then the
survivor continues60 native frames without damage or unintended enrollment.

The real class26 support at1224,1464 is admitted from its cartridge event
and runs the existing matched original/co-op45-update approach. AtY1434,
both actual riders have native surface contact. Either can withdraw in either
roster; a floating NPC is never accepted as an ordinary terrain landing for
the returning participant. All four refusals leave the complete machine
unchanged. The original two-rider future is restored afterward. No production
landing-policy change was needed. Actual83/180 accepted,97open,1/15rollups.


### Actual peer roster and full/stock startup policies — N01

N01 is accepted by native-session-policy(child0), GUI SHA256
846f3e29ff2649cbb337a97bd44dae1509d2358a196a7139693f0e44a65aa8b5,
HEADd2edf6d plus the bounded TRACE diagnostic fields. Two real rollback
processes run each complete native title/opening and enroll at tick3653.
The reversed full-campaign case selects Zero/X and both-player doors; the
stock case selects X/Zero and either-player doors. Each uses its own local
independent view and the current frozen package catalog. Automated profiles
disable physical gamepads and use muted dummy SDL, preserving desktop focus.

Both peers agree on portable profile0/transcript0c17fb26. At ages0/50/130/210,
their actual character assignments, restriction flag, life states, positions
and complete personal actor-codec digest agree. Reversed full campaign keeps
both living; stock keeps Zero absent while X moves. At age210 their personal
digests are ec95817b and caa14a18 respectively. Required executable/content
fingerprints and agreed mod settings gate startup; existing N11 rejection
evidence remains independent. No desync is reported in either run. The final
records are net-lan/session-policy83/result.json with both per-peer logs.

This uses the existing LAN transport over loopback for the new policy cases;
it does not replace N02's separate real-PC/SERVER evidence. No additional
online combat, HUD, scene, rollback-episode or disconnect criterion is claimed.
Actual84/180 accepted,96open,1/15rollups.


### Complete co-op peer confirmation and real delayed combat — N06

N06 is accepted by native-online-simtick(child0), GUI SHA256
85a7aea3b80940aca4d1a89bbbdc8a98c1244ea2129627139cbe0a4d3ead1aec.
Both actual processes cold-boot through the native introduction, then receive
real held/released Y and native combo presses. A loopback UDP proxy applies
25ms packet delay and one asymmetric300ms hold around the charged release.
Both actors attain actual charge, allocate native attacks (up to three X and
two Zero shots in the sampled pools), and remain living. Both peers perform
one real rollback episode/two resimulated ticks. Personal-codec and all40
shared-effect record digests match on all17 sampled simulation ticks, including
ages400/480 after reconciliation; no desync is reported. The final personal
digest is dec7e60d and shared effects936e6d48. Evidence is
net-lan/online-combat84-simtick/observations.json plus both process logs.

The validation exposed a missing confirmation domain: the generic engine
digest provides a title-extra callback, but X3 had not registered it. Native
WRAM alone omits the stored partner, creator tags and host-side continuations.
X3 now streams its existing complete RuntimeSave representation into that
callback, covering actors, ownership, passes, scene/input history, activation,
views, menu and presentation state. Hashing restores the exact prior actor
context after the serializer's canonical-body capture, so confirmation itself
changes no simulation state. This is enabled for every network session.

Native-coop-digest exits0 on state SHA256
3514126e463eaa3743d39b01733e3252b28b8b77d852ad9e5a5ad71e5d5fb982:
changing only the stored partner's HP or a valid creator tag changes the
digest while complete WRAM stays identical. Repeated hashing leaves the full
machine unchanged; local selected/split choices keep the same digest.
Native-rollback-combat also exits0 on
ff218bc6d928564fcc488345421c21dccabdc54dc9d492bdf17366d76a67a69d.
An actual current-build rollback snapshot restores charged Buffalo combat;
all32 subsequent WRAM and complete co-op serializer streams match ordinary
execution, with replay rendering suppressed as in the real rollback driver.
Only that isolated local comparison supplies protection/boss-HP prerequisites.
The actual network run uses the native cold boot with no such assistance.

The first network comparison used the live finish-call counter, which differs
from the simulation tick after rollback. Its host/guest offsets were two/six
frames, so it compared different states. The diagnostic now samples the actual
simulation tick; it never masks state differences. The engine's resolved-input
watermark is not treated as proof of matching hashes. A separate attempted
local-snapshot startup was correctly refused by the cold-boot guard; that
guard remains intact. Both failed preparations are retained with the evidence.

Native-session-policy-digest also exits0 on the same85a7aea3 GUI, retaining
N01's actual full/reversed and stock/normal behavior after the confirmation
change. These checks do not close the independent online HUD, death/wipe,
menu, scene or disconnect criteria. Actual85/180 accepted,95open,1/15rollups.


### Either living region earns native checkpoints

The original3D:FF1D special task compared only the shared-window actor. With
both players living, the distant non-anchor could cross stage5 checkpoint1
at2944,1152 without earning it; actual native input reached3001,1199 after120
frames while1FB5 stayed0. The task now compares either eligible living body
inside its own exact native culling rectangle. Its original constructor,
checkpoint ordering, commit and deletion still execute once, with no actor
projection, world replay or new serialized state. The generated checkpoint
entry is routed through the existing observable interpreter bridge.

Native-checkpoint-controls and native-checkpoint-fallen-regate both exit0 on
state SHA256 b3d6717f2fedd21a080de2cc16a6de4da611d6ebb28bb12321397ebf6fdcaeca.
All eight actual target/roster/entrant cases now cross the two authored stage5
lines with the other living anchor remaining in its original distant region.
Each crossing repeats on the same input frame and restores the entire machine
exactly. Direct native-handler controls prove a beyond-line body in a distant
region cannot earn the local checkpoint, and equality at the forward line
does not count as crossing. The affected eight fallen-partner cases still
preserve the complete fallen context, HP, life count and forbidden rejoin.
This repairs a checkpoint ownership defect; broader S06/N09 and activation
frontier criteria remain open. Actual acceptance stays85/180,95open,1/15.


### Actual online paired menus and shared reserves — N08

N08 is accepted by native-online-menu-qualified and native-online-menu-reversed
(both child0), GUI SHA256
5276b9e64211074e77a4fa612d06b576098703eca48e9948beb39b5d150860a3.
Both real peers cold-boot through the native introduction in normal and reversed
rosters. Fixed-frame TRACE preparation supplies only low HP, acquired X weapon
and shared full/partial reserves; real controller actions perform every pause,
selection, navigation, tank request and resume. The existing rollback frame
serial makes those prerequisites replayable without a second test clock.

Both panels independently select/navigate, converge on tank0, and compete in
the same native update. Seat0 wins that reserve once. Its live native healing
refuses the partner's Start while allowing navigation to tank1 and a separate
partial heal. The native full tank restores X to16 or Zero to28; tank1 restores
the partner from1 to4. Both reserves end empty without underflow, and empty
requests do not heal. The world clock stays239(normal)/237(reversed) through
all sampled interactive/healing cuts. Simultaneous Start completes one exit;
both actual bodies then move from2656 to2733/2735 under real Right input.

A25ms UDP delay plus asymmetric300ms hold runs in each case. The normal case
performs one actual rollback/resimulated tick; the reversed case needs no
resimulation. Complete RuntimeSave digests and menu/health/reserve/navigation
fields match on all15 compared simulation ticks. There is no reported desync.
Final digests are0627e8da(normal) andb28f09d1(reversed). Evidence remains in
net-lan/online-menu85-qualified-{x,zero}/observations.json and both peer logs.
The first staging attempt used the package directory instead of the mod root
and failed before launching a game. The first complete run's assertions used
unapplied input cuts and an incorrect full-tank endpoint; actual native reserve
semantics and the configured network input delay informed the corrected checks.
No menu correctness change was required. Actual86/180 accepted,94open,1/15.


### Actual online survivor death and native team retry — N07

N07 is accepted on GUI SHA256
48cf3e5d443699b5e2acba6bf23e139bee1e19aa4ddf6530b2214d3f2f1a5afd.
The two real peers cold-boot normally, then request the native stage5 constructor
at the existing serialized frame3900. Travel and later spike approaches are
positioning prerequisites. No death, fallen, wipe, fade, checkpoint, life debit
or retry-completion result is supplied. Each roster runs the same actual native
spike row, first under guest/seat1, later under the surviving host/seat0.

At tick5480 both peers retain a living host and fallen guest with two lives,
bright gameplay and no team wipe. Real host Right input moves1568to1598 while
the guest stays fallen. After the survivor approaches the real spikes, both
peers have two fallen actors and one wipe at5650; the original fade reaches
black at6000 with the shared life count2to1. By6200 the native stage entry
restores both original characters and16/28 or28/16HP. The actual current-build
arrival ends, and real Left input moves both384to353 while lives remain1.
Normal/reversed final full co-op digests are d00668f4/e38fb9ee. All13 compared
post-setup simulation cuts match, with no reported desync under25ms delay and
one300ms asymmetric packet hold. The reversed retry restores its original
X/seat1 anchor; the earlier survivor correctly owned recovery before the wipe.

Native-online-death-reversed exits0. The normal native-online-death run
completed every gameplay cut but its initial assertion reversed the engine's
physical Left/Right bit labels. The corrected exact same acceptance checks
validate its retained observations with verify-online-death85.py(child0); no
new gameplay or altered expected lifecycle was substituted. Evidence remains
net-lan/online-death85-{x,zero}/observations.json with both peer logs. Four
CTests pass. Actual87/180 accepted,93open,1/15rollups. N03/N09/N10 remain open.


The same checkpoint repair also passes actual remote peer qualification:
native-online-checkpoint exits0 on GUI SHA256
198c3dad62d04f83ddb58ec25e1cd439069cce9e4d70fdfee751c1c08f6ce033.
Both peers run the real native stage5 constructor. Guest Zero approaches the
authored line while host X and its independent camera remain at the native
entry. Actual guest Right/jump/dash crosses2944 and earns checkpoint1 once;
X stays384, both remain living, life count stays2 and anchor stays0. Allfive
pre/post-crossing complete co-op digests agree, ending05c7ffb1 at5598, with
no desync under the same25ms/300ms UDP treatment. This is N09's checkpoint
portion, not acceptance of its remaining doors/scripts/reward transitions.


### Bounded online peer loss preserves fallen status — N10

N10 is accepted by native-online-disconnect-guest and
native-online-disconnect-host-retry(both child0), GUI SHA256
0ed7d8a405f651dcaa7a541ea651f9eb174d34a422b482fce7f0e5b56183eb04.
Each actual two-process session first reaches stage5 through the native loader
and earns guest death on the authored spikes. Both peers have life states
1/2 at5480, before any team wipe or life debit. One test-owned peer is then
terminated: one case leaves the living host; the other leaves the fallen guest.

The remaining real process reports exactly one packet_silence diagnostic with
its1500ms budget, transport, session/seat, simulation tick5515, last remote
wire5508, prediction-cap stall, packet counts and timing. It exits normally
with code0 after1.812s(host) or1.797s(guest), including teardown. A read-only
TRACE exit observation retains initialized life1/2, wipe0, two lives and
HP16/0 in stage5. Neither departure revives the fallen actor, starts a retry
or resumes disconnected simulation. The first host setup found occupied
system UDP ports and failed before play; its replacement used checked free
ports without stopping the unrelated port owners. Evidence remains in
net-lan/online-disconnect87-guest/observations.json and
net-lan/online-disconnect87-retry-host/observations.json with their process logs.
This qualifies the direct-session coherent exit path; existing pre-frame-zero
refusal evidence remains separate. Actual88/180 accepted,92open,1/15rollups.


### Actual independent peer cameras and private native HUDs — N03

N03 is accepted by native-online-private-both and
native-online-private-reversed(both child0), GUI SHA256
7c0061369118ff52d69418cce0fc942355b47f9ff23e4cdb168d904c719e3d71.
Both real peers use their assigned independent view in both roster mappings.
The actual native stage5 constructor precedes route positioning. Guest input
first crosses the authored checkpoint and moves its private camera from2816
to2873 atY1024 while the host remains at384/camera256,Y2560. The host then
moves under actual Right input to467/camera339 while the guest stays3001
in its own room. Both actors remain living with their native16/28HP, life
count2 and checkpoint1. All eight compared complete co-op digest cuts agree,
ending2e0171fd(normal) and65d9c24d(reversed); there is no reported desync.

The existing network guard keeps the mutation-capable TCP debugger disabled.
A bounded read-only TRACE wrapper instead retains the actual desktop draw
callback output, including its native-field fallback at256px. Each process
records32 real presented frames across both movements. These show the proper
X or Zero body, distinct native room content, independently scrolling scenery,
and the owning native meter/badge in each seat. The reversed case visibly
swaps both body and appropriate16/28HP HUD. Native meter/badge pixels stay
identical across all31 post-positioning frames per peer; the same character
also matches pixel-exactly after moving to the other peer/room.

Evidence is net-lan/online-private87-both-{x,zero}/observations.json, both
capture sets and private-views87-meter-observation.json. Normal motion clips
are retained with the actual source frames; this narrow peer-camera check
does not accept the full V14 moving-scene review or remaining presentation
families. The earlier first capture kept the host stationary and was retained
as preparation, then extended to measure both actors and reversed ownership.
Actual89/180 accepted,91open,1/15rollups; N09 remains open.


N09's authored gate/following-script portion also passes native-online-door
(child0), GUI SHA256
ec99ad610a5b9e0d2c375ef3f5ee7a2b4cc74bd0816efddd73c6f448f219197d.
The native stage1 constructor precedes guest Zero's real3C:D27C gate approach.
Original allocation/admission adopts guest anchor1; one forced walk and native
following miniboss introduction retain the scene through6000. By6200 both
participants arrive at2577 with their original16/28HP and scene control released.
Subsequent native hostile damage to Zero agrees on both peers, then actual
Right input moves both bodies. All13 compared complete co-op digest cuts agree,
endingf1854b04 at6500, with no desync, wipe or life debit. Positioning bypasses
prior travel, while original admission, script, locks and cleanup decide results.
Evidence remains net-lan/online-door88-x/observations.json with both process logs.
Remote reward coverage remains open underN09; no acceptance increment follows.


### Native pause requests across scenes, fatal contact and restore — M12

M12 is accepted by native-transition-menu-door and native-transition-menu-death
(both child0), state SHA256
3891012602dde9f292bf069effa5d7c4739d793c6b0193e22b5ac138bb9e1882.
Four actual gate cases cover both X/Zero entrants and both roster assignments.
Both players press real Start during the original door/transport sequence.
The native scene retains ownership without opening a pause; actual pending
snapshots after requests at cuts0/32/60 load successfully without a hidden
panel owner. The original forced walk and following miniboss introduction
complete, and each player then moves independently under its own input.
The complete machine replays identically with the same actual Start sequence.
Unrelated earlier gate lifecycle/eligibility cases are omitted in this focused
mode; ordinary qualification retains them.

Four authored stage5 spike cases cover either real victim in both rosters.
Original lethal terrain selects one fatal contact (22/23frames) and retains
the free survivor. Fallen Start cannot open pause. The survivor opens the one
native pause, saves and restores it, then the fallen actor's real Start/Fire
cannot steal ownership, heal, or change any byte of its retained personal
context. One survivor Start resumes the restored pause with canonical actor
ownership and no hidden panel or revival. Subsequent actual survivor movement
and bright, unfrozen world progression still pass. No production menu change
was required. Actual90/180 accepted,90open,2/15rollups.


### Actual remote native boss reward and next-stage return — N09

N09 is accepted after its checkpoint and authored gate/script portions above,
plus native-online-reward-early and native-online-reward-early-reversed
(both child0), GUI SHA256
25217579df2437cd98679cb477190e8f3b97f85396c17be77d88f0d41f9a3e08.
Both real peers run the native stage2 constructor and final entrance, with
actual guest Right input admitting the boss room and following introduction.
Prior intro progress and checkpoint2 bypass earlier travel as explicit setup
prerequisites. They are not evidence of those earlier campaign/checkpoint
results; the separate real remote checkpoint check supplies that criterion.

The original Buffalo constructor reaches actual primary2/32HP before the
assistance. Once the native introduction releases control, position, personal
protection and bossHP1 isolate the finishing-hit/ownership decision. Only the
guest fires real native attacks: Zero in the normal roster and X in the
reversed roster. Original boss primary4/HP0 follows, then the native reward
and actual next-stage construction finish. Shared1FC8 changes from unearned0
through its temporaryFF animation value to the original completedDC. Final
stage1 is playable at phase4/4, with both original characters alive, no pending
scene/wipe and the unchanged two lives. All18 compared complete co-op digest
and native reward cuts agree between peers, with no desync under25ms packet
delay and one300ms asymmetric hold. Both final exactDC bytes were checked.

The initial normal late-setup case also passed on8f61a411b54ff12726e2eb57368f85bcd1e91fd238f65b92236bab2cc80a783e.
Its reversed case lost most of X's HP during the hundreds of idle combat frames
and died before earning a reward. That unqualified preparation is retained.
The corrected setup starts at actual native control release, records32HP
before assistance, and excludes a retired actor from further positioning. It
does not supply a hit, boss death, reward flag or exit, nor add a production
combat exception. Evidence is net-lan/online-reward89-early-{x,zero}/observations.json
and both peer logs. Actual91/180 accepted,89open,3/15rollups.

### Native Ride Armor boarding and empty-vehicle eligibility — P14

P14 is accepted. Actual92/180 accepted,88open,3/15rollups.

The matched native station test exposed an actual shared-window defect. With
X above the stage1 station and the living Zero anchor in the lower room,
retail armor appeared at1712,393, then the next original controller update
clamped it to1264,392 using Zero's room limits and subsequently retired it.
The original single-X reference boarded and moved the armor32px. Station
cleanup also needed its actual X menu recipient through the late boarding
HUD-mode change. The native station03:D777 and empty-armor03:9639 predicates both
explicitly reject Zero; this restriction is retained.

The shared00:DA48/DA6B callback now borrows the unique X's complete personal
actor and private camera through the original return, then restores the
anchor and camera. The one native armor pool runs once. The original03:A5EC
half-open culling comparison accepts an actual living view, retaining its
wraparound arithmetic and excluding the space between distant views. The
existing serialized WorldTarget and private-camera backup hold the borrow;
no additional persistent pilot field or duplicated vehicle simulation is used.
The cinematic armor class remains on its original path.

Frozen ROM-backed binary
bc02d820ebd4cd218c44a278aafde75c5312fb7d0e284746686cf17da673724e:
native-vehicle-boarding-original, -coop and -reversed each return0. Actual
station interaction summons and boards;30 physical Right frames move the
armor1712to1744. The other actor's opposite Left really moves him while all80
armor bytes match the neutral-partner reference. Active piloting and free
movement replay the complete machine exactly. Actual Up+jump releases the
pilot and occupied flag. Only after observing that native exit, position and
velocity prerequisites prepare a falling Zero at the empty armor, with X112px
away: neither boards. The inverse approach lets falling X board the same
armor while Zero remains free. Both roster assignments pass. A first exit
probe waited90frames and saw the original X naturally reboard after falling
back; the corrected check observes the earned empty interval before preparing
the next contact approach, without writing a boarding/exit/result flag.

Evidence: build-coop/qa-state/native-vehicle-{matched,lifetime,pilot-view,boarding}-*
receipts/logs and current pilot fixtures in activation-route-work and
weapon-state-work. Four CTests pass. Earlier regression job definitions pointed
to413017-byte historical snapshots; these were safely rejected. The retained
413057-byte effect-owner-fixtures supply the current intro/support recheck:
native-vehicle-regate-current-intro and -support each return0 on the same
frozen binary, covering native captures, rescue, retired participants and
separate original supports in both rosters.
P15 variant damage/presentation and P16 vehicle/support lifecycle remain open.

### Native vehicle variants, immediate inputs and presentation — P15

P15 is accepted. Actual93/180 accepted,87open,3/15rollups. The actual native
station selector, not a supplied vehicle result, chooses all four acquired
variants. Real Right/Left, jump or held flight, punch and paired missile
allocations execute with their original controllers. X is the unique native
pilot in either seat; the other actor remains free in a distinct view.

This found two additional defects. Native03:A1BC selects HUD mode0A for the
armor meter at03:9155. Treating that value as a shared scene collapsed both
private views onto the anchor. Runtime and presentation now share one view-lock
policy that recognizes actual playable armor. The world callback also needed
current-frame physical input: the stored non-anchor input was one frame old.
It now projects the pilot's held/previous/edge words before the original armor
call, leaving the actor controller to commit previous_actions once later.
The private pilot keeps personal HP and gains the original armor meter/R badge,
using the health already committed in native OAM. It remains visible when the
armor sprite flashes and disappears after ejection. No snapshot field or format
was added. Actual native and private pilot images were inspected for the four
models and both roster roles; the other view retains native Zero and his meter.

All12 native/normal-roster/reversed-roster variant cases return0 on frozen
binary e194d13620188bf0a8ba3603af415e5c8ef762a2f4aa24b78b2d586a6fb866f6.
Evidence jobs: native-vehicle-weapon-hit-original0/-coop2,
native-vehicle-weapon-qualified-original/-coop/-reversed. Each verifies current
port input, native jump/flight and punch/missile control plus whole-machine
replay. An original3C:D1F4 class08 constructor supplies the actual enemy at
1840,1056, bypassing travel by its original column admission and then position.
Its native DB06 table gives9HP and2contact damage. Armor loses32to30 once;
personal HP16/28 is unchanged. A no-fire control leaves the weapon target at9HP.
Real Fire damages that target with the original punch or missile. All120 enemy
health samples match the single-player reference exactly for every variant in
both rosters; first damage is at zero-based sample3/3/0/2 for variants0/1/2/3. Contact and weapon
damage replay the complete machine. A separate1HP armor prerequisite then lets
the real hit earn destruction; the original accelerating30..1 warning and
explosion finish before retirement and native pilot release, without supplying
any defeat, ejection or removal flag. Post-release enemy contact can then hurt
X normally, identically in the reference and co-op.

The Frog control preparation waits for its actual idle continuation after the
hop, not merely its first terrain contact. Hawk/Frog use held Jump through
startup. The corrected Frog traces match native position/velocity throughout
jump (minimumY337 from392); Hawk held flight reaches322. Single-player checks
explicitly disable co-op after each load because restoring an enrolled state
intentionally resumes co-op. Constructor calls use DB06 and restore the entire
outer CPU except elapsed guest time. Earlier DB00 and mixed-reference attempts
are discarded preparation, not native armor-immunity findings or game failures.
Extra image rendering occurs after the replay's saved result so native PPU
observations are not an unmatched replay operation.

Machine-readable samples/source hashes: build-coop/qa-state/native-vehicle-qualification.json.
Actual before/hurt/weapon-hit/retirement PPMs and pilot fixtures remain in the
matching native-vehicle-weapon-* directories. Native-vehicle-p14-true-original
returns0 on the same build, including actual Up+jump release, and the four
native-vehicle-variant-final-ctest checks pass. P16 support/vehicle retirement,
withdrawal/death history and the other terrain/campaign/presentation criteria
remain separate open work.

### Native vehicle missile retirement and rider death — P16 partial

P16 remains open until authored support destruction is qualified. Native firing
allocates each Hawk/Frog missile pair in X's personal pool through02:D8C4 while
CC8+4D counts the live pair. On83e90a2f, actual Up+Jump after the original firing
lock releases X with two missiles still live. Withdrawal cleared those personal
sources but left the shared count at2. Transient cancellation and fatal cleanup
now clear that count only for X's active class0 armor. Zero's retirement and the
class1 cinematic armor retain their separate ownership.

Native-vehicle-return-camera exits0 on state SHA256
2d4f300c3c27406f73028020038da6e793df192e98e31882c30a1065e6a3d9ec:
Hawk/Frog, both rosters, actual native station/fire/exit. Riding withdrawal and
last-player withdrawal are refused without changing the complete machine.
Withdrawing Zero leaves all80 armor bytes and X's actual live missiles equal;
withdrawing X releases his sources/count while retaining the empty armor and
Zero's health/shots. Withdrawal plus survivor movement replays the whole machine.
The return setup teleports only the empty vehicle and free actor to the known
lower plateau and installs its complete native/private camera as a prerequisite;
this bypasses travel and does not claim safe return inside the station corridor.
The armor survives30 original world updates while X is absent, safe return works,
actual falling/contact reboards, and native Fire allocates a fresh missile pair.
An earlier setup changed only private camera records and incorrectly used
AdoptPlayer for the anchor, which intentionally skips that seat; the resulting
empty-armor cull was invalid preparation, not a product defect.

Native-vehicle-fatal-pilot exits0 in both rosters on state SHA256
a64b00d34cdcca1f852a178e45b276f26b53ec1c20e1fa52fd598a1edd6193a3.
The actual stage1 class08 foe hits occupied one-HP armor; its original warning,
ejection and explosion run, and subsequent real contact kills one-HP X. No death,
ejection, removal or survivor result is supplied. The complete sequence replays;
Zero stays alive, moves in a bright world and advances the clock once per frame.
Fallen X cannot return through the public API. Four CTests remain passing.
Actual93/180 accepted,87open,3/15rollups; no new whole criterion is claimed here.

### Authored support destruction and rider cleanup — P16

P16 is accepted. Actual94/180 accepted,86open,3/15rollups. Native jungle records
3C:EC96/EC9E create class1F solid enemies at640,848 and880,816 with18HP. Native
DB06 event allocation/constructors and real falling contact establish separate
riders. For the weapon branch only, the second support and its approach are
positioned atx760 inside X's original projectile retirement range; this bypasses
travel and supplies no damage, death, rider or destruction result. One-HP target
health is a prerequisite. No Fire leaves it alive; real native buster fire earns
lethal contact on frame16. All120 health samples match the original solo run in
both co-op rosters. X retains his separate18HP support and actual rider contact;
Zero falls naturally after his support disappears, retaining personal health.
The entire destruction future replays exactly.

The measured old runtime retained82 on the retired support after120frames
(state46f78a1e), and actual one-frame fatal enemy08 contact retained81 on the
fallen X's support (9a6ef0ec). Cleanup now releases inactive support history on
the existing balanced native enemy/item returns; initializers retain their prior
slot-reuse behavior. Fatal retirement clears only that seat's rider bits. No new
serialized fields, world pass or generated entry is needed.

Native-support-retirement-final-jungle/original/support/intro all exit0 on state
SHA2562aded3e9e6b217d74eb72ae5139e697261a758adf32a3dc9f01bea59b26d3855.
Both roster orders cover withdrawal of either earned rider and real fatal
contact against either rider. Jungle3C:ED92 supplies the authored class08 foe;
only collision positioning, one personal HP and expired victim protection are
prerequisites. At the actual death frame, only the dead rider bit is removed;
the unrelated support, living rider and health remain. Full-machine replay is
exact, and the surviving support subsequently carries its rider for96 native
updates in a continuously bright world. Existing moving-support, capsule/intro
and current-build restore gates pass; the GUI rebuild and four CTests pass.

The vehicle portion is supplied by the preceding qualified actual Hawk/Frog
withdrawal/return and native armor-destruction/rider-fatal tests, including both
rosters, unrelated ownership controls and complete-machine replays. The earlier
stage5 class26 public rider-withdrawal evidence also remains applicable. This
acceptance does not qualify remaining collapsing/one-way/terrain/crush criteria.

### Actual survivor colors through native corpse cleanup — D10

D10 is accepted. Native-corpse-private-admission exits0 on state SHA256
d154abd413bd34392ad6ada35632f74d65aa8e80a08d6a136aac28f1be8ec8af.
Both jungle supports retain their original640/880 positions, with independently
following cameras. Both rosters and either victim earn actual one-frame fatal
class08 contact while riding. From that same live-contact cut, voluntary living
withdrawal supplies a reference without a corpse. At native frames0/7/15/31/63/
95/127/179, the actual selected-survivor compositor produces byte-identical
64x80 body and48x80 background/HUD patches, at identical native body/camera
coordinates. Both actors are native X3 bodies; personal health is unchanged.
The continuing native world and actual support carry remain qualified.

After the genuine initial contact, only the living hostile's position is moved
away in both branches, allowing original culling to remove it before later AI
can obscure the color comparison. No death, brightness, palette, retirement or
survivor result is supplied. Both original view regions are restored before
native constructors update existing supports; disabling them through that whole
preparation would incorrectly cull the distant support and is not a product
failure. Pixel capture preserves guest RAM/time and the original fatal outcome
is restored before the remaining replay/control checks.

Actual absent/corpse PPMs remain in qa-state/native-corpse-private-visual. Inspected
native X/Zero PNGs show the appropriate bright survivor and jungle. These checks
qualify corpse color/brightness isolation; the remaining general visual, HUD,
weather, aspect and audio criteria stay open. Actual95/180 accepted,85open,
3/15rollups.

### Authored ladders and native camera-limit timing — P10

P10 is accepted. Native-ladder-region-timing exits0 on state SHA256
5c3854e504e5697fd1217627d202ecffb18ad1e13ab853508c6add6484a72202.
A real native stage2 load supplies the free start. The live collision map has
ladder columns atx1472/capy704/run720..848 andx2384/capy608/run624..688
(types12/13 hex). Body/camera positioning bypasses travel; native column1408
admits all four actual3C:D557/D55E/D565/D56C region scripts in the16-byte1D18
pool before either reference. No attachment, movement, exit or limit result is
written. Neutral waiting below this hanging ladder is not an attachment proof.

Both roster orders and both tested seats run240 real input frames:120Up,
90Down, then30Right with the initial Jump edge. The tested actor is non-anchor
in co-op; the other actor grabs the separate ladder and then holds its rung.
Every original/co-op frame matches position, vertical/horizontal velocity,
native pose/substate, health, grounded flags, camera coordinates and all four
current camera limits. Both characters traverse the long and short paths,
clear the authored upper ledge/ceiling boundary, descend through native top
admission and lower exit, and detach. The long path additionally requires
Jump from an actual2A mid-climb pose. Original X reachesy687 and Zero679 at
its upper cap; the short path reaches591/583. World advancement is once per
frame and the complete paired-machine future replays exactly.

The complete native reference exposed one premature private limit update:
00DA0D moves the native camera before00DA2D executes region scripts. Parsing
valid region targets again inside the private body-bound pass applied the next
left boundary before the original camera. The pass now initializes missing
views there and retains already committed targets until normal00DE43 admission.
No new state fields or native entry bridges are added. Earlier teleported
references without the actual native region controllers are excluded.

Native-ladder-private-pit-regression exits0 on the same binary, retaining all
four actual independent-room deaths and the original live class05 support
control. The GUI rebuild and all four CTests pass. This accepts the ladder
mechanic and its per-view native boundaries; remaining general frontier,
terrain, crush, visual and performance criteria remain open. Actual96/180
accepted,84open,3/15rollups.


### Native one-way ladder tops — P07

P07 is accepted. Native-oneway-cap-approach exits0 on state SHA256
`990cad2bd92314566639c8d97de8b926c511e0f5e22cc270e9f74dc6e096792d`.
The actual stage2 collision map supplies type13hex surfaces at1480,704 and
2392,608. Both rosters and either tested non-anchor actor run160 native frames
against the original controller. Each actor first earns a native ground Jump;
a position-only teleport then puts the already jumping body24px below its
cap. No Up/Down, ladder attachment, action, velocity, grounding, landing or
rider result is supplied. Actual regional controllers are admitted through
the original event column before comparison.

Both characters pass upward through the underside and land on descent. X
stands17px above the cap at native landing frame36; Zero stands25px above it
at frame32. Their ascent minima are674 on the704 cap and578 on the608 cap.
Right input then carries the body across the tile edge onto the neighboring
native floor. Every body/velocity/pose/HP/ground sample, private camera and
current limit matches original; all four complete-machine futures replay
exactly and the world advances160 times. Evidence:
`build-coop/qa-state/native-oneway-qualification.json`.

The Gravity Well class37 lift is excluded from this check: original04:BFAE
resolves every side, including its underside/crush path. It is a solid lift,
not the game's one-way mechanism. An initial48px-below-cap approach also
failed to clear the top in original because it exceeded the measured Jump
clearance; moving the position prerequisite closer established the actual
positive. Neither rejected setup is recorded as a co-op defect or pass.

Actual97/180 accepted,83 open/partial; R/M/N remain the three accepted areas.


### Native slope charge/origins and widescreen source lifetime — P08

P08 is accepted on state SHA256
`2d04c4b97f626960c937f2ff3a583c30584414ae39919fb5a38bd41f7ff4e58f`; native-slope-widescreen exits0 for both
rosters and either non-anchor actor. The real stage2 ramp uses collision
profiles0C/0B/0A/09 at832/848/864/880,Y928. Actors earn charge through180 real
input frames on the free native floor before a position-only approach. The
original slope pushes the body downhill, including against walking/dash
input; the check follows that real traversal rather than inventing an uphill
standing requirement. Every run touches all four grounded ramp profiles.
X height changes911..926 with charge6 and12/29grounded samples; Zero changes
903..918 with charge8 and16samples. Actual attacks allocate while grounded,
including the charged X shot at actor837,912/muzzle863,905 and native Zero
shot at actor879,915/muzzle887,906. These are actual original results.

The comparison found an objective source-lifetime regression at frame42: X
retains8 native particles/counter8 in the original but7 in co-op. Its main
projectile also starts retirement early. The original title widens horizontal
object windows for widescreen in generated and private-ROM execution. The
co-op cull replacement had kept stock bounds, bypassing those established
constants. Native-slope-lifetime preserves this failure on26dd5b66... and
its original/paired current-build snapshots. An initial camera-phase
experiment did not resolve it and was fully reverted.

The runtime now reuses the existing X3WsObjWinAdd for horizontal extents at
02:D58A/D611/D636 and00:DF9F, retaining the same native-controller exclusion.
Variable-radius D5AB/D5DA contracts remain native. No new serialized state
is added. All96 body/camera/limit/charge/facing/palette/auxiliary/projectile
samples and the full-machine futures now match original in all four cases.
A margin-disabled original/co-op control, both-roster private pits and all
four P07/P10 cases pass on the same binary; four CTests pass. GUI build0,
SHA256`eff3e6cb26197f0c27da0987c74dc138e6a21b9f9753b547bbb6415d161f1b26`. Evidence:
`build-coop/qa-state/native-slope-qualification.json`. C08 retains its separate
ground/air/facing acceptance. Actual98/180 accepted,82open/partial,3/15areas.


### Earned ground, air and slope attack origins — C08

C08 is accepted. Native-origin-isolated-qualification exits0 on state SHA256
c3f958943525b8420656255ab67f39cbae03ae0ab1e5dbcd27bae7fedfcb906c, with production unchanged from0c95bd2.
Both rosters/either seat earn actual Left/Right facing before attack, with a
real eight-frame Jump for airborne cuts. Each posture starts with empty
native shot capacity, never a supplied projectile or pose. Both original and
paired modes emit on frame0 and compare every body, camera, limit, facing,
charge, palette, auxiliary and projectile byte for12frames, followed by a
strict complete-machine replay.

Actual first-emission offsets from the acting body are Xground±26,-7 and
Xair±22,-9; native Zero ground±24,-1 and air±24,-4. Grounded charged slope
emissions use X±26,-7 and Zero±8,-9. The real Left/Right direction is earned
before180frames of charge, then a position-only approach traverses every
ramp profile. All96 native slope samples, source lifetimes/counters and
complete futures match. Only the tested actor charges: an unrelated
companion charged release consumes shared RNG and is not an equivalent
single-actor reference. No comparison field was removed to obtain the pass.
Evidence: build-coop/qa-state/native-attack-origin-qualification.json.

### Independent authored ice forces in both directions — P11

P11 is accepted through the actual stage2 slippery-terrain contract. Live
profiles8C/8B/8A/89 at832..880 impart native+0400 horizontal force; the
opposite85/86/87/88 at1760..1808 impartFC00. Both actors approach real terrain
with normal motion and protection, without supplied force, terrain
classification or grounding results. Native input-free contact earns each
force and displaces the body while its input velocity remains zero.

Every paired actor is compared with its own separate original reference in
both rosters and either canonical-anchor order. The48-frame positive run
also exercises opposed inputs and ordinary-floor recovery; the12-frame
negative run stays safely above the adjacent authored spikes. All body,
private-camera/current-limit, terrain2E, forceA7/A8 and dragB4 samples match.
Each complete future replays exactly and the shared world advances48/12
times, respectively. The ice source is immutable collision data, without
a second source-AI update. Positive statec3f958943525b8420656255ab67f39cbae03ae0ab1e5dbcd27bae7fedfcb906c;
negative state01a39f555bd31c31e8a906a9a1f472d2feb5edf21ed9e6efc25da945b1b1e2fc; both gates exit0.
Evidence: build-coop/qa-state/native-ice-force-qualification.json.

No production changes were needed for these two criteria beyond the existing
widescreen correction. Actual100/180 accepted,80open/partial,3/15areas(R/M/N).

### Native Buffalo captive retention and closed-arena views — partial G02/E07/E08

A real native charge earns Buffalo's grip at03:CC5B/04:D034. Moving the free
partner nearer then exposed a defect: the next NPC update selected that free
body and moved it to the hand, leaving the original captive behind. The old
09acb12abdc8a8ddeccd8f4091aaa06fd98d5fa087d077c524a420cd959a547d state
executable preserves the failing native-buffalo-capture run. No attack, capture,
release or success flags were supplied.

The runtime retains the actual successful victim in two existing attachment
bits per native slot. It projects that body through the original hold and
immediate defeat-script continuation, clears the bits on native release,
retirement, reuse and stage construction, and refuses participation changes
while the living boss physically holds its victim. Dead-boss script ownership
does not count as a physical capture. The snapshot remains413057bytes. Unknown
attachment bits, competing owners and unenrolled attachment claims are refused
transactionally.

The original final-door flag1F5D.80 persists throughout Buffalo combat. Using it
as an ongoing camera-sharing condition copied the canonical actor's camera
into the other view and produced a1px captive displacement discrepancy. Actual
native scenes now own camera commitment. Both views initialize from the native
scene's release and subsequently follow their own bodies. Native door limits
can raise their minimum before replacing the previous maximum; each private
view preserves that native locked point until its exit. Camera timing remains
00DA38, including the earlier qualified P10 target-order guard.

Native-buffalo-capture-matrix exits0 on
5a857ceabb08b8089f1b9355d520249712c94d5e7dccc21c192c83004d2a4710. Eight
fresh cases cover both rosters, either victim, and canonical/noncanonical
victims. Every139 native hold/wall-release frames match the complete victim
body, owned pools, palette and64-byte captor, with only the established native
body-DMA request bit normalized. The single-actor reference installs the
victim's complete private camera. The free actor retains its input; both public
withdrawals refuse without changing the machine; whole-machine replay agrees.

Native-buffalo-kill-matrix exits0 on
1e84cc853ccb9d7d8e92fccd4b3f4609decab99f2e017e9d76e33a96f0a431da. All
eight fresh cases begin at an actually earned grip. Only boss1HP/cooldown,
personal health/protection and shooter position are assisted. A real partner
shot earns primary4/HP0, native cleanup releases the victim, and the original
defeat continuation retains its owner until the actual rewardDC and stage1
playable return after2553frames. Both actors remain alive, lives remain2,
all captive bits retire, and the complete future replays exactly. Requiring
those bits to clear immediately at the lethal hit was an invalid test
assumption; they describe the original script continuation until retirement.

The GUI build and four CTests pass; GUI SHA256
06d732d94f6103c10bb8d42612052960644da8955bc576fd355f3d0b3811b510. Relevant
intro/rescue/fallen-participant regressions pass on
e6f386b75d35f9ff8059be3932e8c9123e4bb52211305a574fed2ab7670a4d67. The
combined scene/support/preflight gate passes on
dca69e1b42147bdea931a15307ad3eb7dd8172e9e463104edcb522643660221a.
Its former rider-only malformed-bit assertion now uses undefined bit16, since
bit4 is a valid captive owner. The released stage1 door passes preflight and
restore after the native transient-limit correction.

The half-health beam is separate remaining authored work. Actual AI emits its
class20/variant2 source after344frames without supplied attack/spawn flags.
The matched freeze/escape comparison is in progress. G02/E07/E08 remain open
overall; this evidence does not cover every authored captor or all lifecycle
exits. Actual acceptance remains100/180,80open/partial and3/15areas.

### Native beam victim, freeze effects and recorded inputs — G02 partial

The half-health native Buffalo AI earns class20/variant2 after344frames. With
a vulnerable actor64px ahead of its muzzle and a nearer free actor behind it,
the native single-actor reference freezes the victim atframe39. The previous
paired update leaves body6E clear. The failing native-buffalo-native-freeze
run is retained onfb35e335f21f505e49e712fde52f556d0a0800334a540d3cbcfb74f389179292.

Beam targeting now chooses the nearest eligible body along the native forward
span and26px vertical range; the original growing-tip predicate still decides
the actual hit. The actual08:A07E allocation gives its freeze child the victim
owner, while the separate beam graphic remains an unowned world child. Existing
owned-effect calls borrow their creator's physical pressed-edge words as well
as the personal body. Native recorded demonstrations retain their original
JOY1 stream. The unguarded input change broke a real capsule demonstration;
the1F63 guard restores the exact81-frame native control hash in both rosters.
No new native hook entries or serialized fields are added.

Native-buffalo-beam-matrix exits0 on
d99470f2bb04dfcc8d747daba9c9fb3c08be1ba987fbd1420778bc9c7fb04ac8. All eight
roster/victim/anchor cases match every native victim, pool, palette and source
byte through198frames of freeze and real input escape. One actual freeze
child owns its victim, the nearer free body stays unbound, and participation
refusals preserve the whole machine. The actual source retires93frames later
with both actors alive, no old owner or capture, and exact full-future replay.

Native-buffalo-frozen-clear-matrix also exits0, on
9a58089e98adb1be43eb24b765c33f830320024ddb91a08eddcbaf539c43bcb3. Each of eight
fresh actual freezes is interrupted by the free partner's real native attack.
The original defeat/rewardDC/stage1 transition repeats exactly, preserves both
actors and lives2, and retires old freeze owners. The second actor completes
its arrival32frames after the first; both then stand at the native start with
their8px foot difference and independently respond to24 real Right frames.
The initial assertion sampled an unfinished second teleport. CD90 is shared
native scratch used by other stages, so its new-stage value is not evidence
of an old freeze. No blanket scratch reset or forced arrival result is used.

Native-buffalo-final-authored exits0 on
2da24c7fdc3f87da6b62635e67aca5edaa2500314c46f0ec1ddddc9289e1bcef. Real boss
contact kills either partner in either roster; native survivor shots, rewardDC
and actual2585-frame return retain the fallen participant and the living X
or Zero. Actual recorded capsule control, lifecycle interruptions and replay
match native behavior despite conflicting input; the81-frame base hash is
cc33dfad0d9e1d88. Intro/rescue/fallen-helper cases pass. All18 actual charged
weapon reference/replay cases pass the changed owned-effect input path on
be326578433425db64f2b240f81af690db8d7abc330cf29fb1b71f2db7eedcec.

The broad pre-fix input run completed its earlier contact, state, camera, menu,
stock, stage, targeting, weapon and support sections before the recorded-demo
failure. The corrected capsule check above passes; its unrun survivor/retry
tail passes separately in native-buffalo-final-regression-tail on2da24c7f.
Final state/GUI builds and four CTests pass. GUI SHA256
d4e4335f5419450b40938789b15c180715be11a58e2a09fc8218fec416448e0f.

G02 remains open while the additional authored grab input-escape and weakness
release decisions are resolved. No new complete enemy/campaign criterion is
claimed from these subcases. Actual acceptance remains100/180,80open/partial
and3/15areas; presentation, owner validation and final delivery remain open.

### Buffalo native input escape and weakness release — G02 accepted

G02 is accepted after reconciling the earlier earned-intro/native arena and
leader entry records with the current ice, charge/grip, beam, weakness, clear
and actual return checks above. This is one campaign touch point; enemy capture
criteria remain open for other authored actors. Actual acceptance is101/180,
79open/partial and3/15areas.

Native Jump edges earn the actual grip release atframe30, while3B.80 and its
script binding persist throughframe139. The old physical-capture guard wrongly
refused an otherwise ready withdrawal atframe41. The runtime now distinguishes
the actual0A/sub4 hold and unfinished victim pose from the boss's continuing
recovery script; it retains script ownership through native cleanup.
Native-buffalo-release-withdraw preserves the failure on
75847953bccdfb882ba46a1e9c79675addcaed672b689e88f3fbb051f753682f.

Native-buffalo-shake-qualified exits0 on
08b7f2f2b7d1c1180456a17add2f96676cafedc6be52ba01669fdc933b284407. All eight
fresh roster/victim/anchor cases earn native release30, allow withdrawal after
actual control resumes, preserve the original busy refusals, and still match
every139 body/pool/palette/captor frames and the complete replay. The temporary
withdrawal probe restores its exact snapshot before that ongoing comparison.

Native-buffalo-nonlethal-matrix exits0 on the same executable. In four applicable
roster/anchor cases native X selects prerequisite-acquired Parasitic Bomb and
fires a real attack while Zero is genuinely held. The actual hit reduces the
live boss32→29, selects native0E recovery and queues88 only on the real victim.
The free shooter remains unscripted; the released victim withdraws safely, and
the whole32-frame native recovery future and hit timing replay exactly. No
Zero special weapon, capture/release state, damage or success result is supplied.
Final state/GUI builds and four CTests pass; GUI SHA256
fef300868460c413fc8553d517875269f2b0330901404ab322164af86b1cddb3.


### Native Crawfish capture ownership and release (G06/E07–E10 partial)

A real stage6 checkpoint2 entry and native door/intro produce the32-HP
class54 Crawfish. Checkpoint2 must be selected after the loader reaches
D1=4,D2=0; selecting it before that reset reaches checkpoint0 instead.
The native X entry independently passes. No capture/attack/clear flags are
supplied: the actual charge and CC5C collision earn the grip.

That grip exposed three distinct defects: a nearer free body replaced its
victim, the free physical port incremented the captive escape counter, and
the noncanonical health meter retained its native damage-update bit. The
existing attachment owner now follows Crawfish's real action0/sub4 hold
and immediate defeat continuation; D399 completes release and clears it.
The captive's physical input is borrowed and restored around the one NPC
update, and the second living health meter acknowledges03:906D/90BB's
health/clamp contract at the existing HUD boundary. No snapshot fields or
additional world/audio updates were introduced.

Actual periodic capture damage can kill a one-HP captive. The solo
04:D8D5 retirement then incorrectly marked the shared boss for removal.
With a living partner, the two observable native boundaries now use the
original03:D399 release/animation/recovery instead. The31st damage frame
falls only the captive;24 subsequent input frames retain the32-HP boss,
clear the binding, advance24 world ticks and move the surviving actor.
Compiled03:D352/04:D8D5 entries retain the same boundary checks.

Final state binary b4f2152a6b22165388f16442aef64d83ecd3e8c3422bb73b76786f4b1ace8687
passes8 full191-frame native holds with five damage ticks,8 real30-frame
input escapes,8 fatal-grip futures and8 real partner-projectile captor
kills. Each hold/escape compares every body, pool, palette and source byte
(the established body17.80 DMA request is normalized); busy withdrawal
refuses transactionally. Released withdrawal leaves the living encounter
running without a stale binding or false retirement. Kill cases earn
exactly one new acquired weapon bit08 at1FC2=DC, reach the actual2553-frame
stage1 return, finish both arrivals and preserve lives. Full futures replay
exactly. Four additional actual captured-death survivor clears pass on
eb37538b67e7e70cf9b626bf4e690f329bf88c6b51c6145da87c981e1c62c68e:
either X/Zero in either roster retains the fallen partner/zero HP through
the real reward and next-stage arrival. These are executions beneath the
existing criteria, not additional acceptance points.

The8 Buffalo native139-frame grip/Jump/released-withdraw futures and
affected private flags, menus/shared supplies, healing and transactional
preflight regressions pass on the final source. State/GUI builds and all
four CTest gates pass; GUI SHA256
afc7f4a9f228e7b717fc7538ba6cb221f833049beaf9df39f0379bf04e55dd37.
Exact jobs/receipts are summarized in qa-state/native-crawfish-capture-qualification.json.
This capture-only checkpoint left G06's vertical-room route and other captor
families open at101/180. The following qualification completes G06. No owner
playtest or release is implied.


### Native directional frontiers and Crawfish descending room (A04/G06 accepted)

The actual stage6 shaft at4480,2272 drops onto floor2416, then native
Right/Left input walks off the authored ledge and lands on floor2512.
The distant partner remains in the starting room. Four real-gameplay
roster/traveler cases compare every native simulation step: complete body,
owned pools/palette, grounded height, camera and current room limits.
X completes204 steps and Zero206. Extra video frames are permitted only
when that entire sampled actor/camera state is unchanged; each video
frame advances the world at most once. Actual continuous replay preserves
the complete machine, including streamed and visible state. Video-frame
counts differ between cases; this is not a runtime performance claim.

This route exposed a real admission delay: the lower class0D enemy at
3C:E744 /4488,2608 was created11–12 simulation steps late. The previous
rectangle omitted both the native downward row through cameraY+272 and
the aligned column to the left. Widening only its bottom did not fix the
missing column and would not preserve directional eligibility.

Admission now retains each view's actual native horizontal/vertical
frontiers and scan order. The retail column allocator receives its
original row spans; native event flags govern consumed/reused records.
The extra held-visit admission cache is removed. At most22 native column
spans fit the bounded31-entry queue with two frontier bytes in the same
67-byte spawn state. Current snapshots remain413057 bytes. The canonical
resident-background rule and the one shared world scheduler are retained.

All four cases now match the original enemy birth/readiness steps exactly:
188/189 for X and190/191 for Zero. The full body/camera/terrain comparisons,
independent region preservation and complete continuous replay pass on
52f0cbc5ba9bce4d8f877ccedc3a93e6c0f405646ee7f1da4993713dca4fa357.
The existing actual P2 movement/support admission, separated regions,
empty-gap exclusion, overlap, retired-view exclusion, shared-camera
assists and transactional snapshot checks pass. Actual mid-allocation
save/load retains native allocation order. Unknown active row spans and
unknown/contradictory frontier flags are refused without changing the
machine, qualified on final state
c908cf69520213b9c59fbaa53efc2dfd15404f826473d91dd993857e9e93ef2b.

Both real captor reference/release smoke cases and all four actual
Crawfish captured-death survivor clears pass on the new frontiers,
including native weapon bit08, stage1 return, complete arrival, fallen
persistence and whole-machine replay. Final state/GUI builds and all four
CTest gates pass; GUI SHA256
05518dd7da7e1775bbd80bbe4f257b2cf54d7ef887258e2d7d1d709a9508a8d9.
The preceding capture matrix plus this actual vertical-room and progression
qualification accepts G06; the lower native admission/readiness and distant
room boundary qualify A04. A02/A03/A05/A09–A13 and other captor families
remain open. Accepted total103/180,77 open,3/15 areas. No release or owner
playtest has been performed.


### Toxic Seahorse: actual acid-carrier ownership and shared progression

G03 is accepted;104/180 accepted,76open/partial,3/15areas. The actual stage3
class59 encounter earns its capture through13:F641 contact and13:F87E hostile
allocation. Its class22/variant0 carrier has no initialized HP. The previous
nearest-target dispatch dragged the free actor while its actual victim stayed
behind; public withdrawal also succeeded while capture96 was pending.

Bind only the successful native carrier allocation (02:D8F8, stored JSL return
13:F881) to the actual projected victim and parent NPC. Eight private hostile
tags live alongside27 native frontier spans in the existing67-byte shared-world
block; at most22 spans are needed. NPC attachment and effect tags retain their
separate ownership. The original constructor, upward motion, ceiling test,
D0C0 release and six-point damage run once against that victim. Constructors
with zero HP still retain their earned owner. Release, retirement and allocator
reuse clear the carrier tag; the parent follows the same actual capture.

Eight roster/victim/anchor cases pass the complete22-update original comparison:
body, auxiliaries, shots, palette, carrier and parent bytes match each frame.
Pending capture refuses withdrawal; the released actor can withdraw, leaving
the original32HP encounter and free partner controllable. Complete futures
replay exactly. native-toxic-final-hold child0; stateSHA256
576a0f63d06901de5cb76d548070f4d4228e51a4b88c587a6968c91a01dc6eb4.

Eight actual free-partner buster clears and eight captured-death survivor clears
pass on9c6afc282be1fc79affbd2b170189f9798ad99d13b1db5c26a0ad43a87248ed9:
native-toxic-qualified-clear and native-toxic-qualified-survivor child0. A one-HP
captive dies on the actual21st ceiling update; both bindings clear, only that
actor falls, and24 survivor updates retain movement and the living32HP boss.
Native ordinary busters reflect during the occupied guard, then earn the later
vulnerable defeat. These clears are not evidence of rescue by killing an
occupied captor. The native13:F7BF reward writes1FC6; its acquired bit is20 and
the final byteDC. One reward and complete stage1 arrival retain lives2, clean
old bindings and preserve the original living/fallen roster. Both runs of each
case replay the entire machine; return2433updates for living pairs,2553 for the
fallen-captive cases, followed by64 arrival updates.

Current production also passes the full existing Crawfish/Buffalo hold/release
references (native-toxic-captors), native activation/support/shared-assist
regressions (native-toxic-frontier-regression), mid-allocation codec and
transactional preflight (native-toxic-preflight): all child0. Invalid hostile
tag31 is refused without changing the machine; both rosters pass27 malformed
cases and four truncations. native-toxic-final-builds child0 includes the GUI
and four CTests; GUI SHA256
c34ceb9e85ff7c520229c516bf62a86a55fb2ba01ca267bbbfad8ac6f0ef7395.

Other capture families and E07-E10/D12 remain open. A03 cold upward admission
is the next active check; no owner playtest or release has occurred.


### Cold upward streaming and native camera-command lifetime

A03 is accepted;105/180 accepted,75open/partial,3/15areas. The actual stage2
long ladder admits source3C:D557/D55E at1408/640 (flagsFA2F/FA34) on native
world step57, before the actor enters the upper region. The original culling
helper03:DDF3 only retained each camera-command rectangle against the canonical
camera. With the other actor holding a distant ladder, the second view's
controller was incorrectly retired on step78 although its view still needed it.

At03:DE3E, retain the original command while either living view intersects its
authored rectangle and native64/320 margins. The cartridge still executes each
command once against the canonical body; the existing parser supplies private
limits. Hook the separately emitted03:DDF3 culling entry itself; routing its
caller03:DD76 did not expose the nested compiled helper. No state-format change
or actor projection is added. All487 listed native region rectangles across
15 stage lists use ordered coordinates in0..8192, including stage14's alias.

Four actual roster/seat cases pass on native-region-qualified, child0, state
SHA256cc59bce9f16929bb4ee729ef88ad87604e5786cc703691c11fe07b25a2027c33.
Each original/paired240-update Up/Down/Jump sequence matches event eligibility,
body position/velocity/pose, actual private camera and every current limit;
complete futures replay exactly. Both controllers are born on step57 at
actorY767/cameraY671. X climbs to687 and Zero to679 above the real704 ladder cap,
then descends and jumps off through the native path.

A native64-update lateral pan exceeding one screen first fills the lower room's
actual VRAM while both upper flags remain0. Merely teleporting the native camera
left an incomplete visual reference; no compositor repair was justified by that
setup. Sixteen native/private world scanouts at steps57/91/120/240 match exactly,
covering admission, approach, top and reversal. Only the actual top-left HUD
rectangle is excluded from this world comparison; the private owner badge is
intentionally distinct from the retail X badge, and HUD ownership has its own
gates. No extra simulation is run for the private view.

After native Jump detaches the other rider too, public withdrawal removes only
the tested view and the native commands retire. A separate fallen lifecycle
precondition verifies the same exclusion without claiming a native death test.
All8 retirement probes leave the distant living actor progressing8 world ticks;
the complete living future restores afterward.

Affected native-region-world-regression and native-region-captors both child0
on0f8ccabfec23791af8188e3f5172b0ee6fccd0f738e95e4e76c94ffabbf0602d:
actual activation/retention/support/shared assists, mid-allocation/preflight,
and the full existing Crawfish/Buffalo/Toxic hold/release references. Both rosters
retain27 malformed-input and4 truncation refusals. native-region-final-builds
child0 includes production GUI and4CTest; GUI SHA256
01cf0f6e3507f7fd4453296918951771a57500fc885b9effb85ee93870f78689.

Other frontiers, complete per-stage presentation, remaining campaign mechanics,
owner/DualSense checks and final release remain open.


### Native Tunnel Rhino hazards, drill deaths and complete clears — G04

G04 is accepted. The native checkpoint2 loader and actual P2 Right door route
reach class57/32HP with both actors released. The real13:E493 controller earns
its normal leaps and drill attack; no attack state or projectile is supplied.
At32HP the large class23/variant0 drill travels at least80px clear of the boss
before contact. At an assisted8HP the native controller selects its alternate
attack and emits class23/variant4. These health values select real native phases;
the test never supplies damage, defeat, death or reward flags.

Eight32HP roster/victim/anchor cases and four8HP roster/victim cases let a real
separated drill kill the approached one-HP actor on its first update. Existing
hostile scheduler boundaries observe that exact source earning the fatal hit.
The other actor then moves for24 world ticks while the live boss retains32/8HP,
with no team fade, wipe or freeze. Every complete-machine future replays exactly.
The first floor-height probe missed the airborne drill in native geometry; the
qualified approach uses its actual height. That setup miss was not a game defect.

All eight32HP survivor cases and four low-health living-pair cases also earn an
actual partner buster defeat and complete the original reward and next-stage
arrival twice.13:E9C7 writes the native1FBC reward: acquired mask01, final byteDC.
Exactly that weapon is added; prior acquisitions and lives2 remain unchanged.
The fallen actor stays fallen, or both living actors remain alive, through the
2553-update return and64 further arrival updates. Native floor1216 and neutral
firing prerequisites are used; no defeat or transition result is forced.

Outside the arena, the actual checkpoint1 loader prepares the authored
class0B/3C:DFA8 at4752,880 (flagFC14). Original DB06 column allocation and native
constructors establish an entered-room prerequisite; this bypasses traversal and
is not a cold-activation check. Both characters in either seat match the original
controller for96 updates: body, motion, source, personal pools and palettes.
Actual contact on update2 removes twoHP (X16→14, Zero28→26), the free actor stays
alive, the world advances96 times and every full-machine replay agrees.
A prior mere teleport skipped native admission and was rejected as an invalid
setup. No engine/runtime correction was needed for these qualified mechanics.

Evidence in build-coop/qa-state/native-crawfish-capture-qualification.json ->
tunnel_rhino: native-rhino-survivor-clear child0 on state463635e6;
native-rhino-low-fatal and native-rhino-living-clear child0 on1332851d;
native-rhino-stage-qualified child0 on2f68e18c; native-rhino-ctest child0
(four checks). Exact hashes, fixture paths and receipts are retained there.
The earlier native stage table's boss-name labels have also been corrected;
its numeric stage/class/reward results were already accurate.

Actual106/180 accepted,74 open/partial; R/M/N remain the three accepted areas.


### Native Volt Catfish electrical contact and parent cleanup — G05

G05 is accepted. Actual stage5 checkpoint2 and P2 Right door inputs reach the
native class58/32HP encounter, both actors free, floor2992 and camera3328/2816.
The existing earned-projectile/replay check now also uses this original arena,
class27 electrical sources and native13:F0BE reward at1FC0 (acquiredmask04/DC).
No attack, capture, death, defeat or reward result is supplied.

One persistent electrical source exposed a co-op contact defect. The original
controller's actual electrode41/primary2 damages native X16→12 and Zero28→24.
With both living actors overlapping, the previous paired update gave12/28.
Its single-use-shot guard stopped after the first hit even though this source
remained live.08:AFEB/B157 ignore the CB31 result; their native parent/movement
phases retire these hazards. Class27 now follows the existing persistent-ice
continuation and tests both eligible actors in that one world update. Other
single-use hostile hits still stop after their consumed contact.

All four roster/anchor overlap cases now match each character's actual original
damage, update the source once and replay the complete paired machine exactly.
Actual consumed class03 shots and successful native A/X/Y/D/S/P/DB/PB, scratch,
object and health returns still pass. No state-format change was needed.

Eight32HP roster/victim/anchor cases earn the normal class27/variant2 electrical
chain, advanced81px from its real parent (parent3416,2967/source3497,2962).
Its actual update kills the one-HP actor; the free actor retains24 real movement
ticks beside the live32HP boss. Real survivor busters then defeat it and finish
the original reward and next-stage arrival twice per case. Four living-pair
cases use assisted8HP to earn the actual action8 floor-electricity sequence and
variant10 side electrodes,40px from parent. The native super guard is respected;
ordinary fire earns its later vulnerable defeat at360..370 updates. Both living
actors retain their roster through the full return. All twelve clears add only
mask04, preserve lives2, retain fallen/living states and complete2553 return
updates plus64 arrival updates, with exact whole-machine replay.

All four actual electrical parent kills separately verify zero active class27
children20 updates after defeat, while the parent remains primary4 and before
a reward or stage loader can clear the pool. These complete futures also replay
exactly. The child's actual+3E parent is retained from native13:F1F2/TDC.

The isolated sphere0 probe did not select an in-arena separated source while
the protected actors camped at the wall. Its native origin is parentY+16; that
setup is not a proven co-op defect or individual sphere qualification. The real
normal chain and super-floor/electrode sequences above are the accepted authored
checks. An initial180-update firing bound ended during the native guard; its
600 bound permits actual later defeat without forcing vulnerability.

Exact evidence: build-coop/qa-state/native-crawfish-capture-qualification.json ->
volt_catfish. native-catfish-normal-survivors, electrical-living,
overlap-qualified, cleanup-qualified, contact-regression, rhino-regression and
final-builds all child0. Full mechanics stateSHA3e7d81a3; final cleanup/Rhino
stateSHA0eb8f126; GUI SHA256
d239b97e43ba7ed6453dc26bc397373f433f12a90b9f1bb473bd956a75d4b3bf.
The production GUI and fourCTest checks pass. Actual107/180 accepted,73 open;
R/M/N remain the three accepted areas. Remaining capture, visual and other
campaign criteria receive no extra acceptance from these permutations.


### Native Neon Tiger rays and both-wall rush — G07

G07 is accepted. Actual stage7 checkpoint2 and native P2 Right door inputs
reach class55/32HP with both actors free. Actor5915/431X or423Zero, boss6096/424,
camera5888/256 and floor448. Its real dispatcher is3F:E5E4, main3F:E793 and
defeated3F:EAE4. The13:CB87 capture belongs to another native class and was
not attributed to Tiger.

The actual class26/variant2 ray remains source41/primary2 after contact.
Original character references deal four damage each; the previous paired
update damaged only its first actor. Native08:ABF5/AD57 ignore CB31's hit
result and retire rays by movement or parent phase. Class26 now joins the
existing persistent20/27 contact contract, allowing both eligible bodies in
one source update. Four roster/anchor original-versus-paired comparisons now
produceX12/Zero24 orZero24/X12, retain a live source and replay the entire
paired machine exactly. Actual consumed class03 shots, balanced native
successful registers/scratch/object/health and four Catfish overlap cases
still pass. State format remains unchanged.

Eight roster/victim/anchor cases earn an actual ray after native wall contact:
f162, wallflag2, parent5934/424, source6114/275, sourceKind4 and parent+38=0D18.
The actual ray update kills only the approached one-HP actor; the surviving
actor then moves for24 world ticks beside the live32HP boss. Real survivor
busters defeat it and finish native1FCA/acquiredmask80/DC reward,2553 return
updates plus64 arrival updates. Lives2 and the fallen roster persist. Every
fatal/clear/return future repeats exactly.

A separate actual low-health route supplies only bossHP8 and personal player
protection. In all four roster/victim cases the original AI selects its native
action2 rush and physically contacts BOTH authored walls (flags3), over441
video/world updates, with bossHP8 and both actors alive. Every complete route
replays exactly before real living-pair buster defeat and full reward/arrival,
also repeated twice. No attack, wall-contact, death, defeat or reward flags are
supplied. The authored interaction is a grounded wall slam atY424; an earlier
invented climb-height prerequisite was rejected. The earlierHP8 ray cut was
not treated as qualification of this later rush.

Four real parent kills separately leave zero active class26 children20updates
after defeat, while the parent remains primary4 and before a reward or stage
loader can erase the pool. Those full futures replay exactly too. Source
observation, native cartridge contact and actual progression determine results;
position/health/protection prerequisites bypass difficulty rather than the
mechanics being accepted.

Evidence: build-coop/qa-state/native-crawfish-capture-qualification.json ->
neon_tiger. native-tiger-survivors-qualified, overlap-qualified,
cleanup-qualified, rush-qualified, contact-regression and final-builds all
child0. Mechanics stateSHAa0af33db; rush stateSHA092f871a; production GUI SHA256
32b324ba06ec905d6683d342a72a8c11164a05e57f1701e832eee694cb8c44fe;
state/GUI/fourCTest pass. Actual108/180 accepted,72 open,3/15 areas(R/M/N).
Other capture families, visuals and remaining campaign routes stay open.

### Native Gravity Beetle orb and timed-field contracts — G08

G08 is accepted. The actual stage8 checkpoint2 route and native P2 door
inputs admit class56/32HP with both actors free, floor1472 and arena7424..7680.
Native13:DCA0/DE40 selects its own attacks. Moving kind0 orbs and parent-following
kind2 fields are actual class25 children with the allocator-earned parent+38.
Their angular variant0B is native state, rather than a fixed synthetic variant.

The moving orb exposed a persistent-contact omission: original X16→14 and
Zero28→26 became14/28 in the previous paired update. Native08:AB5C ignores
CB31 hit and leaves the orb alive. Class25 now joins the existing20/26/27
persistent contract; ordinary consumed shots retain their one-hit guard. Four
roster/anchor comparisons now damage both bodies exactly as their original
references, update source01/primary2 once and replay the complete machine.

Eight roster/victim/anchor cases earn the native small orb atf325, source
7568/1463 and parent7649/1437,81px apart. Its actual update kills only the
approached one-HP actor. The survivor retains24 real movement ticks beside
the live32HP boss, earns its defeat with a real buster, and completes the
native reward and full next-stage return. Both roster orders and either
survivor retain the fallen actor without revival or a life debit.

With only bossHP assisted to8, the original AI earns its actual timed field
atf105, parent/source7632/1447, native animation0F&40 pulse, sourcekind2/HP8.
The native06:EFA4/EFAE hitbox projects forward and upward. An approach24px
forward and50px above lies inside that hitbox and outside the boss body.
There is no invented96px aura, gravity pull or stun. Four original/paired
comparisons prove actual eight-damage field contact for both native bodies:
X16→8/Zero28→20, with one source update and exact complete paired replay.
Four actual source-observed field fatalities retain the living8HP encounter
and survivor control, then earn native survivor clears and stage returns.
Four living-pair cases also defeat that native field phase and complete the
full reward/return. Native guard/recovery runs normally; the living-pair
buster earns defeat at141 updates. No attack, death, defeat or reward result
is supplied. Position, health and personal protection are prerequisites.

All sixteen full clears add only native1FC4/acquiredmask10/DC, preserve lives2
and the living/fallen roster, and complete2553 return plus64 arrival updates.
Every complete future repeats exactly. Four additional real parent kills
leave zero active class25 children20updates after defeat, parentprimary4,
before reward or loader reset; those futures also replay exactly. The earlier
60px forward setup could not select an eligible in-arena field and was rejected
as a setup failure; the actual24px/−50px native hitbox approach qualifies it.

Evidence: build-coop/qa-state/native-crawfish-capture-qualification.json →
gravity_beetle. native-beetle-survivors-normal/reversed, field-overlap,
field-survivors, field-living, field-cleanup, contact-regression and final-builds
all child0. Actual consumed03, balanced native registers/scratch/object/health,
and four each prior Catfish/Tiger persistent overlaps pass. State/GUI/fourCTest
pass. Orb mechanics SHA6181d5f4; timed-field/regression SHA
5cb436a84489995a56f9dcd43d8859c2f38fc9a6c1a054b8ee69e3fb17a4b991;
production GUI SHAbe904e6f673fbc5386d363cbd1a8ba3e6cf8ab87ac0ba794464798edb9b56c64.
Actual109/180 accepted,71 open,3/15 areas(R/M/N). No other capture, visual or
campaign criterion receives acceptance from these permutations.

### Native Blast Hornet attachments, marked targeting and clears — G09

G09 is accepted. The current stage1/class53 checkpoint2 route earns its real
32HP introduction and control release (loader57/ready307/intro540), floor2224,
arena7936..8192 and camera7936/2048. Native39:9BA9/9DF6 runs the boss once.
Actual39:A1DA/A1A1/A211 create class21 normal bees, low-health homing bees and
the target marker, with their real parent+38. No attack/latch/result flags are
supplied; position, personal protection and assisted low bossHP are prerequisites.

Three measured co-op defects are corrected. A real kind0 bee and kind4 target
marker changed to the nearer free actor after their CC5C latch, while original
native references retained the actual owner. Existing per-hostile owner tags
now retain that actor through native follow/timer/retirement; a bee or marker
does not turn the otherwise free actor into a captive. One actual active bee
also stopped its CB31 continuation after the first actor. Source-specific
observation proved native damage2 for each original body but only Zero damage
in the previous paired source update; a later different bee could conceal the
omission in whole-frame health. Class21 joins persistent20/25/26/27. Ordinary
consumed shots retain their single-hit guard.

The real low-health marker then exposed an independent homing defect. With
parent3F=80 and source1358 at its native steering timer, the marked character's
original reference selects angle10/velocity01C9,FF1C. The old paired update
selected angle12/0169,FE97 toward the nearer unmarked actor. Each actual marked
homing child now inherits its same-parent latched marker's owner and retains
that owner through its own lifetime. Unmarked flight keeps the existing
nearest-actor selection. Four native/paired steering references now match
exactly. Existing world owner tags suffice; there is no snapshot-format change
or additional hook.

Four normal-bee cases match the original body, source, personal pools and
palette for the entire226-update sequence: actual180 timer, native doubled
explosion damage4 and full retirement. Four marker cases match eight native
follow updates; four kind2 impact cases match their native stationary impact
behavior for eight updates. Each paired complete future replays exactly.
Four actual active-source comparisons independently prove both native two-damage
contacts in one source update. Eight real absence cases (four bees/four markers)
retain the withdrawn actor's personal health and actual free-partner movement;
the source retires natively with no owner transfer. Every future repeats exactly.

Eight roster/victim/anchor normal-explosion cases wait191 native updates for
the actual four-damage explosion, then supply one personal HP. Source observation
proves its actual fatal hit; the survivor retains24 real movement ticks beside
the live32HP boss. Actual busters earn defeat, native1FBE/acquiredmask02/DC reward
and2329 return plus64 arrival updates. The native loader is observed before
accepting the new stage1 entry, whose ID repeats. Lives2 and fallen status
persist; every full future repeats exactly. Four low-health marked living-pair
cases also earn real buster defeat, the complete reward and native arrival,
with both actors alive and both native hurt reactions recovered. Four additional
actual parent kills leave zero class21 children20updates after defeat, before
reward or loader reset, with exact complete replay.

Rejected setups remain explicit: an initial pre-draw bee had no active native
contact; an early homing approach was still in its noncolliding launch phase.
A floor-level firing line passed below the healthy airborne boss. The preserved
machine showed advancing AI and a newly repeated marker. An initial muzzle-height
approach instead lets the real buster earn defeat. A reused captive-release
assertion also incorrectly rejected ordinary Hornet hurt action8/reaction8;
Hornet does not capture, and completed native recovery is required at actual
arrival. Neither setup failure was claimed as a game defect.

Evidence: build-coop/qa-state/native-crawfish-capture-qualification.json →
blast_hornet. Normal survivor SHAaec958e2; initial contact/timer SHA07ba17e8;
final marked/owner/living/absence/cleanup/regression SHA
9ac8bdb282f6d4efa6b42fd9fad57fc15da115cf1c411a7ec62644611356f83a.
Production GUI SHA7d6169e6cbb17bf56a895f147f5be51c3a319e38d0424cb31bf1fe3a2e9042fe. Consumed03,
balanced native registers/scratch/object/health, four each Catfish/Tiger/Beetle
persistent contacts and four Toxic native carrier/hold/release comparisons pass.
State/GUI/fourCTest pass. Actual110/180 accepted,70 open,3/15 areas(R/M/N).
Other captures, enemy families, visuals and campaign branches remain open.


### Bit/Byte native ownership checkpoint — G10 remains open

G10 is partial; the suite remains110/180 accepted,70 open and3/15 accepted
areas. Actual stage2 authored records admit Bit(class46) and Byte(class47)
under native weapon-count prerequisites, without writing hunter admission,
visit or outcome flags. Real buster/weakness hits earn retreat/destruction;
sequential encounters earn all four combined results05/0D/07/0F and visits22.
Four roster/anchor cases per outcome retain both living actors, lives and
acquisitions, cross the actual next-room boundary, and replay the full future
exactly twice. A checkpoint past the one-way hunter room is not a valid entry
setup; the supported preparation starts before it and admits the hunter with
real movement after preparing only the skipped pre-room columns.

The native defeated controller previously lost D1CA's actual recipient, leaving
nonleading Zero locked after Byte's destruction. The existing attachment byte
now retains that recipient until native D1EF. Bit destruction also exposed a
free partner falling through the floor while the cutscene suspended terrain
collision. Its native script owner continues updating; the other body uses the
existing balanced controller parking path. All four Bit destruction cases now
retain both living actors and their actual room exit. Live ring attacks leave
the partner free to move.

Actual native kind0 class2A ring contact earns the captured actor, which remains
bound through the original hold and physical escape. Eight roster/victim/anchor
references match every original body, personal pool, palette, ring and parent
byte:64 held updates without escape edges, physical-input release90 and full
retirement91. Busy withdrawal is refused; each full paired future replays
exactly. An initial parent-overlap approach and a later unprotected interruption
by parent damage were rejected as setups for the isolated escape check.

Actual oneHP ring deaths now pass all eight roster/victim/prior-anchor cases.
Native04:D8D5 previously saw the projected fallen captive and permanently
set Bit's retirement byte, disabling subsequent attack collision. The existing
Crawfish captive-death boundary now also keeps Bit active while its ring
releases. Both actual survivors earn a real buster retreat and cross into the
next room, preserving the fallen actor, acquisitions and lives. The complete
futures replay exactly twice. Physical aiming follows the moving hunter; a
stale initial facing was a separate test prerequisite.

Eight occupied-ring parent defeats also retain both living actors and finish
the actual next-room exit. Native AC5A deferred the capture latch to the
defeated parent; in-stage retirement could leave that latch set after the ring
was gone. The existing hostile boundary now clears the earned ring's latch
with its retirement. Source/lock/latch cleanup and complete replay pass. A
second roster reversal inside the nested outcome test was a setup error and
is corrected without changing runtime roster behavior.

All four earned results05/0D/07/0F now admit their actual fortress branches
through completed native door scenes. The earlier outside P1 observations
stopped before scene completion; both living actors now enter together without
a door runtime change. Parent5A selection requires kind0. The lower05/0D/07
branches each pass four roster/shooter real projectile defeats and full native
Lab2 Vile-branch(stage14) arrivals, with unchanged earned hunter results/lives
and complete replay. All eight Maverick clear/acquisition flags are explicit
fortress-unlock prerequisites; they do not fabricate hunter results. These
checkpoint approaches still bypass earlier fortress traversal.

The upper0F route now qualifies its separate native trunk and head phases.
The free actor earns the trunk defeat with actual busters, retains incidental
head damage and waits for all three owned shots to retire before aiming the
head. Four living and four actual parent-contact survivor clears reach the
native Lab2 Vile branch with exact complete replay. The actual trunk future
also replays separately, including natural shot retirement.

Eight native kind2 Press-claw captures retain their actual victim through the
four-update carry, edge release and damage, matching original body, pools,
palette and source bytes. Eight actual captured deaths retain a living parent
and free survivor, then complete real head defeats and full native arrivals.
Eight actual head shots kill an occupied parent before the claw releases.
Retirement previously skipped its D042/D0C0 release; the existing NPC boundary
now applies D0C0's mapped body writes and lets the native queued88 controller
finish. The actual D1CA recipient owns the defeat script, while the existing
balanced parking path protects the other actor while terrain is suspended.
Child/owner cleanup precedes the next-stage pool reset. Every full future
replays twice; lives, living/fallen roster and earned hunter flags are retained.

G10 remains open for the lower fusion boss's remaining native mechanisms.
Its actual32HP AI now earns a class43/kind2 arm capture, and comparison with
the native body exposes another nearest-free-actor ownership mismatch. The
kind2/4/6 arm and carrier sequence must be qualified through its real release,
retirement, survivor and clear consequences. The entry/assisted-clear evidence
above does not substitute for that work or G12 traversal. The suite remains
110/180 accepted,70 open and3/15 accepted areas.

Evidence: build-coop/qa-state/native-crawfish-capture-qualification.json →
bit_byte and exact named job receipts/logs. Earlier in-stage outcome SHA
48f0a19f91f5856e50c44c7ff8c91ba971909dc3a3d49e255b96f6f63e9c8dac;
living ring reference SHA
1c23e45f34f195f88e8c3ee4b50c2a04d61eee1a6b2f96f7d0559a8098b9a8a6;
retired-ring repair SHA
fd2af7e9f7bb251f760d8d4e01369203013f31fe780a28695bf510d5681b6d23.
Bit/fortress qualification SHA
6bc680ebf364ee56f3729b66a3c556c3fe6ed986ce5135d79f30e8987de7e418;
reversed occupied-ring setup correction SHA
8756454a40aa19189aa08c054d5894bf8c9ddc935027df202bd54d096dff3e06.
Press native/living/survivor reference SHA
9e0bb8d0e90bb1c2610976cc3dae2b3b43ecde8a22c0f02a7cf0f28952a3ff35;
final captured-death/occupied-parent qualification SHA
5403a01b4cb3b32ea68b30283b2a259fcb507bb8dd928b2b9621b36a906411d1.
The current production GUI SHA is
4008b40165bda667bdaeb30c006c6de9f6287ce753e84495bfccdc611c03665a;
its build and all four standard CTests pass. Individual unqualified frontier
setups remain in the existing private compact record. No snapshot-format or
hook-list change, owner validation or release is claimed.


### Fusion capture group and retired-victim release: G10 accepted

The lower fortress fusion boss coordinates NPC43 kinds2/4/6 through CD90/CD91.
Its actual arm capture now retains D034's recipient across the slammer and
newly allocated carrier, using the existing attachment bytes and world state.
The root keeps this metadata while targeting a living survivor after captive
death. The actual D1CA defeat recipient takes ownership of the outro.
No snapshot-format change or additional native hook is required.

All eight roster/victim/prior-anchor cases pass the complete396-update native
arm/slam/carrier/release reference, including the carrier's actual allocation
at324. Every byte of the victim's body, own pools/palette and coordinated
captors agrees on each update; the entire paired world replays exactly twice.
The separate static kind0 body can contact the free partner and is covered by
paired-world replay. Healthy X uses the legitimate eight-heart32HP maximum:
the unupgraded native16HP reference dies during the slam, rather than reaching
a living release. Native attack and capture/result fields are never supplied.

Native F5ED skips carrier creation when the slam kills its captive, leaving
F56E waiting forever on CD91. A retired bound actor now takes that arm's native
D0C0 release and completed-carrier handshake. Sixteen fatal cases cover death
during the arm hold and later slam, both rosters/victims/anchors. Each retains
the32HP live parent, clears the binding, permits24 free-survivor updates and
earns the actual survivor kill and Lab2 stage14 arrival with the original
fallen actor and lives retained. Complete futures replay exactly twice.

Eight actual parent kills during the arm hold also pass native child release,
binding cleanup before pool reset, both living arrivals and complete replay.
All eight occupied-carrier parent kills also pass that distinct native release
path, child-binding cleanup before stage reset, both living arrivals and exact
whole-world replay. Together with the actually earned05/0D/07/0F hunter outcomes,
completed lower/upper doors and each branch's actual fortress clear/next-stage
consequences, this accepts G10. The suite is111/180 accepted,69 open and3/15
accepted areas. G11-G16 remain open; these checkpoint approaches do not qualify
untraversed fortress routes or Vile combat.

The full native-reference/fatal test binary is
`7ff6c6293a964ba74381af354ab5a9d353dd84dc1a85ff99a347246d98f2c969`;
the expanded carrier-parent observer is
`fa7eeaf6bb4707fdaa7f26f826222265a0ed46ebb3549d3f7623ce4e6e4e2b62`.
Production GUI
`2421cb0fa8412b3662d9e08204b9c9d3780c15b58f10fc196d5712b088fb8a11`
builds, all four standard CTests pass, and focused actual Bit-ring and Press
claw references pass after the capture-group changes. Remaining campaign,
scene, presentation, owner and package gates remain open.


### Native Lab2 Vile two-form continuation (G11 partial)

The actually earned first-fortress result now supplies the Lab2 stage14
checkpoint2 fixture while retaining its original1FAF fortress counter1. Only
earlier ordinary travel is bypassed. Four roster/physical-leader cases execute
the real doors, descent and armor introduction, admit both living actors and
replay the entire1909-update future exactly twice. No admission or scene-phase
flags are supplied.

Four roster/shooter cases then defeat the actual class63 armor and its
originally32HP class61/kind2 on-foot successor. Each form first starts32HP,
then survives16 idle updates at assisted1HP before a real normal-buster
collision earns its defeat. The native armor-to-foot transition takes752
updates; actual postfight confirmation and loading reach Lab3 stage12 with
fortress counter1FAF advancing1->2, unchanged hunter outcomes/lives and both
original full-campaign actors alive and controllable. Each complete two-form
future, including next-stage construction and64 arrival updates, replays
exactly twice. A separate stage-derived1FD5 byte is not the fortress counter;
the setup retains earned progress rather than resetting it with the generic
standalone-stage helper.

These native checks pass on state test
`a768a6381d2d7df9e6d237810f8610a60c025b316d55f00be632d40f55dbaea5`.
Production code remains at the qualified fusion-capture implementation.
G11 is still open for Vile's earlier optional route/outcome and its alternate
later branch. This does not accept untraversed factory/fortress routes or
substitute for the remaining survivor, scene or presentation checks. The suite
remains111/180 accepted,69 open and3/15 accepted areas.


### Native optional Vile outcomes and later routes - G11

G11 is accepted. Real class1A contact now retains its actual recipient through
native EC4C/EC72/ED00 transport. Full-campaign Zero passes the native stock-only
character exclusion, retains his real character, and uses his own standing
foot alignment at the X-centered capsule. The balanced item return commits the
recipient's view and native terrain/controller anchor together. Height alignment
alone did not repair the nonleading transport; committing its actual owner did.
Stock restrictions retain the original eligibility rule.

All eight roster/physical-leader/anchor combinations earn the Buffalo-to-factory
warp, preserving native source2/return-marker0, hunter0F, lives2 and both living
actors. The native X control takes56 warp plus256 arrival updates; paired cases
take57/58 plus291/292. Actual mid-scene save/load produces the same complete
future as uninterrupted transport. A stale frontier cache in the first manual
approach was a test setup error; the qualified comparison admits the real item.

The actual factory arrival then passes both doors and the armor introduction.
Each native32HP form remains live for16 idle updates after a1HP assist; only a
real projectile earns defeat. Normal buster clears pass with four roster/shooter
permutations and earn2F. X's actually acquired selection10 produces a winning
native class1C on either physical seat and earns3F. Both native outros release
the pair, and real contact with the authored return capsule reaches source2,
checkpoint1, without changing lives or reviving/withdrawing a participant.
Every complete two-form/outro/return future repeats exactly twice.

The native defeated controllers for61 and the alternate Lab2 main4A now retain
D1CA's actual recipient through the existing attachment bytes and balanced
controller parking. Without61 binding, a retired factory Vile left Zero locked128
after3600 updates. Without4A binding, the alternate clear advanced the campaign
with Zero fallen. The qualified main4A clear retains both living actors through
2177 updates and actual Lab3 arrival. No new snapshot fields or format are used.

Actually earned2F and3F are carried through a real first-fortress head defeat.
The original selection reaches stage14 after2316 updates for2F and stage11 after
2318 for3F, with native counter1 and the pair intact. The later stage14 armor63
and on-foot61 clear reaches stage12/counter2 after3497 updates. All four alternate
Lab2 door leaders and main4A shooters pass; the main-only HP assist leaves the
native32HP children untouched. Both later branches preserve their earned Vile
flags and lives, and every complete clear/next-stage future replays exactly.

Ordinary approach/escape terrain and prior campaign unlock travel are explicit
bypasses; direct fortress entry supplies the eight Maverick prerequisites, never
Vile or hunter results. Those traversal checks remain under G12. Scene-owner
death/wipe and the broader lifecycle criteria remain independently open. GUI
40381098ae1a63ea0a790d9afdbc3d74f7f2b52e9020bc2e32f48e772b32bd17 and all four
standard tests pass. The main4A matrix uses state8d3b1e5a; raw jobs, samples,
source/binary identities and counterexamples remain in the existing private
qualification record. Actual total112/180 accepted,68 open,3/15 accepted areas.


## Native Mosquitus and saber-transfer qualification

G13 is accepted. Native freeze/release boundaries now retain the actual script
recipient in the existing serialized NPC attachment bytes. This keeps Zero's
winning hit on his original encounter branch and gives the triggering physical
controller ownership of subsequent dialogue. Live introductions release that
binding normally; defeated scripts keep it until retirement.

The original story helper temporarily reconstructs X in the solo player body.
The existing cinematic bridge now retains both real participants through that
sequence, supplies retained X health instead of the unused solo exchange backup,
and restores the living roster at the finished native floor. Previously the
sequence could strand Zero or interpret the unused backup as X's death.
Forced scene returns also clear the abandoned action's native charge-inhibit
counter; retaining it allowed X to walk/fire but prevented charging afterward.
No new snapshot fields are needed. Actual mid-cinematic save/load repeats the
complete future; an unsupported cinematic stage is rejected transactionally.

Both rosters and physical Zero seats earn the native saber/loss flags, preserve
X's 13 HP and selected weapon, retain full-campaign Zero at the native 1 HP,
and cross the real exit/checkpoint together without a life debit. Both world
anchor choices pass. A withdrawn or previously fallen X retains that exact
participation state without revival. Actual X-finisher counterparts finish
without awarding the saber. Prior fallen/withdrawn setup uses lifecycle APIs;
these cases do not claim authored fatality coverage. Ordinary route approaches
are explicit positioning bypasses; actual encounter admission, winning hits,
story results and exits are earned.

Stock restrictions allow an already-called Zero through the authored Mosquitus
approach and encounter. The actual native award then permanently retires him,
recalls a living withdrawn X when needed, refuses Zero rejoin and lets X exit.
Canonical and reversed rosters pass. Full-campaign policy retains Zero despite
the native stock call predicate becoming false after that same story award.

The actually earned saber is usable by X beside living Zero. On both rosters,
420-update comparisons match each character's original commands, charge, pose,
controls, palette, owned pools and real native damage windows on every frame;
both reach native charge12. X's saber damage is active at updates279-280 and
Zero's at303-315. The complete paired combat future replays exactly. Existing
intro capture/rescue/helper-death checks and the alternate Lab2 main clear
through actual Lab3 arrival pass with the updated runtime.

State qualification uses5ce35d96881ca61994100ade37f6fe6b3d563efe644858b603667a74a891f4fc.
GUI12c96180e2933c762931f87f727a4c92b2b14dde77663eef168dd8d598596205 builds and
all four standard tests pass. Exact jobs, counterexamples and per-build scope
remain in the existing private qualification record. Actual total113/180
accepted,67 open,3/15 accepted areas. G12 and G14-G16 remain open, together with
the separate lifecycle, scene, presentation, runtime and final owner checks.


## Native refight sequence qualification

G14 is accepted. Each of the eight authored class0D pads admits its native
refight, and a real projectile defeat earns exactly one new native1FDA clear
bit before the original automatic hub return. Four complete sequential routes
cover both rosters and X/Zero as the entrant and finisher in either physical
seat. Every route earns masks01/03/07/0F/1F/3F/7F/FF, preserving lives2, the
actually earned saber flagsDC, hunter/Vile results3F and both living actors.
Returned bodies retain their actual native character and can charge.

Each real pad contact/introduction replays its complete future twice. Each
actual winning hit, native clear and automatic hub return also replays its
complete future twice. The return takes111 updates after the measured outro;
no return-position assist is needed in these cases. Each boss starts with
native32HP, remains live during an idle16-update 1HP assist, and loses its
remaining health only to a real native projectile. Approach coordinates and
personal protection bypass ordinary travel without supplying a result.

The first original solo pad reference reaches its live refight after629
updates. An earlier observer rejected that same live32HP main because its
authored kind byte had become native AI state; the qualified observer follows
the active native main. Type0 records identify the actual item pads. A prior
type2 camera-event approach was a test setup failure in both solo and co-op.

The actual Mosquitus saber award was also carried through the native Lab2
checkpoint/door, main4A defeat and original Lab3 loader. Both actors remain
enrolled with native X/Zero identities, and the earned DC story flags survive
into these refights. Exact source/binary identities, per-route jobs and native
clear/return samples remain in the existing private qualification record.

The refight sequence state binary is
733e61ee19f26dc7bfe01ca24a340318715b50f5154a246157c0c92f78e8fdc5, using the
qualified production runtime from b621614. Actual total114/180 accepted,66 open,
3/15 accepted areas. Unlocked-hub platform traversal and the remaining fortress
routes stay under G12; final phases/endings and separate authored death/scene
criteria remain open.


## Doppler native script world ownership

Doppler's actual final hit previously retained the correct NPC script recipient
but left shared dialogue/stage tasks on the other actor's canonical body. Both
X and Zero then remained in the native death continuation after6400 updates.
Selecting the real finisher as world owner before the same encounter completed
both native futures at4807 updates, establishing the ownership dependency.

At the balanced NPC return, an actually tagged native script now commits its
living recipient's body/view as world owner. This reuses the existing native
freeze binding and serialized fields; it adds no native hook or simulation pass.
Four roster/finisher cases now earn the actual stage13/counter3 loader, preserve
both living actors, campaign flags and lives, release both controls, and replay
the complete future exactly twice. The genuinely earned no-saber branch also
passes its real Doppler door/introduction, defeat and final-lab arrival with
story1C and refightsFF; the saber branch retains its earnedDC.

State qualification4d22ad670d757747d887aa493c23e6a4d3818679433bd051e7e80fdcf1a30c63
passes the affected Mosquitus roles/story, stock restrictions, alternate Lab2
main, Bit destruction and an eight-refight sequence. GUI
f59413e924ab6a67bbd8ecc33f5276fc8f4a61bd34e11ff198a9613d2ea7aeac builds;
all four standard tests pass. Earlier route positioning remains an explicit
bypass. Doppler completion does not establish Sigma phases, authored fatality
handoffs, terminal flow or the remaining route checks. Acceptance stays114/180,
66 open and3/15 accepted areas; G12 and G15-G16 remain open.


## Final-lab vertical door admission

The final lab's native item6 contact was absent from co-op door admission.
The same exact contact advanced the original X door but ignored physical P2
Zero. Item6 now uses the existing entrant/required-pair contact policy at its
DD07/DD1E/DD1F boundaries, before native D0E8 forced movement. One additional
native function entry is routed; the existing serialized contact continuation
also validates that entry. No state field or second world update is added.

A distant camera/column shortcut failed in original solo too and lost a life;
it was not a valid Sigma admission fixture. Position/velocity cheats now follow
the actual shaft camera nodes and stop changing positions when the real door
activates. Native camera/frontiers, admission, forced movement and Sigma's
introduction then execute normally. Original X reaches main5E at native32HP
in1513 updates; P2 Zero admits both living actors in1653. Lives2, shared
campaign and both controls survive, and each complete future replays twice.
This does not yet qualify all door policies, authored death handoffs or Sigma
combat/transformations/endings.

The actual-entry state binary is
7a15163a7ce8b61eebcf9fbdddaa06020c1d0b7d7cd4e61a91309a910562c902.
The updated GUI builds and four standard tests pass. Acceptance remains114/180,
66 open and3/15 areas; G12 and G15-G16 remain open.


## Native fatal-attack producer and Sigma transformations

A successful second player's fatal damage scan restored the NPC's first target
before returning to its caller. Kaiser Sigma then froze that nearer actor and
waited on its controller, even when the finisher already owned the world. The
native damage result now retains its actual living producer through the caller
and later script freeze. Existing captive bindings retain their required release.
Boss updates keep a balanced target scope even when the first target is already
the anchor. Classification uses the original class initialization records and
Kaiser's custom constructor; no hook, snapshot field or extra world step is added.

Both real Kaiser wins now reach native virus60 at64HP in2169 updates, retaining
the living pair, controls, campaign and lives. X earns the actual charged03 hit
at223 updates; Zero earns charged01 at1075. Both reach native charge12, and both
complete futures replay exactly twice. Upper-head aiming follows the actual
head rectangle above overlapping arm rectangles. Normal-shot attempts did not
deplete Kaiser; no weapon damage or victory result is supplied by the tests.

The first Sigma form also transforms into native5F at32HP after real X/Zero
winning hits, with both living actors and exact future replay. The actually
earned no-saber1C branch passes its introduction and first transformation too.
The second form reuses its authored kind byte for AI, so its observer follows
the unique active main instead of requiring a constant constructor kind.

Kaiser qualification uses
ec7b7d7dbca5d9cf40ce71b6ad0625e7f0a4687ebdb57278ae11a4a110cbaefa.
Five Mosquitus roles, eight captured Bit-parent cleanup permutations, both real
Doppler clears and an eight-refight sequence pass with the updated runtime.
GUI36565ff5c5dd25d096bda9fd045fc7ea2d7651da581c9e7f62bb2c8681c7dc72
builds and all four standard tests pass. Virus escape, actual terminal flow,
remaining routes and authored fatal handoffs remain open. Acceptance stays
114/180,66 open and3/15 accepted areas; G12 and G15-G16 are not yet accepted.


## Native virus escape and final-scene ownership

The final helper64 now retains03:C142/D24B's actual recorded-control actor.
Previously3F:ED33 borrowed the nearer free partner and waited forever for that
idle body to reach66/8. The existing script attachment, controller parking,
modal input and balanced world-owner machinery carry the actual recipient
through the helper's retirement. No native hook or snapshot field is added.

The virus's moving native hard camera locks also update both living private
views. Independent camera-region retention keeps03:DDF3's original bounds;
its per-view parser already retains both native regions. Previously those
scratch bounds inherited a widened presentation operand from before the
native-width ending. Restoring the same machine after an ending changed two
interpreter progress-counter updates despite identical eventual gameplay.
The complete futures now match without excluding any state bytes.

Position/velocity cheats follow the actual pursuit scroll to its1536 limit,
then stop. Native gravity lands the actors and real Right input crosses the
2032 final trigger. Earlier airborne and unstreamed shortcuts failed original
solo too and are not accepted routes. X-led and Zero-led saber runs reach the
actual ending6/0/progress5 at5704/5703 terminal updates. The actually earned
no-saber1C branch reaches it at5805; its real charged03 Kaiser hit uses native
charge6, rather than the saber branch's12. Both players, lives2 and earned
campaign flags survive. All three complete futures replay exactly twice.

State9d147216ad6149d5c5966cb696a299c87db9c9d455eb36f9829cd772640c8542
passes those branches, the four-roster native ladder/region-retirement reference
and the actual P2 final-lab door/introduction. GUI
18d686f5b2f19ded0bffaf16784774ae37b5909fd96afdea2663cb7ea28eee76
builds and all four standard tests pass. Temporary interpreter tracing is removed;
the shared engine is unchanged. Final-phase fatal handoffs, credits/new-game
flow, remaining routes and presentation remain open. Acceptance stays114/180,
66 open and3/15 areas; G12 and G15-G16 are not yet accepted.


## Earned credits and native Reset/new-game flow

G16 is accepted. The earned saber and no-saber ending fixtures execute their
actual epilogue and authored scrolling credits, including122 native text entries,
then reach the original6/2/4/4/8 final hold. Saber co-op and genuine original
solo both take21915 updates; the no-saber branch takes21921. Campaign flags and
lives remain unchanged, and every complete credits future replays exactly twice.
Native Start/B leave the final screen in its intended hold. Game-over continue
remains the separately qualified T08 path; the ending has no invented automatic
continue or next-stage transition.

The real Reset command exposed a production defect: X3 had no hardware-reset
callback. Cleared hardware resumed the old coroutine cursor, waiting forever
for its discarded NMI latch. The retained failing machine stayed at0/0 after
2400 updates. Reset now clears the title's CPU/bridge clocks and cursor, sprite
history, native ROM window operands and co-op machine state. The selected profile
survives; discarded scene input is not projected into cleared WRAM. Native
persistent data follows the existing preserving Reset path.

Actual Reset and physical Start reach the fresh intro at1339 updates. Original
solo reaches ordinary control2091 updates later; co-op enrolls both living actors
at2092, with either requested physical-seat roster. The native opening formation
precedes enrollment and must not be mistaken for missing players. Both rosters,
original solo and the earned no-saber final hold pass complete restart replay
twice, with fresh campaign resources and no restored old party.

Credits state8902815772b6a2ac95aad6e55d978e28fff30d68aa608fbfdba657ff8662f5f1
and no-saber7356181310825190073f9e08340b71bdf3151f4d495b3ddde6d763da58489225
share the unchanged credits execution path. Reset qualification uses
400c34dd0037f56a56d450c81f07d5e4a31bb8744b98639b1d178b71983f246a.
GUI54d1c9f76d299baad6dd270637614eb1e99e81d19d3c55b73a6baaf4ba54edd0
builds and all four standard tests pass. Acceptance is115/180,65 open and3/15
areas. Campaign G01-G11 and G13-G14/G16 pass; G12 and G15 remain open. This does
not accept final-phase fatal handoffs, moving presentation, runtime or owner QA.

## Final-phase fatal handoffs and presentation-independent activation

G15 is accepted. Actual first-Sigma or Kaiser contact kills the selected actor;
the living partner earns the native winning projectile and transformation.
Both physical-seat rosters and both survivors retain the fallen participant,
lives and shared campaign through the virus escape and native ending. The
earned no-saber branch also passes with either survivor. Zero's charged winning
hit retains his native controller; physical button holds supply the charge.
Neither death, damage, phase nor ending results are written by these checks.

Unified escape exposed a complete-replay mismatch: the prior scene's presentation
margin changed the first restored activation operands and interpreter progress
counter. The desktop co-op adapter already uses native simulation fields and
composes extra columns on the host. The title now applies that contract to all
co-op activation margins, replacing the narrower independent-region exception.
Original solo keeps its existing widescreen activation behavior.

State5af322ab69eea0077154f6e011707f0df13d7c1e34947b64344dd3e602f7d99e
passes actual Unified survivor escape, independent no-saber Zero escape,
genuine original solo escape and first-form fatal transformation; each complete
future matches exactly twice. The affected four-roster native upper-region,
scanout and retired-view checks pass. GUI
d013a3e29ed1445093196e09db3e59c318a49b8daf2ba1a7d6ff59d0b4440390
builds and all four standard tests pass. Earlier survivor permutations retain
their exact jobs and identities in the existing private qualification record.

Acceptance is116/180,64 open and3/15 accepted areas. Campaign G12 remains open;
the other15 campaign points pass. Simultaneous last-hit/death S12, remaining
door/transport cases, traversal, presentation, runtime and owner delivery gates
remain separate unresolved criteria.
