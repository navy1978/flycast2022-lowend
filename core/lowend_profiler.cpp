#include "lowend_profiler.h"

#if defined(FLYCAST_LOWEND_PROFILING)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <time.h>

namespace
{

constexpr unsigned HistogramBuckets = 400;
constexpr uint64_t HistogramBucketNs = 250000;
constexpr uint64_t DefaultReportFrames = 1800;

const char *const stage_names[] = {
	"frontend_frame",
	"emulated_frame_wall",
	"emulated_frame_cpu",
	"ta_parse",
	"translucent_sort",
	"index_generation",
	"renderer_process",
	"renderer_render",
	"renderer_state",
	"draw_submit",
	"draw_opaque",
	"draw_punch_through",
	"draw_translucent",
	"texture_decode",
	"texture_hash",
	"texture_upload",
	"aica_update",
	"audio_mix",
	"present",
};

const char *const dynarec_counter_names[] = {
	"compiled_blocks",
	"failed_to_find_blocks",
	"block_check_failures",
	"cache_clears",
	"blocks_added",
	"blocks_discarded",
	"link_resolutions",
	"exceptions",
	"guest_opcodes",
	"host_opcodes",
	"host_code_bytes",
	"gpr_preloads",
	"gpr_writebacks",
	"fpr_preloads",
	"fpr_writebacks",
	"static_exits",
	"dynamic_exits",
	"conditional_exits",
	"interpreter_fallbacks",
};

const char *const texture_counter_names[] = {
	"cache_lookups",
	"cache_hits",
	"cache_misses",
	"texture_creates",
	"texture_updates",
	"source_bytes",
	"decoded_bytes",
	"uploads",
	"uploaded_bytes",
	"gl_tex_image_2d_calls",
	"gl_tex_sub_image_2d_calls",
	"texture_storage_reuses",
	"gl_generate_mipmap_calls",
	"vram_invalidations",
	"palette_invalidations",
	"identical_reuploads",
	"mip_levels_decoded",
	"mip_levels_uploaded",
	"mip_levels_decoded_not_uploaded",
	"planar_decodes",
	"twiddled_decodes",
	"vq_decodes",
	"paletted_decodes",
};

const char *const texture_decode_format_names[] = {
	"1555",
	"565",
	"4444",
	"yuv",
	"bumpmap",
	"pal4",
	"pal8",
	"reserved",
};

const char *const texture_upload_format_names[] = {
	"565",
	"5551",
	"4444",
	"8888",
	"8bit",
};

static_assert(sizeof(stage_names) / sizeof(stage_names[0])
		== static_cast<unsigned>(LowendProfileStage::Count), "stage name count");
static_assert(sizeof(dynarec_counter_names) / sizeof(dynarec_counter_names[0])
		== static_cast<unsigned>(LowendDynarecCounter::Count), "dynarec counter name count");
static_assert(sizeof(texture_counter_names) / sizeof(texture_counter_names[0])
		== static_cast<unsigned>(LowendTextureCounter::Count), "texture counter name count");

struct StageStats
{
	StageStats()
		: samples(0), calls(0), total_ns(0),
		  min_ns(std::numeric_limits<uint64_t>::max()), max_ns(0)
	{
		for (auto& bucket : histogram)
			bucket.store(0, std::memory_order_relaxed);
	}

	std::atomic<uint64_t> samples;
	std::atomic<uint64_t> calls;
	std::atomic<uint64_t> total_ns;
	std::atomic<uint64_t> min_ns;
	std::atomic<uint64_t> max_ns;
	std::atomic<uint64_t> histogram[HistogramBuckets];
};

StageStats stage_stats[static_cast<unsigned>(LowendProfileStage::Count)];
std::atomic<uint64_t> dynarec_counters[static_cast<unsigned>(LowendDynarecCounter::Count)];
std::atomic<uint64_t> fallback_opcodes[0x10000];
std::atomic<uint64_t> texture_counters[static_cast<unsigned>(LowendTextureCounter::Count)];
std::atomic<uint64_t> texture_decode_counts[
		sizeof(texture_decode_format_names) / sizeof(texture_decode_format_names[0])];
std::atomic<uint64_t> texture_decode_bytes[
		sizeof(texture_decode_format_names) / sizeof(texture_decode_format_names[0])];
std::atomic<uint64_t> texture_decode_ns[
		sizeof(texture_decode_format_names) / sizeof(texture_decode_format_names[0])];
std::atomic<uint64_t> texture_upload_counts[
		sizeof(texture_upload_format_names) / sizeof(texture_upload_format_names[0])];
std::atomic<uint64_t> texture_upload_bytes[
		sizeof(texture_upload_format_names) / sizeof(texture_upload_format_names[0])];
std::atomic<uint64_t> texture_upload_ns[
		sizeof(texture_upload_format_names) / sizeof(texture_upload_format_names[0])];
std::atomic<uint64_t> frontend_frames(0);

uint64_t report_interval_frames()
{
	static const uint64_t interval = []() {
		const char *value = std::getenv("FLYCAST_LOWEND_PROFILE_INTERVAL");
		if (value != nullptr)
		{
			char *end = nullptr;
			unsigned long parsed = std::strtoul(value, &end, 10);
			if (end != value && *end == '\0' && parsed >= 60)
				return static_cast<uint64_t>(parsed);
		}
		return DefaultReportFrames;
	}();
	return interval;
}

void update_min(std::atomic<uint64_t>& target, uint64_t value)
{
	uint64_t previous = target.load(std::memory_order_relaxed);
	while (value < previous
			&& !target.compare_exchange_weak(previous, value,
					std::memory_order_relaxed, std::memory_order_relaxed))
	{
	}
}

void update_max(std::atomic<uint64_t>& target, uint64_t value)
{
	uint64_t previous = target.load(std::memory_order_relaxed);
	while (value > previous
			&& !target.compare_exchange_weak(previous, value,
					std::memory_order_relaxed, std::memory_order_relaxed))
	{
	}
}

uint64_t percentile_ns(const uint64_t *histogram, uint64_t samples, unsigned percentile)
{
	if (samples == 0)
		return 0;
	const uint64_t target = (samples * percentile + 99) / 100;
	uint64_t seen = 0;
	for (unsigned i = 0; i < HistogramBuckets; ++i)
	{
		seen += histogram[i];
		if (seen >= target)
			return (i + 1) * HistogramBucketNs;
	}
	return HistogramBuckets * HistogramBucketNs;
}

uint64_t thread_cpu_now_ns()
{
#if defined(CLOCK_THREAD_CPUTIME_ID)
	timespec value = {};
	if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &value) == 0)
		return static_cast<uint64_t>(value.tv_sec) * 1000000000ULL + value.tv_nsec;
#endif
	return 0;
}

void report_dynarec()
{
	std::fprintf(stderr, "[LOWEND_DYNAREC_CSV] counter,value\n");
	for (unsigned i = 0; i < static_cast<unsigned>(LowendDynarecCounter::Count); ++i)
	{
		uint64_t value = dynarec_counters[i].exchange(0, std::memory_order_relaxed);
		std::fprintf(stderr, "[LOWEND_DYNAREC_CSV] %s,%llu\n",
				dynarec_counter_names[i], static_cast<unsigned long long>(value));
	}

	struct RankedFallback
	{
		uint16_t opcode;
		uint64_t count;
	};
	RankedFallback top[5] = {};
	for (unsigned opcode = 0; opcode < 0x10000; ++opcode)
	{
		uint64_t count = fallback_opcodes[opcode].exchange(0, std::memory_order_relaxed);
		if (count <= top[4].count)
			continue;
		top[4] = { static_cast<uint16_t>(opcode), count };
		for (int rank = 4; rank > 0 && top[rank].count > top[rank - 1].count; --rank)
			std::swap(top[rank], top[rank - 1]);
	}
	std::fprintf(stderr, "[LOWEND_DYNAREC] top_interpreter_fallbacks");
	for (unsigned rank = 0; rank < 5 && top[rank].count != 0; ++rank)
		std::fprintf(stderr, " #%u=%04x:%llu", rank + 1, top[rank].opcode,
				static_cast<unsigned long long>(top[rank].count));
	std::fprintf(stderr, "\n");
}

void report_textures()
{
	std::fprintf(stderr, "[LOWEND_TEXTURE_CSV] counter,value\n");
	for (unsigned i = 0; i < static_cast<unsigned>(LowendTextureCounter::Count); ++i)
	{
		const uint64_t value = texture_counters[i].exchange(0, std::memory_order_relaxed);
		std::fprintf(stderr, "[LOWEND_TEXTURE_CSV] %s,%llu\n",
				texture_counter_names[i], static_cast<unsigned long long>(value));
	}

	std::fprintf(stderr,
			"[LOWEND_TEXTURE_FORMAT_CSV] operation,format,count,bytes,total_ms,avg_ms\n");
	for (unsigned i = 0;
			i < sizeof(texture_decode_format_names) / sizeof(texture_decode_format_names[0]);
			++i)
	{
		const uint64_t count = texture_decode_counts[i].exchange(0, std::memory_order_relaxed);
		const uint64_t bytes = texture_decode_bytes[i].exchange(0, std::memory_order_relaxed);
		const uint64_t duration_ns = texture_decode_ns[i].exchange(0, std::memory_order_relaxed);
		if (count == 0)
			continue;
		std::fprintf(stderr,
				"[LOWEND_TEXTURE_FORMAT_CSV] decode,%s,%llu,%llu,%.6f,%.6f\n",
				texture_decode_format_names[i],
				static_cast<unsigned long long>(count),
				static_cast<unsigned long long>(bytes),
				static_cast<double>(duration_ns) / 1000000.0,
				static_cast<double>(duration_ns) / count / 1000000.0);
	}
	for (unsigned i = 0;
			i < sizeof(texture_upload_format_names) / sizeof(texture_upload_format_names[0]);
			++i)
	{
		const uint64_t count = texture_upload_counts[i].exchange(0, std::memory_order_relaxed);
		const uint64_t bytes = texture_upload_bytes[i].exchange(0, std::memory_order_relaxed);
		const uint64_t duration_ns = texture_upload_ns[i].exchange(0, std::memory_order_relaxed);
		if (count == 0)
			continue;
		std::fprintf(stderr,
				"[LOWEND_TEXTURE_FORMAT_CSV] upload,%s,%llu,%llu,%.6f,%.6f\n",
				texture_upload_format_names[i],
				static_cast<unsigned long long>(count),
				static_cast<unsigned long long>(bytes),
				static_cast<double>(duration_ns) / 1000000.0,
				static_cast<double>(duration_ns) / count / 1000000.0);
	}
}

void report_profile(uint64_t frames)
{
	std::fprintf(stderr, "[LOWEND_PROFILE] frames=%llu histogram_bucket_us=250\n",
			static_cast<unsigned long long>(frames));
	std::fprintf(stderr,
			"[LOWEND_PROFILE_CSV] stage,samples,calls,avg_ms,min_ms,max_ms,p50_ms,p95_ms,p99_ms\n");

	for (unsigned i = 0; i < static_cast<unsigned>(LowendProfileStage::Count); ++i)
	{
		StageStats& stats = stage_stats[i];
		uint64_t histogram[HistogramBuckets];
		uint64_t histogram_samples = 0;
		for (unsigned bucket = 0; bucket < HistogramBuckets; ++bucket)
		{
			histogram[bucket] = stats.histogram[bucket].exchange(0, std::memory_order_relaxed);
			histogram_samples += histogram[bucket];
		}

		const uint64_t samples = stats.samples.exchange(0, std::memory_order_relaxed);
		const uint64_t calls = stats.calls.exchange(0, std::memory_order_relaxed);
		const uint64_t total_ns = stats.total_ns.exchange(0, std::memory_order_relaxed);
		const uint64_t min_ns = stats.min_ns.exchange(
				std::numeric_limits<uint64_t>::max(), std::memory_order_relaxed);
		const uint64_t max_ns = stats.max_ns.exchange(0, std::memory_order_relaxed);
		if (samples == 0 && calls == 0)
			continue;

		const double average_ms = samples == 0 ? 0.0
				: static_cast<double>(total_ns) / samples / 1000000.0;
		const double minimum_ms = samples == 0
				|| min_ns == std::numeric_limits<uint64_t>::max() ? 0.0
				: static_cast<double>(min_ns) / 1000000.0;
		const double maximum_ms = samples == 0 ? 0.0
				: static_cast<double>(max_ns) / 1000000.0;
		const double p50_ms = percentile_ns(histogram, histogram_samples, 50) / 1000000.0;
		const double p95_ms = percentile_ns(histogram, histogram_samples, 95) / 1000000.0;
		const double p99_ms = percentile_ns(histogram, histogram_samples, 99) / 1000000.0;

		std::fprintf(stderr,
				"[LOWEND_PROFILE_CSV] %s,%llu,%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
				stage_names[i],
				static_cast<unsigned long long>(samples),
				static_cast<unsigned long long>(calls),
				average_ms, minimum_ms, maximum_ms, p50_ms, p95_ms, p99_ms);
	}
	report_dynarec();
	report_textures();
	std::fflush(stderr);
}

} // namespace

uint64_t lowend_profile_now_ns()
{
	using Clock = std::chrono::steady_clock;
	return static_cast<uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(
					Clock::now().time_since_epoch()).count());
}

void lowend_profile_record(LowendProfileStage stage, uint64_t duration_ns, uint64_t calls)
{
	StageStats& stats = stage_stats[static_cast<unsigned>(stage)];
	stats.samples.fetch_add(1, std::memory_order_relaxed);
	stats.calls.fetch_add(calls, std::memory_order_relaxed);
	stats.total_ns.fetch_add(duration_ns, std::memory_order_relaxed);
	update_min(stats.min_ns, duration_ns);
	update_max(stats.max_ns, duration_ns);
	const unsigned bucket = std::min<unsigned>(
			duration_ns / HistogramBucketNs, HistogramBuckets - 1);
	stats.histogram[bucket].fetch_add(1, std::memory_order_relaxed);
}

void lowend_profile_count(LowendProfileStage stage, uint64_t calls)
{
	stage_stats[static_cast<unsigned>(stage)].calls.fetch_add(calls, std::memory_order_relaxed);
}

void lowend_profile_frontend_frame_complete()
{
	const uint64_t frames = frontend_frames.fetch_add(1, std::memory_order_relaxed) + 1;
	const uint64_t interval = report_interval_frames();
	if (frames % interval == 0)
		report_profile(interval);
}

void lowend_profile_emulated_frame_boundary()
{
	static thread_local uint64_t previous_wall_ns = 0;
	static thread_local uint64_t previous_cpu_ns = 0;
	const uint64_t wall_ns = lowend_profile_now_ns();
	const uint64_t cpu_ns = thread_cpu_now_ns();
	if (previous_wall_ns != 0)
		lowend_profile_record(LowendProfileStage::EmulatedFrameWall,
				wall_ns - previous_wall_ns);
	if (previous_cpu_ns != 0 && cpu_ns != 0)
		lowend_profile_record(LowendProfileStage::EmulatedFrameCpu,
				cpu_ns - previous_cpu_ns);
	previous_wall_ns = wall_ns;
	previous_cpu_ns = cpu_ns;
}

void lowend_profile_dynarec_add(LowendDynarecCounter counter, uint64_t value)
{
	dynarec_counters[static_cast<unsigned>(counter)].fetch_add(value, std::memory_order_relaxed);
}

void lowend_profile_dynarec_fallback(uint16_t opcode)
{
	lowend_profile_dynarec_add(LowendDynarecCounter::InterpreterFallbacks);
	fallback_opcodes[opcode].fetch_add(1, std::memory_order_relaxed);
}

void lowend_profile_texture_add(LowendTextureCounter counter, uint64_t value)
{
	texture_counters[static_cast<unsigned>(counter)].fetch_add(value,
			std::memory_order_relaxed);
}

void lowend_profile_texture_decode(unsigned format, uint64_t decoded_bytes,
		uint64_t duration_ns)
{
	if (format >= sizeof(texture_decode_format_names) / sizeof(texture_decode_format_names[0]))
		format = 7;
	texture_decode_counts[format].fetch_add(1, std::memory_order_relaxed);
	texture_decode_bytes[format].fetch_add(decoded_bytes, std::memory_order_relaxed);
	texture_decode_ns[format].fetch_add(duration_ns, std::memory_order_relaxed);
}

void lowend_profile_texture_upload(unsigned format, uint64_t uploaded_bytes,
		uint64_t duration_ns)
{
	if (format >= sizeof(texture_upload_format_names) / sizeof(texture_upload_format_names[0]))
		return;
	texture_upload_counts[format].fetch_add(1, std::memory_order_relaxed);
	texture_upload_bytes[format].fetch_add(uploaded_bytes, std::memory_order_relaxed);
	texture_upload_ns[format].fetch_add(duration_ns, std::memory_order_relaxed);
}

#endif
