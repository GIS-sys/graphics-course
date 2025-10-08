#pragma once
#include "Particle.hpp"
#include <vector>
#include <memory>
#include <random>


class ParticleEmitter {
public:
    struct EmitterParams {
        glm::vec3 position;
        float spawnRate;
        float particleLifetime;
        float initialSpeed;
        glm::vec3 velocityVariation;
        float startSize;
        float endSize;
        glm::vec4 startColor;
        glm::vec4 endColor;
        bool gravityEnabled;

        void reset() {
          position = glm::vec3{-20.0f, 3.0f, 5.0f};
          spawnRate = 3.0f;
          particleLifetime = 3.0f;
          initialSpeed = 6.0f;
          velocityVariation = glm::vec3{0.5f, 0.5f, 0.5f};
          startSize = 0.75f;
          endSize = 0.4f;
          startColor = glm::vec4{1.0f, 0.2f, 0.2f, 1.0f};
          endColor = glm::vec4{0.2f, 1.0f, 0.2f, 0.0f};
          gravityEnabled = true;
        }

        void resetN(int N) {
            reset();
            *this = as(N);
        }

        EmitterParams as(int N) {
            EmitterParams other(*this);
            other.position += glm::vec3(20.0f, 0.0f, 0.0f) * float(N);
            return other;
        }
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

