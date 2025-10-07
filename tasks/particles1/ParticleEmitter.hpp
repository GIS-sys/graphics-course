#pragma once
#include "Particle.hpp"
#include <vector>
#include <memory>
#include <random>


class ParticleEmitter {
public:
    struct EmitterParams {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        float spawnRate = 10.0f;
        float particleLifetime = 2.0f;
        float initialSpeed = 2.0f;
        glm::vec4 startColor{1.0f, 0.5f, 0.2f, 1.0f};
        glm::vec4 endColor{0.2f, 0.8f, 1.0f, 0.0f};
        float startSize = 0.1f;
        float endSize = 0.05f;
        glm::vec3 velocityVariation{0.5f, 0.5f, 0.5f};
        bool gravityEnabled = true;
    };

    ParticleEmitter(const EmitterParams& params);
    void update(float deltaTime, const glm::vec3& cameraPosition);
    void setParams(const EmitterParams& params);
    const EmitterParams& getParams() const { return params; }
    const std::vector<Particle>& getParticles() const { return particles; }
    float getDistanceToCamera() const { return distanceToCamera; }

private:
    void spawnParticle();
    void updateParticles(float deltaTime);
    void sortParticles();

    EmitterParams params;
    std::vector<Particle> particles;
    std::mt19937 randomEngine;
    std::uniform_real_distribution<float> dist{-1.0f, 1.0f};

    float spawnAccumulator = 0.0f;
    float distanceToCamera = 0.0f;
    glm::vec3 cameraPosition{0.0f};
};

