#include <glad/glad.h>
#include <yaml-cpp/yaml.h>
#include <stb/stb_image_write.h>
#include <glm/gtc/type_ptr.hpp>

#include "exporters.h"
#include "core/logger/logger.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <ctime>
#include <fstream>
#include <system_error>

namespace
{
	using Clock = std::chrono::steady_clock;

	double MillisecondsSince(Clock::time_point start)
	{
		return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
	}

	/** @brief Path as UTF-8 (path::string() uses the ANSI code page and can throw). */
	std::string Utf8(const std::filesystem::path& path)
	{
		const std::u8string text = path.u8string();
		return std::string(text.begin(), text.end());
	}

	/** @brief Float with enough digits to read back the same float32 value. */
	std::string FloatText(float value)
	{
		char text[32];
		std::snprintf(text, sizeof(text), "%.9g", static_cast<double>(value));
		return text;
	}

	void EmitVec3(YAML::Emitter& out, const char* key, const glm::vec3& value)
	{
		out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq
			<< FloatText(value.x) << FloatText(value.y) << FloatText(value.z) << YAML::EndSeq;
	}

	/** @brief 16 values in GL (column-major) order, as glUniformMatrix4fv expects them. */
	void EmitMat4(YAML::Emitter& out, const char* key, const glm::mat4& value)
	{
		const float* data = glm::value_ptr(value);
		out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (int i = 0; i < 16; ++i)
		{
			out << FloatText(data[i]);
		}
		out << YAML::EndSeq;
	}

	void EmitFloat(YAML::Emitter& out, const char* key, float value)
	{
		out << YAML::Key << key << YAML::Value << FloatText(value);
	}

	void EmitMs(YAML::Emitter& out, const char* key, double value)
	{
		char text[32];
		std::snprintf(text, sizeof(text), "%.3f", value);
		out << YAML::Key << key << YAML::Value << text;
	}

	bool WriteTextFile(const std::filesystem::path& path, const std::string& header, const char* body)
	{
		std::ofstream out(path, std::ios::binary);
		if (!out.is_open())
		{
			return false;
		}
		out << header << body << '\n';
		return out.good();
	}

	void StbWriteToStream(void* context, void* data, int size)
	{
		static_cast<std::ofstream*>(context)->write(static_cast<const char*>(data), size);
	}
}

// --- Shared helpers ---

std::string pwg::debug::SanitizeFileNamePart(const std::string& text, const char* fallback)
{
	std::string result;
	result.reserve(text.size());
	bool replaced = false;
	for (const char c : text)
	{
		const unsigned char u = static_cast<unsigned char>(c);
		const bool keep = (u < 0x80) && (std::isalnum(u) || c == '.' || c == '-');
		if (keep)
		{
			result.push_back(c);
			replaced = false;
		}
		else if (!replaced)
		{
			// A run of other characters (a multi-byte UTF-8 letter, spaces) becomes one '-'.
			result.push_back('-');
			replaced = true;
		}
	}
	// Leading or trailing separators and dots carry no information.
	const size_t first = result.find_first_not_of("-.");
	const size_t last = result.find_last_not_of("-.");
	result = first == std::string::npos ? std::string() : result.substr(first, last - first + 1);
	return result.empty() ? std::string(fallback) : result;
}

std::string pwg::debug::ProjectNameFromMetadata(const Metadata& metadata)
{
	const Metadata::Entry* entry = metadata.Find("project_path");
	if (!entry || entry->value.empty())
	{
		return "untitled";
	}
	const std::u8string utf8(entry->value.begin(), entry->value.end());
	const std::u8string stem = std::filesystem::path(utf8).stem().u8string();
	return SanitizeFileNamePart(std::string(stem.begin(), stem.end()), "untitled");
}

std::string pwg::debug::FormatFileTimestamp(std::chrono::system_clock::time_point time)
{
	const std::time_t seconds = std::chrono::system_clock::to_time_t(time);
	std::tm parts{};
	localtime_s(&parts, &seconds);
	char text[32];
	std::strftime(text, sizeof(text), "%Y%m%d-%H%M%S", &parts);
	return text;
}

std::string pwg::debug::UniqueStem(const std::filesystem::path& folder, const std::string& stem, size_t suffixPosition,
	const std::vector<const char*>& extensions)
{
	const size_t split = std::min(suffixPosition, stem.size());
	const std::string head = stem.substr(0, split);
	const std::string tail = stem.substr(split);
	auto taken = [&](const std::string& candidate)
	{
		std::error_code error;
		for (const char* extension : extensions)
		{
			if (std::filesystem::exists(folder / (candidate + extension), error))
			{
				return true;
			}
		}
		return false;
	};

	std::string candidate = stem;
	for (int counter = 2; taken(candidate) && counter < 10000; ++counter)
	{
		candidate = head + "-" + std::to_string(counter) + tail;
	}
	return candidate;
}

void pwg::debug::FlipRowsVertically(uint8_t* data, size_t rowBytes, size_t rows)
{
	if (!data || rows < 2 || rowBytes == 0)
	{
		return;
	}
	std::vector<uint8_t> scratch(rowBytes);
	for (size_t top = 0, bottom = rows - 1; top < bottom; ++top, --bottom)
	{
		uint8_t* a = data + top * rowBytes;
		uint8_t* b = data + bottom * rowBytes;
		std::memcpy(scratch.data(), a, rowBytes);
		std::memcpy(a, b, rowBytes);
		std::memcpy(b, scratch.data(), rowBytes);
	}
}

bool pwg::debug::WritePng(const std::filesystem::path& path, int width, int height, int channels, const uint8_t* data, int strideBytes)
{
	std::ofstream out(path, std::ios::binary);
	if (!out.is_open())
	{
		return false;
	}
	// stb keeps the write flip as a global; PWG's texture loaders set the (separate) load flip globally, so the
	// write flip is set explicitly here: `data` is already top row first.
	stbi_flip_vertically_on_write(0);
	const int ok = stbi_write_png_to_func(StbWriteToStream, &out, width, height, channels, data, strideBytes);
	out.close();
	return ok != 0 && !out.fail();
}

// --- Screenshot (D-07) ---

pwg::debug::CameraPose pwg::debug::MakeCameraPose(const std::string& camera, const glm::mat4& view, const glm::mat4& projection,
	float fovDeg, float nearPlane, float farPlane, float aspectRatio)
{
	CameraPose pose;
	pose.camera = camera;
	pose.view = view;
	pose.projection = projection;
	pose.fovDeg = fovDeg;
	pose.nearPlane = nearPlane;
	pose.farPlane = farPlane;
	pose.aspectRatio = aspectRatio;

	// Camera-to-world: columns are right, up, -forward and the eye position.
	const glm::mat4 cameraToWorld = glm::inverse(view);
	pose.position = glm::vec3(cameraToWorld[3]);
	pose.forward = glm::normalize(-glm::vec3(cameraToWorld[2]));
	pose.up = glm::normalize(glm::vec3(cameraToWorld[1]));

	const glm::vec3& f = pose.forward;
	pose.yawDeg = glm::degrees(std::atan2(f.z, f.x));
	pose.pitchDeg = glm::degrees(std::asin(std::clamp(f.y, -1.0f, 1.0f)));

	// Roll as in FlyoverCamera::UpdateMatrices: up = levelUp * cos(roll) + right * sin(roll).
	const glm::vec3 right = glm::cross(f, glm::vec3(0.0f, 1.0f, 0.0f));
	if (glm::length(right) > 1e-6f)
	{
		const glm::vec3 levelRight = glm::normalize(right);
		const glm::vec3 levelUp = glm::cross(levelRight, f);
		pose.rollDeg = glm::degrees(std::atan2(glm::dot(pose.up, levelRight), glm::dot(pose.up, levelUp)));
	}
	return pose;
}

bool pwg::debug::Exporters::RequestScreenshot(ScreenshotRequest request)
{
	if (m_pendingScreenshot)
	{
		return false;
	}
	m_pendingScreenshot = std::move(request);
	return true;
}

const pwg::debug::ScreenshotResult& pwg::debug::Exporters::CaptureScreenshot(const ScreenshotSource& source,
	const Metadata& metadata, std::chrono::system_clock::time_point time, uint64_t engineFrame)
{
	ScreenshotResult result;
	result.engineFrame = engineFrame;
	result.width = source.width;
	result.height = source.height;
	auto finish = [&](std::string error) -> const ScreenshotResult&
	{
		result.error = std::move(error);
		if (!result.error.empty())
		{
			PWG_ERROR("Screenshot: {}", result.error);
		}
		m_lastScreenshot = std::move(result);
		m_hasScreenshotResult = true;
		return m_lastScreenshot;
	};

	if (!m_pendingScreenshot)
	{
		return finish("no screenshot was requested");
	}
	const ScreenshotRequest request = std::move(*m_pendingScreenshot);
	m_pendingScreenshot.reset();

	if (source.texture == 0 || source.width <= 0 || source.height <= 0)
	{
		return finish("the scene framebuffer is empty (Main View has no size)");
	}

	// --- File names (design 7.5): <project>_<camera>_<yyyyMMdd-HHmmss>[_<tag>] ---
	const std::filesystem::path folder = request.root / ScreenshotFolderName;
	std::error_code fsError;
	std::filesystem::create_directories(folder, fsError);
	if (fsError)
	{
		return finish("cannot create " + Utf8(folder) + ": " + fsError.message());
	}
	const std::string base = ProjectNameFromMetadata(metadata) + "_" + SanitizeFileNamePart(source.camera.camera, "camera")
		+ "_" + FormatFileTimestamp(time);
	const std::string tag = request.tag.empty() ? std::string() : SanitizeFileNamePart(request.tag, "tag");
	const std::string stem = UniqueStem(folder, tag.empty() ? base : base + "_" + tag, base.size(), { ".png", ".yaml" });
	result.png = folder / (stem + ".png");
	result.sidecar = folder / (stem + ".yaml");

	// --- Readback: RGBA8 rows, GL row 0 (bottom) first ---
	const Clock::time_point readStart = Clock::now();
	const size_t rowBytes = static_cast<size_t>(source.width) * 4u;
	const size_t byteSize = rowBytes * static_cast<size_t>(source.height);
	m_pixelScratch.resize(byteSize);
	GLint packBuffer = 0;
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer);
	if (packBuffer != 0)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);	// Otherwise the pointer below would be an offset into that buffer.
	}
	glGetTextureImage(source.texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(byteSize), m_pixelScratch.data());
	if (packBuffer != 0)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(packBuffer));
	}
	result.readbackMs = MillisecondsSince(readStart);

	// --- PNG: top row first, as on screen (ImGui shows the texture with flipped v) ---
	const Clock::time_point writeStart = Clock::now();
	FlipRowsVertically(m_pixelScratch.data(), rowBytes, static_cast<size_t>(source.height));
	result.pixelsSha256 = ComputeSha256(m_pixelScratch.data(), byteSize);
	if (!WritePng(result.png, source.width, source.height, 4, m_pixelScratch.data(), static_cast<int>(rowBytes)))
	{
		return finish("cannot write " + Utf8(result.png));
	}
	result.writeMs = MillisecondsSince(writeStart);

	// --- Sidecar ---
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "format" << YAML::Value << ScreenshotFormatName;
	out << YAML::Key << "format_version" << YAML::Value << ScreenshotFormatVersion;
	out << YAML::Key << "file" << YAML::Value << YAML::DoubleQuoted << Utf8(result.png.filename());
	out << YAML::Key << "tag" << YAML::Value;
	if (tag.empty())
	{
		out << YAML::Null;
	}
	else
	{
		out << YAML::DoubleQuoted << tag;
	}
	out << YAML::Key << "timestamp_local" << YAML::Value << YAML::DoubleQuoted << FormatIsoTime(time, false);
	out << YAML::Key << "engine_frame" << YAML::Value << engineFrame;
	out << YAML::Key << "width" << YAML::Value << source.width;
	out << YAML::Key << "height" << YAML::Value << source.height;
	out << YAML::Key << "channels" << YAML::Value << 4;
	out << YAML::Key << "pixel_format" << YAML::Value << "rgba8";
	out << YAML::Key << "row_order" << YAML::Value << "top_to_bottom";
	out << YAML::Key << "source" << YAML::Value << "main_view_post_processed";
	out << YAML::Key << "color_encoding" << YAML::Value << "display_gamma_2.2";
	out << YAML::Key << "pixels_sha256" << YAML::Value << YAML::DoubleQuoted << result.pixelsSha256;
	out << YAML::Key << "debug_mode" << YAML::Value << YAML::DoubleQuoted << source.debugMode;
	out << YAML::Key << "wireframe" << YAML::Value << source.wireframe;

	out << YAML::Key << "camera" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "type" << YAML::Value << source.camera.camera;
	EmitVec3(out, "position", source.camera.position);
	EmitVec3(out, "forward", source.camera.forward);
	EmitVec3(out, "up", source.camera.up);
	EmitFloat(out, "yaw_deg", source.camera.yawDeg);
	EmitFloat(out, "pitch_deg", source.camera.pitchDeg);
	EmitFloat(out, "roll_deg", source.camera.rollDeg);
	EmitFloat(out, "fov_deg", source.camera.fovDeg);
	EmitFloat(out, "near", source.camera.nearPlane);
	EmitFloat(out, "far", source.camera.farPlane);
	EmitFloat(out, "aspect", source.camera.aspectRatio);
	EmitMat4(out, "view_matrix", source.camera.view);
	EmitMat4(out, "projection_matrix", source.camera.projection);
	out << YAML::EndMap;

	out << YAML::Key << "timing" << YAML::Value << YAML::BeginMap;
	EmitMs(out, "readback_ms", result.readbackMs);
	EmitMs(out, "png_write_ms", result.writeMs);
	out << YAML::EndMap;

	out << YAML::Key << "metadata" << YAML::Value;
	EmitMetadataYaml(out, metadata);
	out << YAML::EndMap;

	if (!out.good())
	{
		return finish(std::string("sidecar: YAML emitter error: ") + out.GetLastError());
	}
	char header[96];
	std::snprintf(header, sizeof(header), "# PWG Debug View screenshot (%s %d)\n", ScreenshotFormatName, ScreenshotFormatVersion);
	if (!WriteTextFile(result.sidecar, header, out.c_str()))
	{
		return finish("cannot write " + Utf8(result.sidecar));
	}

	result.written = true;
	PWG_INFO("Screenshot {}x{} written to {} (readback {:.2f} ms, PNG {:.2f} ms)",
		source.width, source.height, Utf8(result.png), result.readbackMs, result.writeMs);
	return finish({});
}
