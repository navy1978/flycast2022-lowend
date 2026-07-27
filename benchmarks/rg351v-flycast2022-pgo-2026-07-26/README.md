# RG351V GCC PGO feasibility test

Date: 2026-07-26

## Hypothesis

GCC profile-guided optimization might improve common Flycast paths without
changing emulation behavior. The training set was intended to cover Sonic
Adventure 2, Marvel vs. Capcom 2 and Soul Calibur with the exact archived
RetroRun configuration.

## Environment

- device: RG351V / RK3326;
- build VM compiler: GCC 9.4.0 on AArch64;
- base command: `make platform=arm64 FORCE_GLES=1 HAVE_LTCG=0`;
- all low-end implementation switches disabled;
- generate flags: `-fprofile-generate`;
- full EmulationStation stop/start/stop DRM reset before every device run.

## Result

The generate build linked and loaded, but every device run stopped before the
first rendered frame with exit status 133. No `.gcda` file was produced.

GDB localized the fault to the generated SH4 dispatcher:

```text
SH4_TCB+868: ldr x0, [x2, x1, lsl #3]
ngen_mainloop
```

The first fault address was the intentionally protected FPCB page. Under the
normal instrumented run, the Flycast signal path did not resolve it and entered
`DEBUGBREAK`.

The following isolated feasibility attempts all reproduced the same failure:

- removing the unrelated legacy `-pg` flag;
- leaving naked AICA, ARM7 and AArch64 backend translation units
  uninstrumented;
- leaving the signal/memory fault path uninstrumented;
- leaving the SH4 dynarec driver uninstrumented;
- training with native mapped memory temporarily disabled.

The last test confirmed in its log that native mapped memory was disabled, but
still stopped at the same generated dispatcher instruction. This rules out DRM
reset and the 4 GB mapping itself as the cause.

## Decision

**Reject as infeasible with the current GCC 9/Flycast 2022 dynarec
architecture.** There is no valid training profile, so no profile-use build or
FPS claim was produced. All PGO-only source workarounds were removed after the
test.

The retained raw evidence is:

- `gdb-detail.txt`;
- `sonic-smoke-driver-safe.log`;
- `sonic-smoke-no-nvmem.log`.
