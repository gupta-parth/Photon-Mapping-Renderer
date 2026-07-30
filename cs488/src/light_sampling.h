#pragma once

#include "geometry.h"
#include "rng.h"

#include <algorithm>
#include <cstddef>
#include <vector>

struct LightPhotonSample {
	Ray ray;
	float3 flux = float3(0.0f);
	std::size_t lightIndex = 0;
	float lightChoosePdf = 0.0f;
	float directionPdf = 0.0f;
};

inline bool samplePointLightPhoton(
	const std::vector<PointLightSource*>& pointLights,
	LightPhotonSample& sample, RNG &rng) {
	if (pointLights.empty()) {
		sample = LightPhotonSample();
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

inline bool sampleAreaLight(const std::vector<AreaLight> &areaLights, float &totalArea,
	LightPhotonSample &sample, RNG &rng) {
	if (areaLights.empty()) return false;

	// Sample the emissive triangle based on area;
	float cdf = rng.next1D() * totalArea;

	// we get a pointer to the light;
	auto selected = std::upper_bound(areaLights.begin(), areaLights.end(), cdf, 
	[](float value, const AreaLight &light) {
		return value < light.cummulativeArea;
	});
	if (selected == areaLights.end()) return false;
	auto selectedIndex = std::distance(areaLights.begin(), selected);
	const AreaLight &light = *selected;
	sample.lightIndex = selectedIndex;
	sample.lightChoosePdf = light.area / totalArea;

	// Now we sample a uniform point on the light source;
	const Triangle &tri = *light.tri;
	const float u1 = rng.next1D();
    const float u2 = rng.next1D();
    const float sqrtU1 = std::sqrt(u1);

    const float b0 = 1.0f - sqrtU1;
    const float b1 = sqrtU1 * (1.0f - u2);
    const float b2 = sqrtU1 * u2;

    const float3 position = b0 * tri.positions[0] + b1 * tri.positions[1] +
        b2 * tri.positions[2];

	
	const float3 e1 = tri.positions[1] - tri.positions[0];
    const float3 e2 = tri.positions[2] - tri.positions[0];
    float3 normal = cross(e1, e2);
    const float normalLength = length(normal);

    if (!(normalLength > 0.0f)) {
        sample = LightPhotonSample();
        return false;
    }
	normal = normalize(normal);


	// We do cosine-weighted hemisphere sampling for direction (same as material.h lambertian sampling)
	const float u3 = rng.next1D();
    const float u4 = rng.next1D();

    const float r = std::sqrt(u3);
    const float phi = 2.0f * PI * u4;

    const float localX = r * std::cos(phi);
    const float localY = r * std::sin(phi);
    const float localZ =
        std::sqrt(std::max(0.0f, 1.0f - u3));

    const float3 helper =
        std::abs(normal.x) > 0.9f
        ? float3(0.0f, 1.0f, 0.0f)
        : float3(1.0f, 0.0f, 0.0f);

    const float3 tangent = normalize(cross(helper, normal));
    const float3 bitangent = cross(normal, tangent);

    const float3 direction = normalize(
        localX * tangent +
        localY * bitangent +
        localZ * normal);

    const float cosTheta =
        std::max(0.0f, dot(normal, direction));

    sample.directionPdf = cosTheta / PI;
	if (!(sample.directionPdf > 0.0f)) {
        sample = LightPhotonSample();
        return false;
    }
	sample.ray = Ray(
        position + Epsilon * normal,
        direction);

	
	 // General expression:
    //
    // Ke * cosTheta /
    // (triangleChoosePdf * positionPdf * directionPdf)
    //
    // With area-weighted triangle selection and cosine-weighted
    // direction sampling, this simplifies exactly to
	sample.flux = PI * light.area * light.emission / sample.lightChoosePdf;
	return true;
}
