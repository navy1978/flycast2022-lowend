# RG351V product-profile follow-up

This directory preserves the fixed-frame tests used to select low-end profiles
for specific Dreamcast product numbers. No frameskip was used. RetroRun reset
DRM before every run, locked the RG351V clocks, loaded the same compatible
automatic state for each pair, and stopped after the requested number of core
frames.

## Product numbers

| Product | Content | State |
| --- | --- | --- |
| `RDC-0149` | Dead or Alive 2 Europe RDC | user-created in-game audio state |
| `T1401D  50` | Soul Calibur Europe | user-created in-game state |
| `MK-51035` | Crazy Taxi | new state captured after 90 seconds of no-input attract mode |

The Crazy Taxi state is archived under
`benchmarks/device-states/crazy-taxi-attract/`. Its SHA-256 is
`b2618d2f57b8280265f281e82e6e79f3b30f50943851b15006622408d0517f17`.

## Retained results

### Dead or Alive 2

The complete candidate combines `vertex_fast_log`, mipmapping and fog disabled,
opaque-strip grouping, texture storage reuse, accurate audio and an EGL D24S0
surface. Three 600-frame control/candidate pairs produced:

- control durations: 24.899, 25.833 and 26.532 seconds;
- candidate durations: 20.039, 21.708 and 19.775 seconds;
- median improvement: 28.91%;
- aggregate-time improvement: 25.59%;
- zero skipped frames.

The user approved video, audio, controls and perceived game speed. AICA ARM7
underclocking remains a separate disabled-by-default experiment: the initial
32-versus-8 series was variable and improved aggregate time by only about 4%.

### Soul Calibur

The accurate-order control removes the health-bar/scenery ordering bug but is
too slow. Per-triangle compatibility sorting is visually correct but recovered
only about 4%. The retained candidate uses `top_hud_last`: ordinary world
transparency retains the fast depth pre-sort while wide geometry confined to
the upper HUD band is submitted last in original order.

Two alternating 1,200-frame pairs before the final classifier optimization
were:

| Pair | Correct-order control | `top_hud_last` candidate | Improvement |
| --- | ---: | ---: | ---: |
| 1 | 27.996 s | 23.265 s | 20.34% |
| 2 | 29.014 s | 23.011 s | 26.09% |
| Aggregate | 57.010 s | 46.276 s | 23.20% |

All runs presented every requested frame. Reusing the HUD compaction storage
improved two 1,200-frame pairs by 3.73% in aggregate relative to the immediately
preceding HUD implementation. The stricter final long pair on the same final
core was:

| Profile | 2,400-frame duration | Effective FPS |
| --- | ---: | ---: |
| Correct translucent order | 49.781 s | 48.21 |
| Final `top_hud_last` | 42.623 s | 56.31 |

That is +16.79%, with 2,400/2,400 frames presented and zero skipped. Earlier
standalone candidate runs reached 42.283 seconds, but the same-core paired
result is the conservative number. A final manual review near trees and
mountains is still required, and the long-run 20% target is not yet met.

The optional audio thread gained in one pair and lost in the next: aggregate
time was 45.328 seconds for the normal path and 45.349 seconds for the thread,
so it is retained only for manual audio comparison. AICA at 8 cycles gained
5.61% in two short aggregate pairs, then regressed to 47.689 seconds over 2,400
frames. The final Soul profile keeps audio threading off and AICA at 32.

Later manual review confirmed that `top_hud_last` can still let scenery appear
over the health bar in some scenes. The catalog therefore uses the visually
correct `per_triangle` profile for `best_validated` and keeps `top_hud_last`
only as the documented `best_performance` alternative.

### Crazy Taxi

The 600-frame control was approximately 15.3 seconds (about 39.2 effective
FPS). Fast depth, fog/mipmap removal, opaque and punch-through grouping,
direct SH4 FPSCR, AICA underclock/mixer changes and D24S0 were neutral or
slower. DRM direct scanout handled 599/600 frames directly but increased the
run to 15.622 seconds. No Crazy Taxi optimization from this wave is retained.

## Rejected follow-ups

- low-end audio mixing at effective 22.05 and 11.025 kHz was slower than the
  accurate mixer in the DOA state. On Soul, quarter-rate gained 9.84% in two
  short aggregate pairs but regressed from 42.708 to 45.583 seconds in the
  2,400-frame run, so both reduced-rate implementations were removed;
- punch-through geometry grouping was exactly neutral in Crazy Taxi;
- direct DRM scanout worked but was slower on the tested Mali/GO2 path;
- SH4 FPSCR and opaque grouping were within about 0.2% of the Crazy Taxi
  control.
- disabling PowerVR volume modifiers gained in one Soul pair and lost in the
  next; aggregate time was slightly worse, so shadows remain enabled;
- a 16-bit EGL depth buffer is unavailable with the RG351V's RGB565 ARM EGL
  configuration; the exact request was rejected before rendering.

The rejected implementations were removed from the production source when
they had no other validated use. Raw JSON, logs and exact configuration files
are preserved in `raw/` and
`rg351v-product-profile-raw-20260727.tgz` (SHA-256
`c307704af41a8b9ad35a3038b2e92a6ca8aea31f81b51c2a17545f5b58c4c596`).

The persistent Flycast/RetroRun artifacts passed 120-frame smoke tests for Soul
Calibur, DOA2 and Crazy Taxi: every test presented 120/120 frames and skipped
zero. The final clean non-profiler core
(`6cf4a036bc351b02efbb8e66e31f2e6b5c5c9dd51a49b3f7059c8ab91987c888`)
also passed a fixed 120-frame Sonic smoke with the shadow-safe profile:
120/120 frames presented, zero skipped, and product number `MK-51117`.
