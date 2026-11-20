#ifndef SRC_DEBUG_MEMORYSTATS_H_
#define SRC_DEBUG_MEMORYSTATS_H_

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

// Set by CMake (option PWG_DEBUG_VIEW_INSTRUMENTATION, default ON). Same default as debugView.h.
#ifndef PWG_DEBUG_VIEW_INSTRUMENTATION
#define PWG_DEBUG_VIEW_INSTRUMENTATION 1
#endif

namespace pwg::debug
{
	struct EventRow;

	/**
	* @brief Allocation classes of the own accounting (design 5.7). Only these allocations are counted; every
	* other GL allocation of the engine (mesh vertex and index buffers, framebuffers, shadow maps, erosion
	* and water textures, cube maps, texture arrays, ...) is not.
	*/
	enum class MemoryCategory : uint8_t
	{
		TEXTURE,		/**< pwg::Texture objects: constructors, ~Texture and the re-upload paths. */
		PARTICLE_SSBO,	/**< GpuParticleErosion particle buffer. */
		HEIGHT_PBO,		/**< ChunkManager height readback pixel pack buffers. */
		INSTANCE_VBO,	/**< Mesh instance buffers (capacity, grown by DrawInstanced). */
		COUNT
	};

	constexpr size_t MemoryCategoryCount = static_cast<size_t>(MemoryCategory::COUNT);

	/** @brief Short name of a category, used in the UI and as `item=` in events.csv. */
	const char* GetMemoryCategoryName(MemoryCategory category);

	/** @brief Totals of one allocation class. Plain values, copied out of the atomic counters. */
	struct MemoryCategoryTotals
	{
		uint64_t liveBytes{ 0 };			/**< Bytes of objects that exist now. */
		uint64_t liveObjects{ 0 };
		uint64_t peakBytes{ 0 };			/**< Highest liveBytes seen since start (or since ResetPeaks). */
		uint64_t allocatedBytes{ 0 };		/**< Cumulative bytes allocated or re-allocated. */
		uint64_t releasedBytes{ 0 };		/**< Cumulative bytes released (object destroyed or storage replaced). */
		uint64_t orphanedBytes{ 0 };		/**< Bytes of destroyed objects whose GL storage was never deleted (see OnRelease). */
		uint64_t orphanedObjects{ 0 };
	};

	/** @brief Process memory from GetProcessMemoryInfo (label `os_process`). */
	struct ProcessMemory
	{
		bool available{ false };
		uint64_t privateBytes{ 0 };			/**< PrivateUsage (commit charge of the process). */
		uint64_t workingSetBytes{ 0 };		/**< WorkingSetSize (resident pages). */
		uint64_t peakWorkingSetBytes{ 0 };	/**< PeakWorkingSetSize. */
	};

	/** @brief Video memory from GL_NVX_gpu_memory_info (label `driver_nvx`, or `unavailable`). */
	struct GpuMemory
	{
		bool available{ false };
		uint64_t dedicatedBytes{ 0 };		/**< GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX. */
		uint64_t availableBytes{ 0 };		/**< GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX. */
		uint64_t usedBytes{ 0 };			/**< dedicated - available: every process and the driver, not only this one. */
	};

	/** @brief All three sources at one moment. */
	struct MemorySnapshot
	{
		MemoryCategoryTotals own[MemoryCategoryCount];
		ProcessMemory process;
		GpuMemory gpu;

		uint64_t OwnLiveBytes() const;
		uint64_t OwnOrphanedBytes() const;
	};

	/**
	* @brief Memory statistics of the Debug View (design 5.7): own accounting of texture and buffer bytes, process RAM
	* and driver VRAM. Each value comes from one method and is labelled with it; they are never merged.
	*
	* Own accounting: the allocation sites call OnAlloc, OnResize and OnRelease (through the PWG_MEM_* macros, which are
	* empty when PWG_DEBUG_VIEW_INSTRUMENTATION=OFF). The counters are relaxed atomics and count whether the Debug View
	* is enabled or not, because most allocations happen before the user switches it on. The numbers are nominal
	* sizes (width x height x bytes per texel of the internal format, plus the mip chain when mipmaps were generated;
	* buffer sizes as requested). They exclude driver padding, alignment and any copy the driver keeps.
	*
	* Process RAM and VRAM are read on request: GetProcessMemory works on any thread, QueryGpuMemory needs the GL
	* context (main thread).
	*/
	class MemoryStats
	{
	public:
		/** @brief A new object of `bytes` exists. */
		static void OnAlloc(MemoryCategory category, uint64_t bytes);

		/** @brief The storage of an existing object was replaced: `oldBytes` released, `newBytes` allocated. No-op if equal. */
		static void OnResize(MemoryCategory category, uint64_t oldBytes, uint64_t newBytes);

		/**
		* @brief An object of `bytes` was destroyed. `glDeleted` is false if its GL name was not deleted (the engine's
		* ~Texture does not call glDeleteTextures); the bytes then stay in the driver and are counted as orphaned.
		*/
		static void OnRelease(MemoryCategory category, uint64_t bytes, bool glDeleted);

		static MemoryCategoryTotals GetTotals(MemoryCategory category);

		/** @brief Sets every peak to the current live bytes. */
		static void ResetPeaks();

		static ProcessMemory GetProcessMemory();

		/** @brief Needs the GL context. `available` is false if the extension is not present. */
		static GpuMemory QueryGpuMemory();

		/** @brief Needs the GL context (for the VRAM part). */
		static MemorySnapshot TakeSnapshot();
	};

	/**
	* @brief Converts a snapshot into events.csv rows of event `memory_snapshot` (design 7.3): `metric` bytes, `unit` B,
	* `source` own_accounting | os_process | driver_nvx | unavailable, `detail` `point=<where>;item=<value>`.
	* `engineFrame` and `sequence` are set by the caller; the rows of one snapshot get `index` = `snapshotIndex`.
	*/
	void AppendMemorySnapshotRows(const MemorySnapshot& snapshot, const std::string& point, int64_t snapshotIndex, std::vector<EventRow>& out);

	/** @brief Bytes of one level-0 texel of a sized internal format; 0 for a format the accounting does not know. */
	uint32_t BytesPerTexel(int internalFormat);

	/** @brief Bytes of a 2D texture: level 0, plus the full mip chain (each level halved, rounded down, to 1 x 1) if `mipmapped`. */
	uint64_t Texture2DBytes(int internalFormat, int width, int height, bool mipmapped);
} // namespace pwg::debug

#if PWG_DEBUG_VIEW_INSTRUMENTATION
#define PWG_MEM_ALLOC(category, bytes) ::pwg::debug::MemoryStats::OnAlloc(::pwg::debug::MemoryCategory::category, static_cast<uint64_t>(bytes))
#define PWG_MEM_RESIZE(category, oldBytes, newBytes) ::pwg::debug::MemoryStats::OnResize(::pwg::debug::MemoryCategory::category, static_cast<uint64_t>(oldBytes), static_cast<uint64_t>(newBytes))
#define PWG_MEM_RELEASE(category, bytes, glDeleted) ::pwg::debug::MemoryStats::OnRelease(::pwg::debug::MemoryCategory::category, static_cast<uint64_t>(bytes), glDeleted)
/** @brief Declares a member that only exists when the accounting is compiled in, e.g. PWG_MEM_MEMBER(uint64_t m_bytes{ 0 };). */
#define PWG_MEM_MEMBER(declaration) declaration
/** @brief Runs `statement` only when the accounting is compiled in. */
#define PWG_MEM_DO(statement) do { statement; } while (0)
#else
#define PWG_MEM_ALLOC(category, bytes) ((void)0)
#define PWG_MEM_RESIZE(category, oldBytes, newBytes) ((void)0)
#define PWG_MEM_RELEASE(category, bytes, glDeleted) ((void)0)
#define PWG_MEM_MEMBER(declaration)
#define PWG_MEM_DO(statement) ((void)0)
#endif

#endif // !SRC_DEBUG_MEMORYSTATS_H_
