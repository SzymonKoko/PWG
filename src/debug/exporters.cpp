#include <glad/glad.h>
#include <yaml-cpp/yaml.h>
#include <stb/stb_image_write.h>
#include <glm/gtc/type_ptr.hpp>

#include "exporters.h"
#include "core/logger/logger.h"
#include "terrain/chunk/chunkManager.h"
#include "terrain/graph/universalTerrainGraph.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <ctime>
#include <fstream>
#include <limits>
#include <system_error>
#include <unordered_map>

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

// --- Mask and heightmap export (D-08, design 7.4) ---

namespace
{
	static_assert(std::endian::native == std::endian::little, "the .f32 format is little-endian; a big-endian build would need a byte swap");
	static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559, "the .f32 format is IEEE 754 float32");

	using MaskMap = std::unordered_map<std::string, std::shared_ptr<pwg::TerrainMask>>;

	/** @brief Pixel transfer format for glGetTextureImage and a readable name of the internal format. */
	struct MaskFormat
	{
		GLenum format{ GL_RED };
		int channels{ 0 };
		const char* name{ "" };
	};

	bool LookupMaskFormat(GLint internalFormat, MaskFormat& out)
	{
		switch (internalFormat)
		{
		case GL_R8: out = { GL_RED, 1, "R8" }; return true;
		case GL_R16: out = { GL_RED, 1, "R16" }; return true;
		case GL_R16F: out = { GL_RED, 1, "R16F" }; return true;
		case GL_R32F: out = { GL_RED, 1, "R32F" }; return true;
		case GL_RG8: out = { GL_RG, 2, "RG8" }; return true;
		case GL_RG16: out = { GL_RG, 2, "RG16" }; return true;
		case GL_RG16F: out = { GL_RG, 2, "RG16F" }; return true;
		case GL_RG32F: out = { GL_RG, 2, "RG32F" }; return true;
		case GL_RGB8: out = { GL_RGB, 3, "RGB8" }; return true;
		case GL_RGB16: out = { GL_RGB, 3, "RGB16" }; return true;
		case GL_RGB16F: out = { GL_RGB, 3, "RGB16F" }; return true;
		case GL_RGB32F: out = { GL_RGB, 3, "RGB32F" }; return true;
		case GL_RGBA8: out = { GL_RGBA, 4, "RGBA8" }; return true;
		case GL_SRGB8_ALPHA8: out = { GL_RGBA, 4, "SRGB8_ALPHA8" }; return true;
		case GL_RGBA16: out = { GL_RGBA, 4, "RGBA16" }; return true;
		case GL_RGBA16F: out = { GL_RGBA, 4, "RGBA16F" }; return true;
		case GL_RGBA32F: out = { GL_RGBA, 4, "RGBA32F" }; return true;
		default: return false;
		}
	}

	/** @brief Graph temporary masks are named `node_<id>_<port>` (TerrainComputePipeline). */
	bool ParseTempMaskName(const std::string& name, int& nodeId, std::string& port)
	{
		constexpr size_t PrefixLength = 5;	// "node_"
		if (name.compare(0, PrefixLength, "node_") != 0)
		{
			return false;
		}
		const size_t separator = name.find('_', PrefixLength);
		if (separator == std::string::npos || separator == PrefixLength || separator + 1 >= name.size())
		{
			return false;
		}
		int id = 0;
		for (size_t i = PrefixLength; i < separator; ++i)
		{
			if (!std::isdigit(static_cast<unsigned char>(name[i])) || id > 100000000)
			{
				return false;
			}
			id = id * 10 + (name[i] - '0');
		}
		nodeId = id;
		port = name.substr(separator + 1);
		return true;
	}

	const pwg::TerrainGraphParameter* FindParameter(const pwg::TerrainGraphNodeInstance& node, const char* id)
	{
		for (const pwg::TerrainGraphParameter& parameter : node.parameters)
		{
			if (parameter.id == id)
			{
				return &parameter;
			}
		}
		return nullptr;
	}

	/**
	* @brief Node and output port that produced the mask `name` in the current graph asset:
	* 1. a temporary mask `node_<id>_<port>` names it directly;
	* 2. FinalHeight and `outputMask` targets are copies of the node linked into the output node's input
	*    (the last such output node in execution order wins, as in ExecuteUniversalGraph);
	* 3. otherwise a temporary mask that shares the texture (e.g. `node_7_rivers` and RiverChannelMask), the
	*    last in execution order.
	* Engine masks that no node writes (BaseHeight, NormalMask, ...) have no source node.
	*/
	pwg::debug::MaskSourceNode ResolveMaskSource(const std::string& name, const pwg::TerrainMask* mask, const MaskMap& masks,
		const pwg::TerrainGraphAsset& asset)
	{
		pwg::debug::MaskSourceNode source;
		auto fromNode = [&](int nodeId, const std::string& port)
		{
			const pwg::TerrainGraphNodeInstance* node = asset.FindNode(nodeId);
			if (!node)
			{
				return false;
			}
			source.known = true;
			source.id = node->id;
			source.type = node->typeId;
			source.port = port;
			return true;
		};

		int nodeId = 0;
		std::string port;
		if (ParseTempMaskName(name, nodeId, port) && fromNode(nodeId, port))
		{
			return source;
		}

		const pwg::TerrainGraphValidationResult validation = pwg::ValidateTerrainGraphAsset(asset);
		std::unordered_map<int, size_t> orderOf;
		for (size_t i = 0; i < validation.executionOrder.size(); ++i)
		{
			orderOf[validation.executionOrder[i]] = i;
		}

		const pwg::TerrainGraphLink* outputLink = nullptr;
		for (int id : validation.executionOrder)
		{
			const pwg::TerrainGraphNodeInstance* node = asset.FindNode(id);
			if (!node || !node->enabled)
			{
				continue;
			}
			const char* inputPort = nullptr;
			if (node->typeId == "outputHeight" && name == "FinalHeight")
			{
				inputPort = "height";
			}
			else if (node->typeId == "outputMask")
			{
				const pwg::TerrainGraphParameter* target = FindParameter(*node, "target");
				if ((target ? target->stringValue : std::string("FinalMask")) == name)
				{
					inputPort = "mask";
				}
			}
			if (inputPort)
			{
				if (const pwg::TerrainGraphLink* link = asset.FindInputLink(node->id, inputPort))
				{
					outputLink = link;
				}
			}
		}
		if (outputLink && fromNode(outputLink->fromNodeId, outputLink->fromPortId))
		{
			return source;
		}

		size_t bestOrder = 0;
		int bestId = 0;
		std::string bestPort;
		for (const auto& [otherName, other] : masks)
		{
			if (other.get() != mask || otherName == name || !ParseTempMaskName(otherName, nodeId, port))
			{
				continue;
			}
			const auto order = orderOf.find(nodeId);
			if (order != orderOf.end() && (bestPort.empty() || order->second > bestOrder))
			{
				bestOrder = order->second;
				bestId = nodeId;
				bestPort = port;
			}
		}
		if (!bestPort.empty())
		{
			fromNode(bestId, bestPort);
		}
		return source;
	}

	void EmitOptionalFloat(YAML::Emitter& out, const char* key, float value, bool available)
	{
		out << YAML::Key << key << YAML::Value;
		if (available)
		{
			out << FloatText(value);
		}
		else
		{
			out << YAML::Null;
		}
	}

	void EmitMetadataValue(YAML::Emitter& out, const char* key, const pwg::debug::Metadata& metadata)
	{
		out << YAML::Key << key << YAML::Value;
		const pwg::debug::Metadata::Entry* entry = metadata.Find(key);
		if (entry && entry->kind != pwg::debug::Metadata::Kind::EMPTY && !entry->value.empty())
		{
			out << YAML::DoubleQuoted << entry->value;
		}
		else
		{
			out << YAML::Null;
		}
	}

	/** @brief Restores the pixel pack state that the mask readback sets. */
	class PackStateGuard
	{
	public:
		PackStateGuard()
		{
			glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &m_buffer);
			glGetIntegerv(GL_PACK_ALIGNMENT, &m_alignment);
			glGetIntegerv(GL_PACK_ROW_LENGTH, &m_rowLength);
			glGetIntegerv(GL_PACK_SKIP_ROWS, &m_skipRows);
			glGetIntegerv(GL_PACK_SKIP_PIXELS, &m_skipPixels);
			glGetIntegerv(GL_PACK_SWAP_BYTES, &m_swapBytes);
			if (m_buffer != 0)
			{
				glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);	// Otherwise the pointer would be an offset into that buffer.
			}
			// Tightly packed float rows, no byte swap.
			glPixelStorei(GL_PACK_ALIGNMENT, 4);
			glPixelStorei(GL_PACK_ROW_LENGTH, 0);
			glPixelStorei(GL_PACK_SKIP_ROWS, 0);
			glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
			glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
		}
		~PackStateGuard()
		{
			glPixelStorei(GL_PACK_ALIGNMENT, m_alignment);
			glPixelStorei(GL_PACK_ROW_LENGTH, m_rowLength);
			glPixelStorei(GL_PACK_SKIP_ROWS, m_skipRows);
			glPixelStorei(GL_PACK_SKIP_PIXELS, m_skipPixels);
			glPixelStorei(GL_PACK_SWAP_BYTES, m_swapBytes);
			if (m_buffer != 0)
			{
				glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(m_buffer));
			}
		}
		PackStateGuard(const PackStateGuard&) = delete;
		PackStateGuard& operator=(const PackStateGuard&) = delete;

	private:
		GLint m_buffer{ 0 };
		GLint m_alignment{ 4 };
		GLint m_rowLength{ 0 };
		GLint m_skipRows{ 0 };
		GLint m_skipPixels{ 0 };
		GLint m_swapBytes{ 0 };
	};
}

const std::vector<pwg::debug::MaskExportResult>& pwg::debug::Exporters::ExportMasks(ChunkManager& chunks,
	const MaskExportRequest& request, const Metadata& metadata, std::chrono::system_clock::time_point time, uint64_t engineFrame)
{
	m_lastMaskExport.clear();
	m_lastMaskExport.reserve(request.names.size());
	for (const std::string& name : request.names)
	{
		m_lastMaskExport.push_back(ExportMask(chunks, name, request, metadata, time, engineFrame));
	}
	m_hasMaskResult = true;
	// The buffers of a full-size mask are tens of MB; do not keep them between exports.
	m_maskScratch.clear();
	m_maskScratch.shrink_to_fit();
	m_maskVerifyScratch.clear();
	m_maskVerifyScratch.shrink_to_fit();
	return m_lastMaskExport;
}

pwg::debug::MaskExportResult pwg::debug::Exporters::ExportMask(ChunkManager& chunks, const std::string& name,
	const MaskExportRequest& request, const Metadata& metadata, std::chrono::system_clock::time_point time, uint64_t engineFrame)
{
	MaskExportResult result;
	result.name = name;
	result.engineFrame = engineFrame;
	result.generationRevision = chunks.GetGenerationRevision();
	auto fail = [&](std::string error) -> MaskExportResult
	{
		result.error = std::move(error);
		result.written = false;
		PWG_ERROR("Mask export {}: {}", name, result.error);
		return result;
	};

	MaskMap& masks = chunks.GetMasks();
	const auto found = masks.find(name);
	if (found == masks.end() || !found->second || !found->second->texture)
	{
		return fail("no such mask");
	}
	const std::shared_ptr<TerrainMask> mask = found->second;
	const GLuint texture = mask->texture->GetTextureID();
	if (texture == 0)
	{
		return fail("the mask has no GL texture");
	}

	// --- Size and format as stored on the GPU (not as the Texture object remembers them) ---
	GLint width = 0;
	GLint height = 0;
	GLint internalFormat = 0;
	glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
	glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
	glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
	MaskFormat format;
	if (!LookupMaskFormat(internalFormat, format))
	{
		char text[64];
		std::snprintf(text, sizeof(text), "unsupported internal format 0x%04X", static_cast<unsigned int>(internalFormat));
		return fail(text);
	}
	if (width <= 0 || height <= 0)
	{
		return fail("the mask texture is empty");
	}
	result.width = width;
	result.height = height;
	result.channels = format.channels;
	result.glInternalFormat = format.name;
	const size_t valueCount = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(format.channels);
	const size_t byteSize = valueCount * sizeof(float);
	result.byteSize = byteSize;
	if (byteSize > static_cast<size_t>(std::numeric_limits<GLsizei>::max()))
	{
		return fail("the mask is larger than 2 GiB");
	}

	// --- File names: <root>/masks/<name>[_<tag>] (design 7.4), -2, -3, ... if taken ---
	const std::filesystem::path folder = request.root / MaskFolderName;
	std::error_code fsError;
	std::filesystem::create_directories(folder, fsError);
	if (fsError)
	{
		return fail("cannot create " + Utf8(folder) + ": " + fsError.message());
	}
	const std::string tag = request.tag.empty() ? std::string() : SanitizeFileNamePart(request.tag, "tag");
	const std::string baseStem = SanitizeFileNamePart(name, "mask") + (tag.empty() ? std::string() : "_" + tag);
	const std::string stem = UniqueStem(folder, baseStem, std::string::npos, { ".f32", ".yaml", ".png" });
	result.raw = folder / (stem + ".f32");
	result.sidecar = folder / (stem + ".yaml");

	// --- Readback: float32, GL texel row 0 first, channels interleaved; no flip (design 7.4) ---
	const Clock::time_point readStart = Clock::now();
	m_maskScratch.assign(valueCount, 0.0f);
	{
		PackStateGuard packState;
		// Image stores of the compute passes must be visible to the read below.
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		while (glGetError() != GL_NO_ERROR)
		{
			// Errors of earlier calls would otherwise be reported as a failed readback.
		}
		glGetTextureImage(texture, 0, format.format, GL_FLOAT, static_cast<GLsizei>(byteSize), m_maskScratch.data());
		const GLenum glError = glGetError();
		if (glError != GL_NO_ERROR)
		{
			char text[64];
			std::snprintf(text, sizeof(text), "glGetTextureImage failed (GL error 0x%04X)", static_cast<unsigned int>(glError));
			return fail(text);
		}
	}
	result.readbackMs = MillisecondsSince(readStart);

	// --- Statistics, hash, raw file ---
	const Clock::time_point rawStart = Clock::now();
	const int channels = format.channels;
	result.channelMin.assign(channels, std::numeric_limits<float>::infinity());
	result.channelMax.assign(channels, -std::numeric_limits<float>::infinity());
	for (size_t i = 0; i < valueCount; ++i)
	{
		const float value = m_maskScratch[i];
		if (!std::isfinite(value))
		{
			++result.nonFinite;
			continue;
		}
		const size_t channel = i % static_cast<size_t>(channels);
		result.channelMin[channel] = std::min(result.channelMin[channel], value);
		result.channelMax[channel] = std::max(result.channelMax[channel], value);
	}
	const bool anyFinite = result.nonFinite < valueCount;
	result.min = *std::min_element(result.channelMin.begin(), result.channelMin.end());
	result.max = *std::max_element(result.channelMax.begin(), result.channelMax.end());
	result.sha256 = ComputeSha256(m_maskScratch.data(), byteSize);
	{
		std::ofstream out(result.raw, std::ios::binary | std::ios::trunc);
		if (!out.is_open())
		{
			return fail("cannot write " + Utf8(result.raw));
		}
		out.write(reinterpret_cast<const char*>(m_maskScratch.data()), static_cast<std::streamsize>(byteSize));
		out.close();
		if (out.fail())
		{
			return fail("cannot write " + Utf8(result.raw));
		}
	}
	result.rawWriteMs = MillisecondsSince(rawStart);

	// --- Round trip (acceptance, design 7.4): the file read back equals the exported buffer byte for byte ---
	const Clock::time_point verifyStart = Clock::now();
	{
		std::ifstream in(result.raw, std::ios::binary);
		const uint64_t fileSize = std::filesystem::file_size(result.raw, fsError);
		if (!in.is_open() || fsError)
		{
			return fail("cannot read back " + Utf8(result.raw));
		}
		if (fileSize != byteSize)
		{
			return fail("round trip: file has " + std::to_string(fileSize) + " bytes, expected " + std::to_string(byteSize));
		}
		m_maskVerifyScratch.resize(byteSize);
		in.read(reinterpret_cast<char*>(m_maskVerifyScratch.data()), static_cast<std::streamsize>(byteSize));
		result.roundTripIdentical = in.gcount() == static_cast<std::streamsize>(byteSize)
			&& std::memcmp(m_maskVerifyScratch.data(), m_maskScratch.data(), byteSize) == 0;
	}
	result.verifyMs = MillisecondsSince(verifyStart);
	if (!result.roundTripIdentical)
	{
		return fail("round trip: the file read back differs from the exported buffer");
	}

	// --- PNG preview: (v - min) / (max - min) per channel to 0..255, top row first (flipped) ---
	if (request.writePreview)
	{
		const Clock::time_point previewStart = Clock::now();
		const int previewChannels = channels == 1 ? 1 : 3;		// 2 channels: R, G, B = 0; 4 channels: alpha dropped.
		const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
		m_pixelScratch.assign(pixelCount * static_cast<size_t>(previewChannels), 0);
		for (int c = 0; c < std::min(channels, previewChannels); ++c)
		{
			// In double, so the preview can be recomputed exactly from the .f32 and the sidecar (D-12):
			// floor((v - min) * (255 / (max - min)) + 0.5).
			const double low = static_cast<double>(result.channelMin[c]);
			const double range = static_cast<double>(result.channelMax[c]) - low;
			if (!(range > 0.0) || !std::isfinite(range))
			{
				continue;	// Constant channel (or no finite value): preview 0.
			}
			const double scale = 255.0 / range;
			for (size_t p = 0; p < pixelCount; ++p)
			{
				const float value = m_maskScratch[p * static_cast<size_t>(channels) + c];
				if (!std::isfinite(value))
				{
					continue;
				}
				const double scaled = std::clamp((static_cast<double>(value) - low) * scale, 0.0, 255.0);
				m_pixelScratch[p * static_cast<size_t>(previewChannels) + c] = static_cast<uint8_t>(std::lround(scaled));
			}
		}
		const size_t rowBytes = static_cast<size_t>(width) * static_cast<size_t>(previewChannels);
		FlipRowsVertically(m_pixelScratch.data(), rowBytes, static_cast<size_t>(height));
		result.png = folder / (stem + ".png");
		if (!WritePng(result.png, width, height, previewChannels, m_pixelScratch.data(), static_cast<int>(rowBytes)))
		{
			return fail("cannot write " + Utf8(result.png));
		}
		result.previewMs = MillisecondsSince(previewStart);
	}

	// --- Source node, seeds, simulation state ---
	const std::shared_ptr<TerrainComputePipeline> pipeline = chunks.GetTerrainComputePipeline();
	const TerrainGraphAsset* asset = pipeline ? &static_cast<const TerrainComputePipeline&>(*pipeline).GetUniversalGraphAsset() : nullptr;
	if (asset)
	{
		result.source = ResolveMaskSource(name, mask.get(), masks, *asset);
	}
	for (const auto& [otherName, other] : masks)
	{
		if (other == mask && otherName != name)
		{
			result.aliases.push_back(otherName);
		}
	}
	std::sort(result.aliases.begin(), result.aliases.end());

	// --- Sidecar (design 7.4 keys first, then additions) ---
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "format" << YAML::Value << MaskFormatName;
	out << YAML::Key << "format_version" << YAML::Value << MaskFormatVersion;
	out << YAML::Key << "name" << YAML::Value << YAML::DoubleQuoted << name;
	out << YAML::Key << "file" << YAML::Value << YAML::DoubleQuoted << Utf8(result.raw.filename());
	out << YAML::Key << "dtype" << YAML::Value << "float32";
	out << YAML::Key << "endianness" << YAML::Value << "little";
	out << YAML::Key << "width" << YAML::Value << width;
	out << YAML::Key << "height" << YAML::Value << height;
	out << YAML::Key << "channels" << YAML::Value << channels;
	out << YAML::Key << "row_order" << YAML::Value << "bottom_to_top";
	out << YAML::Comment("GL texel row 0 first");
	out << YAML::Key << "byte_size" << YAML::Value << static_cast<unsigned long long>(byteSize);
	EmitOptionalFloat(out, "min", result.min, anyFinite);
	EmitOptionalFloat(out, "max", result.max, anyFinite);

	out << YAML::Key << "source_node" << YAML::Value << YAML::Flow << YAML::BeginMap;
	if (result.source.known)
	{
		out << YAML::Key << "id" << YAML::Value << result.source.id;
		out << YAML::Key << "type" << YAML::Value << YAML::DoubleQuoted << result.source.type;
		out << YAML::Key << "port" << YAML::Value << YAML::DoubleQuoted << result.source.port;
	}
	else
	{
		out << YAML::Key << "id" << YAML::Value << YAML::Null;
		out << YAML::Key << "type" << YAML::Value << YAML::Null;
		out << YAML::Key << "port" << YAML::Value << YAML::Null;
	}
	out << YAML::EndMap;

	// All enabled nodes with a seed parameter, by node id.
	out << YAML::Key << "seeds" << YAML::Value << YAML::Flow << YAML::BeginMap;
	if (asset)
	{
		std::vector<const TerrainGraphNodeInstance*> nodes;
		for (const TerrainGraphNodeInstance& node : asset->nodes)
		{
			if (node.enabled && FindParameter(node, "seed"))
			{
				nodes.push_back(&node);
			}
		}
		std::sort(nodes.begin(), nodes.end(), [](const auto* a, const auto* b) { return a->id < b->id; });
		for (const TerrainGraphNodeInstance* node : nodes)
		{
			const TerrainGraphParameter* seed = FindParameter(*node, "seed");
			out << YAML::Key << node->id << YAML::Value;
			if (seed->type == TerrainGraphParameterType::Float)
			{
				out << FloatText(seed->floatValue);
			}
			else
			{
				out << seed->intValue;
			}
		}
	}
	out << YAML::EndMap;

	out << YAML::Key << "generation_revision" << YAML::Value << static_cast<unsigned long long>(result.generationRevision);
	out << YAML::Key << "simulation" << YAML::Value << YAML::Flow << YAML::BeginMap;
	if (pipeline)
	{
		out << YAML::Key << "hydraulic_steps" << YAML::Value << pipeline->GetParticleStepCount();
		out << YAML::Key << "thermal_steps" << YAML::Value << pipeline->GetThermalStepCount();
		out << YAML::Key << "hydrology_generated" << YAML::Value << pipeline->IsHydrologyGenerated();
		out << YAML::Key << "hydrology_running" << YAML::Value << pipeline->IsHydrologyRunning();
	}
	out << YAML::EndMap;
	EmitMetadataValue(out, "project_path", metadata);
	EmitMetadataValue(out, "pwg_commit", metadata);

	// Additions to design 7.4.
	out << YAML::Key << "sha256" << YAML::Value << YAML::DoubleQuoted << result.sha256;
	out << YAML::Key << "roundtrip_identical" << YAML::Value << result.roundTripIdentical;
	out << YAML::Key << "non_finite_count" << YAML::Value << static_cast<unsigned long long>(result.nonFinite);
	out << YAML::Key << "channel_min" << YAML::Value << YAML::Flow << YAML::BeginSeq;
	for (int c = 0; c < channels; ++c)
	{
		if (std::isfinite(result.channelMin[c])) out << FloatText(result.channelMin[c]); else out << YAML::Null;
	}
	out << YAML::EndSeq;
	out << YAML::Key << "channel_max" << YAML::Value << YAML::Flow << YAML::BeginSeq;
	for (int c = 0; c < channels; ++c)
	{
		if (std::isfinite(result.channelMax[c])) out << FloatText(result.channelMax[c]); else out << YAML::Null;
	}
	out << YAML::EndSeq;
	out << YAML::Key << "gl_internal_format" << YAML::Value << result.glInternalFormat;
	out << YAML::Key << "aliases" << YAML::Value << YAML::Flow << YAML::BeginSeq;
	for (const std::string& alias : result.aliases)
	{
		out << YAML::DoubleQuoted << alias;
	}
	out << YAML::EndSeq;
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
	out << YAML::Key << "engine_frame" << YAML::Value << static_cast<unsigned long long>(engineFrame);

	out << YAML::Key << "preview" << YAML::Value << YAML::BeginMap;
	if (request.writePreview)
	{
		out << YAML::Key << "file" << YAML::Value << YAML::DoubleQuoted << Utf8(result.png.filename());
		out << YAML::Key << "channels" << YAML::Value << (channels == 1 ? 1 : 3);
		out << YAML::Key << "normalization" << YAML::Value << "per_channel_min_max";
		out << YAML::Key << "row_order" << YAML::Value << "top_to_bottom";
	}
	else
	{
		out << YAML::Key << "file" << YAML::Value << YAML::Null;
	}
	out << YAML::EndMap;

	out << YAML::Key << "timing" << YAML::Value << YAML::BeginMap;
	EmitMs(out, "readback_ms", result.readbackMs);
	EmitMs(out, "raw_write_ms", result.rawWriteMs);
	EmitMs(out, "verify_ms", result.verifyMs);
	EmitMs(out, "png_write_ms", result.previewMs);
	out << YAML::EndMap;

	out << YAML::Key << "metadata" << YAML::Value;
	EmitMetadataYaml(out, metadata);
	out << YAML::EndMap;

	if (!out.good())
	{
		return fail(std::string("sidecar: YAML emitter error: ") + out.GetLastError());
	}
	char header[96];
	std::snprintf(header, sizeof(header), "# PWG Debug View mask export (%s %d)\n", MaskFormatName, MaskFormatVersion);
	if (!WriteTextFile(result.sidecar, header, out.c_str()))
	{
		return fail("cannot write " + Utf8(result.sidecar));
	}

	result.written = true;
	PWG_INFO("Mask {} {}x{}x{} exported to {} (readback {:.2f} ms, raw {:.2f} ms, verify {:.2f} ms, PNG {:.2f} ms, sha256 {})",
		name, width, height, channels, Utf8(result.raw), result.readbackMs, result.rawWriteMs, result.verifyMs,
		result.previewMs, result.sha256);
	return result;
}
