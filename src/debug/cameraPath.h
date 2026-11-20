#ifndef SRC_DEBUG_CAMERAPATH_H_
#define SRC_DEBUG_CAMERAPATH_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace pwg::debug
{
	/** @brief Pose of the flyover camera: position, yaw and pitch in degrees, gimbal roll in radians (design 7.6). */
	struct CameraPathPose
	{
		glm::vec3 position{ 0.0f };
		float yawDeg{ 0.0f };
		float pitchDeg{ 0.0f };
		float rollRad{ 0.0f };
	};

	/** @brief One key of a path: time in seconds from the start of the recording, and the pose at that time. */
	struct CameraKey
	{
		float t{ 0.0f };
		CameraPathPose pose;
	};

	/** @brief Projection values that, with the pose and the aspect ratio, fix the matrices of the flyover camera. */
	struct CameraPathProjection
	{
		float fovDeg{ 60.0f };
		float nearPlane{ 0.01f };
		float farPlane{ 20000.0f };
	};

	/** @brief Options of one playback. */
	struct CameraPathPlayback
	{
		double stepSeconds{ 1.0 / 60.0 };			/**< Time between two path frames (design 7.6: default 1/60 s). */
		std::filesystem::path matrixDumpFile;		/**< If not empty: write the matrix dump here when the playback ends. */
	};

	/** @brief Result of the matrix dump of the last playback. */
	struct MatrixDumpResult
	{
		bool written{ false };
		std::filesystem::path file;
		uint64_t frames{ 0 };						/**< Number of path frames in the file. */
		std::string sha256;							/**< SHA-256 of the file; two playbacks with identical dumps have identical values. */
		std::string error;
	};

	/**
	* @brief Camera path of the flyover camera: record, play back at a fixed time step, save and load (D-10,
	* design section 7.6). Owned by DebugView; EditorScene calls it once per frame. No GL and no UI in this class.
	*
	* Recording: AddSample once per frame stores the pose with the time from the first sample (steady_clock).
	* Yaw is unwrapped, so interpolation never jumps by 360 degrees.
	*
	* Playback is indexed by frame, not by wall time: path frame k has t = k * step, and its pose is the linear
	* interpolation of the two surrounding keys (position, yaw, pitch, roll component-wise). The pose of frame k is
	* therefore the same in every run, whatever the frame rate. The playback has `GetFrameCount()` frames and ends
	* when TickPlayback is called for the first frame after them.
	*
	* Matrix dump (optional): for every path frame the view and projection matrix the scene was rendered with, as hex
	* floats (`%a`), kept in memory during the playback and written when it ends, so there is no file I/O inside it.
	*/
	class CameraPath
	{
	public:
		using Clock = std::chrono::steady_clock;

		enum class State : uint8_t
		{
			IDLE,
			RECORDING,
			PLAYING
		};

		static constexpr const char* FormatName = "pwg-camera-path";
		static constexpr int FormatVersion = 1;
		static constexpr const char* FileExtension = ".pwgpath.yaml";
		static constexpr const char* FolderName = "paths";			/**< Sub-folder of a capture root or run folder (design 7.1). */
		static constexpr const char* MatrixDumpFormatName = "pwg-camera-path-matrices";
		static constexpr size_t MaxKeys = 2'000'000;				/**< Recording stops here (about 56 MB). */
		static constexpr float MaxPitchDegrees = 89.0f;				/**< Same range as FlyoverCamera::MaxPitchDegrees. */

		State GetState() const { return m_state; }
		bool IsRecording() const { return m_state == State::RECORDING; }
		bool IsPlaying() const { return m_state == State::PLAYING; }
		bool IsIdle() const { return m_state == State::IDLE; }

		// ---- Path data ----

		const std::vector<CameraKey>& GetKeys() const { return m_keys; }
		size_t GetKeyCount() const { return m_keys.size(); }
		/** @brief Time of the last key in seconds, 0 without keys. */
		double GetDuration() const { return m_keys.empty() ? 0.0 : static_cast<double>(m_keys.back().t); }
		/** @brief True if there are at least two keys, the minimum for a playback. */
		bool HasPath() const { return m_keys.size() >= 2; }
		const CameraPathProjection& GetProjection() const { return m_projection; }
		/** @brief File of the last Save or Load; empty for a new recording. */
		const std::filesystem::path& GetFile() const { return m_file; }
		/** @brief SHA-256 of the path as saved (Save), as read (Load) or as it would be saved (recording). Empty without keys. */
		const std::string& GetSha256() const { return m_sha256; }

		/** @brief Drops the keys. Refused (false) while recording or playing. */
		bool Clear();

		/** @brief Pose at time `t` (seconds): linear interpolation, clamped to the first and last key. Needs keys. */
		CameraPathPose Evaluate(double t) const;

		/** @brief Number of frames of a path of `durationSeconds` at `stepSeconds`: floor(duration / step) + 1. */
		static uint64_t FrameCount(double durationSeconds, double stepSeconds);

		// ---- Recording ----

		/**
		* @brief Starts a new recording; the previous keys are dropped. Refused while recording or playing. The
		* recording stays empty until the first AddSample, whose time is 0.
		*/
		bool StartRecording();

		/**
		* @brief Sets the projection values stored with the path (fov, near, far of the flyover camera). The caller sets
		* them before the first AddSample; later changes of the camera's fov during the recording are not recorded.
		*/
		void SetProjection(const CameraPathProjection& projection) { if (m_state == State::RECORDING) m_projection = projection; }

		/**
		* @brief Adds a key at `time` (time of the first key is 0). Returns false if the sample was not stored: no
		* recording, time not later than the last key, or the key limit was reached (which also stops the recording).
		*/
		bool AddSample(const CameraPathPose& pose, Clock::time_point time);

		/** @brief Ends the recording and computes the SHA-256 of the path. */
		void StopRecording();

		// ---- Playback ----

		/** @brief Starts a playback with frame 0 at the next TickPlayback. Needs HasPath() and an idle state. */
		bool StartPlayback(const CameraPathPlayback& options, std::string& error);

		/**
		* @brief Gives the pose of the next path frame. Returns false if there is none: no playback, or the frame after
		* the last one (the playback ends and the matrix dump is written). Call once per frame, before the scene renders.
		*/
		bool TickPlayback(CameraPathPose& pose);

		/** @brief Ends the playback after the frames shown so far and writes the matrix dump. */
		void StopPlayback();

		/** @brief Path frame whose pose was applied last, -1 before the first one and while not playing. */
		int64_t GetAppliedFrame() const { return m_state == State::PLAYING ? m_appliedFrame : -1; }
		uint64_t GetFrameCount() const { return m_frameCount; }
		double GetStep() const { return m_step; }

		/**
		* @brief Matrix dump: called after the scene's camera got its final matrices for the frame, with the size the
		* scene is rendered at. Stores a row for the current path frame; no effect without an active dump, and only one
		* row per path frame.
		*/
		void NoteRenderedMatrices(const glm::mat4& view, const glm::mat4& projection, int width, int height);

		bool HasDumpResult() const { return m_hasDumpResult; }
		const MatrixDumpResult& GetLastDump() const { return m_lastDump; }

		// ---- Files ----

		/** @brief Text of the path file (YAML, floats with 9 significant digits). */
		std::string Serialize() const;

		/**
		* @brief Writes `file` (parent folders are created). Refused while recording or playing, without keys, and, unless
		* `overwrite`, if the file exists. On success the path remembers the file and its SHA-256.
		*/
		bool Save(const std::filesystem::path& file, bool overwrite, std::string& error);

		/** @brief Reads and checks `file`. The current path is replaced only if everything is valid. Refused while active. */
		bool Load(const std::filesystem::path& file, std::string& error);

	private:
		void FinishPlayback();
		bool WriteDump();

		State m_state{ State::IDLE };
		std::vector<CameraKey> m_keys;
		CameraPathProjection m_projection;
		std::filesystem::path m_file;
		std::string m_sha256;

		Clock::time_point m_recordStart{};

		double m_step{ 1.0 / 60.0 };
		uint64_t m_frameCount{ 0 };
		uint64_t m_nextFrame{ 0 };
		int64_t m_appliedFrame{ -1 };

		std::filesystem::path m_dumpFile;			/**< Empty: no dump in this playback. */
		std::string m_dumpRows;
		int64_t m_dumpedFrame{ -1 };
		uint64_t m_dumpFrames{ 0 };
		int m_dumpWidth{ 0 };
		int m_dumpHeight{ 0 };
		bool m_dumpSizeChanged{ false };
		MatrixDumpResult m_lastDump;
		bool m_hasDumpResult{ false };
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_CAMERAPATH_H_
