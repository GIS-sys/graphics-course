#pragma once
#include "ParticleEmitter.hpp"
#include <vector>
#include <memory>


class ParticleSystem {
public:
    ParticleSystem();
    
    void update(float deltaTime, const glm::vec3& cameraPosition);
    void render();
    void addEmitter(ParticleEmitter::EmitterParams params);
    void clearEmitters();
    
    std::vector<std::unique_ptr<ParticleEmitter>>& getEmitters() { return emitters; }
    const std::vector<std::unique_ptr<ParticleEmitter>>& getEmitters() const { return emitters; }

private:
    void sortEmitters(const glm::vec3& cameraPosition);

    std::vector<std::unique_ptr<ParticleEmitter>> emitters;
};

