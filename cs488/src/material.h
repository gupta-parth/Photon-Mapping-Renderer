#pragma once

#include "config.h"
#include "rng.h"

// ====== implement it in A2, if you want ======
enum enumMaterialType {
	MAT_LAMBERTIAN,
	MAT_METAL,
	MAT_GLASS
};


// Inspired from PBRT's transport mode
// Camera is the radiance mode and Importance is the light mode
enum class TransportMode {
	Camera,
	Light
};

struct BSDFSample {
	float3 wi = float3(0.0f);
    float3 f = float3(0.0f);
    float pdf = 0.0f;
    bool valid = false;
};



static float fresnal(float cosTheta, float etaFrom, float etaTo) {
	const float ratio =
        (etaFrom - etaTo) / (etaFrom + etaTo);

    const float f0 = ratio * ratio;

    cosTheta = std::min(
        1.0f,
        std::max(0.0f, std::abs(cosTheta))
    );

    const float x = 1.0f - cosTheta;
    const float x5 = x * x * x * x * x;

    return f0 + (1.0f - f0) * x5;
}




class Material {
public:
	std::string name;

	enumMaterialType type = MAT_LAMBERTIAN;
	float eta = 1.0f;
	float glossiness = 1.0f;

	float3 Ka = float3(0.0f);
	float3 Kd = float3(0.9f);
	float3 Ks = float3(0.0f);
	float3 Ke = float3(0.0f);
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

			// Check if either direction is below the surface
			const float3 normal = normalize(n);
			const float3 nwi = normalize(wi);
			const float3 nwo = normalize(wo);
			if (dot(normal, nwi) <= 0.0f || dot(normal, nwo) <= 0.0f) {
				return float3(0.0f);
			}
			brdfValue = Kd / PI;
		} else if (type == MAT_METAL) {
			return float3(0.0f);
		} else if (type == MAT_GLASS) {
			return float3(0.0f);
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
			pdfValue = 1.0f;
		} else if (type == MAT_GLASS) {
			// empty
		}
		return pdfValue;
	}

	float3 sampler(const float3& wGiven, const float3& n, float& pdfValue, RNG &rng) const {
		// sample a vector and record its probability density as pdfValue

		// we use cosine weighted importance sampling for Lambertian because it will be useful for 
		// area lights later on
		float3 smp = float3(0.0f);
		if (type == MAT_LAMBERTIAN) {
			float u1 = rng.next1D();
			float u2 = rng.next1D();

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
			float3 normal = normalize(n);
			smp = normalize(-2 * dot(-wGiven, normal) * normal - wGiven);
		} else if (type == MAT_GLASS) {
			// empty
		}

		pdfValue = PDF(wGiven, smp, n);
		return smp;
	}

	BSDFSample sampleBSDF(const float3 &wo, const float3 &normal, TransportMode mode, RNG &rng) const {
		BSDFSample result;
		if (type == MAT_LAMBERTIAN) {
			float u1 = rng.next1D();
			float u2 = rng.next1D();

			float r = sqrtf(u1);
			float phi = 2.0f * PI * u2;

			float x = r * cosf(phi);
			float y = r * sinf(phi);
			float z = sqrtf(std::max(0.0f, 1.0f - u1));

			float3 n = normalize(normal);
			float3 helper = fabsf(n.x) > 0.9f ? float3(0, 1, 0) : float3(1, 0, 0);
			float3 tangent = normalize(cross(helper, n));
			float3 bitangent = cross(n, tangent);

			result.wi = normalize(x * tangent + y * bitangent + z * n);
			const float cosTheta = std::max(0.0f, dot(result.wi, n));
			if (cosTheta == 0.0f) {
				return result;
			}
			result.f = Kd / PI;
			result.pdf = cosTheta / PI;
			result.valid = true;
			return result;
		}
		if (type == MAT_METAL) {
			const float3 n = normalize(normal);
			result.wi = normalize(-wo + 2.0f * dot(wo, n) * n);
			result.pdf = 1.0f;
			result.f = Ks / (std::abs(dot(result.wi, n)));
			result.valid = true;
			return result;
		}
		if (type == MAT_GLASS) {
			float etaFrom = 1.0f;
			float etaTo = eta;
			float3 n = normalize(normal);
			if (dot(wo, normal) < 0.0f) {
				// we are exiting glass 
				n = -n;
				etaFrom = eta;
				etaTo = 1.0f;
			}
			float fresnal_ref = fresnal(dot(wo, normal), etaFrom, etaTo);

			// Decide whether to reflect or refract
			if (rng.next1D() < fresnal_ref) {
				result.wi = normalize(-wo + 2.0f * dot(wo, n) * n);
				result.pdf = 1.0f;
				result.f = Ks / (std::abs(dot(result.wi, n)));
				result.valid = true;
				return result;
			}
			else {
				float3 tangent = (-etaFrom/etaTo) * (wo - dot(wo, n) * n);
				if (length2(tangent) > 1.0f) {
					// total internal reflection
					result.wi = normalize(-wo + 2.0f * dot(wo, n) * n);
					result.pdf = 1.0f;
					result.f = Ks / (std::abs(dot(result.wi, n)));
					result.valid = true;
					return result;
				}
				float3 normalComponent = -std::sqrt(std::max(0.0f, 1.0f - length2(tangent))) * n;
				result.wi = tangent + normalComponent;
				result.pdf = 1.0f;
				result.valid = true;
				if (mode == TransportMode::Camera) {
					result.f = (etaFrom * etaFrom) / (etaTo * etaTo) * (Ks / (std::abs(dot(result.wi, n))));
				}
				else {
					result.f = Ks / (std::abs(dot(result.wi, n)));
				}
				return result;
			}
		}
		return result; 
	}
};

