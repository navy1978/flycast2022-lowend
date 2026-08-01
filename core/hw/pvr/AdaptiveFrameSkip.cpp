#include "AdaptiveFrameSkip.h"

#include <algorithm>

extern "C" u32 flycast_retrorun_get_audio_queue_pressure_v1(void);

namespace
{
constexpr u32 PatternSize = 12;
constexpr u32 MaxLevel = 6;
constexpr u32 BalancedMaxLevel = 4;
constexpr u32 PatternLevels = 10;
constexpr u32 WarmupFrames = 24;
constexpr u32 CadenceWindow = 24;
constexpr u32 Native30MinimumSamples = 20;
constexpr u32 Native30ExitSamples = 4;
constexpr u32 Native30CooldownSamples = 48;
constexpr double DreamcastFrameSeconds = 1.0 / 60.0;
constexpr double RaiseBelowSpeed = 0.97;
constexpr double LowerAboveSpeed = 0.995;

// The first seven levels of MAME's evenly distributed 12-frame skip table.
// Level 6 is the maximum: render/skip alternation, never two skips in a row.
constexpr bool SkipPattern[PatternLevels][PatternSize] = {
	{ false, false, false, false, false, false, false, false, false, false, false, false },
	{ false, false, false, false, false, false, false, false, false, false, false, true  },
	{ false, false, false, false, false, true,  false, false, false, false, false, true  },
	{ false, false, false, true,  false, false, false, true,  false, false, false, true  },
	{ false, false, true,  false, false, true,  false, false, true,  false, false, true  },
	{ false, true,  false, false, true,  false, true,  false, false, true,  false, true  },
	{ false, true,  false, true,  false, true,  false, true,  false, true,  false, true  },
	{ false, true,  true,  false, true,  false, true,  true,  false, true,  false, true  },
	{ false, true,  true,  false, true,  true,  false, true,  true,  false, true,  true  },
	{ false, true,  true,  true,  false, true,  true,  true,  false, true,  true,  true  },
};
}

AdaptiveFrameSkipController& adaptiveFrameSkipController()
{
	static AdaptiveFrameSkipController controller;
	return controller;
}

extern "C" bool flycast_retrorun_adaptive_skip_draw_v1(void)
{
	return adaptiveFrameSkipController().skipDrawCurrentFrame();
}

void AdaptiveFrameSkipController::reset()
{
	speed_ema = 1.0;
	level = 0;
	slot = 0;
	warmup_frames = 0;
	window_frames = 0;
	good_windows = 0;
	total_frames = 0;
	total_skipped = 0;
	telemetry_windows = 0;
	native_30_entry_level = 0;
	active = false;
	have_speed_sample = false;
	skip_current = false;
	native_30_mode = false;
}

void AdaptiveFrameSkipController::start(Clock::time_point now)
{
	reset();
	active = true;
	window_start = now;
	last_boundary = now;
}

bool AdaptiveFrameSkipController::beginFrame(u32 mode_value)
{
	const bool enabled = isMode(mode_value);
	maximum_level = mode_value == BalancedModeValue ? BalancedMaxLevel : MaxLevel;
	skip_current = false;
	const Clock::time_point now = Clock::now();
	if (!enabled)
	{
		if (active)
			reset();
		return false;
	}

	if (!active)
	{
		start(now);
		return false;
	}

	const double gap_ms =
			std::chrono::duration<double, std::milli>(
					now - last_boundary).count();
	last_boundary = now;
	if (gap_ms < 0.0 || gap_ms > 250.0)
	{
		start(now);
		return false;
	}

	const bool detected = native_30_detected.load(std::memory_order_acquire);
	if (detected && !native_30_mode)
	{
		native_30_mode = true;
		native_30_entry_level = level;
		level = 0;
		slot = 0;
		good_windows = 0;
		NOTICE_LOG(PVR, "Adaptive frame skip: native 30 FPS submission "
				"cadence detected, draw skipping suspended");
	}
	else if (!detected && native_30_mode)
	{
		native_30_mode = false;
		level = native_30_entry_level;
		slot = 0;
		good_windows = 0;
		NOTICE_LOG(PVR, "Adaptive frame skip: native 30 FPS submission "
				"cadence ended, restoring level %u", level);
	}

	if (native_30_mode)
		return false;

	if (warmup_frames < WarmupFrames)
		return false;

	skip_current = SkipPattern[level][slot];
	return skip_current;
}

void AdaptiveFrameSkipController::noteRenderSubmission(
		bool enabled, u32 vblank_count)
{
	if (!enabled)
	{
		if (submission_tracking_enabled)
		{
			submission_tracking_enabled = false;
			have_submission_vblank = false;
			submission_cadence_samples = 0;
			submission_two_vblank_samples = 0;
			submission_one_vblank_exit_samples = 0;
			submission_cooldown_samples = 0;
			native_30_detected.store(false, std::memory_order_release);
		}
		return;
	}
	submission_tracking_enabled = true;

	if (!have_submission_vblank)
	{
		submission_last_vblank = vblank_count;
		have_submission_vblank = true;
		return;
	}

	const u32 delta = vblank_count - submission_last_vblank;
	submission_last_vblank = vblank_count;
	if (submission_cooldown_samples > 0)
		submission_cooldown_samples--;

	if (native_30_detected.load(std::memory_order_relaxed))
	{
		if (delta == 1)
			submission_one_vblank_exit_samples++;
		else if (delta == 2)
			submission_one_vblank_exit_samples = 0;

		if (submission_one_vblank_exit_samples >= Native30ExitSamples)
		{
			submission_cadence_samples = 0;
			submission_two_vblank_samples = 0;
			submission_one_vblank_exit_samples = 0;
			submission_cooldown_samples = Native30CooldownSamples;
			native_30_detected.store(false, std::memory_order_release);
		}
		return;
	}

	submission_cadence_samples++;
	if (delta == 2)
		submission_two_vblank_samples++;

	if (submission_cadence_samples < CadenceWindow)
		return;

	if (submission_cooldown_samples == 0
			&& submission_two_vblank_samples >= Native30MinimumSamples)
		native_30_detected.store(true, std::memory_order_release);

	submission_cadence_samples = 0;
	submission_two_vblank_samples = 0;
}

void AdaptiveFrameSkipController::endFrame(bool enabled, bool rendered)
{
	if (!enabled || !active)
		return;

	const Clock::time_point now = Clock::now();
	total_frames++;
	if (!rendered)
		total_skipped++;
	if (warmup_frames < WarmupFrames)
		warmup_frames++;

	slot = (slot + 1) % PatternSize;
	window_frames++;
	if (window_frames >= PatternSize)
		adjustLevel(now);
}

void AdaptiveFrameSkipController::adjustLevel(Clock::time_point now)
{
	const double elapsed_seconds =
			std::chrono::duration<double>(now - window_start).count();
	window_start = now;
	window_frames = 0;

	if (elapsed_seconds <= 0.0 || elapsed_seconds > 1.5)
	{
		have_speed_sample = false;
		good_windows = 0;
		return;
	}

	const double measured_speed =
			(PatternSize * DreamcastFrameSeconds) / elapsed_seconds;
	if (!have_speed_sample)
	{
		speed_ema = measured_speed;
		have_speed_sample = true;
	}
	else
	{
		speed_ema = speed_ema * 0.75 + measured_speed * 0.25;
	}

	if (warmup_frames >= WarmupFrames)
	{
		const u32 audio_occupancy =
				flycast_retrorun_get_audio_queue_pressure_v1();
		if (native_30_mode)
		{
			level = 0;
			good_windows = 0;
			return;
		}

		if (speed_ema < RaiseBelowSpeed
				|| (audio_occupancy <= 12 && speed_ema < LowerAboveSpeed))
		{
			const u32 step = speed_ema < 0.55 ? 2 : 1;
			level = std::min(maximum_level, level + step);
			good_windows = 0;
			slot = 0;
		}
		else if (speed_ema > LowerAboveSpeed && audio_occupancy >= 35)
		{
			if (++good_windows >= 3)
			{
				if (level > 0)
					level--;
				good_windows = 0;
				slot = 0;
			}
		}
		else
		{
			good_windows = 0;
		}

		if (++telemetry_windows % 4 == 0)
			DEBUG_LOG(PVR, "Adaptive frame skip: level %u, speed %.1f%%, "
					"skipped %u/%u, audio %u%%",
					level, speed_ema * 100.0, total_skipped, total_frames,
					audio_occupancy);
	}
}
