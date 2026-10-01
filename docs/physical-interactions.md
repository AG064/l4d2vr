# Experimental physical interactions

This branch contains opt-in grip pickup/drop, body inventory access, manual
pump cycling, and independently tracked dual pistols. These features have
automated state and geometry coverage and compile in the Windows x86 Release
build. Headset and multiplayer validation remain required.

Enable `ManualThrowEnabled`, `GripReleaseDropEnabled`, and
`DualPistolsIndependentHandsEnabled` to test pistol detachment on a server
running the same build. Keep `DualPistolsNativeReloadFallbackEnabled` enabled
for dual pistols and for a single pistol retained in the gameplay left hand.
The repository config leaves these experimental switches off by default.

With the supplied Quest binding:

- Right grip picks up inventory items and drops the held item on release.
- Left grip can pick up a loose pistol when the pistol hand is free.
- Each trigger aims and fires its own pistol while dual pistols are equipped.
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

For a headset test, start with both grips released, then grip each gun. Fire
with each trigger and check its aim. Release one grip, confirm that a world
pistol falls at that hand and the other pistol stays held, then grip the
dropped pistol to recover the pair. Repeat with the other hand, both grips
released together, partially spent clips, a weapon switch, and revival.
Check `vrmod_log.txt` for `[VR][PistolDetach]` and `[VR][PistolPickup]` events
with `VrHandsDebugLog` enabled.
