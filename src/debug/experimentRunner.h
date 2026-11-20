#ifndef SRC_DEBUG_EXPERIMENTRUNNER_H_
#define SRC_DEBUG_EXPERIMENTRUNNER_H_

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include "debug/exporters.h"
#include "debug/runMetadata.h"

namespace pwg
{
	class ChunkManager;
	class EditorScene;
	class Gui;
	class SceneManager;
	class Window;
}

namespace YAML
{
	class Emitter;
}

namespace pwg::debug
{
	/** @brief What the runner needs from the application: the window, the editor's GUI (project load/save) and the scenes. */
	struct ExperimentHost
	{
		Window& window;
		Gui& gui;
		SceneManager& scene;
	};

	/** @brief Value of a config parameter: bool, number, string or a list of numbers. */
	struct ExperimentValue
	{
		enum class Kind : uint8_t
		{
			BOOL,
			NUMBER,
			STRING,
			LIST
		};

		Kind kind{ Kind::NUMBER };
		bool boolean{ false };
		double number{ 0.0 };
		std::string text;				/**< The scalar as written in the config; for a list, the numbers joined by '/'. */
		std::vector<double> list;
	};

	using ExperimentParam = std::pair<std::string, ExperimentValue>;

	/** @brief One entry of `prepare:`. */
	struct ExperimentStep
	{
		enum class Type : uint8_t
		{
			RUN_GRAPH,			/**< `run_graph: true`: ChunkManager::RunUniversalGraph. */
			REFRESH_ASSETS,		/**< `refresh_assets: true`: ChunkManager::RefreshProceduralAssets (vegetation layers). */
			HYDROLOGY,			/**< `hydrology: generate`: starts the worker, waits for the result. */
			HYDRAULIC,			/**< `hydraulic: { steps: N, per_frame: K }`: fixed-step mode. */
			THERMAL,			/**< `thermal: { steps: N | until: stable, max_steps: M, per_frame: K }`. */
			SAVE,				/**< `save: { file: name.pwg }`: a copy of the project in the run folder. */
			RELOAD,				/**< `reload: { file: name.pwg }` or `reload: project`: load a project again. */
			WAIT				/**< `wait: { frames: N }`. */
		};

		Type type{ Type::RUN_GRAPH };
		std::string label;				/**< `name:` of the entry; captures `after: <label>` refer to it. Default `<type><index>`. */
		int steps{ 0 };
		int perFrame{ 1 };
		bool untilStable{ false };
		int maxSteps{ 0 };
		int frames{ 0 };
		std::string file;				/**< save / reload file, relative to the run folder (reload without file: the project). */

		/** @brief One line describing the entry, written to run.yaml. */
		std::string Describe() const;
	};

	/** @brief One entry of `captures:`. */
	struct ExperimentCapture
	{
		std::string after{ "prepare" };	/**< `prepare`, `measure` or the label of a prepare entry. */
		bool screenshot{ false };
		int pathFrame{ -1 };			/**< Path frame to show; -1 = the camera as it is. */
		std::string tag;
		std::vector<std::string> masks;	/**< Keys of ChunkManager::GetMasks(). */
		bool preview{ true };			/**< PNG preview of the masks. */
	};

	struct ExperimentVariant
	{
		std::string name;
		std::vector<ExperimentParam> params;
	};

	/** @brief Parsed experiment config (design 8.2), with defaults filled in. */
	struct ExperimentConfig
	{
		std::filesystem::path file;			/**< Absolute path of the config. */
		std::string fileSha256;
		std::string experimentId;
		std::string description;
		std::filesystem::path project;		/**< Absolute path. */
		std::string expectRenderer;
		std::filesystem::path output;		/**< Absolute path of the experiment folder (`--out` overrides). */
		bool debugView{ true };

		bool benchmarkEnabled{ true };
		int renderWidth{ 1920 };
		int renderHeight{ 1080 };
		bool fullscreenMainView{ false };
		int gpuRingFrames{ 4 };
		bool wallclockBracket{ false };
		uint64_t maxGpuDropped{ 0 };

		std::vector<std::pair<std::string, std::string>> manual;
		std::vector<ExperimentParam> baseParams;
		std::vector<ExperimentStep> prepare;
		std::vector<ExperimentCapture> captures;

		std::filesystem::path cameraPath;	/**< Empty: no path. */
		double cameraStep{ 1.0 / 60.0 };
		bool cameraLoop{ false };
		bool matrixDump{ false };
		bool memorySnapshots{ false };		/**< D-04: add `memory_snapshot` rows at the points of the CSV window (debug_view_runner.md). */

		uint64_t warmupFrames{ 0 };
		uint64_t measureFrames{ 0 };
		enum class Record : uint8_t
		{
			NONE,
			PREPARE,		/**< The CSV window covers the prepare steps (events of E-05 … E-09). */
			MEASURE			/**< The CSV window covers the measure frames (E-10 … E-12). */
		};
		Record record{ Record::MEASURE };
		int repetitions{ 1 };
		bool interleaved{ true };
		double cooldownSeconds{ 0.0 };
		std::vector<ExperimentVariant> variants;
	};

	/**
	* @brief Reads and checks an experiment config. Unknown keys, unknown parameters and wrong types are errors, so a run
	* stops before it starts (design 8.3). Returns false with a reason in `error`.
	*/
	bool LoadExperimentConfig(const std::filesystem::path& file, const std::filesystem::path& outputOverride,
		ExperimentConfig& config, std::string& error);

	/**
	* @brief Experiment runner (D-11, design section 8): loads a YAML config and drives the editor through
	* load project, apply parameters, prepare, captures, warm-up, measure, export, for every variant and repetition.
	*
	* It never runs its own loop. Application::Run calls Tick() once per frame, after the frame ended, so rendering,
	* ImGui and the query harvest keep working during a run. One step happens per tick; waiting steps count frames.
	* Work that must not fall into a measurement window (project load, graph and hydrology runs outside a recorded
	* prepare phase, screenshots, mask exports, file writes) runs between windows. The CSV files come from
	* DebugView::StartRecording (D-09): one window per (variant, repetition), written to `<output>/<run id>/`.
	* Main thread only; works in every build (instrumentation off: the files hold the frame timing only).
	*/
	class ExperimentRunner
	{
	public:
		using Clock = std::chrono::steady_clock;

		enum class State : uint8_t
		{
			IDLE,
			RUNNING,
			DONE,			/**< Every unit ran; some may have failed (see GetFailedUnits). */
			ABORTED,
			REJECTED		/**< The config was not accepted; nothing ran. */
		};

		explicit ExperimentRunner(ExperimentHost host);
		~ExperimentRunner();
		ExperimentRunner(const ExperimentRunner&) = delete;
		ExperimentRunner& operator=(const ExperimentRunner&) = delete;

		/**
		* @brief Loads `configFile` and starts the run with the next Tick. `outputOverride` replaces `output:` when not empty.
		* With `exitWhenDone` the application closes after the last unit. Returns false with a reason if the config is not
		* accepted or a run is active.
		*/
		bool Start(const std::filesystem::path& configFile, const std::filesystem::path& outputOverride, bool exitWhenDone, std::string& error);

		/** @brief One step of the state machine; called once per frame after the frame ended. */
		void Tick();

		/** @brief Stops the run: the current unit is closed as aborted and the rest is skipped. */
		void Abort();

		bool IsRunning() const { return m_state == State::RUNNING; }
		State GetState() const { return m_state; }

		/** @brief 0 if the run finished and every unit was complete, 1 otherwise (config rejected, aborted, a unit failed). */
		int GetExitCode() const;

		/** @brief True once after a run asked for the application to close; Application polls it. */
		bool WantsExit() const { return m_wantsExit; }

		struct Status
		{
			State state{ State::IDLE };
			std::string phase;				/**< Readable name of the current phase. */
			std::string experimentId;
			std::string variant;
			int repetition{ 0 };
			int unit{ 0 };					/**< 1-based index of the current unit. */
			int unitsTotal{ 0 };
			uint64_t phaseFrame{ 0 };		/**< Frames spent in the phase (warm-up, measure, waits). */
			uint64_t phaseFrames{ 0 };		/**< Frames the phase lasts, 0 if open-ended. */
			std::filesystem::path unitFolder;
			std::filesystem::path outputFolder;
			int unitsFailed{ 0 };
			std::string message;			/**< Last notice or reason. */
		};
		Status GetStatus() const;

	private:
		enum class StepResult : uint8_t
		{
			RUNNING,
			DONE,
			FAILED
		};

		enum class Phase : uint8_t
		{
			NONE,
			UNIT_BEGIN,
			LOAD,
			SETTLE_LOAD,
			APPLY,
			SETTLE_APPLY,
			BENCH_ON,
			SETTLE_BENCH,
			PREPARE_RECORD,
			PREPARE_STEPS,
			PREPARE_DRAIN,
			CAPTURE,
			PRE_WARMUP,
			WARMUP,
			MEASURE_START,
			MEASURE,
			MEASURE_DRAIN,
			RESTORE,
			FINISH,
			COOLDOWN,
			FINISHED
		};

		struct Unit
		{
			int index{ 0 };					/**< 1-based. */
			size_t variant{ 0 };
			int repetition{ 1 };
		};

		struct CaptureRecord
		{
			std::string kind;				/**< `mask` or `screenshot`. */
			std::string after;
			std::string name;				/**< Mask key or screenshot tag. */
			std::string file;				/**< Relative to the run folder. */
			std::string sha256;				/**< Mask buffer hash or screenshot pixel hash. */
			std::string detail;
		};

		struct StepLog
		{
			std::string step;
			std::string result;
		};

		// ---- Phases ----
		void BeginUnit();
		void TickPrepare();
		void TickMeasure();
		void TickCaptures();
		void BeginCaptures(const std::string& after, Phase next);
		void FinishUnit();
		void FailUnit(const std::string& reason);
		void Cleanup();
		void EndRun(State state);
		void NextUnitOrFinish();

		// ---- Pieces ----
		bool ApplyParameters(std::string& error);
		bool ApplyParameter(const ExperimentParam& param, std::string& error);
		void ResetRuntimeControls();
		StepResult TickStep(const ExperimentStep& step, std::string& error);
		void EmitSteppingWindow(const char* kind, const ExperimentStep& step, int stepsDone, double wallMs, uint64_t frames);
		bool StartPlayback(std::string& error);
		void RestartPlaybackIfEnded();
		void SetPhase(Phase phase, uint64_t phaseFrames = 0);
		void WriteUnitReport(const Metadata& metadata, bool windowWritten);
		/** @brief Closes the unit's run.yaml with the metadata read while the benchmark state was still set. */
		void CloseUnit(bool windowWritten);
		void EmitExperimentBlock(YAML::Emitter& out, const std::string& status) const;
		RunInfo MakeRunInfo() const;
		ChunkManager* GetChunks() const;
		EditorScene* GetEditor() const;
		std::string PhaseName() const;
		void Notice(const std::string& message);

		ExperimentHost m_host;
		State m_state{ State::IDLE };
		ExperimentConfig m_config;
		bool m_exitWhenDone{ false };
		bool m_wantsExit{ false };
		bool m_rejected{ false };
		std::string m_message;

		std::vector<Unit> m_units;
		size_t m_unitIndex{ 0 };
		int m_unitsFailed{ 0 };
		bool m_anyFailed{ false };
		std::string m_stamp;				/**< Local start time of the run, the first part of every run id. */
		std::filesystem::path m_outputRoot;

		// Current unit
		Phase m_phase{ Phase::NONE };
		uint64_t m_phaseFrame{ 0 };
		uint64_t m_phaseFrames{ 0 };
		RunInfo m_run;
		std::filesystem::path m_unitFolder;
		std::filesystem::path m_pathCopy;	/**< The path file in `<unit>/paths/`. */
		std::chrono::system_clock::time_point m_unitStart{};
		Clock::time_point m_unitStartSteady{};
		Clock::time_point m_cooldownStart{};
		std::vector<std::string> m_failures;
		std::vector<std::string> m_warnings;
		std::vector<StepLog> m_stepLog;
		std::vector<CaptureRecord> m_captureLog;
		bool m_windowActive{ false };		/**< A CSV window was started in this unit. */
		bool m_memoryStartTaken{ false };	/**< The first memory snapshot of the window (`prepare_start` or `measure_start`) was added. */
		bool m_memoryEndTaken{ false };		/**< The `measure_end` snapshot was added. */
		bool m_windowWritten{ false };
		uint64_t m_windowGpuDropped{ 0 };
		double m_windowMaxSleepMs{ 0.0 };
		uint64_t m_windowFrames{ 0 };
		std::string m_pathSha256;
		uint64_t m_pathFrames{ 0 };
		bool m_unitOpen{ false };			/**< A unit folder exists and its run.yaml is still to be written. */
		bool m_savedDebugEnabled{ false };
		bool m_fullscreenSet{ false };
		bool m_abortRequested{ false };
		bool m_firstUnitChecked{ false };

		// Prepare steps
		size_t m_stepIndex{ 0 };
		bool m_stepStarted{ false };
		int m_stepDone{ 0 };
		uint64_t m_stepFrames{ 0 };
		Clock::time_point m_stepStartTime{};
		uint64_t m_stepStartFrame{ 0 };

		// Captures
		std::vector<ExperimentCapture> m_captureQueue;
		size_t m_captureIndex{ 0 };
		int m_captureStage{ 0 };
		bool m_capturePlaying{ false };
		Phase m_phaseAfterCaptures{ Phase::NONE };
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_EXPERIMENTRUNNER_H_
