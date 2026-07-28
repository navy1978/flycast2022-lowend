# RG351V native-30-FPS VBlank guard

## Purpose

The RG351V adaptive v9 controller improves Dead or Alive 2 gameplay, but draw
skipping makes native-30-FPS intermissions less fluid without increasing core
throughput. This experimental guard suspends adaptive draw skipping only while
the game itself submits one PVR frame every two emulated Dreamcast VBlanks.

The detector observes `STARTRENDER` submissions on the emulation thread, before
the renderer queue can discard intermediate contexts. Measuring cadence when
the render thread consumes a frame is invalid under load because queue loss can
make 60-FPS gameplay look like native-30-FPS content.

## Detector

- sample window: 24 screen-render submissions;
- native-30-FPS entry: at least 20 submissions separated by two VBlanks;
- exit: four consecutive submissions separated by one VBlank;
- re-entry cooldown after exit: 48 submissions;
- render-thread communication: atomic native-30-FPS state;
- native mode action: save the current adaptive level, force level zero, then
  restore the saved level when 60-FPS cadence returns.

The detector does not use geometry, title identifiers, wall-clock FPS or a list
of known scenes.

## Rejected first implementation

The first implementation sampled cadence in `beginFrame()`, after render-queue
loss. It correctly improved the intermissions but falsely classified gameplay
and reduced it to 27.86 FPS. It was not committed.

## Clean-build results

All tests used RetroRun commit `c0754a7`, the accepted v28 audio environment,
the same DOA2 ROM and fixed performance governors.

| Test | Core frames | Presented | Adaptive/duplicate skips | Audio underruns |
| --- | ---: | ---: | ---: | ---: |
| Gameplay, VBlank guard v2, 30.002 s | 1200 | 620 | 580 | 9 |
| Intermission slot 2, guard v2 | 112 | 104 | 8 | 0 |
| Intermission slot 3, guard v2 | 103 | 96 | 7 | 0 |

The gameplay result is 40.00 FPS. No native-30-FPS transition was logged during
the gameplay window. Both intermission windows logged entry into native mode
and retained all frames except frontend initialization/duplication frames.

For comparison, the reproducible adaptive-v9 commit build produced 38.30 FPS
on the same heavier gameplay state. The protected historical candidate reached
41.58 FPS on the earlier reference state.

## Manual validation

The clean v2 build was also tested interactively on the RG351V. Gameplay,
native-30-FPS intermissions, audio quality and the return to gameplay were
reported as correct. This completed the release-candidate validation.

Cross-game testing should still be expanded before making the cadence guard a
generic default for every Flycast profile. The protected adaptive-v9 binary and
tag remain the fallback.
