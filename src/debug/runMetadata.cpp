#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#include <powrprof.h>
#include <intrin.h>
#include <glad/glad.h>
#include <omp.h>
#include <yaml-cpp/yaml.h>

#include "runMetadata.h"
#include "debug/debugView.h"
#include "core/logger/logger.h"
#include "pwgBuildInfo.h"	// Generated at build time by cmake/pwgBuildInfo.cmake.

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <ostream>
#include <thread>

// Build configuration name ($<CONFIG>) from CMakeLists.txt.
#ifndef PWG_BUILD_CONFIG
#define PWG_BUILD_CONFIG unknown
#endif
#define PWG_STRINGIFY_INNER(x) #x
#define PWG_STRINGIFY(x) PWG_STRINGIFY_INNER(x)

namespace
{
	std::string WideToUtf8(const wchar_t* text)
	{
		if (!text || !*text)
		{
			return {};
		}
		const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		if (size <= 1)
		{
			return {};
		}
		std::string result(static_cast<size_t>(size - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
		return result;
	}

	std::string GlString(GLenum name)
	{
		const GLubyte* value = glGetString(name);
		return value ? std::string(reinterpret_cast<const char*>(value)) : std::string();
	}

	bool HasGlExtension(const char* extension)
	{
		GLint count = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &count);
		for (GLint i = 0; i < count; ++i)
		{
			const GLubyte* name = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
			if (name && std::strcmp(reinterpret_cast<const char*>(name), extension) == 0)
			{
				return true;
			}
		}
		return false;
	}

	/** @brief CPU brand string from CPUID leaves 0x80000002..4, trimmed. */
	std::string CpuBrandString()
	{
		std::array<int, 4> regs{};
		__cpuid(regs.data(), static_cast<int>(0x80000000));
		if (static_cast<unsigned int>(regs[0]) < 0x80000004u)
		{
			return {};
		}
		char brand[49]{};
		for (int leaf = 0; leaf < 3; ++leaf)
		{
			__cpuid(regs.data(), static_cast<int>(0x80000002 + leaf));
			std::memcpy(brand + leaf * 16, regs.data(), 16);
		}
		std::string result(brand);
		const size_t first = result.find_first_not_of(' ');
		const size_t last = result.find_last_not_of(' ');
		return first == std::string::npos ? std::string() : result.substr(first, last - first + 1);
	}

	/** @brief Windows version from RtlGetVersion, which, unlike GetVersionEx, is not affected by the manifest. */
	std::string OsVersion()
	{
		using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
		HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
		if (!ntdll)
		{
			return {};
		}
		auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")));
		if (!rtlGetVersion)
		{
			return {};
		}
		RTL_OSVERSIONINFOW info{};
		info.dwOSVersionInfoSize = sizeof(info);
		if (rtlGetVersion(&info) != 0)
		{
			return {};
		}
		char text[64];
		std::snprintf(text, sizeof(text), "Windows %lu.%lu.%lu", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
		return text;
	}

	std::string GuidToString(const GUID& guid)
	{
		char text[40];
		std::snprintf(text, sizeof(text), "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		return text;
	}

	/** @brief Values must stay on one line in the CSV comment header. */
	std::string OneLine(const std::string& value)
	{
		std::string result = value;
		for (char& c : result)
		{
			if (c == '\r' || c == '\n')
			{
				c = ' ';
			}
		}
		return result;
	}
}

// --- Metadata ---

void pwg::debug::Metadata::Put(const std::string& key, std::string value, Kind kind)
{
	for (Entry& entry : m_entries)
	{
		if (entry.key == key)
		{
			entry.value = std::move(value);
			entry.kind = kind;
			return;
		}
	}
	m_entries.push_back(Entry{ key, std::move(value), kind });
}

void pwg::debug::Metadata::Set(const std::string& key, const std::string& value)
{
	Put(key, value, value.empty() ? Kind::EMPTY : Kind::STRING);
}

void pwg::debug::Metadata::SetNumber(const std::string& key, int64_t value)
{
	Put(key, std::to_string(value), Kind::NUMBER);
}

void pwg::debug::Metadata::SetNumber(const std::string& key, uint64_t value)
{
	Put(key, std::to_string(value), Kind::NUMBER);
}

void pwg::debug::Metadata::SetNumber(const std::string& key, double value)
{
	if (std::isnan(value))
	{
		SetEmpty(key);
		return;
	}
	char text[64];
	std::snprintf(text, sizeof(text), "%.10g", value);
	Put(key, text, Kind::NUMBER);
}

void pwg::debug::Metadata::SetBool(const std::string& key, bool value)
{
	Put(key, value ? "true" : "false", Kind::BOOL);
}

void pwg::debug::Metadata::SetEmpty(const std::string& key)
{
	Put(key, std::string(), Kind::EMPTY);
}

const pwg::debug::Metadata::Entry* pwg::debug::Metadata::Find(const std::string& key) const
{
	for (const Entry& entry : m_entries)
	{
		if (entry.key == key)
		{
			return &entry;
		}
	}
	return nullptr;
}

// --- RunMetadata ---

void pwg::debug::RunMetadata::CaptureContextInfo()
{
	if (m_contextCaptured)
	{
		return;
	}

	// OpenGL 4.5 spec 22.2 (GetString) and 4.2.1 (QUERY_COUNTER_BITS). Read once; none of them changes
	// while the context exists.
	m_glVendor = GlString(GL_VENDOR);
	m_glRenderer = GlString(GL_RENDERER);
	m_glVersion = GlString(GL_VERSION);
	m_glslVersion = GlString(GL_SHADING_LANGUAGE_VERSION);
	GLint bits = -1;
	glGetQueryiv(GL_TIME_ELAPSED, GL_QUERY_COUNTER_BITS, &bits);
	m_timeElapsedBits = bits;
	bits = -1;
	glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &bits);
	m_timestampBits = bits;
	m_nvxMemoryInfo = HasGlExtension("GL_NVX_gpu_memory_info");

	m_cpuName = CpuBrandString();
	m_logicalCores = std::thread::hardware_concurrency();
	m_ompMaxThreads = omp_get_max_threads();
	MEMORYSTATUSEX memory{};
	memory.dwLength = sizeof(memory);
	if (GlobalMemoryStatusEx(&memory))
	{
		m_ramTotalMb = static_cast<uint64_t>(memory.ullTotalPhys / (1024ull * 1024ull));
	}
	m_osVersion = OsVersion();
	m_contextCaptured = true;

	PWG_INFO("OpenGL: {} | {} | {} | GLSL {}", m_glVendor, m_glRenderer, m_glVersion, m_glslVersion);
	PWG_INFO("Timer query bits: TIME_ELAPSED {}, TIMESTAMP {}", m_timeElapsedBits, m_timestampBits);
}

void pwg::debug::RunMetadata::AppendRunInfo(Metadata& out, const RunInfo& run, std::chrono::system_clock::time_point start) const
{
	out.Set("format", FormatName);
	out.SetNumber("format_version", FormatVersion);
	out.Set("run_id", run.runId);
	out.Set("experiment_id", run.experimentId);
	out.Set("variant", run.variant);
	out.SetNumber("repetition", run.repetition);
	out.Set("timestamp_local", FormatIsoTime(start, false));
	out.Set("timestamp_utc", FormatIsoTime(start, true));
}

void pwg::debug::RunMetadata::AppendBuildInfo(Metadata& out) const
{
	out.Set("pwg_commit", PWG_GIT_COMMIT);
	if (PWG_GIT_DIRTY < 0)
	{
		out.SetEmpty("pwg_dirty");
	}
	else
	{
		out.SetBool("pwg_dirty", PWG_GIT_DIRTY != 0);
	}
	out.Set("build_type", PWG_STRINGIFY(PWG_BUILD_CONFIG));
#ifdef NDEBUG
	out.SetBool("ndebug", true);
#else
	out.SetBool("ndebug", false);
#endif
#ifdef _MSC_FULL_VER
	out.Set("compiler", "MSVC " + std::to_string(_MSC_FULL_VER));
#else
	out.SetEmpty("compiler");
#endif
	out.SetBool("instrumentation_compiled", InstrumentationCompiled);
}

void pwg::debug::RunMetadata::AppendMachineInfo(Metadata& out) const
{
	out.Set("gl_vendor", m_glVendor);
	out.Set("gl_renderer", m_glRenderer);
	out.Set("gl_version", m_glVersion);
	out.Set("glsl_version", m_glslVersion);
	if (m_contextCaptured && m_timeElapsedBits >= 0)
	{
		out.SetNumber("query_bits_time_elapsed", m_timeElapsedBits);
		out.SetNumber("query_bits_timestamp", m_timestampBits);
		out.SetBool("nvx_memory_info", m_nvxMemoryInfo);
	}
	else
	{
		out.SetEmpty("query_bits_time_elapsed");
		out.SetEmpty("query_bits_timestamp");
		out.SetEmpty("nvx_memory_info");
	}

	out.Set("cpu_name", m_cpuName);
	out.SetNumber("logical_cores", m_logicalCores);
	out.SetNumber("omp_max_threads", m_ompMaxThreads);
	if (m_ramTotalMb > 0)
	{
		out.SetNumber("ram_total_mb", m_ramTotalMb);
	}
	else
	{
		out.SetEmpty("ram_total_mb");
	}
	out.Set("os_version", m_osVersion);

	// Power state is read per run: the laptop can be unplugged or switched to another plan between runs.
	SYSTEM_POWER_STATUS power{};
	if (GetSystemPowerStatus(&power))
	{
		if (power.ACLineStatus == 0 || power.ACLineStatus == 1)
		{
			out.SetBool("ac_power", power.ACLineStatus == 1);
		}
		else
		{
			out.SetEmpty("ac_power");
		}
		if (power.BatteryLifePercent <= 100)
		{
			out.SetNumber("battery_percent", static_cast<int>(power.BatteryLifePercent));
		}
		else
		{
			out.SetEmpty("battery_percent");
		}
	}
	else
	{
		out.SetEmpty("ac_power");
		out.SetEmpty("battery_percent");
	}

	std::string schemeName;
	std::string schemeGuid;
	GUID* scheme = nullptr;
	if (PowerGetActiveScheme(nullptr, &scheme) == ERROR_SUCCESS && scheme)
	{
		schemeGuid = GuidToString(*scheme);
		DWORD size = 0;
		if (PowerReadFriendlyName(nullptr, scheme, nullptr, nullptr, nullptr, &size) == ERROR_SUCCESS && size > 0)
		{
			std::vector<UCHAR> buffer(size);
			if (PowerReadFriendlyName(nullptr, scheme, nullptr, nullptr, buffer.data(), &size) == ERROR_SUCCESS)
			{
				schemeName = WideToUtf8(reinterpret_cast<const wchar_t*>(buffer.data()));
			}
		}
		LocalFree(scheme);
	}
	out.Set("power_scheme", schemeName);
	out.Set("power_scheme_guid", schemeGuid);

	using Period = std::chrono::steady_clock::period;
	out.SetNumber("steady_clock_period_ns", static_cast<double>(Period::num) * 1e9 / static_cast<double>(Period::den));
}

void pwg::debug::RunMetadata::AppendSceneInfo(Metadata& out) const
{
	// The keys are always written, so every header has the same key list; a missing value stays empty.
	static const char* const keys[] = {
		"project_path", "project_sha256", "pwgsim_sha256", "project_unsaved_changes",
		"world_size", "chunk_count", "chunk_size", "fullscreen_main_view", "camera_path", "camera_path_sha256" };
	for (const char* key : keys)
	{
		out.SetEmpty(key);
	}
	if (m_sceneProvider)
	{
		m_sceneProvider(out);
	}
}

// --- Free functions ---

std::string pwg::debug::ComputeFileSha256(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		return {};
	}

	BCRYPT_ALG_HANDLE algorithm = nullptr;
	if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
	{
		return {};
	}
	BCRYPT_HASH_HANDLE hash = nullptr;
	std::string result;
	if (BCRYPT_SUCCESS(BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0)))
	{
		std::vector<char> buffer(1 << 20);
		bool ok = true;
		while (ok && file)
		{
			file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
			const std::streamsize read = file.gcount();
			if (read > 0)
			{
				ok = BCRYPT_SUCCESS(BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(read), 0));
			}
		}
		std::array<UCHAR, 32> digest{};
		if (ok && !file.bad() && BCRYPT_SUCCESS(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0)))
		{
			static const char hex[] = "0123456789abcdef";
			result.reserve(64);
			for (UCHAR byte : digest)
			{
				result.push_back(hex[byte >> 4]);
				result.push_back(hex[byte & 0x0f]);
			}
		}
		BCryptDestroyHash(hash);
	}
	BCryptCloseAlgorithmProvider(algorithm, 0);
	return result;
}

std::string pwg::debug::FormatIsoTime(std::chrono::system_clock::time_point time, bool utc)
{
	const std::time_t seconds = std::chrono::system_clock::to_time_t(time);
	std::tm parts{};
	if (utc)
	{
		gmtime_s(&parts, &seconds);
	}
	else
	{
		localtime_s(&parts, &seconds);
	}
	char text[32];
	std::strftime(text, sizeof(text), utc ? "%Y-%m-%dT%H:%M:%SZ" : "%Y-%m-%dT%H:%M:%S", &parts);
	return text;
}

void pwg::debug::WriteMetadataComments(std::ostream& out, const Metadata& metadata, const char* file)
{
	out << "# file: " << file << '\n';
	for (const Metadata::Entry& entry : metadata.GetEntries())
	{
		out << "# " << entry.key << ':';
		if (!entry.value.empty())
		{
			out << ' ' << OneLine(entry.value);
		}
		out << '\n';
	}
}

bool pwg::debug::WriteRunYaml(const std::filesystem::path& path, const Metadata& metadata, const Metadata& result,
	const std::vector<std::string>& files)
{
	auto emitMap = [](YAML::Emitter& emitter, const Metadata& values)
	{
		emitter << YAML::BeginMap;
		for (const Metadata::Entry& entry : values.GetEntries())
		{
			emitter << YAML::Key << entry.key << YAML::Value;
			switch (entry.kind)
			{
			case Metadata::Kind::EMPTY: emitter << YAML::Null; break;
			case Metadata::Kind::STRING: emitter << YAML::DoubleQuoted << entry.value; break;
			default: emitter << entry.value; break;
			}
		}
		emitter << YAML::EndMap;
	};

	YAML::Emitter emitter;
	emitter << YAML::BeginMap;
	emitter << YAML::Key << "metadata" << YAML::Value;
	emitMap(emitter, metadata);
	emitter << YAML::Key << "result" << YAML::Value;
	emitMap(emitter, result);
	emitter << YAML::Key << "files" << YAML::Value << YAML::BeginSeq;
	for (const std::string& file : files)
	{
		emitter << file;
	}
	emitter << YAML::EndSeq;
	emitter << YAML::EndMap;
	if (!emitter.good())
	{
		PWG_ERROR("run.yaml: YAML emitter error: {}", emitter.GetLastError());
		return false;
	}

	std::ofstream out(path, std::ios::binary);
	if (!out.is_open())
	{
		return false;
	}
	out << "# PWG Debug View run (" << RunMetadata::FormatName << " " << RunMetadata::FormatVersion << ")\n";
	out << emitter.c_str() << '\n';
	return out.good();
}
