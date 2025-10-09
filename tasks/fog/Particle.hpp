#pragma once
#include <glm/glm.hpp>


struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float lifetime;
    float maxLifetime;
    float size;

    Particle()
        : position(0.0f)
        , velocity(0.0f)
        , color(1.0f)
        , lifetime(0.0f)
        , maxLifetime(1.0f)
        , size(1.0f) {}
};

