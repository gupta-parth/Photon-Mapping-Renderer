#pragma once

#include "geometry.h"
#include "rng.h"

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
	PointLightPhotonSample& sample, RNG &rng) {
	if (pointLights.empty()) {
		sample = PointLightPhotonSample();
		return false;
	}

	const std::size_t lightCount = pointLights.size();
	const std::size_t sampledIndex =
		static_cast<std::size_t>(rng.next1D() * static_cast<float>(lightCount));
	sample.lightIndex = std::min(sampledIndex, lightCount - 1);
	sample.lightChoosePdf = 1.0f / static_cast<float>(lightCount);

	const PointLightSource& light = *pointLights[sample.lightIndex];
	const float u1 = rng.next1D();
	const float u2 = rng.next1D();

	const float z = 1.0f - 2.0f * u1;
	const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
	const float phi = 2.0f * PI * u2;

	sample.directionPdf = 1.0f / (4.0f * PI);

	const float3 direction(
		r * std::cos(phi),
		r * std::sin(phi),
		z
	);
	sample.ray = Ray(light.position, direction);

	// wattage is total power Phi, so an isotropic point light has
	// intensity I = Phi / (4 * PI). The later radiance estimate divides by
	// the number of emitted photons, so numPhotons is not included here.
	const float3 intensity = light.wattage / (4.0f * PI);
	sample.flux =
		intensity / (sample.lightChoosePdf * sample.directionPdf);
	return true;
}
