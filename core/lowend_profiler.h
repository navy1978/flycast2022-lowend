#pragma once

#if defined(FLYCAST_LOWEND_PROFILING)

#include <stdint.h>

enum class LowendProfileStage : unsigned
{
	FrontendFrame,
	EmulatedFrameWall,
	EmulatedFrameCpu,
	TAParse,
	TranslucentSort,
	IndexGeneration,
	RendererProcess,
	RendererRender,
	RendererState,
	DrawSubmit,
	DrawOpaque,
	DrawPunchThrough,
	DrawTranslucent,
	TextureDecode,
	TextureHash,
	TextureUpload,
	AICAUpdate,
	AudioMix,
	Present,
	Count
};

enum class LowendDynarecCounter : unsigned
{
	CompiledBlocks,
	FailedToFindBlocks,
	BlockCheckFailures,
	CacheClears,
	BlocksAdded,
	BlocksDiscarded,
	LinkResolutions,
	Exceptions,
	GuestOpcodes,
	HostOpcodes,
	HostCodeBytes,
	GprPreloads,
	GprWritebacks,
	FprPreloads,
	FprWritebacks,
	StaticExits,
	DynamicExits,
	ConditionalExits,
	InterpreterFallbacks,
	Count
};

enum class LowendTextureCounter : unsigned
{
	CacheLookups,
	CacheHits,
	CacheMisses,
	TextureCreates,
	TextureUpdates,
	SourceBytes,
	DecodedBytes,
	Uploads,
	UploadedBytes,
	TexImage2DCalls,
	TexSubImage2DCalls,
	TextureStorageReuses,
	GenerateMipmapCalls,
	VramInvalidations,
	PaletteInvalidations,
	IdenticalReuploads,
	MipLevelsDecoded,
	MipLevelsUploaded,
	MipLevelsDecodedNotUploaded,
	PlanarDecodes,
	TwiddledDecodes,
	VqDecodes,
	PalettedDecodes,
	Count
};

uint64_t lowend_profile_now_ns();
void lowend_profile_record(LowendProfileStage stage, uint64_t duration_ns, uint64_t calls = 1);
void lowend_profile_count(LowendProfileStage stage, uint64_t calls = 1);
void lowend_profile_frontend_frame_complete();
void lowend_profile_emulated_frame_boundary();

void lowend_profile_dynarec_add(LowendDynarecCounter counter, uint64_t value = 1);
void lowend_profile_dynarec_fallback(uint16_t opcode);
void lowend_profile_texture_add(LowendTextureCounter counter, uint64_t value = 1);
void lowend_profile_texture_decode(unsigned format, uint64_t decoded_bytes,
		uint64_t duration_ns);
void lowend_profile_texture_upload(unsigned format, uint64_t uploaded_bytes,
		uint64_t duration_ns);

class LowendProfileScope
{
public:
	explicit LowendProfileScope(LowendProfileStage stage, uint64_t calls = 1)
		: stage(stage), start_ns(lowend_profile_now_ns()), calls(calls)
	{
	}

	~LowendProfileScope()
	{
		lowend_profile_record(stage, lowend_profile_now_ns() - start_ns, calls);
	}

private:
	LowendProfileStage stage;
	uint64_t start_ns;
	uint64_t calls;
};

class LowendProfileSampledScope
{
public:
	LowendProfileSampledScope(LowendProfileStage stage, unsigned& counter,
			unsigned period)
		: stage(stage), start_ns(0), calls(0)
	{
		if (++counter >= period)
		{
			counter = 0;
			calls = period;
			start_ns = lowend_profile_now_ns();
		}
	}

	~LowendProfileSampledScope()
	{
		if (calls != 0)
			lowend_profile_record(stage, lowend_profile_now_ns() - start_ns, calls);
	}

private:
	LowendProfileStage stage;
	uint64_t start_ns;
	uint64_t calls;
};

#define LOWEND_PROFILE_JOIN2(a, b) a##b
#define LOWEND_PROFILE_JOIN(a, b) LOWEND_PROFILE_JOIN2(a, b)
#define LOWEND_PROFILE_SCOPE(stage) \
	LowendProfileScope LOWEND_PROFILE_JOIN(lowend_profile_scope_, __LINE__)(LowendProfileStage::stage)
#define LOWEND_PROFILE_SCOPE_NO_COUNT(stage) \
	LowendProfileScope LOWEND_PROFILE_JOIN(lowend_profile_scope_, __LINE__)(LowendProfileStage::stage, 0)
#define LOWEND_PROFILE_SAMPLED_SCOPE(stage, period) \
	static thread_local unsigned LOWEND_PROFILE_JOIN(lowend_profile_sample_counter_, __LINE__) = 0; \
	LowendProfileSampledScope LOWEND_PROFILE_JOIN(lowend_profile_sample_scope_, __LINE__)( \
			LowendProfileStage::stage, \
			LOWEND_PROFILE_JOIN(lowend_profile_sample_counter_, __LINE__), period)
#define LOWEND_PROFILE_COUNT(stage, calls) \
	lowend_profile_count(LowendProfileStage::stage, calls)
#define LOWEND_DYNAREC_ADD(counter, value) \
	lowend_profile_dynarec_add(LowendDynarecCounter::counter, value)
#define LOWEND_TEXTURE_ADD(counter, value) \
	lowend_profile_texture_add(LowendTextureCounter::counter, value)

#else

#define LOWEND_PROFILE_SCOPE(stage) do { } while (0)
#define LOWEND_PROFILE_SCOPE_NO_COUNT(stage) do { } while (0)
#define LOWEND_PROFILE_SAMPLED_SCOPE(stage, period) do { } while (0)
#define LOWEND_PROFILE_COUNT(stage, calls) do { } while (0)
#define LOWEND_DYNAREC_ADD(counter, value) do { } while (0)
#define LOWEND_TEXTURE_ADD(counter, value) do { } while (0)
#define lowend_profile_frontend_frame_complete() do { } while (0)
#define lowend_profile_emulated_frame_boundary() do { } while (0)
#define lowend_profile_dynarec_fallback(opcode) do { } while (0)
#define lowend_profile_texture_decode(format, decoded_bytes, duration_ns) do { } while (0)
#define lowend_profile_texture_upload(format, uploaded_bytes, duration_ns) do { } while (0)

#endif
