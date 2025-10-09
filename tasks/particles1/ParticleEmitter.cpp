#include "ParticleEmitter.hpp"
#include <algorithm>
#include <cmath>

ParticleEmitter::ParticleEmitter(const EmitterParams& params)
    : params(params)
    , randomEngine(std::random_device{}()) {
}

void ParticleEmitter::update(float deltaTime, const glm::vec3& cameraPos) {
    cameraPosition = cameraPos;
    distanceToCamera = glm::length(params.position - cameraPosition);

    spawnAccumulator += deltaTime;
    float spawnInterval = 1.0f / params.spawnRate;
    while (spawnAccumulator >= spawnInterval) {
        spawnParticle();
        spawnAccumulator -= spawnInterval;
    }
    updateParticles(deltaTime);
    sortParticles();
}

void ParticleEmitter::spawnParticle() {
    Particle particle;
    particle.position = params.position;
    particle.color = params.startColor;
    particle.lifetime = params.particleLifetime;
    particle.maxLifetime = params.particleLifetime;
    particle.size = params.startSize;

    glm::vec3 randomDir{
        dist(randomEngine),
        std::abs(dist(randomEngine)),
        dist(randomEngine)
    };
    randomDir = glm::normalize(randomDir);
    particle.velocity = randomDir * params.initialSpeed;
    particle.velocity += glm::vec3{
        dist(randomEngine) * params.velocityVariation.x,
        dist(randomEngine) * params.velocityVariation.y,
        dist(randomEngine) * params.velocityVariation.z
    };

    particles.push_back(particle);
}

void ParticleEmitter::updateParticles(float deltaTime) {
    const glm::vec3 gravity(0.0f, -2.0f, 0.0f);

    for (auto it = particles.begin(); it != particles.end();) {
        Particle& particle = *it;

        particle.lifetime -= deltaTime;
        if (particle.lifetime <= 0.0f) {
            it = particles.erase(it);
            continue;
        }
        if (params.gravityEnabled) {
            particle.velocity += gravity * deltaTime;
        }
        particle.position += particle.velocity * deltaTime;

        float lifeRatio = particle.lifetime / particle.maxLifetime;
        particle.color = glm::mix(params.endColor, params.startColor, lifeRatio);
        particle.size = glm::mix(params.endSize, params.startSize, lifeRatio);

        ++it;
    }
}

void ParticleEmitter::sortParticles() {
    std::sort(particles.begin(), particles.end(),
        [this](const Particle& a, const Particle& b) {
            float distA = glm::length(a.position - cameraPosition);
            float distB = glm::length(b.position - cameraPosition);
            return distA > distB;
        });
}

void ParticleEmitter::setParams(const EmitterParams& newParams) {
    params = newParams;
}

