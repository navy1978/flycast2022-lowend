#pragma once

#include "types.h"

#include <chrono>

class AdaptiveFrameSkipController
{
public:
	static constexpr u32 ModeValue = 7;

	bool beginFrame(bool enabled);
	void endFrame(bool enabled, bool rendered);
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
	u32 slot = 0;
	u32 warmup_frames = 0;
	u32 window_frames = 0;
	u32 good_windows = 0;
	u32 total_frames = 0;
	u32 total_skipped = 0;
	u32 telemetry_windows = 0;
	bool active = false;
	bool have_speed_sample = false;
	bool skip_current = false;
};

AdaptiveFrameSkipController& adaptiveFrameSkipController();
