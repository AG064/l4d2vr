# Optional manual reload input

The existing controller bindings remain the default. The optional Quest profile
uses left grip for off-hand support, magazines and the slide; right A for magazine
release; right stick press for jump; and left X for the flashlight.

To use it, back up `vr/SteamVRActionManifest/bindings_oculus_touch.json` in the game
installation and replace it with `bindings_oculus_touch_manual_reload.json` from
this repository. Restart SteamVR and the game so the binding is reloaded. Enable
these VR settings:

- `MagazineInteractionEnabled=true`
- `MagazineInteractionUseButtonGripInput=true`
- `MagazineInteractionSeparateButtonInput=false`
- `MagazineReleaseButtonRequired=true`
- `MagazineInteractionQuickReloadMode=false`

The profile is for right-handed Quest controllers. Left-handed users can assign
the optional Off Hand Grip and Magazine Release actions to their preferred hands
in SteamVR. Other controller profiles can bind those actions without adopting
this layout.

In button mode, running out of ammunition does not eject a detachable magazine.
Magazine Release can eject a loaded or empty magazine. Off-hand grip then handles
the loose magazine and the existing slide interaction. A loaded magazine change
retains one existing chambered round; an empty pistol does not gain a round from
its infinite reserve.

This input change does not implement separate physical magazines or chambers for
dual pistols. Dual-pistol physical reload, native hand tracking without gloves,
and recovery after incapacitation still need in-game verification. The existing
VR Gloves workaround remains available.

An older workshop `config.sample` must not erase the saved optional release
setting. A commented sample default seeds a missing setting; an active sample
assignment can still override a saved value under the repository's current
startup behavior.
