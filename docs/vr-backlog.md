# VR interaction backlog

These are local work items for the contribution branch. They are not public
GitHub issues. Existing upstream reports are linked to avoid duplicate reports.
User observations describe earlier local builds and are not claims that a
specific commit caused a failure.

## VR-001: Recover manual reload and support-hand state

Priority: high. Reload can stop working after weapon changes or incapacitation.
VR Gloves currently provide the usable hand-tracking path.

Implemented offline: interaction ownership is checked before the grip-release
wait. Weapon/player changes, incapacitation, spectator state and input-mode
changes clear held-hand state, stale pose regions and old ammo updates. Normal
reload completion retains its pending settlement. Gameplay recovery remains
to be verified in a headset.

Chamber history now retains an unfinished cycle across weapon switches when
empty auto reload is suppressed. Native handle serials distinguish replacement
entities; unavailable handle data falls back to the current draw only.

- Audit stale weapon, magazine, bolt and support-grip ownership on weapon swap,
  downing, revival, death and map change.
- Check a full and empty pistol, Magnum and SMG for ten consecutive reloads,
  including same-type weapon swaps and downing/revival. Grip release must cancel
  ownership without leaving the hand attached.
- Verify slide travel, release and chamber state against actual ammunition.

Related: [native hands #406](https://github.com/keyou91/l4d2vr/issues/406),
[intermittent reload #343](https://github.com/keyou91/l4d2vr/issues/343).

## VR-002: Make grip pickup and release work with native weapons

Priority: high. Right-grip release did not drop weapons in the earlier prototype.

October 4 headset feedback confirms grip release drops weapons in `cd6e3a9`.
Table pickup, drop placement, free-hand orientation, ammo placement and reload
timing produced additional failures. `51239cf` addresses those paths but still
needs headset verification. Later offline checks protect grip packets from
Object Pull conflicts and retain per-player reload ownership across them.

- Trace contact pickup and the client/server drop request through native weapon
  ownership. Distinguish close-range grip pickup from remote Grip Pull.
- Holding grip near a reachable weapon must pick it up. Releasing grip must
  create a visible, usable dropped weapon when this optional mode is enabled.
- Check two clients and prevent a released grip from dropping a subsequently
  selected weapon. Keep the existing control preset available.

## VR-003: Finish independent dual pistols

Priority: high. Earlier dual pistols followed only the right controller and
aimed both shots from that hand; the prototype also lost reload functionality.

Further offline fixes remove the dual-shot early return that skipped firing
haptics and predicted hit feedback. The chosen ray is retained through the
shared feedback path, and haptics select its firing hand. Live cached poses are
scoped to the player/weapon with a bounded age. Exact prediction replay timing,
native muzzle effects, animations and headset feedback remain to be verified.

- Complete independent hand tracking, shot aim, effects and haptics. Preserve
  the selected hand on retransmitted input commands.
- Define magazine and chamber handling for two pistols while respecting the
  native shared ammunition model. Capacity must remain correct below 15 rounds
  and with supported replacement models.
- Test alternating and simultaneous triggers, loaded/empty reloads, swapping
  between single and dual pistols, and recovery after revival.

The [30-round capacity fix #371](https://github.com/keyou91/l4d2vr/pull/371)
is already in the base and does not implement these independent interactions.

## VR-004: Finish optional manual pump cycling

Priority: medium. Shell insertion worked in an earlier build, while the shotgun
cycled automatically and the fore-end grab did not work reliably.

- Couple backward and forward fore-end travel to the native firing and chamber
  state. Prevent another shot until cycling completes in manual mode.
- Check partial travel, grip release halfway through, interrupted shell loading,
  empty ammunition, and weapon changes. Automatic shotguns must retain their
  automatic cycle.

Related: [shotgun empty reload #385](https://github.com/keyou91/l4d2vr/issues/385).

Further offline fixes allow a chambered shotgun to fire after a partial top-up,
protect pending shell settlement from firing or a second insertion, and move
the pump grab region with the visible fore-end. Unknown shell reserve data no
longer causes a guessed clip increase. These paths await headset testing.

## VR-005: Stabilize body inventory and magazine poses

Priority: high. Earlier body anchors showed debug spheres and magazines moved
unpredictably. Picking up a magazine must give it a consistent hand pose.

- Use a consistent body transform for visible ammo, grab regions and holsters.
  Check head turning, room-scale movement, snap turning and recentering.
- Calibrate magazine orientation and insertion per weapon without preserving an
  accidental wrist angle from pickup. Keep debug geometry disabled by default.
- Treat full-body IK as a separate implementation task, including shoulder,
  elbow and torso behavior. Body visibility alone is not a physical body.

Further offline fixes give fresh ammunition a calibrated controller-local
orientation instead of capturing an accidental pickup angle. Its model grip
point remains at the palm across wrist motion in the shared input/render path.
Invalid capture leaves the item at the body anchor and does not take the hand.
Source vector/angle calibration defaults are initialized explicitly. Reload,
shell-port alignment and glove/native-hand visuals still need a headset test.

Related: [magazine pose #392](https://github.com/keyou91/l4d2vr/issues/392),
[body IK #393](https://github.com/keyou91/l4d2vr/issues/393).

## VR-006: Verify multiplayer pose replication and physical melee

Priority: high. The user could not see the host's VR movements.

Implemented offline: another player's command cannot consume the host's queued
manual-reload ammo update. This is separate from the remaining pose-replication
checks below.

Further offline changes retain physical melee pose and swing intent per command,
detect wrist-led swings, remove walking velocity, and reject repeated collision
commands. Sweep history is scoped to a living owner and melee weapon. Native
collision/gore calls are retained; headset and two-client outcomes remain untested.

- Verify the committed listen-server acknowledgement with two VR clients,
  including a late join, compatibility fallback and map change.
- Check both hands and melee sweeps on the other client. Physical swings must
  use native damage and gore without requiring a button in the selected mode.
- Keep dedicated-server relay capability distinct from VR user-command support.

Related: [remote tracking #408](https://github.com/keyou91/l4d2vr/issues/408).

## VR-007: Investigate crashes with evidence

Priority: high. A friend crashed using a pipe bomb. Another player's upstream
report associates crashes with Grip Pull; neither establishes the friend's cause.

- Obtain a matching crash dump and logs before attributing the pipe-bomb crash.
- Audit object-pull entity lifetime and client/server transitions. Compare
  enabled and disabled Grip Pull using a controlled future gameplay test.
- Preserve the separately diagnosed shadow-renderer workaround while investigating
  these other failures. Avoid changing unrelated settings during a crash test.

Related: [Grip Pull crash report #400](https://github.com/keyou91/l4d2vr/issues/400).

## VR-008: Complete pistol, Magnum and ammunition models

Priority: medium. Rotated stock weapons expose missing or culled surfaces.

- Distinguish back-face culling from genuinely missing mesh geometry. The local
  material tool addresses existing surfaces only.
- Check the full weapon, magazine, slide and chamber from all angles, and align
  interaction regions to the visible parts. Use locally derived game assets;
  do not bundle proprietary assets in the contribution.

## VR-009: Verify the integrated calibration in VR

Priority: high before release. The PR integration is subject to offline checks;
headset alignment and multiplayer outcomes still require validation.

- Compare zero offsets with the previous committed behavior. Check a small
  position offset and each rotation with the aim line visible and hidden.
- Check client and host impacts, backup input commands, AutoGrip, scopes, mouse
  mode, left-handed mode and the dual-pistol prototype.
- Confirm saved settings persist on restart with an older sample config.

Reference: [calibration PR #404](https://github.com/keyou91/l4d2vr/pull/404).

## VR-010: Add authoritative remote physical ammunition requests

Priority: high for friend-hosted play. Physical ammo needs its own negotiated
host transaction; pose and physical-control acknowledgements alone cannot
establish authoritative ammunition on the friend's host.

Implemented offline on October 8: a separate one-shell protocol negotiates a
connection token, validates the owned weapon's network serial and recent input,
reads native server capacity/reserve, and transfers one existing round. The
guest waits for a matching result and Source ammo replication. Duplicate requests
reuse their result; stale or changed ammo/ownership is rejected. Unsupported or
timed-out leases restore conventional reload. Friend-hosted delivery and native
replication still need testing.

Detachable-magazine transactions are also implemented offline: explicit eject
retains at most one chambered round, and insert transfers available native
reserve up to native capacity. The host reads the engine's infinite-ammo rule;
it does not assume all pistols are infinite. Duplicate requests cannot transfer
ammo twice, and insertion requires a preceding accepted ejection. Source ammo
replication and the matching result must both arrive before the guest advances.
Empty reloads still require the slide; a late slide region cannot automatically
complete that remote reload. Native dual pistols keep their shared reload
fallback. Magazine protocol version 2 implements grip catch/reinsert with the
actual removed rounds, preserving partial and empty contents without spending
reserve. Withdrawal from the well is required before return. Contact and server
accounting are covered offline; headset feel remains unverified. Floor pickup,
persistent magazine storage and transfer to another player remain unfinished.

The supported listen-host now uses the same magazine ledger as the guest,
instead of filling a predicted clip to its default maximum. Tests include a
partial reserve and spending the retained chamber between eject/insert.
Backend changes fence old queued updates. Single-magazine local prediction and
its queued clip/reserve writers have been removed. Unsupported magazine backends
use native reload, including the Magazine Release button, while retaining support
grip and clearing virtual ammo/chamber state. The local shotgun writer and
reserve-offset search have also been retired. Both host and guest load one
native-confirmed shell per insertion. Unsupported/expired shell support restores
stock reload, pumping and audio; changing the capability fences current hand
state. Local command delivery, actual replication and physical cycling still
require gameplay verification.

- Define a separately negotiated request/acknowledgement for physical shell and
  magazine completion, scoped to the player's actual owned weapon and entity
  serial. Reject duplicate, stale and incompatible requests.
- Read clip capacity and reserve on the server. Consume only confirmed physical
  insertions and replicate the result without client-side reserve-slot guesses.
- Preserve conventional reload on unsupported servers. Do not enable remote
  shell loading by treating pose or grip acknowledgement as ammo support.
- Test the guest and host separately, including backup commands, map/weapon
  changes, interruption, no reserve ammo and a late join.
