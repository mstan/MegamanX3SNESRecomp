# Mega Man X3 co-op — committed stopping point and full continuation

## Current continuation - 2026-11-13

**130/180 accepted;50 open;4/15 areas (T,R,M,N).** E10 and E08 are accepted
after P09. Campaign15/16: G12 remains open. The full180 objective
and solo/local-only constraints remain. No release is ready or delivered.
Historical checkpoints below retain their original counts; this section and the
latest `docs/COOP_DEVELOPMENT.md` entries supersede their next-action directions.

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

This correction adds no acceptance point:130/180,50open,4/15 areas remain.
Physical controller and owner presentation checks still require observations.
Continue a decision-ready remaining criterion while those observations are
pending. Keep the G12 navigation calibration frozen, and do not retain the
neutral resource-index candidate or restart that local tuning loop. The known
separated-view runtime p95 remains an unresolved Q03 qualification gap.
Keep delivery unchanged while acceptance remains open. After the entire
applicable objective is resolved and validated, automatically deliver the verified
X3 ZIP through the established completion steps below. Remote push and central
sync remain outside this local continuation.

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
The compact private record is q/native-crawfish-capture-qualification.json →
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
