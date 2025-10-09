#include "ParticleSystem.hpp"
#include <algorithm>
#include <exception>


ParticleSystem::ParticleSystem() {
}

void ParticleSystem::update(float deltaTime, const glm::vec3& cameraPosition) {
    for (auto& emitter : emitters) {
        emitter->update(deltaTime, cameraPosition);
    }
    sortEmitters(cameraPosition);
}

void ParticleSystem::addEmitter(ParticleEmitter::EmitterParams params) {
    emitters.push_back(std::make_unique<ParticleEmitter>(params));
}

void ParticleSystem::clearEmitters() {
    emitters.clear();
}

void ParticleSystem::sortEmitters(const glm::vec3& cameraPosition) {
    std::sort(emitters.begin(), emitters.end(),
        [&cameraPosition](const std::unique_ptr<ParticleEmitter>& a, 
                          const std::unique_ptr<ParticleEmitter>& b) {
            return a->getDistanceToCamera() > b->getDistanceToCamera();
        }
    );
}

void render() {
    throw std::exception();
}

