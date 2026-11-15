# Mega Man X3 co-op â€” committed stopping point and full continuation

## Current continuation - 2026-11-15

**171/180 accepted;9 open;8/15 areas complete.** Remaining criteria:
D06; A13; P06,P12; E06; G12; V07,V12; Z04.
Native creator graphics, attachment/armor ordering, entry/reversal/lock
frontiers and independent streamed room pictures are now qualified. The latest
source identities and individual evidence are at the end of
`docs/COOP_DEVELOPMENT.md` and in the existing ignored qualification record.
Original menus, controller ownership/reconnect, audio and moving presentation
checks remain accepted. Preserve their completed evidence.

Current-room palette animation and native script priority are now repaired and
qualified. Both actual lighting arenas, animated normal/alternate entries and
the Lab2 priority3/color-math scene match their original PPU references. The
final22-phase visual regression and four standard tests pass. V07 still needs
weather/view-local scope, and V12 still needs a valid complete-scene reference.
The failed teleport-door source is frozen outside the suite; its native tile
base was uninitialized, so do not patch the private art to reproduce it.

Z01 and Z02 production package verification are accepted. TRACEoff/NETon build and all4
standard tests pass; three fresh extractions and three current-build resumes
run with package/Windows-only PATH, correct native co-op input isolation and
actual342/684-pixel adaptive gameplay. Explicit saved widescreen-off remains
256px; updates preserve each profile's settings/input/mods/snapshots exactly.
Tool-only9234735 fixes the release helper's configured-engine path. Internal
0.2.0-coop-qa ZIP remains outside delivery. The production launcher provider API verifies actual exposed readable controls,
all seven option/choice callbacks and independent commit/reload persistence,
including widescreen off/on, without UI automation. Provider receipt child0
on958cdd7d; actual defaults/gameplay/input evidence above is retained.
See ignored native-package-qa/results.json and six process receipts. Preserve
accepted physical reviews; do not use desktop/browser UI automation.

A new actual V07 weather defect is established: co-op shows dense repeated
glyphs instead of native snowfall. A control with co-op disabled before the
original stage2 loader reaches the same real outdoor terrain/camera and renders
proper snowfall. Source f28e06d8, original control718d0b21; native actual package
GUI reproduces it. Original-raster/detached/VRAM attribution is in progress.
Fix this actual BG3 finding before V07 acceptance; do not discard it as an
unqualified teleport source. Tests are uncommitted; no production fix yet.

The full180 objective and solo/local-only constraints remain. No release is
ready or delivered. Complete the remaining authored gameplay, weather/scene
and final packaging criteria; retain the frozen crush/fortress calibration
limits instead of restarting unsuccessful setup loops. Historical checkpoints
below retain their original source identities and counts; this section and the
latest development evidence supersede their next-action directions.

## Native cartridge pause continuation - 2026-11-14

The per-character cartridge pause replacement is validated locally. Shared
output shows the requesting character; split output shows each original screen,
including a read-only native screen for a fallen character. Original navigation,
weapon ownership, shared tank costs/locks, Exit eligibility and one paused world
remain. There is no separate host menu layout or confirmation modal.

Final state-test SHA256:
3693255e4d51b5fde2020add5fd51ffb567659572252dc2424cfdf77a00ce78c.
Final GUI SHA256:
e41b6bd6ebaaee35adcb5ed46f69b69466b5d84432f5a8b32e8a6b2bc4fef591.
Full snapshots are554210 bytes with X3M2 controls and X3U1 native pictures.
Active older menu snapshots are rejected; inactive X3M1 fixtures remain usable.
Current-build replay/rewind/rollback and native persistent saves remain required.

Both rosters/either requester match complete original weapon and actual admitted
page8 equipment images, also with gold armor and an existing selected weapon.
Each56-tick native heal, simultaneous tank ownership, concurrent separate tanks,
interrupted/full-machine restore, malformed transaction, resume/weapon commit
and native Exit passes. Door/fatal-contact menu overlap and the actual survivor
checks pass. Ignored high address bits wrap to64KiB VRAM; cached healing cannot
contradict tank ownership. All4 standard tests pass. The two actual delayed-input
peer cases match all15 compared RuntimeSave cuts with no reported desync.
Do not repeat accepted native-menu gates without a relevant change or failure.

The native owner review in build-coop/qa-state/owner-review has3 small setups:
controller ownership, reconnect with X fallen, and original per-character pause
screens. Shared/Split choices are readable and remembered. Its manifest points
to the current frozen GUI; the automated setup verification is child0. Prior
observations are preserved, and new observations include their binary and view.
The old owner game and selector were observed closed before replacement.
Local implementation commit9f1df51 is validated. The updated native selector
and game were launched in a normal window with normal audio, paused on the
controller-ownership setup. Read session.json and saved observations for their
actual running/review state before touching any process. The physical and visual
results remain pending; do not infer acceptance from the setup verification.
Never interrupt an active owner playtest or query its TCP while the owner tests.

The menu correction adds no acceptance point. The later Q01 native-solo check
below and subsequent Q02-Q05 bring the actual total to135/180,45open,4/15 areas.
Physical controller and owner presentation checks still require observations.
Continue a decision-ready remaining criterion while those observations are
pending. Keep the G12 navigation calibration frozen, and do not retain the
neutral resource-index candidate or restart that local tuning loop. The known
separated-view runtime gap is resolved by the later Q03 qualification below.
Keep delivery unchanged while acceptance remains open. After the entire
applicable objective is resolved and validated, automatically deliver the verified
X3 ZIP through the established completion steps below. Remote push and central
sync remain outside this local continuation.

Q01 is accepted on GUI e41b6bd6 and unchanged game11fd733 rebuilt on the same
engine614694ab/UIcf6315ad. Fresh disabled-co-op cold boots use normal mod
settings, no initialized co-op state or prototype override. All6 complete WRAM
and image cuts match exactly: native control arrival, walking, jumping, charged
fire, original Start pause, and resumed movement. P2 cannot control solo or open
its menu, the paused world stops, co-op enrollment/menu ownership stays inactive,
and both builds replay their complete guest futures. Only the documented host
DSP output ring is excluded from replay comparison; guest DSP state remains.
Receipts native-stock-disabled-v4-current and baseline are child0. Evidence is
in build-coop/qa-state/runtime-qualification/stock-disabled-v4. The later Q02/Q03
checks cover runtime; an independent simulation's shared presentation is not Unified.

Q02 is accepted: actual Unified independent=0 and disabled co-op run the same
native stage1 world/starting X pose on GUIe41b6bd6,120warmup frames,8 simultaneous
jump/fire cycles and120tail frames. Each has616 measured samples. Native mean
3.392/p954.340ms; Unified mean4.431/p955.554/max6.331ms with0samples>16.7ms.
Unified first frame.120s and peak working set285.9MiB; both actors remain living,
full-health and unlocked. Scope is SDL dummy/software with muted emulated audio;
no desktop GPU or separated-view claim. native-unified-runtime-measure is child0;
raw CSV/logs/snapshots/results are in runtime-qualification/matched-unified-v1.
The later Q03 qualification below resolves the separated-view raster gap.
Q04-Q06 remain open.

Q03 is accepted. The actual portable tile-row renderer resolves each8pixel
fragment's tilemap/ROM indirection once, retaining per-pixel resource/palette
seams and the original large-tile/mosaic path. Same-binary reference/candidate
split mean16.136->9.153ms, p9518.733->10.978ms, raster10.809->3.902ms. Selected
P1 mean10.241->6.663ms, p9512.521->8.033ms; shared is neutral. Actual default
GUI f462ba1da5cca7634e629dca585ca10227c818fa11fb65213e9a995e80888bfd then passes
split p9510.869ms and P1 p957.584ms. All18 captured presented images, full guest
futures and WRAM match; simulation advances once in every presentation. Every
background pixel/color additionally matches the original implementation through
the moving stage1/stage2 workloads and actual native fixture regression,
including342/448px adaptive panes, restore and transactional negative checks.
All4standard tests pass. State binary cb1155910d682f3c6227b1812e397ca5aad9df126f158944d6c182e961077d95.
The25 remaining test-only attachment locations after X3U1 now use the existing
wire locator; actual unknown-bit rejection/unchanged-machine checks pass.
Evidence: runtime-qualification/separated-rows-*-v1-results.json and
initial-rows-verify-v1-results.json; native-tile-rows-* receipts all child0.
Scope is local Windows SDL dummy/software; Q05's NET-enabled budget remains
open. Keep the live owner review on its existing build untouched. Local commit
32d7138 integrates the renderer and test corrections;00a9152 records Q01.

Q04 is accepted on the same frozen GUI, with no further production change.
Three real216frame charge/combo waves in stage2 separated regions reach each
native personal buster limit of3, a13/40 shared-effect peak and Zero's actual5C
saber. Both actors retain16/28HP, naturally recover and then dismount/jump and
move18px left. All36 complete guest cuts match across shared, selected and split;
only the documented host audio ring is excluded. Each mode has1045 timing
samples; split mean9.516/p9511.088ms, selected p958.424ms, shared p956.185ms.
Peak working sets428.1-432.3MiB remain bounded. Raw timing includes intermediate
snapshot I/O and its isolated stalls. Actual full allocator recovery stays A13.
native-separated-pressure-q04-v2 is child0; evidence is in
runtime-qualification/separated-pressure-rows-default-v2 and its comparison JSON.
The later Q05 check below resolves the NET-enabled budget. Q06 remains open.
Do not repeat accepted timing without a relevant change.

Q05 is accepted on NET GUI66a84831eef80106e0c6971e3e0488e5ec3b07bce778b948206ee4070cf54776.
Two actual peers use the same fixed profile0/transcript0c17fb26,25ms datagram
delay and a300ms asymmetric hold during native charged attacks. The original
strict actor/effect/replay checks pass; both remain alive, no desync, actual7/8
replayed ticks and confirmed frontiers4135/4130 beyond required4056. Both exit
normally and retain complete timing CSVs. QA-only normal exit and public host
frame numbers enable the measurement; no game or shared-engine behavior changes.
Inclusive interval timing uses consecutive presentation timestamps minus only
explicitly measured host wait, including admission/snapshot/replay work outside
component timers. All intervals p958.251/8.197ms; guest-bearing intervals
p958.381/8.233ms. A single45.8/47.0ms recovery interval per peer follows the
deliberate300ms hold. Native charged-game frames remain below16.7ms. Held-frame
presentations and raw stalls remain in the evidence; no other interval exclusion.
native-net-runtime-combat-q05 and standard are child0 (all4tests). Evidence:
net-lan/runtime-combat-q05-66a84831/{runtime-results.json,observations.json,
host/frames.csv,guest/frames.csv,host/process.log,guest/process.log}.
Q06 is the remaining runtime criterion. Keep the existing owner review untouched.

Q06 is accepted after actual Ubuntu/WSL Linux x86_64 compilation and runtime.
Game62b4156, engine614694ab and UIcf6315ad regenerate through the supported
strict-idempotent native pipeline and build NET-enabled Release with GCC13.3,
SDL3.4.14 and one compile job. GUI SHA256 is
6cf22f7098d0546ac788a6d7ab95e6b6a210a515424c7f68b2ba91472fbb7051;
state-test SHA256 is
be91a7610234b97b3b2c2026851bf8f122f250c092ff55cc72256145edb1ef47.
All4standard tests, real-ROM replay/rewind, two actual native GUI runs, the
complete co-op fixture with reference pixel verification/full-machine futures,
and the independently earned survivor fixture pass. Shared changes retain
portable C paths without new assembly or CPU-specific dispatch. Scope is this
representative Linux target and existing Windows evidence, not Deck/ARM testing.
The broad teleport fixture requires fresh admission frontiers: a settled
independent fixture fails the identical item setup in both Windows reference
and Linux. A native Unified GUI fixture supplies that prerequisite without
editing success flags. Independent Linux gameplay/survivor evidence remains.
Receipts native-linux-portability-compile and native-linux-portability-native-fixture
are child0; phase logs/identities are in runtime-qualification/linux-portability.
The initial postprocessor's special slot12 filename mistake is corrected without
repeating the passing independent GUI run. Optional SDK pytest was not claimed.
Current total136/180,44open,5/15areas. A05 P1 takeover is prepared/running but
unaccepted. Keep the existing owner review and frozen G12 calibration untouched.

A05 is now accepted. Both actual P2-first stage1 route worlds retain their native
objects, eligibility and history. P2 uses public living withdrawal/return beside
the waiting P1, then P1 actually traverses the two authored class08/01 enemy
regions at736/976. X reaches1103 in414 updates; Zero reaches1102 in331. Each
world advances exactly once per frame, both enemies stay single instances, the
partner remains stationary, and both actors retain full16/28HP. Each complete
current-build machine future repeats exactly twice. The endpoint is after both
regions and before the later pit; this does not certify that later traversal.
An arbitrary480frame overrun killed Zero at that pit and remains failed evidence.
No production change. Optional real-ROM test binary SHA256:
b4a1f960a423f606622e7a6a7447b2a34d6a5df61922aedf1427742a3d605eb2.
native-activation-handover-region is child0; all4standard tests pass. Evidence:
activation-handover-work/p1-handover-region.log and standard-region.log.
Current total137/180,43open,5/15areas. A11 permanent-item streaming revisit is
the next selected criterion; no result yet. Keep the owner review untouched.

A11 is now accepted. Both actual roster orders earn the original stage1 heart
and stage5 subtank through native eligibility/award logic before32 total
away/return visits with either player leading. Position/pose, camera entry/bounds
and contact protection bypass travel; no collection, capacity or tank-result
flags are supplied. Native streaming cannot recreate either collected item,
regrant capacity/reserves, change personal HP or alter earned campaign flags.
All4 entire-machine streaming futures repeat exactly. This does not certify
travel, native camera locks, retry, armor or presentation/audio. The four actual
earned current-build fixtures and logs remain in permanent-streaming-work.
State-test SHA256:
ab2680e7a0bc0827489185cb5b140c5c3ffab643d5388de8bcfb459c0b5932b9.
native-permanent-streaming is child0; all4standard tests pass. No production
change. Current total138/180,42open,5/15areas. A12's existing native team retry
path is selected for inspection next; no new A12 result. Preserve owner review.

A12 is now accepted. Current earned heart fixtures cross the actual stage5
constructor with X capacity18, then earn the native subtank and actually visit
the authored moving support. Approach protection is cleared before both native
stage5 spike deaths at22; either roster retries at599, lives2->1, full own
HP18/28, unchanged earned flags and shared reserves. The old support object and
FA84 eligibility clear; actual P2 controls re-admit/move it at13/11 updates.
Each native wipe/retry repeats the entire machine. Later cold region visits
still cannot regrant the tank or capacity and repeat their full future.
Initial retained-protection setup had no deaths and remains unqualified; the
passing job reuses its actual hashed earned/visited inputs. No production change.
State-test SHA256:
758d492cb6fd8940addf988da7688bd92c938a68acaaa6e980bcdbf32f1c3be8.
native-reward-retry-v2 is child0; all4standard tests pass. Earned, visited and
retried snapshots plus both logs remain in reward-retry-work. Current total
139/180,41open,5/15areas. Native one-shot event revisit A10 is selected for
inspection next; no result yet. Keep owner review untouched and G12 frozen.

A10 is now accepted. Actual body/arm armor and chip capsules in stages5/8/1
(masks04/02/10) complete their original single-X and matched co-op openings,
awards and recorded demonstrations in both rosters. Chip armor/normal-chip
prerequisites are supplied; its result high10 is earned. Across48 actual
away/entry/contact visits with either view owner, consumed class4D capsules
remain absent (all counts0), flags/resources stay unchanged and dialogue,
demonstration and scene ownership never restart. Six full-machine futures repeat.
Manual position/pose, camera entry/bounds and contact protection bypass travel.
Capsule test teleports now let the normal camera boundary establish cold entry
instead of retaining a settled fixture's old directional frontier. No production
change. Initial chip attempt omitted native prerequisites and remains invalid.
State-test SHA256:
9ee966e677bbf409b4999ce9880e34dafab4fc886f0cbb92c2e442fa5f130441.
native-one-shot-revisit-v3 is child0; all4standard tests pass. Exact logs and
six current earned fixtures remain in one-shot-revisit-work/stage*-v3*.
Total140/180,40open,5/15areas. A02's native left-route helper is prepared in
tests/x3_coop_left_test.inc, but not yet included, built or run. Use the actually
earned stage5-v3-p1/p2-complete fixtures for that check. Preserve owner review.

A02 is now accepted using the actually earned heart fixtures and native stage
entry to rebuild playable terrain (stage5/1 at357/392 updates). Actual P2 right
inputs first visit NPC736/class08/01; native culling clears FA16 and the old
object while P1 stays384. A pose-only stop preserves position and eligibility.
Actual left backtracking re-admits the enemy with9HP at217/186, ending221/190
frames atP2x889/camera761 with one world tick perframe. Both retain own18/28HP;
the two complete left futures repeat. Sixteen subsequent heart-region visits
retain the one-shot reward and native capacity/reserve/HP, with whole futures.
No progression, death or admission result flags are supplied. No production
change. State-test SHA256:
19718f42c105bbb17ee4350b43eee4fc0a9952362e513d2cddc9448d1604e05e.
native-left-heart is child0; all4standard tests pass. Evidence: left-frontier-work/
left-heart.log and standard-heart.log. The earlier direct teleports are invalid
setups. A forced stage request from the completed chip retains flags1F0F but
stalls in pending entry/native mode4; its full machine is retained separately.
Do not turn that unqualified caller into another navigation loop or claim a
product defect without an original-reference check. Keep it frozen for now.
Total141/180,39open,5/15areas; A09/A13 remain. E05's existing actual native
AI target-switch/attack case is selected next. Preserve the owner review.

Current local commits include0e77fc8 (P09) and375a459 (native story handoff).
The pending save-preflight correction and its actual menu roundtrip coverage
are validated locally. The original F: checkout, engine and UI remain unchanged.

The concrete Doppler defect is fixed: a pending stage12 entry now follows the
fresh supported native stage1 constructor from its original normal caller,
then uses the existing enrollment gate. Four roster/prior-owner cases complete
the actual occupied kill, child retirement, one reward, native story and arrival
at7865/7985 updates. Both characters have positive HP16/28, visible native art
and opposite24-update controls; lives2 and full futures repeat exactly twice.
The explicit original reference completes7833; the old6000 bound was too short.
Broad S06/S13 remain open.

A second real defect was reproduced at a native reward/menu snapshot: modes0/2
reuse1FAE as presentation data while the pending scene retains its prior stage.
Preflight now permits that legitimate non-gameplay alias; active gameplay
still requires the exact supported stage and existing malformed-state checks.
All4 roster/prior-owner full futures save/load multiple actual menu boundaries
and match the entire machine. The retained refused snapshot now restores through
the GUI to a real stage1 arrival and independently controlled living actors.
All12 existing stage/lifecycle/negative transaction cases, the current GUI and
four standard tests pass. No codec/version or state migration was added.

Validated final state-test SHA256:
225b1e702507ec4f200b7547b4c24748e2a246c89ede8ee140524bf084010f0b.
Current production GUI SHA256:
4ad749d5478af0e76259cc93c3cbf947903e02902f7e12c089b0d5a015327519.
Receipts native-reward-final-qualified, native-reward-final-ctest,
native-reward-roundtrip-rest, native-reward-stage-preflight-regression,
native-reward-menu-restore-gui and native-reward-validation-build-gates are child0.

E08 now has positive native occupied-parent kills and actual victim/child cleanup
for Buffalo grip/freeze, Crawfish, Bit ring, Press claw, God arm/carrier,
Hornet attachments and Gravity Beetle's carrier, alongside the intro references.
The final Gravity family has4 legal free-X/Zero-captive cases, both rosters and
prior owners: exact winning NPC return held1/class1C, hit15, child retirement4
with tag0 before any stage pool reconstruction, then the complete reward and
controlled arrival above. The existing family records retain their exact jobs
and individual counterexamples. Free Zero's original attacks reflect against
this occupied guard; no special weapons are imported to Zero.

Additional charged-X precast experiments failed to earn an occupied winning
hit. They are unqualified setups, neither passes nor an impossibility claim.
Stop expanding those experiments: positive authored-family kills establish E08.
The optional strict self-cast regression remains; failed raw diagnostics are
retained privately. Do not weaken it or restart a positioning calibration loop.

E10 now passes actual same-child-slot reuse in all8 roster/victim/prior-owner
cases. After the first real ring escapes/retires, live Bit allocates a fresh
ring into the same1318 slot at1361/1363 updates. Its old tag/latch are0 before
contact. The opposite character earns the new lock; only that character and
the parent receive the new tag/binding. Actual physical escape retires the
second ring without rebinding the old victim, losing either actor or ending
the32HP encounter. Every complete native future replays exactly twice. This
adds to the existing retired-captor slot reset/intro references. Production is
unchanged; all4 standard tests pass. Frozen state SHA256 is
5c4d3bee51ea718de4d77e77b7f5a97bedb319ee17e32377ecf65b221219c6b6.
Receipts native-bit-ring-reuse-first/rest/ctest are child0.

Native identity reconciliation is complete (native-identity-check child0),
with canonical seed hashes preserved and actual boss art inspected. Legacy
fixture selectors remain for reproducibility, but actual mapping is:
stage3/59/toxic=Gravity Beetle;4/57/rhino=Toxic Seahorse;
5/58/catfish=Volt Catfish;7/55/tiger=Tunnel Rhino;
8/56/beetle=Neon Tiger;1/53/hornet=Blast Hornet.

G03 can use the existing actual Seahorse arena/projectiles, native stage hazard
and clear evidence under old rhino. G04 is reopened for actual Rhino stage
hazards; its real boss drill/wall-rush/clear evidence is under old tiger. G07
is reopened for actual Neon wall interactions; its projectile/field/clear
evidence is under old beetle. G08 is reopened for actual Gravity projectiles;
its capture/arena/clear evidence is under old toxic. These are unverified gaps,
not newly observed runtime failures. No relabeling earns a point. The honest
count is124/180 after reopening those3.

Current Gravity result: actual22/kind2 orb exposed a real second-recipient
omission (native8/20HP, paired8/28). The existing persistent-contact rule now
includes that exact kind. Normal original/paired full body, personal pools,
palette and one source update pass all4 rosters/first owners.8 actual source
fatalities retain24 survivor controls and complete real clear/reward/arrival
2553+64 twice exactly;4 living-pair clears retire kind2 children before reward
and return2433+64 twice exactly.8 kind0 carrier native regressions pass.
Current GUI894e5d6edb78d17f2f3d28d4cebe4f4bc9c804365d98dd689addb2627b917623
and4standard tests pass. Frozen normal state5d23ff70 records final geometry.

G08 is now accepted with both actual kind2 orb and low-health kind4 field.
Kind4 independently exposed the same second-actor omission; adding that exact
kind resolves it. All4 original/paired complete body/pool/palette/source cases,
8 actual field fatalities/survivor clears and4 living-pair clears pass. Every
complete future repeats exactly, including actual child retirement before
reward/pool reconstruction, one reward and stage1 arrival. Normal regression,
current GUI and4standard tests pass. State16f3561dd3d00c43919621d93bf52fd06fb56e24b14a8c5f9603d60c8216f998;
GUI591c5fe3d8bd06279910a59b8b5c331085c3d758188585dbcb7e1e5df17477cb.
Receipts native-gravity-field-fixed-first/rest/survivors/living,
native-gravity-normal-regression and native-gravity-final-build-gates are child0.

G07 is now accepted with the correct actual Neon56/stage8 identity. All4
roster/physical-seat wall continuations pass: native32HP boss reaches both
walls, jumpsminY1349,611video/worldticks/actions0551, then the entire route
replays exactly. Actual native buster defeat, one1FC4 reward and stage1 return
2553+64 retain both living actors/lives2 and replay twice. Existing actualNeon
projectile/field/clear evidence under legacybeetle remains valid. StateSHA256
0d559877d32700b9eaa33f220908ef224463646d5154e013cf9247888268172c;
native-neon-wall-first/rest/ctest are child0. All jobs are terminal/collected.

G04 is accepted with actual stage7 Rhino hazards, retaining the actual boss55
native drill/wall-rush/clear proof under legacytiger. Native checkpoint0 level
construction and original event-column allocation prepare actual NPC35/kind1,
3C:E889/FA02 at640/1840; prior traversal/cold activation is explicitly bypassed.
All4 character/physical-seat references match the complete body/pools/palette
and real source on each of96updates. Native damage1 is X16->12/Zero28->24;
source moves to639/1922/state2/10, worldticks96, both living. The whole paired
future repeats exactly. No attack/contact/movement/success result is supplied.
This does not accept P06/D06/A02 or fortress traversal. StateSHA256
62a4a3029d94c61f72425aff6d9aaf576a9a5415a17e2e248be7a6ac80e39a64;
native-rhino-actual-stage-start/hazard-first/hazard-rest/hazard-ctest are child0.
Current production GUI remains591c5fe3; all jobs terminal/collected.

G12 partial evidence is frozen while the next bounded decision is evaluated.
All8 lower05/upper3F, roster and physical-leader checkpoint0 first-door cases
pass complete-machine replay twice on0c6f4e81; native release1394 explains the
old1200 timeout. Refactored prefix43252c46 exports the identical whole future.
The first remainder case earns a full32HP authored51 defeat825, retirement122,
postscript settlement1658, actual trailing-player Right235 and the second
FAE8 door288; both players arrive3099/3104 alive/unlocked. It then fails a
later assisted shaft step3720/384. That corridor, optional dead end, later
doors and other fortress stages are NOT qualified. No production fix or G12
acceptance follows from these partial results. Stop expanding/tuning this
route harness until a concrete required gameplay decision justifies it.

S11 is now accepted after actual Crawfish54/Buffalo52 entry coverage for both
rosters and either physical entrant, each complete native intro future twice.
All8cases retain fresh32HP parents, both actors inside/alive/unlocked, lives2
and campaign flags. Native releases434/452; stateSHA25698a853e2cb5d117930cbb5883752cfed2bd4ff7e39effe986fbf37ff99136b82.
All4standard tests pass. This combines the existing qualified main-boss,
hunter/fortress, Vile, Mosquitus, refight, Doppler and Sigma admission families
and the new8first-fortress intro cases. Broader scene criteria remain open.

The bounded D06 God43/kind0 setup is rejected: original X earns actual grounded
Dash14 and position-only approach, but8native updates give0actualC05B branches
and retain16HP. D06 remains open; stop calibrating that pose without a newly
demonstrated trigger. No production change. All owned jobs are terminal.

S04 is also accepted: current98a853e2 native-door4roster/entrant cases match
one original admitted update, preserve native forced movement/entering camera
through intro and complete cleanup, restore independent controls and replay
whole futures exactly. All8new arena exports plus the qualified Sigma
vertical/horizontal arena retain both native private-camera commitments.
Receipt native-s04-door-qualified is child0; current4standard tests pass.
Visual seams and moving clips remain V12/V14 work. All owned jobs terminal.

S05 is accepted on6a419c27: both actual bodies independently qualify for
the authored gate from the same pre-contact state, then simultaneous contact
earns exactly one native01:E0F1 success. All4roster/physical-winner cases
complete770native updates through forced movement, intro and cleanup with
both alive/health retained and independent controls restored. Whole-machine
replay matches and never repeats admission. Native item bytes match original;
no admission/result is supplied. native-s05-door-qualified and ctest are child0,
all4standard tests pass. Production GUI remains591c5fe3. Actual130/180,50open.

Next: evaluate another decision-ready remaining criterion using existing
qualified paths. Do not resume the fortress navigation/calibration loop merely
because G12 remains open. Prepared native owner QA can cover outstanding
physical/controller and presentation judgments while independent checks proceed.

Historical G12 continuation plan (prepared content geometry is private q;
this is no longer the immediate critical path):

Next: G12 actual fortress traversal/sublevels/alternate exits. Reuse existing
native route/camera streaming and earned branch fixtures; the accepted Sigma
shaft native-route example advances positions32px with12 real updates, then
uses the real door. Existing end-room teleports/column preparation only prove
admissions, not the skipped fortress path. Derive route coverage from the real
native stage/column/layout data and preserve all earned hunter/Vile branches.
Do not create a general navigation framework or repeat accepted boss mechanics.

D06 still needs an actual authored lethal crush. F10 physical controller
reconnect, G12 fortress routes/exits, presentation/runtime checks and owner QA
remain. Prior D13/T06,S12,T07,D08,D12,E07/E09 and P09 evidence is preserved in
the existing development/private records; do not repeat unchanged suites.

Remaining IDs: D06;F10;A02,A05,A09-A13;P06,P12,P13;
E05,E06,E12-E14;C06,C11;S06-S07,S09-S10,S13-S14;G12;V01-V14;
Q01-Q06;Z01-Z04.

Use repository tools and headless game/TCP interfaces. Computer Use requires
explicit owner approval. GUI --resume-state consumes its input: always copy a
disposable test-owned resume file and verify the canonical seed hash afterward.
RuntimeLoad re-enables enrolled co-op, so env0 alone is not an original reference.
Native references explicitly disable co-op after the actual earned release.

Keep retained q evidence/build outputs until their decisions are resolved;
remove superseded task-owned binaries with audited non-forced cleanup later.
No delivery folder has been touched. Complete all applicable criteria and owner
observations, then deliver the verified one-current-build Windows ZIP to
G:/Nextcloud/Games/SNES automatically. No remote push/PR/merge/Beads sync.

Updated 2026-11-12. This is a handoff, not completion of the co-op goal.

## Read first and resume the existing work

1. Read the current global/repository AGENTS instructions.
2. Read this entire handoff, [GOAL-X3-COOP.md](GOAL-X3-COOP.md),
   [docs/COOP_DEVELOPMENT.md](docs/COOP_DEVELOPMENT.md), and
   [docs/COOP_FIX_REFERENCES.md](docs/COOP_FIX_REFERENCES.md).
3. Inspect the existing worktree and private evidence before changing anything.
   Resume from the first unresolved decision below. Do not restart the suite.

The complete objective, feature contract, all180 acceptance points, constraints,
owner validation and final delivery contract are in GOAL-X3-COOP.md. Its original
prototype-checkpoint narrative is historical; this handoff and the current
qualification records supersede that narrative, without reducing its scope.

- Worktree: G:/Temp/Codex/mmx3-coop; branch feat/x3-coop.
- Code/tests/qualification checkpoint: **995201c0056f098704e733c600d833862470844b**.
- The subsequent commit containing this handoff is documentation only; inspect
  git log -2 for both identities. All requested source work is committed.
- Engine: G:/Temp/Codex/mmx3-coop-engine,
  614694abde7ff0449c2b86e87c2ae4a81b693788.
- UI: G:/Temp/Codex/mmx3-coop-ui, cf6315ad; unchanged in this continuation.
- Original F:/Projects/snesrecomp/MegamanX3SNESRecomp checkout is untouched.
- Private owning issue: beads-8wg.19.5 under beads-8wg.19, central database
  F:/Software/beads/issues. Update that existing issue locally; do not duplicate it.
- **Solo/local only. No agents, push, PR, merge, remote release or Beads sync.**
  Local commits/private local issue updates are allowed. The current session
  stopped at the owner's explicit request; the next /goal activates continuation.
- The old goal tool reports usageLimited, not complete. Do not mark the old
  goal or issue complete, or create a duplicate goal in the same active thread.

## Honest acceptance state

**110/180 accepted;70 open/partial. Three of15 areas accepted: R, M and N.**
G01-G09 are accepted. G10 is still open. No new acceptance point was claimed in
this checkpoint. Fixtures, builds, permutations and partial branches do not
increase the count. Do not repeat unchanged accepted suites.

Remaining IDs (full definitions are in GOAL-X3-COOP.md):

| Area | Open IDs | Count |
| --- | --- | --- |
| Death | D06,D08,D12,D13 |4|
| Team wipe | T06,T07 |2|
| Fallen persistence | F10 |1|
| Activation | A02,A05,A09-A13 |7|
| Terrain/supports | P06,P09,P12,P13 |4|
| Enemies/captures | E05-E10,E12-E14 |9|
| Combat | C06,C11 |2|
| Scenes | S04-S07,S09-S14 |10|
| Campaign | G10-G16 |7|
| Presentation | V01-V14 |14|
| Built runtime/portability | Q01-Q06 |6|
| Owner/package/delivery | Z01-Z04 |4|

Existing accepted boss evidence and implementations must be preserved: intro,
Buffalo, Toxic Seahorse, Rhino, Catfish, Crawfish, Tiger, Beetle and Hornet.
The compact private record retains each boss, exact jobs/hashes, counterexamples
and complete-future comparisons. Do not turn this into another inventory or
harness project; implement/measure the next concrete unresolved behavior.

## What this checkpoint implements and measures

The committed runtime changes use existing serialized NPC attachment bytes and
hostile-owner tags. **No snapshot-format change or additional native hook.**

1. The actual Bit/Byte defeated controller now retains D1CA's recipient until
   native D1EF. Before the fix, an actual Byte destruction left nonleading Zero
   locked128 after2400 updates despite the earned0C result.
2. During hunter defeat, the native script owner keeps updating; the other
   living body uses the existing balanced controller parking epilogue. Before
   this repair, Bit destruction released both locks but free Zero fell toY1671
   belowfloor1472 and became FALLEN. Four roster/anchor destruction cases now
   retain both living actors and actual movement into the next room.
3. A real Bit kind0/class2A ring retains the actor actually caught and borrows
   that actor's physical input edges. Eight roster/victim/anchor references
   match original body, personal pools, palette, ring and parent:64 held updates
   without escape edges, release90 from real B edges, full retirement91. Busy
   withdrawal is refused and every complete paired future replays exactly.
   Live rings do not park the free partner.
4. A fallen actor retains its last captured pose; that previously kept the ring,
   global1CD91 latch and parent binding alive after actual captive death. A
   retired owner's ring now takes its own39:AC5A release continuation (substate10).
   The measured Zero-death case dies on native contact at55 updates, clears
   all three, leaves Bit alive32HP, lets X move for24 native ticks and repeats
   the entire future exactly twice. Other fatal permutations are not yet run.

All four combined campaign results05/0D/07/0F and visits22 are actually earned
by sequential native Bit then Byte encounters. No hunter admission, visit or
outcome flags are supplied. Each Byte outcome has four roster/owner-anchor
cases, both living/no life debit/acquisitions retained, actual next-room
movement, and complete-future replay twice. Native Bit retreat, destruction and
Byte's standalone retreat/destruction also have their detailed records.

The native first-fortress data now supplies real lower/upper entry fixtures.
Earned05 reaches a live5D fusion boss with43 parts. Earned0F reaches a live5A
parent and four5A parts. These are preparation/entry observations, not G10/G12
acceptance: **P1 remains outside the arena, native mechanics/clears and complete
later consequences remain unqualified.**

## Exact stopping-point validation and failure

All jobs from this session have finished and their execution sessions were
collected. **No healthy build/test remains running.** All four test CWDs are idle.

- Current state test SHA256:
  47e109e3c1afed475edf5469de09d4c7e216da85ca92a8ad6538543bf13576db
- Current GUI SHA256:
  ce13455ffc45136f44c7592d055cf1f98f3d95bd21af3b5f86c5150cd0b77fdf
- native-hunter-checkpoint-gates: child0; current production GUI builds and
  all four standard CTests pass.
- native-bit-ring-qualified: child0; eight full native living-ring references
  on1c23e45f34f195f88e8c3ee4b50c2a04d61eee1a6b2f96f7d0559a8098b9a8a6.
- native-bit-ring-fatal-fixed: child0; the actual captured Zero death/cleanup
  case onfd2af7e9f7bb251f760d8d4e01369203013f31fe780a28695bf510d5681b6d23.
- **native-bit-ring-survivor-checkpoint: child1.** Its two captured-death and
  cleanup replays pass, then the full survivor-clear attempt fails: X emits
  class00 busters for1200 updates but never earns the final hit. X remains
  alive16HP at2985/1455, Bit1HP at3039/1447, state2/0/2, invulnerability0;
  fallen Zero remains at2960/1447 with its stored capture pose. No false success
  or forced outcome is recorded. The test supplies facing only once before
  the target moves; investigate that aiming prerequisite before blaming AI.

The last failed machine is retained at:
G:/Temp/Codex/mmx3-coop/build-coop/qa-state/weapon-state-work/native-hunter-fire-unqualified.sav.
The same CWD also retains native-bit-ring-survivor.sav (actual earned survivor),
native-bit-earned-ring.sav and prior failure snapshots. Preserve them until the
specific decisions are resolved. Never overwrite a running executable or a
fixture being consumed by another job.

## First unresolved decision: finish real hunter survivor progression

1. Read native-bit-ring-survivor-checkpoint-result.log and its job/receipt.
   Read native_hunter_outcome and native_bit_ring in
   tests/x3_coop_campaign_test.inc and the actual retained machine.
2. Inspect the live buster positions/velocities and facing against Bit. The
   initial facing can become stale when he crosses behind X. Correct real
   controller aiming/muzzle prerequisites if demonstrated. Do not write hit,
   victory, admission or result flags, revive Zero, or relax the living/fallen
   roster checks. Compare native solo behavior if the original fixture still
   cannot produce the hit. One failed aiming test is not proof of a runtime bug.
3. Reuse native-bit-ring-survivor-checkpoint-job.json with a newly frozen exact
   executable/hash after changes. Its env uses X3_COOP_BIT_RING=1,
   X3_COOP_BIT_RING_FATAL=1, X3_COOP_HUNTER_SURVIVOR_CLEAR=1 and the actual
   native-bit-paired-columns-arena.sav foundation. The same helper now retains
   the originally fallen participant through native hit/outcome/actual room exit.
4. Qualify both actual survivors and both rosters, then the remaining captured
   death/owner-anchor cases and actual parent-kill ring cleanup. Reuse the
   existing matrix job; no new harness. A first-case pass is not all-permutation
   acceptance. Keep default living-pair tests strict about both actors living.

## Second unresolved decision: finish actual first-fortress consequences

Derive content from the actual loaded native F800 column table. Each column
points to five-byte WRAM entries (flag,Y,ROM pointer); ROM records are seven
bytes in bank3C (type,Y,class,kind,X). **NPC+0C is a WRAM event flag pointer,
not a ROM record address.** Native00:DED3 is the existing column allocator.

- Lower record3C:F5AE: class45, kind0,5808/1920, WRAMflagFC7D.
- Upper record3C:F5B6: class5A, kind0,5856/1600, flagFC82.
- Native45 entry07:8AA3 tests Bit destroyed bit02 and Byte destroyed mask0C,
  allocates45 kind2/4 for surviving parts, and its actual fusion produces5D
  with43 parts. Do not mislabel class5A as Bit/Byte or13:CB87 as Tiger.
- Stage10 CPP2 gives5504/1967X,1959Zero, camera5376/1792: lower approach.
- The upper approach uses actual5504/1711X,1703Zero, camera5376/1536. The
  teleport skips upper pre-room admission, so the existing test calls original
  DB06/00DED3 for columns5344..<5664, Y1504,height288. Boss5856 is outside
  those columns; actual P2 input subsequently admits it. This is a preparation
  bypass, never a claim that the skipped route or cold activation passed.
- native-fortress-hunters-retreat-branch passes its narrow entry observation
  after1694 updates: camera5632/1792, actors5504/1967 and5659/1959, boss5D
  slot0DD8 HP32.
- native-fortress-hunters-destroy-columns passes the upper observation after155:
  camera5632/1536, actors5504/1711 and5641/1703; parent5A slot0D18 and four
  parts. The current observer can return a32HP child: require kind0 explicitly
  before using that slot to damage/qualify the parent.

Next actions:

1. Diagnose why X stays at5504 while P2 enters. This is still unclassified
   between setup/scene timing and a real shared-door defect. Use existing native
   **01:E0AE/E0E0/E0F0/E0F1** callbacks plus scene_begin/scene_tick, not bank00.
   Item class7 dispatches via00:F4EB to01:E037; native_door already recognizes
   item7/10/11. Observe actual door ownership, scene completion and both bodies
   before assuming a missing class or adding hooks. Never teleport X past the
   result that the door check claims to verify.
2. Qualify lower/upper native mechanisms, valid hits on the parent/parts,
   real defeat and actual next-stage/sublevel consequences. Keep campaign
   unlock prerequisites distinct from earned hunter results.
3. Consume all four earned combined states05/0D/07/0F. Do not fabricate combined
   flags. Cover meaningful both-roster/leader/survivor cases. A stage load,
   initialized boss or table match is not the required G10 consequence pass.
4. Only then decide G10 acceptance. G12 still needs actual systematic fortress
   routes/sublevels/alternate exits; this boss-entry bypass does not satisfy it.

## Reusable native hunter facts and fixtures

- Bit NPC46 entry07:8F43, live07:9116, defeated07:92FF.
- Byte NPC47 entry3C:C444, live3C:C636, defeated3C:C86E.
- Stage2 authored Byte record3C:D64D/flagFAD4 and Bit3C:D654/flagFAD9;
  both2976/1440. Arena2816..3072, floor1472, camera2816/1280.
- CPP1 is already past the one-way room. Actual supported entry startsCPP0,
  prepares the skipped pre-room columns2592..<2912 with DB06/00DED3 at
  Y1248,height288, then uses actual P2Right80/neutral120/Right24 and periodic
  real B dialogue input. The hunter2976 is outside those prepared columns.
- Native eligibility02:E27B rejects an already-cleared current stage;
  02:E0FB counts weapon pairs. Bit count4 guarantees admission, Byte count7.
  FlagsDC at1FBC+slot*2 are authorized unlock prerequisites; leave current
  stage2 slot6/1FC8 unset. Do not set1FD8/1FD9. Constructor earns visit02/20/22.
- Native results: Bit retreat01/destruction03; Byte retreat04/destruction0C.
  NPC+3D is native branch1 retreat/0 destruction. Actual projectile class is1F2F.
- X weapon selection6 (native index3, acquired1FC0) earns Bit class1B destruction;
  selection16(index8, acquired1FCA) earns Byte class0E. Zero keeps native combat.
- Native Bit ring2A kind37=0, entry39:AA8F, real catch39:ABC3 via CB31,
  hold39:ABED, physical escape39:AC3C, release39:AC5A; parent pointer+38.
  Do not add it to persistent multi-body damage sources: it catches one actor.
- Native D1CA queues bodyA3=9E/6E=80. D1EF requires action1E before release.
  Do not infer that a post-defeat1E lock was necessarily a prior ring capture.

All paths below are under build-coop/qa-state/effect-owner-fixtures/:

- native-bit-paired-columns-arena.sav / native-byte-paired-columns-arena.sav.
- native-bit-retreat-earned.sav / native-bit-destroy-earned.sav.
- native-byte-after-bit-retreat-arena.sav / native-byte-after-bit-destroy-arena.sav.
- native-both-hunters-retreat-earned.sav (05).
- native-bit-retreat-byte-destroy-earned.sav (0D).
- native-bit-destroy-byte-retreat-earned.sav (07).
- native-both-hunters-destroy-earned.sav (0F).
- native-fortress-hunters-retreat-branch-arena.sav.
- native-fortress-hunters-destroy-branch-arena.sav (the corrected columns job
  writes this path; inspect the job env rather than inventing a new filename).

## How to build, run and inspect without disturbing the desktop

Let q=G:/Temp/Codex/mmx3-coop/build-coop/qa-state/.
The compact private record is q/native-crawfish-capture-qualification.json â†’
bit_byte. Its other properties retain all accepted boss/frontier evidence.
Raw job JSON, result logs and receipts are in the same existing q directory.
The old resume history is preserved at q/RESUME-before-995201c.md.

- Python: C:/Users/Matthew/.pyenv/pyenv-win/versions/3.12.5/python.exe.
- Git: C:/Program Files/Git/cmd/git.exe.
- CMake/CTest: C:/msys64/mingw64/bin/cmake.exe and ctest.exe; build -j1.
- q/native-saber-window-build-job.json builds current mmx_state_tests.
- q/native-retired-weapon-gui-build-job.json builds the actual GUI in build-coop-net.
- q/native-charged-qualification-ctest-job.json runs the four existing CTests.
- q/native-hunter-checkpoint-gates-job.json runs GUI then CTest serially.
- Reuse q/run-x3-measured-job.py with --os windows --host local,
  --owner beads-8wg.19.5, --cpu1 --gpu0, memory.9/disk.05 tests or
  memory1.5/disk.15 build. Declarations are advisory; inspect measured pressure.
  The actual CLI uses separate flag/value arguments, e.g. --cpu 1.
- One job per CWD: native-buffalo-mechanics-work, activation-route-work,
  weapon-state-work, hostile-source-work. Freeze a unique EXE into an idle CWD,
  hash it and update job argv[3] EXE /argv[4] SHA before running. Never overwrite
  a live EXE. Existing Python wrappers verify the frozen SHA before execution.
- Every test uses SDL_VIDEODRIVER=dummy, SDL_AUDIODRIVER=dummy,
  MMX_REPLAY_RECORD=0, SNESRECOMP_REWIND_INTERVAL=1. Copy the existing job env;
  do not print its full PATH. Do not open native SRAM/save pickers.
- A receipt's child exit_code is authoritative. PowerShell may exit1 for stderr
  despite child0. Yield/resume the same healthy process; never restart it on a
  tool timeout. No current job needs resuming at this handoff.
- Use the game's test/TCP interfaces, not desktop activation. Owner handoffs
  later require normal window/audio and the actual DualSense, with input
  isolation verified in the running build and observations read back.

Example existing measured runner invocation (PowerShell; adjust job/out paths
for a new run so old evidence remains identifiable):

~~~powershell
& 'C:/Users/Matthew/.pyenv/pyenv-win/versions/3.12.5/python.exe' 'G:/Temp/Codex/mmx3-coop/build-coop/qa-state/run-x3-measured-job.py' run --os windows --host local --owner beads-8wg.19.5 --cpu 1 --gpu 0 --memory .9 --disk .05 --job-file 'G:/Temp/Codex/mmx3-coop/build-coop/qa-state/native-bit-ring-survivor-checkpoint-job.json' --out 'G:/Temp/Codex/mmx3-coop/build-coop/qa-state/native-bit-ring-survivor-next-receipt.json'
~~~

Current snapshot fixtures are413057 bytes. WRAM begins171225; X3C5 actor chunk
has header8 and two1399-byte players (body272/aux480/shots576/palette64;
life1394,char1395). Locate the chunk magic instead of relying on its incidental
absolute position. NPC0D18..10D7 stride64; hostile1318..1517 stride64;
items1518..1817 stride48. Current shared-world extension is67 bytes.
Old save-state compatibility is not required. Current-build replay/rewind and
native saves remain required. Preserve useful fixtures and requalify only
when an actual implementation/format change warrants it.

## Completion and retained storage

Continue all70 remaining acceptance points after G10, including actual Vile
branches, all fortress/refights, Zero-sensitive saber consequences, final
phases/endings, remaining terrain/capture/scene cases, moving visual/audio
review, runtime/portability and the owner's real DualSense observations.
Keep each meaningful failure/deferred observation open. Do not count static
screenshots, initialized content or builds as mechanics/visual acceptance.

The established local release destination is G:/Nextcloud/Games/SNES.
The prior MegaManX3SNESRecomp-windows-x64-v0.1.1.zip has not been replaced.
Do not ship this partial checkpoint. After the entire applicable suite and
owner observations are resolved, deliver the verified current Windows ZIP
there automatically, then remove superseded X3 ZIPs only after verification.
Preserve saves/settings/unrelated games; one current X3 build. Fresh release
widescreen defaults to adaptive Fit to screen; retain saved owner choices.
Exclude ROMs/extracted ROM data/private QA state and prohibited loading mods.

Retain q and current build outputs because they contain exact pending machines,
accepted evidence, frozen binaries and prepared jobs. No task downloads or
owner-facing launches were created in this continuation. Clean only verified
agent-created disposable copies when their decision is resolved; use native
non-forced Windows cleanup, verify containment/reparse/nested-repo safety, and
never touch user saves/settings. The retained old resume copy is historical
context until the relevant evidence has been consolidated. Do not commit q,
ROMs or private raw evidence; public technical summaries belong in existing docs.

Do not mark complete at another checkpoint. Completion means all applicable
suite criteria, required corrections and owner observations, verified delivery,
and required cleanup have actually finished. If a necessary human prerequisite
remains, finish independent work and prepare a concrete small handoff while
keeping that acceptance open. The instruction to prepare this handoff did not
activate a new goal.

/goal Execute G:/Temp/Codex/mmx3-coop/RESUME-X3-COOP.md to completion.

## E05 native in-progress attack target changes

E05 is accepted with no production change. The actual stage1 class08/01 enemy
starts in its earned jump attack. Player positions/poses, contact protection and
initial private camera positions prepare the check; enemy attack state, child
constructors and results remain native. Both roster orders run240 fields twice.
The parent updates exactly240 times, target ownership follows the nearest living
actor and restores the world anchor each field. There are19/4 actual nearest-seat
changes, three native02/00 projectile births and45 continuing child steps per
run. Parent movement is at most4px and live child movement at most7px per field;
neither parent nor child advances twice or relocates to the changed target.
Both actors remain alive at their fixture's16/24HP. Complete machine futures
match exactly for each roster. All4standard tests pass.

State-test SHA256:
052d00e73a4ec08aea952b82057b5c2142ac5e41f791215070aba664ba568625.
Receipt native-target-switch-v4 is child0; raw evidence is
build-coop/qa-state/target-switch-work/{target-switch-v4.log,standard-v4.log}.
The initial observer ran before runtime target selection and was moved after
the native SEP instruction. An initial prepared camera lag also culled the
enemy before any switch; v4 supplies correct starting cameras. Those failed
setups are retained and do not establish production defects. Total142/180,
38open,5/15areas. E06 and E12-E14 remain; the owner review is untouched.

## E14 marked native minion through survivor handoff

E14 is accepted. Existing original-body/source comparisons and survivor/child
cleanup evidence for the authored boss hazards are supplemented by actual
Hornet marked homing flights through either participant's death. Both roster
orders earn the native marker and its same-parent kind2 flight, including the
original steering comparison. Only player positions, low victim HP and survivor
protection prepare the fatal contact; the live parent's body contact earns the
death. No mark, child parent, target binding or death result is supplied.

Each240-field future retains the living8HP parent and fallen actor while the
survivor moves33px. The homing child executes53 native updates when the free
actor dies,52 when the marked actor dies, keeping its actual parent and earned
marked target throughout. It then retires natively. The observer runs after the
actual target-selection instruction and verifies the projected native character,
including the retired target. Every field advances the world once, restores the
canonical survivor and updates the child at most once. All four cases repeat
complete machine futures exactly; all4standard tests pass. No production change.

State-test SHA256:
318fad172d9f7cb7903dc15934a51b61ad4e45eb3cc5e41d0899747e4d06075c.
Receipt native-marked-minion-survivor is child0, both case exits0. Evidence:
minion-survivor-work/{outcomes.json,marked-roster0.log,marked-roster1.log};
standard checks: readiness-work/standard-v6.log (four passed). The separate
readiness experiment is not accepted: its original pending-constructor body
checks passed, but the stronger setup lost Zero's live projectile before contact.
Its failed optional setup/source is retained outside the test suite under
readiness-work/unqualified-readiness-test.inc. Stop this calibration; E13 remains
open and no runtime defect is asserted. Total143/180,37open,5/15areas.
Keep the live owner review and frozen navigation/crush setups untouched.
