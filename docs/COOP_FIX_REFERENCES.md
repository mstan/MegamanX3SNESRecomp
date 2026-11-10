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
| X1 `f983118` | The platform wrapper had the same return-register reentry error; new landing tests require the original caller's inputs. | Adapted at native X3 BFAE/C07F with entry/result registers and scratch, per-seat rider histories and serialized continuations. Shared-support probes cover separate riders/save-load; the authored stage5 class26 case covers P01 and P04 narrowly, including P2-only admission. Other support families remain open. |
| X1 `744b476` | The co-op rider-bit projection covered native item classes 0E–10 but omitted 13/14. The patch extends that class predicate. | Reclassified from the graphics group: this is support coverage, not private CHR. X3's content classes must be derived independently and tested under P01–P16. No X1 class number is adopted. |
| X1 `6d1f58c` | The co-op platform predicate omitted the road and slab actors even though they call the same native rider helpers. It adds those enemy/item classes. | X3 registers both shared enemy and item/script pools at its own BFAE/C07F boundaries, without importing an X1 class allowlist. The authored class26 check is qualified narrowly; other native/custom support paths remain open under P01/P06. |
| X1 `8371f13` | A one-time falling-slab block directly modifies the world body's grounded state and rider latch before ordinary platform handling. Pairing only the later helper leaves the other actor out of that transition. | The X1 address-specific block is excluded. Its failure mode remains an open P06 requirement: identify and test X3's actual drop/one-time support transitions rather than assuming the generic solid helper covers them. |
| X1 `2cbe5e4` | The co-op model projected separate subtank contents and lacked a successful-collector owner for native item continuations. The correction keeps subtanks in shared world RAM, retries only native pickup contact, and retains the winning collector through later item updates. | X3 keeps its native subtank bytes shared and latches the actual refiller across normal/frozen item updates and snapshots. Authored class02 health refill and native Zero overflow banking are qualified narrowly. The broader pickup contact-retry policy, other item classes and menu spending remain open under M06/M07/M08/M09. X1 slot addresses and its two-player weapon inventory are excluded. |
| X1 `6ce61dd` | Existing co-op scheduling let the other body move during the collector's refill action while native terrain collision was globally skipped. The correction parks both actors while the original item task continues. | Adapted to X3's actual1F2B collision pause: the non-collector's controller pass parks; the collector's native animation and item timer continue. An authored health item plus movement/jump input reproduces the unsafe movement before the repair and passes afterward. Withdrawal/fallen lifecycle probes also retain the item until its native cleanup releases the freeze. Full scene/menu integration remains open. |
