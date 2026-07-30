#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */


struct retro_core_option_v2_category option_cats_us[] = {
   {
      "video",
      "Video",
      "Configure visual buffers & effects, display parameters, framerate/-skip and rendering/texture parameters."
   },
   {
      "input",
      "Input",
      "Configure controller & light gun settings."
   },
   {
      "vmu",
      "Visual Memory",
      "Configure settings related to the Visual Memory Units/Systems (VMU)."
   },
   {
      "hacks",
      "Emulation Hacks",
      "Configure different emulation hacks."
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {
#if ((FEAT_SHREC == DYNAREC_JIT && HOST_CPU == CPU_X86) || (HOST_CPU == CPU_ARM) || (HOST_CPU == CPU_ARM64) || (HOST_CPU == CPU_X64)) && defined(TARGET_NO_JIT)
   {/* TODO: needs explanation */
      CORE_OPTION_NAME "_cpu_mode",
      "CPU Mode (Restart Required)",
      NULL,
      "",
      NULL,
      NULL,
      {
#if (FEAT_SHREC == DYNAREC_JIT && HOST_CPU == CPU_X86) || (HOST_CPU == CPU_ARM) || (HOST_CPU == CPU_ARM64) || (HOST_CPU == CPU_X64)
         { "dynamic_recompiler", "Dynamic Recompiler" },
#endif
#ifdef TARGET_NO_JIT
         { "generic_recompiler", "Generic Recompiler" },
#endif
         { NULL, NULL },
      },
#if (FEAT_SHREC == DYNAREC_JIT && HOST_CPU == CPU_X86) || (HOST_CPU == CPU_ARM) || (HOST_CPU == CPU_ARM64) || (HOST_CPU == CPU_X64)
      "dynamic_recompiler",
#elif defined(TARGET_NO_JIT)
      "generic_recompiler",
#endif
   },
#endif
   {
      CORE_OPTION_NAME "_sh4clock",
      "SH4 CPU Under/Overclock",
      NULL,
      "Changes the emulated SH4 clock. Underclocking may reduce host CPU load on slow devices, but changes game timing. Use with caution.",
      NULL,
      "hacks",
      {
         { "50",  "50 MHz" },
         { "75",  "75 MHz" },
         { "100", "100 MHz" },
         { "120", "120 MHz" },
         { "140", "140 MHz" },
         { "160", "160 MHz" },
         { "180", "180 MHz" },
         { "200", "200 MHz (Default)" },
         { "220", "220 MHz" },
         { "240", "240 MHz" },
         { "260", "260 MHz" },
         { "280", "280 MHz" },
         { "300", "300 MHz" },
         { "350", "350 MHz" },
         { "400", "400 MHz" },
         { NULL, NULL },
      },
      "200",
   },
   {
      CORE_OPTION_NAME "_boot_to_bios",
      "Boot to BIOS (Restart Required)",
      NULL,
      "Boot directly into the Dreamcast BIOS menu.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_system",
      "System Type (Restart Required)",
      NULL,
      "",
      NULL,
      NULL,
      {
         { "auto",       "Auto" },
         { "dreamcast",  "Dreamcast" },
         { "naomi",      "NAOMI" },
         { "atomiswave", "Atomiswave" },
         { NULL, NULL },
      },
      "auto",
   },
   {
      CORE_OPTION_NAME "_hle_bios",
      "HLE BIOS",
      NULL,
      "Force use of high-level emulation BIOS.",
      NULL,
      NULL,
      {
         { "disabled",  NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "disabled",
   },
#if defined(HAVE_OIT) || defined(HAVE_VULKAN)
   {
      CORE_OPTION_NAME "_oit_abuffer_size",
      "Accumulation Pixel Buffer Size (Restart Required)",
      NULL,
      "Higher values might be required for higher resolutions to output correctly.",
      NULL,
      "video",
      {
         { "512MB", NULL },
         { "1GB",   NULL },
         { "2GB",   NULL },
         { "4GB",   NULL },
         { NULL, NULL },
      },
      "512MB",
   },
#endif
   {
      CORE_OPTION_NAME "_internal_resolution",
      "Internal Resolution (Restart Required)",
      NULL,
      "Modify rendering resolution.",
      NULL,
      "video",
      {
         { "320x240",    NULL },
         { "640x480",    NULL },
         { "800x600",    NULL },
         { "960x720",    NULL },
         { "1024x768",   NULL },
         { "1280x960",   NULL },
         { "1440x1080",  NULL },
         { "1600x1200",  NULL },
         { "1920x1440",  NULL },
         { "2560x1920",  NULL },
         { "2880x2160",  NULL },
         { "3200x2400",  NULL },
         { "3840x2880",  NULL },
         { "4480x3360",  NULL },
         { "5120x3840",  NULL },
         { "5760x4320",  NULL },
         { "6400x4800",  NULL },
         { "7040x5280",  NULL },
         { "7680x5760",  NULL },
         { "8320x6240",  NULL },
         { "8960x6720",  NULL },
         { "9600x7200",  NULL },
         { "10240x7680", NULL },
         { "10880x8160", NULL },
         { "11520x8640", NULL },
         { "12160x9120", NULL },
         { "12800x9600", NULL },
         { NULL, NULL },
      },
#ifdef LOW_RES
      "320x240",
#else
      "640x480",
#endif
   },
   {
      CORE_OPTION_NAME "_screen_rotation",
      "Screen Orientation",
      NULL,
      "",
      NULL,
      "video",
      {
         { "horizontal", "Horizontal" },
         { "vertical",   "Vertical" },
         { NULL, NULL },
      },
      "horizontal",
   },
   {/* TODO: needs explanation */
      CORE_OPTION_NAME "_alpha_sorting",
      "Alpha Sorting",
      NULL,
      "",
      NULL,
      NULL,
      {
         { "per-strip (fast, least accurate)", "Per-Strip (fast, least accurate)" },
         { "per-triangle (normal)",            "Per-Triangle (normal)" },
#if defined(HAVE_OIT) || defined(HAVE_VULKAN)
         { "per-pixel (accurate)",             "Per-Pixel (accurate, but slowest)" },
#endif
         { NULL, NULL },
      },
#if defined(LOW_END)
      "per-strip (fast, least accurate)",
#else
      "per-triangle (normal)",
#endif
   },
   {
      CORE_OPTION_NAME "_gdrom_fast_loading",
      "GD-ROM Fast Loading (inaccurate)",
      NULL,
      "Speeds up GD-ROM loading.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
#ifdef LOW_END
      "enabled",
#else
      "disabled",
#endif
   },
#if defined(FLYCAST_LOWEND_TEXTURE_SUBIMAGE)
   {
      CORE_OPTION_NAME "_texture_storage_reuse",
      "Texture Storage Reuse",
      NULL,
      "Reuses an existing compatible GLES texture allocation with "
      "glTexSubImage2D instead of redefining it with glTexImage2D. This "
      "improves texture-heavy games on low-end devices. Disable to restore "
      "the original texture upload path.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled",
   },
#endif
   {
      CORE_OPTION_NAME "_palette_fog_storage_reuse",
      "Palette/Fog Texture Storage Reuse (Experimental)",
      NULL,
      "Reuses the fixed-size GLES allocations used by the Dreamcast palette "
      "and fog lookup textures. This may reduce texture allocation overhead "
      "on low-end GPUs. Keep disabled unless the game has been visually "
      "verified.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_fast_depth",
      "Fast Depth Calculation (Inaccurate)",
      NULL,
      "Moves the GLES3 depth calculation from every fragment to the vertex "
      "shader, allowing the GPU to interpolate depth. This can substantially "
      "reduce fragment cost on low-end GPUs, but may cause z-fighting or "
      "incorrect surface intersections. Restart content after changing this "
      "option.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { "vertex_log", "Vertex Logarithmic" },
         { "vertex_fast_log", "Vertex Fast Logarithmic" },
         { "menu_guarded", "Vertex Fast Logarithmic + Menu Guard (Aggressive)" },
         { "menu_guarded_shadow_safe", "Vertex Fast Logarithmic + Menu/Shadow Guard" },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_adjacent_state_elision",
      "Adjacent Render-State Elision (Experimental)",
      NULL,
      "Skips redundant GLES state setup for adjacent polygons with an exact "
      "state match. This is visually validated on the current test matrix, "
      "but measured performance gains are workload-dependent and may be "
      "negligible. Keep disabled unless testing.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_translucent_strip_merge",
      "Translucent Strip Merge (Experimental)",
      NULL,
      "Reorders and merges compatible translucent strips to reduce GLES draw "
      "calls on low-end devices. This can improve performance substantially, "
      "but may break menus, transparency, or rendering order in some games. "
      "Menu Guard keeps likely 2D menu/overlay strips as separate draws. "
      "Aggressive preserves the previous fastest inaccurate behavior. "
      "Disabled preserves the original accurate path.",
      NULL,
      "video",
      {
         { "disabled",     NULL },
         { "menu_guarded", "Menu Guard" },
         { "inaccurate",   "Aggressive (Inaccurate)" },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_opaque_strip_merge",
      "Opaque Strip Merge (Experimental)",
      NULL,
      "Groups opaque strips with identical render state before index "
      "generation so they can share one GLES draw submission. This can "
      "substantially reduce driver overhead on draw-call-heavy games, but "
      "may change the result of equal-depth geometry. Keep disabled unless "
      "the game has been visually verified.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_translucent_menu_guard_strategy",
      "Menu Guard Strategy",
      NULL,
      "Controls which translucent strips Menu Guard keeps as separate draws. "
      "Scored combines several menu-like signals. Flat protects every short "
      "constant-depth strip. All Short is the broadest diagnostic setting. "
      "Top HUD Last keeps wide geometry confined to the upper HUD band in "
      "submission order and draws it after sorted world transparency. HUD "
      "Last additionally protects narrow vertical gauges.",
      NULL,
      "video",
      {
         { "scored",    "Scored Heuristic" },
         { "flat",      "All Flat Short Strips" },
         { "all_short", "All Short Strips" },
         { "top_hud_last", "Top HUD Last (Experimental)" },
         { "hud_last", "Top + Vertical HUD Last (Experimental)" },
         { NULL, NULL },
      },
      "scored",
   },
   {
      CORE_OPTION_NAME "_translucent_menu_guard_max_vertices",
      "Menu Guard Maximum Vertices",
      NULL,
      "Maximum source vertex count considered by Menu Guard. Raise this when "
      "a menu is built from longer strips. Higher values can reduce the "
      "performance benefit of translucent strip merging.",
      NULL,
      "video",
      {
         { "4",  NULL },
         { "8",  "8 (Default)" },
         { "16", NULL },
         { "32", NULL },
         { "64", NULL },
         { NULL, NULL },
      },
      "8",
   },
   {
      CORE_OPTION_NAME "_translucent_menu_guard_risk",
      "Menu Guard Risk Threshold",
      NULL,
      "Minimum heuristic risk score protected by the Scored strategy. Lower "
      "values recognize more possible menu strips but preserve more draw "
      "boundaries. Ignored by Flat and All Short strategies.",
      NULL,
      "video",
      {
         { "3", NULL },
         { "4", NULL },
         { "5", "5 (Default)" },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { NULL, NULL },
      },
      "5",
   },
   {
      CORE_OPTION_NAME "_translucent_menu_guard_depth_tolerance",
      "Menu Guard Flat-Depth Tolerance",
      NULL,
      "Relative depth range considered flat. Raise this when transformed menu "
      "geometry is not perfectly coplanar. Larger values can also classify "
      "gameplay effects as menu-like.",
      NULL,
      "video",
      {
         { "0.000001", "0.000001" },
         { "0.00001",  "0.00001" },
         { "0.0001",   "0.0001 (Default)" },
         { "0.001",    "0.001" },
         { "0.01",     "0.01" },
         { NULL, NULL },
      },
      "0.0001",
   },
   {
      CORE_OPTION_NAME "_translucent_menu_guard_overlap",
      "Menu Guard Overlap Protection",
      NULL,
      "Controls whether overlapping screen-space bounds prevent a merge. "
      "Risky requires at least one menu-like strip. All protects every "
      "overlapping candidate pair.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "risky",    "Risky Only (Default)" },
         { "all",      "All Overlaps" },
         { NULL, NULL },
      },
      "risky",
   },
   {
      CORE_OPTION_NAME "_translucent_menu_guard_draw_sorting",
      "Menu Guard Draw Sorting",
      NULL,
      "Controls translucent draw submission after Menu Guard preprocessing. "
      "Standard follows the selected core alpha-sort mode. Per-Triangle keeps "
      "the guarded strip preprocessing but uses the per-triangle draw path; "
      "this can restore menus that remain broken with geometric detection, "
      "at an additional CPU cost.",
      NULL,
      "video",
      {
         { "standard",     "Standard (Default)" },
         { "per_triangle", "Per-Triangle Compatibility" },
         { NULL, NULL },
      },
      "standard",
   },
   {/* TODO: needs explanation */
      CORE_OPTION_NAME "_mipmapping",
      "Mipmapping",
      NULL,
      "",
      NULL,
      "video",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled",
   },
   {
      CORE_OPTION_NAME "_fog",
      "Fog Effects",
      NULL,
      "",
      NULL,
      "video",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled",
   },
   {
      CORE_OPTION_NAME "_volume_modifier_enable",
      "Volume Modifier",
      NULL,
      "A Dreamcast GPU feature that is typically used by games to draw object shadows. This should normally be enabled - the performance impact is usually minimal to negligible.",
      NULL,
      "video",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled",
   },
   {
      CORE_OPTION_NAME "_widescreen_hack",
      "Widescreen Hack (Restart Required)",
      NULL,
      "",
      NULL,
      "hacks",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_widescreen_cheats",
      "Widescreen Cheats (Restart Required)",
      NULL,
      "Activates cheats that allow certain games to display in widescreen format.",
      NULL,
      "hacks",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_cable_type",
      "Cable Type",
      NULL,
      "The output signal type. 'TV (Composite)' is the most widely supported.",
      NULL,
      "video",
      {
         { "TV (RGB)",       NULL },
         { "TV (Composite)", NULL },
         { "VGA (RGB)",      NULL },
         { NULL, NULL },
      },
#ifdef LOW_END
      "VGA (RGB)",
#else
      "TV (Composite)",
#endif
   },
   {
      CORE_OPTION_NAME "_broadcast",
      "Broadcast Standard",
      NULL,
      "",
      NULL,
      "video",
      {
         { "Default", NULL },
         { "PAL_M",   "PAL-M (Brazil)" },
         { "PAL_N",   "PAL-N (Argentina, Paraguay, Uruguay)" },
         { "NTSC",    NULL },
         { "PAL",     "PAL (World)" },
         { NULL, NULL },
      },
      "Default",
   },
   {
      CORE_OPTION_NAME "_framerate",
      "Framerate",
      NULL,
      "Affects how emulator interacts with frontend. 'Full Speed' - emulator returns control to RetroArch each time a frame has been rendered. 'Normal' - emulator returns control to RetroArch each time a V-blank interrupt is generated. 'Full Speed' should be used in most cases. 'Normal' may improve frame pacing on some systems, but can cause unresponsive input when the screen is static (e.g. loading/pause screens). Note: This setting only applies when 'Threaded Rendering' is disabled.",
      NULL,
      "video",
      {
         { "fullspeed", "Full Speed" },
         { "normal",    "Normal" },
         { NULL, NULL },
      },
      "fullspeed",
   },
   {
      CORE_OPTION_NAME "_region",
      "Region",
      NULL,
      "",
      NULL,
      NULL,
      {
         { "Default", NULL },
         { "Japan",   NULL },
         { "USA",     NULL },
         { "Europe",  NULL },
         { NULL, NULL },
      },
      "Default",
   },
   {
      CORE_OPTION_NAME "_language",
      "Language",
      NULL,
      "",
      NULL,
      NULL,
      {
         { "Default",  NULL },
         { "Japanese", NULL },
         { "English",  NULL },
         { "German",   NULL },
         { "French",   NULL },
         { "Spanish",  NULL },
         { "Italian",  NULL },
         { NULL, NULL },
      },
      "Default",
   },
   {
      CORE_OPTION_NAME "_div_matching",
      "DIV Matching",
      NULL,
      "Optimize integer division",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "auto",     "Auto" },
         { NULL, NULL },
      },
      "auto",
   },
   {
      CORE_OPTION_NAME "_force_wince",
      "Force Windows CE Mode",
      NULL,
      "Enable full MMU (Memory Management Unit) emulation and other settings for Windows CE games.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_analog_stick_deadzone",
      "Analog Stick Deadzone",
      NULL,
      "",
      NULL,
      "input",
      {
         { "0%",  NULL },
         { "5%",  NULL },
         { "10%", NULL },
         { "15%", NULL },
         { "20%", NULL },
         { "25%", NULL },
         { "30%", NULL },
         { NULL, NULL },
      },
      "15%",
   },
   {
      CORE_OPTION_NAME "_trigger_deadzone",
      "Trigger Deadzone",
      NULL,
      "",
      NULL,
      "input",
      {
         { "0%",  NULL },
         { "5%",  NULL },
         { "10%", NULL },
         { "15%", NULL },
         { "20%", NULL },
         { "25%", NULL },
         { "30%", NULL },
         { NULL, NULL },
      },
      "0%",
   },
   {
      CORE_OPTION_NAME "_digital_triggers",
      "Digital Triggers",
      NULL,
      "",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_enable_dsp",
      "Enable DSP",
      NULL,
      "Enable emulation of the Dreamcast's audio DSP (digital signal processor). Improves the accuracy of generated sound, but increases performance requirements.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
#ifdef LOW_END
      "disabled",
#else
      "enabled",
#endif
   },
   {
      CORE_OPTION_NAME "_audio_mixer",
      "Audio Mixer",
      NULL,
      "Accurate uses Flycast's optimized AICA mixer. Low-end Simplified uses "
      "a reduced frame-major mixer that omits envelopes, filters, LFO, pan, "
      "DSP routing and CD audio. It is deliberately inaccurate but improves "
      "performance on slow handhelds. DSP must be disabled for the simplified "
      "path.",
      NULL,
      "audio",
      {
         { "accurate", "Accurate (Flycast)" },
         { "lowend",   "Low-end Simplified (Inaccurate)" },
         { NULL, NULL },
      },
      "accurate",
   },
   {
      CORE_OPTION_NAME "_aica_arm_cycles",
      "AICA ARM7 Cycles per Sample (Experimental)",
      NULL,
      "Underclocks only the emulated AICA ARM7 sound CPU while keeping output "
      "at 44.1 kHz. Lower values can reduce host CPU usage, but may delay or "
      "break music, effects, channel updates, or sound-driver timing. The "
      "Dreamcast-accurate value is 32.",
      NULL,
      "audio",
      {
         { "32", "32 (Accurate)" },
         { "24", "24 (75%)" },
         { "16", "16 (50%)" },
         { "12", "12 (37.5%)" },
         { "8",  "8 (25%)" },
         { NULL, NULL },
      },
      "32",
   },
   {
      CORE_OPTION_NAME "_anisotropic_filtering",
      "Anisotropic Filtering",
      NULL,
      "Enhance the quality of textures on surfaces that are at oblique viewing angles with respect to the camera.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "2",  NULL },
         { "4",  NULL },
         { "8",  NULL },
         { "16",  NULL },
         { NULL, NULL },
      },
      "4",
   },
   {
      CORE_OPTION_NAME "_pvr2_filtering",
      "PowerVR2 Post-processing Filter",
      NULL,
      "Post-process the rendered image to simulate effects specific to the PowerVR2 GPU and analog video signals.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
#ifdef HAVE_TEXUPSCALE
   {
      CORE_OPTION_NAME "_texupscale",
      "Texture Upscaling (xBRZ)",
      NULL,
      "Enhance hand-drawn 2D pixel art graphics. Should only be used with 2D pixelated games.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "2x",  NULL },
         { "4x",  NULL },
         { "6x",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {/* TODO: needs clarification */
      CORE_OPTION_NAME "_texupscale_max_filtered_texture_size",
      "Texture Upscaling Max. Filtered Size",
      NULL,
      "",
      NULL,
      "video",
      {
         { "256",  NULL },
         { "512",  NULL },
         { "1024", NULL },
         { NULL, NULL },
      },
      "256",
   },
#endif
   {/* TODO: needs explanation */
      CORE_OPTION_NAME "_enable_rttb",
      "Enable RTT (Render To Texture) Buffer",
      NULL,
      "",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_render_to_texture_upscaling",
      "Render To Texture Upscaling",
      NULL,
      "",
      NULL,
      "video",
      {
         { "1x", NULL },
         { "2x", NULL },
         { "3x", NULL },
         { "4x", NULL },
         { "8x", NULL },
         { NULL, NULL },
      },
      "1x",
   },
#if !defined(TARGET_NO_THREADS)
   {
      CORE_OPTION_NAME "_threaded_rendering",
      "Threaded Rendering (Restart Required)",
      NULL,
      "Runs the GPU and CPU on different threads. Highly recommended.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled",
   },
   {
      CORE_OPTION_NAME "_synchronous_rendering",
      "Synchronous Rendering",
      NULL,
      "Waits for the GPU to finish rendering the previous frame instead of dropping the current one. Note: This setting only applies when 'Threaded Rendering' is enabled.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
#ifdef LOW_END
      "disabled",
#else
      "enabled",
#endif
   },
   {
      CORE_OPTION_NAME "_delay_frame_swapping",
      "Delay Frame Swapping",
      NULL,
      "Useful to avoid flashing screens or glitchy videos. Not recommended on slow platforms. Note: This setting only applies when 'Threaded Rendering' is enabled.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
#endif
   {
      CORE_OPTION_NAME "_frame_skipping",
      "Frame Skipping",
      NULL,
      "Sets the number of frames to skip between each displayed frame.",
      NULL,
      "video",
      {
         { "disabled",  NULL },
         { "1",         NULL },
         { "2",         NULL },
         { "3",         NULL },
         { "4",         NULL },
         { "5",         NULL },
         { "6",         NULL },
         { "adaptive",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_enable_purupuru",
      "Purupuru Pack/Vibration Pack",
      NULL,
      "Enables controller force feedback.",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled",
   },
   {
      CORE_OPTION_NAME "_allow_service_buttons",
      "Allow NAOMI Service Buttons",
      NULL,
      "Enables SERVICE button for NAOMI, to enter cabinet settings.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_enable_naomi_15khz_dipswitch",
      "Enable NAOMI 15KHz DIP switch",
      NULL,
      "This can force display in 240p, 480i or have no effect at all depending on the game.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {/* TODO: needs explanation */
      CORE_OPTION_NAME "_custom_textures",
      "Load Custom Textures",
      NULL,
      "",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {/* TODO: probably needs explanation */
      CORE_OPTION_NAME "_dump_textures",
      "Dump Textures",
      NULL,
      "",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_per_content_vmus",
      "Per-Game Visual Memory Units/Systems (VMU)",
      "Per-Game VMUs",
      "When disabled, all games share 4 VMU save files (A1, B1, C1, D1) located in RetroArch's system directory. The 'VMU A1' setting creates a unique VMU 'A1' file in RetroArch's save directory for each game that is launched. The 'All VMUs' setting creates 4 unique VMU files (A1, B1, C1, D1) for each game that is launched.",
      NULL,
      "vmu",
      {
         { "disabled", NULL },
         { "VMU A1",   NULL },
         { "All VMUs", NULL },
         { NULL, NULL},
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_show_vmu_screen_settings",
      "Show Visual Memory Unit/System (VMU) Display Settings",
      "Show VMU Display Settings",
      "Enable configuration of emulated VMU LCD screen visibility, size, position and color. NOTE: Quick Menu must be toggled for this setting to take effect.",
      NULL,
      "vmu",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      CORE_OPTION_NAME "_vmu1_screen_display",
      "VMU Screen 1 Display",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_vmu1_screen_position",
      "VMU Screen 1 Position",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "Upper Left",  NULL },
         { "Upper Right", NULL },
         { "Lower Left",  NULL },
         { "Lower Right", NULL },
         { NULL, NULL },
      },
      "Upper Left",
   },
   {
      CORE_OPTION_NAME "_vmu1_screen_size_mult",
      "VMU Screen 1 Size",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "1x", NULL },
         { "2x", NULL },
         { "3x", NULL },
         { "4x", NULL },
         { "5x", NULL },
         { NULL, NULL },
      },
      "1x",
   },
   {
      CORE_OPTION_NAME "_vmu1_pixel_on_color",
      "VMU Screen 1 Pixel On Color",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "DEFAULT_ON 00",  "Default ON" },
         { "DEFAULT_OFF 01", "Default OFF" },
         { "BLACK 02",          "Black" },
         { "BLUE 03",           "Blue" },
         { "LIGHT_BLUE 04",     "Light Blue" },
         { "GREEN 05",          "Green" },
         { "CYAN 06",           "Cyan" },
         { "CYAN_BLUE 07",      "Cyan Blue" },
         { "LIGHT_GREEN 08",    "Light Green" },
         { "CYAN_GREEN 09",     "Cyan Green" },
         { "LIGHT_CYAN 10",     "Light Cyan" },
         { "RED 11",            "Red" },
         { "PURPLE 12",         "Purple" },
         { "LIGHT_PURPLE 13",   "Light Purple" },
         { "YELLOW 14",         "Yellow" },
         { "GRAY 15",           "Gray" },
         { "LIGHT_PURPLE_2 16", "Light Purple (2)" },
         { "LIGHT_GREEN_2 17",  "Light Green (2)" },
         { "LIGHT_GREEN_3 18",  "Light Green (3)" },
         { "LIGHT_CYAN_2 19",   "Light Cyan (2)" },
         { "LIGHT_RED_2 20",    "Light Red (2)" },
         { "MAGENTA 21",        "Magenta" },
         { "LIGHT_PURPLE_3 22",   "Light Purple (3)" },
         { "LIGHT_ORANGE 23",   "Light Orange" },
         { "ORANGE 24",         "Orange" },
         { "LIGHT_PURPLE_4 25", "Light Purple (4)" },
         { "LIGHT_YELLOW 26",   "Light Yellow" },
         { "LIGHT_YELLOW_2 27", "Light Yellow (2)" },
         { "WHITE 28",          "White" },
         { NULL, NULL },
      },
      "DEFAULT_ON 00",
   },
   {
      CORE_OPTION_NAME "_vmu1_pixel_off_color",
      "VMU Screen 1 Pixel Off Color",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "DEFAULT_OFF 01", "Default OFF" },
         { "DEFAULT_ON 00",  "Default ON" },
         { "BLACK 02",          "Black" },
         { "BLUE 03",           "Blue" },
         { "LIGHT_BLUE 04",     "Light Blue" },
         { "GREEN 05",          "Green" },
         { "CYAN 06",           "Cyan" },
         { "CYAN_BLUE 07",      "Cyan Blue" },
         { "LIGHT_GREEN 08",    "Light Green" },
         { "CYAN_GREEN 09",     "Cyan Green" },
         { "LIGHT_CYAN 10",     "Light Cyan" },
         { "RED 11",            "Red" },
         { "PURPLE 12",         "Purple" },
         { "LIGHT_PURPLE 13",   "Light Purple" },
         { "YELLOW 14",         "Yellow" },
         { "GRAY 15",           "Gray" },
         { "LIGHT_PURPLE_2 16", "Light Purple (2)" },
         { "LIGHT_GREEN_2 17",  "Light Green (2)" },
         { "LIGHT_GREEN_3 18",  "Light Green (3)" },
         { "LIGHT_CYAN_2 19",   "Light Cyan (2)" },
         { "LIGHT_RED_2 20",    "Light Red (2)" },
         { "MAGENTA 21",        "Magenta" },
         { "LIGHT_PURPLE_3 22",   "Light Purple (3)" },
         { "LIGHT_ORANGE 23",   "Light Orange" },
         { "ORANGE 24",         "Orange" },
         { "LIGHT_PURPLE_4 25", "Light Purple (4)" },
         { "LIGHT_YELLOW 26",   "Light Yellow" },
         { "LIGHT_YELLOW_2 27", "Light Yellow (2)" },
         { "WHITE 28",          "White" },
         { NULL, NULL },
      },
      "DEFAULT_OFF 01",
   },
   {
      CORE_OPTION_NAME "_vmu1_screen_opacity",
      "VMU Screen 1 Opacity",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "10%",  NULL },
         { "20%",  NULL },
         { "30%",  NULL },
         { "40%",  NULL },
         { "50%",  NULL },
         { "60%",  NULL },
         { "70%",  NULL },
         { "80%",  NULL },
         { "90%",  NULL },
         { "100%", NULL },
         { NULL,   NULL },
      },
      "100%",
   },
   {
      CORE_OPTION_NAME "_vmu2_screen_display",
      "VMU Screen 2 Display",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_vmu2_screen_position",
      "VMU Screen 2 Position",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "Upper Left",  NULL },
         { "Upper Right", NULL },
         { "Lower Left",  NULL },
         { "Lower Right", NULL },
         { NULL, NULL },
      },
      "Upper Left",
   },
   {
      CORE_OPTION_NAME "_vmu2_screen_size_mult",
      "VMU Screen 2 Size",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "1x", NULL },
         { "2x", NULL },
         { "3x", NULL },
         { "4x", NULL },
         { "5x", NULL },
         { NULL, NULL },
      },
      "1x",
   },
   {
      CORE_OPTION_NAME "_vmu2_pixel_on_color",
      "VMU Screen 2 Pixel On Color",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "DEFAULT_ON 00",  "Default ON" },
         { "DEFAULT_OFF 01", "Default OFF" },
         { "BLACK 02",          "Black" },
         { "BLUE 03",           "Blue" },
         { "LIGHT_BLUE 04",     "Light Blue" },
         { "GREEN 05",          "Green" },
         { "CYAN 06",           "Cyan" },
         { "CYAN_BLUE 07",      "Cyan Blue" },
         { "LIGHT_GREEN 08",    "Light Green" },
         { "CYAN_GREEN 09",     "Cyan Green" },
         { "LIGHT_CYAN 10",     "Light Cyan" },
         { "RED 11",            "Red" },
         { "PURPLE 12",         "Purple" },
         { "LIGHT_PURPLE 13",   "Light Purple" },
         { "YELLOW 14",         "Yellow" },
         { "GRAY 15",           "Gray" },
         { "LIGHT_PURPLE_2 16", "Light Purple (2)" },
         { "LIGHT_GREEN_2 17",  "Light Green (2)" },
         { "LIGHT_GREEN_3 18",  "Light Green (3)" },
         { "LIGHT_CYAN_2 19",   "Light Cyan (2)" },
         { "LIGHT_RED_2 20",    "Light Red (2)" },
         { "MAGENTA 21",        "Magenta" },
         { "LIGHT_PURPLE_3 22",   "Light Purple (3)" },
         { "LIGHT_ORANGE 23",   "Light Orange" },
         { "ORANGE 24",         "Orange" },
         { "LIGHT_PURPLE_4 25", "Light Purple (4)" },
         { "LIGHT_YELLOW 26",   "Light Yellow" },
         { "LIGHT_YELLOW_2 27", "Light Yellow (2)" },
         { "WHITE 28",          "White" },
         { NULL, NULL },
      },
      "DEFAULT_ON 00",
   },
   {
      CORE_OPTION_NAME "_vmu2_pixel_off_color",
      "VMU Screen 2 Pixel Off Color",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "DEFAULT_OFF 01", "Default OFF" },
         { "DEFAULT_ON 00",  "Default ON" },
         { "BLACK 02",          "Black" },
         { "BLUE 03",           "Blue" },
         { "LIGHT_BLUE 04",     "Light Blue" },
         { "GREEN 05",          "Green" },
         { "CYAN 06",           "Cyan" },
         { "CYAN_BLUE 07",      "Cyan Blue" },
         { "LIGHT_GREEN 08",    "Light Green" },
         { "CYAN_GREEN 09",     "Cyan Green" },
         { "LIGHT_CYAN 10",     "Light Cyan" },
         { "RED 11",            "Red" },
         { "PURPLE 12",         "Purple" },
         { "LIGHT_PURPLE 13",   "Light Purple" },
         { "YELLOW 14",         "Yellow" },
         { "GRAY 15",           "Gray" },
         { "LIGHT_PURPLE_2 16", "Light Purple (2)" },
         { "LIGHT_GREEN_2 17",  "Light Green (2)" },
         { "LIGHT_GREEN_3 18",  "Light Green (3)" },
         { "LIGHT_CYAN_2 19",   "Light Cyan (2)" },
         { "LIGHT_RED_2 20",    "Light Red (2)" },
         { "MAGENTA 21",        "Magenta" },
         { "LIGHT_PURPLE_3 22",   "Light Purple (3)" },
         { "LIGHT_ORANGE 23",   "Light Orange" },
         { "ORANGE 24",         "Orange" },
         { "LIGHT_PURPLE_4 25", "Light Purple (4)" },
         { "LIGHT_YELLOW 26",   "Light Yellow" },
         { "LIGHT_YELLOW_2 27", "Light Yellow (2)" },
         { "WHITE 28",          "White" },
         { NULL, NULL },
      },
      "DEFAULT_OFF 01",
   },
   {
      CORE_OPTION_NAME "_vmu2_screen_opacity",
      "VMU Screen 2 Opacity",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "10%",  NULL },
         { "20%",  NULL },
         { "30%",  NULL },
         { "40%",  NULL },
         { "50%",  NULL },
         { "60%",  NULL },
         { "70%",  NULL },
         { "80%",  NULL },
         { "90%",  NULL },
         { "100%", NULL },
         { NULL,   NULL },
      },
      "100%",
   },
   {
      CORE_OPTION_NAME "_vmu3_screen_display",
      "VMU Screen 3 Display",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_vmu3_screen_position",
      "VMU Screen 3 Position",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "Upper Left",  NULL },
         { "Upper Right", NULL },
         { "Lower Left",  NULL },
         { "Lower Right", NULL },
         { NULL, NULL },
      },
      "Upper Left",
   },
   {
      CORE_OPTION_NAME "_vmu3_screen_size_mult",
      "VMU Screen 3 Size",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "1x", NULL },
         { "2x", NULL },
         { "3x", NULL },
         { "4x", NULL },
         { "5x", NULL },
         { NULL, NULL },
      },
      "1x",
   },
   {
      CORE_OPTION_NAME "_vmu3_pixel_on_color",
      "VMU Screen 3 Pixel On Color",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "DEFAULT_ON 00",  "Default ON" },
         { "DEFAULT_OFF 01", "Default OFF" },
         { "BLACK 02",          "Black" },
         { "BLUE 03",           "Blue" },
         { "LIGHT_BLUE 04",     "Light Blue" },
         { "GREEN 05",          "Green" },
         { "CYAN 06",           "Cyan" },
         { "CYAN_BLUE 07",      "Cyan Blue" },
         { "LIGHT_GREEN 08",    "Light Green" },
         { "CYAN_GREEN 09",     "Cyan Green" },
         { "LIGHT_CYAN 10",     "Light Cyan" },
         { "RED 11",            "Red" },
         { "PURPLE 12",         "Purple" },
         { "LIGHT_PURPLE 13",   "Light Purple" },
         { "YELLOW 14",         "Yellow" },
         { "GRAY 15",           "Gray" },
         { "LIGHT_PURPLE_2 16", "Light Purple (2)" },
         { "LIGHT_GREEN_2 17",  "Light Green (2)" },
         { "LIGHT_GREEN_3 18",  "Light Green (3)" },
         { "LIGHT_CYAN_2 19",   "Light Cyan (2)" },
         { "LIGHT_RED_2 20",    "Light Red (2)" },
         { "MAGENTA 21",        "Magenta" },
         { "LIGHT_PURPLE_3 22",   "Light Purple (3)" },
         { "LIGHT_ORANGE 23",   "Light Orange" },
         { "ORANGE 24",         "Orange" },
         { "LIGHT_PURPLE_4 25", "Light Purple (4)" },
         { "LIGHT_YELLOW 26",   "Light Yellow" },
         { "LIGHT_YELLOW_2 27", "Light Yellow (2)" },
         { "WHITE 28",          "White" },
         { NULL, NULL },
      },
      "DEFAULT_ON 00",
   },
   {
      CORE_OPTION_NAME "_vmu3_pixel_off_color",
      "VMU Screen 3 Pixel Off Color",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "DEFAULT_OFF 01", "Default OFF" },
         { "DEFAULT_ON 00",  "Default ON" },
         { "BLACK 02",          "Black" },
         { "BLUE 03",           "Blue" },
         { "LIGHT_BLUE 04",     "Light Blue" },
         { "GREEN 05",          "Green" },
         { "CYAN 06",           "Cyan" },
         { "CYAN_BLUE 07",      "Cyan Blue" },
         { "LIGHT_GREEN 08",    "Light Green" },
         { "CYAN_GREEN 09",     "Cyan Green" },
         { "LIGHT_CYAN 10",     "Light Cyan" },
         { "RED 11",            "Red" },
         { "PURPLE 12",         "Purple" },
         { "LIGHT_PURPLE 13",   "Light Purple" },
         { "YELLOW 14",         "Yellow" },
         { "GRAY 15",           "Gray" },
         { "LIGHT_PURPLE_2 16", "Light Purple (2)" },
         { "LIGHT_GREEN_2 17",  "Light Green (2)" },
         { "LIGHT_GREEN_3 18",  "Light Green (3)" },
         { "LIGHT_CYAN_2 19",   "Light Cyan (2)" },
         { "LIGHT_RED_2 20",    "Light Red (2)" },
         { "MAGENTA 21",        "Magenta" },
         { "LIGHT_PURPLE_3 22",   "Light Purple (3)" },
         { "LIGHT_ORANGE 23",   "Light Orange" },
         { "ORANGE 24",         "Orange" },
         { "LIGHT_PURPLE_4 25", "Light Purple (4)" },
         { "LIGHT_YELLOW 26",   "Light Yellow" },
         { "LIGHT_YELLOW_2 27", "Light Yellow (2)" },
         { "WHITE 28",          "White" },
         { NULL, NULL },
      },
      "DEFAULT_OFF 01",
   },
   {
      CORE_OPTION_NAME "_vmu3_screen_opacity",
      "VMU Screen 3 Opacity",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "10%",  NULL },
         { "20%",  NULL },
         { "30%",  NULL },
         { "40%",  NULL },
         { "50%",  NULL },
         { "60%",  NULL },
         { "70%",  NULL },
         { "80%",  NULL },
         { "90%",  NULL },
         { "100%", NULL },
         { NULL,   NULL },
      },
      "100%",
   },
   {
      CORE_OPTION_NAME "_vmu4_screen_display",
      "VMU Screen 4 Display",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_vmu4_screen_position",
      "VMU Screen 4 Position",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "Upper Left",  NULL },
         { "Upper Right", NULL },
         { "Lower Left",  NULL },
         { "Lower Right", NULL },
         { NULL, NULL },
      },
      "Upper Left",
   },
   {
      CORE_OPTION_NAME "_vmu4_screen_size_mult",
      "VMU Screen 4 Size",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "1x", NULL },
         { "2x", NULL },
         { "3x", NULL },
         { "4x", NULL },
         { "5x", NULL },
         { NULL, NULL },
      },
      "1x",
   },
   {
      CORE_OPTION_NAME "_vmu4_pixel_on_color",
      "VMU Screen 4 Pixel On Color",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "DEFAULT_ON 00",  "Default ON" },
         { "DEFAULT_OFF 01", "Default OFF" },
         { "BLACK 02",          "Black" },
         { "BLUE 03",           "Blue" },
         { "LIGHT_BLUE 04",     "Light Blue" },
         { "GREEN 05",          "Green" },
         { "CYAN 06",           "Cyan" },
         { "CYAN_BLUE 07",      "Cyan Blue" },
         { "LIGHT_GREEN 08",    "Light Green" },
         { "CYAN_GREEN 09",     "Cyan Green" },
         { "LIGHT_CYAN 10",     "Light Cyan" },
         { "RED 11",            "Red" },
         { "PURPLE 12",         "Purple" },
         { "LIGHT_PURPLE 13",   "Light Purple" },
         { "YELLOW 14",         "Yellow" },
         { "GRAY 15",           "Gray" },
         { "LIGHT_PURPLE_2 16", "Light Purple (2)" },
         { "LIGHT_GREEN_2 17",  "Light Green (2)" },
         { "LIGHT_GREEN_3 18",  "Light Green (3)" },
         { "LIGHT_CYAN_2 19",   "Light Cyan (2)" },
         { "LIGHT_RED_2 20",    "Light Red (2)" },
         { "MAGENTA 21",        "Magenta" },
         { "LIGHT_PURPLE_3 22",   "Light Purple (3)" },
         { "LIGHT_ORANGE 23",   "Light Orange" },
         { "ORANGE 24",         "Orange" },
         { "LIGHT_PURPLE_4 25", "Light Purple (4)" },
         { "LIGHT_YELLOW 26",   "Light Yellow" },
         { "LIGHT_YELLOW_2 27", "Light Yellow (2)" },
         { "WHITE 28",          "White" },
         { NULL, NULL },
      },
      "DEFAULT_ON 00",
   },
   {
      CORE_OPTION_NAME "_vmu4_pixel_off_color",
      "VMU Screen 4 Pixel Off Color",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "DEFAULT_OFF 01", "Default OFF" },
         { "DEFAULT_ON 00",  "Default ON" },
         { "BLACK 02",          "Black" },
         { "BLUE 03",           "Blue" },
         { "LIGHT_BLUE 04",     "Light Blue" },
         { "GREEN 05",          "Green" },
         { "CYAN 06",           "Cyan" },
         { "CYAN_BLUE 07",      "Cyan Blue" },
         { "LIGHT_GREEN 08",    "Light Green" },
         { "CYAN_GREEN 09",     "Cyan Green" },
         { "LIGHT_CYAN 10",     "Light Cyan" },
         { "RED 11",            "Red" },
         { "PURPLE 12",         "Purple" },
         { "LIGHT_PURPLE 13",   "Light Purple" },
         { "YELLOW 14",         "Yellow" },
         { "GRAY 15",           "Gray" },
         { "LIGHT_PURPLE_2 16", "Light Purple (2)" },
         { "LIGHT_GREEN_2 17",  "Light Green (2)" },
         { "LIGHT_GREEN_3 18",  "Light Green (3)" },
         { "LIGHT_CYAN_2 19",   "Light Cyan (2)" },
         { "LIGHT_RED_2 20",    "Light Red (2)" },
         { "MAGENTA 21",        "Magenta" },
         { "LIGHT_PURPLE_3 22",   "Light Purple (3)" },
         { "LIGHT_ORANGE 23",   "Light Orange" },
         { "ORANGE 24",         "Orange" },
         { "LIGHT_PURPLE_4 25", "Light Purple (4)" },
         { "LIGHT_YELLOW 26",   "Light Yellow" },
         { "LIGHT_YELLOW_2 27", "Light Yellow (2)" },
         { "WHITE 28",          "White" },
         { NULL, NULL },
      },
      "DEFAULT_OFF 01",
   },
   {
      CORE_OPTION_NAME "_vmu4_screen_opacity",
      "VMU Screen 4 Opacity",
      NULL,
      "",
      NULL,
      "vmu",
      {
         { "10%",  NULL },
         { "20%",  NULL },
         { "30%",  NULL },
         { "40%",  NULL },
         { "50%",  NULL },
         { "60%",  NULL },
         { "70%",  NULL },
         { "80%",  NULL },
         { "90%",  NULL },
         { "100%", NULL },
         { NULL,   NULL },
      },
      "100%",
   },
   {
      CORE_OPTION_NAME "_show_lightgun_settings",
      "Show Light Gun Settings",
      NULL,
      "Enable configuration of light gun crosshair display options. NOTE: Quick Menu must be toggled for this setting to take effect.",
      NULL,
      "input",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      CORE_OPTION_NAME "_lightgun1_crosshair",
      "Gun Crosshair 1 Display",
      NULL,
      "",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "White",    NULL },
         { "Red",      NULL },
         { "Green",    NULL },
         { "Blue",     NULL },
         { NULL,       NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_lightgun2_crosshair",
      "Gun Crosshair 2 Display",
      NULL,
      "",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "White",    NULL },
         { "Red",      NULL },
         { "Green",    NULL },
         { "Blue",     NULL },
         { NULL,       NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_lightgun3_crosshair",
      "Gun Crosshair 3 Display",
      NULL,
      "",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "White",    NULL },
         { "Red",      NULL },
         { "Green",    NULL },
         { "Blue",     NULL },
         { NULL,       NULL },
      },
      "disabled",
   },
   {
      CORE_OPTION_NAME "_lightgun4_crosshair",
      "Gun Crosshair 4 Display",
      NULL,
      "",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "White",    NULL },
         { "Red",      NULL },
         { "Green",    NULL },
         { "Blue",     NULL },
         { NULL,       NULL },
      },
      "disabled",
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_options_v2 *options_intl[RETRO_LANGUAGE_LAST] = {
   &options_us, /* RETRO_LANGUAGE_ENGLISH */
   &options_ja,      /* RETRO_LANGUAGE_JAPANESE */
   &options_fr,      /* RETRO_LANGUAGE_FRENCH */
   &options_es,      /* RETRO_LANGUAGE_SPANISH */
   &options_de,      /* RETRO_LANGUAGE_GERMAN */
   &options_it,      /* RETRO_LANGUAGE_ITALIAN */
   &options_nl,      /* RETRO_LANGUAGE_DUTCH */
   &options_pt_br,   /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   &options_pt_pt,   /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   &options_ru,      /* RETRO_LANGUAGE_RUSSIAN */
   &options_ko,      /* RETRO_LANGUAGE_KOREAN */
   &options_cht,     /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   &options_chs,     /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   &options_eo,      /* RETRO_LANGUAGE_ESPERANTO */
   &options_pl,      /* RETRO_LANGUAGE_POLISH */
   &options_vn,      /* RETRO_LANGUAGE_VIETNAMESE */
   &options_ar,      /* RETRO_LANGUAGE_ARABIC */
   &options_el,      /* RETRO_LANGUAGE_GREEK */
   &options_tr,      /* RETRO_LANGUAGE_TURKISH */
   &options_sv,      /* RETRO_LANGUAGE_SLOVAK */
   &options_fa,      /* RETRO_LANGUAGE_PERSIAN */
   &options_he,      /* RETRO_LANGUAGE_HEBREW */
   &options_ast,     /* RETRO_LANGUAGE_ASTURIAN */
   &options_fi,      /* RETRO_LANGUAGE_FINNISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb,
      bool *categories_supported)
{
   unsigned version  = 0;
#ifndef HAVE_NO_LANGEXTRA
   unsigned language = 0;
#endif

   if (!environ_cb || !categories_supported)
      return;

   *categories_supported = false;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_v2_intl core_options_intl;

      core_options_intl.us    = &options_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = options_intl[language];

      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL,
            &core_options_intl);
#else
      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
#endif
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
#ifndef HAVE_NO_LANGEXTRA
      size_t num_options_intl          = 0;
      struct retro_core_option_v2_definition
            *option_defs_intl          = NULL;
      struct retro_core_option_definition
            *option_v1_defs_intl       = NULL;
      struct retro_core_options_intl
            core_options_v1_intl;
#endif
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine total number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      if (version >= 1)
      {
         /* Allocate US array */
         option_v1_defs_us = (struct retro_core_option_definition *)
               calloc(num_options + 1, sizeof(struct retro_core_option_definition));

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
            struct retro_core_option_value *option_values         = option_def_us->values;
            struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
            struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

            option_v1_def_us->key           = option_def_us->key;
            option_v1_def_us->desc          = option_def_us->desc;
            option_v1_def_us->info          = option_def_us->info;
            option_v1_def_us->default_value = option_def_us->default_value;

            /* Values must be copied individually... */
            while (option_values->value)
            {
               option_v1_values->value = option_values->value;
               option_v1_values->label = option_values->label;

               option_values++;
               option_v1_values++;
            }
         }

#ifndef HAVE_NO_LANGEXTRA
         if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
             (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
             options_intl[language])
            option_defs_intl = options_intl[language]->definitions;

         if (option_defs_intl)
         {
            /* Determine number of intl options */
            while (true)
            {
               if (option_defs_intl[num_options_intl].key)
                  num_options_intl++;
               else
                  break;
            }

            /* Allocate intl array */
            option_v1_defs_intl = (struct retro_core_option_definition *)
                  calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_intl array */
            for (i = 0; i < num_options_intl; i++)
            {
               struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
               struct retro_core_option_value *option_values           = option_def_intl->values;
               struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
               struct retro_core_option_value *option_v1_values        = option_v1_def_intl->values;

               option_v1_def_intl->key           = option_def_intl->key;
               option_v1_def_intl->desc          = option_def_intl->desc;
               option_v1_def_intl->info          = option_def_intl->info;
               option_v1_def_intl->default_value = option_def_intl->default_value;

               /* Values must be copied individually... */
               while (option_values->value)
               {
                  option_v1_values->value = option_values->value;
                  option_v1_values->label = option_values->label;

                  option_values++;
                  option_v1_values++;
               }
            }
         }

         core_options_v1_intl.us    = option_v1_defs_us;
         core_options_v1_intl.local = option_v1_defs_intl;

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
      }
      else
      {
         /* Allocate arrays */
         variables  = (struct retro_variable *)calloc(num_options + 1,
               sizeof(struct retro_variable));
         values_buf = (char **)calloc(num_options, sizeof(char *));

         if (!variables || !values_buf)
            goto error;

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            const char *key                        = option_defs_us[i].key;
            const char *desc                       = option_defs_us[i].desc;
            const char *default_value              = option_defs_us[i].default_value;
            struct retro_core_option_value *values = option_defs_us[i].values;
            size_t buf_len                         = 3;
            size_t default_index                   = 0;

            values_buf[i] = NULL;

            if (desc)
            {
               size_t num_values = 0;

               /* Determine number of values */
               while (true)
               {
                  if (values[num_values].value)
                  {
                     /* Check if this is the default value */
                     if (default_value)
                        if (strcmp(values[num_values].value, default_value) == 0)
                           default_index = num_values;

                     buf_len += strlen(values[num_values].value);
                     num_values++;
                  }
                  else
                     break;
               }

               /* Build values string */
               if (num_values > 0)
               {
                  buf_len += num_values - 1;
                  buf_len += strlen(desc);

                  values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                  if (!values_buf[i])
                     goto error;

                  strcpy(values_buf[i], desc);
                  strcat(values_buf[i], "; ");

                  /* Default value goes first */
                  strcat(values_buf[i], values[default_index].value);

                  /* Add remaining values */
                  for (j = 0; j < num_values; j++)
                  {
                     if (j != default_index)
                     {
                        strcat(values_buf[i], "|");
                        strcat(values_buf[i], values[j].value);
                     }
                  }
               }
            }

            variables[option_index].key   = key;
            variables[option_index].value = values_buf[i];
            option_index++;
         }

         /* Set variables */
         environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
      }

error:
      /* Clean up */

      if (option_v1_defs_us)
      {
         free(option_v1_defs_us);
         option_v1_defs_us = NULL;
      }

#ifndef HAVE_NO_LANGEXTRA
      if (option_v1_defs_intl)
      {
         free(option_v1_defs_intl);
         option_v1_defs_intl = NULL;
      }
#endif

      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
