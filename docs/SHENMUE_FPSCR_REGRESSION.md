# Shenmue direct FPSCR regression

Date: 30 July 2026

Shenmue USA (`MK-51059`) consistently stalled at `Now Loading` with the
experimental direct `LDS Rn,FPSCR` opcode dispatch, even when the
`reicast_sh4_fpscr` option was disabled.

The RG351V comparison kept RetroRun, the CHD and core settings fixed:

- AmberELEC Flycast 2021 passed the transition.
- A clean upstream `4c293f3` build passed the transition.
- The low-end fork with all private options disabled stalled.
- Reverting the AICA changes did not fix the stall.
- Reverting the complete SH4/dynarec group fixed the stall.
- Reverting only the decoder/opcode group fixed the stall.
- Removing only the direct FPSCR handler fixed the stall.

The disabled handler called a custom fallback and terminated the compiled
block, so it was not semantically identical to the upstream null-handler
dispatch. The experiment had only small, noisy Sonic measurements and no
validated performance benefit. It was removed rather than hidden behind
another compatibility option.
