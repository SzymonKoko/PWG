#ifndef SRC_DEBUG_EXPORTERS_H_
#define SRC_DEBUG_EXPORTERS_H_

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "debug/runMetadata.h"

namespace pwg
{
	class ChunkManager;
}

namespace pwg::debug
{
	// ---------------------------------------------------------------------------------------------
	// Shared helpers of the capture exporters (screenshot D-07, mask export D-08)
	// ---------------------------------------------------------------------------------------------

	/** @brief Sub-folders of a capture root (design 7.1: a run folder holds `screenshots/` and `masks/`). */
	inline constexpr const char* ScreenshotFolderName = "screenshots";
	inline constexpr const char* MaskFolderName = "masks";

	/**
	* @brief File name part from free text: ASCII letters, digits, '.' and '-' are kept, every other character
	* (including '_', the separator of the name fields) becomes '-'. An empty result becomes `fallback`.
	*/
	std::string SanitizeFileNamePart(const std::string& text, const char* fallback);

	/** @brief Project name for file names: stem of `project_path` in the metadata, or "untitled". */
	std::string ProjectNameFromMetadata(const Metadata& metadata);

	/** @brief Local time as yyyyMMdd-HHmmss (design 7.5). */
	std::string FormatFileTimestamp(std::chrono::system_clock::time_point time);

	/**
	* @brief Returns `<folder>/<stem><extension>`, or `<folder>/<stem>-<n><extension>` (n = 2, 3, ...) if a file
	* with any of `extensions` and that stem already exists, so a capture never overwrites an earlier one.
	* `stem` is split at `suffixPosition` (the counter goes there); npos puts it at the end.
	*/
	std::string UniqueStem(const std::filesystem::path& folder, const std::string& stem, size_t suffixPosition,
		const std::vector<const char*>& extensions);

	/** @brief Reverses the order of `rows` rows of `rowBytes` bytes in place (GL row 0 is the bottom row). */
	void FlipRowsVertically(uint8_t* data, size_t rowBytes, size_t rows);

	/**
	* @brief Writes an 8-bit PNG with stb_image_write. The file is opened through std::filesystem, so non-ASCII
	* paths work. `data` holds `height` rows of `strideBytes` bytes, top row first.
	*/
	bool WritePng(const std::filesystem::path& path, int width, int height, int channels, const uint8_t* data, int strideBytes);

	// ---------------------------------------------------------------------------------------------
	// Screenshot export (D-07, design 7.5)
	// ---------------------------------------------------------------------------------------------

	/**
	* @brief Pose and projection of the camera that rendered the captured frame.
	*
	* Every value is derived from the matrices the frame was rendered with, so the definition is the same for
	* all camera types. Angle convention of FlyoverCamera: forward = (cos p cos y, sin p, cos p sin y);
	* roll turns the up vector towards `right = normalize(cross(forward, worldUp))`.
	*/
	struct CameraPose
	{
		std::string camera;							/**< editor, flyover or player */
		glm::vec3 position{ 0.0f };					/**< Eye position (inverse view matrix). */
		glm::vec3 forward{ 0.0f, 0.0f, -1.0f };
		glm::vec3 up{ 0.0f, 1.0f, 0.0f };
		float yawDeg{ 0.0f };						/**< atan2(forward.z, forward.x), in (-180, 180]. */
		float pitchDeg{ 0.0f };						/**< asin(forward.y). */
		float rollDeg{ 0.0f };						/**< 0 for a camera whose up vector lies in the forward/world-up plane. */
		float fovDeg{ 0.0f };						/**< Vertical field of view. */
		float nearPlane{ 0.0f };
		float farPlane{ 0.0f };
		float aspectRatio{ 0.0f };
		glm::mat4 view{ 1.0f };
		glm::mat4 projection{ 1.0f };
	};

	/** @brief Fills the derived values of a pose from the view matrix and the camera properties. */
	CameraPose MakeCameraPose(const std::string& camera, const glm::mat4& view, const glm::mat4& projection,
		float fovDeg, float nearPlane, float farPlane, float aspectRatio);

	/** @brief Frame to capture, provided by EditorScene::Draw right after post-processing. */
	struct ScreenshotSource
	{
		unsigned int texture{ 0 };					/**< LDR colour texture of the scene framebuffer (GL_RGBA8). */
		int width{ 0 };
		int height{ 0 };
		CameraPose camera;
		std::string debugMode{ "none" };			/**< Terrain debug view in use ("none" or the entry name). */
		bool wireframe{ false };
	};

	/** @brief A screenshot to take at the next frame. */
	struct ScreenshotRequest
	{
		std::filesystem::path root;					/**< Capture root; the PNG goes to `<root>/screenshots/`. */
		std::string tag;							/**< Optional `_<tag>` at the end of the file name. */
		RunInfo run;								/**< Run identifiers for the metadata (the runner passes its own). */
	};

	struct ScreenshotResult
	{
		bool written{ false };
		std::filesystem::path png;
		std::filesystem::path sidecar;
		int width{ 0 };
		int height{ 0 };
		std::string pixelsSha256;					/**< SHA-256 of the RGBA rows as written (top row first). */
		double readbackMs{ 0.0 };					/**< glGetTextureImage, synchronous. */
		double writeMs{ 0.0 };						/**< Flip, hash, PNG encode and write, sidecar. */
		uint64_t engineFrame{ 0 };					/**< Debug View frame index of the captured frame. */
		std::string error;
	};

	// ---------------------------------------------------------------------------------------------
	// Mask and heightmap export (D-08, design 7.4)
	// ---------------------------------------------------------------------------------------------

	/** @brief Masks to export now, from ChunkManager::GetMasks(). */
	struct MaskExportRequest
	{
		std::filesystem::path root;					/**< Capture or run root; the files go to `<root>/masks/`. */
		std::vector<std::string> names;				/**< Keys of ChunkManager::GetMasks(), exported in this order. */
		std::string tag;							/**< Optional `_<tag>` after the mask name (e.g. a pipeline stage). */
		RunInfo run;								/**< Run identifiers for the metadata (the runner passes its own). */
		bool writePreview{ true };					/**< Write the 8-bit PNG preview. */
	};

	/** @brief Graph node and output port that produced a mask (null in the sidecar when not known). */
	struct MaskSourceNode
	{
		bool known{ false };
		int id{ 0 };
		std::string type;
		std::string port;
	};

	struct MaskExportResult
	{
		bool written{ false };						/**< Raw file, sidecar and (if asked) preview written, round trip identical. */
		std::string name;							/**< Mask key in ChunkManager::GetMasks(). */
		std::filesystem::path raw;					/**< `<stem>.f32` */
		std::filesystem::path sidecar;				/**< `<stem>.yaml` */
		std::filesystem::path png;					/**< `<stem>.png`, empty if no preview was written. */
		int width{ 0 };
		int height{ 0 };
		int channels{ 0 };
		std::string glInternalFormat;				/**< e.g. R32F, as stored on the GPU. */
		uint64_t byteSize{ 0 };
		float min{ 0.0f };							/**< Over all finite values of all channels. */
		float max{ 0.0f };
		std::vector<float> channelMin;				/**< Per channel (multi-channel masks; also filled for one channel). */
		std::vector<float> channelMax;
		uint64_t nonFinite{ 0 };					/**< NaN or infinite values (excluded from min/max). */
		std::string sha256;							/**< SHA-256 of the exported buffer (= of the .f32 file). */
		bool roundTripIdentical{ false };			/**< The .f32 read back equals the buffer byte for byte. */
		MaskSourceNode source;
		std::vector<std::string> aliases;			/**< Other keys of GetMasks() that share this texture. */
		uint64_t generationRevision{ 0 };
		uint64_t engineFrame{ 0 };
		double readbackMs{ 0.0 };					/**< glGetTextureImage, synchronous. */
		double rawWriteMs{ 0.0 };					/**< Statistics, hash and the one binary write of the .f32. */
		double verifyMs{ 0.0 };						/**< Reading the .f32 back and comparing. */
		double previewMs{ 0.0 };					/**< Normalise, flip and PNG encode/write. */
		std::string error;
	};

	/**
	* @brief Capture exporters of the Debug View. Owned by DebugView; the Capture tab and the experiment
	* runner (D-11) use them through DebugView.
	*
	* Screenshot: a request is queued (RequestScreenshot) and taken in the next EditorScene::Draw, right after
	* the post-process pass and before ImGui renders, so the image holds the scene only. The readback is
	* synchronous; DebugView refuses requests while a CSV window is recorded (design principle 6).
	*
	* Mask export (D-08): runs at once when called (no queue), on the main GL context. Each mask is read with
	* glGetTextureImage as float32, written as raw `.f32`, read back and compared
	* byte for byte, then described in a YAML sidecar and previewed as an 8-bit PNG. Also synchronous, so
	* DebugView refuses it while a CSV window is recorded.
	*/
	class Exporters
	{
	public:
		static constexpr const char* ScreenshotFormatName = "pwg-screenshot";
		static constexpr int ScreenshotFormatVersion = 1;
		static constexpr const char* MaskFormatName = "pwg-mask";
		static constexpr int MaskFormatVersion = 1;

		/**
		* @brief Exports the masks named in `request` from `chunks` (design 7.4). Main GL context. `metadata` is
		* the run header for the export time `time`. Returns one result per requested name (also for failures);
		* the same list is kept as GetLastMaskExport().
		*/
		const std::vector<MaskExportResult>& ExportMasks(ChunkManager& chunks, const MaskExportRequest& request,
			const Metadata& metadata, std::chrono::system_clock::time_point time, uint64_t engineFrame);

		bool HasMaskExportResult() const { return m_hasMaskResult; }
		const std::vector<MaskExportResult>& GetLastMaskExport() const { return m_lastMaskExport; }

		/** @brief Queues a screenshot. Returns false if one is already pending. */
		bool RequestScreenshot(ScreenshotRequest request);
		bool HasPendingScreenshot() const { return m_pendingScreenshot.has_value(); }
		const ScreenshotRequest* GetPendingScreenshot() const { return m_pendingScreenshot ? &*m_pendingScreenshot : nullptr; }
		void CancelScreenshot() { m_pendingScreenshot.reset(); }

		/**
		* @brief Takes the pending screenshot from `source`: PNG and YAML sidecar. Main GL context, outside any
		* framebuffer that renders to `source.texture`. `metadata` is the run header (DebugView::CollectRunMetadata)
		* for the capture time `time`.
		*/
		const ScreenshotResult& CaptureScreenshot(const ScreenshotSource& source, const Metadata& metadata,
			std::chrono::system_clock::time_point time, uint64_t engineFrame);

		bool HasScreenshotResult() const { return m_hasScreenshotResult; }
		const ScreenshotResult& GetLastScreenshot() const { return m_lastScreenshot; }

	private:
		std::optional<ScreenshotRequest> m_pendingScreenshot;
		ScreenshotResult m_lastScreenshot;
		bool m_hasScreenshotResult{ false };
		std::vector<uint8_t> m_pixelScratch;		/**< Reused readback buffer. */

		/** @brief Exports one mask; called by ExportMasks. */
		MaskExportResult ExportMask(ChunkManager& chunks, const std::string& name, const MaskExportRequest& request,
			const Metadata& metadata, std::chrono::system_clock::time_point time, uint64_t engineFrame);

		std::vector<MaskExportResult> m_lastMaskExport;
		bool m_hasMaskResult{ false };
		std::vector<float> m_maskScratch;			/**< Reused mask readback buffer. */
		std::vector<uint8_t> m_maskVerifyScratch;	/**< Reused buffer for the .f32 read-back. */
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_EXPORTERS_H_
