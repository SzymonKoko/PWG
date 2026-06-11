#include "particleErosionPass.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <queue>

namespace pwg
{
	namespace
	{
		float SmoothStep(float edge0, float edge1, float value)
		{
			if (edge0 == edge1)
			{
				return value < edge0 ? 0.0f : 1.0f;
			}

			float t = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
			return t * t * (3.0f - 2.0f * t);
		}
	}

	ParticleErosionPass::ParticleErosionPass(int terrainSize)
		: m_terrainSize(terrainSize)
	{
		m_inputTextures = { "FinalHeight", "BaseHeight" };
		m_outputTextures = { "FinalHeight", "ErodedHeight", "DepositMask", "WaterMask", "SedimentMask", "FlowAccumulationMask", "RiverChannelMask", "ErosionDelta" };
	}

	void ParticleErosionPass::SetHydraulicSettings(HydraulicSimulationSettings* settings)
	{
		m_parameters.settings = settings;
	}

	void ParticleErosionPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		if (!m_parameters.settings)
		{
			m_parameters.settings = &ctx.hydraulicSettings;
		}
		if (!ctx.hydraulicSettings.particleErosionEnabled)
		{
			return;
		}

		EnsureCpuCache(masks, ctx);
		std::fill(ctx.cpuErosionDelta.begin(), ctx.cpuErosionDelta.end(), 0.0f);
		std::fill(ctx.cpuWater.begin(), ctx.cpuWater.end(), 0.0f);

		int particleCount = std::max(ctx.hydraulicSettings.particlesPerStep, 0);
		size_t count = static_cast<size_t>(m_terrainSize) * static_cast<size_t>(m_terrainSize);
		std::vector<float> heightSnapshot = ctx.cpuHeight;
		std::vector<float> depositSnapshot = ctx.cpuDeposit;
		std::vector<float> deltaHeight(count, 0.0f);
		std::vector<float> deltaDeposit(count, 0.0f);
		std::vector<float> deltaFlow(count, 0.0f);
		std::vector<float> deltaRiver(count, 0.0f);
		std::vector<float> deltaErosion(count, 0.0f);

#pragma omp parallel for schedule(static)
		for (int i = 0; i < particleCount; ++i)
		{
			std::mt19937 rng(static_cast<uint32_t>(ctx.hydraulicSettings.particleSeed + m_stepCounter * 9176 + i * 9781));
			SimulateDroplet(ctx, rng, heightSnapshot, depositSnapshot, deltaHeight, deltaDeposit, deltaFlow, deltaRiver, deltaErosion);
		}

#pragma omp parallel for schedule(static)
		for (int i = 0; i < static_cast<int>(count); ++i)
		{
			ctx.cpuHeight[i] += deltaHeight[i];
			ctx.cpuDeposit[i] = std::max(0.0f, ctx.cpuDeposit[i] + deltaDeposit[i]);
			ctx.cpuFlowAccumulation[i] = std::min(1.0f, ctx.cpuFlowAccumulation[i] + deltaFlow[i]);
			ctx.cpuRiverChannels[i] = std::min(1.0f, ctx.cpuRiverChannels[i] + deltaRiver[i]);
			ctx.cpuErosionDelta[i] = deltaErosion[i];
		}

		if (ctx.hydraulicSettings.looseSedimentSmoothing > 0.0f)
		{
			SmoothLooseSediment(ctx);
		}

		if (ctx.deferParticleTextureUpload)
		{
			m_stepCounter++;
			return;
		}

		if (ctx.hydraulicSettings.lakeFillEnabled > 0.5f)
		{
			FillLakes(ctx);
		}

		UploadCpuCache(masks, ctx);
		m_stepCounter++;
	}

	std::vector<UIParameters> ParticleErosionPass::GetParameters()
	{
		if (!m_parameters.settings)
		{
			return {};
		}

		return {
			{"Enable Particle Erosion", ParameterType::Bool, &m_parameters.settings->particleErosionEnabled, 0, 1},
			{"Run - steps per second", ParameterType::Int, &m_parameters.settings->waterStepsPerSecond, 1, 120},
			{"Run - substeps", ParameterType::Int, &m_parameters.settings->waterSubsteps, 1, 8},
			{"Step - passes per click", ParameterType::Int, &m_parameters.settings->stepsPerUpdate, 1, 16},
			{"Bake - steps", ParameterType::Int, &m_parameters.settings->bakeParticleSteps, 1, 200},
			{"Bake - steps per frame", ParameterType::Int, &m_parameters.settings->bakeStepsPerFrame, 1, 8},
			{"Bake - preview interval", ParameterType::Int, &m_parameters.settings->bakeUploadInterval, 1, 32},
			{"Particles per step", ParameterType::Int, &m_parameters.settings->particlesPerStep, 1000, 120000},
			{"Droplet lifetime", ParameterType::Int, &m_parameters.settings->dropletLifetime, 8, 160},
			{"Particle seed", ParameterType::Int, &m_parameters.settings->particleSeed, 1, 100000},
			{"Particle inertia", ParameterType::Float, &m_parameters.settings->particleInertia, 0.0f, 0.95f},
			{"Particle gravity", ParameterType::Float, &m_parameters.settings->particleGravity, 0.1f, 20.0f},
			{"Particle capacity", ParameterType::Float, &m_parameters.settings->particleCapacity, 0.1f, 20.0f},
			{"Particle erosion strength", ParameterType::Float, &m_parameters.settings->particleErosionStrength, 0.0f, 0.3f},
			{"Particle deposition strength", ParameterType::Float, &m_parameters.settings->particleDepositionStrength, 0.0f, 0.5f},
			{"Particle brush radius", ParameterType::Float, &m_parameters.settings->particleBrushRadius, 0.5f, 5.0f},
			{"Particle deposit radius", ParameterType::Float, &m_parameters.settings->particleDepositRadius, 0.5f, 12.0f},
			{"Deposit slope start", ParameterType::Float, &m_parameters.settings->particleDepositSlopeStart, 0.0f, 0.08f},
			{"Deposit slope end", ParameterType::Float, &m_parameters.settings->particleDepositSlopeEnd, 0.02f, 0.25f},
			{"Deposit valley boost", ParameterType::Float, &m_parameters.settings->particleDepositValleyBoost, 0.0f, 5.0f},
			{"Deposit shelf boost", ParameterType::Float, &m_parameters.settings->particleDepositShelfBoost, 0.0f, 2.0f},
			{"Deposit min suitability", ParameterType::Float, &m_parameters.settings->particleDepositMinSuitability, 0.0f, 0.5f},
			{"Loose sediment erosion", ParameterType::Float, &m_parameters.settings->looseSedimentErosionStrength, 0.0f, 2.0f},
			{"Loose sediment smoothing", ParameterType::Float, &m_parameters.settings->looseSedimentSmoothing, 0.0f, 0.5f},
			{"Particle channel strength", ParameterType::Float, &m_parameters.settings->particleChannelStrength, 0.0f, 0.2f},
			{"Particle flow strength", ParameterType::Float, &m_parameters.settings->particleFlowStrength, 0.0f, 0.1f},
			{"Lake fill", ParameterType::Float, &m_parameters.settings->lakeFillEnabled, 0.0f, 1.0f},
			{"Lake min flow", ParameterType::Float, &m_parameters.settings->lakeMinFlow, 0.0f, 1.0f},
			{"Lake max depth", ParameterType::Float, &m_parameters.settings->lakeMaxDepth, 0.05f, 10.0f}
		};
	}

	TerrainPassTypes ParticleErosionPass::GetPassType()
	{
		return m_passType;
	}

	std::string ParticleErosionPass::GetName()
	{
		return m_name;
	}

	void ParticleErosionPass::EnsureCpuCache(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		size_t count = static_cast<size_t>(m_terrainSize) * static_cast<size_t>(m_terrainSize);
		if (ctx.cpuTerrainCacheValid && ctx.cpuHeight.size() == count)
		{
			return;
		}

		ctx.cpuHeight = masks.at("FinalHeight")->texture->ReadFloatData();
		ctx.cpuBaseHeight = masks.at("BaseHeight")->texture->ReadFloatData();
		ctx.cpuDeposit.assign(count, 0.0f);
		ctx.cpuWater.assign(count, 0.0f);
		ctx.cpuSediment.assign(count, 0.0f);
		ctx.cpuFlowAccumulation.assign(count, 0.0f);
		ctx.cpuRiverChannels.assign(count, 0.0f);
		ctx.cpuErosionDelta.assign(count, 0.0f);

		if (ctx.cpuHeight.size() < count)
		{
			ctx.cpuHeight.assign(count, 0.0f);
		}
		if (ctx.cpuBaseHeight.size() < count)
		{
			ctx.cpuBaseHeight = ctx.cpuHeight;
		}

		ctx.cpuTerrainCacheValid = true;
	}

	void ParticleErosionPass::UploadCpuCache(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		masks.at("FinalHeight")->texture->UpdateFloatData(ctx.cpuHeight, m_terrainSize, m_terrainSize);
		masks.at("ErodedHeight")->texture->UpdateFloatData(ctx.cpuHeight, m_terrainSize, m_terrainSize);
		masks.at("DepositMask")->texture->UpdateFloatData(ctx.cpuDeposit, m_terrainSize, m_terrainSize);
		masks.at("WaterMask")->texture->UpdateFloatData(ctx.cpuWater, m_terrainSize, m_terrainSize);
		masks.at("SedimentMask")->texture->UpdateFloatData(ctx.cpuSediment, m_terrainSize, m_terrainSize);
		masks.at("FlowAccumulationMask")->texture->UpdateFloatData(ctx.cpuFlowAccumulation, m_terrainSize, m_terrainSize);
		masks.at("RiverChannelMask")->texture->UpdateFloatData(ctx.cpuRiverChannels, m_terrainSize, m_terrainSize);
		masks.at("ErosionDelta")->texture->UpdateFloatData(ctx.cpuErosionDelta, m_terrainSize, m_terrainSize);
	}

	void ParticleErosionPass::SimulateDroplet(TerrainPassContext& ctx, std::mt19937& rng, const std::vector<float>& heightSnapshot, const std::vector<float>& depositSnapshot, std::vector<float>& deltaHeight, std::vector<float>& deltaDeposit, std::vector<float>& deltaFlow, std::vector<float>& deltaRiver, std::vector<float>& deltaErosion)
	{
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		float x = unit(rng) * static_cast<float>(m_terrainSize - 1);
		float y = unit(rng) * static_cast<float>(m_terrainSize - 1);

		for (int tries = 0; tries < 8; ++tries)
		{
			float candidateX = unit(rng) * static_cast<float>(m_terrainSize - 1);
			float candidateY = unit(rng) * static_cast<float>(m_terrainSize - 1);
			HeightSample sample = SampleHeight(heightSnapshot, candidateX, candidateY);
			float ridge = glm::length(sample.gradient);
			if (ridge > ctx.hydraulicSettings.particleMinSlope || unit(rng) < 0.25f)
			{
				x = candidateX;
				y = candidateY;
				break;
			}
		}

		glm::vec2 direction{ 0.0f };
		float speed = 1.0f;
		float water = ctx.hydraulicSettings.particleInitialWater;
		float sediment = 0.0f;

		for (int lifetime = 0; lifetime < ctx.hydraulicSettings.dropletLifetime; ++lifetime)
		{
			HeightSample sample = SampleHeight(heightSnapshot, x, y);
			glm::vec2 downhill = -sample.gradient;
			if (glm::dot(downhill, downhill) < 0.0000001f)
			{
				DepositAt(ctx, heightSnapshot, x, y, sediment, deltaHeight, deltaDeposit, deltaErosion);
				break;
			}

			downhill = glm::normalize(downhill);
			direction = direction * ctx.hydraulicSettings.particleInertia + downhill * (1.0f - ctx.hydraulicSettings.particleInertia);
			if (glm::dot(direction, direction) < 0.0000001f)
			{
				break;
			}
			direction = glm::normalize(direction);

			float nextX = x + direction.x;
			float nextY = y + direction.y;
			if (nextX < 1.0f || nextY < 1.0f || nextX >= static_cast<float>(m_terrainSize - 2) || nextY >= static_cast<float>(m_terrainSize - 2))
			{
				break;
			}

			float nextHeight = SampleHeight(heightSnapshot, nextX, nextY).height;
			float heightDelta = nextHeight - sample.height;
			float capacity = std::max(-heightDelta, ctx.hydraulicSettings.particleMinSlope)
				* speed * water * ctx.hydraulicSettings.particleCapacity;

			if (heightDelta > 0.0f || sediment > capacity)
			{
				float depositAmount = heightDelta > 0.0f
					? std::min(heightDelta, sediment)
					: (sediment - capacity) * ctx.hydraulicSettings.particleDepositionStrength;
				float deposited = DepositAt(ctx, heightSnapshot, x, y, depositAmount, deltaHeight, deltaDeposit, deltaErosion);
				sediment = std::max(0.0f, sediment - deposited);
			}
			else
			{
				float erosionAmount = std::min((capacity - sediment) * ctx.hydraulicSettings.particleErosionStrength, -heightDelta);
				if (erosionAmount > 0.0f)
				{
					sediment += ErodeAt(ctx, x, y, erosionAmount, heightSnapshot, depositSnapshot, deltaHeight, deltaDeposit, deltaErosion);
				}
			}

			AddFlowAt(x, y, water * ctx.hydraulicSettings.particleFlowStrength, deltaFlow, deltaRiver, ctx);
			speed = std::sqrt(std::max(0.0f, speed * speed + (-heightDelta) * ctx.hydraulicSettings.particleGravity));
			water *= (1.0f - ctx.hydraulicSettings.particleEvaporation);
			if (water <= 0.01f)
			{
				break;
			}

			x = nextX;
			y = nextY;
		}
	}

	float ParticleErosionPass::DepositAt(const TerrainPassContext& ctx, const std::vector<float>& heightSnapshot, float x, float y, float amount, std::vector<float>& deltaHeight, std::vector<float>& deltaDeposit, std::vector<float>& deltaErosion)
	{
		if (amount <= 0.0f)
			return 0.0f;

		float radius = std::max(ctx.hydraulicSettings.particleDepositRadius, 0.5f);
		int minX = std::max(static_cast<int>(std::floor(x - radius)), 0);
		int maxX = std::min(static_cast<int>(std::ceil(x + radius)), m_terrainSize - 1);
		int minY = std::max(static_cast<int>(std::floor(y - radius)), 0);
		int maxY = std::min(static_cast<int>(std::ceil(y + radius)), m_terrainSize - 1);

		float totalWeight = 0.0f;
		for (int yy = minY; yy <= maxY; ++yy)
		{
			for (int xx = minX; xx <= maxX; ++xx)
			{
				float distance = glm::length(glm::vec2(static_cast<float>(xx) - x, static_cast<float>(yy) - y));
				float radialWeight = std::max(0.0f, radius - distance);
				float suitability = DepositionSuitability(ctx, heightSnapshot, xx, yy);
				totalWeight += radialWeight * suitability;
			}
		}
		if (totalWeight <= 0.0f)
			return 0.0f;

		float deposited = 0.0f;
		for (int yy = minY; yy <= maxY; ++yy)
		{
			for (int xx = minX; xx <= maxX; ++xx)
			{
				float distance = glm::length(glm::vec2(static_cast<float>(xx) - x, static_cast<float>(yy) - y));
				float radialWeight = std::max(0.0f, radius - distance);
				float suitability = DepositionSuitability(ctx, heightSnapshot, xx, yy);
				float weight = (radialWeight * suitability) / totalWeight;
				int idx = Index(xx, yy);
				float heightLimit = ctx.cpuBaseHeight.empty()
					? heightSnapshot[idx] + ctx.hydraulicSettings.maxDepositionHeight
					: ctx.cpuBaseHeight[idx] + ctx.hydraulicSettings.maxDepositionHeight;
				float delta = std::min(amount * weight, std::max(0.0f, heightLimit - heightSnapshot[idx]));
				if (delta <= 0.0f)
				{
					continue;
				}
				deposited += delta;
#pragma omp atomic
				deltaHeight[idx] += delta;
#pragma omp atomic
				deltaDeposit[idx] += delta;
#pragma omp atomic
				deltaErosion[idx] += delta;
			}
		}
		return deposited;
	}

	float ParticleErosionPass::ErodeAt(TerrainPassContext& ctx, float x, float y, float amount, const std::vector<float>& heightSnapshot, const std::vector<float>& depositSnapshot, std::vector<float>& deltaHeight, std::vector<float>& deltaDeposit, std::vector<float>& deltaErosion)
	{
		if (amount <= 0.0f)
		{
			return 0.0f;
		}

		float radius = std::max(ctx.hydraulicSettings.particleBrushRadius, 0.5f);
		int minX = std::max(static_cast<int>(std::floor(x - radius)), 0);
		int maxX = std::min(static_cast<int>(std::ceil(x + radius)), m_terrainSize - 1);
		int minY = std::max(static_cast<int>(std::floor(y - radius)), 0);
		int maxY = std::min(static_cast<int>(std::ceil(y + radius)), m_terrainSize - 1);

		float totalWeight = 0.0f;
		for (int yy = minY; yy <= maxY; ++yy)
		{
			for (int xx = minX; xx <= maxX; ++xx)
			{
				float distance = glm::length(glm::vec2(static_cast<float>(xx) - x, static_cast<float>(yy) - y));
				totalWeight += std::max(0.0f, radius - distance);
			}
		}
		if (totalWeight <= 0.0f)
			return 0.0f;

		float eroded = 0.0f;
		for (int yy = minY; yy <= maxY; ++yy)
		{
			for (int xx = minX; xx <= maxX; ++xx)
			{
				float distance = glm::length(glm::vec2(static_cast<float>(xx) - x, static_cast<float>(yy) - y));
				float weight = std::max(0.0f, radius - distance) / totalWeight;
				float delta = amount * weight;
				int idx = Index(xx, yy);
				float looseAvailable = depositSnapshot.empty() ? 0.0f : depositSnapshot[idx];
				float looseCut = std::min(delta * ctx.hydraulicSettings.looseSedimentErosionStrength, looseAvailable);
				float remaining = std::max(0.0f, delta - looseCut);
				float baseLimit = ctx.cpuBaseHeight.empty() ? -1000000.0f : ctx.cpuBaseHeight[idx] - ctx.hydraulicSettings.maxErosionDepth;
				float rockCut = std::min(remaining, std::max(0.0f, heightSnapshot[idx] - looseCut - baseLimit));
				float actual = looseCut + rockCut;
				if (actual <= 0.0f)
				{
					continue;
				}
				eroded += actual;
#pragma omp atomic
				deltaHeight[idx] -= actual;
				if (looseCut > 0.0f)
				{
#pragma omp atomic
					deltaDeposit[idx] -= looseCut;
				}
#pragma omp atomic
				deltaErosion[idx] -= actual;
			}
		}
		return eroded;
	}

	void ParticleErosionPass::AddFlowAt(float x, float y, float amount, std::vector<float>& deltaFlow, std::vector<float>& deltaRiver, const TerrainPassContext& ctx)
	{
		int ix = std::clamp(static_cast<int>(x), 0, m_terrainSize - 1);
		int iy = std::clamp(static_cast<int>(y), 0, m_terrainSize - 1);
		int idx = Index(ix, iy);
#pragma omp atomic
		deltaFlow[idx] += amount;
#pragma omp atomic
		deltaRiver[idx] += amount * ctx.hydraulicSettings.particleChannelStrength;
	}

	void ParticleErosionPass::FillLakes(TerrainPassContext& ctx)
	{
		size_t count = static_cast<size_t>(m_terrainSize) * static_cast<size_t>(m_terrainSize);
		int lakeSearchRadius = 5;
#pragma omp parallel for schedule(static)
		for (int i = 0; i < static_cast<int>(count); ++i)
		{
			int x = i % m_terrainSize;
			int y = i / m_terrainSize;
			if (x <= lakeSearchRadius || y <= lakeSearchRadius || x >= m_terrainSize - 1 - lakeSearchRadius || y >= m_terrainSize - 1 - lakeSearchRadius)
			{
				ctx.cpuWater[i] = 0.0f;
				continue;
			}

			float h = ctx.cpuHeight[i];
			float lowestRim = 1000000.0f;
			float localAverage = 0.0f;
			int localCount = 0;

			for (int yy = y - lakeSearchRadius; yy <= y + lakeSearchRadius; ++yy)
			{
				for (int xx = x - lakeSearchRadius; xx <= x + lakeSearchRadius; ++xx)
				{
					float sample = ctx.cpuHeight[Index(xx, yy)];
					bool onRim = xx == x - lakeSearchRadius || xx == x + lakeSearchRadius || yy == y - lakeSearchRadius || yy == y + lakeSearchRadius;
					if (onRim)
					{
						lowestRim = std::min(lowestRim, sample);
					}
					else
					{
						localAverage += sample;
						localCount++;
					}
				}
			}

			localAverage = localCount > 0 ? localAverage / static_cast<float>(localCount) : h;
			float basinDepth = std::max(0.0f, std::min(lowestRim, localAverage) - h);
			float flow = ctx.cpuFlowAccumulation[i];
			float flowGate = SmoothStep(ctx.hydraulicSettings.lakeMinFlow, ctx.hydraulicSettings.lakeMinFlow + 0.08f, flow);
			float puddleGate = SmoothStep(ctx.hydraulicSettings.lakeMinFlow * 0.35f, ctx.hydraulicSettings.lakeMinFlow, flow);
			float riverWater = SmoothStep(ctx.hydraulicSettings.lakeMinFlow * 0.5f, ctx.hydraulicSettings.lakeMinFlow + 0.12f, flow) * 0.045f;
			float basinWater = basinDepth * flowGate;
			float puddleWater = std::max(0.0f, localAverage - h) * 0.18f * puddleGate;
			ctx.cpuWater[i] = std::clamp(std::max(basinWater, puddleWater) + riverWater, 0.0f, ctx.hydraulicSettings.lakeMaxDepth);
		}
	}

	void ParticleErosionPass::SmoothLooseSediment(TerrainPassContext& ctx)
	{
		if (ctx.cpuDeposit.empty() || ctx.cpuHeight.empty())
		{
			return;
		}

		float smoothing = std::clamp(ctx.hydraulicSettings.looseSedimentSmoothing, 0.0f, 0.5f);
		if (smoothing <= 0.0f)
		{
			return;
		}

		std::vector<float> depositSnapshot = ctx.cpuDeposit;
		size_t count = static_cast<size_t>(m_terrainSize) * static_cast<size_t>(m_terrainSize);
#pragma omp parallel for schedule(static)
		for (int i = 0; i < static_cast<int>(count); ++i)
		{
			int x = i % m_terrainSize;
			int y = i / m_terrainSize;
			if (x <= 0 || y <= 0 || x >= m_terrainSize - 1 || y >= m_terrainSize - 1)
			{
				continue;
			}

			float center = depositSnapshot[i];
			if (center <= 0.0f)
			{
				continue;
			}

			float neighborAverage = (
				depositSnapshot[Index(x - 1, y)] +
				depositSnapshot[Index(x + 1, y)] +
				depositSnapshot[Index(x, y - 1)] +
				depositSnapshot[Index(x, y + 1)]
				) * 0.25f;
			float smoothed = glm::mix(center, neighborAverage, smoothing);
			float delta = smoothed - center;
			ctx.cpuDeposit[i] = std::max(0.0f, smoothed);
			ctx.cpuHeight[i] += delta;
			ctx.cpuErosionDelta[i] += delta;
		}
	}

	float ParticleErosionPass::DepositionSuitability(const TerrainPassContext& ctx, const std::vector<float>& heightSnapshot, int x, int y) const
	{
		if (x <= 0 || y <= 0 || x >= m_terrainSize - 1 || y >= m_terrainSize - 1)
		{
			return 0.0f;
		}

		int idx = Index(x, y);
		HeightSample sample = SampleHeight(heightSnapshot, static_cast<float>(x), static_cast<float>(y));
		float slope = glm::length(sample.gradient);
		float flatFactor = 1.0f - SmoothStep(
			ctx.hydraulicSettings.particleDepositSlopeStart,
			ctx.hydraulicSettings.particleDepositSlopeEnd,
			slope
		);

		float center = heightSnapshot[idx];
		float cardinalAverage = (
			heightSnapshot[Index(x - 1, y)] +
			heightSnapshot[Index(x + 1, y)] +
			heightSnapshot[Index(x, y - 1)] +
			heightSnapshot[Index(x, y + 1)]
			) * 0.25f;
		float diagonalAverage = (
			heightSnapshot[Index(x - 1, y - 1)] +
			heightSnapshot[Index(x + 1, y - 1)] +
			heightSnapshot[Index(x - 1, y + 1)] +
			heightSnapshot[Index(x + 1, y + 1)]
			) * 0.25f;
		float localBasin = std::max(0.0f, ((cardinalAverage * 0.7f + diagonalAverage * 0.3f) - center));
		float valleyFactor = SmoothStep(0.0025f, 0.055f, localBasin) * ctx.hydraulicSettings.particleDepositValleyBoost;
		float shelfFactor = flatFactor * ctx.hydraulicSettings.particleDepositShelfBoost;

		float suitability = std::max(valleyFactor, shelfFactor) * flatFactor;
		if (suitability < ctx.hydraulicSettings.particleDepositMinSuitability)
		{
			return 0.0f;
		}
		return std::clamp(suitability, 0.0f, 1.0f);
	}

	ParticleErosionPass::HeightSample ParticleErosionPass::SampleHeight(const std::vector<float>& height, float x, float y) const
	{
		HeightSample sample;
		sample.height = SampleValue(height, x, y);
		float left = SampleValue(height, x - 1.0f, y);
		float right = SampleValue(height, x + 1.0f, y);
		float down = SampleValue(height, x, y - 1.0f);
		float up = SampleValue(height, x, y + 1.0f);
		sample.gradient = glm::vec2((right - left) * 0.5f, (up - down) * 0.5f);
		return sample;
	}

	float ParticleErosionPass::SampleValue(const std::vector<float>& values, float x, float y) const
	{
		if (values.empty())
			return 0.0f;

		x = std::clamp(x, 0.0f, static_cast<float>(m_terrainSize - 1));
		y = std::clamp(y, 0.0f, static_cast<float>(m_terrainSize - 1));
		int x0 = static_cast<int>(std::floor(x));
		int y0 = static_cast<int>(std::floor(y));
		int x1 = std::min(x0 + 1, m_terrainSize - 1);
		int y1 = std::min(y0 + 1, m_terrainSize - 1);
		float tx = x - static_cast<float>(x0);
		float ty = y - static_cast<float>(y0);

		float a = glm::mix(values[Index(x0, y0)], values[Index(x1, y0)], tx);
		float b = glm::mix(values[Index(x0, y1)], values[Index(x1, y1)], tx);
		return glm::mix(a, b, ty);
	}

	int ParticleErosionPass::Index(int x, int y) const
	{
		return y * m_terrainSize + x;
	}
}
