#pragma once


#include "scene.h"

// fill in the missing parts
static float3 shade(const HitInfo& hit, const float3& viewDir, const int level) {
	// prevent runaway recursion from perfect reflections
	const int MAX_SHADE_DEPTH = 200;
	if (level > MAX_SHADE_DEPTH) {
		std::cout << "Max shade depth reached." << std::endl;
		return float3(1,0,0);
	}

	if (hit.material->type == MAT_LAMBERTIAN) {
		// you may want to add shadow ray tracing here in A2
		float3 L = float3(0.0f);
		float3 brdf, irradiance;

		
		// loop over all of the point light sources
		for (int i = 0; i < globalScene.pointLightSources.size(); i++) {
			float3 l = globalScene.pointLightSources[i]->position - hit.P;
			bool inShadow = false;
			// the inverse-squared falloff
			const float falloff = length2(l);

			// normalize the light direction
			l /= sqrtf(falloff);

			const Ray shadowRay = Ray(float3(hit.P + hit.geometricNormal * Epsilon), l);
			HitInfo shadowHitInfo;
			if (globalScene.intersect(shadowHitInfo, shadowRay, 0.00001, sqrtf(falloff)))
				inShadow = true;

			if (inShadow == false) {
				irradiance = float(std::max(0.0f, dot(hit.N, l)) / (4.0 * PI * falloff)) * globalScene.pointLightSources[i]->wattage;
				brdf = hit.material->BRDF(l, viewDir, hit.N);

				if (hit.material->isTextured) {
					brdf *= hit.material->fetchTexture(hit.T);
				}
				//return brdf * PI; //debug output
				L += irradiance * brdf;
			}
		}
		return L;

	}
	else if (hit.material->type == MAT_METAL) {
		float3 reflectedDir = normalize(-2 * dot(-viewDir, hit.N) * hit.N  - viewDir);
		Ray reflectedRay = Ray(hit.P + hit.geometricNormal * Epsilon, reflectedDir);
		HitInfo reflectedHitInfo;
		if (globalScene.intersect(reflectedHitInfo, reflectedRay, 0.00001)) {
			return 0.9f * shade(reflectedHitInfo, -reflectedDir, level + 1);
		} else {
			return float3(0.0f);
		}
	return hit.material->Kd;

	} else if (hit.material->type == MAT_GLASS) {
		float3 I = normalize(-viewDir);
		float3 N = normalize(hit.N);
		float etai = 1.0f;
		float etat = 1.5f;
		float NdotI = std::max(-1.0f, std::min(1.0f, dot(N, I)));
		if (NdotI < 0) {
			NdotI = -NdotI;
		}
		else {
			std::swap(etai, etat);
			N *= -1;
		}
		float etaRatio = etai / etat;
		float temp = 1 - (etaRatio * etaRatio) * (1 - NdotI * NdotI);
		if (temp < 0.0f) {
			// total internal reflection
			float3 reflectedDir = normalize(-2 * dot(-viewDir, N) * N - viewDir);
			Ray reflectedRay = Ray(hit.P + hit.geometricNormal * Epsilon, reflectedDir);
			HitInfo reflectedHitInfo;
			if (globalScene.intersect(reflectedHitInfo, reflectedRay, 0.00001)) {
				return shade(reflectedHitInfo, -reflectedDir, level + 1);
			} else {
				return float3(0.0f);
			}
		}

		float3 refractDir = normalize(etaRatio * I + (etaRatio * NdotI - sqrtf(temp)) * N);
		Ray refractedRay = Ray(hit.P + (dot(refractDir, hit.geometricNormal) > 0.0f ? hit.geometricNormal
			: -hit.geometricNormal * Epsilon), refractDir);
		HitInfo refractedHitInfo;
		if (globalScene.intersect(refractedHitInfo, refractedRay, 0.00001)) {
			return shade(refractedHitInfo, -refractDir, level + 1);
		}
		else {
			return float3(0.0f);
		}
	return hit.material->Ks;

	} else {
		std::cout << "error" << std::endl;
		// something went wrong - make it apparent that it is an error
		return float3(100.0f, 0.0f, 100.0f);
	}
}







