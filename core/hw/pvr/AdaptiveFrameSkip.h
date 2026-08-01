#pragma once

#include "types.h"

#include <atomic>
#include <chrono>

class AdaptiveFrameSkipController
{
public:
	static constexpr u32 ModeValue = 7;
	static constexpr u32 BalancedModeValue = 8;
	static bool isMode(u32 value)
	{
		return value == ModeValue || value == BalancedModeValue;
	}

	bool beginFrame(u32 mode_value);
	void endFrame(bool enabled, bool rendered);
	void noteRenderSubmission(bool enabled, u32 vblank_count);
	bool skipDrawCurrentFrame() const { return skip_current; }
	void reset();

private:
	using Clock = std::chrono::steady_clock;

	void start(Clock::time_point now);
	void adjustLevel(Clock::time_point now);

	Clock::time_point window_start;
	Clock::time_point last_boundary;
	double speed_ema = 1.0;
	u32 level = 0;
	u32 maximum_level = 6;
	u32 slot = 0;
	u32 warmup_frames = 0;
	u32 window_frames = 0;
	u32 good_windows = 0;
	u32 total_frames = 0;
	u32 total_skipped = 0;
	u32 telemetry_windows = 0;
	u32 native_30_entry_level = 0;
	u32 submission_last_vblank = 0;
	u32 submission_cadence_samples = 0;
	u32 submission_two_vblank_samples = 0;
	u32 submission_one_vblank_exit_samples = 0;
	u32 submission_cooldown_samples = 0;
	bool active = false;
	bool have_speed_sample = false;
	bool skip_current = false;
	bool native_30_mode = false;
	bool submission_tracking_enabled = false;
	bool have_submission_vblank = false;
	std::atomic<bool> native_30_detected{false};
};

AdaptiveFrameSkipController& adaptiveFrameSkipController();
