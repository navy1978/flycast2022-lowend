# Low-End Compatibility Notes

These observations describe the experimental renderer options in Flycast 2022
Low-End. They are not a general Dreamcast compatibility list.

Test platform:

- Anbernic RG351V
- RK3326 / Cortex-A35
- Mali-G31 OpenGL ES
- AmberELEC
- RetroRun GO2 video and audio

## Current observations

Compatible GLES texture storage reuse passed the user's visual and audible
RG351V tests and is enabled by default. It can be disabled without rebuilding:

```ini
reicast_texture_storage_reuse = disabled
```

## Fast Depth

`reicast_fast_depth` is an inaccurate, per-game option. It moves logarithmic
depth calculation from each fragment to the vertex shader and lets GLES
interpolate it. It remains disabled by default because compatibility varies by
game.

| Game and product number | RG351V result | Recommendation |
| --- | --- | --- |
| Sonic Adventure 2, `MK-51117` | The final `menu_guarded_shadow_safe` plus opaque-merge profile measured 30.35 FPS against 23.24 FPS for the previous shadow-safe profile (`+30.6%`) on the same save-state, with no skipped frames. The user approved gameplay, menus, audio and projected shadows. | Use `menu_guarded_shadow_safe` with opaque merge for the validated performance profile. Keep the older `menu_guarded` depth-only mode only for reproducing the rejected shadow artifact. |
| Soul Calibur, `T1401D  50` | The ordinary fast translucent pre-sort can put scenery over the health bar. The new `top_hud_last` candidate gained 23.2% in two short paired tests, but 16.79% in the stricter 2,400-frame final pair; all presented every frame. | Await the final manual health-bar review. The stable long-run 20% target is not yet demonstrated. |
| Dead or Alive 2 observed CDI, `RDC-0140` / `RDC-0149` | The original mode flashed. The later `vertex_fast_log` profile with fog/mipmapping off, opaque merge and D24S0 passed the user's video, audio and input checks. Three paired tests improved the median by 28.91% and aggregate time by 25.59%. Correctly cataloging the observed `RDC-0140` image changed its measured result from about 31.6 to 41.2 FPS. | Use only the complete tested profile; do not generalize the initial mode result. |

The product number is read from the disc metadata and appears in the Flycast
boot log. It is a better per-game configuration key than a filename, but
different releases of the same game can have different product numbers.

`menu_guarded` does not inspect game RAM or hard-code a Sonic address. It uses
three renderer-level signals: a low-complexity scene, a cluster of similarly
sized font-like quads in an interface-sized scene, or three consecutive
matching sampled geometry signatures. The last signal catches a paused 3D
scene without mistaking the normal two-frame repetition of a 30-FPS game for a
pause. Moving gameplay continues to use `vertex_fast_log`.

`menu_guarded_shadow_safe` adds a renderer-level PowerVR check during moving
gameplay. Opaque polygons marked as shadow receivers use accurate fragment
depth only when the maximum vertex depth is more than four times the minimum.
The threshold was selected empirically after isolating the Sonic defect to the
opaque list: accurate translucent, punch-through and modifier-volume-only
tests did not fix it; all shadow receivers did, but cost 30.382 seconds for
600 frames versus a 25.727-second fast control. The 4x filter passed the
user's visual test. The early stable 600-frame aggregate totaled 51.679 seconds
versus 51.500 seconds for the controls. On the final clean public build,
`menu_guarded` completed 600 frames in 27.033 seconds and the shadow-safe mode
in 28.070 seconds, a 3.84% aggressive-mode advantage. Two 300-frame pairs
showed larger differences, so the cost depends on how many risky shadow
receivers occur in the measured scene. The first filtered run (28.448 seconds)
is preserved as an outlier rather than omitted from the raw evidence.

For `MK-51117`, the older `menu_guarded` mode remains available as a diagnostic
alternative. It keeps the menu and pause detection but applies the fast mode
to all moving-gameplay opaque polygons and can produce black rectangular
projected shadows. It was not faster than the final shadow-safe plus
opaque-merge combination in the controlled 45-second comparison.

## Low-end audio mixer

`reicast_audio_mixer = lowend` deliberately omits several AICA effects and
routes. The user considered the tested Sonic Adventure 2 audio correct. In
Dead or Alive 2 Europe the audio was noticeably lower quality, so the accurate
mixer is recommended for that release unless the extra CPU saving is required.

Reducing AICA ARM7 cycles from 32 to 8 produced only a small, variable DOA
signal (about 4% by aggregate time in the first series). It remains an optional
manual-audio candidate and is not part of the approved DOA profile.

On Soul Calibur, 8 cycles briefly gained 5.61% in two short aggregate pairs but
regressed to 47.689 seconds in the long run. Values 24 and 16 were slower and
12 was neutral. The Soul profile therefore keeps the accurate value 32.

## Crazy Taxi

The compatible 90-second attract-mode state identifies product `MK-51035`.
Fast depth, fog/mipmap removal, AICA underclock/mixer changes, direct SH4
FPSCR, D24S0, opaque grouping and direct DRM scanout did not produce a
repeatable useful gain. Direct scanout was functional but slower. Keep the
accurate control profile until a different bottleneck is found.

## Older renderer experiments

| Game | Adjacent state elision | Translucent strip merge | Notes |
| --- | --- | --- | --- |
| Soul Calibur | No visible regression in tested scenes | Usable in the corrected-order build | Strong performance and working menus in the latest manual test. Re-test additional stages before treating it as fully compatible. |
| Dead or Alive 2 | No visible regression in tested scenes | Usable in tested USA image | No obvious transparency or menu regression was reported. Audio quality depends on the frontend backend and is not controlled by this core option. |
| Sonic Adventure 2 | Controls and gameplay work | Not recommended | The faster path leaves menu text readable but can render menu backgrounds/elements incorrectly. |
| Crazy Taxi | No visible regression in the tested run | Not yet classified | Gameplay and audio were reported as good, but the inaccurate strip merge path still needs an isolated A/B visual test. |
| Power Stone | No visible regression in the tested run | Not yet classified | Requires an isolated A/B visual test before enabling the inaccurate path. |

## Reporting a result

Record all of the following:

1. device and operating system;
2. game region and image format;
3. internal resolution;
4. alpha sorting mode;
5. both low-end option values;
6. whether menus, transparency, shadows and render-to-texture effects work;
7. measured emulation speed and rendered FPS when available.

Short title screens are not sufficient. Test gameplay, menus and at least one
effect-heavy scene.
