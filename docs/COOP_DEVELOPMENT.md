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
| Individual death | Not qualified | Either survivor retains play and progression, with no global fade, blanking or stale freeze. |
| Team wipe | Not qualified | Simultaneous/staggered deaths produce one native retry and the intended life cost. |
| Fallen persistence | Component checks only | No checkpoint, stage exit, voluntary rejoin or menu revives a fallen player before team wipe. |
| Separated spawning | Not implemented | Either leader activates native enemies, supports and one-shot events without duplicate visits or cross-gap activation. |
| Supports/hazards | Terrain pass prepared | Both riders retain support, momentum and crush behavior while each platform advances once. |
| Enemy interactions | Not implemented | Targeting, contact, capture and release recognize either actor and clean up victims. |
| Combat ownership | Not qualified | Concurrent attacks retain native damage/cooldowns and clear on lifecycle exits. |
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
  fixture also passed immediate restore and deterministic partner-input replay
  before the input-history correction. That fixture must be regenerated and
  requalified for the new context format; none of these checks certifies a
  campaign or all rollback boundaries.

The unchanged-title comparison reaches a rendered scene on the same engine.
The stage-1 development route enrolled both actors and produced captures of
native Zero moving right while X's position stayed fixed, followed by both
actors resting on the floor. This is a narrow movement/presentation observation,
not stage-wide collision or visual acceptance. The immediate gate is native
survivor handoff, death presentation and retry behavior. Then qualify world
contact and survivor lifecycle before expanding camera and menu presentation.
Do not deliver or
replace the existing X3 archive with this unqualified prototype.
