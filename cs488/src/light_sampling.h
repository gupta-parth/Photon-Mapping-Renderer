#pragma once

#include "geometry.h"
#include "random.h"

#include <algorithm>
#include <cstddef>
#include <vector>

struct PointLightPhotonSample {
	Ray ray;
	float3 flux = float3(0.0f);
	std::size_t lightIndex = 0;
	float lightChoosePdf = 0.0f;
	float directionPdf = 0.0f;
};

inline bool samplePointLightPhoton(
	const std::vector<PointLightSource*>& pointLights,
	PointLightPhotonSample& sample) {
	if (pointLights.empty()) {
		sample = PointLightPhotonSample();
		return false;
	}

	const std::size_t lightCount = pointLights.size();
	const std::size_t sampledIndex =
		static_cast<std::size_t>(PCG32::rand() * static_cast<float>(lightCount));
	sample.lightIndex = std::min(sampledIndex, lightCount - 1);
	sample.lightChoosePdf = 1.0f / static_cast<float>(lightCount);

	const PointLightSource& light = *pointLights[sample.lightIndex];
	const float3 direction = PCG32::sampleUniformSphere(sample.directionPdf);
	sample.ray = Ray(light.position, direction);

	// wattage is total power Phi, so an isotropic point light has
	// intensity I = Phi / (4 * PI). The later radiance estimate divides by
	// the number of emitted photons, so numPhotons is not included here.
	const float3 intensity = light.wattage / (4.0f * PI);
	sample.flux =
		intensity / (sample.lightChoosePdf * sample.directionPdf);
	return true;
}
