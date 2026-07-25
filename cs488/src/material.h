#pragma once

#include "config.h"
#include "random.h"

// ====== implement it in A2, if you want ======
enum enumMaterialType {
	MAT_LAMBERTIAN,
	MAT_METAL,
	MAT_GLASS
};
class Material {
public:
	std::string name;

	enumMaterialType type = MAT_LAMBERTIAN;
	float eta = 1.0f;
	float glossiness = 1.0f;

	float3 Ka = float3(0.0f);
	float3 Kd = float3(0.9f);
	float3 Ks = float3(0.0f);
	float Ns = 0.0;

	// support 8-bit texture
	bool isTextured = false;
	unsigned char* texture = nullptr;
	int textureWidth = 0;
	int textureHeight = 0;

	Material() {};
	virtual ~Material() {};

	void setReflectance(const float3& c) {
		if (type == MAT_LAMBERTIAN) {
			Kd = c;
		} else if (type == MAT_METAL) {
			// empty
		} else if (type == MAT_GLASS) {
			// empty
		}
	}

	float3 fetchTexture(const float2& tex) const {
		// repeating
		int x = int(tex.x * textureWidth) % textureWidth;
		int y = int(tex.y * textureHeight) % textureHeight;
		if (x < 0) x += textureWidth;
		if (y < 0) y += textureHeight;

		int pix = (x + y * textureWidth) * 3;
		const unsigned char r = texture[pix + 0];
		const unsigned char g = texture[pix + 1];
		const unsigned char b = texture[pix + 2];
		return float3(r, g, b) / 255.0f;
	}

	float3 BRDF(const float3& wi, const float3& wo, const float3& n) const {
		float3 brdfValue = float3(0.0f);
		if (type == MAT_LAMBERTIAN) {
			// BRDF
			brdfValue = Kd / PI;
		} else if (type == MAT_METAL) {
			// empty
		} else if (type == MAT_GLASS) {
			// empty
		}
		return brdfValue;
	};

	float PDF(const float3& wGiven, const float3& wSample, const float3& n) const {
		// probability density function for a given direction and a given sample
		// it has to be consistent with the sampler
		float pdfValue = 0.0f;
		if (type == MAT_LAMBERTIAN) {
			float3 normal = normalize(n);
			float3 sample = normalize(wSample);
			float cosTheta = dot(normal, sample);
			pdfValue = cosTheta > 0.0f ? cosTheta / PI : 0.0f;
		} else if (type == MAT_METAL) {
			// empty
		} else if (type == MAT_GLASS) {
			// empty
		}
		return pdfValue;
	}

	float3 sampler(const float3& wGiven, const float3& n, float& pdfValue) const {
		// sample a vector and record its probability density as pdfValue

		// we use cosine weighted importance sampling for Lambertian because it will be useful for 
		// area lights later on
		float3 smp = float3(0.0f);
		if (type == MAT_LAMBERTIAN) {
			float u1 = PCG32::rand();
			float u2 = PCG32::rand();

			float r = sqrtf(u1);
			float phi = 2.0f * PI * u2;

			float x = r * cosf(phi);
			float y = r * sinf(phi);
			float z = sqrtf(std::max(0.0f, 1.0f - u1));

			float3 normal = normalize(n);
			float3 helper = fabsf(normal.x) > 0.9f ? float3(0, 1, 0) : float3(1, 0, 0);
			float3 tangent = normalize(cross(helper, normal));
			float3 bitangent = cross(normal, tangent);

			smp = normalize(x * tangent + y * bitangent + z * normal);
		} else if (type == MAT_METAL) {
			// empty
		} else if (type == MAT_GLASS) {
			// empty
		}

		pdfValue = PDF(wGiven, smp, n);
		return smp;
	}
};

