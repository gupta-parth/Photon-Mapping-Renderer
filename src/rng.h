#pragma once

#include <random>
#include <cstdint>


class RNG {
private:
    std::mt19937 engine;
    std::uniform_real_distribution<float> uniform{0.0f, 1.0f};

public:
    RNG(
        uint64_t baseSeed,
        uint32_t iteration,
        uint64_t pathIndex)
    {
        std::seed_seq sequence{
            static_cast<uint32_t>(baseSeed),
            static_cast<uint32_t>(baseSeed >> 32),
            iteration,
            static_cast<uint32_t>(pathIndex),
            static_cast<uint32_t>(pathIndex >> 32)
        };

        engine.seed(sequence);
    }

    float next1D() {
        return uniform(engine);
    }
};