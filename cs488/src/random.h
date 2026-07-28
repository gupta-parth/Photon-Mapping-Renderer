#pragma once

#include "config.h"

#include <cmath>
#include <stdint.h>
namespace PCG32 {
	static uint64_t mcg_state = 0xcafef00dd15ea5e5u;	// must be odd
	static uint64_t const multiplier = 6364136223846793005u;
	uint32_t pcg32_fast(void) {
		uint64_t x = mcg_state;
		const unsigned count = (unsigned)(x >> 61);
		mcg_state = x * multiplier;
		x ^= x >> 22;
		return (uint32_t)(x >> (22 + count));
	}


	float rand() {
		return float(double(pcg32_fast()) / 4294967296.0);
	}

	void seed(uint64_t value) {
        // The MCG state must be odd.
        mcg_state = (value << 1u) | 1u;

        // Advance once to mix the initial state.
        pcg32_fast();
    }

	float3 sampleUniformSphere(float& pdf) {
		const float u1 = rand();
		const float u2 = rand();

		const float z = 1.0f - 2.0f * u1;
		const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
		const float phi = 2.0f * PI * u2;

		pdf = 1.0f / (4.0f * PI);
		return float3(r * std::cos(phi), r * std::sin(phi), z);
	}
}




