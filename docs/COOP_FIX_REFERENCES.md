# Co-op repair references

These public code-history references identify failure modes to check in X3.
They are not a cherry-pick queue or evidence that X3 already passes. Review
the actual code and parent behavior before classifying a change as a co-op
regression; omit superseded experiments from the adoption set.

| Reference | Commits | Failure modes |
| --- | --- | --- |
| X1 | `67a44c8`, `1014232`, `41a678b`, `f426fa7` | Survivor control, fatal-contact boss ownership, team retry and P2 pit death. |
| X1 | `01dbb86`, `98744b3`, `197fa8d`, `7f83bfd`, `832b1bb` | Stage roster/resources, single-run scripted sequences and camera-bound handoffs. |
| X1 | `732f4eb`, `836f7d0`, `a07e0e3`, `6faeb10`, `bdc843a` | World boundaries, shared-camera rounding, catch-up, doors and native pans. |
| X1 | `eff6da1`, `f983118`, `3089a80`, `f24c7a8`, `72a6cef`, `63a2c1e`, `d2d5a4f`, `3c42da1`, `6d1f58c`, `8371f13` | Balanced contact continuations, independent rider latches, same-frame carry, lifts and falling supports. |
| X1 | `4717a1a`, `e3887fd`, `54deba3`, `088678d`, `98942f1`, `e703b7b`, `7c7082c` | Solid contacts, shared supports, wind, captures and either-actor targeting. |
| X1 | `2cbe5e4`, `6ce61dd`, `5e47b5c`, `b23514a`, `5ac74b3`, `e35486d`, `6cc4c40` | Pickup attribution, refill freeze, capsule ownership and stranded partners. |
| X1 | `884b40b`, `f7972a9`, `bde027d`, `8e88f96`, `71843b5`, `7f49250` | Private effect graphics, palettes, priority, hidden OAM and vehicles. |
| X1 | `4c45567`, `277febb` | Retry spawn eligibility and generated spawn-hook coverage. |
| X2 | `79c0137` | Corpse palette fading the surviving world. |
| X2 | `f95fd07`, `a5669dd`, `f4e2a67`, `65c2d9a` | Private HUD planes, scene-camera attribution and compiled boundaries. |
| X2 | `ee40cd9`, `73c3d35`, `72d7a04`, `a0a0c71`, `8e6e086`, `48eb3eb` | Original spawn frontiers, retry bounds, live pools, clocks, support and pinch. |
| X2 | `4c8530b`, `b7457bf`, `ac059f7`, `da24fe8` | Armor formation, low roofs, attack origins and charge-audio attribution. |
| X4 | `7ac01d6`, `0248ab0`, `4a4e511`, `86eda19`, `3366733` | Survivor exits, warps, fortress/refight progression and room completion. |
| X4 | `bcf62c6`, `f054a9e`, `8bf31bb`, `0932537`, `dbdd5b8`, `09c6b32`, `996ed0e` | Scene ownership, paired-door traversal, crush avoidance and vehicle cameras. |
| X4 | `8336b0d`, `1042c53`, `2dee3c0` | Platforms, later routes and enemy waves for either leader. |
| X4 | `98d2510`, `318f408`, `8439270`, `3ac1021` | View-local flashes, weather, lighting and attachments. |
| X4 | `3091716`, `8ee1557`, `7513962`, `384ffe5` | Pause recovery/exits, stored-charge HUD ownership and arrival HUDs. |
| X4 | `9122a77`, `011acea` | Capture/attachment release and native combat recovery. |

## Code-reviewed dispositions

These decisions compare the actual source changes with their parents; they do
not infer the defect from a commit subject or transfer X1 addresses into X3.

| Reference | Parent behavior and correction | X3 disposition and checks |
| --- | --- | --- |
| X1 `eff6da1` | The co-op contact wrapper reentered the partner's test with the first return's registers. The change saves the entry A/X/Y/P/DB and restores them for reentry. | Adapted in X3's native CB31/CB74 paired contacts: entry registers and scratch are restored, while balanced terminal matching preserves the chosen result. ROM-backed contacts cover E01/E02/E03/E11 narrowly; solid authored objects remain open. X1's host-only continuation invalidation is superseded by X3 snapshot serialization/preflight. |
| X1 `f983118` | The platform wrapper had the same return-register reentry error; new landing tests require the original caller's inputs. | Entry/result separation is retained in X3's terrain and owned-pool passes. This is preparation for P01–P16, not proof that native platforms work for P2. The actual platform contact/rider contracts still need mapping and authored tests. |
| X1 `744b476` | The co-op rider-bit projection covered native item classes 0E–10 but omitted 13/14. The patch extends that class predicate. | Reclassified from the graphics group: this is support coverage, not private CHR. X3's content classes must be derived independently and tested under P01–P16. No X1 class number is adopted. |
