# RG351V Fast Depth validation

These runs isolate `reicast_fast_depth` on the same AArch64 core binary and
the same Sonic Adventure 2 automatic state (`MK-51117`).

Common conditions:

- RG351V, RK3326/Cortex-A35, Mali-G31 GLES 3.2 and AmberELEC;
- RetroRun GO2 backend;
- internal resolution 640x480;
- no frontend or core frameskip;
- threaded rendering enabled, synchronous rendering disabled;
- Texture Storage Reuse and the experimental low-end audio mixer enabled;
- complete DRM reset before every run;
- 600 core frames after warm-up.

| Variant | Seconds / 600 frames | Mean seconds | Mean FPS |
| --- | --- | ---: | ---: |
| Fast Depth disabled | 28.386, 27.879 | 28.1325 | 21.3276 |
| Fast Depth enabled | 22.391, 22.129 | 22.2600 | 26.9542 |

The mean rendered-FPS gain is **26.38%**. Every run reports 600 presented
frames with zero duplicated and zero skipped frames. Mean video callback time
fell from 17.282 ms to 7.913 ms.

The user's manual RG351V results were:

- Sonic Adventure 2: video and audio approved;
- Soul Calibur (`T1401D  50`): gameplay approved with small, limited visual
  imperfections;
- Dead or Alive 2 Europe RDC (`RDC-0149`): visible flashing with Fast Depth,
  while video was reported perfect when Fast Depth was disabled.

The final clean review build has SHA-256
`5c0832336979612697ffd9f23e85869861fe8cbed4822ecb9db54a48ca76d261`.
Its `sonic-final-review` smoke run loaded the same state, presented all 600
frames in 21.221 seconds (28.274 FPS), and again reported zero duplicated or
skipped frames.
