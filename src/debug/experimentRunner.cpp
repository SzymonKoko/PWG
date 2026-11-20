#include <glad/glad.h>
#include "experimentRunner.h"
#include "debug/debugView.h"
#include "core/gui/gui.h"
#include "core/logger/logger.h"
#include "core/window/window.h"
#include "project/projectSerializer.h"
#include "scene/editorScene.h"
#include "scene/sceneManager.h"
#include "terrain/chunk/chunkManager.h"
#include "terrain/lod/LODManager.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <system_error>

namespace
{
	using pwg::debug::ExperimentCapture;
	using pwg::debug::ExperimentConfig;
	using pwg::debug::ExperimentParam;
	using pwg::debug::ExperimentStep;
	using pwg::debug::ExperimentValue;
	using pwg::debug::ExperimentVariant;

	std::string PathText(const std::filesystem::path& path)
	{
		const std::u8string text = path.u8string();
		return std::string(text.begin(), text.end());
	}

	/** @brief `path` relative to `base`, written with '/' so run.yaml reads the same on every system. */
	std::string RelativeText(const std::filesystem::path& path, const std::filesystem::path& base)
	{
		const std::u8string text = std::filesystem::relative(path, base).generic_u8string();
		return std::string(text.begin(), text.end());
	}

	std::filesystem::path PathFromText(const std::string& text)
	{
		return std::filesystem::path(std::u8string(text.begin(), text.end()));
	}

	std::string FormatNumber(double value)
	{
		std::ostringstream stream;
		stream.precision(12);
		stream << value;
		return stream.str();
	}

	std::string ReadTextFile(const std::filesystem::path& path)
	{
		std::ifstream in(path, std::ios::binary);
		std::ostringstream buffer;
		buffer << in.rdbuf();
		return buffer.str();
	}

	bool ParseNumber(const std::string& text, double& value)
	{
		if (text.empty())
		{
			return false;
		}
		errno = 0;
		char* end = nullptr;
		const double parsed = std::strtod(text.c_str(), &end);
		if (end == text.c_str() || *end != '\0' || errno == ERANGE || !std::isfinite(parsed))
		{
			return false;
		}
		value = parsed;
		return true;
	}

	bool IsInteger(double value)
	{
		return std::isfinite(value) && std::floor(value) == value && std::fabs(value) < 2.0e9;
	}

	// ---- Config reading ----

	bool CheckKeys(const YAML::Node& node, std::initializer_list<const char*> allowed, const std::string& where, std::string& error)
	{
		if (!node.IsMap())
		{
			error = where + " must be a map";
			return false;
		}
		for (const auto& entry : node)
		{
			const std::string key = entry.first.as<std::string>("");
			bool known = false;
			for (const char* name : allowed)
			{
				known = known || key == name;
			}
			if (!known)
			{
				error = "unknown key '" + key + "' in " + where;
				return false;
			}
		}
		return true;
	}

	bool ParseValue(const YAML::Node& node, ExperimentValue& value, std::string& error, const std::string& where)
	{
		if (node.IsSequence())
		{
			value.kind = ExperimentValue::Kind::LIST;
			value.list.clear();
			value.text.clear();
			for (const auto& item : node)
			{
				double number = 0.0;
				if (!item.IsScalar() || !ParseNumber(item.Scalar(), number))
				{
					error = where + ": a list holds numbers only";
					return false;
				}
				value.list.push_back(number);
				value.text += (value.text.empty() ? "" : "/") + FormatNumber(number);
			}
			return true;
		}
		if (!node.IsScalar())
		{
			error = where + ": value must be a scalar or a list of numbers";
			return false;
		}
		value.text = node.Scalar();
		if (value.text == "true" || value.text == "false")
		{
			value.kind = ExperimentValue::Kind::BOOL;
			value.boolean = value.text == "true";
			return true;
		}
		double number = 0.0;
		if (ParseNumber(value.text, number))
		{
			value.kind = ExperimentValue::Kind::NUMBER;
			value.number = number;
			return true;
		}
		value.kind = ExperimentValue::Kind::STRING;
		return true;
	}

	/** @brief `graph.node.<id>.<param>`: splits the key. */
	bool SplitGraphNodeKey(const std::string& key, int& nodeId, std::string& param)
	{
		const std::string prefix = "graph.node.";
		if (key.compare(0, prefix.size(), prefix) != 0)
		{
			return false;
		}
		const size_t dot = key.find('.', prefix.size());
		if (dot == std::string::npos || dot + 1 >= key.size())
		{
			return false;
		}
		double id = 0.0;
		if (!ParseNumber(key.substr(prefix.size(), dot - prefix.size()), id) || !IsInteger(id) || id < 1.0)
		{
			return false;
		}
		nodeId = static_cast<int>(id);
		param = key.substr(dot + 1);
		return true;
	}

	/** @brief The fixed list of parameters the runner may set (design 8.3). Checks key and value type. */
	bool ValidateParam(const ExperimentParam& param, std::string& error)
	{
		const std::string& key = param.first;
		const ExperimentValue& value = param.second;
		using Kind = ExperimentValue::Kind;
		auto needBool = [&]()
		{
			if (value.kind != Kind::BOOL)
			{
				error = "parameter '" + key + "' needs true or false";
				return false;
			}
			return true;
		};
		auto needInt = [&](double low, double high)
		{
			if (value.kind != Kind::NUMBER || !IsInteger(value.number) || value.number < low || value.number > high)
			{
				error = "parameter '" + key + "' needs an integer from " + FormatNumber(low) + " to " + FormatNumber(high);
				return false;
			}
			return true;
		};

		if (key == "terrain.chunk_count")
		{
			return needInt(3, 31);
		}
		if (key == "terrain.chunk_size")
		{
			return needInt(33, 257);
		}
		if (key == "graph.file")
		{
			if (value.kind != Kind::STRING)
			{
				error = "parameter 'graph.file' needs a file name";
				return false;
			}
			return true;
		}
		int nodeId = 0;
		std::string name;
		if (SplitGraphNodeKey(key, nodeId, name))
		{
			if (name == "enabled")
			{
				return needBool();
			}
			return true;	// The parameter id and type are checked against the loaded graph.
		}
		if (key == "lod.enabled" || key == "culling.enabled" || key == "vegetation.enabled" || key == "shadows.enabled" || key == "water.visible")
		{
			return needBool();
		}
		if (key == "lod.scale")
		{
			if (value.kind != Kind::NUMBER || !(value.number > 0.0) || value.number > pwg::LODManager::MaxScale)
			{
				error = "parameter 'lod.scale' needs a number above 0 and up to " + FormatNumber(pwg::LODManager::MaxScale);
				return false;
			}
			return true;
		}
		if (key == "lod.distances")
		{
			bool valid = value.kind == Kind::LIST && value.list.size() == static_cast<size_t>(pwg::LODManager::ThresholdCount);
			for (size_t i = 0; valid && i < value.list.size(); ++i)
			{
				valid = value.list[i] >= 0.0 && (i == 0 || value.list[i] >= value.list[i - 1]);
			}
			if (!valid)
			{
				error = "parameter 'lod.distances' needs four numbers, not decreasing, from 0";
				return false;
			}
			return true;
		}
		error = "unknown parameter '" + key + "'";
		return false;
	}

	int ParamGroup(const std::string& key)
	{
		if (key.compare(0, 8, "terrain.") == 0) return 0;
		if (key == "graph.file") return 1;
		if (key.compare(0, 11, "graph.node.") == 0) return 2;
		return 3;
	}

	bool ReadParams(const YAML::Node& node, std::vector<ExperimentParam>& out, const std::string& where, std::string& error)
	{
		if (!node.IsMap())
		{
			error = where + " must be a map of parameter: value";
			return false;
		}
		for (const auto& entry : node)
		{
			ExperimentParam param;
			param.first = entry.first.as<std::string>("");
			if (!ParseValue(entry.second, param.second, error, where + " '" + param.first + "'") || !ValidateParam(param, error))
			{
				error = where + ": " + error;
				return false;
			}
			out.push_back(std::move(param));
		}
		return true;
	}

	bool ReadFlag(const YAML::Node& node, const char* key, bool& value, const std::string& where, std::string& error)
	{
		const YAML::Node child = node[key];
		if (!child)
		{
			return true;
		}
		const std::string text = child.IsScalar() ? child.Scalar() : "";
		if (text != "true" && text != "false")
		{
			error = where + "." + key + " needs true or false";
			return false;
		}
		value = text == "true";
		return true;
	}

	bool ReadNumber(const YAML::Node& node, const char* key, double& value, double low, double high, const std::string& where, std::string& error)
	{
		const YAML::Node child = node[key];
		if (!child)
		{
			return true;
		}
		double number = 0.0;
		if (!child.IsScalar() || !ParseNumber(child.Scalar(), number) || number < low || number > high)
		{
			error = where + "." + key + " needs a number from " + FormatNumber(low) + " to " + FormatNumber(high);
			return false;
		}
		value = number;
		return true;
	}

	const char* StepTypeName(ExperimentStep::Type type)
	{
		switch (type)
		{
		case ExperimentStep::Type::RUN_GRAPH: return "run_graph";
		case ExperimentStep::Type::REFRESH_ASSETS: return "refresh_assets";
		case ExperimentStep::Type::HYDROLOGY: return "hydrology";
		case ExperimentStep::Type::HYDRAULIC: return "hydraulic";
		case ExperimentStep::Type::THERMAL: return "thermal";
		case ExperimentStep::Type::SAVE: return "save";
		case ExperimentStep::Type::RELOAD: return "reload";
		case ExperimentStep::Type::WAIT: return "wait";
		}
		return "?";
	}

	bool ReadStep(const YAML::Node& node, ExperimentStep& step, const std::string& where, std::string& error)
	{
		if (!node.IsMap())
		{
			error = where + " must be a map";
			return false;
		}
		std::string action;
		YAML::Node value;
		for (const auto& entry : node)
		{
			const std::string key = entry.first.as<std::string>("");
			if (key == "name")
			{
				step.label = entry.second.as<std::string>("");
				continue;
			}
			if (!action.empty())
			{
				error = where + " has more than one action ('" + action + "' and '" + key + "')";
				return false;
			}
			action = key;
			value = entry.second;
		}
		if (action.empty())
		{
			error = where + " has no action";
			return false;
		}

		const std::string at = where + " (" + action + ")";
		auto steps = [&](const char* key, int& out, int low, int high)
		{
			double number = 0.0;
			if (!ReadNumber(value, key, number, low, high, at, error) || (value[key] && !IsInteger(number)))
			{
				if (error.empty())
				{
					error = at + "." + key + " needs an integer";
				}
				return false;
			}
			if (value[key])
			{
				out = static_cast<int>(number);
			}
			return true;
		};

		if (action == "run_graph" || action == "refresh_assets")
		{
			if (!value.IsScalar() || value.Scalar() != "true")
			{
				error = at + " needs true";
				return false;
			}
			step.type = action == "run_graph" ? ExperimentStep::Type::RUN_GRAPH : ExperimentStep::Type::REFRESH_ASSETS;
		}
		else if (action == "hydrology")
		{
			if (!value.IsScalar() || value.Scalar() != "generate")
			{
				error = at + " needs generate";
				return false;
			}
			step.type = ExperimentStep::Type::HYDROLOGY;
		}
		else if (action == "hydraulic" || action == "thermal")
		{
			if (!CheckKeys(value, { "steps", "per_frame", "until", "max_steps" }, at, error))
			{
				return false;
			}
			step.type = action == "hydraulic" ? ExperimentStep::Type::HYDRAULIC : ExperimentStep::Type::THERMAL;
			if (!steps("per_frame", step.perFrame, 1, 100000))
			{
				return false;
			}
			if (value["until"])
			{
				if (step.type != ExperimentStep::Type::THERMAL || value["until"].Scalar() != "stable" || value["steps"])
				{
					error = at + ": 'until: stable' is for thermal erosion and replaces 'steps'";
					return false;
				}
				step.untilStable = true;
				step.maxSteps = 20000;
				if (!steps("max_steps", step.maxSteps, 1, 10000000))
				{
					return false;
				}
			}
			else
			{
				if (!value["steps"] || value["max_steps"])
				{
					error = at + " needs steps (or, for thermal, until: stable with max_steps)";
					return false;
				}
				if (!steps("steps", step.steps, 1, 10000000))
				{
					return false;
				}
			}
		}
		else if (action == "save" || action == "reload")
		{
			step.type = action == "save" ? ExperimentStep::Type::SAVE : ExperimentStep::Type::RELOAD;
			if (value.IsMap())
			{
				if (!CheckKeys(value, { "file" }, at, error))
				{
					return false;
				}
				step.file = value["file"].as<std::string>("");
			}
			else if (value.IsScalar() && step.type == ExperimentStep::Type::RELOAD && (value.Scalar() == "project" || value.Scalar() == "true"))
			{
				step.file.clear();
			}
			else
			{
				error = at + " needs { file: name.pwg }" + (step.type == ExperimentStep::Type::RELOAD ? " or 'project'" : "");
				return false;
			}
			if (step.type == ExperimentStep::Type::SAVE && step.file.empty())
			{
				error = at + " needs a file";
				return false;
			}
			if (!step.file.empty())
			{
				const std::filesystem::path file(step.file);
				if (file.is_absolute() || step.file.find("..") != std::string::npos)
				{
					error = at + ": the file is relative to the run folder and may not leave it";
					return false;
				}
			}
		}
		else if (action == "wait")
		{
			if (!CheckKeys(value, { "frames" }, at, error))
			{
				return false;
			}
			step.type = ExperimentStep::Type::WAIT;
			step.frames = 1;
			if (!steps("frames", step.frames, 1, 10000000))
			{
				return false;
			}
		}
		else
		{
			error = where + ": unknown action '" + action + "'";
			return false;
		}
		return true;
	}

	bool ReadCapture(const YAML::Node& node, ExperimentCapture& capture, const std::string& where, std::string& error)
	{
		if (!CheckKeys(node, { "after", "screenshot", "masks", "preview" }, where, error))
		{
			return false;
		}
		if (node["after"])
		{
			capture.after = node["after"].as<std::string>("");
		}
		if (node["screenshot"])
		{
			const YAML::Node shot = node["screenshot"];
			capture.screenshot = true;
			if (shot.IsMap())
			{
				if (!CheckKeys(shot, { "path_frame", "tag" }, where + ".screenshot", error))
				{
					return false;
				}
				double frame = -1.0;
				if (!ReadNumber(shot, "path_frame", frame, 0, 100000000, where + ".screenshot", error) || (shot["path_frame"] && !IsInteger(frame)))
				{
					if (error.empty())
					{
						error = where + ".screenshot.path_frame needs an integer";
					}
					return false;
				}
				capture.pathFrame = shot["path_frame"] ? static_cast<int>(frame) : -1;
				capture.tag = shot["tag"].as<std::string>("");
			}
			else if (!(shot.IsScalar() && shot.Scalar() == "true"))
			{
				error = where + ".screenshot needs true or { path_frame, tag }";
				return false;
			}
		}
		if (node["masks"])
		{
			if (!node["masks"].IsSequence())
			{
				error = where + ".masks must be a list of mask names";
				return false;
			}
			for (const auto& name : node["masks"])
			{
				capture.masks.push_back(name.as<std::string>(""));
				if (capture.masks.back().empty())
				{
					error = where + ".masks holds an empty name";
					return false;
				}
			}
		}
		if (!ReadFlag(node, "preview", capture.preview, where, error))
		{
			return false;
		}
		if (!capture.screenshot && capture.masks.empty())
		{
			error = where + " needs a screenshot or masks";
			return false;
		}
		return true;
	}

	std::string TimestampStamp(std::chrono::system_clock::time_point time)
	{
		std::string stamp = pwg::debug::FormatIsoTime(time, false);
		std::replace(stamp.begin(), stamp.end(), ':', '-');
		return stamp;
	}

	// ---- run.yaml emission ----

	void KeyString(YAML::Emitter& out, const char* key, const std::string& value)
	{
		out << YAML::Key << key << YAML::Value << YAML::DoubleQuoted << value;
	}

	void KeyNumber(YAML::Emitter& out, const char* key, double value)
	{
		out << YAML::Key << key << YAML::Value << FormatNumber(value);
	}

	void KeyBool(YAML::Emitter& out, const char* key, bool value)
	{
		out << YAML::Key << key << YAML::Value << (value ? "true" : "false");
	}

	void KeyStrings(YAML::Emitter& out, const char* key, const std::vector<std::string>& values)
	{
		out << YAML::Key << key << YAML::Value;
		if (values.empty())
		{
			out << YAML::Flow;
		}
		out << YAML::BeginSeq;
		for (const std::string& value : values)
		{
			out << YAML::DoubleQuoted << value;
		}
		out << YAML::EndSeq;
	}

	void AppendText(const std::filesystem::path& file, const std::string& text)
	{
		std::ofstream out(file, std::ios::binary | std::ios::app);
		out << '\n' << text << '\n';
	}

	constexpr uint64_t SettleFrames = 3;
	constexpr uint64_t ScreenshotTimeoutFrames = 600;
	constexpr double HydrologyTimeoutSeconds = 1800.0;
}

std::string pwg::debug::ExperimentStep::Describe() const
{
	std::string text = StepTypeName(type);
	switch (type)
	{
	case Type::HYDRAULIC:
		text += " steps=" + std::to_string(steps) + " per_frame=" + std::to_string(perFrame);
		break;
	case Type::THERMAL:
		if (untilStable)
		{
			text += " until=stable max_steps=" + std::to_string(maxSteps) + " per_frame=" + std::to_string(perFrame);
		}
		else
		{
			text += " steps=" + std::to_string(steps) + " per_frame=" + std::to_string(perFrame);
		}
		break;
	case Type::SAVE:
	case Type::RELOAD:
		text += " file=" + (file.empty() ? std::string("project") : file);
		break;
	case Type::WAIT:
		text += " frames=" + std::to_string(frames);
		break;
	default:
		break;
	}
	return text;
}

bool pwg::debug::LoadExperimentConfig(const std::filesystem::path& file, const std::filesystem::path& outputOverride,
	ExperimentConfig& config, std::string& error)
{
	config = ExperimentConfig{};
	std::error_code ec;
	const std::filesystem::path absolute = std::filesystem::absolute(file, ec);
	if (ec || !std::filesystem::is_regular_file(absolute, ec))
	{
		error = "config file not found: " + PathText(file);
		return false;
	}
	config.file = absolute;
	config.fileSha256 = ComputeFileSha256(absolute);
	const std::filesystem::path base = absolute.parent_path();

	YAML::Node root;
	try
	{
		root = YAML::LoadFile(PathText(absolute));
	}
	catch (const std::exception& e)
	{
		error = std::string("config is not valid YAML: ") + e.what();
		return false;
	}

	try
	{
		if (!CheckKeys(root, { "format", "format_version", "experiment_id", "description", "project", "expect_renderer", "output",
			"debug_view", "benchmark", "manual", "params", "prepare", "camera", "warmup", "measure", "record", "repetitions",
			"order", "cooldown_s", "variants", "captures", "memory_snapshots" }, "the config", error))
		{
			return false;
		}
		if (root["format"].as<std::string>("") != "pwg-experiment" || root["format_version"].as<int>(0) != 1)
		{
			error = "the config needs format: pwg-experiment and format_version: 1";
			return false;
		}
		config.experimentId = root["experiment_id"].as<std::string>("");
		if (config.experimentId.empty() ||
			config.experimentId.find_first_of("\\/:*?\"<>|") != std::string::npos || config.experimentId.find("..") != std::string::npos)
		{
			error = "experiment_id is missing or not usable as a file name";
			return false;
		}
		config.description = root["description"].as<std::string>("");
		if (!root["project"])
		{
			error = "the config needs project";
			return false;
		}
		config.project = (base / PathFromText(root["project"].as<std::string>(""))).lexically_normal();
		if (!std::filesystem::is_regular_file(config.project, ec))
		{
			error = "project not found: " + PathText(config.project);
			return false;
		}
		config.expectRenderer = root["expect_renderer"].as<std::string>("");
		if (!outputOverride.empty())
		{
			config.output = std::filesystem::absolute(outputOverride, ec).lexically_normal();
		}
		else if (root["output"])
		{
			config.output = (base / PathFromText(root["output"].as<std::string>(""))).lexically_normal();
		}
		else
		{
			config.output = (base / "results" / config.experimentId).lexically_normal();
		}
		if (!ReadFlag(root, "memory_snapshots", config.memorySnapshots, "the config", error))
		{
			return false;
		}
		if (!ReadFlag(root, "debug_view", config.debugView, "the config", error))
		{
			return false;
		}

		if (root["benchmark"])
		{
			const YAML::Node b = root["benchmark"];
			if (!CheckKeys(b, { "enabled", "render_size", "fullscreen_main_view", "gpu_ring_frames", "wallclock_bracket", "max_gpu_dropped" }, "benchmark", error) ||
				!ReadFlag(b, "enabled", config.benchmarkEnabled, "benchmark", error) ||
				!ReadFlag(b, "fullscreen_main_view", config.fullscreenMainView, "benchmark", error) ||
				!ReadFlag(b, "wallclock_bracket", config.wallclockBracket, "benchmark", error))
			{
				return false;
			}
			if (b["render_size"])
			{
				double w = 0.0;
				double h = 0.0;
				if (!b["render_size"].IsSequence() || b["render_size"].size() != 2 ||
					!ParseNumber(b["render_size"][0].Scalar(), w) || !ParseNumber(b["render_size"][1].Scalar(), h) ||
					!IsInteger(w) || !IsInteger(h) || w < 16 || h < 16 || w > 8192 || h > 8192)
				{
					error = "benchmark.render_size needs [width, height] from 16 to 8192";
					return false;
				}
				config.renderWidth = static_cast<int>(w);
				config.renderHeight = static_cast<int>(h);
			}
			double ring = config.gpuRingFrames;
			double dropped = 0.0;
			if (!ReadNumber(b, "gpu_ring_frames", ring, 2, GpuProfiler::MaxRingFrames, "benchmark", error) ||
				!ReadNumber(b, "max_gpu_dropped", dropped, 0, 1.0e9, "benchmark", error))
			{
				return false;
			}
			config.gpuRingFrames = static_cast<int>(ring);
			config.maxGpuDropped = static_cast<uint64_t>(dropped);
		}

		if (root["manual"])
		{
			if (!root["manual"].IsMap())
			{
				error = "manual must be a map";
				return false;
			}
			for (const auto& entry : root["manual"])
			{
				config.manual.emplace_back(entry.first.as<std::string>(""), entry.second.as<std::string>(""));
			}
		}
		if (root["params"] && !ReadParams(root["params"], config.baseParams, "params", error))
		{
			return false;
		}

		if (root["prepare"])
		{
			if (!root["prepare"].IsSequence())
			{
				error = "prepare must be a list";
				return false;
			}
			for (const auto& entry : root["prepare"])
			{
				ExperimentStep step;
				if (!ReadStep(entry, step, "prepare entry " + std::to_string(config.prepare.size() + 1), error))
				{
					return false;
				}
				config.prepare.push_back(std::move(step));
			}
			// Default labels: the action name, numbered from the second use on.
			for (size_t i = 0; i < config.prepare.size(); ++i)
			{
				ExperimentStep& step = config.prepare[i];
				if (!step.label.empty())
				{
					continue;
				}
				const std::string base = StepTypeName(step.type);
				int count = 0;
				for (size_t j = 0; j <= i; ++j)
				{
					count += std::string(StepTypeName(config.prepare[j].type)) == base ? 1 : 0;
				}
				step.label = count == 1 ? base : base + "_" + std::to_string(count);
			}
			for (size_t i = 0; i < config.prepare.size(); ++i)
			{
				for (size_t j = 0; j < i; ++j)
				{
					if (config.prepare[i].label == config.prepare[j].label)
					{
						error = "two prepare entries have the name '" + config.prepare[i].label + "'";
						return false;
					}
				}
			}
		}

		if (root["camera"])
		{
			const YAML::Node c = root["camera"];
			if (!CheckKeys(c, { "path", "dt", "loop", "matrix_dump" }, "camera", error) ||
				!ReadFlag(c, "loop", config.cameraLoop, "camera", error) ||
				!ReadFlag(c, "matrix_dump", config.matrixDump, "camera", error) ||
				!ReadNumber(c, "dt", config.cameraStep, 1.0e-4, 1.0, "camera", error))
			{
				return false;
			}
			if (c["path"])
			{
				config.cameraPath = (base / PathFromText(c["path"].as<std::string>(""))).lexically_normal();
				if (!std::filesystem::is_regular_file(config.cameraPath, ec))
				{
					error = "camera path not found: " + PathText(config.cameraPath);
					return false;
				}
			}
			if (config.cameraPath.empty() && (c["loop"] || c["matrix_dump"]))
			{
				error = "camera.loop and camera.matrix_dump need camera.path";
				return false;
			}
		}

		double frames = 0.0;
		for (const char* key : { "warmup", "measure" })
		{
			if (!root[key])
			{
				continue;
			}
			if (!CheckKeys(root[key], { "frames" }, key, error) || !ReadNumber(root[key], "frames", frames, 0, 10000000, key, error) ||
				(root[key]["frames"] && !IsInteger(frames)))
			{
				if (error.empty())
				{
					error = std::string(key) + ".frames needs an integer";
				}
				return false;
			}
			(std::string(key) == "warmup" ? config.warmupFrames : config.measureFrames) = static_cast<uint64_t>(frames);
		}

		if (root["record"])
		{
			const std::string record = root["record"].as<std::string>("");
			if (record == "measure") config.record = ExperimentConfig::Record::MEASURE;
			else if (record == "prepare") config.record = ExperimentConfig::Record::PREPARE;
			else if (record == "none") config.record = ExperimentConfig::Record::NONE;
			else
			{
				error = "record needs measure, prepare or none";
				return false;
			}
		}
		else
		{
			config.record = config.measureFrames > 0 ? ExperimentConfig::Record::MEASURE : ExperimentConfig::Record::NONE;
		}
		if (config.record == ExperimentConfig::Record::MEASURE && config.measureFrames == 0)
		{
			error = "record: measure needs measure.frames above 0";
			return false;
		}
		if (config.record == ExperimentConfig::Record::PREPARE && config.prepare.empty())
		{
			error = "record: prepare needs prepare entries";
			return false;
		}
		if (config.measureFrames > 0 && config.record == ExperimentConfig::Record::PREPARE)
		{
			error = "measure.frames with record: prepare: one CSV window per run, choose what it covers (record: measure or none)";
			return false;
		}

		if (config.memorySnapshots && config.record == ExperimentConfig::Record::NONE)
		{
			error = "memory_snapshots: true with record: none writes nothing; use record: prepare or measure";
			return false;
		}

		double repetitions = 1.0;
		if (!ReadNumber(root, "repetitions", repetitions, 1, 1000, "the config", error) ||
			!ReadNumber(root, "cooldown_s", config.cooldownSeconds, 0, 86400, "the config", error))
		{
			return false;
		}
		config.repetitions = static_cast<int>(repetitions);
		if (root["order"])
		{
			const std::string order = root["order"].as<std::string>("");
			if (order != "interleaved" && order != "sequential")
			{
				error = "order needs interleaved or sequential";
				return false;
			}
			config.interleaved = order == "interleaved";
		}

		// Variants: an explicit list or a matrix (design 8.2).
		const YAML::Node variants = root["variants"];
		if (!variants)
		{
			config.variants.push_back({ "default", {} });
		}
		else if (variants.IsMap())
		{
			if (!CheckKeys(variants, { "matrix" }, "variants", error) || !variants["matrix"].IsMap() || variants["matrix"].size() == 0)
			{
				if (error.empty())
				{
					error = "variants.matrix must be a map of parameter: [values]";
				}
				return false;
			}
			std::vector<std::pair<std::string, std::vector<ExperimentValue>>> axes;
			for (const auto& entry : variants["matrix"])
			{
				if (!entry.second.IsSequence() || entry.second.size() == 0)
				{
					error = "variants.matrix '" + entry.first.as<std::string>("") + "' needs a list of values";
					return false;
				}
				std::vector<ExperimentValue> values;
				for (const auto& item : entry.second)
				{
					ExperimentValue value;
					if (!ParseValue(item, value, error, "variants.matrix"))
					{
						return false;
					}
					values.push_back(std::move(value));
				}
				axes.emplace_back(entry.first.as<std::string>(""), std::move(values));
			}
			std::vector<size_t> index(axes.size(), 0);
			while (true)
			{
				ExperimentVariant variant;
				for (size_t a = 0; a < axes.size(); ++a)
				{
					const ExperimentValue& value = axes[a].second[index[a]];
					variant.params.emplace_back(axes[a].first, value);
					variant.name += (a == 0 ? "" : ";") + axes[a].first + "=" + value.text;
				}
				config.variants.push_back(std::move(variant));
				size_t a = axes.size();
				while (a > 0 && ++index[a - 1] == axes[a - 1].second.size())
				{
					index[--a] = 0;
				}
				if (a == 0)
				{
					break;
				}
			}
		}
		else if (variants.IsSequence())
		{
			for (const auto& entry : variants)
			{
				ExperimentVariant variant;
				if (!CheckKeys(entry, { "name", "params" }, "a variant", error))
				{
					return false;
				}
				variant.name = entry["name"].as<std::string>("");
				if (variant.name.empty())
				{
					error = "every variant needs a name";
					return false;
				}
				if (entry["params"] && !ReadParams(entry["params"], variant.params, "variant '" + variant.name + "'", error))
				{
					return false;
				}
				config.variants.push_back(std::move(variant));
			}
			if (config.variants.empty())
			{
				error = "variants is empty";
				return false;
			}
		}
		else
		{
			error = "variants must be a list or { matrix: ... }";
			return false;
		}
		for (size_t i = 0; i < config.variants.size(); ++i)
		{
			for (size_t j = 0; j < i; ++j)
			{
				if (config.variants[i].name == config.variants[j].name)
				{
					error = "two variants have the name '" + config.variants[i].name + "'";
					return false;
				}
			}
		}

		// A parameter that changes the terrain or the graph needs a step that applies it.
		bool needsApply = false;
		for (const ExperimentVariant& variant : config.variants)
		{
			const std::vector<const std::vector<ExperimentParam>*> lists{ &config.baseParams, &variant.params };
			for (const auto* list : lists)
			{
				for (const ExperimentParam& param : *list)
				{
					needsApply = needsApply || ParamGroup(param.first) <= 2;
				}
			}
		}
		if (needsApply && std::none_of(config.prepare.begin(), config.prepare.end(), [](const ExperimentStep& step)
			{ return step.type == ExperimentStep::Type::RUN_GRAPH || step.type == ExperimentStep::Type::REFRESH_ASSETS; }))
		{
			error = "terrain.* and graph.* parameters need prepare: run_graph (or refresh_assets for asset layers) to take effect";
			return false;
		}

		if (root["captures"])
		{
			if (!root["captures"].IsSequence())
			{
				error = "captures must be a list";
				return false;
			}
			for (const auto& entry : root["captures"])
			{
				ExperimentCapture capture;
				const std::string where = "capture " + std::to_string(config.captures.size() + 1);
				if (!ReadCapture(entry, capture, where, error))
				{
					return false;
				}
				const bool isStep = std::any_of(config.prepare.begin(), config.prepare.end(), [&](const ExperimentStep& step) { return step.label == capture.after; });
				if (capture.after != "prepare" && capture.after != "measure" && !isStep)
				{
					error = where + ": after: '" + capture.after + "' is neither prepare, measure nor the name of a prepare entry";
					return false;
				}
				if (capture.after == "measure" && config.measureFrames == 0)
				{
					error = where + ": after: measure needs measure.frames";
					return false;
				}
				if (isStep && config.record == ExperimentConfig::Record::PREPARE)
				{
					error = where + ": captures after a prepare entry cannot run inside the recorded prepare window (use record: none or measure)";
					return false;
				}
				if (capture.pathFrame >= 0 && config.cameraPath.empty())
				{
					error = where + ": path_frame needs camera.path";
					return false;
				}
				config.captures.push_back(std::move(capture));
			}
		}
	}
	catch (const std::exception& e)
	{
		error = std::string("config error: ") + e.what();
		return false;
	}
	return true;
}

// -------------------------------------------------------------------------------------------------
// Runner
// -------------------------------------------------------------------------------------------------

pwg::debug::ExperimentRunner::ExperimentRunner(ExperimentHost host)
	: m_host(host)
{
}

pwg::debug::ExperimentRunner::~ExperimentRunner()
{
	if (DebugView::HasInstance() && DebugView::Get().GetExperimentRunner() == this)
	{
		DebugView::Get().SetExperimentRunner(nullptr);
	}
}

pwg::ChunkManager* pwg::debug::ExperimentRunner::GetChunks() const
{
	EditorScene* editor = GetEditor();
	return editor && editor->GetTerrain() ? editor->GetTerrain()->GetChunkManager().get() : nullptr;
}

pwg::EditorScene* pwg::debug::ExperimentRunner::GetEditor() const
{
	return m_host.scene.GetEditorScene();
}

void pwg::debug::ExperimentRunner::Notice(const std::string& message)
{
	m_message = message;
	PWG_INFO("Experiment: {}", message);
}

bool pwg::debug::ExperimentRunner::Start(const std::filesystem::path& configFile, const std::filesystem::path& outputOverride,
	bool exitWhenDone, std::string& error)
{
	if (m_state == State::RUNNING)
	{
		error = "a run is active";
		return false;
	}
	m_exitWhenDone = exitWhenDone;
	m_wantsExit = false;
	if (!LoadExperimentConfig(configFile, outputOverride, m_config, error))
	{
		m_state = State::REJECTED;
		m_message = "Config rejected: " + error;
		PWG_ERROR("Experiment: {}", m_message);
		return false;
	}
	if (!GetEditor() || !GetChunks())
	{
		error = "the editor scene is not ready";
		m_state = State::REJECTED;
		return false;
	}

	m_units.clear();
	const int repetitions = m_config.repetitions;
	const size_t variants = m_config.variants.size();
	int index = 0;
	if (m_config.interleaved)
	{
		for (int r = 1; r <= repetitions; ++r)
		{
			for (size_t v = 0; v < variants; ++v)
			{
				m_units.push_back({ ++index, v, r });
			}
		}
	}
	else
	{
		for (size_t v = 0; v < variants; ++v)
		{
			for (int r = 1; r <= repetitions; ++r)
			{
				m_units.push_back({ ++index, v, r });
			}
		}
	}

	// Output folder, the config copy (design 7.1) and a stamp that makes the run ids unique.
	std::error_code ec;
	m_outputRoot = m_config.output;
	std::filesystem::create_directories(m_outputRoot, ec);
	if (ec)
	{
		error = "cannot create the output folder " + PathText(m_outputRoot) + ": " + ec.message();
		m_state = State::REJECTED;
		m_message = error;
		PWG_ERROR("Experiment: {}", error);
		return false;
	}
	const std::string text = ReadTextFile(m_config.file);
	std::filesystem::path copy = m_outputRoot / (m_config.experimentId + ".yaml");
	if (std::filesystem::exists(copy, ec) && ReadTextFile(copy) != text)
	{
		// An earlier run used another version of the config: keep both, the copy is named after its hash.
		copy = m_outputRoot / (m_config.experimentId + "-" + m_config.fileSha256.substr(0, 8) + ".yaml");
	}
	if (!std::filesystem::exists(copy, ec))
	{
		std::ofstream out(copy, std::ios::binary);
		out.write(text.data(), static_cast<std::streamsize>(text.size()));
	}

	m_stamp = TimestampStamp(std::chrono::system_clock::now());
	for (int attempt = 2; std::filesystem::exists(m_outputRoot / (m_stamp + "_r01"), ec); ++attempt)
	{
		m_stamp = TimestampStamp(std::chrono::system_clock::now()) + "-" + std::to_string(attempt);
	}

	m_unitIndex = 0;
	m_unitsFailed = 0;
	m_anyFailed = false;
	m_abortRequested = false;
	m_firstUnitChecked = false;
	m_failures.clear();
	m_state = State::RUNNING;
	SetPhase(Phase::UNIT_BEGIN);
	Notice("run of " + m_config.experimentId + " started: " + std::to_string(m_units.size()) + " units, output " + PathText(m_outputRoot));
	return true;
}

void pwg::debug::ExperimentRunner::Abort()
{
	if (m_state == State::RUNNING)
	{
		m_abortRequested = true;
	}
}

int pwg::debug::ExperimentRunner::GetExitCode() const
{
	if (m_state == State::DONE && !m_anyFailed && m_unitsFailed == 0)
	{
		return 0;
	}
	return m_state == State::IDLE ? 0 : 1;
}

void pwg::debug::ExperimentRunner::SetPhase(Phase phase, uint64_t phaseFrames)
{
	m_phase = phase;
	m_phaseFrame = 0;
	m_phaseFrames = phaseFrames;
}

std::string pwg::debug::ExperimentRunner::PhaseName() const
{
	switch (m_phase)
	{
	case Phase::UNIT_BEGIN: return "start of unit";
	case Phase::LOAD: return "loading project";
	case Phase::SETTLE_LOAD: return "settling after load";
	case Phase::APPLY: return "applying parameters";
	case Phase::SETTLE_APPLY: return "settling after parameters";
	case Phase::BENCH_ON: return "benchmark mode on";
	case Phase::SETTLE_BENCH: return "settling after benchmark mode";
	case Phase::PREPARE_RECORD: return "starting the prepare window";
	case Phase::PREPARE_STEPS:
		return m_stepIndex < m_config.prepare.size() ? "prepare: " + m_config.prepare[m_stepIndex].label : std::string("prepare");
	case Phase::PREPARE_DRAIN: return "writing the prepare window";
	case Phase::CAPTURE: return "captures";
	case Phase::PRE_WARMUP: return "preparing warm-up";
	case Phase::WARMUP: return "warm-up";
	case Phase::MEASURE_START: return "starting the measurement";
	case Phase::MEASURE: return "measuring";
	case Phase::MEASURE_DRAIN: return "writing the measurement";
	case Phase::RESTORE: return "restoring";
	case Phase::FINISH: return "writing the report";
	case Phase::COOLDOWN: return "cooldown";
	case Phase::FINISHED: return "finished";
	default: return "idle";
	}
}

pwg::debug::ExperimentRunner::Status pwg::debug::ExperimentRunner::GetStatus() const
{
	Status status;
	status.state = m_state;
	status.phase = PhaseName();
	status.experimentId = m_config.experimentId;
	status.unitsTotal = static_cast<int>(m_units.size());
	status.unitsFailed = m_unitsFailed;
	status.outputFolder = m_outputRoot;
	status.message = m_message;
	if (m_state == State::RUNNING && m_unitIndex < m_units.size())
	{
		const Unit& unit = m_units[m_unitIndex];
		status.unit = unit.index;
		status.variant = m_config.variants[unit.variant].name;
		status.repetition = unit.repetition;
		status.phaseFrame = m_phaseFrame;
		status.phaseFrames = m_phaseFrames;
		status.unitFolder = m_unitFolder;
	}
	return status;
}

pwg::debug::RunInfo pwg::debug::ExperimentRunner::MakeRunInfo() const
{
	return m_run;
}

void pwg::debug::ExperimentRunner::Tick()
{
	if (m_state != State::RUNNING)
	{
		return;
	}
	if (m_abortRequested)
	{
		++m_unitsFailed;
		m_anyFailed = true;
		if (m_unitOpen)
		{
			m_failures.push_back("aborted by the user");
			CloseUnit(false);
		}
		EndRun(State::ABORTED);
		return;
	}
	++m_phaseFrame;

	switch (m_phase)
	{
	case Phase::UNIT_BEGIN:
		BeginUnit();
		break;

	case Phase::LOAD:
	{
		if (!m_host.gui.OpenProjectFromPath(PathText(m_config.project), false))
		{
			FailUnit("the project could not be loaded: " + PathText(m_config.project));
			break;
		}
		SetPhase(Phase::SETTLE_LOAD, SettleFrames);
		break;
	}

	case Phase::SETTLE_LOAD:
		if (m_phaseFrame >= m_phaseFrames)
		{
			SetPhase(Phase::APPLY);
		}
		break;

	case Phase::APPLY:
	{
		std::string error;
		if (!ApplyParameters(error))
		{
			FailUnit(error);
			break;
		}
		SetPhase(Phase::SETTLE_APPLY, SettleFrames);
		break;
	}

	case Phase::SETTLE_APPLY:
		if (m_phaseFrame >= m_phaseFrames)
		{
			SetPhase(Phase::BENCH_ON);
		}
		break;

	case Phase::BENCH_ON:
	{
		DebugView& view = DebugView::Get();
		view.SetGpuRingFrames(static_cast<uint32_t>(m_config.gpuRingFrames));
		view.SetWallclockBracket(m_config.wallclockBracket);
		if (m_config.benchmarkEnabled)
		{
			view.SetBenchmarkRenderSize(m_config.renderWidth, m_config.renderHeight);
			view.SetBenchmarkMode(true);
		}
		SetPhase(Phase::SETTLE_BENCH, SettleFrames);
		break;
	}

	case Phase::SETTLE_BENCH:
		if (m_phaseFrame >= m_phaseFrames)
		{
			m_stepIndex = 0;
			m_memoryStartTaken = false;
			m_memoryEndTaken = false;
			m_stepStarted = false;
			m_stepFrames = 0;
			if (m_config.record == ExperimentConfig::Record::PREPARE)
			{
				SetPhase(Phase::PREPARE_RECORD);
			}
			else
			{
				SetPhase(Phase::PREPARE_STEPS);
			}
		}
		break;

	case Phase::PREPARE_RECORD:
	{
		// The window begins with the next frame, so the first step runs one tick later and falls inside it.
		if (!DebugView::Get().StartRecording(m_run, m_outputRoot, 0))
		{
			FailUnit("the CSV window could not be started");
			break;
		}
		m_windowActive = true;
		SetPhase(Phase::PREPARE_STEPS);
		break;
	}

	case Phase::PREPARE_STEPS:
		TickPrepare();
		break;

	case Phase::PREPARE_DRAIN:
	{
		if (DebugView::Get().GetRecorder().IsActive())
		{
			break;
		}
		const ExportResult& result = DebugView::Get().GetRecorder().GetLastResult();
		m_windowWritten = result.written;
		m_windowFrames = result.frames;
		m_windowGpuDropped = result.gpuDropped;
		m_windowMaxSleepMs = result.maxSleepMs;
		if (!result.written)
		{
			FailUnit("the CSV files were not written: " + result.error);
			break;
		}
		if (result.gpuDropped > m_config.maxGpuDropped)
		{
			m_failures.push_back(std::to_string(result.gpuDropped) + " GPU samples dropped (limit " + std::to_string(m_config.maxGpuDropped) +
				"); repeat with a larger benchmark.gpu_ring_frames");
		}
		BeginCaptures("prepare", m_config.measureFrames > 0 ? Phase::PRE_WARMUP : Phase::FINISH);
		break;
	}

	case Phase::CAPTURE:
		TickCaptures();
		break;

	case Phase::PRE_WARMUP:
	{
		if (m_config.benchmarkEnabled && m_config.fullscreenMainView && !m_fullscreenSet)
		{
			// Panels draw nothing in the fullscreen main view, so the Debug View costs no UI time (design 5.4).
			m_host.gui.SetMainViewFullscreen(true);
			m_fullscreenSet = true;
		}
		std::string error;
		if (!m_config.cameraPath.empty() && !StartPlayback(error))
		{
			FailUnit(error);
			break;
		}
		SetPhase(Phase::WARMUP, std::max<uint64_t>(m_config.warmupFrames, 1) + SettleFrames);
		break;
	}

	case Phase::WARMUP:
	{
		RestartPlaybackIfEnded();
		if (m_phaseFrame >= m_phaseFrames)
		{
			SetPhase(Phase::MEASURE_START);
		}
		break;
	}

	case Phase::MEASURE_START:
	{
		DebugView& view = DebugView::Get();
		if (m_config.benchmarkEnabled && (m_host.window.GetFrameTimeLimit() > 0.0f || m_host.window.IsVSyncEnabled()))
		{
			FailUnit("benchmark mode is on but the frame limiter or VSync is active");
			break;
		}
		if (!m_config.cameraPath.empty())
		{
			if (view.GetCameraPath().IsPlaying())
			{
				view.GetCameraPath().StopPlayback();
			}
			std::string error;
			if (!m_config.cameraLoop && m_pathFrames < m_config.measureFrames)
			{
				FailUnit("measure.frames (" + std::to_string(m_config.measureFrames) + ") is more than the " + std::to_string(m_pathFrames) +
					" frames of the camera path; shorten it or set camera.loop");
				break;
			}
			if (!StartPlayback(error))
			{
				FailUnit(error);
				break;
			}
		}
		if (!view.StartRecording(m_run, m_outputRoot, m_config.measureFrames))
		{
			FailUnit("the CSV window could not be started");
			break;
		}
		m_windowActive = true;
		m_stepStartFrame = view.GetFrameIndex();
		SetPhase(Phase::MEASURE, m_config.measureFrames);
		break;
	}

	case Phase::MEASURE:
		TickMeasure();
		break;

	case Phase::MEASURE_DRAIN:
	{
		if (DebugView::Get().GetRecorder().IsActive())
		{
			break;
		}
		const ExportResult& result = DebugView::Get().GetRecorder().GetLastResult();
		m_windowWritten = result.written;
		m_windowFrames = result.frames;
		m_windowGpuDropped = result.gpuDropped;
		m_windowMaxSleepMs = result.maxSleepMs;
		if (!result.written)
		{
			FailUnit("the CSV files were not written: " + result.error);
			break;
		}
		if (result.gpuDropped > m_config.maxGpuDropped)
		{
			m_failures.push_back(std::to_string(result.gpuDropped) + " GPU samples dropped (limit " + std::to_string(m_config.maxGpuDropped) +
				"); repeat with a larger benchmark.gpu_ring_frames");
		}
		if (m_config.benchmarkEnabled && result.maxSleepMs > 0.0)
		{
			m_failures.push_back("cpu_sleep_ms was above 0 in benchmark mode (largest " + FormatNumber(result.maxSleepMs) + " ms)");
		}
		BeginCaptures("measure", Phase::FINISH);
		break;
	}

	case Phase::FINISH:
		FinishUnit();
		break;

	case Phase::COOLDOWN:
		if (std::chrono::duration<double>(Clock::now() - m_cooldownStart).count() >= m_config.cooldownSeconds)
		{
			SetPhase(Phase::UNIT_BEGIN);
		}
		break;

	default:
		break;
	}
}

void pwg::debug::ExperimentRunner::BeginUnit()
{
	DebugView& view = DebugView::Get();
	const Unit& unit = m_units[m_unitIndex];
	const ExperimentVariant& variant = m_config.variants[unit.variant];

	// The renderer check runs once, before the first measurement (design 7.2, DEC-08).
	if (!m_firstUnitChecked)
	{
		m_firstUnitChecked = true;
		const std::string& renderer = view.GetRunMetadata().GetGlRenderer();
		if (!m_config.expectRenderer.empty() && renderer.find(m_config.expectRenderer) == std::string::npos)
		{
			m_run = RunInfo{};
			m_run.runId = m_stamp + "_r01";
			m_run.experimentId = m_config.experimentId;
			m_run.variant = variant.name;
			m_run.repetition = unit.repetition;
			m_unitFolder = m_outputRoot / m_run.runId;
			std::error_code ec;
			std::filesystem::create_directories(m_unitFolder, ec);
			m_unitStart = std::chrono::system_clock::now();
			m_failures.clear();
			m_warnings.clear();
			m_stepLog.clear();
			m_captureLog.clear();
			m_windowActive = false;
			m_windowWritten = false;
			m_failures.push_back("renderer check failed: expected '" + m_config.expectRenderer + "' in GL_RENDERER, got '" + renderer +
				"'; nothing was measured (DEC-08)");
			++m_unitsFailed;
			m_anyFailed = true;
			m_unitOpen = true;
			PWG_ERROR("Experiment: {}", m_failures.back());
			m_message = m_failures.back();
			CloseUnit(false);
			EndRun(State::DONE);
			return;
		}
	}

	m_unitStart = std::chrono::system_clock::now();
	m_unitStartSteady = Clock::now();
	m_failures.clear();
	m_warnings.clear();
	m_stepLog.clear();
	m_captureLog.clear();
	m_windowActive = false;
	m_windowWritten = false;
	m_windowGpuDropped = 0;
	m_windowMaxSleepMs = 0.0;
	m_windowFrames = 0;
	m_fullscreenSet = false;
	m_pathSha256.clear();
	m_pathFrames = 0;
	m_pathCopy.clear();

	char suffix[16];
	std::snprintf(suffix, sizeof(suffix), "_r%02d", unit.index);
	m_run = RunInfo{};
	m_run.runId = m_stamp + suffix;
	m_run.experimentId = m_config.experimentId;
	m_run.variant = variant.name;
	m_run.repetition = unit.repetition;
	m_run.manual = m_config.manual;

	// Merged parameters in application order: terrain, graph.file, graph nodes, the rest.
	std::vector<ExperimentParam> merged = m_config.baseParams;
	for (const ExperimentParam& param : variant.params)
	{
		auto existing = std::find_if(merged.begin(), merged.end(), [&](const ExperimentParam& other) { return other.first == param.first; });
		if (existing != merged.end())
		{
			existing->second = param.second;
		}
		else
		{
			merged.push_back(param);
		}
	}
	std::stable_sort(merged.begin(), merged.end(), [](const ExperimentParam& a, const ExperimentParam& b) { return ParamGroup(a.first) < ParamGroup(b.first); });
	for (const ExperimentParam& param : merged)
	{
		m_run.params.emplace_back(param.first, param.second.text);
	}

	m_unitFolder = m_outputRoot / m_run.runId;
	std::error_code ec;
	std::filesystem::create_directories(m_unitFolder, ec);
	m_unitOpen = !ec;
	if (ec)
	{
		FailUnit("cannot create " + PathText(m_unitFolder) + ": " + ec.message());
		return;
	}

	// The camera path used by this run is copied byte for byte into the run folder (design 7.1) and played from there.
	if (!m_config.cameraPath.empty())
	{
		std::filesystem::create_directories(m_unitFolder / CameraPath::FolderName, ec);
		m_pathCopy = m_unitFolder / CameraPath::FolderName / m_config.cameraPath.filename();
		std::filesystem::copy_file(m_config.cameraPath, m_pathCopy, std::filesystem::copy_options::none, ec);
		if (ec)
		{
			FailUnit("cannot copy the camera path to " + PathText(m_pathCopy) + ": " + ec.message());
			return;
		}
		std::string error;
		CameraPath& path = view.GetCameraPath();
		if (path.IsPlaying())
		{
			path.StopPlayback();
		}
		if (!path.Load(m_pathCopy, error))
		{
			FailUnit("the camera path is not valid: " + error);
			return;
		}
		m_pathSha256 = path.GetSha256();
		m_pathFrames = CameraPath::FrameCount(path.GetDuration(), m_config.cameraStep);
	}

	m_savedDebugEnabled = DebugView::IsEnabled();
	view.SetEnabled(m_config.debugView);
	Notice("unit " + std::to_string(unit.index) + " of " + std::to_string(m_units.size()) + ": variant '" + variant.name +
		"', repetition " + std::to_string(unit.repetition) + " -> " + PathText(m_unitFolder));
	SetPhase(Phase::LOAD);
}

bool pwg::debug::ExperimentRunner::ApplyParameters(std::string& error)
{
	EditorScene* editor = GetEditor();
	ChunkManager* chunks = GetChunks();
	if (!editor || !chunks)
	{
		error = "the editor scene is not ready";
		return false;
	}
	ResetRuntimeControls();

	// The same merged and ordered list as the run info.
	const ExperimentVariant& variant = m_config.variants[m_units[m_unitIndex].variant];
	std::vector<ExperimentParam> merged = m_config.baseParams;
	for (const ExperimentParam& param : variant.params)
	{
		auto existing = std::find_if(merged.begin(), merged.end(), [&](const ExperimentParam& other) { return other.first == param.first; });
		if (existing != merged.end())
		{
			existing->second = param.second;
		}
		else
		{
			merged.push_back(param);
		}
	}
	std::stable_sort(merged.begin(), merged.end(), [](const ExperimentParam& a, const ExperimentParam& b) { return ParamGroup(a.first) < ParamGroup(b.first); });

	// World size: both values are applied together, once.
	int count = chunks->GetChunkCount();
	int size = chunks->GetChunkSize();
	bool terrainSet = false;
	for (const ExperimentParam& param : merged)
	{
		if (param.first == "terrain.chunk_count")
		{
			count = static_cast<int>(param.second.number);
			terrainSet = true;
		}
		else if (param.first == "terrain.chunk_size")
		{
			size = static_cast<int>(param.second.number);
			terrainSet = true;
		}
	}
	if (terrainSet && (count != chunks->GetChunkCount() || size != chunks->GetChunkSize()))
	{
		chunks->ConfigureTerrain(count, size);
		chunks->GenerateWorld();	// as ProjectSerializer::Load does; the graph itself runs in prepare
		if (chunks->GetChunkCount() != count || chunks->GetChunkSize() != size)
		{
			error = "the world size " + std::to_string(count) + " x " + std::to_string(size) + " was not accepted";
			return false;
		}
	}

	for (const ExperimentParam& param : merged)
	{
		if (!ApplyParameter(param, error))
		{
			error = "parameter '" + param.first + "': " + error;
			return false;
		}
	}
	return true;
}

void pwg::debug::ExperimentRunner::ResetRuntimeControls()
{
	// Runtime controls are not part of a project, so each unit starts from the defaults (no state carries over).
	if (ChunkManager* chunks = GetChunks())
	{
		chunks->SetLodEnabled(true);
		chunks->SetLodScale(1.0f);
		chunks->ResetLodDistances();
		chunks->SetCullingEnabled(true);
		chunks->SetCullingFrozen(false);
	}
	if (EditorScene* editor = GetEditor())
	{
		editor->SetVegetationEnabled(true);
	}
}

bool pwg::debug::ExperimentRunner::ApplyParameter(const ExperimentParam& param, std::string& error)
{
	using Kind = ExperimentValue::Kind;
	const std::string& key = param.first;
	const ExperimentValue& value = param.second;
	EditorScene* editor = GetEditor();
	ChunkManager* chunks = GetChunks();

	if (key == "terrain.chunk_count" || key == "terrain.chunk_size")
	{
		return true;	// applied together before the loop
	}
	if (key == "graph.file")
	{
		const std::filesystem::path file = (m_config.file.parent_path() / PathFromText(value.text)).lexically_normal();
		TerrainGraphAsset loaded;
		try
		{
			if (!ProjectSerializer::LoadGraph(PathText(file), loaded))
			{
				error = "no graph in " + PathText(file);
				return false;
			}
		}
		catch (const std::exception& e)
		{
			error = "cannot read " + PathText(file) + ": " + e.what();
			return false;
		}
		chunks->GetTerrainComputePipeline()->GetUniversalGraphAsset() = std::move(loaded);
		editor->ReloadVegetationAssets();
		return true;
	}
	int nodeId = 0;
	std::string name;
	if (SplitGraphNodeKey(key, nodeId, name))
	{
		TerrainGraphAsset& asset = chunks->GetTerrainComputePipeline()->GetUniversalGraphAsset();
		TerrainGraphNodeInstance* node = asset.FindNode(nodeId);
		if (!node)
		{
			error = "the graph has no node " + std::to_string(nodeId);
			return false;
		}
		if (name == "enabled")
		{
			node->enabled = value.boolean;
			return true;
		}
		auto parameter = std::find_if(node->parameters.begin(), node->parameters.end(), [&](const TerrainGraphParameter& p) { return p.id == name; });
		if (parameter == node->parameters.end())
		{
			error = "node " + std::to_string(nodeId) + " (" + node->typeId + ") has no parameter '" + name + "'";
			return false;
		}
		switch (parameter->type)
		{
		case TerrainGraphParameterType::Float:
			if (value.kind != Kind::NUMBER)
			{
				error = "needs a number";
				return false;
			}
			parameter->floatValue = static_cast<float>(value.number);
			break;
		case TerrainGraphParameterType::Int:
			if (value.kind != Kind::NUMBER || !IsInteger(value.number))
			{
				error = "needs an integer";
				return false;
			}
			parameter->intValue = static_cast<int>(value.number);
			break;
		case TerrainGraphParameterType::Bool:
			if (value.kind != Kind::BOOL)
			{
				error = "needs true or false";
				return false;
			}
			parameter->boolValue = value.boolean;
			break;
		case TerrainGraphParameterType::String:
			parameter->stringValue = value.text;
			break;
		}
		return true;
	}
	if (key == "lod.enabled") { chunks->SetLodEnabled(value.boolean); return true; }
	if (key == "lod.scale")
	{
		if (!chunks->SetLodScale(static_cast<float>(value.number)))
		{
			error = "the scale was rejected";
			return false;
		}
		return true;
	}
	if (key == "lod.distances")
	{
		LODManager::Thresholds distances{};
		for (size_t i = 0; i < distances.size(); ++i)
		{
			distances[i] = static_cast<float>(value.list[i]);
		}
		if (!chunks->SetLodDistances(distances))
		{
			error = "the distances were rejected";
			return false;
		}
		return true;
	}
	if (key == "culling.enabled") { chunks->SetCullingEnabled(value.boolean); return true; }
	if (key == "vegetation.enabled") { editor->SetVegetationEnabled(value.boolean); return true; }
	if (key == "shadows.enabled")
	{
		editor->GetEnvironmentSystem()->GetSettings().shadowsEnabled = value.boolean;
		return true;
	}
	if (key == "water.visible")
	{
		chunks->GetTerrainComputePipeline()->GetHydraulicSettings().showWaterSurface = value.boolean;
		return true;
	}
	error = "unknown parameter";
	return false;
}

bool pwg::debug::ExperimentRunner::StartPlayback(std::string& error)
{
	CameraPath& path = DebugView::Get().GetCameraPath();
	CameraPathPlayback options;
	options.stepSeconds = m_config.cameraStep;
	// The matrix dump belongs to the measured playback only; it is written when the playback ends, outside the window.
	if (m_config.matrixDump && m_phase == Phase::MEASURE_START)
	{
		options.matrixDumpFile = m_unitFolder / CameraPath::FolderName / (m_pathCopy.stem().stem().string() + ".matrices.txt");
	}
	if (!path.StartPlayback(options, error))
	{
		error = "camera path playback: " + error;
		return false;
	}
	return true;
}

void pwg::debug::ExperimentRunner::RestartPlaybackIfEnded()
{
	if (m_config.cameraPath.empty())
	{
		return;
	}
	CameraPath& path = DebugView::Get().GetCameraPath();
	const bool atEnd = path.IsPlaying() && path.GetAppliedFrame() >= 0 &&
		static_cast<uint64_t>(path.GetAppliedFrame()) + 1 >= path.GetFrameCount();
	if (path.IsPlaying() && !atEnd)
	{
		return;
	}
	if (path.IsPlaying())
	{
		path.StopPlayback();
	}
	// The warm-up always loops the path; the measurement only with camera.loop.
	if (m_phase == Phase::WARMUP || (m_phase == Phase::MEASURE && m_config.cameraLoop))
	{
		std::string error;
		StartPlayback(error);
	}
}

void pwg::debug::ExperimentRunner::TickMeasure()
{
	DebugView& view = DebugView::Get();
	if (view.GetRecorder().GetState() == CsvRecorder::State::RECORDING)
	{
		RestartPlaybackIfEnded();
		m_phaseFrame = view.GetFrameIndex() - m_stepStartFrame;
		if (m_config.memorySnapshots)
		{
			// First and last frame of the window (the window covers frames start + 1 ... start + measure.frames).
			if (!m_memoryStartTaken)
			{
				m_memoryStartTaken = view.RecordMemorySnapshot("measure_start");
			}
			else if (!m_memoryEndTaken && m_phaseFrame >= m_config.measureFrames)
			{
				m_memoryEndTaken = view.RecordMemorySnapshot("measure_end");
			}
		}
		return;
	}
	// The window is over: it drains and writes at the start of the coming frames.
	if (view.GetCameraPath().IsPlaying())
	{
		view.GetCameraPath().StopPlayback();
	}
	SetPhase(Phase::MEASURE_DRAIN);
}

void pwg::debug::ExperimentRunner::EmitSteppingWindow(const char* kind, const ExperimentStep& step, int stepsDone, double wallMs, uint64_t frames)
{
	EventRow row;
	row.event = "stepping_window";
	row.detail = std::string("kind=") + kind + ";per_frame=" + std::to_string(step.perFrame) + ";frames=" + std::to_string(frames) +
		";glfinish=" + (m_config.wallclockBracket ? "true" : "false");
	row.metric = "steps";
	row.value = static_cast<double>(stepsDone);
	row.unit = "1";
	row.source = "runner";
	DebugView& view = DebugView::Get();
	view.AddRecordedEvent(row);
	row.metric = "wall_ms";
	row.value = wallMs;
	row.unit = "ms";
	row.source = "steady_clock";
	view.AddRecordedEvent(row);
}

pwg::debug::ExperimentRunner::StepResult pwg::debug::ExperimentRunner::TickStep(const ExperimentStep& step, std::string& error)
{
	ChunkManager* chunks = GetChunks();
	auto pipeline = chunks->GetTerrainComputePipeline();

	switch (step.type)
	{
	case ExperimentStep::Type::RUN_GRAPH:
		if (!chunks->RunUniversalGraph())
		{
			error = "RunUniversalGraph failed (graph not valid, or erosion is active)";
			return StepResult::FAILED;
		}
		m_stepLog.push_back({ step.Describe(), "ok" });
		return StepResult::DONE;

	case ExperimentStep::Type::REFRESH_ASSETS:
		if (!chunks->RefreshProceduralAssets())
		{
			error = "RefreshProceduralAssets failed: " + chunks->GetProceduralAssetStatus();
			return StepResult::FAILED;
		}
		m_stepLog.push_back({ step.Describe(), "ok" });
		return StepResult::DONE;

	case ExperimentStep::Type::HYDROLOGY:
	{
		if (!m_stepStarted)
		{
			if (!chunks->GenerateHydrology())
			{
				error = "GenerateHydrology did not start: " + chunks->GetHydrologyStatus();
				return StepResult::FAILED;
			}
			m_stepStarted = true;
			m_stepFrames = 0;
			m_stepStartTime = Clock::now();
			return StepResult::RUNNING;
		}
		++m_stepFrames;
		if (chunks->IsHydrologyRunning())
		{
			if (std::chrono::duration<double>(Clock::now() - m_stepStartTime).count() > HydrologyTimeoutSeconds)
			{
				error = "hydrology did not finish within " + FormatNumber(HydrologyTimeoutSeconds) + " s";
				return StepResult::FAILED;
			}
			return StepResult::RUNNING;
		}
		// The worker result is taken and uploaded by ChunkManager::Update; running is false once that happened.
		if (!pipeline->IsHydrologyGenerated())
		{
			error = "hydrology did not produce a result: " + chunks->GetHydrologyStatus();
			return StepResult::FAILED;
		}
		m_stepLog.push_back({ step.Describe(), "ok, " + std::to_string(m_stepFrames) + " frames" });
		return StepResult::DONE;
	}

	case ExperimentStep::Type::HYDRAULIC:
	case ExperimentStep::Type::THERMAL:
	{
		const bool hydraulic = step.type == ExperimentStep::Type::HYDRAULIC;
		if (!m_stepStarted)
		{
			if (hydraulic ? !chunks->IsParticleErosionReady() : !chunks->IsThermalErosionReady())
			{
				error = hydraulic ? "no enabled particle erosion node with amount above 0 in the graph (run_graph first)"
					: "no enabled thermal erosion node with amount above 0 in the graph (run_graph first)";
				return StepResult::FAILED;
			}
			if (chunks->IsParticleErosionActive() || chunks->IsThermalErosionActive())
			{
				error = "an erosion simulation is running; fixed-step mode needs it paused";
				return StepResult::FAILED;
			}
			m_stepStarted = true;
			m_stepDone = 0;
			m_stepFrames = 0;
			if (m_config.wallclockBracket)
			{
				glFinish();
			}
			m_stepStartTime = Clock::now();
		}
		++m_stepFrames;
		const int limit = step.untilStable ? step.maxSteps : step.steps;
		bool finished = false;
		for (int i = 0; i < step.perFrame && m_stepDone < limit && !finished; ++i)
		{
			const bool stepped = hydraulic ? chunks->StepParticleErosion() : chunks->StepThermalErosion();
			if (!stepped)
			{
				if (!hydraulic && step.untilStable && chunks->IsThermalErosionStable())
				{
					finished = true;
					break;
				}
				error = std::string(hydraulic ? "StepParticleErosion" : "StepThermalErosion") + " returned false after " + std::to_string(m_stepDone) + " steps";
				return StepResult::FAILED;
			}
			++m_stepDone;
			if (!hydraulic && step.untilStable && chunks->IsThermalErosionStable())
			{
				finished = true;
			}
		}
		if (!finished && m_stepDone >= limit)
		{
			if (step.untilStable)
			{
				error = "thermal erosion was not stable after " + std::to_string(step.maxSteps) + " steps";
				return StepResult::FAILED;
			}
			finished = true;
		}
		if (!finished)
		{
			return StepResult::RUNNING;
		}
		if (m_config.wallclockBracket)
		{
			glFinish();
		}
		const double wallMs = std::chrono::duration<double, std::milli>(Clock::now() - m_stepStartTime).count();
		EmitSteppingWindow(hydraulic ? "hydraulic" : "thermal", step, m_stepDone, wallMs, m_stepFrames);
		m_stepLog.push_back({ step.Describe(), std::to_string(m_stepDone) + " steps, " + FormatNumber(wallMs) + " ms wall, " + std::to_string(m_stepFrames) + " frames" });
		return StepResult::DONE;
	}

	case ExperimentStep::Type::SAVE:
	{
		const std::filesystem::path file = m_unitFolder / PathFromText(step.file);
		std::error_code ec;
		std::filesystem::create_directories(file.parent_path(), ec);
		if (!m_host.gui.SaveProjectCopy(PathText(file)))
		{
			error = "the project could not be saved to " + PathText(file);
			return StepResult::FAILED;
		}
		m_stepLog.push_back({ step.Describe(), "saved, sha256 " + ComputeFileSha256(file) });
		return StepResult::DONE;
	}

	case ExperimentStep::Type::RELOAD:
	{
		const std::filesystem::path file = step.file.empty() ? m_config.project : m_unitFolder / PathFromText(step.file);
		if (!m_stepStarted)
		{
			if (!m_host.gui.OpenProjectFromPath(PathText(file), false))
			{
				error = "the project could not be loaded from " + PathText(file);
				return StepResult::FAILED;
			}
			m_stepStarted = true;
			m_stepFrames = 0;
			return StepResult::RUNNING;
		}
		// A few frames let the scene update with the loaded state before the next entry runs.
		if (++m_stepFrames >= SettleFrames)
		{
			m_stepLog.push_back({ step.Describe(), "loaded, sha256 " + ComputeFileSha256(file) });
			return StepResult::DONE;
		}
		return StepResult::RUNNING;
	}

	case ExperimentStep::Type::WAIT:
		if (!m_stepStarted)
		{
			m_stepStarted = true;
			m_stepFrames = 0;
		}
		if (++m_stepFrames >= static_cast<uint64_t>(step.frames))
		{
			m_stepLog.push_back({ step.Describe(), "ok" });
			return StepResult::DONE;
		}
		return StepResult::RUNNING;
	}
	return StepResult::FAILED;
}

void pwg::debug::ExperimentRunner::TickPrepare()
{
	DebugView& view = DebugView::Get();

	if (m_stepIndex >= m_config.prepare.size())
	{
		// All steps ran. A recorded prepare window ends with this frame (its last events started in it).
		if (m_config.record == ExperimentConfig::Record::PREPARE && m_windowActive)
		{
			view.StopRecording();
			SetPhase(Phase::PREPARE_DRAIN);
			return;
		}
		BeginCaptures("prepare", m_config.measureFrames > 0 ? Phase::PRE_WARMUP : Phase::FINISH);
		return;
	}

	const ExperimentStep& step = m_config.prepare[m_stepIndex];
	const bool memoryWindow = m_config.memorySnapshots && m_config.record == ExperimentConfig::Record::PREPARE && m_windowActive;
	if (memoryWindow && !m_memoryStartTaken)
	{
		// Before the first entry: the baseline of the window.
		m_memoryStartTaken = view.RecordMemorySnapshot("prepare_start");
	}
	std::string error;
	const StepResult result = TickStep(step, error);
	if (result == StepResult::RUNNING)
	{
		return;
	}
	if (result == StepResult::FAILED)
	{
		FailUnit("prepare '" + step.label + "': " + error);
		return;
	}
	if (memoryWindow)
	{
		// The entry is done; fixed erosion steps and uploads may still be queued on the GPU, which the VRAM value does not wait for.
		view.RecordMemorySnapshot("after:" + step.label);
	}
	++m_stepIndex;
	m_stepStarted = false;
	m_stepFrames = 0;
	// Captures after this entry (design 9, E-13: one capture set per pipeline stage).
	const bool hasCaptures = std::any_of(m_config.captures.begin(), m_config.captures.end(),
		[&](const ExperimentCapture& capture) { return capture.after == step.label; });
	if (hasCaptures)
	{
		BeginCaptures(step.label, Phase::PREPARE_STEPS);
	}
}

void pwg::debug::ExperimentRunner::BeginCaptures(const std::string& after, Phase next)
{
	m_captureQueue.clear();
	for (const ExperimentCapture& capture : m_config.captures)
	{
		if (capture.after == after)
		{
			m_captureQueue.push_back(capture);
		}
	}
	m_captureIndex = 0;
	m_captureStage = 0;
	m_capturePlaying = false;
	m_phaseAfterCaptures = next;
	SetPhase(Phase::CAPTURE);
	if (m_captureQueue.empty())
	{
		SetPhase(next);
	}
}

void pwg::debug::ExperimentRunner::TickCaptures()
{
	DebugView& view = DebugView::Get();
	if (m_captureIndex >= m_captureQueue.size())
	{
		SetPhase(m_phaseAfterCaptures);
		return;
	}
	const ExperimentCapture& capture = m_captureQueue[m_captureIndex];
	ChunkManager* chunks = GetChunks();

	auto nextCapture = [&]()
	{
		++m_captureIndex;
		m_captureStage = 0;
		m_phaseFrame = 0;
	};

	if (m_captureStage == 0)
	{
		// Mask exports run at once (synchronous, between windows).
		if (!capture.masks.empty())
		{
			MaskExportRequest request;
			request.root = m_unitFolder;
			request.names = capture.masks;
			request.tag = capture.after == "prepare" || capture.after == "measure" ? std::string("after_") + capture.after : "after_" + capture.after;
			request.run = m_run;
			request.writePreview = capture.preview;
			const bool ok = view.ExportMasks(*chunks, request);
			for (const MaskExportResult& result : view.GetExporters().GetLastMaskExport())
			{
				CaptureRecord record;
				record.kind = "mask";
				record.after = capture.after;
				record.name = result.name;
				record.sha256 = result.sha256;
				if (result.written)
				{
					record.file = RelativeText(result.raw, m_unitFolder);
					record.detail = std::to_string(result.width) + "x" + std::to_string(result.height) + "x" + std::to_string(result.channels);
				}
				else
				{
					record.detail = "failed: " + result.error;
				}
				m_captureLog.push_back(std::move(record));
			}
			if (!ok)
			{
				FailUnit("mask export failed (after " + capture.after + ")");
				return;
			}
		}
		if (!capture.screenshot)
		{
			nextCapture();
			return;
		}

		ScreenshotRequest request;
		request.root = m_unitFolder;
		request.tag = capture.tag;
		request.run = m_run;
		if (capture.pathFrame < 0)
		{
			if (!view.RequestScreenshot(request))
			{
				FailUnit("the screenshot was refused");
				return;
			}
			m_captureStage = 2;
			m_phaseFrame = 0;
			return;
		}
		// A screenshot of a path frame: play the path and request the capture one frame before, so the next frame
		// applies the pose of the wanted path frame and captures it while the path is still playing.
		if (static_cast<uint64_t>(capture.pathFrame) >= m_pathFrames)
		{
			FailUnit("screenshot path_frame " + std::to_string(capture.pathFrame) + " is beyond the " + std::to_string(m_pathFrames) + " frames of the path");
			return;
		}
		std::string error;
		if (view.GetCameraPath().IsPlaying())
		{
			view.GetCameraPath().StopPlayback();
		}
		if (!StartPlayback(error))
		{
			FailUnit(error);
			return;
		}
		m_capturePlaying = true;
		if (capture.pathFrame == 0)
		{
			if (!view.RequestScreenshot(request))
			{
				FailUnit("the screenshot was refused");
				return;
			}
			m_captureStage = 2;
		}
		else
		{
			m_captureStage = 1;
		}
		m_phaseFrame = 0;
		return;
	}

	if (m_captureStage == 1)
	{
		CameraPath& path = view.GetCameraPath();
		if (!path.IsPlaying() || m_phaseFrame > m_pathFrames + 10)
		{
			FailUnit("the camera path ended before path frame " + std::to_string(capture.pathFrame));
			return;
		}
		if (path.GetAppliedFrame() + 1 == capture.pathFrame)
		{
			ScreenshotRequest request;
			request.root = m_unitFolder;
			request.tag = capture.tag;
			request.run = m_run;
			if (!view.RequestScreenshot(request))
			{
				FailUnit("the screenshot was refused");
				return;
			}
			m_captureStage = 2;
			m_phaseFrame = 0;
		}
		return;
	}

	if (m_captureStage == 2)
	{
		if (view.GetExporters().HasPendingScreenshot())
		{
			if (m_phaseFrame > ScreenshotTimeoutFrames)
			{
				view.CancelScreenshot();
				FailUnit("the screenshot was not taken (no scene frame was drawn)");
			}
			return;
		}
		if (m_capturePlaying)
		{
			view.GetCameraPath().StopPlayback();
			m_capturePlaying = false;
		}
		const ScreenshotResult& result = view.GetExporters().GetLastScreenshot();
		CaptureRecord record;
		record.kind = "screenshot";
		record.after = capture.after;
		record.name = capture.tag;
		record.sha256 = result.pixelsSha256;
		if (result.written)
		{
			record.file = RelativeText(result.png, m_unitFolder);
			record.detail = std::to_string(result.width) + "x" + std::to_string(result.height) +
				(capture.pathFrame >= 0 ? ", path_frame " + std::to_string(capture.pathFrame) : std::string());
		}
		else
		{
			record.detail = "failed: " + result.error;
		}
		m_captureLog.push_back(std::move(record));
		if (!result.written)
		{
			FailUnit("the screenshot failed: " + result.error);
			return;
		}
		nextCapture();
	}
}

void pwg::debug::ExperimentRunner::Cleanup()
{
	if (!DebugView::HasInstance())
	{
		return;
	}
	DebugView& view = DebugView::Get();
	if (view.GetRecorder().IsActive())
	{
		view.AbortRecording();
	}
	if (view.GetExporters().HasPendingScreenshot())
	{
		view.CancelScreenshot();
	}
	if (view.GetCameraPath().IsPlaying())
	{
		view.GetCameraPath().StopPlayback();
	}
	if (m_fullscreenSet)
	{
		m_host.gui.SetMainViewFullscreen(false);
		m_fullscreenSet = false;
	}
	if (view.IsBenchmarkMode())
	{
		view.SetBenchmarkMode(false);
	}
	view.SetWallclockBracket(false);
	view.SetEnabled(m_savedDebugEnabled);
	ResetRuntimeControls();
}

void pwg::debug::ExperimentRunner::FailUnit(const std::string& reason)
{
	PWG_ERROR("Experiment: unit {} failed: {}", m_units[m_unitIndex].index, reason);
	m_message = "unit failed: " + reason;
	m_failures.push_back(reason);
	++m_unitsFailed;
	m_anyFailed = true;
	CloseUnit(false);
	NextUnitOrFinish();
}

void pwg::debug::ExperimentRunner::FinishUnit()
{
	if (!m_failures.empty())
	{
		++m_unitsFailed;
		m_anyFailed = true;
		m_message = "unit finished with problems: " + m_failures.front();
	}
	CloseUnit(m_windowWritten);
	const Unit& unit = m_units[m_unitIndex];
	Notice("unit " + std::to_string(unit.index) + " of " + std::to_string(m_units.size()) + (m_failures.empty() ? " complete" : " finished with problems") +
		" (" + FormatNumber(std::chrono::duration<double>(Clock::now() - m_unitStartSteady).count()) + " s)");
	NextUnitOrFinish();
}

void pwg::debug::ExperimentRunner::NextUnitOrFinish()
{
	++m_unitIndex;
	if (m_unitIndex >= m_units.size())
	{
		EndRun(State::DONE);
		return;
	}
	if (m_config.cooldownSeconds > 0.0)
	{
		m_cooldownStart = Clock::now();
		SetPhase(Phase::COOLDOWN, 0);
	}
	else
	{
		SetPhase(Phase::UNIT_BEGIN);
	}
}

void pwg::debug::ExperimentRunner::EndRun(State state)
{
	Cleanup();
	m_state = state;
	SetPhase(Phase::FINISHED);
	Notice(std::string("run of ") + m_config.experimentId + (state == State::ABORTED ? " aborted" : " finished") + ", " +
		std::to_string(m_unitsFailed) + " of " + std::to_string(m_units.size()) + " units with problems");
	if (m_exitWhenDone)
	{
		m_wantsExit = true;
		m_host.gui.ForceClose();
	}
}

void pwg::debug::ExperimentRunner::EmitExperimentBlock(YAML::Emitter& out, const std::string& status) const
{
	const Unit& unit = m_units[std::min(m_unitIndex, m_units.size() - 1)];
	const ExperimentVariant& variant = m_config.variants[unit.variant];
	out << YAML::BeginMap << YAML::Key << "experiment" << YAML::Value << YAML::BeginMap;
	KeyString(out, "format", "pwg-experiment-run");
	KeyNumber(out, "format_version", 1);
	KeyString(out, "status", status);
	KeyStrings(out, "failures", m_failures);
	KeyStrings(out, "warnings", m_warnings);
	KeyNumber(out, "unit", unit.index);
	KeyNumber(out, "units_total", static_cast<double>(m_units.size()));

	// Everything that defines the run. The same config gives the same block for every repetition of a variant
	// (the acceptance rule of D-11: same inputs, new measurements).
	out << YAML::Key << "inputs" << YAML::Value << YAML::BeginMap;
	KeyString(out, "experiment_id", m_config.experimentId);
	KeyString(out, "description", m_config.description);
	KeyString(out, "config_file", PathText(m_config.file));
	KeyString(out, "config_sha256", m_config.fileSha256);
	KeyString(out, "project", PathText(m_config.project));
	KeyString(out, "expect_renderer", m_config.expectRenderer);
	KeyString(out, "variant", variant.name);
	out << YAML::Key << "params" << YAML::Value;
	if (m_run.params.empty())
	{
		out << YAML::Flow;
	}
	out << YAML::BeginMap;
	for (const auto& [name, value] : m_run.params)
	{
		KeyString(out, name.c_str(), value);
	}
	out << YAML::EndMap;
	KeyBool(out, "debug_view", m_config.debugView);
	if (m_config.memorySnapshots)
	{
		KeyBool(out, "memory_snapshots", true);		// Only when set, so the inputs of runs of earlier configs are unchanged.
	}
	out << YAML::Key << "benchmark" << YAML::Value << YAML::BeginMap;
	KeyBool(out, "enabled", m_config.benchmarkEnabled);
	KeyNumber(out, "render_width", m_config.renderWidth);
	KeyNumber(out, "render_height", m_config.renderHeight);
	KeyBool(out, "fullscreen_main_view", m_config.fullscreenMainView);
	KeyNumber(out, "gpu_ring_frames", m_config.gpuRingFrames);
	KeyBool(out, "wallclock_bracket", m_config.wallclockBracket);
	KeyNumber(out, "max_gpu_dropped", static_cast<double>(m_config.maxGpuDropped));
	out << YAML::EndMap;
	std::vector<std::string> steps;
	for (const ExperimentStep& step : m_config.prepare)
	{
		steps.push_back(step.label == StepTypeName(step.type) ? step.Describe() : step.label + ": " + step.Describe());
	}
	KeyStrings(out, "prepare", steps);
	out << YAML::Key << "camera" << YAML::Value << YAML::BeginMap;
	KeyString(out, "path", m_config.cameraPath.empty() ? "" : PathText(m_config.cameraPath.filename()));
	KeyString(out, "path_sha256", m_pathSha256);
	KeyNumber(out, "dt", m_config.cameraStep);
	KeyBool(out, "loop", m_config.cameraLoop);
	KeyBool(out, "matrix_dump", m_config.matrixDump);
	KeyNumber(out, "path_frames", static_cast<double>(m_pathFrames));
	out << YAML::EndMap;
	KeyNumber(out, "warmup_frames", static_cast<double>(m_config.warmupFrames));
	KeyNumber(out, "measure_frames", static_cast<double>(m_config.measureFrames));
	KeyString(out, "record", m_config.record == ExperimentConfig::Record::MEASURE ? "measure" :
		m_config.record == ExperimentConfig::Record::PREPARE ? "prepare" : "none");
	KeyNumber(out, "repetitions", m_config.repetitions);
	KeyString(out, "order", m_config.interleaved ? "interleaved" : "sequential");
	KeyNumber(out, "cooldown_s", m_config.cooldownSeconds);
	out << YAML::EndMap;

	// What this run produced. The hashes of masks and screenshots are deterministic, the timings are not.
	out << YAML::Key << "results" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "prepare" << YAML::Value;
	if (m_stepLog.empty())
	{
		out << YAML::Flow;
	}
	out << YAML::BeginSeq;
	for (const StepLog& log : m_stepLog)
	{
		out << YAML::BeginMap;
		KeyString(out, "step", log.step);
		KeyString(out, "result", log.result);
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::Key << "captures" << YAML::Value;
	if (m_captureLog.empty())
	{
		out << YAML::Flow;
	}
	out << YAML::BeginSeq;
	for (const CaptureRecord& capture : m_captureLog)
	{
		out << YAML::BeginMap;
		KeyString(out, "kind", capture.kind);
		KeyString(out, "after", capture.after);
		KeyString(out, "name", capture.name);
		KeyString(out, "file", capture.file);
		KeyString(out, "sha256", capture.sha256);
		KeyString(out, "detail", capture.detail);
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::Key << "window" << YAML::Value << YAML::BeginMap;
	KeyBool(out, "written", m_windowWritten);
	KeyNumber(out, "frames", static_cast<double>(m_windowFrames));
	KeyNumber(out, "gpu_dropped", static_cast<double>(m_windowGpuDropped));
	KeyNumber(out, "max_cpu_sleep_ms", m_windowMaxSleepMs);
	out << YAML::EndMap;
	KeyNumber(out, "unit_wall_s", std::chrono::duration<double>(Clock::now() - m_unitStartSteady).count());
	out << YAML::EndMap;
	out << YAML::EndMap << YAML::EndMap;
}

void pwg::debug::ExperimentRunner::CloseUnit(bool windowWritten)
{
	if (!m_unitOpen)
	{
		Cleanup();
		return;
	}
	// The metadata is read while the benchmark state of the unit is still set; then the editor is put back.
	const Metadata metadata = DebugView::Get().CollectRunMetadata(m_run, m_unitStart);
	Cleanup();
	WriteUnitReport(metadata, windowWritten);
	m_unitOpen = false;
}

void pwg::debug::ExperimentRunner::WriteUnitReport(const Metadata& metadata, bool windowWritten)
{
	const std::string status = m_failures.empty() ? "complete" : (m_abortRequested ? "aborted" : "failed");
	const std::filesystem::path runYaml = m_unitFolder / "run.yaml";
	std::error_code ec;
	std::filesystem::create_directories(m_unitFolder, ec);
	if (!windowWritten || !std::filesystem::exists(runYaml, ec))
	{
		// No CSV window ran (record: none, or the unit failed before it): run.yaml carries the metadata and the reason.
		Metadata result;
		result.Set("status", m_windowWritten ? "complete" : (m_config.record == ExperimentConfig::Record::NONE && m_failures.empty() ? "no_csv_window" : "failed"));
		std::string reasons;
		for (const std::string& failure : m_failures)
		{
			reasons += (reasons.empty() ? "" : "; ") + failure;
		}
		result.Set("warnings", reasons);
		WriteRunYaml(runYaml, metadata, result, {});
	}
	YAML::Emitter out;
	out.SetIndent(2);
	EmitExperimentBlock(out, status);
	AppendText(runYaml, out.c_str());
}
