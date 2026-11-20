#include <yaml-cpp/yaml.h>

#include "cameraPath.h"
#include "core/logger/logger.h"
#include "debug/runMetadata.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <system_error>

namespace
{
	/** @brief Path as UTF-8 (path::string() uses the ANSI code page and can throw). */
	std::string Utf8(const std::filesystem::path& path)
	{
		const std::u8string text = path.u8string();
		return std::string(text.begin(), text.end());
	}

	/** @brief Float with 9 significant digits, which reads back the same float32 (locale independent). */
	std::string FloatText(float value)
	{
		char buffer[48];
		const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general, 9);
		return std::string(buffer, result.ptr);
	}

	bool ParseFloat(const YAML::Node& node, float& value)
	{
		if (!node.IsDefined() || !node.IsScalar())
		{
			return false;
		}
		const std::string& text = node.Scalar();
		const char* first = text.data();
		const char* last = first + text.size();
		const auto result = std::from_chars(first, last, value);
		return result.ec == std::errc() && result.ptr == last && std::isfinite(value);
	}

	/** @brief Hex float text of `%a`, which is exact for a float value. */
	void AppendHex(std::string& out, float value)
	{
		char buffer[48];
		const int length = std::snprintf(buffer, sizeof(buffer), "%a", static_cast<double>(value));
		out.append(buffer, length > 0 ? static_cast<size_t>(length) : 0u);
	}

	void AppendMatrix(std::string& out, const glm::mat4& matrix)
	{
		for (int column = 0; column < 4; ++column)
		{
			for (int row = 0; row < 4; ++row)
			{
				out += ' ';
				AppendHex(out, matrix[column][row]);
			}
		}
	}
}

bool pwg::debug::CameraPath::Clear()
{
	if (m_state != State::IDLE)
	{
		return false;
	}
	m_keys.clear();
	m_file.clear();
	m_sha256.clear();
	return true;
}

pwg::debug::CameraPathPose pwg::debug::CameraPath::Evaluate(double t) const
{
	if (m_keys.empty())
	{
		return {};
	}
	if (!(t > static_cast<double>(m_keys.front().t)))
	{
		return m_keys.front().pose;
	}
	if (t >= static_cast<double>(m_keys.back().t))
	{
		return m_keys.back().pose;
	}
	// First key with time > t; it exists and is not the first key, because of the two checks above.
	const auto upper = std::upper_bound(m_keys.begin(), m_keys.end(), t,
		[](double time, const CameraKey& key) { return time < static_cast<double>(key.t); });
	const CameraKey& b = *upper;
	const CameraKey& a = *(upper - 1);
	const double t0 = static_cast<double>(a.t);
	const double alpha = (t - t0) / (static_cast<double>(b.t) - t0);

	auto lerp = [alpha](float from, float to) {
		return static_cast<float>(static_cast<double>(from) + (static_cast<double>(to) - static_cast<double>(from)) * alpha);
	};
	CameraPathPose pose;
	pose.position = glm::vec3(lerp(a.pose.position.x, b.pose.position.x), lerp(a.pose.position.y, b.pose.position.y),
		lerp(a.pose.position.z, b.pose.position.z));
	pose.yawDeg = lerp(a.pose.yawDeg, b.pose.yawDeg);
	pose.pitchDeg = lerp(a.pose.pitchDeg, b.pose.pitchDeg);
	pose.rollRad = lerp(a.pose.rollRad, b.pose.rollRad);
	return pose;
}

uint64_t pwg::debug::CameraPath::FrameCount(double durationSeconds, double stepSeconds)
{
	if (!(stepSeconds > 0.0) || !(durationSeconds >= 0.0))
	{
		return 0;
	}
	// The small term keeps a duration that is an exact multiple of the step from losing its last frame to rounding.
	return static_cast<uint64_t>(std::floor(durationSeconds / stepSeconds + 1e-6)) + 1;
}

// ---------------------------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------------------------

bool pwg::debug::CameraPath::StartRecording()
{
	if (m_state != State::IDLE)
	{
		return false;
	}
	m_keys.clear();
	m_file.clear();
	m_sha256.clear();
	m_projection = CameraPathProjection{};
	m_state = State::RECORDING;
	return true;
}

bool pwg::debug::CameraPath::AddSample(const CameraPathPose& pose, Clock::time_point time)
{
	if (m_state != State::RECORDING)
	{
		return false;
	}
	if (m_keys.size() >= MaxKeys)
	{
		PWG_WARN("Camera path: recording stopped at the limit of {} keys", MaxKeys);
		StopRecording();
		return false;
	}

	CameraKey key;
	key.pose = pose;
	key.pose.pitchDeg = std::clamp(key.pose.pitchDeg, -MaxPitchDegrees, MaxPitchDegrees);
	if (m_keys.empty())
	{
		m_recordStart = time;
		key.t = 0.0f;
	}
	else
	{
		key.t = std::chrono::duration<float>(time - m_recordStart).count();
		if (!(key.t > m_keys.back().t))
		{
			return false;
		}
		// Unwrap yaw: take the representation of the angle that is closest to the previous key.
		const double previous = static_cast<double>(m_keys.back().pose.yawDeg);
		const double delta = static_cast<double>(key.pose.yawDeg) - previous;
		key.pose.yawDeg = static_cast<float>(previous + (delta - 360.0 * std::round(delta / 360.0)));
	}
	m_keys.push_back(key);
	return true;
}

void pwg::debug::CameraPath::StopRecording()
{
	if (m_state != State::RECORDING)
	{
		return;
	}
	m_state = State::IDLE;
	if (m_keys.empty())
	{
		m_sha256.clear();
		return;
	}
	const std::string text = Serialize();
	m_sha256 = ComputeSha256(text.data(), text.size());
	PWG_INFO("Camera path: recorded {} keys, {:.3f} s", m_keys.size(), GetDuration());
}

// ---------------------------------------------------------------------------------------------
// Playback
// ---------------------------------------------------------------------------------------------

bool pwg::debug::CameraPath::StartPlayback(const CameraPathPlayback& options, std::string& error)
{
	if (m_state != State::IDLE)
	{
		error = "a recording or playback is active";
		return false;
	}
	if (!HasPath())
	{
		error = "the path has fewer than two keys";
		return false;
	}
	if (!(options.stepSeconds >= 1.0e-6 && options.stepSeconds <= 10.0))
	{
		error = "the time step must be between 1e-6 and 10 seconds";
		return false;
	}
	m_step = options.stepSeconds;
	m_frameCount = FrameCount(GetDuration(), m_step);
	m_nextFrame = 0;
	m_appliedFrame = -1;
	m_dumpFile = options.matrixDumpFile;
	m_dumpRows.clear();
	m_dumpedFrame = -1;
	m_dumpFrames = 0;
	m_dumpWidth = 0;
	m_dumpHeight = 0;
	m_dumpSizeChanged = false;
	m_hasDumpResult = false;
	m_state = State::PLAYING;
	PWG_INFO("Camera path: playback started, {} frames at step {:.6f} s", m_frameCount, m_step);
	return true;
}

bool pwg::debug::CameraPath::TickPlayback(CameraPathPose& pose)
{
	if (m_state != State::PLAYING)
	{
		return false;
	}
	if (m_nextFrame >= m_frameCount)
	{
		FinishPlayback();
		return false;
	}
	pose = Evaluate(static_cast<double>(m_nextFrame) * m_step);
	m_appliedFrame = static_cast<int64_t>(m_nextFrame);
	++m_nextFrame;
	return true;
}

void pwg::debug::CameraPath::StopPlayback()
{
	if (m_state == State::PLAYING)
	{
		FinishPlayback();
	}
}

void pwg::debug::CameraPath::FinishPlayback()
{
	m_state = State::IDLE;
	m_appliedFrame = -1;
	if (!m_dumpFile.empty())
	{
		WriteDump();
		m_dumpFile.clear();
		m_dumpRows.clear();
		m_dumpRows.shrink_to_fit();
	}
	PWG_INFO("Camera path: playback ended after {} of {} frames", m_nextFrame, m_frameCount);
}

void pwg::debug::CameraPath::NoteRenderedMatrices(const glm::mat4& view, const glm::mat4& projection, int width, int height)
{
	if (m_state != State::PLAYING || m_dumpFile.empty() || m_appliedFrame < 0 || m_appliedFrame == m_dumpedFrame)
	{
		return;
	}
	if (m_dumpFrames == 0)
	{
		m_dumpWidth = width;
		m_dumpHeight = height;
	}
	else if (width != m_dumpWidth || height != m_dumpHeight)
	{
		m_dumpSizeChanged = true;
	}
	m_dumpedFrame = m_appliedFrame;
	++m_dumpFrames;
	m_dumpRows += std::to_string(m_appliedFrame);
	AppendMatrix(m_dumpRows, view);
	AppendMatrix(m_dumpRows, projection);
	m_dumpRows += '\n';
}

bool pwg::debug::CameraPath::WriteDump()
{
	MatrixDumpResult result;
	result.file = m_dumpFile;
	result.frames = m_dumpFrames;
	if (m_dumpFrames == 0)
	{
		result.error = "no frame was rendered during the playback";
		m_lastDump = std::move(result);
		m_hasDumpResult = true;
		return false;
	}

	// Everything in the file is a function of the path, the step and the render size, so two playbacks of the
	// same path at the same size give identical files (no time stamps, no file names).
	std::string text;
	text.reserve(m_dumpRows.size() + 512);
	text += "# format: ";
	text += MatrixDumpFormatName;
	text += "\n# format_version: 1\n# path_sha256: ";
	text += m_sha256;
	text += "\n# key_count: " + std::to_string(m_keys.size());
	text += "\n# step_s_hex: ";
	{
		char buffer[48];
		const int length = std::snprintf(buffer, sizeof(buffer), "%a", m_step);
		text.append(buffer, length > 0 ? static_cast<size_t>(length) : 0u);
	}
	text += "\n# render_size: " + std::to_string(m_dumpWidth) + "x" + std::to_string(m_dumpHeight);
	if (m_dumpSizeChanged)
	{
		text += " (changed during the playback; later frames used another size)";
	}
	text += "\n# columns: path_frame, then view matrix (16 values) and projection matrix (16 values), column-major as in GL, "
		"hex floats (printf %a)\n";
	text += m_dumpRows;

	std::error_code error;
	if (m_dumpFile.has_parent_path())
	{
		std::filesystem::create_directories(m_dumpFile.parent_path(), error);
	}
	if (error)
	{
		result.error = "cannot create " + Utf8(m_dumpFile.parent_path()) + ": " + error.message();
	}
	else
	{
		std::ofstream out(m_dumpFile, std::ios::binary);
		if (!out.is_open())
		{
			result.error = "cannot write " + Utf8(m_dumpFile);
		}
		else
		{
			out.write(text.data(), static_cast<std::streamsize>(text.size()));
			out.close();
			if (out.fail())
			{
				result.error = "write error in " + Utf8(m_dumpFile);
			}
		}
	}
	result.written = result.error.empty();
	if (result.written)
	{
		result.sha256 = ComputeSha256(text.data(), text.size());
		PWG_INFO("Camera path: matrix dump of {} frames written to {}, sha256 {}", result.frames, Utf8(m_dumpFile), result.sha256);
	}
	else
	{
		PWG_ERROR("Camera path: matrix dump failed: {}", result.error);
	}
	m_lastDump = std::move(result);
	m_hasDumpResult = true;
	return m_lastDump.written;
}

// ---------------------------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------------------------

std::string pwg::debug::CameraPath::Serialize() const
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "format" << YAML::Value << FormatName;
	out << YAML::Key << "format_version" << YAML::Value << FormatVersion;
	out << YAML::Key << "camera" << YAML::Value << "flyover";
	out << YAML::Key << "fov_deg" << YAML::Value << FloatText(m_projection.fovDeg);
	out << YAML::Key << "near" << YAML::Value << FloatText(m_projection.nearPlane);
	out << YAML::Key << "far" << YAML::Value << FloatText(m_projection.farPlane);
	out << YAML::Key << "duration_s" << YAML::Value << FloatText(m_keys.empty() ? 0.0f : m_keys.back().t);
	out << YAML::Key << "keys" << YAML::Value << YAML::BeginSeq;
	for (const CameraKey& key : m_keys)
	{
		out << YAML::Flow << YAML::BeginSeq
			<< FloatText(key.t)
			<< FloatText(key.pose.position.x) << FloatText(key.pose.position.y) << FloatText(key.pose.position.z)
			<< FloatText(key.pose.yawDeg) << FloatText(key.pose.pitchDeg) << FloatText(key.pose.rollRad)
			<< YAML::EndSeq;
	}
	out << YAML::EndSeq << YAML::EndMap;
	std::string text = out.c_str();
	text += '\n';
	return text;
}

bool pwg::debug::CameraPath::Save(const std::filesystem::path& file, bool overwrite, std::string& error)
{
	if (m_state != State::IDLE)
	{
		error = "a recording or playback is active";
		return false;
	}
	if (m_keys.empty())
	{
		error = "there is no path to save";
		return false;
	}
	std::error_code code;
	if (!overwrite && std::filesystem::exists(file, code))
	{
		error = "the file exists: " + Utf8(file);
		return false;
	}
	if (file.has_parent_path())
	{
		std::filesystem::create_directories(file.parent_path(), code);
		if (code)
		{
			error = "cannot create " + Utf8(file.parent_path()) + ": " + code.message();
			return false;
		}
	}

	const std::string text = Serialize();
	{
		std::ofstream out(file, std::ios::binary);
		if (!out.is_open())
		{
			error = "cannot write " + Utf8(file);
			return false;
		}
		out.write(text.data(), static_cast<std::streamsize>(text.size()));
		out.close();
		if (out.fail())
		{
			error = "write error in " + Utf8(file);
			return false;
		}
	}
	m_file = file;
	m_sha256 = ComputeSha256(text.data(), text.size());
	PWG_INFO("Camera path: saved {} keys to {}", m_keys.size(), Utf8(file));
	return true;
}

bool pwg::debug::CameraPath::Load(const std::filesystem::path& file, std::string& error)
{
	if (m_state != State::IDLE)
	{
		error = "a recording or playback is active";
		return false;
	}

	std::ifstream in(file, std::ios::binary);
	if (!in.is_open())
	{
		error = "cannot open " + Utf8(file);
		return false;
	}
	const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	if (in.bad())
	{
		error = "read error in " + Utf8(file);
		return false;
	}

	std::vector<CameraKey> keys;
	CameraPathProjection projection;
	try
	{
		const YAML::Node root = YAML::Load(text);
		if (!root.IsMap())
		{
			error = "not a YAML map";
			return false;
		}
		if (!root["format"] || root["format"].as<std::string>() != FormatName)
		{
			error = std::string("format is not ") + FormatName;
			return false;
		}
		if (!root["format_version"] || root["format_version"].as<int>() != FormatVersion)
		{
			error = "unsupported format_version (this build reads " + std::to_string(FormatVersion) + ")";
			return false;
		}
		if (!root["camera"] || root["camera"].as<std::string>() != "flyover")
		{
			error = "camera is not flyover";
			return false;
		}
		float duration = 0.0f;
		if (!ParseFloat(root["fov_deg"], projection.fovDeg) || !(projection.fovDeg > 0.0f && projection.fovDeg < 180.0f))
		{
			error = "fov_deg is missing or not in (0, 180)";
			return false;
		}
		if (!ParseFloat(root["near"], projection.nearPlane) || !(projection.nearPlane > 0.0f))
		{
			error = "near is missing or not positive";
			return false;
		}
		if (!ParseFloat(root["far"], projection.farPlane) || !(projection.farPlane > projection.nearPlane))
		{
			error = "far is missing or not larger than near";
			return false;
		}
		if (!ParseFloat(root["duration_s"], duration))
		{
			error = "duration_s is missing or not a number";
			return false;
		}
		const YAML::Node list = root["keys"];
		if (!list.IsDefined() || !list.IsSequence() || list.size() < 2)
		{
			error = "keys must be a list of at least two keys";
			return false;
		}
		if (list.size() > MaxKeys)
		{
			error = "more than " + std::to_string(MaxKeys) + " keys";
			return false;
		}

		keys.reserve(list.size());
		for (size_t i = 0; i < list.size(); ++i)
		{
			const YAML::Node row = list[i];
			const std::string where = "key " + std::to_string(i) + ": ";
			if (!row.IsSequence() || row.size() != 7)
			{
				error = where + "expected [t, px, py, pz, yaw_deg, pitch_deg, roll_rad]";
				return false;
			}
			float v[7];
			for (size_t c = 0; c < 7; ++c)
			{
				if (!ParseFloat(row[c], v[c]))
				{
					error = where + "value " + std::to_string(c) + " is not a finite number";
					return false;
				}
			}
			CameraKey key;
			key.t = v[0];
			key.pose.position = glm::vec3(v[1], v[2], v[3]);
			key.pose.yawDeg = v[4];
			key.pose.pitchDeg = v[5];
			key.pose.rollRad = v[6];
			if (keys.empty() ? key.t != 0.0f : !(key.t > keys.back().t))
			{
				error = where + (keys.empty() ? "the first time must be 0" : "times must increase strictly");
				return false;
			}
			if (std::fabs(key.pose.pitchDeg) > MaxPitchDegrees)
			{
				error = where + "pitch_deg is outside [-89, 89]";
				return false;
			}
			keys.push_back(key);
		}
		if (duration != keys.back().t)
		{
			error = "duration_s does not equal the time of the last key";
			return false;
		}
	}
	catch (const YAML::Exception& exception)
	{
		error = std::string("YAML error: ") + exception.what();
		return false;
	}

	m_keys = std::move(keys);
	m_projection = projection;
	m_file = file;
	m_sha256 = ComputeSha256(text.data(), text.size());
	PWG_INFO("Camera path: loaded {} keys, {:.3f} s from {}", m_keys.size(), GetDuration(), Utf8(file));
	return true;
}
