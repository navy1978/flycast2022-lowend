#include "gles.h"
#include "lowend_profiler.h"
#include <glsym/glsym_es2.h>

#include <algorithm>
#include <cmath>
#include <cstring>

/*

Drawing and related state management
Takes vertex, textures and renders to the currently set up target
*/

const static u32 CullModes[]= 
{
	GL_NONE, //0    No culling          No culling
	GL_NONE, //1    Cull if Small       Cull if ( |det| < fpu_cull_val )

	GL_FRONT, //2   Cull if Negative    Cull if ( |det| < 0 ) or ( |det| < fpu_cull_val )
	GL_BACK,  //3   Cull if Positive    Cull if ( |det| > 0 ) or ( |det| < fpu_cull_val )
};
const u32 Zfunction[] =
{
	GL_NEVER,       //0 Never
	GL_LESS,        //1 Less
	GL_EQUAL,       //2 Equal
	GL_LEQUAL,      //3 Less Or Equal
	GL_GREATER,     //4 Greater
	GL_NOTEQUAL,    //5 Not Equal
	GL_GEQUAL,      //6 Greater Or Equal
	GL_ALWAYS,      //7 Always
};

/*
0   Zero                  (0, 0, 0, 0)
1   One                   (1, 1, 1, 1)
2   Other Color           (OR, OG, OB, OA)
3   Inverse Other Color   (1-OR, 1-OG, 1-OB, 1-OA)
4   SRC Alpha             (SA, SA, SA, SA)
5   Inverse SRC Alpha     (1-SA, 1-SA, 1-SA, 1-SA)
6   DST Alpha             (DA, DA, DA, DA)
7   Inverse DST Alpha     (1-DA, 1-DA, 1-DA, 1-DA)
*/

const u32 DstBlendGL[] =
{
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA
};

const u32 SrcBlendGL[] =
{
	GL_ZERO,
	GL_ONE,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA
};

extern int screen_width;
extern int screen_height;

PipelineShader* CurrentShader;
extern u32 gcflip;
GLuint vmuTextureId[4]={0,0,0,0};
GLuint lightgunTextureId[4]={0,0,0,0};

void SetCull(u32 CullMode)
{
	if (CullModes[CullMode] == GL_NONE)
		glcache.Disable(GL_CULL_FACE);
	else
	{
		glcache.Enable(GL_CULL_FACE);
		glcache.CullFace(CullModes[CullMode]); //GL_FRONT/GL_BACK, ...
	}
}

static void SetTextureRepeatMode(GLuint dir, u32 clamp, u32 mirror)
{
	if (clamp)
		glcache.TexParameteri(GL_TEXTURE_2D, dir, GL_CLAMP_TO_EDGE);
	else
		glcache.TexParameteri(GL_TEXTURE_2D, dir, mirror ? GL_MIRRORED_REPEAT : GL_REPEAT);
}

static void SetBaseClipping()
{
	if (ShaderUniforms.base_clipping.enabled)
	{
		glcache.Enable(GL_SCISSOR_TEST);
		glcache.Scissor(ShaderUniforms.base_clipping.x, ShaderUniforms.base_clipping.y, ShaderUniforms.base_clipping.width, ShaderUniforms.base_clipping.height);
	}
	else
		glcache.Disable(GL_SCISSOR_TEST);
}

static bool UsePerTriangleTranslucentSorting()
{
	return settings.pvr.Emulation.AlphaSortMode == 0
			|| (settings.rend.TranslucentStripMerge == 2
				&& settings.rend.TranslucentMenuGuardDrawSorting == 1);
}

struct FastDepthGuardState
{
	u64 frame_index;
	u64 previous_signature;
	u32 repeat_streak;
	u32 frame_mode;
};

static FastDepthGuardState fast_depth_guard = {};

static u64 FastDepthHashWord(u64 hash, u32 value)
{
	hash ^= value;
	return hash * 1099511628211ULL;
}

static u32 FastDepthFloatBits(float value)
{
	u32 bits;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static u64 FastDepthSceneSignature()
{
	u64 hash = 1469598103934665603ULL;
	hash = FastDepthHashWord(hash, pvrrc.global_param_op.used());
	hash = FastDepthHashWord(hash, pvrrc.global_param_pt.used());
	hash = FastDepthHashWord(hash, pvrrc.global_param_tr.used());
	hash = FastDepthHashWord(hash, pvrrc.verts.used());

	// A full vertex hash would waste CPU on the hardware this mode targets.
	// Ninety-six evenly distributed positions are enough to distinguish
	// moving gameplay from the byte-stable geometry emitted while SA2 is
	// paused. Three consecutive matching frames are required so a 30-FPS
	// game that repeats each frame once is not mistaken for a pause.
	const u32 vertex_count = pvrrc.verts.used();
	if (vertex_count != 0)
	{
		const Vertex *vertices = pvrrc.verts.head();
		const u32 stride = std::max(1u, vertex_count / 96u);
		for (u32 i = 0; i < vertex_count; i += stride)
		{
			hash = FastDepthHashWord(hash, FastDepthFloatBits(vertices[i].x));
			hash = FastDepthHashWord(hash, FastDepthFloatBits(vertices[i].y));
			hash = FastDepthHashWord(hash, FastDepthFloatBits(vertices[i].z));
		}
		const Vertex& last = vertices[vertex_count - 1];
		hash = FastDepthHashWord(hash, FastDepthFloatBits(last.x));
		hash = FastDepthHashWord(hash, FastDepthFloatBits(last.y));
		hash = FastDepthHashWord(hash, FastDepthFloatBits(last.z));
	}
	return hash;
}

struct FastDepthFontCluster
{
	u32 texture_address;
	u16 height_bin;
	s16 line_bin;
	u16 glyphs;
};

static void FindFontGlyphs(const List<PolyParam>& list,
		FastDepthFontCluster *clusters, u32& cluster_count,
		u32 cluster_capacity)
{
	const u32 *indices = pvrrc.idx.head();
	const Vertex *vertices = pvrrc.verts.head();
	for (u32 poly_index = 0; poly_index < list.used(); ++poly_index)
	{
		const PolyParam& poly = list.head()[poly_index];
		if (!poly.pcw.Texture || poly.count < 4 || poly.count > 8
				|| poly.first >= pvrrc.idx.used()
				|| poly.count > pvrrc.idx.used() - poly.first)
			continue;

		float min_x = 1e30f;
		float max_x = -1e30f;
		float min_y = 1e30f;
		float max_y = -1e30f;
		float min_z = 1e30f;
		float max_z = -1e30f;
		u32 corner_vertices = 0;
		bool valid = true;
		for (u32 i = 0; i < poly.count; ++i)
		{
			const u32 vertex_index = indices[poly.first + i];
			if (vertex_index >= pvrrc.verts.used())
			{
				valid = false;
				break;
			}
			const Vertex& vertex = vertices[vertex_index];
			min_x = std::min(min_x, vertex.x);
			max_x = std::max(max_x, vertex.x);
			min_y = std::min(min_y, vertex.y);
			max_y = std::max(max_y, vertex.y);
			min_z = std::min(min_z, vertex.z);
			max_z = std::max(max_z, vertex.z);
		}
		const float width = max_x - min_x;
		const float height = max_y - min_y;
		if (!valid || width < 2.f || width > 128.f
				|| height < 5.f || height > 96.f
				|| width > height * 2.5f)
			continue;
		const float z_scale = std::max(1.f,
				std::max(std::abs(min_z), std::abs(max_z)));
		if (max_z - min_z > z_scale * 0.0015f)
			continue;

		const float x_tolerance = std::max(1.f, width * 0.025f);
		const float y_tolerance = std::max(1.f, height * 0.025f);
		for (u32 i = 0; i < poly.count; ++i)
		{
			const Vertex& vertex = vertices[indices[poly.first + i]];
			const bool aligned_x = std::abs(vertex.x - min_x) <= x_tolerance
					|| std::abs(vertex.x - max_x) <= x_tolerance;
			const bool aligned_y = std::abs(vertex.y - min_y) <= y_tolerance
					|| std::abs(vertex.y - max_y) <= y_tolerance;
			corner_vertices += aligned_x && aligned_y;
		}
		if (corner_vertices * 5 < poly.count * 4)
			continue;

		const u16 height_bin = (u16)std::min(65535L,
				std::max(0L, std::lround(height / 4.f)));
		const s16 line_bin = (s16)std::max(-32768L,
				std::min(32767L, std::lround((min_y + max_y) / 24.f)));
		u32 cluster_index = 0;
		for (; cluster_index < cluster_count; ++cluster_index)
		{
			FastDepthFontCluster& cluster = clusters[cluster_index];
			if (cluster.texture_address == poly.tcw.TexAddr
					&& cluster.height_bin == height_bin
					&& cluster.line_bin == line_bin)
			{
				++cluster.glyphs;
				break;
			}
		}
		if (cluster_index == cluster_count
				&& cluster_count < cluster_capacity)
		{
			FastDepthFontCluster& cluster = clusters[cluster_count++];
			cluster.texture_address = poly.tcw.TexAddr;
			cluster.height_bin = height_bin;
			cluster.line_bin = line_bin;
			cluster.glyphs = 1;
		}
	}
}

static u32 CountFontLikeGlyphs()
{
	FastDepthFontCluster clusters[64] = {};
	u32 cluster_count = 0;
	FindFontGlyphs(pvrrc.global_param_op, clusters, cluster_count, 64);
	FindFontGlyphs(pvrrc.global_param_pt, clusters, cluster_count, 64);
	FindFontGlyphs(pvrrc.global_param_tr, clusters, cluster_count, 64);

	u32 glyphs = 0;
	u32 largest_line = 0;
	for (u32 i = 0; i < cluster_count; ++i)
	{
		largest_line = std::max(largest_line, (u32)clusters[i].glyphs);
		if (clusters[i].glyphs >= 5)
			glyphs += clusters[i].glyphs;
	}
	// Normal gameplay HUD counters stay below this threshold. A real menu has
	// several words or choices made of repeated glyph quads from one atlas.
	return glyphs >= 24 && largest_line >= 6 ? glyphs : 0;
}

static void UpdateFastDepthGuard()
{
	if (settings.rend.FastDepth < 4)
		return;

	++fast_depth_guard.frame_index;
	const u32 poly_count = pvrrc.global_param_op.used()
			+ pvrrc.global_param_pt.used()
			+ pvrrc.global_param_tr.used();
	const u64 signature = FastDepthSceneSignature();
	if (fast_depth_guard.frame_index > 1
			&& signature == fast_depth_guard.previous_signature)
		++fast_depth_guard.repeat_streak;
	else
		fast_depth_guard.repeat_streak = 0;
	fast_depth_guard.previous_signature = signature;

	const bool simple_menu_frame = poly_count <= 180;
	// Font-like particles and billboard chains occur in normal gameplay too.
	// Only use the glyph signal for interface-sized scenes: richer than the
	// simple title menu, but far below the polygon load of an active SA2 level.
	const u32 font_glyphs = poly_count > 180 && poly_count <= 900
			? CountFontLikeGlyphs() : 0;
	const bool font_menu_frame = font_glyphs != 0;
	const bool menu_frame = simple_menu_frame || font_menu_frame;
	const bool pause_frame = !menu_frame
			&& fast_depth_guard.repeat_streak >= 2;
	fast_depth_guard.frame_mode = menu_frame || pause_frame ? 0 : 3;
}

static bool ShadowReceiverHasDepthVariation(const PolyParam *poly,
		float ratio)
{
	if (poly == nullptr || !poly->pcw.Shadow || poly->count < 3
			|| poly->first >= pvrrc.idx.used()
			|| poly->count > pvrrc.idx.used() - poly->first)
		return false;

	const u32 *indices = pvrrc.idx.head();
	const Vertex *vertices = pvrrc.verts.head();
	float min_z = 1e30f;
	float max_z = 0.f;
	for (u32 i = 0; i < poly->count; ++i)
	{
		const u32 vertex_index = indices[poly->first + i];
		if (vertex_index >= pvrrc.verts.used())
			return true;
		const float z = std::abs(vertices[vertex_index].z);
		if (!std::isfinite(z) || z <= 0.f)
			return true;
		min_z = std::min(min_z, z);
		max_z = std::max(max_z, z);
	}
	return max_z > min_z * ratio;
}

static u32 SelectFastDepthMode(const PolyParam *poly, u32 list_type)
{
	if (settings.rend.FastDepth < 4)
		return std::min(settings.rend.FastDepth, 3u);
	if (fast_depth_guard.frame_mode == 0)
		return 0;
	if (settings.rend.FastDepth == 5
			&& list_type == ListType_Opaque
			&& ShadowReceiverHasDepthVariation(poly, 4.f))
		return 0;
	return fast_depth_guard.frame_mode;
}

template <u32 Type, bool SortingEnabled>
__forceinline
	void SetGPState(const PolyParam* gp, u32 fast_depth, u32 cflip=0)
{
	LOWEND_PROFILE_SAMPLED_SCOPE(RendererState, 16);
	if (gp->pcw.Texture && gp->tsp.FilterMode > 1 && Type != ListType_Punch_Through && gp->tcw.MipMapped == 1)
	{
		ShaderUniforms.trilinear_alpha = 0.25 * (gp->tsp.MipMapD & 0x3);
		if (gp->tsp.FilterMode == 2)
			// Trilinear pass A
			ShaderUniforms.trilinear_alpha = 1.0 - ShaderUniforms.trilinear_alpha;
	}
	else
		ShaderUniforms.trilinear_alpha = 1.f;

	bool color_clamp = gp->tsp.ColorClamp && (pvrrc.fog_clamp_min != 0 || pvrrc.fog_clamp_max != 0xffffffff);
	int fog_ctrl = settings.rend.Fog ? gp->tsp.FogCtrl : 2;

	int clip_rect[4] = {};
	TileClipping clipmode = GetTileClip(gp->tileclip, ViewportMatrix, clip_rect);
	bool palette = BaseTextureCacheData::IsGpuHandledPaletted(gp->tsp, gp->tcw);

	CurrentShader = GetProgram(Type == ListType_Punch_Through ? true : false,
								  clipmode == TileClipping::Inside,
								  gp->pcw.Texture,
								  gp->tsp.UseAlpha,
								  gp->tsp.IgnoreTexA,
								  gp->tsp.ShadInstr,
								  gp->pcw.Offset,
								  fog_ctrl,
								  gp->pcw.Gouraud,
								  gp->tcw.PixelFmt == PixelBumpMap,
								  color_clamp,
								  ShaderUniforms.trilinear_alpha != 1.f,
								  palette,
								  fast_depth);

	glcache.UseProgram(CurrentShader->program);
	if (CurrentShader->trilinear_alpha != -1)
		glUniform1f(CurrentShader->trilinear_alpha, ShaderUniforms.trilinear_alpha);
	if (palette)
	{
		if (gp->tcw.PixelFmt == PixelPal4)
			ShaderUniforms.palette_index = gp->tcw.PalSelect << 4;
		else
			ShaderUniforms.palette_index = (gp->tcw.PalSelect >> 4) << 8;
		glUniform1i(CurrentShader->palette_index, ShaderUniforms.palette_index);
	}

	if (clipmode == TileClipping::Inside)
		glUniform4f(CurrentShader->pp_ClipTest, clip_rect[0], clip_rect[1], clip_rect[0] + clip_rect[2], clip_rect[1] + clip_rect[3]);
	if (clipmode == TileClipping::Outside)
	{
		glcache.Enable(GL_SCISSOR_TEST);
		glcache.Scissor(clip_rect[0], clip_rect[1], clip_rect[2], clip_rect[3]);
	}
	else
		SetBaseClipping();

	//This bit control which pixels are affected
	//by modvols
	const u32 stencil=(gp->pcw.Shadow!=0)?0x80:0x0;

	glcache.StencilFunc(GL_ALWAYS,stencil,stencil);

	glcache.BindTexture(GL_TEXTURE_2D, gp->texid == (u64)-1 ? 0 : (GLuint)gp->texid);

	SetTextureRepeatMode(GL_TEXTURE_WRAP_S, gp->tsp.ClampU, gp->tsp.FlipU);
	SetTextureRepeatMode(GL_TEXTURE_WRAP_T, gp->tsp.ClampV, gp->tsp.FlipV);

	//set texture filter mode
	if (gp->tsp.FilterMode == 0 || palette)
	{
		//disable filtering, mipmaps
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		//bilinear filtering
		//PowerVR supports also trilinear via two passes, but we ignore that for now
		bool mipmapped = gp->tcw.MipMapped != 0 && gp->tcw.ScanOrder == 0 && settings.rend.UseMipmaps;
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmapped ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_TEXTURE_LOD_BIAS
		if (!gl.is_gles && gl.gl_major >= 3 && mipmapped)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, D_Adjust_LoD_Bias[gp->tsp.MipMapD]);
#endif
		if (gl.max_anisotropy > 1.f)
		{
			if (settings.rend.AnisotropicFiltering > 1)
			{
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
						std::min((f32)settings.rend.AnisotropicFiltering, gl.max_anisotropy));
				// Set the recommended minification filter for best results
				if (mipmapped)
					glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
		}
	}

	// Apparently punch-through polys support blending, or at least some combinations
	if (Type == ListType_Translucent || Type == ListType_Punch_Through)
	{
		glcache.Enable(GL_BLEND);
		glcache.BlendFunc(SrcBlendGL[gp->tsp.SrcInstr], DstBlendGL[gp->tsp.DstInstr]);
	}
	else
		glcache.Disable(GL_BLEND);

	//set cull mode !
	//cflip is required when exploding triangles for triangle sorting
	//gcflip is global clip flip, needed for when rendering to texture due to mirrored Y direction
	SetCull(gp->isp.CullMode ^ cflip ^ gcflip);

	//set Z mode, only if required
	if (Type == ListType_Punch_Through || (Type == ListType_Translucent && SortingEnabled))
	{
		glcache.DepthFunc(GL_GEQUAL);
	}
	else
	{
		glcache.DepthFunc(Zfunction[gp->isp.DepthMode]);
	}

	if (SortingEnabled && UsePerTriangleTranslucentSorting())
		glcache.DepthMask(GL_FALSE);
	else
	{
		// Z Write Disable seems to be ignored for punch-through.
		// Fixes Worms World Party, Bust-a-Move 4 and Re-Volt
		if (Type == ListType_Punch_Through)
			glcache.DepthMask(GL_TRUE);
		else
			glcache.DepthMask(!gp->isp.ZWriteDis);
	}
}

static bool SamePolyState(const PolyParam& left, const PolyParam& right)
{
	return left.pcw.full == right.pcw.full
			&& left.tcw.full == right.tcw.full
			&& left.tsp.full == right.tsp.full
			&& left.isp.full == right.isp.full
			&& left.tcw1.full == right.tcw1.full
			&& left.tsp1.full == right.tsp1.full
			&& left.tileclip == right.tileclip
			&& left.texid == right.texid
			&& left.texid1 == right.texid1;
}

#ifndef LOWEND_GLES_MULTI_DRAW
#define LOWEND_GLES_MULTI_DRAW 1
#endif

template <u32 Type, bool SortingEnabled>
static void DrawList(const List<PolyParam>& gply, int first, int count)
{
	PolyParam* params= &gply.head()[first];

	/* We want at least 1 PParam */
	if (count==0)
		return;

	/* set some 'global' modes for all primitives */
	glcache.Enable(GL_STENCIL_TEST);
	glcache.StencilFunc(GL_ALWAYS,0,0);
	glcache.StencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);

	const PolyParam *active_state = NULL;
	u32 active_fast_depth = ~0u;
	while(count-->0)
	{
		if (params->count>2) /* this actually happens for some games. No idea why .. */
		{
			const u32 fast_depth = SelectFastDepthMode(params, Type);
			if (!settings.rend.AdjacentStateElision
					|| active_state == NULL
					|| !SamePolyState(*active_state, *params)
					|| active_fast_depth != fast_depth)
			{
				SetGPState<Type,SortingEnabled>(params, fast_depth);
				active_state = params;
				active_fast_depth = fast_depth;
			}
			int submitted_draws = 1;
#if LOWEND_GLES_MULTI_DRAW
			GLsizei draw_counts[64];
			const GLvoid *draw_offsets[64];
			draw_counts[0] = params->count;
			draw_offsets[0] = (GLvoid *)(gl.get_index_size() * params->first);
			while (glMultiDrawElementsEXT != nullptr
					&& submitted_draws < 64 && submitted_draws <= count)
			{
				const PolyParam *next = params + submitted_draws;
				if (next->count <= 2
						|| !SamePolyState(*params, *next)
						|| SelectFastDepthMode(next, Type) != fast_depth)
					break;

				draw_counts[submitted_draws] = next->count;
				draw_offsets[submitted_draws] =
						(GLvoid *)(gl.get_index_size() * next->first);
				submitted_draws++;
			}
#endif
			LOWEND_PROFILE_COUNT(DrawSubmit, submitted_draws);
#if defined(FLYCAST_LOWEND_PROFILING)
			for (int i = 0; i < submitted_draws; i++)
				lowend_profile_count(Type == ListType_Opaque
						? LowendProfileStage::DrawOpaque
						: Type == ListType_Punch_Through
								? LowendProfileStage::DrawPunchThrough
								: LowendProfileStage::DrawTranslucent);
#endif
#if LOWEND_GLES_MULTI_DRAW
			if (submitted_draws > 1 && glMultiDrawElementsEXT != nullptr)
			{
				glMultiDrawElementsEXT(GL_TRIANGLE_STRIP, draw_counts, gl.index_type,
						draw_offsets, submitted_draws);
				params += submitted_draws - 1;
				count -= submitted_draws - 1;
			}
			else
#endif
			{
				glDrawElements(GL_TRIANGLE_STRIP, params->count, gl.index_type,
							(GLvoid*)(gl.get_index_size() * params->first));
				submitted_draws = 1;
			}
		}

		params++;
	}
}

static std::vector<SortTrigDrawParam> pidx_sort;

static void SortTriangles(int first, int count)
{
	std::vector<u32> vidx_sort;
	GenSorted(first, count, pidx_sort, vidx_sort);

	//Upload to GPU if needed
	if (!pidx_sort.empty())
	{
		//Bind and upload sorted index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.vbo.idxs2); glCheck();
		if (gl.index_type == GL_UNSIGNED_SHORT)
		{
			static bool overrun;
			static List<u16> short_vidx;
			if (short_vidx.daty != NULL)
				short_vidx.Free();
			short_vidx.Init(vidx_sort.size(), &overrun, NULL);
			for (u32 i = 0; i < vidx_sort.size(); i++)
				*(short_vidx.Append()) = vidx_sort[i];
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, short_vidx.bytes(), short_vidx.head(), GL_STREAM_DRAW);
		}
		else
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, vidx_sort.size() * sizeof(u32), &vidx_sort[0], GL_STREAM_DRAW);
	}
}

void DrawSorted(bool multipass)
{
	//if any drawing commands, draw them
	if (!pidx_sort.empty())
	{
		u32 count=pidx_sort.size();

		{
			//set some 'global' modes for all primitives

			glcache.Enable(GL_STENCIL_TEST);
			glcache.StencilFunc(GL_ALWAYS, 0, 0);
			glcache.StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

			for (u32 p=0; p<count; p++)
			{
				const PolyParam* params = pidx_sort[p].ppid;
				if (pidx_sort[p].count>2) //this actually happens for some games. No idea why ..
				{
					SetGPState<ListType_Translucent, true>(params,
							SelectFastDepthMode(params, ListType_Translucent));
					LOWEND_PROFILE_COUNT(DrawSubmit, 1);
					glDrawElements(GL_TRIANGLES, pidx_sort[p].count, gl.index_type,
						(GLvoid*)(gl.get_index_size() * pidx_sort[p].first));
				}
				params++;
			}

			if (multipass && settings.rend.TranslucentPolygonDepthMask)
			{
				// Write to the depth buffer now. The next render pass might need it. (Cosmic Smash)
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glcache.Disable(GL_BLEND);

				glcache.StencilMask(0);

				// We use the modifier volumes shader because it's fast. We don't need textures, etc.
				glcache.UseProgram(gl.modvol_shader.program);
				glUniform1f(gl.modvol_shader.sp_ShaderColor, 1.f);

				glcache.DepthFunc(GL_GEQUAL);
				glcache.DepthMask(GL_TRUE);

				for (u32 p = 0; p < count; p++)
				{
					const PolyParam* params = pidx_sort[p].ppid;
					if (pidx_sort[p].count > 2 && !params->isp.ZWriteDis) {
						// FIXME no clipping in modvol shader
						//SetTileClip(gp->tileclip,true);

						SetCull(params->isp.CullMode ^ gcflip);

						LOWEND_PROFILE_COUNT(DrawSubmit, 1);
						glDrawElements(GL_TRIANGLES, pidx_sort[p].count, gl.index_type,
							(GLvoid*)(gl.get_index_size() * pidx_sort[p].first));
					}
				}
				glcache.StencilMask(0xFF);
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			}

		}
		// Re-bind the previous index buffer for subsequent render passes
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.vbo.idxs);
	}
}

//All pixels are in area 0 by default.
//If inside an 'in' volume, they are in area 1
//if inside an 'out' volume, they are in area 0
/*
	Stencil bits:
		bit 7: mv affected (must be preserved)
		bit 1: current volume state
		but 0: summary result (starts off as 0)

	Lower 2 bits:

	IN volume (logical OR):
	00 -> 00
	01 -> 01
	10 -> 01
	11 -> 01

	Out volume (logical AND):
	00 -> 00
	01 -> 00
	10 -> 00
	11 -> 01
*/
void SetMVS_Mode(ModifierVolumeMode mv_mode, ISP_Modvol ispc)
{
	if (mv_mode == Xor)
	{
		// set states
		glcache.Enable(GL_DEPTH_TEST);
		// write only bit 1
		glcache.StencilMask(2);
		// no stencil testing
		glcache.StencilFunc(GL_ALWAYS, 0, 2);
		// count the number of pixels in front of the Z buffer (xor zpass)
		glcache.StencilOp(GL_KEEP, GL_KEEP, GL_INVERT);

		//Cull mode needs to be set
		SetCull(ispc.CullMode);
	}
	else if (mv_mode == Or)
	{
		// set states
		glcache.Enable(GL_DEPTH_TEST);
		// write only bit 1
		glcache.StencilMask(2);
		// no stencil testing
		glcache.StencilFunc(GL_ALWAYS, 2, 2);
		// Or'ing of all triangles
		glcache.StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		// Cull mode needs to be set
		SetCull(ispc.CullMode);
	}
	else
	{
		// Inclusion or Exclusion volume

		// no depth test
		glcache.Disable(GL_DEPTH_TEST);
		// write bits 1:0
		glcache.StencilMask(3);

		if (mv_mode == Inclusion)
		{
			// Inclusion volume
			//res : old : final 
			//0   : 0      : 00
			//0   : 1      : 01
			//1   : 0      : 01
			//1   : 1      : 01
			
			// if (1<=st) st=1; else st=0;
			glcache.StencilFunc(GL_LEQUAL, 1, 3);
			glcache.StencilOp(GL_ZERO, GL_ZERO, GL_REPLACE);
		}
		else
		{
			// Exclusion volume
			/*
				I've only seen a single game use it, so i guess it doesn't matter ? (Zombie revenge)
				(actually, i think there was also another, racing game)
			*/

			// The initial value for exclusion volumes is 1 so we need to invert the result before and'ing.
			//res : old : final 
			//0   : 0   : 00
			//0   : 1   : 01
			//1   : 0   : 00
			//1   : 1   : 00

			// if (1 == st) st = 1; else st = 0;
			glcache.StencilFunc(GL_EQUAL, 1, 3);
			glcache.StencilOp(GL_ZERO, GL_ZERO, GL_KEEP);
		}
	}
}

static void SetupMainVBO(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo.geometry);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.vbo.idxs);

	//setup vertex buffers attrib pointers
	glEnableVertexAttribArray(VERTEX_POS_ARRAY);
	glVertexAttribPointer(VERTEX_POS_ARRAY, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,x));

	glEnableVertexAttribArray(VERTEX_COL_BASE_ARRAY);
	glVertexAttribPointer(VERTEX_COL_BASE_ARRAY, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex,col));

	glEnableVertexAttribArray(VERTEX_COL_OFFS_ARRAY);
	glVertexAttribPointer(VERTEX_COL_OFFS_ARRAY, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex,vtx_spc));

	glEnableVertexAttribArray(VERTEX_UV_ARRAY);
	glVertexAttribPointer(VERTEX_UV_ARRAY, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,u));
}

static void SetupModvolVBO(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo.modvols);

	//setup vertex buffers attrib pointers
	glEnableVertexAttribArray(VERTEX_POS_ARRAY);
	glVertexAttribPointer(VERTEX_POS_ARRAY, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, (void*)0);

	glDisableVertexAttribArray(VERTEX_UV_ARRAY);
	glDisableVertexAttribArray(VERTEX_COL_OFFS_ARRAY);
	glDisableVertexAttribArray(VERTEX_COL_BASE_ARRAY);
}

static void DrawModVols(int first, int count)
{
	/* A bit of explanation:
	 * In theory it works like this: generate a 1-bit stencil for each polygon
	 * volume, and then AND or OR it against the overall 1-bit tile stencil at 
	 * the end of the volume. */

	if (count == 0)
		return;

	SetupModvolVBO();

	glcache.Disable(GL_BLEND);
	SetBaseClipping();

	glcache.UseProgram(gl.modvol_shader.program);
	glUniform1f(gl.modvol_shader.sp_ShaderColor, 1 - FPU_SHAD_SCALE.scale_factor / 256.f);

	glcache.Enable(GL_DEPTH_TEST);
	glcache.DepthMask(GL_FALSE);
	glcache.DepthFunc(GL_GREATER);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	ModifierVolumeParam* params = &pvrrc.global_param_mvo.head()[first];

	int mod_base = -1;

	for (int cmv = 0; cmv < count; cmv++)
	{
		ModifierVolumeParam& param = params[cmv];

		if (param.count == 0)
			continue;

		u32 mv_mode = param.isp.DepthMode;

		if (mod_base == -1)
			mod_base = param.first;

		if (!param.isp.VolumeLast && mv_mode > 0)
			SetMVS_Mode(Or, param.isp);		// OR'ing (open volume or quad)
		else
			SetMVS_Mode(Xor, param.isp);	// XOR'ing (closed volume)
		LOWEND_PROFILE_COUNT(DrawSubmit, 1);
		glDrawArrays(GL_TRIANGLES, param.first * 3, param.count * 3);

		if (mv_mode == 1 || mv_mode == 2)
		{
			// Sum the area
			SetMVS_Mode(mv_mode == 1 ? Inclusion : Exclusion, param.isp);
			LOWEND_PROFILE_COUNT(DrawSubmit, 1);
			glDrawArrays(GL_TRIANGLES, mod_base * 3, (param.first + param.count - mod_base) * 3);
			mod_base = -1;
		}
	}
	//disable culling
	SetCull(0);
	//enable color writes
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

	//black out any stencil with '1'
	glcache.Enable(GL_BLEND);
	glcache.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glcache.Enable(GL_STENCIL_TEST);
	glcache.StencilFunc(GL_EQUAL, 0x81, 0x81); //only pixels that are Modvol enabled, and in area 1

	//clear the stencil result bit
	glcache.StencilMask(0x3);    //write to lsb
	glcache.StencilOp(GL_ZERO, GL_ZERO, GL_ZERO);

	//don't do depth testing
	glcache.Disable(GL_DEPTH_TEST);

	SetupMainVBO();
	LOWEND_PROFILE_COUNT(DrawSubmit, 1);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	//restore states
	glcache.Enable(GL_DEPTH_TEST);
}

void DrawStrips()
{
	LOWEND_PROFILE_SCOPE_NO_COUNT(DrawSubmit);
	UpdateFastDepthGuard();
	SetupMainVBO();
	//Draw the strips !

	//We use sampler 0
	glActiveTexture(GL_TEXTURE0);

	RenderPass previous_pass = {};
	for (int render_pass = 0; render_pass < pvrrc.render_passes.used(); render_pass++)
	{
		const RenderPass& current_pass = pvrrc.render_passes.head()[render_pass];

		DEBUG_LOG(RENDERER, "Render pass %d OP %d PT %d TR %d MV %d", render_pass + 1,
			current_pass.op_count - previous_pass.op_count,
			current_pass.pt_count - previous_pass.pt_count,
			current_pass.tr_count - previous_pass.tr_count,
			current_pass.mvo_count - previous_pass.mvo_count);

		//initial state
		glcache.Enable(GL_DEPTH_TEST);
		glcache.DepthMask(GL_TRUE);

		//Opaque
		DrawList<ListType_Opaque, false>(pvrrc.global_param_op, 
			previous_pass.op_count, current_pass.op_count - previous_pass.op_count);

		//Alpha tested
		DrawList<ListType_Punch_Through, false>(pvrrc.global_param_pt,
			previous_pass.pt_count, current_pass.pt_count - previous_pass.pt_count);

		// Modifier volumes
		if (gl.stencil_present && settings.rend.ModifierVolumes)
			DrawModVols(previous_pass.mvo_count, current_pass.mvo_count - previous_pass.mvo_count);

		//Alpha blended
		{
			if (current_pass.autosort)
			{
				if (UsePerTriangleTranslucentSorting())
				{
					SortTriangles(previous_pass.tr_count, current_pass.tr_count - previous_pass.tr_count);
					DrawSorted(render_pass < pvrrc.render_passes.used() - 1);
				}
				else
				{
					if (settings.rend.TranslucentStripMerge == 0)
						SortPParams(previous_pass.tr_count, current_pass.tr_count - previous_pass.tr_count);
					DrawList<ListType_Translucent, true>(pvrrc.global_param_tr, previous_pass.tr_count, current_pass.tr_count - previous_pass.tr_count );
				}
			}
			else
				DrawList<ListType_Translucent, false>(pvrrc.global_param_tr, previous_pass.tr_count, current_pass.tr_count - previous_pass.tr_count);
		}

		previous_pass = current_pass;
		}

		vertex_buffer_unmap();
}

static void DrawQuad(GLuint texId, float x, float y, float w, float h, float u0, float v0, float u1, float v1)
{
	struct Vertex vertices[] = {
		{ x,     y + h, 0.1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, u0, v1 },
		{ x,     y,     0.1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, u0, v0 },
		{ x + w, y + h, 0.1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, u1, v1 },
		{ x + w, y,     0.1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, u1, v0 },
	};
	GLushort indices[] = { 0, 1, 2, 1, 3 };

	glcache.Disable(GL_SCISSOR_TEST);
	glcache.Disable(GL_DEPTH_TEST);
	glcache.Disable(GL_STENCIL_TEST);
	glcache.Disable(GL_CULL_FACE);
	glcache.Disable(GL_BLEND);

	ShaderUniforms.trilinear_alpha = 1.0;

	PipelineShader *shader = GetProgram(false, false, true, false, true, 0,
			false, 2, false, false, false, false, false,
			settings.rend.FastDepth >= 4 ? 3 : settings.rend.FastDepth);
	glcache.UseProgram(shader->program);

	glActiveTexture(GL_TEXTURE0);
	glcache.BindTexture(GL_TEXTURE_2D, texId);

	SetupMainVBO();
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

	LOWEND_PROFILE_COUNT(DrawSubmit, 1);
	glDrawElements(GL_TRIANGLE_STRIP, 5, GL_UNSIGNED_SHORT, (void *)0);
}

void DrawFramebuffer()
{
	DrawQuad(fbTextureId, 0, 0, 640.f, 480.f, 0, 0, 1, 1);
	glcache.DeleteTextures(1, &fbTextureId);
	fbTextureId = 0;
}

void UpdateVmuTexture(int vmu_screen_number)
{
	s32 x,y ;
	u8 temp_tex_buffer[VMU_SCREEN_HEIGHT*VMU_SCREEN_WIDTH*4];
	u8 *dst = temp_tex_buffer;
	u8 *src = NULL ;
	u8 *origsrc = NULL ;
	u8 vmu_pixel_on_R = vmu_screen_params[vmu_screen_number].vmu_pixel_on_R ;
	u8 vmu_pixel_on_G = vmu_screen_params[vmu_screen_number].vmu_pixel_on_G ;
	u8 vmu_pixel_on_B = vmu_screen_params[vmu_screen_number].vmu_pixel_on_B ;
	u8 vmu_pixel_off_R = vmu_screen_params[vmu_screen_number].vmu_pixel_off_R ;
	u8 vmu_pixel_off_G = vmu_screen_params[vmu_screen_number].vmu_pixel_off_G ;
	u8 vmu_pixel_off_B = vmu_screen_params[vmu_screen_number].vmu_pixel_off_B ;
	u8 vmu_screen_opacity = vmu_screen_params[vmu_screen_number].vmu_screen_opacity ;

	if (vmuTextureId[vmu_screen_number] == 0)
	{
		vmuTextureId[vmu_screen_number] = glcache.GenTexture();
		glcache.BindTexture(GL_TEXTURE_2D, vmuTextureId[vmu_screen_number]);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
		glcache.BindTexture(GL_TEXTURE_2D, vmuTextureId[vmu_screen_number]);


	origsrc = vmu_screen_params[vmu_screen_number].vmu_lcd_screen ;

	if ( origsrc == NULL )
		return ;


	for ( y = VMU_SCREEN_HEIGHT-1 ; y >= 0 ; y--)
	{
		src = origsrc + (y*VMU_SCREEN_WIDTH) ;

		for ( x = 0 ; x < VMU_SCREEN_WIDTH ; x++)
		{
			if ( *src++ > 0 )
			{
				*dst++ = vmu_pixel_on_R ;
				*dst++ = vmu_pixel_on_G ;
				*dst++ = vmu_pixel_on_B ;
				*dst++ = vmu_screen_opacity ;
			}
			else
			{
				*dst++ = vmu_pixel_off_R ;
				*dst++ = vmu_pixel_off_G ;
				*dst++ = vmu_pixel_off_B ;
				*dst++ = vmu_screen_opacity ;
			}
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VMU_SCREEN_WIDTH, VMU_SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp_tex_buffer);

	vmu_screen_params[vmu_screen_number].vmu_screen_needs_update = false ;

}

void DrawVmuTexture(u8 vmu_screen_number)
{
	glActiveTexture(GL_TEXTURE0);

	float x=0 ;
	float y=0 ;
	float w=VMU_SCREEN_WIDTH*vmu_screen_params[vmu_screen_number].vmu_screen_size_mult ;
	float h=VMU_SCREEN_HEIGHT*vmu_screen_params[vmu_screen_number].vmu_screen_size_mult ;

	if (vmu_screen_params[vmu_screen_number].vmu_screen_needs_update || vmuTextureId[vmu_screen_number] == 0)
		UpdateVmuTexture(vmu_screen_number) ;

	switch ( vmu_screen_params[vmu_screen_number].vmu_screen_position )
	{
		case UPPER_LEFT :
		{
			x = 0 ;
			y = 0 ;
			break ;
		}
		case UPPER_RIGHT :
		{
			x = 640-w ;
			y = 0 ;
			break ;
		}
		case LOWER_LEFT :
		{
			x = 0 ;
			y = 480-h ;
			break ;
		}
		case LOWER_RIGHT :
		{
			x = 640-w ;
			y = 480-h ;
			break ;
		}
	}

	glcache.BindTexture(GL_TEXTURE_2D, vmuTextureId[vmu_screen_number]);

	glcache.Disable(GL_SCISSOR_TEST);
	glcache.Disable(GL_DEPTH_TEST);
	glcache.Disable(GL_STENCIL_TEST);
	glcache.Disable(GL_CULL_FACE);
	glcache.Enable(GL_BLEND);
	glcache.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SetupMainVBO();
	PipelineShader *shader = GetProgram(0, false, 1, 1, 0, 0, 0, 2,
			false, false, false, false, false,
			settings.rend.FastDepth >= 4 ? 3 : settings.rend.FastDepth);
	glcache.UseProgram(shader->program);

	{
		struct Vertex vertices[] = {
				{ x,   y+h, 1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, 0, 1 },
				{ x,   y,   1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, 0, 0 },
				{ x+w, y+h, 1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, 1, 1 },
				{ x+w, y,   1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, 1, 0 },
		};
		GLushort indices[] = { 0, 1, 2, 1, 3 };

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
	}

	LOWEND_PROFILE_COUNT(DrawSubmit, 1);
	glDrawElements(GL_TRIANGLE_STRIP, 5, GL_UNSIGNED_SHORT, (void *)0);
}

void UpdateLightGunTexture(int port)
{
	s32 x,y ;
	u8 temp_tex_buffer[LIGHTGUN_CROSSHAIR_SIZE*LIGHTGUN_CROSSHAIR_SIZE*4];
	u8 *dst = temp_tex_buffer;
	u8 *src = NULL ;

	if (lightgunTextureId[port] == 0)
	{
		lightgunTextureId[port] = glcache.GenTexture();
		glcache.BindTexture(GL_TEXTURE_2D, lightgunTextureId[port]);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
		glcache.BindTexture(GL_TEXTURE_2D, lightgunTextureId[port]);

	u8* colour = &( lightgun_palette[ lightgun_params[port].colour * 3 ] );

	for ( y = LIGHTGUN_CROSSHAIR_SIZE-1 ; y >= 0 ; y--)
	{
		src = lightgun_img_crosshair + (y*LIGHTGUN_CROSSHAIR_SIZE) ;

		for ( x = 0 ; x < LIGHTGUN_CROSSHAIR_SIZE ; x++)
		{
			if ( src[x] )
			{
				*dst++ = colour[0] ;
				*dst++ = colour[1] ;
				*dst++ = colour[2] ;
				*dst++ = 0xFF ;
			}
			else
			{
				*dst++ = 0 ;
				*dst++ = 0 ;
				*dst++ = 0 ;
				*dst++ = 0 ;
			}
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LIGHTGUN_CROSSHAIR_SIZE, LIGHTGUN_CROSSHAIR_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp_tex_buffer);

	lightgun_params[port].dirty = false;
}

void DrawGunCrosshair(u8 port)
{
	if ( lightgun_params[port].offscreen || (lightgun_params[port].colour==0) )
		return;

	glActiveTexture(GL_TEXTURE0);

	float x=0;
	float y=0;
	float w=LIGHTGUN_CROSSHAIR_SIZE;
	float h=LIGHTGUN_CROSSHAIR_SIZE;

	x = lightgun_params[port].x - ( LIGHTGUN_CROSSHAIR_SIZE / 2 );
	y = lightgun_params[port].y - ( LIGHTGUN_CROSSHAIR_SIZE / 2 );

	if ( lightgun_params[port].dirty || lightgunTextureId[port] == 0)
		UpdateLightGunTexture(port);

	glcache.BindTexture(GL_TEXTURE_2D, lightgunTextureId[port]);

	glcache.Disable(GL_SCISSOR_TEST);
	glcache.Disable(GL_DEPTH_TEST);
	glcache.Disable(GL_STENCIL_TEST);
	glcache.Disable(GL_CULL_FACE);
	glcache.Enable(GL_BLEND);
	glcache.BlendFunc(GL_SRC_ALPHA, GL_ONE);

	SetupMainVBO();
	PipelineShader *shader = GetProgram(0, false, 1, 1, 0, 0, 0, 2,
			false, false, false, false, false,
			settings.rend.FastDepth >= 4 ? 3 : settings.rend.FastDepth);
	glcache.UseProgram(shader->program);

	{
		struct Vertex vertices[] = {
				{ x,   y+h, 1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, 0, 1 },
				{ x,   y,   1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, 0, 0 },
				{ x+w, y+h, 1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, 1, 1 },
				{ x+w, y,   1, { 255, 255, 255, 255 }, { 0, 0, 0, 0 }, 1, 0 },
		};
		GLushort indices[] = { 0, 1, 2, 1, 3 };

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
	}

	LOWEND_PROFILE_COUNT(DrawSubmit, 1);
	glDrawElements(GL_TRIANGLE_STRIP, 5, GL_UNSIGNED_SHORT, (void *)0);

	glcache.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
