#ifndef SRC_DEBUG_RUNMETADATA_H_
#define SRC_DEBUG_RUNMETADATA_H_

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

namespace pwg::debug
{
	/**
	* @brief Ordered list of metadata keys and values (design section 7.2).
	*
	* The same list is written as `# key: value` comment lines at the top of every CSV file and as the
	* `metadata` map of run.yaml. Keys keep their insertion order. Setting an existing key replaces its value
	* in place. A value that is not available is an empty value, never 0.
	*/
	class Metadata
	{
	public:
		enum class Kind : uint8_t
		{
			STRING,		/**< Written quoted in YAML. */
			NUMBER,		/**< Written plain in YAML. */
			BOOL,		/**< true / false. */
			EMPTY		/**< Not available: empty in CSV, null in YAML. */
		};

		struct Entry
		{
			std::string key;
			std::string value;
			Kind kind{ Kind::STRING };
		};

		void Set(const std::string& key, const std::string& value);
		void SetNumber(const std::string& key, int64_t value);
		void SetNumber(const std::string& key, uint64_t value);
		void SetNumber(const std::string& key, int value) { SetNumber(key, static_cast<int64_t>(value)); }
		void SetNumber(const std::string& key, unsigned int value) { SetNumber(key, static_cast<uint64_t>(value)); }
		void SetNumber(const std::string& key, double value);
		void SetBool(const std::string& key, bool value);
		void SetEmpty(const std::string& key);

		const Entry* Find(const std::string& key) const;
		const std::vector<Entry>& GetEntries() const { return m_entries; }

	private:
		void Put(const std::string& key, std::string value, Kind kind);

		std::vector<Entry> m_entries;
	};

	/**
	* @brief Identifiers of one measured run. The runner (D-11) fills all of them; a manual export from the
	* Performance tab uses its own run id, no experiment id, variant "manual" and repetition 1.
	*/
	struct RunInfo
	{
		std::string runId;
		std::string experimentId;
		std::string variant{ "manual" };
		int repetition{ 1 };
		std::vector<std::pair<std::string, std::string>> params;	/**< Variant parameters: `param.<name>` in metadata and summary.csv. */
		std::vector<std::pair<std::string, std::string>> manual;	/**< Values typed into the config by the author: `manual.<name>`. */
	};

	/**
	* @brief Build, OpenGL context and machine information for the metadata header (design section 7.2).
	*
	* The GL strings and timer query bits are read once, right after the main GL context is created
	* (CaptureContextInfo). Machine values that cannot change while the process runs (CPU, cores, RAM, OS)
	* are read at the same time. Power state is read again for every run, because the laptop can be
	* unplugged between runs. Every read is a plain query; nothing here changes GL or system state.
	*/
	class RunMetadata
	{
	public:
		/** @brief Writes values of the current scene (project, world size). Registered by the editor. */
		using SceneProvider = std::function<void(Metadata&)>;

		static constexpr const char* FormatName = "pwg-debugview-csv";
		static constexpr int FormatVersion = 1;

		/** @brief Reads GL strings, query counter bits and static machine info. Main GL context must be current. */
		void CaptureContextInfo();
		bool HasContextInfo() const { return m_contextCaptured; }

		const std::string& GetGlRenderer() const { return m_glRenderer; }

		void SetSceneProvider(SceneProvider provider) { m_sceneProvider = std::move(provider); }
		void ClearSceneProvider() { m_sceneProvider = nullptr; }

		/** @brief Adds format, run identifiers and run start time. */
		void AppendRunInfo(Metadata& out, const RunInfo& run, std::chrono::system_clock::time_point start) const;

		/** @brief Adds commit, dirty flag, build type, compiler and instrumentation state. */
		void AppendBuildInfo(Metadata& out) const;

		/** @brief Adds the values read by CaptureContextInfo, plus power state and clock period read now. */
		void AppendMachineInfo(Metadata& out) const;

		/** @brief Adds the scene values of the registered provider; keys stay empty without one. */
		void AppendSceneInfo(Metadata& out) const;

	private:
		bool m_contextCaptured{ false };
		std::string m_glVendor;
		std::string m_glRenderer;
		std::string m_glVersion;
		std::string m_glslVersion;
		int m_timeElapsedBits{ -1 };
		int m_timestampBits{ -1 };
		bool m_nvxMemoryInfo{ false };
		std::string m_cpuName;
		unsigned int m_logicalCores{ 0 };
		int m_ompMaxThreads{ 0 };
		uint64_t m_ramTotalMb{ 0 };
		std::string m_osVersion;
		SceneProvider m_sceneProvider;
	};

	/** @brief Lower-case hex SHA-256 of a file, or an empty string if it cannot be read. */
	std::string ComputeFileSha256(const std::filesystem::path& path);

	/** @brief Local or UTC time as ISO 8601 with seconds, e.g. 2027-03-02T14:05:11 (UTC gets a trailing Z). */
	std::string FormatIsoTime(std::chrono::system_clock::time_point time, bool utc);

	/**
	* @brief Writes the metadata as `# key: value` lines, preceded by `# file: <file>`.
	* Line breaks inside values are replaced by spaces, so each key stays on one comment line.
	*/
	void WriteMetadataComments(std::ostream& out, const Metadata& metadata, const char* file);

	/**
	* @brief Writes run.yaml: the metadata map, the `result` map and the files of the run.
	* `result` holds values known only at the end of the run (status, row counts, warnings).
	*/
	bool WriteRunYaml(const std::filesystem::path& path, const Metadata& metadata, const Metadata& result,
		const std::vector<std::string>& files);
} // namespace pwg::debug

#endif // !SRC_DEBUG_RUNMETADATA_H_
