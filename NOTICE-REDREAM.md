# Redream-derived low-end AICA path

The optional reduced-fidelity AICA mixer in
`core/hw/aica/sgc_if.cpp` was derived from the design and processing order of
Redream's `src/guest/aica/aica.c` at revision
`ffb7302245ff40515cb9f0f0b0e233a4b39342d3`.

Redream copyright belongs to Anthony Pesch and the Redream contributors.
The referenced Redream source is licensed under GNU GPL version 3.

The Flycast implementation retains Flycast's channel decoding and state. It
adds a separately selectable path comprising `ChannelEx::StepLowend()` and
`AICA_SampleLowend32()`. That path deliberately omits envelopes, filters, LFO,
pan, DSP routing and CDDA to reduce host CPU work.

The original reicast/Flycast files are GPL-2.0-or-later. A combined
distribution of this fork containing the Redream-derived path is therefore
distributed under GPLv3. The complete GPLv3 text is present at
`core/deps/picotcp/LICENSE.GPLv3`.

Modified files directly involved in this path:

- `core/hw/aica/aica.cpp`
- `core/hw/aica/sgc_if.cpp`
- `core/hw/aica/sgc_if.h`
- `core/libretro/libretro.cpp`
- `core/libretro/libretro_core_options.h`
- `core/types.h`
