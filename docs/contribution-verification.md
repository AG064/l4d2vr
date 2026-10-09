# Contribution verification

## Draft scope and evidence

This contribution includes the full experimental interaction branch: input and
reload lifecycle recovery, optional magazine release, native grip pickup/drop,
body inventory anchors, hand/ammo alignment, manual bolt and pump cycling,
independent dual-pistol tracking/aim and native detachment, physical melee,
weapon/bullet calibration adapted from PR #404, and separately negotiated
friend-hosted shell and detachable-magazine transactions. See `physical-interactions.md`,
`manual-reload-input.md`, `weapon-calibration.md` and `vr-backlog.md` for their
controls, limitations and remaining tests.

The current runtime source passed 21 CTest suites and Windows Release x86
DLL/pose-server builds with v143 in a temporary build tree. Seven native ammo
signatures matched uniquely against the installed server binary. These are
offline checks. Later documentation edits do not change the tested runtime.

The experimental switches are disabled in the repository defaults. Saved user
configuration is preserved. The material tool derives overrides locally and
does not add Valve weapon models/materials or compiled game binaries to this PR.

This is an umbrella draft for maintainer feedback and can be divided into
smaller contributions. Headset and two-client acceptance remain pending;
compilation and state-machine tests do not establish gameplay success.

`VR interaction tests` runs the offline CTest suites on Windows x86 for relevant
pushes and pull requests, including drafts. Its build output is confined to the
runner temporary directory. It uses a read-only token and does not load L4D2 or
publish game binaries. Fork pull-request runs can require maintainer approval.
The existing manually triggered DLL-build workflow is separate.

Run the small offline tests in a temporary directory. They do not launch L4D2,
load its DLL, or modify the game installation.

```powershell
$testRoot = Join-Path $env:TEMP ('l4d2vr-tests-' + [guid]::NewGuid().ToString('N'))
cmake -S tests -B $testRoot -G 'Visual Studio 17 2022' -A Win32
cmake --build $testRoot --config Release --clean-first
ctest --test-dir $testRoot -C Release --output-on-failure
cmake --build $testRoot --config Debug --target vr_hand_alignment_tests
ctest --test-dir $testRoot -C Debug -R '^vr_hand_alignment$' --output-on-failure
```

Python 3 is needed for the controller-profile and material tests. CMake reports
whether it found an interpreter. The C++ suites remain available without Python.

For a DLL build, use the repository's x86 build instructions. Its normal build
has a post-build copy into a game installation. For offline verification, copy
the source to a temporary directory, remove the `PostBuildEvent` from the staged
project, and build that copy. Keep output and intermediate directories there.

Use `Rebuild` for the staged DLL solution. MSBuild can omit dependencies under
TEMP from its incremental tracking, so a header or `.inl` edit may otherwise
leave old object code in a seemingly passing build. The final checks use clean
test builds and a full DLL/pose-server rebuild. The Debug hand test enables
the SDK's `VECTOR_PARANOIA` behavior, which seeds unspecified vector/angle
values with NaN and detects accidental assumptions about zero defaults.

The contribution changes were checked with the v143 compiler. The bundled
MinHook archive was incompatible with that compiler's link-time code generation,
so MinHook 1.3.4 was rebuilt under the same temporary toolchain. That replacement
was used only in the temporary build; the repository's dependency archive was
not changed.

## Local pistol material preparation

The tool derives two material overrides from an installed copy of L4D2. It does
not redistribute Valve materials, edit the game installation, or launch it.
Without `--output`, it writes into a new temporary directory.

```powershell
python tools/prepare_vr_materials.py --vpk 'PATH_TO_GAME/left4dead2/pak01_dir.vpk'
```

It verifies VPK size and CRC, preserves shader parameters, refuses unresolved
patch materials, and refuses to overwrite existing output. The overrides make
existing pistol and Deagle surfaces visible from both sides. Missing geometry
still requires model work; this tool does not reconstruct it.

## Remaining gameplay checks

- Two VR clients on a friend-hosted local server: join after the initial fallback
  window, confirm VR movement and pose relay, then test melee and projectiles.
- Single detachable-magazine weapons: loaded and empty button release, insertion,
  and the existing slide interaction, with legacy mode also checked.
- Same-type weapon changes and incapacitation/revival: confirm support grip and
  manual reload state recover.
- Headset visuals from different pistol/Deagle angles: confirm the local material
  overrides behave correctly with the active models and skins.
- Grip pickup/drop and body inventory: table contact, world placement, hand/ammo
  poses, snap turning, looking down and tracking-loss recovery.
- Fresh ammo grip: repeat pickup at different wrist/gun angles, check the
  configured grip rotation, palm contact and the same pose in queued rendering.
- Dual pistols: each hand's aim, alternating/simultaneous triggers, detachment,
  pickup and the native shared-ammo reload fallback.
- Pistol ammunition: join unequal clips, fire from one hand, and drop either
  pistol. Retained plus dropped ammunition must match the native shared total,
  including native reloads, weapon replacement, owner changes and unknown pairs.
- Native firing boundary: switch to pistols and immediately fire either hand;
  verify multi-shot frames, effects, native clip replication and optional-hook
  fallback. Runtime per-hand ammo delivery and replication acceptance remain
  unfinished.
- Pistol-ammo feed: matching capability/session, loaded/empty and unequal clips,
  snapshot-before/after-native-replication delivery, weapon/player changes,
  menu/paused recovery, heartbeat/timeout and an older unsupported host.
- Client pistol prediction: replay an older left shot after a newer right shot;
  check ray, haptic hand and hit-feedback command, owner/weapon replacement,
  capture expiry, non-prediction events and the optional-hook fallback.
- Per-hand ammo prediction: acknowledgement and older-protocol fallback; right
  empty/left loaded and the reverse; simultaneous triggers; native reload and
  down/revive; server acknowledgement arriving during replay; same-command
  multiple firing events; native infinite-ammo modes; clip replication and
  missing/unexpected native bullet events. Confirm no duplicate consumption or
  guessed clip/reserve writes on either player.
- Native pistol execution: command-to-hand association across delayed commands,
  backup packets, weapon changes, two players and the optional-hook fallback.
- Executed reload policy: packet batching, grip/pull commands, ordinary reload
  permission, weapon switching, revival and the optional-hook fallback.
- Pump shotguns: partial tube top-up, empty loading, rear/forward strokes,
  continuous support grip and re-grabbing a partially moved fore-end.
- Physical melee: trigger released, wrist-led and translated swings, native
  damage/gore, per-swing hit limits and duplicate input commands.
- Remote shells: matching host/guest builds, real clip/reserve replication,
  stale/duplicate requests, timeout/older-host fallback and map changes.
- Remote magazines: loaded and empty ejection, partial reserve, native finite
  and infinite pistol ammo, insertion then slide cycling, weapon changes and
  both orders of result/replicated-ammo delivery.
- Magazine fallback: unsupported and timed-out hosts must use stock ammo and
  reload, including Magazine Release while the support grip is held. Check menu,
  weapon/player changes and restoration of the negotiated physical path.
- Retained magazine: grip catch at the well, withdrawal and return, partial and
  empty magazines, unchanged reserve, spending the chamber while it is removed,
  release/cancellation, incompatible capacity and a failed native insertion.

Offline builds and tests do not confirm those headset or multiplayer results.
Full body IK, independent dual-pistol magazines/chambers, complete missing mesh
geometry, dropped-magazine floor recovery/storage and dedicated-server physical ammo remain
unfinished. The reported pipe-bomb crash and missing remote movement have not
been proven fixed. These limitations are part of the draft review scope.
