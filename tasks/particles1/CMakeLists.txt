
add_executable(particles1
  main.cpp
  App.cpp
  ParticleEmitter.cpp
  ParticleSystem.cpp
)

target_link_libraries(particles1
  PRIVATE glfw etna glm::glm wsi gui  render_utils scene)

target_add_shaders(particles1
  shaders/toy.frag
  shaders/toy.vert
  shaders/texture.frag
  shaders/particle.frag
  shaders/particle.vert
)

