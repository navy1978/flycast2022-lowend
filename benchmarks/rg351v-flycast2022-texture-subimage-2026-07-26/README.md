# RG351V texture storage reuse experiment

Date: 2026-07-26

## Hypothesis

Marvel vs. Capcom 2 repeatedly redefines already allocated, same-size PAL4
textures with `glTexImage2D`. Replacing only compatible repeated definitions
with `glTexSubImage2D` should avoid Mali driver allocation work without
changing decoded pixels.

## Builds

- baseline SHA-256:
  `24ea8c843ab41bb6a6cd5e9ddaaa2f16105dbd67ccdf714cfff366982143d35a`
- candidate SHA-256:
  `8ddc6ea232b39e46bf9ca745a5ddcab89b34a71cb3f719b3885b601cb00e933c`
- candidate build ID:
  `0562cdba801a68251f0288527e67dbe25c67b0e5`
- profiled candidate SHA-256:
  `6a2051af8c5a36423ae79b32484c137908ceeba8ed8d3da563b26e6a47989d28`
- profiled candidate build ID:
  `74381dc25bb74ffae887430153522b915828b203`

Both performance builds have profiler, DIV1 and exact-source shadow disabled.
Only the candidate defines `FLYCAST_LOWEND_TEXTURE_SUBIMAGE`.

## Procedure

- AmberELEC RG351V, RK3326 Cortex-A35 and Mali-G31;
- exact `retrorun.cfg` SHA-256
  `31dfed80909c20fe30feb80d2c15079641c9559ff4740344f4bdeb069b52365b`;
- Marvel vs. Capcom 2 (USA), automatic state load and benchmark-only confirm
  input;
- five alternating baseline/candidate pairs;
- 30-second warm-up and 60-second measurement;
- fixed CPU 1.296 GHz, DMC 786 MHz and GPU 520 MHz during each run;
- full EmulationStation stop/start/wait/stop DRM reset before every run;
- same RetroRun frontend and resolved settings in all ten JSON files.

## Result

Mean FPS increased from 41.722 to 44.832 (+7.45%). Mean core time decreased
from 12.316 to 10.355 ms (-15.93%) and active-frame p95 decreased from 34.934
to 31.713 ms (-9.22%). Every pair improved both FPS and core time.

Audio underruns were similar (31 baseline, 34 candidate). Duplicated callback
frames were 20 baseline and 40 candidate; the candidate also completed more
game frames and reached different parts of the dynamic scene, so this counter
requires longer deterministic validation rather than being treated as either
a benefit or a regression.

The three-pair Sonic Adventure 2 guard was neutral within noise and did not
show an audio regression. Sonic and Soul Calibur smoke runs produced no crash,
assert, benchmark failure or GL error marker.

The candidate subsequently passed the user's manual visual and audible RG351V
tests. Decision: **Keep and enable by default**, with
`reicast_texture_storage_reuse = disabled` as the runtime opt-out. Raw JSON
and logs are stored beside this file; `results.csv` contains the
machine-readable summary.
