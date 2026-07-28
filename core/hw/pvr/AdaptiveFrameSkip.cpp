#include "AdaptiveFrameSkip.h"

#include <algorithm>

extern "C" u32 flycast_retrorun_get_audio_queue_pressure_v1(void);

namespace
{
constexpr u32 PatternSize = 12;
constexpr u32 MaxLevel = 6;
constexpr u32 PatternLevels = 10;
constexpr u32 WarmupFrames = 24;
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
	active = false;
	have_speed_sample = false;
	skip_current = false;
}

void AdaptiveFrameSkipController::start(Clock::time_point now)
{
	reset();
	active = true;
	window_start = now;
	last_boundary = now;
}

bool AdaptiveFrameSkipController::beginFrame(bool enabled)
{
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

	if (warmup_frames < WarmupFrames)
		return false;

	skip_current = SkipPattern[level][slot];
	return skip_current;
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
		if (speed_ema < RaiseBelowSpeed
				|| (audio_occupancy <= 12 && speed_ema < LowerAboveSpeed))
		{
			const u32 step = speed_ema < 0.55 ? 2 : 1;
			level = std::min(MaxLevel, level + step);
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
