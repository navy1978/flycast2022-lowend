# RG351V Flycast 2022 profiler run

These files are the raw 15-second RetroRun results described in
`docs/PERFORMANCE_BASELINE.md`.

Each workload has a normal `baseline-drmreset-r2` JSON/log pair and a
compile-time `profile-drmreset-r2` JSON/log pair. These are the only retained
measurements: earlier files were discarded after an incomplete KMS/DRM reset
left the panel black.

The launcher forces a complete display handoff before every run:
EmulationStation stop, start, six-second modeset wait, stop, then a
three-second wait before RetroRun. The shared configuration uses the source
`reicast_*` prefix, the accurate renderer path, an eight-second warm-up, fixed
device frequencies during measurement, and automatic restoration on exit.

The saves and game images are not included. Results cannot be reproduced
without the same content and save-state scene.
