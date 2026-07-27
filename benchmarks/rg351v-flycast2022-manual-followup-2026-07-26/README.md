# RG351V final manual follow-up

The device directory is:

```text
/storage/retrorun-test/flycast2022-manual-followup
```

It was cleaned after the automated tests and contains exactly:

```text
retrorun
retrorun.cfg
flycast_libretro.so
```

There is no launcher script in the final directory. The direct Sonic command
is:

```sh
cd /storage/retrorun-test/flycast2022-manual-followup
./retrorun -c ./retrorun.cfg -d /storage/roms/bios \
  -s /storage/roms/dreamcast ./flycast_libretro.so \
  "/storage/roms/dreamcast/Sonic Adventure 2.cdi"
```

Perform the full DRM/EmulationStation reset before starting a manual run. The
normal frontend is used, not the fixed-frame benchmark frontend.

## Final defaults

```ini
retrorun_force_audio_multithread = false
reicast_texture_storage_reuse = enabled
reicast_palette_fog_storage_reuse = disabled
reicast_sh4_fpscr = disabled
```

`reicast_sh4_fpscr` enables direct dynarec compilation of the SH4
`LDS Rn,FPSCR` instruction. It is part of the same final core and requires a
content restart after changing it. Audio threading remains a RetroRun setting.

## Final binary

```text
SHA-256: 74d40ab1facbe0279ade9e1fa5a32c0ddc1f5d75a58b34fb72e7ae23c9ec1f50
Build ID: bdffd691bec2cb111548a80957de3ab5c3ccd514
Architecture: AArch64
```

The normal RetroRun binary on the device has SHA-256
`4787d67b0d236230b3824e08681ae5fda65a5813cf1ea0051ece08a6f6fa2ad5`.

## Corrected runtime smoke test

The final core completed two 300-frame Sonic tests with a full DRM reset before
each launch:

| Setting | Frames | Duration | Mean core time |
| --- | ---: | ---: | ---: |
| `reicast_sh4_fpscr = disabled` | 300 | 13.124 s | 27.491 ms |
| `reicast_sh4_fpscr = enabled` | 300 | 13.204 s | 27.631 ms |

Both runs loaded the compatible save and exited cleanly. No invalid operation,
GL error, debugbreak or load failure was found. This short smoke test validates
both runtime paths; the earlier three paired 600-frame measurements remain the
performance evidence for the small average FPS signal. The final corrected
logs are archived in `final-runtime-corrected-smoke.tar.gz`.

The directory also retains the earlier candidate configurations and logs as
development evidence. They are not present in the cleaned device directory.
