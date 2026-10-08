# Experimental physical interactions

Remote shotgun shell insertion is now implemented as a separately negotiated
protocol. Both the guest and listen-server host need this build. The host reads
its actual owned weapon, script clip capacity and native reserve. One request
transfers exactly one existing round, ends any earlier conventional reload, and
marks the weapon and owner for Source replication. The guest waits for both a
matching result and replicated clip/reserve values; it does not write predicted
ammo for this path. The local listen-host shell settlement remains separate.

Connection tokens, weapon network-handle serials, command freshness, expected
ammo, sequence watermarks and a duplicate-response cache protect the transaction.
The token binds a session; it is not an authentication credential. A request
cannot target someone else's gun or supply a desired clip count. Native function
signatures gate support. A missing backend, unsupported layout or a 2.5-second
reply/replication timeout preserves conventional reload and disables remote
shell insertion for that lease. A new map/connection can negotiate again.

This first protocol handles shotgun shells only. Remote detachable magazines,
independent dual-pistol ammunition and dedicated-server ammo support are still
unfinished. Native bindings and transaction tests passed offline; client/server
delivery, Source replication ordering and interaction feel remain unverified.
The edict notification and network-handle definitions follow the
[L4D2 SDK headers](https://github.com/alliedmodders/hl2sdk/tree/l4d2/public).
Handle serial comparison follows the SDK's
[send proxy](https://github.com/alliedmodders/hl2sdk/blob/l4d2/game/server/sendproxy.cpp).

For a future friend-hosted test, confirm `[VR][RemoteShell][client] protocol=1`
on the guest. Load one shell into a partial and an empty shotgun, check both
clip and reserve on the host, then test pumping and firing. Repeat with a weapon
switch, downing/revival, no reserve ammo and a map change. Check server
`[VR][RemoteShell][server]` and guest confirmation logs. Also test an older host
and a deliberately interrupted connection for conventional reload recovery.

This branch contains opt-in grip pickup/drop, body inventory access, manual
pump cycling, and independently tracked dual pistols. These features have
automated state and geometry coverage and compile in the Windows x86 Release
build. Headset and multiplayer validation remain required.

Enable `ManualThrowEnabled`, `GripReleaseDropEnabled`, and
`DualPistolsIndependentHandsEnabled` to test pistol detachment on a server
running the same build. Keep `DualPistolsNativeReloadFallbackEnabled` enabled
for dual pistols and for a single pistol retained in the gameplay left hand.
The repository config leaves these experimental switches off by default.
The server also advertises `l4d2vr_interaction_ack 1` separately from its pose
relay acknowledgement. Grip transport, independent pistol commands and native
reload suppression require that matching capability. A pose-only or older
server cannot enable them just by acknowledging VR movement. The capability
is cleared when the client changes server sessions; ordinary controls remain
available without it.

With the supplied Quest binding:

- Right grip picks up inventory items and drops the held item on release.
- Left grip can pick up a loose pistol when the pistol hand is free.
- Each trigger aims and fires its own pistol while dual pistols are equipped.
- Both pistol hands use the common predicted hit-feedback path. Firing haptics
  select the firing gameplay hand and follow the left-handed input mapping.
  Cached live shot rays are scoped to the current player/weapon and expire;
  they cannot redirect feedback for a subsequently selected gun.
- Releasing either grip splits a pair into a retained single pistol and a
  stock world `weapon_pistol`. The retained pistol stays in the other hand.
- Releasing both grips queues the second drop until the native split is
  confirmed. Tracking loss and holding grips across an ordinary weapon switch
  do not create release events.
- Right A releases a magazine for physical reloads. For dual pistols and a
  single pistol held in the left hand, it requests native reload instead.
- Left X toggles the flashlight; right stick click jumps.

Pistol splitting uses the native `CPistol::RemoveDualWeapons` function after
preparing a world pistol. Signature and clip-layout checks disable the
transaction if the server binary differs. Clips are partitioned without
adding rounds. A scoped native pickup hook restores the sum of the two clips
when an existing single pistol becomes a pair. Drop markers 60 and 61 are
stripped before native gameplay input and carry a command-number watermark
to reject duplicate backup commands. Switching the command pose between
hands never contributes a spurious throwing velocity.

The pair still uses L4D2's shared clip, firing cadence, animations, and native
reload. A split uses the stock single-pistol model, so the distinct left-hand
Glock appearance is not retained after detachment. Independent magazines,
chambers, safeties, and per-pistol cosmetic identity remain unfinished.
First-person bone retargeting does not yet separate remote world-model
pistols. Body inventory follows tracked head position and yaw; full body IK
and character-specific avatar fitting remain separate work.

Grip release places the item at the tracked hand with no throwing impulse.
Trigger-release throwing remains separate. Grip pickup uses nearby native
item selection with surface contact and controller aim on the server.
Physical reloads send a per-player reload gate; validated gun and shotgun
Reload hooks prevent the native automatic reload while the physical system
owns it. The gate is lifted for its explicit backend reload, and native dual
reload remains available. Both client and server need this build.

Free right gloves use the mirrored left glove calibration by default, rather
than the held viewmodel hand calibration. Advanced overrides are
`VrHandsRightFreePoseOffsetMeters` and `VrHandsRightFreePoseRotationOffsetDeg`.
Held ammo uses the glove's rotation convention and palm center. An observed
empty chamber stays empty until its required manual bolt cycle is completed.
Manual pump movement uses a closed rest pose rebased onto the current gun;
moving the pump hand does not steer the gun's aim. Native automatic pump
sounds are muted, while the sound from an actual manual stroke is allowed.

With empty-clip auto reload suppression enabled, chamber history is retained per
weapon across ordinary inventory switches. Inserting a magazine or receiving a
predicted clip refill does not clear an observed empty chamber. A completed
slide/bolt cycle clears it. Native entity handles distinguish replacement guns
at a reused address. When the handle cannot be resolved, history is limited to
the current draw. Death, incapacitation, disabled physical reload and player
replacement clear ownership; native dual-pistol reload remains separate.

A shotgun with a chambered round can fire after a partial tube reload. Holding
the next shell, cycling the action, an unfinished backend reload or a pending
shell ammunition update blocks firing. Inserting a shell into an empty gun still
requires a cycle. Shell insertion waits for a readable native ammo slot instead
of guessing a reserve or overlapping a previous insertion. The short settlement
gate clears on local server completion, session reset or a bounded timeout. Releasing a
partly moved pump retains its position; the grab region follows that position.

Grip pickup and release take priority over a simultaneous Object Pull packet,
including the controller pose associated with the grip action. Object Pull
and transient grip actions preserve the current reload gate for the same
weapon, rather than treating a missing reload impulse as permission to reload.
A different player entity starts a fresh reload command sequence. Controller
profile validation covers all default bindings and the optional Quest preset;
Knuckles and Cosmos keep analog turning without undeclared legacy turn actions.
Ordinary weapon swaps discard stale magazine and hand poses but retain the
same player's outstanding pump cycles. Death, incapacitation, disabled VR
interaction, and player replacement clear that cycling ownership.
Before a queued inventory drop executes, the server rechecks the living owner,
weapon identity, active inventory selection and bounded command age. A weapon
switch or an expired request cancels the transaction before inventory changes.

Physical melee uses the tracked weapon hand without an attack-button press on
a VR-aware server. Detection removes HMD translation and includes wrist rotation
as estimated weapon-tip motion. Rest arms the next swing; tracking loss, menus,
incapacitation and weapon/player changes require resting again before damage.
Each real input command retains its hand pose and swing state, including when
it is sent again as a backup. Stale tracking samples cannot start a swing.

The server selects the newest decoded pose in each packet and rejects commands
already used for a collision sweep. Weapon changes and long command gaps rebase
the sweep rather than tracing between unrelated poses. Collision calls still
use the native melee damage path and its per-swing hit list. This retains native
damage/gore handling, but blade alignment, hit feedback and multiplayer results
need a headset test. This is a directional native melee sweep, not a complete
rigid-body blade simulation. Very short gestures between network packets still
need evaluation. With `VrHandsDebugLog`, `[VR][PhysicalMelee]` identifies the
server player and command that started a traced swing.

For melee testing, release the attack trigger, rest the weapon hand briefly,
then try horizontal, overhead, straight and wrist-led swings. Walking with the
hand still should not attack. Check the same zombie across one continuous swing,
then rest and swing again. Repeat after a weapon swap, opening a menu, revival
and a map change. Compare damage and gore on the host and a joining VR client.

For reload testing, empty a pistol, insert a magazine, switch away before cycling
the slide, then draw it again and finish the cycle. Repeat with a different gun
of the same type. With a loaded shotgun, insert one shell into a partly filled
tube, release the shell grip and try firing after settlement. Check loading an
empty gun and cycling it after one shell. Release and re-grab the pump midway
through both strokes. These changes still need headset validation.

For a headset test, start with both grips released, then grip each gun. Fire
with each trigger and check its aim. Release one grip, confirm that a world
pistol falls at that hand and the other pistol stays held, then grip the
dropped pistol to recover the pair. Repeat with the other hand, both grips
released together, partially spent clips, a weapon switch, and revival.
Check `vrmod_log.txt` for `[VR][PistolDetach]` and `[VR][PistolPickup]` events
with `VrHandsDebugLog` enabled.

`[VR][GripDrop][input]` reports final eligibility, release state, and the number
of extra camera samples. These samples keep the live grip latch, rather than
resetting it simply because their command number is zero. `blockers` is a
hexadecimal mask: 0x1 inactive VR, 0x2 outside a game, 0x4 unreadable player,
0x8 dead/observer/wrong team, 0x10 incapacitated, 0x20 frozen, 0x40 paused,
0x80 menu cursor, 0x100 mouse mode, 0x200 teleport scout, 0x400 suppressed input,
0x800 control not ready, 0x1000 special infected control, 0x2000 mounted weapon,
0x8000 missing inventory weapon, and 0x10000 unreadable session. Server support
and the inventory-drop backend are logged separately.
