# Contribution verification

Run the small offline tests in a temporary directory. They do not launch L4D2,
load its DLL, or modify the game installation.

```powershell
$testRoot = Join-Path $env:TEMP ('l4d2vr-tests-' + [guid]::NewGuid().ToString('N'))
cmake -S tests -B $testRoot -G 'Visual Studio 17 2022' -A Win32
cmake --build $testRoot --config Release
ctest --test-dir $testRoot -C Release --output-on-failure
```

Python 3 is needed for the controller-profile and material tests. CMake reports
whether it found an interpreter. The C++ test remains available without Python.

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

Offline builds and tests do not confirm those headset or multiplayer results.
The experimental body inventory, pump cycling, grip drop and dual-pistol aiming
work remains outside this contribution until its reported failures are resolved.
