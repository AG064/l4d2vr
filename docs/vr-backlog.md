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

- Trace contact pickup and the client/server drop request through native weapon
  ownership. Distinguish close-range grip pickup from remote Grip Pull.
- Holding grip near a reachable weapon must pick it up. Releasing grip must
  create a visible, usable dropped weapon when this optional mode is enabled.
- Check two clients and prevent a released grip from dropping a subsequently
  selected weapon. Keep the existing control preset available.

## VR-003: Finish independent dual pistols

Priority: high. Earlier dual pistols followed only the right controller and
aimed both shots from that hand; the prototype also lost reload functionality.

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

## VR-005: Stabilize body inventory and magazine poses

Priority: high. Earlier body anchors showed debug spheres and magazines moved
unpredictably. Picking up a magazine must give it a consistent hand pose.

- Use a consistent body transform for visible ammo, grab regions and holsters.
  Check head turning, room-scale movement, snap turning and recentering.
- Calibrate magazine orientation and insertion per weapon without preserving an
  accidental wrist angle from pickup. Keep debug geometry disabled by default.
- Treat full-body IK as a separate implementation task, including shoulder,
  elbow and torso behavior. Body visibility alone is not a physical body.

Related: [magazine pose #392](https://github.com/keyou91/l4d2vr/issues/392),
[body IK #393](https://github.com/keyou91/l4d2vr/issues/393).

## VR-006: Verify multiplayer pose replication and physical melee

Priority: high. The user could not see the host's VR movements.

Implemented offline: another player's command cannot consume the host's queued
manual-reload ammo update. This is separate from the remaining pose-replication
checks below.

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
