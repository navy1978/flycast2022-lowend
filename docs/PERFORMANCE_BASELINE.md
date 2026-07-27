# RG351V performance baseline

Date: 2026-07-26

This baseline is for investigation, not a release performance claim. The
accurate renderer path is used: all low-end renderer experiments are disabled
and the emulated SH4 clock remains at 200 MHz.

## Source and build

```text
Repository commit: 1ca39aea8599a5cacaff41a1ba44ed849b6f96ef
Base Flycast commit: 4c293f306bc16a265c2d768af5d0cea138426054
Build host: Ubuntu 20.04, AArch64
Compiler: gcc/g++ 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.2)
Linker: GNU ld 2.34
```

No `CFLAGS`, `CXXFLAGS`, `CPPFLAGS` or `LDFLAGS` environment overrides were
set. The target flags were supplied through `CPUFLAGS`; the Makefile adds its
normal release, GLES, libretro and dependency flags.

```sh
make clean
make platform=arm64 FORCE_GLES=1 HAVE_LTCG=0 LOWEND_PROFILING=0 AS=gcc \
  CPUFLAGS="-mcpu=cortex-a35+crc+fp+simd+crypto -mabi=lp64 \
  -mno-outline-atomics -fomit-frame-pointer -Ofast" -j4
```

`AS=gcc` is required with this toolchain because invoking GNU `as` directly
would pass it the compiler-only `-mno-outline-atomics` option.

The resulting baseline is an ELF64 little-endian AArch64 shared object:

```text
SHA-256: ea7f1158d28a39a1eafefa001c8b1e8fd36862009c641336812446d52180a72e
Build ID: 2dfa8991efdaa09f38778711b684aa64d4fc2525
```

This is the clean final rebuild after all profiler source changes. Searching
the normal binary for `LOWEND_PROFILE` returns zero matches, and the
compile-time-disabled macros emit no profiler calls. The profiling build must
still be treated as a separate diagnostic artifact.

The measured profiling build was produced with `LOWEND_PROFILING=1` and has:

```text
SHA-256: b70c14269834f76395288c32f73eee1875fa5298e101dbe039dcc6d6d620f773
Build ID: a8ac55a505d6046dfe43ce3dd442c9f144f7b095
```

## Target and frontend

```text
Device: Anbernic RG351V
SoC: RK3326
CPU: 4x Cortex-A35
GPU: Mali-G31, ARM r13p0 OpenGL ES 3.2
OS: AmberELEC pr-1028-dev-20250320_0944-c75e6e9
Kernel: Linux 4.4.189, AArch64
Frontend: RetroRun 3.1.2, GO2/DRM video and GO2/OpenAL audio
Display mode: 640x480; RGB565 EGL surface
```

During every measured window the launcher temporarily used:

```text
CPU governor/frequency: performance, 1296 MHz
DMC governor/frequency: userspace, 786 MHz
GPU governor/frequency: userspace, 520 MHz
```

It restored CPU `ondemand`, DMC `dmc_ondemand` and GPU `dmc_ondemand` after
every run, including failed runs. The temperature after the final run was
62.9 C.

## Frontend and core settings

The complete test configuration is preserved with the raw benchmark results.
The settings that most affect interpretation are:

```ini
retrorun_vsync = false
retrorun_loop_declared_fps = true
retrorun_frameskip = 0
retrorun_adaptive_frameskip = false
retrorun_force_video_multithread = false
retrorun_audio_backend = go2
retrorun_audio_stable_buffer = true
retrorun_go2_audio_stretch_percent = 0
reicast_internal_resolution = 640x480
reicast_cpu_mode = dynamic_recompiler
reicast_sh4clock = 200
reicast_alpha_sorting = per-strip (fast, least accurate)
reicast_threaded_rendering = enabled
reicast_frame_skipping = disabled
reicast_adjacent_state_elision = disabled
reicast_translucent_strip_merge = disabled
```

Benchmarks use an eight-second warm-up, load the existing automatic save
state, inject A then B only during warm-up, disable persistent writes, and
measure 15 seconds.

Before every run the launcher performs the complete RG351V KMS/DRM reset:
stop EmulationStation, start it, wait six seconds for its modeset, stop it,
then wait three seconds before starting RetroRun. A shorter
`restart`/three-second sequence produced a black panel even though RetroRun
reported presented frames, so those measurements were discarded. The final
six runs below used the complete sequence; Sonic was also visually confirmed
on the panel. A trap restarts EmulationStation on exit.

## Workloads and initial results

The existing save states provide three useful but not yet fully deterministic
workloads:

| Workload | Intended class | Baseline core frames | Profiled core frames | Baseline core avg | Profiled core avg |
| --- | --- | ---: | ---: | ---: | ---: |
| Sonic Adventure 2, product `MK-51117` | draw-call-heavy | 288 / 15.046 s | 279 / 15.011 s | 32.341 ms | 33.860 ms |
| Soul Calibur | CPU/dynarec and scene transitions | 601 / 15.000 s | 620 / 15.000 s | 17.182 ms | 16.374 ms |
| Marvel vs. Capcom 2 | texture activity | 630 / 15.006 s | 632 / 15.016 s | 12.656 ms | 12.197 ms |

Sonic is the cleanest current profiler-overhead comparison: this short pair
showed about 4.7% additional average core time. Soul Calibur has periodic
100 ms stalls and duplicated frames, while Marvel vs. Capcom 2 advances
through a dynamic scene; their baseline/profile differences must not be
interpreted as speedups or slowdowns.

These runs establish that the cores load, restore state, render, produce
audio, emit benchmark JSON, and exit cleanly. They do not replace manual
visual and audible correctness checks for every game. The retained raw files
are the `*-drmreset-r2.json` and `*-drmreset-r2.log` pairs.

## Repetition requirements

Before accepting an optimization:

1. replace or supplement the dynamic saves with fixed camera/input scenes;
2. run at least five alternating baseline/candidate measurements of 60 seconds;
3. record starting and ending temperature;
4. verify menus, transparency, audio and save-state behavior manually;
5. reject claims whose change is inside the run-to-run variance.
