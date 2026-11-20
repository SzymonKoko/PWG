#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// Version 2 maps GetProcessMemoryInfo to K32GetProcessMemoryInfo in kernel32, so no psapi.lib is needed.
#ifndef PSAPI_VERSION
#define PSAPI_VERSION 2
#endif
#include <windows.h>
#include <psapi.h>
#include <glad/glad.h>

#include "memoryStats.h"
#include "csvWriter.h"

#include <algorithm>
#include <cstring>

// GL_NVX_gpu_memory_info (not in every glad build).
#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#endif
#ifndef GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#endif

namespace
{
	struct Counters
	{
		std::atomic<uint64_t> liveBytes{ 0 };
		std::atomic<uint64_t> liveObjects{ 0 };
		std::atomic<uint64_t> peakBytes{ 0 };
		std::atomic<uint64_t> allocatedBytes{ 0 };
		std::atomic<uint64_t> releasedBytes{ 0 };
		std::atomic<uint64_t> orphanedBytes{ 0 };
		std::atomic<uint64_t> orphanedObjects{ 0 };
	};

	Counters s_counters[pwg::debug::MemoryCategoryCount];

	Counters& CountersOf(pwg::debug::MemoryCategory category)
	{
		return s_counters[static_cast<size_t>(category)];
	}

	void RaisePeak(Counters& counters, uint64_t live)
	{
		uint64_t peak = counters.peakBytes.load(std::memory_order_relaxed);
		while (live > peak && !counters.peakBytes.compare_exchange_weak(peak, live, std::memory_order_relaxed))
		{
		}
	}

	/** @brief True if the extension is in the extension list; checked once (needs the GL context on first use). */
	bool HasNvxMemoryInfo()
	{
		static int cached = -1;
		if (cached >= 0)
		{
			return cached == 1;
		}
		GLint count = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &count);
		bool found = false;
		for (GLint i = 0; i < count && !found; ++i)
		{
			const GLubyte* name = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
			found = name && std::strcmp(reinterpret_cast<const char*>(name), "GL_NVX_gpu_memory_info") == 0;
		}
		cached = found ? 1 : 0;
		return found;
	}
}

const char* pwg::debug::GetMemoryCategoryName(MemoryCategory category)
{
	switch (category)
	{
	case MemoryCategory::TEXTURE:		return "texture";
	case MemoryCategory::PARTICLE_SSBO:	return "particle_ssbo";
	case MemoryCategory::HEIGHT_PBO:	return "height_pbo";
	case MemoryCategory::INSTANCE_VBO:	return "instance_vbo";
	case MemoryCategory::COUNT:			break;
	}
	return "?";
}

void pwg::debug::MemoryStats::OnAlloc(MemoryCategory category, uint64_t bytes)
{
	Counters& counters = CountersOf(category);
	const uint64_t live = counters.liveBytes.fetch_add(bytes, std::memory_order_relaxed) + bytes;
	counters.liveObjects.fetch_add(1, std::memory_order_relaxed);
	counters.allocatedBytes.fetch_add(bytes, std::memory_order_relaxed);
	RaisePeak(counters, live);
}

void pwg::debug::MemoryStats::OnResize(MemoryCategory category, uint64_t oldBytes, uint64_t newBytes)
{
	if (oldBytes == newBytes)
	{
		return;
	}
	Counters& counters = CountersOf(category);
	counters.liveBytes.fetch_sub(oldBytes, std::memory_order_relaxed);
	counters.releasedBytes.fetch_add(oldBytes, std::memory_order_relaxed);
	const uint64_t live = counters.liveBytes.fetch_add(newBytes, std::memory_order_relaxed) + newBytes;
	counters.allocatedBytes.fetch_add(newBytes, std::memory_order_relaxed);
	RaisePeak(counters, live);
}

void pwg::debug::MemoryStats::OnRelease(MemoryCategory category, uint64_t bytes, bool glDeleted)
{
	Counters& counters = CountersOf(category);
	counters.liveBytes.fetch_sub(bytes, std::memory_order_relaxed);
	counters.liveObjects.fetch_sub(1, std::memory_order_relaxed);
	counters.releasedBytes.fetch_add(bytes, std::memory_order_relaxed);
	if (!glDeleted)
	{
		counters.orphanedBytes.fetch_add(bytes, std::memory_order_relaxed);
		counters.orphanedObjects.fetch_add(1, std::memory_order_relaxed);
	}
}

pwg::debug::MemoryCategoryTotals pwg::debug::MemoryStats::GetTotals(MemoryCategory category)
{
	const Counters& counters = CountersOf(category);
	MemoryCategoryTotals totals;
	totals.liveBytes = counters.liveBytes.load(std::memory_order_relaxed);
	totals.liveObjects = counters.liveObjects.load(std::memory_order_relaxed);
	totals.peakBytes = counters.peakBytes.load(std::memory_order_relaxed);
	totals.allocatedBytes = counters.allocatedBytes.load(std::memory_order_relaxed);
	totals.releasedBytes = counters.releasedBytes.load(std::memory_order_relaxed);
	totals.orphanedBytes = counters.orphanedBytes.load(std::memory_order_relaxed);
	totals.orphanedObjects = counters.orphanedObjects.load(std::memory_order_relaxed);
	return totals;
}

void pwg::debug::MemoryStats::ResetPeaks()
{
	for (Counters& counters : s_counters)
	{
		counters.peakBytes.store(counters.liveBytes.load(std::memory_order_relaxed), std::memory_order_relaxed);
	}
}

pwg::debug::ProcessMemory pwg::debug::MemoryStats::GetProcessMemory()
{
	ProcessMemory memory;
	PROCESS_MEMORY_COUNTERS_EX counters{};
	counters.cb = sizeof(counters);
	if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters)))
	{
		memory.available = true;
		memory.privateBytes = static_cast<uint64_t>(counters.PrivateUsage);
		memory.workingSetBytes = static_cast<uint64_t>(counters.WorkingSetSize);
		memory.peakWorkingSetBytes = static_cast<uint64_t>(counters.PeakWorkingSetSize);
	}
	return memory;
}

pwg::debug::GpuMemory pwg::debug::MemoryStats::QueryGpuMemory()
{
	GpuMemory memory;
	if (!HasNvxMemoryInfo())
	{
		return memory;
	}
	GLint dedicatedKb = 0;
	GLint availableKb = 0;
	glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedicatedKb);
	glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availableKb);
	if (dedicatedKb <= 0 || availableKb < 0)
	{
		return memory;		// The call did not return usable values.
	}
	memory.available = true;
	memory.dedicatedBytes = static_cast<uint64_t>(dedicatedKb) * 1024ull;
	memory.availableBytes = static_cast<uint64_t>(availableKb) * 1024ull;
	memory.usedBytes = memory.dedicatedBytes > memory.availableBytes ? memory.dedicatedBytes - memory.availableBytes : 0;
	return memory;
}

pwg::debug::MemorySnapshot pwg::debug::MemoryStats::TakeSnapshot()
{
	MemorySnapshot snapshot;
	for (size_t i = 0; i < MemoryCategoryCount; ++i)
	{
		snapshot.own[i] = GetTotals(static_cast<MemoryCategory>(i));
	}
	snapshot.process = GetProcessMemory();
	snapshot.gpu = QueryGpuMemory();
	return snapshot;
}

uint64_t pwg::debug::MemorySnapshot::OwnLiveBytes() const
{
	uint64_t sum = 0;
	for (const MemoryCategoryTotals& totals : own)
	{
		sum += totals.liveBytes;
	}
	return sum;
}

uint64_t pwg::debug::MemorySnapshot::OwnOrphanedBytes() const
{
	uint64_t sum = 0;
	for (const MemoryCategoryTotals& totals : own)
	{
		sum += totals.orphanedBytes;
	}
	return sum;
}

void pwg::debug::AppendMemorySnapshotRows(const MemorySnapshot& snapshot, const std::string& point, int64_t snapshotIndex, std::vector<EventRow>& out)
{
	auto add = [&](const char* source, const std::string& item, bool available, uint64_t bytes)
	{
		EventRow row;
		row.event = "memory_snapshot";
		row.index = snapshotIndex;
		row.detail = "point=" + point + ";item=" + item;
		row.metric = "bytes";
		row.value = available ? static_cast<double>(bytes) : NotAvailable;
		row.unit = "B";
		row.source = source;
		out.push_back(std::move(row));
	};

	for (size_t i = 0; i < MemoryCategoryCount; ++i)
	{
		add("own_accounting", std::string(GetMemoryCategoryName(static_cast<MemoryCategory>(i))) + "_live", true, snapshot.own[i].liveBytes);
	}
	add("own_accounting", "total_live", true, snapshot.OwnLiveBytes());
	add("own_accounting", "texture_orphaned", true, snapshot.own[static_cast<size_t>(MemoryCategory::TEXTURE)].orphanedBytes);

	add("os_process", "private_usage", snapshot.process.available, snapshot.process.privateBytes);
	add("os_process", "working_set", snapshot.process.available, snapshot.process.workingSetBytes);

	const char* gpuSource = snapshot.gpu.available ? "driver_nvx" : "unavailable";
	add(gpuSource, "vram_dedicated", snapshot.gpu.available, snapshot.gpu.dedicatedBytes);
	add(gpuSource, "vram_available", snapshot.gpu.available, snapshot.gpu.availableBytes);
	add(gpuSource, "vram_used", snapshot.gpu.available, snapshot.gpu.usedBytes);
}

uint32_t pwg::debug::BytesPerTexel(int internalFormat)
{
	switch (internalFormat)
	{
	case GL_R8: case GL_R8I: case GL_R8UI: case GL_R8_SNORM:
		return 1;
	case GL_RG8: case GL_R16F: case GL_R16: case GL_R16I: case GL_R16UI:
		return 2;
	case GL_RGB8: case GL_SRGB8:
		return 3;
	case GL_RGBA8: case GL_SRGB8_ALPHA8: case GL_RG16F: case GL_R32F: case GL_R32I: case GL_R32UI:
	case GL_DEPTH_COMPONENT32F: case GL_DEPTH_COMPONENT24: case GL_DEPTH24_STENCIL8:
		return 4;
	case GL_RGBA16F: case GL_RG32F: case GL_RGBA16:
		return 8;
	case GL_RGBA32F:
		return 16;
	default:
		return 0;
	}
}

uint64_t pwg::debug::Texture2DBytes(int internalFormat, int width, int height, bool mipmapped)
{
	if (width <= 0 || height <= 0)
	{
		return 0;
	}
	const uint64_t texel = BytesPerTexel(internalFormat);
	uint64_t total = 0;
	uint64_t w = static_cast<uint64_t>(width);
	uint64_t h = static_cast<uint64_t>(height);
	for (;;)
	{
		total += w * h * texel;
		if (!mipmapped || (w == 1 && h == 1))
		{
			break;
		}
		w = std::max<uint64_t>(1, w / 2);
		h = std::max<uint64_t>(1, h / 2);
	}
	return total;
}
