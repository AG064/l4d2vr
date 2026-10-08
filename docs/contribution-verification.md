# Contribution verification

## Draft scope and evidence

This contribution includes the full experimental interaction branch: input and
reload lifecycle recovery, optional magazine release, native grip pickup/drop,
body inventory anchors, hand/ammo alignment, manual bolt and pump cycling,
independent dual-pistol tracking/aim and native detachment, physical melee,
weapon/bullet calibration adapted from PR #404, and separately negotiated
friend-hosted shotgun shell transactions. See `physical-interactions.md`,
`manual-reload-input.md`, `weapon-calibration.md` and `vr-backlog.md` for their
controls, limitations and remaining tests.

The current runtime source passed 13 CTest suites and Windows Release x86
DLL/pose-server builds with v143 in a temporary build tree. Six new native shell
signatures matched uniquely against the installed server binary. These are
offline checks. Later documentation edits do not change the tested runtime.

The experimental switches are disabled in the repository defaults. Saved user
configuration is preserved. The material tool derives overrides locally and
does not add Valve weapon models/materials or compiled game binaries to this PR.

This is an umbrella draft for maintainer feedback and can be divided into
smaller contributions. Headset and two-client acceptance remain pending;
compilation and state-machine tests do not establish gameplay success.

Run the small offline tests in a temporary directory. They do not launch L4D2,
load its DLL, or modify the game installation.

```powershell
$testRoot = Join-Path $env:TEMP ('l4d2vr-tests-' + [guid]::NewGuid().ToString('N'))
cmake -S tests -B $testRoot -G 'Visual Studio 17 2022' -A Win32
cmake --build $testRoot --config Release
ctest --test-dir $testRoot -C Release --output-on-failure
```

Python 3 is needed for the controller-profile and material tests. CMake reports
whether it found an interpreter. The C++ suites remain available without Python.

For a DLL build, use the repository's x86 build instructions. Its normal build
has a post-build copy into a game installation. For offline verification, copy
the source to a temporary directory, remove the `PostBuildEvent` from the staged
project, and build that copy. Keep output and intermediate directories there.

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
- Dual pistols: each hand's aim, alternating/simultaneous triggers, detachment,
  pickup and the native shared-ammo reload fallback.
- Pump shotguns: partial tube top-up, empty loading, rear/forward strokes,
  continuous support grip and re-grabbing a partially moved fore-end.
- Physical melee: trigger released, wrist-led and translated swings, native
  damage/gore, per-swing hit limits and duplicate input commands.
- Remote shells: matching host/guest builds, real clip/reserve replication,
  stale/duplicate requests, timeout/older-host fallback and map changes.

Offline builds and tests do not confirm those headset or multiplayer results.
Full body IK, independent dual-pistol magazines/chambers, complete missing mesh
geometry, remote detachable magazines and dedicated-server physical ammo remain
unfinished. The reported pipe-bomb crash and missing remote movement have not
been proven fixed. These limitations are part of the draft review scope.
