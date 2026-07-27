# RK3566 / dArkOS benchmark

## Purpose

This comparison isolates the effect of the Flycast compiler target and checks
the current low-end stack against the Flycast and RetroRun binaries shipped by
dArkOS.

Device and method:

- Anbernic RG353M, RK3566 / Cortex-A55, dArkOS;
- CPU, GPU and DMC performance governors;
- process priority `nice -n -19`;
- 30-second warm-up followed by a 90-second measurement;
- the same ROM for each comparison.

The original RetroRun 2.7.7 and Flycast pair is always launched with the
dArkOS distribution configuration, `/home/ark/.config/retrorun.cfg`. This is
the configuration seen by dArkOS users and is therefore the valid old-stack
reference.

## Results

| Game / stack | Average FPS |
| --- | ---: |
| Sonic Adventure 2 — current RetroRun, generic ARM64 Flycast | 41.886 |
| Sonic Adventure 2 — current RetroRun, RK3566/Cortex-A55 Flycast | 47.287 |
| Sonic Adventure 2 — current RetroRun, original dArkOS Flycast | 47.511 |
| Sonic Adventure 2 — instrumented dArkOS RetroRun 2.7.7, original dArkOS Flycast | 47.722 |
| Sonic Adventure 2 — instrumented dArkOS RetroRun 2.7.7, profiled dArkOS Flycast | 47.829 |
| Sonic Adventure 2 — dArkOS distribution stack, repeated with distribution config | 48.021 |
| Sonic Adventure 2 — current RK3566 release, hardware pacing override | 47.380 |
| Sonic Adventure 2 — current RK3566, AICA-only PGO | 50.913 |
| Soul Calibur — instrumented dArkOS RetroRun 2.7.7, original dArkOS Flycast | 57.387 |
| Soul Calibur — current RetroRun, RK3566 Flycast and catalog profile | 58.860 |
| Soul Calibur — current RK3566, AICA-only PGO trained on Sonic | 58.778 |
| Dead or Alive 2 combat save-state — dArkOS distribution stack | 36.620 |
| Dead or Alive 2 combat save-state — current RK3566 release | 43.372 |
| Dead or Alive 2 combat save-state — current RK3566, AICA-only PGO | 43.685 |

The RK3566 build improves Sonic Adventure 2 by 12.9% over the generic ARM64
build. It is within 0.5% of the original dArkOS core on Sonic, while the
current stack is 2.6% faster than the original dArkOS stack on Soul Calibur.

The AICA-only PGO candidate improves Sonic from 47.380 to 50.913 FPS
(+7.46%) in the reverse-order check and from 47.131 to 50.913 FPS (+8.02%)
against the earlier equivalent release run. It also exceeds the repeated
dArkOS distribution stack by 6.02%. Soul Calibur remains effectively neutral
(-0.14%) and retains the same number of audio underruns.

Dead or Alive 2 was also compared from the same 27,752,984-byte combat
save-state. Both current and original dArkOS cores accepted the state through
`retro_unserialize`. After a five-second warm-up, the 20-second measurements
were 43.685 FPS for PGO, 43.372 FPS for the non-PGO RK3566 release and 36.620
FPS for the dArkOS distribution stack. The current stack is therefore 19.3%
faster than dArkOS in the representative combat scene. AICA-only PGO itself
adds 0.72% because the validated DOA profile uses the accurate mixer; the
remaining gain comes from the RK3566 build and validated low-end graphics
configuration.

The 51 FPS displayed by the old RetroRun menu during the Sonic test is not
directly comparable: RetroRun 2.7.7 averages partial per-frame estimates. The
instrumented fixed-window benchmark measured 47.722 FPS.

The measurements above established the compiler-target regression. A final
release comparison should repeat each stack with identical overlay and audio
buffer settings; the diagnostic files retain those differences explicitly.

## Internal profile

The instrumented dArkOS Flycast core measured the following during Sonic:

| Stage | Total | Average |
| --- | ---: | ---: |
| Emulator-thread CPU | 89.250 s | — |
| AICA update | 46.938 s | 0.384 ms/update |
| Render wait | 51.819 s | 12.037 ms/frame |
| PVR process | 5.250 s | 1.224 ms/frame |
| PVR render | 30.978 s | 7.222 ms/frame |

AICA consumed about 52.6% of emulator-thread CPU time in this window.
Rendering is the other major optimization target.

The current low-end mixer profile gives a more specific result: almost all
measured AICA update time is inside audio mixing, while ARM7 and timer work are
a small remainder. This makes the mixer a better target than further reducing
AICA ARM cycles for Sonic. The current renderer profile also observes roughly
340 draw submissions per rendered frame; texture decode and upload are minor
during the steady gameplay window.

## Release build

```sh
make clean
make platform=RK3566 FORCE_GLES=1 HAVE_OPENMP=1 HAVE_LTCG=0 -j$(nproc)
```

After applying a source patch, including a rumble patch, clean and rerun the
same command. Never reuse objects created with another platform target.

At startup, verify:

```text
Build target: RK3566-cortex-a55
```

The benchmark instrumentation used for the old dArkOS comparison is maintained
in separate diagnostic worktrees and is not part of release builds.
