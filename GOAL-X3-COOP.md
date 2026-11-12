# Finish Mega Man X3 co-op implementation and the entire QA burndown

## Objective and completion contract

Execute the entire approved X3 co-op plan, continuing the existing implementation
through feature parity, post-implementation QA, repairs, owner validation where
needed, and verified local release delivery. Do not stop at another prototype,
implementation checkpoint, successful build, checklist, or ready-for-release
statement. Finish the actual behaviors and their acceptance criteria.

The owner expects roughly **150–200 substantive post-implementation touch
points**. The execution suite below contains **180** under the existing 15 broad
areas. The 15 headings are rollups, not the complete checklist. Implement the
feature set, then systematically execute this suite, fixing discovered bugs and
rerunning the affected checks. Use focused checks during implementation too.

The touch points describe meaningful second-actor failure modes. Assign their
concrete stages, objects, routes and fixtures from the actual X3 content tables
and native behavior. Do not pad the count with controller smoke checks, duplicate
mode permutations, screenshots of stage loads, or bookkeeping. One touch point
may require several real stage/type/mode executions. Keep those executions as
evidence beneath it rather than claiming each permutation as a new feature.

If a listed mechanism does not exist in X3, prove that from the actual content,
mark it not applicable rather than passed, and replace it with another relevant
authored failure mode. Preserve a reconciled suite of approximately 180 real
touch points within the owner's 150–200 expectation. Do not delete failed checks,
shrink scope, weaken acceptance or invent content to improve completion counts.

Completion requires the full feature contract, evidence-backed disposition of
the entire applicable suite, all required corrections validated, remaining human
checks actually resolved, and the verified current Windows ZIP delivered to
`G:/Nextcloud/Games/SNES`. No remote pushes or remote releases are authorized.

## Resume from the actual local checkpoint

Read the current global/repository AGENTS instructions and these files first:

- `G:/Temp/Codex/mmx3-coop/docs/COOP_DEVELOPMENT.md`
- `G:/Temp/Codex/mmx3-coop/docs/COOP_FIX_REFERENCES.md`
- This goal file, including the full QA suite below.

Owning private central issue: `beads-8wg.19.5`, under X3 epic `beads-8wg.19`.
Use the central database at `F:/Software/beads/issues`; do not initialize a
project database or create 180 separate tracking issues. Reuse the existing
issue and compact execution records. Keep meaningful findings, accepted/failing
checks, exact source/binary identities, evidence paths and the ready-to-test
queue there. Keep the issue open until the requested outcome is validated.

Existing game worktree: `G:/Temp/Codex/mmx3-coop`, branch `feat/x3-coop`.
Local commits:

- `cc05530` — native actor-context prototype and initial qualification gates.
- `410e462` — survivor scheduling/input repairs and native QA scenarios/tests.

The game branch started from X3 default branch `11fd733`. Its source repository
is `F:/Projects/snesrecomp/MegamanX3SNESRecomp`, not the similarly named
`MegamanX3Recomp` directory. Do not work in or reset the original checkout.

Shared worktrees used for the checkpoint:

- Engine: `G:/Temp/Codex/mmx3-coop-engine`, based on main `e9c20732`.
- UI: `G:/Temp/Codex/mmx3-coop-ui`, based on master `cf6315a`.
- Unchanged-title comparison: `G:/Temp/Codex/mmx3-coop-baseline`.

At activation, inspect status and fetch/freeze current remote default heads
(read-only network operations are allowed). Continue the existing local work;
integrate/rebase onto newer main/master in isolated worktrees when necessary.
Do not discard local commits, redo accepted checks without a relevant change,
or treat this goal as permission to edit other active sessions' worktrees.
Use worktrees for all further game, engine and UI changes. Leave commits local.

Current evidence is narrow: two native actors enroll in a stage, Zero moves
independently while X stays still, initialized actor save/load and replay pass,
and one natural-contact X-death scenario leaves Zero alive without world freeze
or fade. Zero's native palette transfers correctly in that handoff. This is
**one concrete regression resolved, not campaign certification**; the existing
record has 0/15 broad QA areas fully accepted. The experiment is enabled through
`SNESRECOMP_X3_COOP_PROTOTYPE=1`, not a finished launcher mod.

The death bug had two native boundaries: the actor death state could freeze the
shared pools inside its current controller pass, and the stage task at
`$00:9C60` could queue a team fade before the next actor entry. Preserve the
tested repairs while completing all other lifecycle cases. Early black captures
were startup transients; do not reopen an alleged renderer regression on that
evidence. The unchanged-title comparison reaches a rendered scene.

Reusable local evidence/tools:

- `tests/scenarios/coop_survivor.script`: real stage setup and natural-contact
  death; it does not write death/completion/survivor flags.
- `build-coop/qa/saves/save11.sav`: disposable initialized co-op fixture.
- `build-coop/qa/saves/savequick.sav`: disposable survivor fixture (host slot 12).
- `build-coop/qa-state/native-survivor-result.log`: ROM-backed check output.
- `x3_coop_test`, `tests/test_coop_hooks.py`, and `mmx_state_tests`.
- `X3_COOP_TEST_STATE` and `X3_COOP_SURVIVOR_STATE` select the optional absolute
  fixture paths for the ROM-backed test. Its rewind scenario assumes
  `SNESRECOMP_REWIND_INTERVAL=1`.

Treat fixtures as build-specific and regenerate them after incompatible changes.
No old save-state compatibility is required. Native progress, passwords/saves,
owner settings, observations and unrelated files remain protected.

## Required feature contract

### Native actors, lifecycle and campaign

- Simultaneous X and native X3 Zero, with either character assigned to either
  seat. Use X3's existing Zero; do not backport the X1/X2 replacement character.
- Full-campaign Zero is the default. Provide an explicit stock-restrictions
  option restoring original participation/loss rules. Verify the real native
  rules; stock restrictions must never leak into the default or create waits
  for a character who cannot participate.
- Zero retains native X3 combat. Giving Zero special weapons is out of scope.
  X keeps his native weapons/upgrades and their real acquisition requirements.
- A fallen actor returns only when the team is wiped and it is time for the
  native retry. No revival at checkpoints, section changes, boss clears, stage
  exits, voluntary rejoin, reconnect, pause or settings changes. The explicit
  stock option retains its documented native participation/loss exceptions.
- Either survivor can keep the world, camera, combat, scripts and campaign
  progressing. Team retries, game over, native saves and campaign rewards run
  once with the intended shared resource cost.
- Safe withdrawal/rejoin for living participants, retained personal resources,
  catch-up transport and applicable X1 co-op assists/learner options. Assists
  must not revive an already fallen actor or substitute for correctness fixes.
- Shared campaign unlocks and subtanks; personal health, charge, selection,
  attacks, input history and owned transient effects. World actors/scheduler,
  enemy AI, stage scripts, audio and Cx4 simulation advance once.

### Views, HUD, menus and online play

- Unified shared camera with safe separation/catch-up behavior.
- Local split display with two independently following views in one window.
- X1-style local independent selected-player view.
- Online independent screens, one per peer, plus online Unified.
- A single shared world with activation/retention for both living regions,
  per-view native limits and spawn/visit frontiers. Do not activate the empty
  rectangle between distant players or rerun the world to produce another view.
- Correct per-view backgrounds, parallax, streamed tiles, weather, lighting,
  palettes, OAM/CHR, priorities, Cx4 effects and native forced camera sequences.
- Compact/stacked and horizontal shared HUD choices; private views show their
  owning actor's appropriate meters/charge state. No invented Zero weapon bar.
- Paired pause panels with independent navigation, deterministic shared subtank
  spending, healing/unpause coordination and correct shared modal ownership.
- Either living player opens doors by default; optional both-living-present
  rule. Dead, unavailable or withdrawn actors cannot prevent progression.
- Existing launcher/netplay integration, rollback, session compatibility and
  diagnostics. Settle simulation-affecting options/profile before frame zero.
  Local seat/presentation choice cannot select different world simulation.
- Current-build save/load and rewind preserve the complete actor, scheduler,
  contact continuation, scene, activation, menu and camera state. Invalid input
  is rejected before mutating the machine.
- A usable recomp-ui co-op package/settings flow replaces the prototype-only
  environment switch. Fresh packaged widescreen defaults to adaptive Fit to
  screen; retain saved player choices when updating.

### Reference review and scope boundaries

Review X1 most deeply, X2's active co-op implementation rather than only master,
and X4's delivered co-op/QoL implementation rather than its stale local master.
Starting reference locations:

- X1: `F:/Projects/snesrecomp/MegamanXRecomp`, including current main and relevant
  camera/HUD/learner branches; preserve dirty source state.
- X2: `G:/Temp/Codex/mmx2-coop` and its owning issue `beads-8wg.18.4`; it remains
  active work and is not automatically a qualified dependency.
- X4: `G:/Temp/Codex/psxrecomp/x4-issue-review-20261103/source`, plus relevant
  newer delivered branches found through its repository worktree/history index.

Complete the existing fix-commit follow-up list by following actual subsystem
code history, including commits whose subjects omit co-op. Record adopted,
adapted, excluded and superseded fixes with the X3 failure mode and check ID.
Review parent behavior before saying a bug was introduced by co-op. Do not
blindly cherry-pick address-specific fixes or preserve obsolete experiments.
Keep private comments/messages, source identities and attachments out of public
project text. Public records may describe independently verified software
behavior and technical commit references.

Do not expand this task into unrelated weapon ports, Boss Rush additions,
three/four-player co-op or X4-specific story/secret-code content. Transfer the
applicable co-op behavior, not every elective mod in those games. Carry over
applicable door, menu, recovery, HUD and balance options after checking their
actual feature contracts; unrelated enhancements remain outside this goal.

## Execution discipline and QA evidence

Work solo under the owner's current subagent policy. Do not use task size as
delegation authorization. Honor any applicable narrow focus-audit authorization
without turning it into implementation delegation or an approval gate.

Prioritize actual implementation and qualification. Reuse existing debugger,
generation, build, replay, test and packaging paths. Small necessary additions
must resolve a concrete decision; do not build a general test framework,
inventory system, dashboard or receipt pipeline. After two unsuccessful repair
experiments, reassess the approach and make the next experiment decision-ready.

For each QA ID record status (not run / passed / failed / blocked / proven N/A),
the real trigger, expected and observed result, exact source/binary/settings,
stage/object/seat/view coverage, evidence location, fix commit and rerun result.
One or two compact records are enough. Keep code-prepared, built, tested and
accepted states distinct. Report actual accepted/failing touch-point counts and
the 15 rollups; never count a build, fixture, hook, checklist or test command as
a completed gameplay criterion.

Derive routes, sublevels, checkpoint variants, optional branches and event types
from actual content tables. Use cheats/teleports for positioning and prerequisites
without directly setting the result being tested. A bypass remains a bypass.
Neither a screenshot, registered hook, stage load nor native comparison alone
proves a mechanic. Verify collision, spawning, combat, music/SFX, deaths/retries,
bosses, stage ends and actual next-stage transitions independently as relevant.

Test failure-prone cases with both roster assignments and reversed leaders.
Cover Unified, local split, local selected independent and online independent
where the behavior changes. Use meaningful coverage of both online peers, not
an unbounded Cartesian matrix. Retain individual failures within bundles.

Use the game's TCP/debug surface for inspection, screenshots and input. Automated
QA stays muted and preserves desktop focus. Audio emulation stays active where
needed. Owner-facing handoffs use a normal window, normal audio and the actual
requested controller. Prepare a lightweight selector/checklist with reproducible
current-build states and local observations. Open Explorer selecting a review
clip when asking the owner to review video. Do not interrupt a live playtest.

Use the measured benchmark host pool and ownership records for substantial
builds/tests. Inspect real CPU/GPU/RAM/storage pressure, bound parallelism and I/O,
and preserve healthy unrelated work. If capacity is unavailable, prepare useful
independent implementation and ready-to-test jobs rather than stopping. Keep
matched performance comparisons on one compatible host/configuration. Record
neutral/inconclusive variation honestly; compilation is not runtime performance.

Search only narrow known repositories/paths, respect ignore files and foreground
timeouts; never crawl drives or all project roots. Use native Windows-safe file
operations. Track task-created storage and clean only verified disposable data,
without force/bypasses or touching owner files. Preserve ongoing useful evidence.

## The 180 substantive post-implementation QA touch points

All begin **not accepted** in this goal unless current-build evidence is explicitly
reused after confirming the implementation/settings still match. The prior X
contact-death result is a narrow reusable case under D01, not a pass for every
death hazard or roster.

### 1. Individual death and survivor presentation — 16

- D01: X dies from native enemy contact; Zero retains world control and progression.
- D02: Zero dies from native enemy contact; X retains world control and progression.
- D03: P1 dies in a real pit while P2 remains on safe terrain; no team retry starts.
- D04: P2 dies in a real pit while P1 remains on safe terrain; camera ownership survives.
- D05: Either actor dies on authored spikes while the other remains outside their collision.
- D06: Authored crush death affects the trapped actor without killing or teleporting the free partner.
- D07: Fatal boss/projectile contact transfers combat/target ownership to the survivor.
- D08: Fatal damage selected inside a running actor pass cannot freeze shared pools.
- D09: Fatal contact after actor updates cannot make the stage task queue a team fade.
- D10: A corpse's palette/brightness/death flashes cannot darken the surviving view/world.
- D11: Fatal damage during charging cancels that actor's attacks/effects without cancelling the survivor's.
- D12: Dying while carried/captured releases ownership without dragging, hiding or freezing the partner.
- D13: Dying during a door/warp admission cannot strand the survivor behind a permanent lock.
- D14: Independent-view pit/death tests use the actor's real room bounds, not the other camera's bottom.
- D15: Shared-camera catch-up does not turn a recoverable offscreen living actor into a corpse.
- D16: Current-build restore/replay during fatal contact preserves the same survivor and visible death outcome.

### 2. Team wipe, retry and game over — 10

- T01: Simultaneous fatal contacts trigger one team retry and one intended resource debit.
- T02: X dies first and Zero later; the retry becomes reachable exactly once.
- T03: Zero dies first and X later; the retry becomes reachable exactly once.
- T04: A last survivor's pit death reaches native retry without an infinite death loop.
- T05: A last survivor's boss death clears boss locks and retries at the correct checkpoint.
- T06: Team wipe during transport/door handling cannot leave a partially projected actor or stuck continuation.
- T07: Retry reconstructs both bodies/resources and clears stale shots, captures, supports and hurt protection.
- T08: Exhausted retries reach one coherent game-over/continue flow and preserve native progress.
- T09: Last active actor dying while the other is voluntarily absent cannot deadlock team recovery.
- T10: Full-campaign retry restores Zero; the explicit stock option follows native loss rules without blocking X.

### 3. Fallen persistence, withdrawal and assists — 10

- F01: Checkpoint activation by the survivor does not revive the fallen actor.
- F02: Crossing a section/room boundary does not revive the fallen actor.
- F03: Boss victory and reward flow do not revive the fallen actor.
- F04: Stage exit and next-stage loading preserve fallen status until team retry.
- F05: Withdraw/rejoin controls cannot be used to resurrect a fallen actor.
- F06: Simultaneous withdrawal requests cannot remove the last living participant.
- F07: Living withdrawal/rejoin preserves personal HP/selection and finds safe landing space.
- F08: Rejoining above hazards, moving supports or low ceilings fails safely or uses a genuinely safe landing.
- F09: Learner/catch-up options protect only their intended living participants and never clear fallen state.
- F10: Pause, state restore, reconnect and option changes do not accidentally re-enroll a fallen participant.

### 4. Activation, streaming and separated views — 14

- A01: P2 leading right activates the authored enemies/platforms that P1 has not visited.
- A02: P2 leading left/backtracking activates eligible native content without replaying consumed one-shots.
- A03: P2 leading upward admits vertical-room content and streaming before it becomes interactive.
- A04: P2 leading downward admits lower content without borrowing P1's death/camera boundary.
- A05: P1-leading behavior remains correct after the same regions were first visited by P2.
- A06: Distant views do not activate the empty rectangle between the two actual regions.
- A07: World objects visible to one living view are retained when they leave the other view.
- A08: An object shared by overlapping views is created and updated once.
- A09: Original column/row spawn frontiers remain correct on entry, reversal and camera lock changes.
- A10: One-shot enemies/events cannot be duplicated by alternating view ownership or revisiting a frontier.
- A11: Collected items/upgrades remain collected across independent visits and native streaming refreshes.
- A12: Team retry resets appropriate spawn eligibility without regranting persistent campaign rewards.
- A13: Object-pool exhaustion caused by two active regions recovers without wrong-owner slot reuse or missing progression.
- A14: A fallen/withdrawn view stops casting activation while the remaining actor's actual region continues.

### 5. Terrain, platforms, supports and vehicles — 16

- P01: Both actors on one moving support receive the same native displacement with one platform update.
- P02: Actors on separate supports retain independent rider histories and velocities.
- P03: Either actor landing on a moving support on its movement frame is carried without a one-frame fall-through.
- P04: Jumping/dashing off a support releases only that rider's latch and preserves native momentum.
- P05: Support reversal/end stops carry riders once without doubling displacement.
- P06: A falling/collapsing support is triggered by either actor and executes its native drop frame once.
- P07: One-way platforms distinguish landing, ascent through the underside and walking off for each actor.
- P08: Slopes preserve each actor's grounded height, charge state and attack origin during movement.
- P09: Low roofs and dash exits respect native X/Zero clearance without standing into terrain.
- P10: Ladders beside supports/ceilings retain correct attach, detach and room transition behavior for both actors.
- P11: Conveyors, ice, water currents or other authored forces apply independently without advancing their source twice.
- P12: Moving-wall/platform pinch and crush predicates evaluate the body actually trapped.
- P13: A support that moves between independent cameras stays solid and visible to its real rider.
- P14: Ride Armor boarding/exit and empty-vehicle ownership cannot select the wrong actor or duplicate a pilot.
- P15: Supported vehicle variants retain rider controls, damage, destruction and correct native pilot presentation.
- P16: Support/vehicle destruction, rider withdrawal and rider death clear only the relevant ownership/contact history.

### 6. Enemy targeting, contact and captures — 14

- E01: Native body contact damages either actor under that actor's own invulnerability state.
- E02: Player projectiles from either actor reach native contact tests without borrowing another pool's state.
- E03: A consumed enemy projectile cannot hit both actors through an unintended retry.
- E04: Enemy AI picks a valid living target when P2 is nearest or the only survivor.
- E05: Nearest-target changes do not advance enemy AI twice or teleport an in-progress attack to another victim.
- E06: Shared ground shocks/environmental stuns affect each eligible actor once with independent recovery.
- E07: Authored grabs/captures bind to the actor actually caught, including P2-leading approaches.
- E08: Killing a captor releases its actual victim and clears attachments/child ownership.
- E09: A captured victim's death or withdrawal cannot leave the other actor frozen or bound.
- E10: A captor/child-slot reuse cannot attach its old victim to an unrelated new object.
- E11: Contact retry preserves balanced guest registers, stack and the caller's successful result.
- E12: Immunity, reflection and shields do not become extra damage opportunities when checking the other actor.
- E13: Newly spawned offscreen objects obey original readiness rules before either actor can interact with them.
- E14: Boss-owned minions, hazards and attachments keep correct parent/target identity across survivor handoff.

### 7. Attacks, damage and owned effects — 12

- C01: Simultaneous native buster fire preserves per-seat firing commands and native shot capacity.
- C02: Holding/releasing charge in one seat cannot arm, cancel or flash the other actor's charge.
- C03: Zero's native charge/combo/saber progression retains original timing and hit windows beside active X attacks.
- C04: X's acquired normal and charged weapons preserve native unlock, energy and damage rules during co-op.
- C05: Concurrent attacks preserve boss invulnerability/hit cooldown semantics without double-counted damage.
- C06: Shielded, armored and multi-part targets receive attacks on the correct native part/response path.
- C07: Weapon/projectile pool pressure cannot steal another actor's live shot, effect or hit history.
- C08: Native muzzle offsets and ground/air/slope attack origins follow their actual actor and facing.
- C09: Effects whose lifetime outlasts hurt or movement retain correct ownership; cancelled effects retire at the real exit.
- C10: Death, withdrawal, stage change and team retry remove stale damage sources and invulnerability remnants.
- C11: Charge loops, buster shots, saber swings and hit cues play with native cadence without duplicate world audio updates.
- C12: Survivor handoff retains the surviving actor's real weapon/charge state and removes dead-actor damage opportunities.

### 8. Doors, scenes, progression triggers and rewards — 14

- S01: Default either-player door admission works when P2 reaches the door first.
- S02: Both-player door admission waits for living present actors and moves them through without crushing.
- S03: A dead, absent or stock-unavailable actor cannot become an impossible door prerequisite.
- S04: Split/independent doors retain native movement and camera commitment until the admitted actors actually clear.
- S05: Simultaneous contact with a scene trigger runs its native sequence once.
- S06: Scripted actors, dialogue and camera pans stay with their actual triggering/required character.
- S07: A scene owner dying/withdrawing transfers or safely completes the scene without losing shared progression.
- S08: Capsule upgrades belong to the correct character even when the other actor reaches the capsule first.
- S09: Upgrade demonstrations/forced control park and release participants without stranding or killing a partner.
- S10: Warps and section transfers finish for the living team without half-completed transports or stale source-room actors.
- S11: Boss intros/arena closure admit the living team and release controls without a phantom missing player.
- S12: The last boss hit and simultaneous actor death resolve victory/retry ownership once and coherently.
- S13: Rewards, acquisitions, shared campaign flags and save/continue actions are committed once under simultaneous input.
- S14: Boss exits/refight pads and native next-stage/overworld transitions work with either survivor, not just the original anchor.

### 9. Authored campaign and branch coverage — 16

- G01: Intro/rescue sequences coexist with the live roster without duplicating playable/NPC Zero or losing X after rescue.
- G02: Blizzard Buffalo's authored arena/mechanics, boss clear and actual return transition tolerate either leader/survivor.
- G03: Toxic Seahorse's authored stage/arena interactions and clear progression tolerate either actor and independent views.
- G04: Tunnel Rhino's authored stage hazards and boss interactions do not retain a hard-coded original player target.
- G05: Volt Catfish's authored arena/electrical mechanics affect the correct actor and complete the shared encounter.
- G06: Crush Crawfish's authored capture/vertical-room interactions release and progress with either survivor.
- G07: Neon Tiger's authored attacks/wall interactions and stage clear remain correct with both actor contexts active.
- G08: Gravity Beetle's authored gravity/projectile/arena behavior preserves per-actor effects and one boss progression.
- G09: Blast Hornet's authored targeting/attached hazards and clear flow bind to the proper participant.
- G10: Actual Bit/Byte encounter/outcome branches and their later consequences remain coherent for the shared campaign.
- G11: Actual Vile optional route, encounter outcome and later route consequences work with Zero participation policy.
- G12: Every authored fortress stage/sublevel/alternate route has a real traversal/exit check that does not strand a partner.
- G13: Zero-sensitive encounter and saber-transfer consequences do not silently delete the default full-campaign co-op partner.
- G14: Every authored refight admission, refight clear and return pad works with either living actor and records one clear.
- G15: Final encounter phases, transformations and final defeat survive actor death/handoff without stuck transition tasks.
- G16: Actual endings, credits and native continue/new-stage behavior reflect the shared campaign without duplicated or skipped terminal flow.

### 10. Paired menus, pickups and shared supplies — 12

- M01: Either actor can open its own panel during gameplay without projecting the wrong inventory into the world.
- M02: Both panels can navigate simultaneously while the world remains paused once.
- M03: One actor's selection changes cannot overwrite the other actor's selection/charge/ammo history.
- M04: Simultaneous pause/resume requests leave both input mappings and the world in one coherent state.
- M05: Healing blocks resume only for the intended operation while the partner can continue allowed navigation.
- M06: Simultaneous shared-subtank requests cannot duplicate energy, underflow reserves or heal a fallen actor.
- M07: Full-health/empty-tank cases close correctly without consuming another actor's resources or sticking in refill freeze.
- M08: Collector-owned health/energy drops refill the correct actor, with full-meter overflow following the selected contract.
- M09: Persistent heart/subtank/upgrade acquisition occurs once when both actors touch the item.
- M10: A pickup's native world freeze parks/releases the other actor without erasing movement/contact state.
- M11: Quit, shared modals, save/continue and controller navigation have one deterministic requesting owner.
- M12: Menus opened around doors, transport, death or a restored snapshot cannot leave a hidden owner or frozen partner.

### 11. Visual and audio presentation — 14

- V01: Both native character bodies retain correct CHR, palettes and original size during concurrent movement and attacks.
- V02: Dynamic CHR uploads for one actor/weapon do not replace the other's body, armor or effects.
- V03: Hidden/offscreen OAM entries never wrap into a second view or flash across a margin.
- V04: Sprite/BG priority, occlusion, pilot overlays and attachments follow the correct actor in each view.
- V05: Native Cx4 effects/objects render correctly in both active views without a second simulation update.
- V06: Per-view tile streaming, finite map edges and parallax remain correct while the actors occupy different rooms/columns.
- V07: Weather, lighting, color math and local flashes remain animated and correctly scoped to the affected view/actor.
- V08: Compact/stacked shared HUDs show correct personal health, charge and applicable weapon information during damage/refill.
- V09: Horizontal shared HUDs retain correct ownership and placement during view/aspect changes and survivor handoff.
- V10: Each independent view shows its owner's private meters, including arrival, pause and a partner's death.
- V11: Native aspect, widescreen and ultrawide resizing preserve real margin content, HUD anchors and actor visibility.
- V12: View transitions, door pans, transport and menu fades have no stale scene, seam, missing actor or copied partner background.
- V13: Native music/SFX, charge-loop ownership and simultaneous combat cues are preserved without doubling audio work.
- V14: Review moving clips of death, charging, attacks, scrolling and scene handoffs; static captures cannot hide flicker or temporal errors.

### 12. Snapshot, rewind and rollback state — 10

- R01: Complete initialized co-op immediate save/load preserves both bodies, owned pools, inputs and resources.
- R02: Snapshot/replay during the secondary controller pass preserves balanced guest continuation and elapsed time.
- R03: Snapshot/replay during post-enemy terrain/contact/support ownership preserves the same native result.
- R04: Snapshot/replay across fatal contact, survivor handoff and full wipe yields the same lifecycle and native stage phase.
- R05: Snapshot/replay during arrival, withdrawal/rejoin and active support/capture restores the same ownership.
- R06: Paired-menu/shared-healing snapshots retain both panel states and deterministic resource spending.
- R07: Independent camera bounds, spawn visitation/frontiers and scene ownership restore with the simulation.
- R08: Actual rewind restores an initialized co-op scene and resumes both actors without discarded-timeline state.
- R09: Malformed/truncated actor/pass/state chunks are refused transactionally, including current-build state invariants.
- R10: Visible output after restore/replay retains correct animation/CHR/effect history, not merely identical WRAM.

### 13. Netplay and session behavior — 12

- N01: Host/join in two real processes agrees on roster, full/stock restrictions, simulation options and exact compatibility identity.
- N02: LAN qualification on compatible targets reaches real co-op gameplay and shared progression.
- N03: Each peer's independent camera/HUD follows its assigned actor while both peers simulate the same world.
- N04: Unified online play retains the same shared camera/activation decisions on both peers.
- N05: Local selected-view changes cannot alter simulation hashes, targeting, activation or pit checks.
- N06: Delayed/late input and rollback preserve both actors' combat and complete owned state.
- N07: Online survivor death and team wipe preserve agreement, fades, retry count and recovery ownership.
- N08: Paired menus, shared healing and simultaneous pause/resume cannot desynchronize inventory or world pause.
- N09: Door/script/checkpoint/reward transitions remain deterministic when the initiating actor is remote.
- N10: Disconnect/error handling leaves useful bounded diagnostics and a coherent exit/recovery path without resurrecting a fallen actor.
- N11: Invalid/incompatible builds/settings are rejected before frame zero rather than silently selecting unlike behavior.
- N12: Actual CPU/backend capabilities select a fixed qualified profile or common portable fallback before play; do not infer compatibility from version strings.

### 14. Built runtime and stock regressions — 6

- Q01: Disabling co-op restores ordinary native solo progression, rendering, controls and menus without stale hooks/state.
- Q02: Matched Unified workloads establish usable frame times/startup/memory cost in the built executable.
- Q03: Matched local split/independent workloads qualify dual-view composition without duplicated simulation cost.
- Q04: Separated actor regions plus pool/effect pressure remain responsive and bounded without dropping necessary gameplay work.
- Q05: Netplay/rollback workloads preserve correctness and practical frame budget under representative latency.
- Q06: Changed shared code retains portable fallback and representative supported-target compilation/runtime behavior; isolate pre-existing baseline failures.

### 15. Packaging, owner validation and delivery — 4

- Z01: A freshly extracted package exposes usable co-op settings/input ownership and defaults to adaptive Fit to screen with real gameplay verified.
- Z02: Package contents/dependency closure and launch are verified; omit source ROMs, extracted ROM art/data, prohibited loading mods and developer/private state; retain saved owner choices on update.
- Z03: Prepared normal-window/normal-audio owner checks have observations read back and every failed/deferred material case resolved before acceptance.
- Z04: Verify the final local release ZIP, deliver it to `G:/Nextcloud/Games/SNES`, then remove superseded X3 archives only after replacement verification; preserve saves/settings/unrelated games.

## Delivery, reporting and stopping conditions

The current delivery archive is
`G:/Nextcloud/Games/SNES/MegaManX3SNESRecomp-windows-x64-v0.1.1.zip` at this
checkpoint. Inspect the folder again before delivery. Keep one current X3 ZIP,
verify its replacement before removing the old archive, and stage all temporary
builds/extractions/manifests outside the delivery folder. Preserve native saves,
settings and unrelated titles. Do not replace the current archive with the
unqualified environment-switch prototype.

No pushes, PR publication, merges or remote releases. Do not push the private
central tracker either while this no-push constraint applies. Keep durable local
commits and tracker updates. Automatically finish the local release/package
delivery when the whole burndown is resolved and validated; do not ask for a
second release request or stop at readiness.

If a genuinely necessary human prerequisite remains, finish all independent
work, prepare a concrete small validation handoff, identify exactly which IDs
remain unverified, and keep them open. Do not relabel a deferred human judgment
as passed. Do not request routine permission for already-authorized reversible
engineering, fixtures, fixes or build delivery.

Give concise progress updates at meaningful decisions: features actually made
usable, concrete bugs fixed, passed/failed/unrun touch-point counts, rollup area
counts, runtime findings and next decision. A remaining-count report is not a
stopping condition. Preserve the ready-to-test queue across capacity waits and
session handoffs. Do not stop simply because a turn/checkpoint has ended, a build
passed, the scope is large or another setup document was written.

Only mark the goal and owning issue complete after the feature contract and
applicable QA suite are satisfied, required owner observations are resolved,
final ZIP delivery is verified, and disposable task data is cleaned or explicitly
retained with a concrete reason/cleanup need. Report the final local commit(s),
actual acceptance counts, evidence/QA record and delivered archive path. Keep any
remaining gaps honest; an incomplete outcome must never be called complete.
Preserve this requested goal file and final deliverables/evidence during cleanup;
move them to durable project storage before retiring their containing worktree.

Preparing this file does not activate the goal. The owner's subsequent `/goal`
invocation directs execution of these instructions.
