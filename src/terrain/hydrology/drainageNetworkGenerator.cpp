#include "drainageNetworkGenerator.h"
#include "debug/debugView.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <queue>
#include <utility>

namespace pwg
{
	namespace
	{
		constexpr std::array<int, 8> DX{ 1, 1, 0, -1, -1, -1, 0, 1 };
		constexpr std::array<int, 8> DY{ 0, 1, 1, 1, 0, -1, -1, -1 };
		constexpr float Pi = 3.14159265358979323846f;

		struct QueueCell
		{
			float height;
			int index;
			bool operator>(const QueueCell& other) const
			{
				return height == other.height ? index > other.index : height > other.height;
			}
		};

		int ClampCoord(int value, int size) { return std::clamp(value, 0, size - 1); }
		int Index(int x, int y, int size) { return y * size + x; }

		std::vector<float> Resample(const std::vector<float>& source, int sourceSize, int targetSize)
		{
			if (sourceSize == targetSize) return source;
			std::vector<float> result(static_cast<size_t>(targetSize) * targetSize);
			for (int y = 0; y < targetSize; ++y)
			{
				float sy = static_cast<float>(y) * static_cast<float>(sourceSize - 1) / static_cast<float>(targetSize - 1);
				int y0 = static_cast<int>(std::floor(sy));
				int y1 = std::min(y0 + 1, sourceSize - 1);
				float fy = sy - static_cast<float>(y0);
				for (int x = 0; x < targetSize; ++x)
				{
					float sx = static_cast<float>(x) * static_cast<float>(sourceSize - 1) / static_cast<float>(targetSize - 1);
					int x0 = static_cast<int>(std::floor(sx));
					int x1 = std::min(x0 + 1, sourceSize - 1);
					float fx = sx - static_cast<float>(x0);
					float a = std::lerp(source[Index(x0, y0, sourceSize)], source[Index(x1, y0, sourceSize)], fx);
					float b = std::lerp(source[Index(x0, y1, sourceSize)], source[Index(x1, y1, sourceSize)], fx);
					result[Index(x, y, targetSize)] = std::lerp(a, b, fy);
				}
			}
			return result;
		}

		float Bilinear(const std::vector<float>& values, int size, float x, float y)
		{
			int x0 = ClampCoord(static_cast<int>(std::floor(x)), size);
			int y0 = ClampCoord(static_cast<int>(std::floor(y)), size);
			int x1 = std::min(x0 + 1, size - 1);
			int y1 = std::min(y0 + 1, size - 1);
			float fx = x - std::floor(x);
			float fy = y - std::floor(y);
			return std::lerp(std::lerp(values[Index(x0, y0, size)], values[Index(x1, y0, size)], fx),
				std::lerp(values[Index(x0, y1, size)], values[Index(x1, y1, size)], fx), fy);
		}

		/**
		* @brief Worker wall time per stage on steady_clock. Runs on the worker thread and writes only into the
		* result it is given, so it needs no synchronisation and makes no GL calls. Compiled out with the
		* Debug View instrumentation.
		*/
		class StageTimer
		{
		public:
			using Clock = std::chrono::steady_clock;

			explicit StageTimer(DrainageNetworkResult& result) : m_result(result)
			{
				if constexpr (debug::InstrumentationCompiled)
				{
					m_workerStart = Clock::now();
					m_stageStart = m_workerStart;
				}
			}

			void Resampled()
			{
				if constexpr (debug::InstrumentationCompiled)
				{
					m_result.resampleMs = Milliseconds(Clock::now() - m_workerStart);
				}
			}

			/** @brief Called just before the next stage starts, so `stage` has finished. */
			void Finished(int stage)
			{
				if constexpr (debug::InstrumentationCompiled)
				{
					const Clock::time_point now = Clock::now();
					if (stage >= 1 && stage <= static_cast<int>(DrainageNetworkResult::StageCount))
					{
						m_result.stageMs[static_cast<size_t>(stage - 1)] = Milliseconds(now - m_stageStart);
					}
					m_result.workerMs = Milliseconds(now - m_workerStart);
					m_stageStart = now;
				}
			}

		private:
			static double Milliseconds(Clock::duration duration)
			{
				return std::chrono::duration<double, std::milli>(duration).count();
			}

			DrainageNetworkResult& m_result;
			Clock::time_point m_workerStart{};
			Clock::time_point m_stageStart{};
		};
	}

	bool DrainageNetworkResult::IsValid() const
	{
		const size_t count = static_cast<size_t>(size) * size;
		return !cancelled && size > 1 && finalHeight.size() == count && water.size() == count &&
			flow.size() == count && rivers.size() == count && lakes.size() == count &&
			velocity.size() == count * 2 && carveDelta.size() == count;
	}

	DrainageNetworkGenerator::~DrainageNetworkGenerator()
	{
		Cancel();
		if (m_future.valid()) m_future.wait();
	}

	bool DrainageNetworkGenerator::Start(std::vector<float> height, int size, DrainageNetworkSettings settings)
	{
		if (m_running || size < 2 || height.size() < static_cast<size_t>(size) * size) return false;
		m_running = true;
		m_cancel = false;
		m_progress = 0.0f;
		m_stage = 1;
		m_future = std::async(std::launch::async, [this, height = std::move(height), size, settings]() mutable
		{
			return Generate(std::move(height), size, settings, m_cancel, m_progress, m_stage);
		});
		return true;
	}

	bool DrainageNetworkGenerator::TryTakeResult(DrainageNetworkResult& result)
	{
		if (!m_running || !m_future.valid() || m_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return false;
		result = m_future.get();
		m_running = false;
		m_stage = result.cancelled ? 0 : 7;
		m_progress = result.cancelled ? 0.0f : 1.0f;
		return true;
	}

	void DrainageNetworkGenerator::Cancel() { m_cancel = true; }
	bool DrainageNetworkGenerator::IsRunning() const { return m_running; }
	float DrainageNetworkGenerator::GetProgress() const { return m_progress; }

	std::string DrainageNetworkGenerator::GetStage() const
	{
		return GetStageName(m_stage.load());
	}

	const char* DrainageNetworkGenerator::GetStageName(int stage)
	{
		switch (stage)
		{
		case 1: return "Preparing heightmap";
		case 2: return "Finding lakes and outlets";
		case 3: return "Routing drainage";
		case 4: return "Accumulating catchments";
		case 5: return "Building rivers and lakes";
		case 6: return "Publishing full-resolution masks";
		case 7: return "Hydrology ready";
		default: return "Hydrology not generated";
		}
	}

	DrainageNetworkResult DrainageNetworkGenerator::Generate(std::vector<float> height, int size,
		DrainageNetworkSettings settings, std::atomic<bool>& cancel, std::atomic<float>& progress, std::atomic<int>& stage)
	{
		DrainageNetworkResult result;
		StageTimer timer(result);
		result.size = size;
		settings.riverAmount = std::clamp(settings.riverAmount, 0.0f, 1.0f);
		settings.lakeAmount = std::clamp(settings.lakeAmount, 0.0f, 1.0f);

		const int analysisSize = std::min(size, 2049);
		const size_t count = static_cast<size_t>(analysisSize) * analysisSize;
		std::vector<float> terrain = Resample(height, size, analysisSize);
		timer.Resampled();
		if (cancel) { result.cancelled = true; return result; }

		auto [minIt, maxIt] = std::minmax_element(terrain.begin(), terrain.end());
		const float rawHeightRange = *maxIt - *minIt;
		if (rawHeightRange < 0.00001f)
		{
			const size_t fullCount = static_cast<size_t>(size) * size;
			result.baseHeight = height;
			result.finalHeight = std::move(height);
			result.water.assign(fullCount, 0.0f);
			result.flow.assign(fullCount, 0.0f);
			result.rivers.assign(fullCount, 0.0f);
			result.lakes.assign(fullCount, 0.0f);
			result.velocity.assign(fullCount * 2, 0.0f);
			result.carveDelta.assign(fullCount, 0.0f);
			progress = 1.0f;
			timer.Finished(1);
			stage = 7;
			return result;
		}
		const float heightRange = std::max(rawHeightRange, 0.001f);
		const float epsilon = heightRange * 1.0e-7f;

		timer.Finished(1);
		stage = 2;
		progress = 0.10f;
		std::vector<float> filled = terrain;
		std::vector<uint8_t> visited(count, 0);
		std::priority_queue<QueueCell, std::vector<QueueCell>, std::greater<QueueCell>> queue;
		auto seedBoundary = [&](int x, int y)
		{
			int i = Index(x, y, analysisSize);
			if (visited[i]) return;
			visited[i] = 1;
			queue.push({ filled[i], i });
		};
		for (int i = 0; i < analysisSize; ++i)
		{
			seedBoundary(i, 0); seedBoundary(i, analysisSize - 1);
			seedBoundary(0, i); seedBoundary(analysisSize - 1, i);
		}
		uint32_t order = 0;
		while (!queue.empty())
		{
			QueueCell current = queue.top(); queue.pop();
			++order;
			int cx = current.index % analysisSize;
			int cy = current.index / analysisSize;
			for (int d = 0; d < 8; ++d)
			{
				int nx = cx + DX[d], ny = cy + DY[d];
				if (nx < 0 || ny < 0 || nx >= analysisSize || ny >= analysisSize) continue;
				int ni = Index(nx, ny, analysisSize);
				if (visited[ni]) continue;
				visited[ni] = 1;
				filled[ni] = std::max(terrain[ni], current.height);
				queue.push({ filled[ni], ni });
			}
			if ((order & 65535u) == 0u)
			{
				progress = 0.10f + 0.22f * static_cast<float>(order) / static_cast<float>(count);
				if (cancel) { result.cancelled = true; return result; }
			}
		}

		timer.Finished(2);
		stage = 3;
		progress = 0.34f;
		std::vector<std::array<int, 2>> receivers(count, { -1, -1 });
		std::vector<std::array<float, 2>> weights(count, { 0.0f, 0.0f });
		for (int y = 1; y < analysisSize - 1; ++y)
		{
			for (int x = 1; x < analysisSize - 1; ++x)
			{
				int i = Index(x, y, analysisSize);
				float gx = (terrain[Index(x + 1, y, analysisSize)] - terrain[Index(x - 1, y, analysisSize)]) * 0.5f;
				float gy = (terrain[Index(x, y + 1, analysisSize)] - terrain[Index(x, y - 1, analysisSize)]) * 0.5f;
				float gradientLength = std::sqrt(gx * gx + gy * gy);
				float totalWeight = 0.0f;
				if (gradientLength > epsilon)
				{
					float angle = std::atan2(-gy, -gx);
					if (angle < 0.0f) angle += 2.0f * Pi;
					float sector = angle / (Pi * 0.25f);
					int d0 = static_cast<int>(std::floor(sector)) & 7;
					int d1 = (d0 + 1) & 7;
					float angularWeight = sector - std::floor(sector);
					std::array<int, 2> dirs{ d0, d1 };
					std::array<float, 2> candidates{ 1.0f - angularWeight, angularWeight };
					for (int k = 0; k < 2; ++k)
					{
						int ni = Index(x + DX[dirs[k]], y + DY[dirs[k]], analysisSize);
						if (terrain[ni] < terrain[i] - epsilon)
						{
							receivers[i][k] = ni;
							weights[i][k] = candidates[k];
							totalWeight += candidates[k];
						}
					}
				}
				if (totalWeight <= 0.00001f)
				{
					float bestSlope = 0.0f;
					int bestReceiver = -1;
					for (int d = 0; d < 8; ++d)
					{
						int ni = Index(x + DX[d], y + DY[d], analysisSize);
						float drop = terrain[i] - terrain[ni];
						float distance = (DX[d] != 0 && DY[d] != 0) ? 1.41421356f : 1.0f;
						float slope = drop / distance;
						if (drop > epsilon && slope > bestSlope)
						{
							bestSlope = slope;
							bestReceiver = ni;
						}
					}
					if (bestReceiver >= 0)
					{
						receivers[i][0] = bestReceiver;
						weights[i][0] = 1.0f;
						totalWeight = 1.0f;
					}
				}
				if (totalWeight > 0.0f)
				{
					weights[i][0] /= totalWeight; weights[i][1] /= totalWeight;
				}
			}
			if ((y & 127) == 0 && cancel) { result.cancelled = true; return result; }
		}

		timer.Finished(3);
		stage = 4;
		progress = 0.48f;
		std::vector<int> sorted(count);
		std::iota(sorted.begin(), sorted.end(), 0);
		std::sort(sorted.begin(), sorted.end(), [&](int a, int b)
		{
			if (terrain[a] != terrain[b]) return terrain[a] > terrain[b];
			return a > b;
		});
		std::vector<float> accumulation(count, 1.0f);
		for (size_t n = 0; n < sorted.size(); ++n)
		{
			int i = sorted[n];
			for (int k = 0; k < 2; ++k)
				if (receivers[i][k] >= 0) accumulation[receivers[i][k]] += accumulation[i] * weights[i][k];
			if ((n & 65535u) == 0u && cancel) { result.cancelled = true; return result; }
		}

		timer.Finished(4);
		stage = 5;
		progress = 0.62f;
		const bool riversEnabled = settings.riverAmount > 0.001f;
		const bool lakesEnabled = settings.lakeAmount > 0.001f;
		const float riverThreshold = static_cast<float>(count) * std::lerp(0.020f, 0.00018f, settings.riverAmount);
		const float lakeAreaThreshold = static_cast<float>(count) * std::lerp(0.0025f, 0.00004f, settings.lakeAmount);
		const float lakeDepthThreshold = heightRange * std::lerp(0.018f, 0.0008f, settings.lakeAmount);
		const size_t lakeCellBudget = static_cast<size_t>(static_cast<double>(count) *
			(0.15 * std::pow(static_cast<double>(settings.lakeAmount), 1.5)));
		std::vector<float> lowFlow(count, 0.0f), lowRiver(count, 0.0f), lowLake(count, 0.0f);
		std::vector<float> lowWater(count, 0.0f), lowLakeSurface(count, 0.0f), lowCarve(count, 0.0f), lowVx(count, 0.0f), lowVy(count, 0.0f);
		std::vector<uint8_t> lakeCandidate(count, 0), componentVisited(count, 0);
		if (lakesEnabled)
			for (size_t i = 0; i < count; ++i) lakeCandidate[i] = filled[i] - terrain[i] > lakeDepthThreshold ? 1 : 0;
		struct LakeBasin
		{
			std::vector<int> cells;
			float spillSurface{ 0.0f };
			float minimumTerrain{ 0.0f };
			float score{ 0.0f };
		};
		std::vector<LakeBasin> basins;
		for (int start = 0; start < static_cast<int>(count); ++start)
		{
			if (!lakeCandidate[start] || componentVisited[start]) continue;
			LakeBasin basin;
			basin.minimumTerrain = terrain[start];
			basin.spillSurface = filled[start];
			std::queue<int> pending;
			pending.push(start); componentVisited[start] = 1;
			while (!pending.empty())
			{
				int i = pending.front(); pending.pop(); basin.cells.push_back(i);
				basin.minimumTerrain = std::min(basin.minimumTerrain, terrain[i]);
				basin.spillSurface = std::max(basin.spillSurface, filled[i]);
				int x = i % analysisSize, y = i / analysisSize;
				for (int d = 0; d < 8; ++d)
				{
					int nx = x + DX[d], ny = y + DY[d];
					if (nx < 0 || ny < 0 || nx >= analysisSize || ny >= analysisSize) continue;
					int ni = Index(nx, ny, analysisSize);
					if (lakeCandidate[ni] && !componentVisited[ni]) { componentVisited[ni] = 1; pending.push(ni); }
				}
			}
			if (static_cast<float>(basin.cells.size()) >= lakeAreaThreshold)
			{
				float depth = std::max(basin.spillSurface - basin.minimumTerrain, 0.0f);
				basin.score = depth * std::sqrt(static_cast<float>(basin.cells.size()));
				basins.push_back(std::move(basin));
			}
		}
		std::sort(basins.begin(), basins.end(), [](const LakeBasin& a, const LakeBasin& b)
		{
			return a.score > b.score;
		});
		size_t retainedLakeCells = 0;
		for (auto& basin : basins)
		{
			if (retainedLakeCells >= lakeCellBudget) break;
			float levelFraction = 0.08f + 0.92f * std::pow(settings.lakeAmount, 0.85f);
			float surface = std::lerp(basin.minimumTerrain, basin.spillSurface, levelFraction);
			std::vector<int> submerged;
			submerged.reserve(basin.cells.size());
			for (int i : basin.cells)
				if (terrain[i] < surface - lakeDepthThreshold * 0.25f) submerged.push_back(i);

			size_t remainingBudget = lakeCellBudget - retainedLakeCells;
			if (submerged.size() > remainingBudget)
			{
				std::nth_element(submerged.begin(), submerged.begin() + static_cast<std::ptrdiff_t>(remainingBudget), submerged.end(),
					[&](int a, int b) { return terrain[a] < terrain[b]; });
				submerged.resize(remainingBudget);
				if (!submerged.empty())
				{
					float highestRetained = terrain[*std::max_element(submerged.begin(), submerged.end(),
						[&](int a, int b) { return terrain[a] < terrain[b]; })];
					surface = std::min(surface, highestRetained + lakeDepthThreshold * 0.25f);
				}
			}
			if (static_cast<float>(submerged.size()) < lakeAreaThreshold) continue;
			for (int i : submerged)
			{
				float depth = std::max(surface - terrain[i], 0.0f);
				if (depth <= 0.0f) continue;
				lowLake[i] = 1.0f;
				lowLakeSurface[i] = surface;
				lowWater[i] = depth;
				++retainedLakeCells;
			}
		}
		const float logDenominator = std::log1p(static_cast<float>(count));
		const float normalizedRiverThreshold = std::log1p(riverThreshold) / logDenominator;
		for (int y = 0; y < analysisSize; ++y)
		{
			for (int x = 0; x < analysisSize; ++x)
			{
				int i = Index(x, y, analysisSize);
				float normalizedFlow = std::clamp(std::log1p(accumulation[i]) / logDenominator, 0.0f, 1.0f);
				lowFlow[i] = normalizedFlow;
				if (!riversEnabled || normalizedFlow < normalizedRiverThreshold || lowLake[i] > 0.5f || receivers[i][0] < 0) continue;
				// A rejected Priority-Flood depression has no physically valid open-water
				// outlet. Drawing its flat routing path would create contour-following rivers.
				if (filled[i] - terrain[i] > lakeDepthThreshold) continue;
				float strength = std::clamp((normalizedFlow - normalizedRiverThreshold) /
					std::max(1.0f - normalizedRiverThreshold, 0.001f), 0.0f, 1.0f);
				float vx = 0.0f, vy = 0.0f;
				for (int k = 0; k < 2; ++k) if (receivers[i][k] >= 0)
				{
					int rx = receivers[i][k] % analysisSize, ry = receivers[i][k] / analysisSize;
					vx += static_cast<float>(rx - x) * weights[i][k];
					vy += static_cast<float>(ry - y) * weights[i][k];
				}
				float velocityLength = std::sqrt(vx * vx + vy * vy);
				if (velocityLength > 0.0001f) { vx = vx / velocityLength * strength; vy = vy / velocityLength * strength; }

				float depth = heightRange * (0.00035f + 0.0018f * std::pow(strength, 0.55f));
				lowRiver[i] = strength;
				lowVx[i] = vx;
				lowVy[i] = vy;
				lowCarve[i] = depth * strength;
				lowWater[i] = std::max(lowWater[i], depth * strength * 0.68f);
			}
			if ((y & 127) == 0 && cancel) { result.cancelled = true; return result; }
		}
		for (int y = 1; y < analysisSize - 1; ++y)
		{
			for (int x = 1; x < analysisSize - 1; ++x)
			{
				int i = Index(x, y, analysisSize);
				if (lowRiver[i] <= 0.0f) continue;
				float gx = (terrain[Index(x + 1, y, analysisSize)] - terrain[Index(x - 1, y, analysisSize)]) * 0.5f;
				float gy = (terrain[Index(x, y + 1, analysisSize)] - terrain[Index(x, y - 1, analysisSize)]) * 0.5f;
				if (lowVx[i] * gx + lowVy[i] * gy <= 0.0f) continue;
				float gradientLength = std::sqrt(gx * gx + gy * gy);
				float speed = std::sqrt(lowVx[i] * lowVx[i] + lowVy[i] * lowVy[i]);
				if (gradientLength > epsilon && speed > 0.0f)
				{
					lowVx[i] = -gx / gradientLength * speed;
					lowVy[i] = -gy / gradientLength * speed;
				}
				else
				{
					lowVx[i] = 0.0f;
					lowVy[i] = 0.0f;
				}
			}
		}

		timer.Finished(5);
		stage = 6;
		progress = 0.80f;
		const size_t fullCount = static_cast<size_t>(size) * size;
		result.finalHeight.resize(fullCount);
		result.water.resize(fullCount);
		result.flow.resize(fullCount);
		result.rivers.resize(fullCount);
		result.lakes.resize(fullCount);
		result.velocity.resize(fullCount * 2);
		result.carveDelta.resize(fullCount);
		for (int y = 0; y < size; ++y)
		{
			float ay = static_cast<float>(y) * static_cast<float>(analysisSize - 1) / static_cast<float>(size - 1);
			for (int x = 0; x < size; ++x)
			{
				float ax = static_cast<float>(x) * static_cast<float>(analysisSize - 1) / static_cast<float>(size - 1);
				size_t i = static_cast<size_t>(Index(x, y, size));
				float river = Bilinear(lowRiver, analysisSize, ax, ay);
				float lakeCoverage = Bilinear(lowLake, analysisSize, ax, ay);
				float lake = lakeCoverage > 0.5f ? 1.0f : 0.0f;
				float carve = Bilinear(lowCarve, analysisSize, ax, ay) * (1.0f - lake);
				result.carveDelta[i] = carve;
				result.finalHeight[i] = height[i] - carve;
				result.rivers[i] = river * (1.0f - lake);
				result.lakes[i] = lake;
				result.flow[i] = Bilinear(lowFlow, analysisSize, ax, ay);
				float water = Bilinear(lowWater, analysisSize, ax, ay);
				if (lake > 0.5f)
				{
					float lakeSurface = Bilinear(lowLakeSurface, analysisSize, ax, ay) / std::max(lakeCoverage, 0.0001f);
					water = std::max(lakeSurface - result.finalHeight[i], 0.0f);
				}
				result.water[i] = water;
				result.velocity[i * 2] = Bilinear(lowVx, analysisSize, ax, ay);
				result.velocity[i * 2 + 1] = Bilinear(lowVy, analysisSize, ax, ay);
			}
			if ((y & 31) == 0)
			{
				progress = 0.80f + 0.19f * static_cast<float>(y) / static_cast<float>(size);
				if (cancel) { result.cancelled = true; return result; }
			}
		}
		progress = 1.0f;
		timer.Finished(6);
		stage = 7;
		result.baseHeight = std::move(height);
		return result;
	}
}
